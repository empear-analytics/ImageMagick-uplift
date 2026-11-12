/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%                  W   W  IIIII  DDDD    GGGG  EEEEE  TTTTT                   %
%                  W   W    I    D   D  G      E        T                     %
%                  W W W    I    D   D  G  GG  EEE      T                     %
%                  WW WW    I    D   D  G   G  E        T                     %
%                  W   W  IIIII  DDDD    GGGG  EEEEE    T                     %
%                                                                             %
%                                                                             %
%                   MagickCore X11 User Interface Methods                     %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                              September 1993                                 %
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
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/string_.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/xwindow-private.h"
#include "MagickCore/widget.h"
#include "MagickCore/widget-private.h"

#if defined(MAGICKCORE_X11_DELEGATE)

/*
  Define declarations.
*/
#define AreaIsActive(matte_info,position)  ( \
  ((position.y >= (matte_info.y-(int) matte_info.bevel_width)) &&  \
   (position.y < (matte_info.y+(int) matte_info.height+(int) matte_info.bevel_width))) \
   ? MagickTrue : MagickFalse)
#define Extent(s)  ((int) strlen(s))
#define MatteIsActive(matte_info,position)  ( \
  ((position.x >= (matte_info.x-(int) matte_info.bevel_width)) && \
   (position.y >= (matte_info.y-(int) matte_info.bevel_width)) &&  \
   (position.x < (matte_info.x+(int) matte_info.width+(int) matte_info.bevel_width)) &&  \
   (position.y < (matte_info.y+(int) matte_info.height+(int) matte_info.bevel_width))) \
   ? MagickTrue : MagickFalse)
#define MaxTextWidth  ((unsigned int) (255*XTextWidth(font_info,"_",1)))
#define MinTextWidth  ((unsigned int) (26*XTextWidth(font_info,"_",1)))
#define QuantumMargin   MagickMax(font_info->max_bounds.width,12)
#define WidgetTextWidth(font_info,text)  \
  ((unsigned int) XTextWidth(font_info,text,Extent(text)))
#define WindowIsActive(window_info,position)  ( \
  ((position.x >= 0) && (position.y >= 0) &&  \
   (position.x < (int) window_info.width) &&  \
   (position.y < (int) window_info.height)) ? MagickTrue : MagickFalse)

/*
  Enum declarations.
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

/*
  Typedef declarations.
*/
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

/*
  Variable declarations.
*/
static XWidgetInfo
  monitor_info =
  {
    (char *) NULL, (char *) NULL, (char *) NULL, 0, 0, 0, 0, 0, 0, 0, 0,
    MagickFalse, MagickFalse, MagickFalse, MagickFalse, MagickFalse
  },
  submenu_info =
  {
    (char *) NULL, (char *) NULL, (char *) NULL, 0, 0, 0, 0, 0, 0, 0, 0,
    MagickFalse, MagickFalse, MagickFalse, MagickFalse, MagickFalse
  },
  *selection_info = (XWidgetInfo *) NULL,
  toggle_info =
  {
    (char *) NULL, (char *) NULL, (char *) NULL, 0, 0, 0, 0, 0, 0, 0, 0,
    MagickFalse, MagickFalse, MagickFalse, MagickFalse, MagickFalse
  };

/*
  Constant declarations.
*/
static const int
  BorderOffset = 4,
  DoubleClick = 250;

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
%   D e s t r o y X W i d g e t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyXWidget() destroys resources associated with the X widget.
%
%  The format of the DestroyXWidget method is:
%
%      void DestroyXWidget()
%
%  A description of each parameter follows:
%
*/
MagickPrivate void DestroyXWidget(void)
{
  if (selection_info != (XWidgetInfo *) NULL)
    selection_info=(XWidgetInfo *) RelinquishMagickMemory(selection_info);
}

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
static void XDrawBevel(Display *display,const XWindowInfo *window_info,
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

static void XDrawBeveledButton(Display *display,const XWindowInfo *window_info,
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
static void XDrawBeveledMatte(Display *display,const XWindowInfo *window_info,
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
static void XDrawMatte(Display *display,const XWindowInfo *window_info,
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
static void XScrollTextMarker(XFontStruct *font_info,XWidgetInfo *text_info,
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

static void XSetVisibleTextMarker(XFontStruct *font_info,
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

static void XDrawMatteText(Display *display,const XWindowInfo *window_info,
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
static void XDrawWidgetText(Display *display,const XWindowInfo *window_info,
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
static void XInsertWidgetText(XWidgetInfo *text_info,char *text)
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

static void XEraseHighlightedText(XWidgetInfo *text_info)
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

static void XEditTextInsertKey(Display *display,XWidgetInfo *text_info,
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

static void XEditText(Display *display,XWidgetInfo *text_info,
  const KeySym key_symbol,char *text,const size_t state)
{
  switch ((int) key_symbol)
  {
    case XK_BackSpace:
    case XK_Delete:
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
          break;
        }
      magick_fallthrough;
    }
    case XK_Left:
    case XK_KP_Left:
    {
      /*
        Move cursor one position left.
      */
      if (text_info->cursor == text_info->text)
        break;
      text_info->cursor--;
      break;
    }
    case XK_Right:
    case XK_KP_Right:
    {
      /*
        Move cursor one position right.
      */
      if (text_info->cursor == (text_info->text+Extent(text_info->text)))
        break;
      text_info->cursor++;
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
static void XGetWidgetInfo(const char *text,XWidgetInfo *widget_info)
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
static void XHighlightWidget(Display *display,const XWindowInfo *window_info,
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

static void XScreenExpose(Display *display,XWindows *windows,XEvent *event)
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

static void XScreenTrackMapping(const XEvent *event,MagickBooleanType *mapped)
{
  if (event->type == MapNotify)
    (*mapped)=MagickTrue;
  if (event->type == UnmapNotify)
    (*mapped)=MagickFalse;
}

static void XScreenConvertAltButton(XEvent *event)
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

static int XScreenEvent(Display *display,XEvent *event,char *data)
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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

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
static void XSetBevelColor(Display *display,const XWindowInfo *window_info,
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
static void XSetMatteColor(Display *display,const XWindowInfo *window_info,
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
static void XSetTextColor(Display *display,const XWindowInfo *window_info,
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
static MagickBooleanType IsUnusableSelection(const Status status,const Atom type,
  const int format,const unsigned long length)
{
  return(((status != Success) || (type != XA_STRING) || (format == 32) ||
    (length == 0)) ? MagickTrue : MagickFalse);
}

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

static size_t ConfigureWidgetWindow(XWindows *windows,const XEvent event,
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

static size_t EnterWidgetWindow(XWindows *windows,const XEvent event,
  size_t state)
{
  if (event.xcrossing.window != windows->widget.id)
    return(state);
  state&=(unsigned int) (~InactiveWidgetState);
  return(state);
}

static size_t ExposeWidgetWindow(XWindows *windows,const XEvent event,
  size_t state)
{
  if (event.xexpose.window != windows->widget.id)
    return(state);
  if (event.xexpose.count != 0)
    return(state);
  state|=RedrawWidgetState;
  return(state);
}

static size_t LeaveWidgetWindow(XWindows *windows,const XEvent event,
  size_t state)
{
  if (event.xcrossing.window != windows->widget.id)
    return(state);
  state|=InactiveWidgetState;
  return(state);
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
    *cancel_info;
} ColorBrowserTargets;

typedef struct _ColorBrowserContext
{
  Display
    *display;

  XWindows
    *windows;
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

MagickPrivate void XColorBrowserWidget(Display *display,XWindows *windows,
  const char *action,char *reply)
{
#define CancelButtonText  "Cancel"
#define ColornameText  "Name:"
#define ColorPatternText  "Pattern:"
#define GrabButtonText  "Grab"
#define ResetButtonText  "Reset"

  char
    **colorlist,
    primary_selection[MagickPathExtent],
    reset_pattern[MagickPathExtent],
    text[MagickPathExtent];

  ExceptionInfo
    *exception;

  int
    x,
    y;

  int
    i;

  static char
    glob_pattern[MagickPathExtent] = "*";

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  Status
    status;

  unsigned int
    height,
    text_width,
    visible_colors,
    width;

  size_t
    colors,
    delay,
    state;

  XColor
    color;

  XEvent
    event;

  XFontStruct
    *font_info;

  XTextProperty
    window_name;

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

  XWindowChanges
    window_changes;

  const FontBrowserTargets
    targets = { &reply_info, &state, &selection_info, &list_info };

  const ColorBrowserTargets
    color_widgets = { &reply_info, &state,
      &grab_info, &reset_info, &action_info, &cancel_info };

  ColorBrowserContext
    color_context;

  /*
    Get color list and sort in ascending order.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(action != (char *) NULL);
  assert(reply != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(reset_pattern,"*",MagickPathExtent);
  exception=AcquireExceptionInfo();
  colorlist=GetColorList(glob_pattern,&colors,exception);
  if (colorlist == (char **) NULL)
    {
      /*
        Pattern failed, obtain all the colors.
      */
      (void) CopyMagickString(glob_pattern,"*",MagickPathExtent);
      colorlist=GetColorList(glob_pattern,&colors,exception);
      if (colorlist == (char **) NULL)
        {
          XNoticeWidget(display,windows,"Unable to obtain colors names:",
            glob_pattern);
          (void) XDialogWidget(display,windows,action,"Enter color name:",
            reply);
          return;
        }
    }
  /*
    Determine Color Browser widget attributes.
  */
  font_info=windows->widget.font_info;
  text_width=0;
  for (i=0; i < (int) colors; i++)
    if (WidgetTextWidth(font_info,colorlist[i]) > text_width)
      text_width=WidgetTextWidth(font_info,colorlist[i]);
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
  height=(unsigned int) (font_info->ascent+font_info->descent);
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
  /*
    Map Color Browser widget.
  */
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
  color_context.display=display;
  color_context.windows=windows;
  do
  {
    if (state & UpdateConfigurationState)
      {
        int
          id;

        /*
          Initialize button information.
        */
        XGetWidgetInfo(CancelButtonText,&cancel_info);
        cancel_info.width=width;
        cancel_info.height=(unsigned int) ((3*height) >> 1);
        cancel_info.x=(int) windows->widget.width-(int) cancel_info.width-
          QuantumMargin-2;
        cancel_info.y=(int) windows->widget.height-(int) cancel_info.height-
          QuantumMargin;
        XGetWidgetInfo(action,&action_info);
        action_info.width=width;
        action_info.height=(unsigned int) ((3*height) >> 1);
        action_info.x=(int) windows->widget.width-(int) action_info.width-
          (int) cancel_info.width-2*QuantumMargin-2;
        action_info.y=cancel_info.y;
        XGetWidgetInfo(GrabButtonText,&grab_info);
        grab_info.width=width;
        grab_info.height=(unsigned int) ((3*height) >> 1);
        grab_info.x=QuantumMargin;
        grab_info.y=((5*QuantumMargin) >> 1)+(int) height;
        XGetWidgetInfo(ResetButtonText,&reset_info);
        reset_info.width=width;
        reset_info.height=(unsigned int) ((3*height) >> 1);
        reset_info.x=QuantumMargin;
        reset_info.y=grab_info.y+(int) grab_info.height+QuantumMargin;
        /*
          Initialize reply information.
        */
        XGetWidgetInfo(reply,&reply_info);
        reply_info.raised=MagickFalse;
        reply_info.bevel_width--;
        reply_info.width=windows->widget.width-width-(unsigned int)
          ((6*QuantumMargin) >> 1);
        reply_info.height=height << 1;
        reply_info.x=(int) width+(QuantumMargin << 1);
        reply_info.y=action_info.y-(int) reply_info.height-QuantumMargin;
        /*
          Initialize mode information.
        */
        XGetWidgetInfo((char *) NULL,&mode_info);
        mode_info.active=MagickTrue;
        mode_info.bevel_width=0;
        mode_info.width=(unsigned int) (action_info.x-(int) (QuantumMargin << 1));
        mode_info.height=action_info.height;
        mode_info.x=QuantumMargin;
        mode_info.y=action_info.y;
        /*
          Initialize scroll information.
        */
        XGetWidgetInfo((char *) NULL,&scroll_info);
        scroll_info.bevel_width--;
        scroll_info.width=height;
        scroll_info.height=(unsigned int) (reply_info.y-grab_info.y-
          (QuantumMargin >> 1));
        scroll_info.x=reply_info.x+(int) (reply_info.width-scroll_info.width);
        scroll_info.y=grab_info.y-(int) reply_info.bevel_width;
        scroll_info.raised=MagickFalse;
        scroll_info.trough=MagickTrue;
        north_info=scroll_info;
        north_info.raised=MagickTrue;
        north_info.width-=(north_info.bevel_width << 1);
        north_info.height=north_info.width-1;
        north_info.x+=(int) north_info.bevel_width;
        north_info.y+=(int) north_info.bevel_width;
        south_info=north_info;
        south_info.y=scroll_info.y+(int) scroll_info.height-(int)
          scroll_info.bevel_width-(int) south_info.height;
        id=slider_info.id;
        slider_info=north_info;
        slider_info.id=id;
        slider_info.width-=2;
        slider_info.min_y=north_info.y+(int) north_info.height+(int)
          north_info.bevel_width+(int) slider_info.bevel_width+2;
        slider_info.height=(unsigned int) ((int) scroll_info.height-
          ((slider_info.min_y-scroll_info.y+1) << 1)+4);
        visible_colors=(unsigned int) (scroll_info.height*
          MagickSafeReciprocal((double) height+(height >> 3)));
        if (colors > visible_colors)
          slider_info.height=(unsigned int) ((visible_colors*
            slider_info.height)/colors);
        slider_info.max_y=south_info.y-(int) south_info.bevel_width-
          (int) slider_info.bevel_width-2;
        slider_info.x=scroll_info.x+(int) slider_info.bevel_width+1;
        slider_info.y=slider_info.min_y;
        expose_info=scroll_info;
        expose_info.y=slider_info.y;
        /*
          Initialize list information.
        */
        XGetWidgetInfo((char *) NULL,&list_info);
        list_info.raised=MagickFalse;
        list_info.bevel_width--;
        list_info.width=(unsigned int)
          (scroll_info.x-reply_info.x-(int) (QuantumMargin >> 1));
        list_info.height=scroll_info.height;
        list_info.x=reply_info.x;
        list_info.y=scroll_info.y;
        if (windows->widget.mapped == MagickFalse)
          state|=JumpListState;
        /*
          Initialize text information.
        */
        *text='\0';
        XGetWidgetInfo(text,&text_info);
        text_info.center=MagickFalse;
        text_info.width=reply_info.width;
        text_info.height=height;
        text_info.x=list_info.x-(int) (QuantumMargin >> 1);
        text_info.y=QuantumMargin;
        /*
          Initialize selection information.
        */
        XGetWidgetInfo((char *) NULL,&selection_info);
        selection_info.center=MagickFalse;
        selection_info.width=list_info.width;
        selection_info.height=(unsigned int) ((9*height) >> 3);
        selection_info.x=list_info.x;
        state&=(unsigned int) (~UpdateConfigurationState);
      }
    if (state & RedrawWidgetState)
      {
        /*
          Redraw Color Browser window.
        */
        x=QuantumMargin;
        y=text_info.y+(int) ((text_info.height-height) >> 1)+font_info->ascent;
        (void) XDrawString(display,windows->widget.id,
          windows->widget.annotate_context,x,y,ColorPatternText,
          Extent(ColorPatternText));
        (void) CopyMagickString(text_info.text,glob_pattern,MagickPathExtent);
        XDrawWidgetText(display,&windows->widget,&text_info);
        XDrawBeveledButton(display,&windows->widget,&grab_info);
        XDrawBeveledButton(display,&windows->widget,&reset_info);
        XDrawBeveledMatte(display,&windows->widget,&list_info);
        XDrawBeveledMatte(display,&windows->widget,&scroll_info);
        XDrawTriangleNorth(display,&windows->widget,&north_info);
        XDrawBeveledButton(display,&windows->widget,&slider_info);
        XDrawTriangleSouth(display,&windows->widget,&south_info);
        x=QuantumMargin;
        y=reply_info.y+(int) ((reply_info.height-height) >> 1)+
          font_info->ascent;
        (void) XDrawString(display,windows->widget.id,
          windows->widget.annotate_context,x,y,ColornameText,
          Extent(ColornameText));
        XDrawBeveledMatte(display,&windows->widget,&reply_info);
        XDrawMatteText(display,&windows->widget,&reply_info);
        XDrawBeveledButton(display,&windows->widget,&action_info);
        XDrawBeveledButton(display,&windows->widget,&cancel_info);
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        selection_info.id=(~0);
        state|=RedrawActionState;
        state|=RedrawListState;
        state&=(unsigned int) (~RedrawWidgetState);
      }
    if (state & UpdateListState)
      {
        const ColorBrowserUpdateTargets
          update_targets = { &action_info, &colorlist, &colors };
        
        ColorBrowserUpdate
          update;

        update.display=display;
        update.windows=windows;
        update.reply=reply;
        update.glob_pattern=glob_pattern;
        update.reset_pattern=reset_pattern;
        update.exception=exception;
        if (UpdateColorBrowserList(&update,&update_targets) == MagickFalse)
          break;
        /*
          Sort color list in ascending order.
        */
        slider_info.height=(unsigned int) ((int) scroll_info.height-
          ((slider_info.min_y-scroll_info.y+1) << 1)+1);
        if (colors > visible_colors)
          slider_info.height=(unsigned int) ((visible_colors*
            slider_info.height)/colors);
        slider_info.max_y=south_info.y-(int) south_info.bevel_width-
          (int) slider_info.bevel_width-2;
        slider_info.id=0;
        slider_info.y=slider_info.min_y;
        expose_info.y=slider_info.y;
        selection_info.id=(~0);
        list_info.id=(~0);
        state|=RedrawListState;
        /*
          Redraw color name & reply.
        */
        *reply_info.text='\0';
        reply_info.cursor=reply_info.text;
        (void) CopyMagickString(text_info.text,glob_pattern,MagickPathExtent);
        XDrawWidgetText(display,&windows->widget,&text_info);
        XDrawMatteText(display,&windows->widget,&reply_info);
        XDrawBeveledMatte(display,&windows->widget,&scroll_info);
        XDrawTriangleNorth(display,&windows->widget,&north_info);
        XDrawBeveledButton(display,&windows->widget,&slider_info);
        XDrawTriangleSouth(display,&windows->widget,&south_info);
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        state&=(unsigned int) (~UpdateListState);
      }
    if (state & JumpListState)
      {
        FontBrowserMatch
          match;

        /*
          Jump scroll to match user color.
        */
        list_info.id=(~0);
        match.fontlist=colorlist;
        match.fonts=(int) colors;
        match.reply=reply;
        MatchFontBrowserReply(&match,&list_info,&i);
        if ((i < slider_info.id) ||
            (i >= (int) (slider_info.id+(int) visible_colors)))
          slider_info.id=i-(int) (visible_colors >> 1);
        selection_info.id=(~0);
        state|=RedrawListState;
        state&=(unsigned int) (~JumpListState);
      }
    if (state & RedrawListState)
      {
        /*
          Determine slider id and position.
        */
        if (slider_info.id >= (int) (colors-visible_colors))
          slider_info.id=(int) (colors-visible_colors);
        if ((slider_info.id < 0) || (colors <= visible_colors))
          slider_info.id=0;
        slider_info.y=slider_info.min_y;
        if (colors != 0)
          slider_info.y+=(int) slider_info.id*(slider_info.max_y-
            slider_info.min_y+1)/(int) colors;
        if (slider_info.id != selection_info.id)
          {
            FontBrowserNames
              names;

            /*
              Redraw scroll bar and file names.
            */
            selection_info.id=slider_info.id;
            selection_info.y=list_info.y+(int) (height >> 3)+2;
            names.display=display;
            names.windows=windows;
            names.fontlist=colorlist;
            names.fonts=(int) colors;
            names.visible_fonts=visible_colors;
            names.list_info=list_info;
            names.slider_info=slider_info;
            DrawFontBrowserNames(&names,&selection_info,&i);
            /*
              Update slider.
            */
            expose_info=PlaceTextViewExpose(expose_info,slider_info);
            XDrawTriangleNorth(display,&windows->widget,&north_info);
            XDrawMatte(display,&windows->widget,&expose_info);
            XDrawBeveledButton(display,&windows->widget,&slider_info);
            XDrawTriangleSouth(display,&windows->widget,&south_info);
            expose_info.y=slider_info.y;
          }
        state&=(unsigned int) (~RedrawListState);
      }
    if (state & RedrawActionState)
      {
        static char
          colorname[MagickPathExtent];

        /*
          Display the selected color in a drawing area.
        */
        color=windows->widget.pixel_info->matte_color;
        (void) XParseColor(display,windows->widget.map_info->colormap,
          reply_info.text,&windows->widget.pixel_info->matte_color);
        XBestPixel(display,windows->widget.map_info->colormap,(XColor *) NULL,
          (unsigned int) windows->widget.visual_info->colormap_size,
          &windows->widget.pixel_info->matte_color);
        mode_info.text=colorname;
        (void) FormatLocaleString(mode_info.text,MagickPathExtent,
          "#%02x%02x%02x",windows->widget.pixel_info->matte_color.red,
          windows->widget.pixel_info->matte_color.green,
          windows->widget.pixel_info->matte_color.blue);
        XDrawBeveledButton(display,&windows->widget,&mode_info);
        windows->widget.pixel_info->matte_color=color;
        state&=(unsigned int) (~RedrawActionState);
      }
    /*
      Wait for next event.
    */
    if (north_info.raised && south_info.raised)
      (void) XIfEvent(display,&event,XScreenEvent,(char *) windows);
    else
      {
        /*
          Brief delay before advancing scroll bar.
        */
        XDelay(display,delay);
        delay=SuspendTime;
        (void) XCheckIfEvent(display,&event,XScreenEvent,(char *) windows);
        if ((north_info.raised == MagickFalse) && (slider_info.id > 0))
          {
            /*
              Move slider up.
            */
            slider_info.id--;
            state|=RedrawListState;
          }
        if ((south_info.raised == MagickFalse) &&
            (slider_info.id < (int) colors))
          {
            /*
              Move slider down.
            */
            slider_info.id++;
            state|=RedrawListState;
          }
        if (event.type != ButtonRelease)
          continue;
      }
    switch (event.type)
    {
      case ButtonPress:
      {
        if (MatteIsActive(slider_info,event.xbutton))
          {
            /*
              Track slider.
            */
            slider_info.active=MagickTrue;
            break;
          }
        if ((MatteIsActive(north_info,event.xbutton)) && (slider_info.id > 0))
          {
            /*
              Move slider up.
            */
            north_info.raised=MagickFalse;
            slider_info.id--;
            state|=RedrawListState;
            break;
          }
        if ((MatteIsActive(south_info,event.xbutton)) &&
            (slider_info.id < (int) colors))
          {
            /*
              Move slider down.
            */
            south_info.raised=MagickFalse;
            slider_info.id++;
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(scroll_info,event.xbutton))
          {
            /*
              Move slider.
            */
            slider_info=PageListBrowserSlider(slider_info,event,visible_colors);
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(list_info,event.xbutton))
          {
            FontBrowserPress
              press;

            press.display=display;
            press.windows=windows;
            press.fontlist=colorlist;
            press.fonts=colors;
            press.height=height;
            press.event=event;
            press.slider_info=slider_info;
            SelectFontBrowserEntry(&press,glob_pattern,&targets);
            break;
          }
        if (MatteIsActive(grab_info,event.xbutton))
          {
            /*
              User pressed Grab button.
            */
            grab_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&grab_info);
            break;
          }
        if (MatteIsActive(reset_info,event.xbutton))
          {
            /*
              User pressed Reset button.
            */
            reset_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&reset_info);
            break;
          }
        if (MatteIsActive(mode_info,event.xbutton))
          {
            /*
              User pressed mode button.
            */
            CopyColorBrowserMode(mode_info,reply_info);
            (void) CopyMagickString(primary_selection,reply_info.text,
              MagickPathExtent);
            (void) XSetSelectionOwner(display,XA_PRIMARY,windows->widget.id,
              event.xbutton.time);
            reply_info.highlight=XGetSelectionOwner(display,XA_PRIMARY) ==
              windows->widget.id ? MagickTrue : MagickFalse;
            reply_info.marker=reply_info.text;
            reply_info.cursor=reply_info.text+Extent(reply_info.text);
            XDrawMatteText(display,&windows->widget,&reply_info);
            break;
          }
        if (MatteIsActive(action_info,event.xbutton))
          {
            /*
              User pressed action button.
            */
            action_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&action_info);
            break;
          }
        if (MatteIsActive(cancel_info,event.xbutton))
          {
            /*
              User pressed Cancel button.
            */
            cancel_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&cancel_info);
            break;
          }
        if (MatteIsActive(reply_info,event.xbutton) == MagickFalse)
          break;
        if (event.xbutton.button != Button2)
          {
            static Time
              click_time;

            ListBrowserClick
              click;

            /*
              Move text cursor to position of button press.
            */
            x=event.xbutton.x-reply_info.x-(int) (QuantumMargin >> 2);
            i=FindListBrowserCursor(font_info,reply_info,x);
            reply_info.cursor=reply_info.marker+i-1;
            click.display=display;
            click.window_info=(&windows->widget);
            click.event=event;
            click.click_time=click_time;
            reply_info=HighlightListBrowserReply(&click,primary_selection,
              reply_info);
            XDrawMatteText(display,&windows->widget,&reply_info);
            click_time=event.xbutton.time;
            break;
          }
        /*
          Request primary selection.
        */
        (void) XConvertSelection(display,XA_PRIMARY,XA_STRING,XA_STRING,
          windows->widget.id,event.xbutton.time);
        break;
      }
      case ButtonRelease:
      {
        ListBrowserRelease
          release;

        if (windows->widget.mapped == MagickFalse)
          break;
        if (north_info.raised == MagickFalse)
          {
            /*
              User released up button.
            */
            delay=SuspendTime << 2;
            north_info.raised=MagickTrue;
            XDrawTriangleNorth(display,&windows->widget,&north_info);
          }
        if (south_info.raised == MagickFalse)
          {
            /*
              User released down button.
            */
            delay=SuspendTime << 2;
            south_info.raised=MagickTrue;
            XDrawTriangleSouth(display,&windows->widget,&south_info);
          }
        if (slider_info.active)
          {
            /*
              Stop tracking slider.
            */
            slider_info.active=MagickFalse;
            break;
          }
        if (grab_info.raised == MagickFalse)
          {
            ColorBrowserGrab
              grab;

            grab.display=display;
            grab.windows=windows;
            grab.event=event;
            grab.grab_info=grab_info;
            grab.exception=exception;
            ReleaseColorBrowserGrab(&grab,&reply_info,&state);
            grab_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&grab_info);
          }
        if (reset_info.raised == MagickFalse)
          {
            FontBrowserBack
              back;

            back.windows=windows;
            back.event=event;
            back.back_info=reset_info;
            back.glob_pattern=glob_pattern;
            back.back_pattern=reset_pattern;
            state=ReleaseFontBrowserBack(&back,state);
            reset_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&reset_info);
          }
        release.display=display;
        release.window_info=(&windows->widget);
        release.event=event;
        ReleaseListBrowserAction(&release,reply_info,&action_info,&state);
        release.display=display;
        release.window_info=(&windows->widget);
        release.event=event;
        ReleaseListBrowserCancel(&release,reply_info,&cancel_info,&state);
        if (MatteIsActive(reply_info,event.xbutton) == MagickFalse)
          break;
        break;
      }
      case ClientMessage:
      {
        ClientColorBrowserMessage(&color_context,event,&color_widgets);
        break;
      }
      case ConfigureNotify:
      {
        state=ConfigureWidgetWindow(windows,event,state);
        break;
      }
      case EnterNotify:
      {
        state=EnterWidgetWindow(windows,event,state);
        break;
      }
      case Expose:
      {
        state=ExposeWidgetWindow(windows,event,state);
        break;
      }
      case KeyPress:
      {
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
          break;
        length=XLookupString((XKeyEvent *) &event.xkey,command,
          (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        *(command+length)='\0';
        if (AreaIsActive(scroll_info,event.xkey))
          {
            /*
              Move slider.
            */
            slider_info=MoveFontBrowserSlider(slider_info,key_symbol,
              visible_colors,(int) colors);
            state|=RedrawListState;
            break;
          }
        if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
          {
            state=ReadColorBrowserPattern(reply_info,glob_pattern,state);
            break;
          }
        if (key_symbol == XK_Control_L)
          {
            state|=ControlState;
            break;
          }
        reply_info=ApplyListBrowserControlKey(reply_info,key_symbol,state);
        XEditText(display,&reply_info,key_symbol,command,state);
        XDrawMatteText(display,&windows->widget,&reply_info);
        state|=JumpListState;
        status=XParseColor(display,windows->widget.map_info->colormap,
          reply_info.text,&color);
        if (status != False)
          state|=RedrawActionState;
        break;
      }
      case KeyRelease:
      {
        state=ReleaseWidgetKey(windows,event,state);
        break;
      }
      case LeaveNotify:
      {
        state=LeaveWidgetWindow(windows,event,state);
        break;
      }
      case MapNotify:
      {
        mask&=(unsigned int) (~CWX);
        mask&=(unsigned int) (~CWY);
        break;
      }
      case MotionNotify:
      {
        /*
          Discard pending button motion events.
        */
        while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
        if (slider_info.active)
          {
            /*
              Move slider matte.
            */
            slider_info=TrackColorBrowserSlider(slider_info,event,colors);
            state|=RedrawListState;
            break;
          }
        if (state & InactiveWidgetState)
          break;
        ToggleColorBrowserButtons(&color_context,event,&color_widgets);
        break;
      }
      case SelectionClear:
      {
        reply_info.highlight=MagickFalse;
        XDrawMatteText(display,&windows->widget,&reply_info);
        break;
      }
      case SelectionNotify:
      {
        SelectColorBrowserReply(&color_context,event,&color_widgets);
        break;
      }
      case SelectionRequest:
      {
        XSelectionEvent
          notify;

        XSelectionRequestEvent
          *request;

        if (reply_info.highlight == MagickFalse)
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
  } while ((state & ExitState) == 0);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
  /*
    Free color list.
  */
  for (i=0; i < (int) colors; i++)
    colorlist[i]=DestroyString(colorlist[i]);
  if (colorlist != (char **) NULL)
    colorlist=(char **) RelinquishMagickMemory(colorlist);
  exception=DestroyExceptionInfo(exception);
  if ((*reply == '\0') || (strchr(reply,'-') != (char *) NULL))
    return;
  status=XParseColor(display,windows->widget.map_info->colormap,reply,&color);
  if (status != False)
    return;
  XNoticeWidget(display,windows,"Color is unknown to X server:",reply);
  (void) CopyMagickString(reply,"gray",MagickPathExtent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X C o m m a n d W i d g e t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XCommandWidget() maps a menu and returns the command pointed to by the user
%  when the button is released.
%
%  The format of the XCommandWidget method is:
%
%      int XCommandWidget(Display *display,XWindows *windows,
%        const char *const *selections,XEvent *event)
%
%  A description of each parameter follows:
%
%    o selection_number: Specifies the number of the selection that the
%      user choose.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o selections: Specifies a pointer to one or more strings that comprise
%      the choices in the menu.
%
%    o event: Specifies a pointer to a X11 XEvent structure.
%
*/
typedef struct _CommandWidgetEvent
{
  Display
    *display;

  XWindows
    *windows;

  XEvent
    *event;

  unsigned int
    number_selections;
} CommandWidgetEvent;

static int PressCommandSelection(const CommandWidgetEvent *command_event,int id)
{
  Display
    *const display = command_event->display;

  XWindows
    *const windows = command_event->windows;

  XEvent
    *const event = command_event->event;

  const unsigned int
    number_selections = command_event->number_selections;

  int
    i;

  for (i=0; i < (int) number_selections; i++)
  {
    if (MatteIsActive(selection_info[i],event->xbutton) == MagickFalse)
      continue;
    if (i >= (int) windows->command.data)
      {
        selection_info[i].raised=MagickFalse;
        XDrawBeveledButton(display,&windows->command,&selection_info[i]);
        break;
      }
    submenu_info=selection_info[i];
    submenu_info.active=MagickTrue;
    toggle_info.y=submenu_info.y+(int) (submenu_info.height >> 1)-
      (int) (toggle_info.height >> 1);
    id=i;
    (void) XCheckWindowEvent(display,windows->widget.id,LeaveWindowMask,
      event);
    break;
  }
  return(id);
}

static int ReleaseCommandSelection(const CommandWidgetEvent *command_event,int id)
{
  Display
    *const display = command_event->display;

  XWindows
    *const windows = command_event->windows;

  XEvent
    *const event = command_event->event;

  const unsigned int
    number_selections = command_event->number_selections;

  int
    i;

  for (i=0; i < (int) number_selections; i++)
  {
    if (MatteIsActive(selection_info[i],event->xbutton) == MagickFalse)
      continue;
    id=i;
    if (id >= (int) windows->command.data)
      {
        selection_info[id].raised=MagickTrue;
        XDrawBeveledButton(display,&windows->command,&selection_info[id]);
        break;
      }
    break;
  }
  return(id);
}

static int TrackCommandSelections(const CommandWidgetEvent *command_event,int id)
{
  Display
    *const display = command_event->display;

  XWindows
    *const windows = command_event->windows;

  XEvent
    *const event = command_event->event;

  const unsigned int
    number_selections = command_event->number_selections;

  int
    i;

  for (i=0; i < (int) number_selections; i++)
  {
    if (i >= (int) windows->command.data)
      {
        if (selection_info[i].raised ==
            MatteIsActive(selection_info[i],event->xmotion))
          {
            /*
              Button status changed.
            */
            selection_info[i].raised=!selection_info[i].raised;
            XDrawBeveledButton(display,&windows->command,
              &selection_info[i]);
          }
        continue;
      }
    if (MatteIsActive(selection_info[i],event->xmotion) == MagickFalse)
      continue;
    submenu_info=selection_info[i];
    submenu_info.active=MagickTrue;
    toggle_info.raised=MagickTrue;
    toggle_info.y=submenu_info.y+(int) (submenu_info.height >> 1)-
      (int) (toggle_info.height >> 1);
    XDrawTriangleEast(display,&windows->command,&toggle_info);
    id=i;
  }
  return(id);
}

static int FollowCommandMotion(const CommandWidgetEvent *motion,int id)
{
  Display
    *const display = motion->display;

  XWindows
    *const windows = motion->windows;

  XEvent
    *const event = motion->event;

  const unsigned int
    number_selections = motion->number_selections;

  for ( ; ; )
  {
    CommandWidgetEvent
      command_event;

    command_event.display=display;
    command_event.windows=windows;
    command_event.event=event;
    command_event.number_selections=number_selections;
    id=TrackCommandSelections(&command_event,id);
    XDelay(display,SuspendTime);
    if (XCheckMaskEvent(display,ButtonMotionMask,event) == MagickFalse)
      break;
    while (XCheckMaskEvent(display,ButtonMotionMask,event)) ;
    toggle_info.raised=MagickFalse;
    if (windows->command.data != 0)
      XDrawTriangleEast(display,&windows->command,&toggle_info);
  }
  return(id);
}

static const unsigned char
  tile_bits[]=
  {
    0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x1e, 0x38, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x1e, 0xbc, 0x9f, 0x03, 0x00, 0x3e, 0x00, 0xc0,
    0x1f, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x1e, 0xfc, 0xff, 0x0f, 0x80, 0x3f,
    0x00, 0xf0, 0x1f, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x1e, 0xfc, 0xff, 0x1f,
    0xe0, 0x3f, 0x00, 0xfc, 0x1f, 0xf0, 0x0f, 0x00, 0x00, 0x00, 0x1e, 0xfc,
    0xff, 0x1f, 0xf0, 0x3f, 0x00, 0xfe, 0x1f, 0xf8, 0x0f, 0x00, 0x00, 0x00,
    0x1e, 0xfc, 0xfc, 0x3f, 0xf8, 0x3f, 0x00, 0xff, 0x1e, 0xfc, 0x0f, 0x00,
    0x00, 0x00, 0x1e, 0x7c, 0xfc, 0x3e, 0xf8, 0x3c, 0x80, 0x1f, 0x1e, 0x7c,
    0x0f, 0x00, 0x00, 0x00, 0x1e, 0x78, 0x78, 0x3c, 0x7c, 0x3c, 0xc0, 0x0f,
    0x1e, 0x3e, 0x0f, 0x00, 0x00, 0x00, 0x1e, 0x78, 0x78, 0x3c, 0x7c, 0x3c,
    0xc0, 0x07, 0x1e, 0x3e, 0x0f, 0x00, 0x00, 0x00, 0x1e, 0x78, 0x78, 0x3c,
    0x7c, 0x7c, 0xc0, 0x0f, 0x1e, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x78,
    0x78, 0x3c, 0xfc, 0x7c, 0x80, 0x7f, 0x1e, 0x7c, 0x00, 0x00, 0x00, 0x00,
    0x1e, 0xf8, 0x78, 0x7c, 0xf8, 0xff, 0x00, 0xff, 0x1f, 0xf8, 0xff, 0x00,
    0x00, 0x00, 0x1e, 0xf8, 0x78, 0x7c, 0xf0, 0xff, 0x07, 0xfe, 0x1f, 0xf8,
    0xff, 0x00, 0x00, 0x00, 0x1e, 0xf8, 0x78, 0x7c, 0xf0, 0xff, 0x07, 0xf8,
    0x1f, 0xf0, 0xff, 0x01, 0x00, 0x00, 0x1e, 0xf8, 0x78, 0x7c, 0xc0, 0xef,
    0x07, 0xe0, 0x1f, 0xc0, 0xff, 0x01, 0x00, 0x00, 0x1e, 0x70, 0x40, 0x78,
    0x00, 0xc7, 0x07, 0x00, 0x1e, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x1e, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x07,
    0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xc0, 0x0f, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x60, 0x00, 0xc0, 0x0f, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x78, 0x00, 0xc0, 0x8f, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0xc0, 0x8f, 0x3f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0xe0, 0x9f, 0x7f, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0xe0, 0xdf,
    0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x78, 0x00,
    0xe0, 0xdf, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x0c,
    0x78, 0x30, 0xf0, 0xff, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e,
    0x00, 0x0f, 0xf8, 0x70, 0xf0, 0xff, 0x7b, 0x00, 0x00, 0x1f, 0x00, 0xe0,
    0x0f, 0x1e, 0x80, 0x0f, 0xf8, 0x78, 0xf0, 0xfd, 0xf9, 0x00, 0xc0, 0x1f,
    0x00, 0xf8, 0x0f, 0x00, 0xe0, 0x1f, 0xf8, 0x7c, 0xf0, 0xfc, 0xf9, 0x00,
    0xf0, 0x1f, 0x00, 0xfe, 0x0f, 0x00, 0xf0, 0x07, 0xf8, 0x3e, 0xf8, 0xfc,
    0xf0, 0x01, 0xf8, 0x1f, 0x00, 0xff, 0x0f, 0x1e, 0xf0, 0x03, 0xf8, 0x3f,
    0xf8, 0xf8, 0xf0, 0x01, 0xfc, 0x1f, 0x80, 0x7f, 0x0f, 0x1e, 0xf8, 0x00,
    0xf8, 0x1f, 0x78, 0x18, 0xf0, 0x01, 0x7c, 0x1e, 0xc0, 0x0f, 0x0f, 0x1e,
    0x7c, 0x00, 0xf0, 0x0f, 0x78, 0x00, 0xf0, 0x01, 0x3e, 0x1e, 0xe0, 0x07,
    0x0f, 0x1e, 0x7c, 0x00, 0xf0, 0x07, 0x7c, 0x00, 0xe0, 0x01, 0x3e, 0x1e,
    0xe0, 0x03, 0x0f, 0x1e, 0x3e, 0x00, 0xf0, 0x0f, 0x7c, 0x00, 0xe0, 0x03,
    0x3e, 0x3e, 0xe0, 0x07, 0x0f, 0x1e, 0x1e, 0x00, 0xf0, 0x1f, 0x3c, 0x00,
    0xe0, 0x03, 0x7e, 0x3e, 0xc0, 0x3f, 0x0f, 0x1e, 0x3e, 0x00, 0xf0, 0x1f,
    0x3e, 0x00, 0xe0, 0x03, 0xfc, 0x7f, 0x80, 0xff, 0x0f, 0x1e, 0xfc, 0x00,
    0xf0, 0x3e, 0x3e, 0x00, 0xc0, 0x03, 0xf8, 0xff, 0x03, 0xff, 0x0f, 0x1e,
    0xfc, 0x07, 0xf0, 0x7c, 0x1e, 0x00, 0xc0, 0x03, 0xf8, 0xff, 0x03, 0xfc,
    0x0f, 0x1e, 0xf8, 0x1f, 0xf0, 0xf8, 0x1e, 0x00, 0xc0, 0x03, 0xe0, 0xf7,
    0x03, 0xf0, 0x0f, 0x1e, 0xe0, 0x3f, 0xf0, 0x78, 0x1c, 0x00, 0x80, 0x03,
    0x80, 0xe3, 0x03, 0x00, 0x0f, 0x1e, 0xc0, 0x3f, 0xf0, 0x30, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0e, 0x00, 0x3e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x0f, 0x00, 0x00, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x0f, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0,
    0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xf0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

static void ClientCommandMessage(Display *display,XWindows *windows,
  XEvent *event)
{
  /*
    If client window delete message, withdraw command widget.
  */
  if (event->xclient.message_type != windows->wm_protocols)
    return;
  if (*event->xclient.data.l != (int) windows->wm_delete_window)
    return;
  (void) XWithdrawWindow(display,windows->command.id,
    windows->command.screen);
}

static size_t ConfigureCommandWindow(XWindows *windows,XEvent *event,
  size_t state)
{
  /*
    Update widget configuration.
  */
  if (event->xconfigure.window != windows->command.id)
    return(state);
  if (event->xconfigure.send_event != 0)
    {
      windows->command.x=event->xconfigure.x;
      windows->command.y=event->xconfigure.y;
    }
  if ((event->xconfigure.width == (int) windows->command.width) &&
      (event->xconfigure.height == (int) windows->command.height))
    return(state);
  windows->command.width=(unsigned int)
    MagickMax(event->xconfigure.width,(int) windows->command.min_width);
  windows->command.height=(unsigned int)
    MagickMax(event->xconfigure.height,(int) windows->command.min_height);
  state|=UpdateConfigurationState;
  return(state);
}

static size_t ExposeCommandWindow(XWindows *windows,XEvent *event,
  size_t state)
{
  if (event->xexpose.window != windows->command.id)
    return(state);
  if (event->xexpose.count != 0)
    return(state);
  state|=RedrawWidgetState;
  return(state);
}

static void HandleCommandWindowEvent(const CommandWidgetEvent *tile,int *id,
  size_t *state)
{
  Display
    *const display = tile->display;

  XWindows
    *const windows = tile->windows;

  XEvent
    *const event = tile->event;

  const unsigned int
    number_selections = tile->number_selections;

  switch (event->type)
  {
    case Expose:
    {
      (*state)=ExposeCommandWindow(windows,event,(*state));
      break;
    }
    case MotionNotify:
    {
      CommandWidgetEvent
        motion;

      /*
        Return the ID of the highlighted menu entry.
      */
      motion.display=display;
      motion.windows=windows;
      motion.event=event;
      motion.number_selections=number_selections;
      (*id)=FollowCommandMotion(&motion,(*id));
      break;
    }
    case MapNotify:
    {
      windows->command.mapped=MagickTrue;
      break;
    }
    case UnmapNotify:
    {
      windows->command.mapped=MagickFalse;
      break;
    }
    default:
      break;
  }
}

static void HandleCommandEvent(const CommandWidgetEvent *tile,int *id,
  size_t *state)
{
  Display
    *const display = tile->display;

  XWindows
    *const windows = tile->windows;

  XEvent
    *const event = tile->event;

  const unsigned int
    number_selections = tile->number_selections;

  if (event != (XEvent *) NULL)
    switch (event->type)
    {
      case ButtonPress:
      {
        CommandWidgetEvent
          command_event;

        command_event.display=display;
        command_event.windows=windows;
        command_event.event=event;
        command_event.number_selections=number_selections;
        (*id)=PressCommandSelection(&command_event,(*id));
        break;
      }
      case ButtonRelease:
      {
        CommandWidgetEvent
          command_event;

        command_event.display=display;
        command_event.windows=windows;
        command_event.event=event;
        command_event.number_selections=number_selections;
        (*id)=ReleaseCommandSelection(&command_event,(*id));
        break;
      }
      case ClientMessage:
      {
        ClientCommandMessage(display,windows,event);
        break;
      }
      case ConfigureNotify:
      {
        (*state)=ConfigureCommandWindow(windows,event,(*state));
        break;
      }
      default:
      {
        HandleCommandWindowEvent(tile,id,state);
        break;
      }
    }
}

#define tile_width 112
#define tile_height 70

static void RedrawCommandButtons(Display *display,XWindows *windows,
  const unsigned int number_selections)
{
  int
    i;

  Pixmap
    tile_pixmap;

  /*
    Draw command buttons.
  */
  tile_pixmap=XCreatePixmapFromBitmapData(display,windows->command.id,
    (char *) tile_bits,tile_width,tile_height,1L,0L,1);
  if (tile_pixmap != (Pixmap) NULL)
    {
      (void) XCopyPlane(display,tile_pixmap,windows->command.id,
        windows->command.annotate_context,0,0,tile_width,tile_height,
        (int) ((windows->command.width-tile_width) >> 1),10,1L);
      (void) XFreePixmap(display,tile_pixmap);
    }
  for (i=0; i < (int) number_selections; i++)
  {
    XDrawBeveledButton(display,&windows->command,&selection_info[i]);
    if (i >= (int) windows->command.data)
      continue;
    toggle_info.raised=MagickFalse;
    toggle_info.y=selection_info[i].y+(int) (selection_info[i].height >> 1)-
      (int) (toggle_info.height >> 1);
    XDrawTriangleEast(display,&windows->command,&toggle_info);
  }
  XHighlightWidget(display,&windows->command,BorderOffset,BorderOffset);
}

typedef struct _CommandLayout
{
  Display
    *display;

  XWindows
    *windows;

  const char
    *const *selections;

  XFontStruct
    *font_info;

  unsigned int
    height,
    number_selections;
} CommandLayout;

static void InitializeCommandButtons(const CommandLayout *layout)
{
  Display
    *const display = layout->display;

  XWindows
    *const windows = layout->windows;

  const char
    *const *const selections = layout->selections;

  XFontStruct
    *const font_info = layout->font_info;

  const unsigned int
    height = layout->height,
    number_selections = layout->number_selections;

  int
    i,
    y;

  y=tile_height+20;
  for (i=0; i < (int) number_selections; i++)
  {
    XGetWidgetInfo(selections[i],&selection_info[i]);
    selection_info[i].center=MagickFalse;
    selection_info[i].bevel_width--;
    selection_info[i].height=(unsigned int) ((3*height) >> 1);
    selection_info[i].x=(QuantumMargin >> 1)+4;
    selection_info[i].width=(unsigned int) ((int) windows->command.width-
      (selection_info[i].x << 1));
    selection_info[i].y=y;
    y+=(int) selection_info[i].height+(int)
      (selection_info[i].bevel_width << 1)+6;
  }
  XGetWidgetInfo((char *) NULL,&toggle_info);
  toggle_info.bevel_width--;
  toggle_info.width=(unsigned int) (((5*height) >> 3)-
    (toggle_info.bevel_width << 1));
  toggle_info.height=toggle_info.width;
  toggle_info.x=selection_info[0].x+(int) selection_info[0].width-
    (int) toggle_info.width-(int) (QuantumMargin >> 1);
  if (windows->command.mapped)
    (void) XClearWindow(display,windows->command.id);
}

static void ReconfigureCommandWindow(Display *display,XWindows *windows)
{
  XTextProperty
    window_name;

  XWindowChanges
    window_changes;

  if (windows->command.id != (Window) NULL)
    {
      Status
        status;

      /*
        Reconfigure command window.
      */
      status=XStringListToTextProperty(&windows->command.name,1,
        &window_name);
      if (status != False)
        {
          XSetWMName(display,windows->command.id,&window_name);
          XSetWMIconName(display,windows->command.id,&window_name);
          (void) XFree((void *) window_name.value);
        }
      window_changes.width=(int) windows->command.width;
      window_changes.height=(int) windows->command.height;
      (void) XReconfigureWMWindow(display,windows->command.id,
        windows->command.screen,(unsigned int) (CWWidth | CWHeight),
        &window_changes);
    }
}

typedef struct _CommandSizing
{
  Display
    *display;

  XWindows
    *windows;

  const char
    *const *selections;

  XFontStruct
    *font_info;

  unsigned int
    height;
} CommandSizing;

static unsigned int SizeCommandWindow(const CommandSizing *sizing)
{
  Display
    *const display = sizing->display;

  XWindows
    *const windows = sizing->windows;

  const char
    *const *const selections = sizing->selections;

  XFontStruct
    *const font_info = sizing->font_info;

  const unsigned int
    height = sizing->height;

  int
    i;

  unsigned int
    number_selections,
    width;

  windows->command.width=0;
  for (i=0; selections[i] != (char *) NULL; i++)
  {
    width=WidgetTextWidth(font_info,(char *) selections[i]);
    if (width > windows->command.width)
      windows->command.width=width;
  }
  number_selections=(unsigned int) i;
  windows->command.width+=(unsigned int) (3*QuantumMargin+10);
  if ((int) windows->command.width < (tile_width+QuantumMargin+10))
    windows->command.width=(unsigned  int) (tile_width+QuantumMargin+10);
  windows->command.height=(unsigned  int) (number_selections*
    (((3*height) >> 1)+10)+tile_height+20);
  windows->command.min_width=windows->command.width;
  windows->command.min_height=windows->command.height;
  XConstrainWindowPosition(display,&windows->command);
  return(number_selections);
}

MagickPrivate int XCommandWidget(Display *display,XWindows *windows,
  const char *const *selections,XEvent *event)
{
  int
    id;

  static unsigned int
    number_selections;

  unsigned int
    height;

  size_t
    state;

  XFontStruct
    *font_info;

  CommandWidgetEvent
    tile;

  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  font_info=windows->command.font_info;
  height=(unsigned int) (font_info->ascent+font_info->descent);
  id=(~0);
  state=DefaultState;
  if (event == (XEvent *) NULL)
    {
      CommandSizing
        sizing;

      /*
        Determine command window attributes.
      */
      assert(selections != (const char **) NULL);
      sizing.display=display;
      sizing.windows=windows;
      sizing.selections=selections;
      sizing.font_info=font_info;
      sizing.height=height;
      number_selections=SizeCommandWindow(&sizing);
      ReconfigureCommandWindow(display,windows);
      /*
        Allocate selection info memory.
      */
      if (selection_info != (XWidgetInfo *) NULL)
        selection_info=(XWidgetInfo *) RelinquishMagickMemory(selection_info);
      selection_info=(XWidgetInfo *) AcquireQuantumMemory(number_selections,
        sizeof(*selection_info));
      if (selection_info == (XWidgetInfo *) NULL)
        {
          ThrowXWindowFatalException(ResourceLimitFatalError,
            "MemoryAllocationFailed","...");
          return(id);
        }
      state|=(int) UpdateConfigurationState | (int) RedrawWidgetState;
    }
  /*
    Wait for next event.
  */
  tile.display=display;
  tile.windows=windows;
  tile.event=event;
  tile.number_selections=number_selections;
  HandleCommandEvent(&tile,&id,&state);
  if (state & UpdateConfigurationState)
    {
      CommandLayout
        layout;

      /*
        Initialize button information.
      */
      assert(selections != (const char **) NULL);
      layout.display=display;
      layout.windows=windows;
      layout.selections=selections;
      layout.font_info=font_info;
      layout.height=height;
      layout.number_selections=number_selections;
      InitializeCommandButtons(&layout);
    }
  if (state & RedrawWidgetState)
    RedrawCommandButtons(display,windows,number_selections);
  return(id);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X C o n f i r m W i d g e t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XConfirmWidget() displays a Confirm widget with a notice to the user. The
%  function returns -1 if Dismiss is pressed, 0 for Cancel, and 1 for Yes.
%
%  The format of the XConfirmWidget method is:
%
%      int XConfirmWidget(Display *display,XWindows *windows,
%        const char *reason,const char *description)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o reason: Specifies the message to display before terminating the
%      program.
%
%    o description: Specifies any description to the message.
%
*/
typedef struct _ConfirmRelease
{
  XWindows
    *windows;

  XEvent
    event;

  int
    *confirm;
} ConfirmRelease;

static size_t ReleaseConfirmChoice(const ConfirmRelease *release,
  const XWidgetInfo button_info,const int choice,size_t state)
{
  XWindows
    *const windows = release->windows;

  const XEvent
    event = release->event;

  int
    *const confirm = release->confirm;

  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(button_info,event.xbutton)))
    {
      (*confirm)=choice;
      state|=ExitState;
    }
  return(state);
}

static size_t ClientWidgetMessage(Display *display,XWindows *windows,
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

typedef struct _ConfirmTargets
{
  XWidgetInfo
    *cancel_info,
    *dismiss_info,
    *yes_info;

  size_t
    *state;

  int
    *confirm;
} ConfirmTargets;

static void PressConfirmButton(Display *display,XWindows *windows,
  const XEvent event,const ConfirmTargets *targets)
{
  XWidgetInfo
    *const cancel_info = targets->cancel_info,
    *const dismiss_info = targets->dismiss_info,
    *const yes_info = targets->yes_info;

  if (MatteIsActive((*cancel_info),event.xbutton))
    {
      /*
        User pressed No button.
      */
      (*cancel_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      return;
    }
  if (MatteIsActive((*dismiss_info),event.xbutton))
    {
      /*
        User pressed Dismiss button.
      */
      (*dismiss_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      return;
    }
  if (MatteIsActive((*yes_info),event.xbutton))
    {
      /*
        User pressed Yes button.
      */
      (*yes_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,yes_info);
      return;
    }
}

static void ReleaseConfirmButtons(Display *display,XWindows *windows,
  const XEvent event,const ConfirmTargets *targets)
{
  XWidgetInfo
    *const cancel_info = targets->cancel_info,
    *const dismiss_info = targets->dismiss_info,
    *const yes_info = targets->yes_info;

  size_t
    *const state = targets->state;

  int
    *const confirm = targets->confirm;

  ConfirmRelease
    release;

  if (windows->widget.mapped == MagickFalse)
    return;
  release.windows=windows;
  release.event=event;
  release.confirm=(confirm);
  if ((*cancel_info).raised == MagickFalse)
    {
      (*state)=ReleaseConfirmChoice(&release,(*cancel_info),0,(*state));
      (*cancel_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
    }
  if ((*dismiss_info).raised == MagickFalse)
    {
      (*state)=ReleaseConfirmChoice(&release,(*dismiss_info),(-1),(*state));
      (*dismiss_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
    }
  if ((*yes_info).raised == MagickFalse)
    {
      (*state)=ReleaseConfirmChoice(&release,(*yes_info),1,(*state));
      (*yes_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,yes_info);
    }
}

static void KeyConfirmYes(Display *display,XWindows *windows,const XEvent event,
  const ConfirmTargets *targets)
{
  XWidgetInfo
    *const yes_info = targets->yes_info;

  int
    *const confirm = targets->confirm;

  size_t
    *const state = targets->state;

  static char
    command[MagickPathExtent];

  static KeySym
    key_symbol;

  /*
    Respond to a user key press.
  */
  if (event.xkey.window != windows->widget.id)
    return;
  (void) XLookupString((XKeyEvent *) &event.xkey,command,
    (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
    {
      (*yes_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,yes_info);
      (*confirm)=1;
      (*state)|=ExitState;
      return;
    }
}

static void MoveConfirmYes(Display *display,XWindows *windows,
  const XEvent event,const ConfirmTargets *targets)
{
  XWidgetInfo
    *const yes_info = targets->yes_info;

  if ((*yes_info).raised == MatteIsActive((*yes_info),event.xmotion))
    {
      /*
        Yes button status changed.
      */
      (*yes_info).raised=(*yes_info).raised == MagickFalse ?
        MagickTrue : MagickFalse;
      XDrawBeveledButton(display,&windows->widget,yes_info);
      return;
    }
}

static void MoveConfirmButtons(Display *display,XWindows *windows,XEvent event,
  const ConfirmTargets *targets)
{
  XWidgetInfo
    *const cancel_info = targets->cancel_info,
    *const dismiss_info = targets->dismiss_info;

  size_t
    *const state = targets->state;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
  if ((*state) & InactiveWidgetState)
    return;
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
  if ((*dismiss_info).raised == MatteIsActive((*dismiss_info),event.xmotion))
    {
      /*
        Dismiss button status changed.
      */
      (*dismiss_info).raised=(*dismiss_info).raised == MagickFalse ?
        MagickTrue : MagickFalse;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      return;
    }
  MoveConfirmYes(display,windows,event,targets);
}

#define CancelButtonText  "Cancel"
#define DismissButtonText  "Dismiss"
#define YesButtonText  "Yes"

typedef struct _ConfirmContext
{
  XWindows
    *windows;

  XFontStruct
    *font_info;

  unsigned int
    height;

  const char
    *description;

  Display
    *display;

  const char
    *reason;
} ConfirmContext;

static void UpdateConfirmButtons(const ConfirmContext *context,
  const ConfirmTargets *targets)
{
  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  const char
    *const description = context->description;

  XWidgetInfo
    *const cancel_info = targets->cancel_info,
    *const dismiss_info = targets->dismiss_info,
    *const yes_info = targets->yes_info;

  size_t
    *const state = targets->state;

  if ((*state) & UpdateConfigurationState)
    {
      /*
        Initialize button information.
      */
      XGetWidgetInfo(CancelButtonText,cancel_info);
      (*cancel_info).width=(unsigned int) QuantumMargin+
        WidgetTextWidth(font_info,CancelButtonText);
      (*cancel_info).height=(unsigned int) ((3*height) >> 1);
      (*cancel_info).x=(int) windows->widget.width-(int) (*cancel_info).width-
        QuantumMargin;
      (*cancel_info).y=(int) (windows->widget.height-
        ((*cancel_info).height << 1));
      (*dismiss_info)=(*cancel_info);
      (*dismiss_info).text=(char *) DismissButtonText;
      if (LocaleCompare(description,"Do you want to save it") == 0)
        (*dismiss_info).text=(char *) "Don't Save";
      (*dismiss_info).width=(unsigned int) QuantumMargin+
        WidgetTextWidth(font_info,(*dismiss_info).text);
      (*dismiss_info).x=(int)
        ((windows->widget.width >> 1)-((*dismiss_info).width >> 1));
      (*yes_info)=(*cancel_info);
      (*yes_info).text=(char *) YesButtonText;
      if (LocaleCompare(description,"Do you want to save it") == 0)
        (*yes_info).text=(char *) "Save";
      (*yes_info).width=(unsigned int) QuantumMargin+
        WidgetTextWidth(font_info,(*yes_info).text);
      if ((*yes_info).width < (*cancel_info).width)
        (*yes_info).width=(*cancel_info).width;
      (*yes_info).x=QuantumMargin;
      (*state)&=(unsigned int) (~UpdateConfigurationState);
    }
}

static void RedrawConfirmWidget(const ConfirmContext *context,
  const ConfirmTargets *targets)
{
  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  const char
    *const description = context->description;

  Display
    *const display = context->display;

  const char
    *const reason = context->reason;

  XWidgetInfo
    *const cancel_info = targets->cancel_info,
    *const dismiss_info = targets->dismiss_info,
    *const yes_info = targets->yes_info;

  size_t
    *const state = targets->state;

  int
    x,
    y;

  unsigned int
    width;

  if ((*state) & RedrawWidgetState)
    {
      /*
        Redraw Confirm widget.
      */
      width=WidgetTextWidth(font_info,(char *) reason);
      x=(int) ((windows->widget.width >> 1)-(width >> 1));
      y=(int) ((windows->widget.height >> 1)-(height << 1));
      (void) XDrawString(display,windows->widget.id,
        windows->widget.annotate_context,x,y,(char *) reason,Extent(reason));
      if (description != (char *) NULL)
        {
          char
            question[MagickPathExtent];

          (void) CopyMagickString(question,description,MagickPathExtent);
          (void) ConcatenateMagickString(question,"?",MagickPathExtent);
          width=WidgetTextWidth(font_info,question);
          x=((int) (windows->widget.width >> 1)-(int) (width >> 1));
          y+=(int) height;
          (void) XDrawString(display,windows->widget.id,
            windows->widget.annotate_context,x,y,question,Extent(question));
        }
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      XDrawBeveledButton(display,&windows->widget,yes_info);
      XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
      (*state)&=(unsigned int) (~RedrawWidgetState);
    }
}

static void HandleConfirmKeyEvent(Display *display,XWindows *windows,
  const XEvent event,const ConfirmTargets *targets)
{
  size_t
    *const state = targets->state;

  switch (event.type)
  {
    case KeyPress:
    {
      KeyConfirmYes(display,windows,event,targets);
      break;
    }
    case LeaveNotify:
    {
      (*state)=LeaveWidgetWindow(windows,event,(*state));
      break;
    }
    case MotionNotify:
    {
      MoveConfirmButtons(display,windows,event,targets);
      break;
    }
    default:
      break;
  }
}

static void HandleConfirmEvent(Display *display,XWindows *windows,
  const XEvent event,const ConfirmTargets *targets)
{
  size_t
    *const state = targets->state;

  switch (event.type)
  {
    case ButtonPress:
    {
      PressConfirmButton(display,windows,event,targets);
      break;
    }
    case ButtonRelease:
    {
      ReleaseConfirmButtons(display,windows,event,targets);
      break;
    }
    case ClientMessage:
    {
      (*state)=ClientWidgetMessage(display,windows,event,(*state));
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
      HandleConfirmKeyEvent(display,windows,event,targets);
      break;
    }
  }
}

static void MapConfirmWidget(Display *display,XWindows *windows)
{
  Status
    status;

  XTextProperty
    window_name;

  XWindowChanges
    window_changes;

  (void) CopyMagickString(windows->widget.name,"Confirm",MagickPathExtent);
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
    (unsigned int) (CWWidth | CWHeight | CWX | CWY),&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
}

typedef struct _ConfirmLayout
{
  Display
    *display;

  XFontStruct
    *font_info;

  unsigned int
    width,
    height;
} ConfirmLayout;

static void PositionConfirmWidget(XWindows *windows,
  const ConfirmLayout *layout)
{
  Display
    *const display = layout->display;

  XFontStruct
    *const font_info = layout->font_info;

  const unsigned int
    width = layout->width,
    height = layout->height;

  /*
    Position Confirm widget.
  */
  windows->widget.width=(unsigned int) ((int) width+9*QuantumMargin);
  windows->widget.min_width=9*(unsigned int) QuantumMargin+
    WidgetTextWidth(font_info,CancelButtonText)+
    WidgetTextWidth(font_info,DismissButtonText)+
    WidgetTextWidth(font_info,YesButtonText);
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int) (12*height);
  windows->widget.min_height=(unsigned int) (7*height);
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
}

MagickPrivate int XConfirmWidget(Display *display,XWindows *windows,
  const char *reason,const char *description)
{
  int
    confirm;

  unsigned int
    height,
    width;

  size_t
    state;

  XEvent
    event;

  XFontStruct
    *font_info;

  XWidgetInfo
    cancel_info,
    dismiss_info,
    yes_info;

  const ConfirmTargets
    targets = { &cancel_info, &dismiss_info, &yes_info, &state, &confirm };

  ConfirmContext
    context;

  ConfirmLayout
    layout;
  /*
    Determine Confirm widget attributes.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(reason != (char *) NULL);
  assert(description != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",reason);
  XCheckRefreshWindows(display,windows);
  font_info=windows->widget.font_info;
  width=WidgetTextWidth(font_info,CancelButtonText);
  if (WidgetTextWidth(font_info,DismissButtonText) > width)
    width=WidgetTextWidth(font_info,DismissButtonText);
  if (WidgetTextWidth(font_info,YesButtonText) > width)
    width=WidgetTextWidth(font_info,YesButtonText);
  width<<=1;
  if (description != (char *) NULL)
    if (WidgetTextWidth(font_info,(char *) description) > width)
      width=WidgetTextWidth(font_info,(char *) description);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  layout.display=display;
  layout.font_info=font_info;
  layout.width=width;
  layout.height=height;
  PositionConfirmWidget(windows,&layout);
  /*
    Map Confirm widget.
  */
  MapConfirmWidget(display,windows);
  /*
    Respond to X events.
  */
  confirm=0;
  state=UpdateConfigurationState;
  XSetCursorState(display,windows,MagickTrue);
  context.windows=windows;
  context.font_info=font_info;
  context.height=height;
  context.description=description;
  context.display=display;
  context.reason=reason;
  do
  {
    UpdateConfirmButtons(&context,&targets);
    RedrawConfirmWidget(&context,&targets);
    /*
      Wait for next event.
    */
    (void) XIfEvent(display,&event,XScreenEvent,(char *) windows);
    HandleConfirmEvent(display,windows,event,&targets);
  } while ((state & ExitState) == 0);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
  return(confirm);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X D i a l o g W i d g e t                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDialogWidget() displays a Dialog widget with a query to the user.  The user
%  keys a reply and presses the Ok or Cancel button to exit.  The typed text is
%  returned as the reply function parameter.
%
%  The format of the XDialogWidget method is:
%
%      int XDialogWidget(Display *display,XWindows *windows,const char *action,
%        const char *query,char *reply)
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
%    o query: Specifies a pointer to the query to present to the user.
%
%    o reply: the response from the user is returned in this parameter.
%
*/
typedef struct _DialogRelease
{
  Display
    *display;

  XWindows
    *windows;

  XEvent
    event;
} DialogRelease;

static void ReleaseDialogAction(const DialogRelease *release,
  XWidgetInfo *action_info,size_t *state)
{
  Display
    *const display = release->display;

  XWindows
    *const windows = release->windows;

  const XEvent
    event = release->event;

  if ((*action_info).raised == MagickFalse)
    {
      if ((event.xbutton.window == windows->widget.id) &&
          (MatteIsActive((*action_info),event.xbutton)))
        (*state)|=ExitState;
      (*action_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,&(*action_info));
    }
}

static void ReleaseDialogCancel(const DialogRelease *release,
  const XWidgetInfo *reply_info,XWidgetInfo *cancel_info,size_t *state)
{
  Display
    *const display = release->display;

  XWindows
    *const windows = release->windows;

  const XEvent
    event = release->event;

  if ((*cancel_info).raised == MagickFalse)
    {
      if ((event.xbutton.window == windows->widget.id) &&
          (MatteIsActive((*cancel_info),event.xbutton)))
        {
          *(*reply_info).text='\0';
          (*state)|=ExitState;
        }
      (*cancel_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,&(*cancel_info));
    }
}

typedef struct _DialogPress
{
  Display
    *display;

  XWindows
    *windows;

  XEvent
    event;

  XFontStruct
    *font_info;
} DialogPress;

static void PressDialogReply(const DialogPress *press,char *primary_selection,
  XWidgetInfo *reply_info)
{
  Display
    *const display = press->display;

  XWindows
    *const windows = press->windows;

  const XEvent
    event = press->event;

  XFontStruct
    *const font_info = press->font_info;

  int
    i,
    x;

  static Time
    click_time;

  /*
    Move text cursor to position of button press.
  */
  x=event.xbutton.x-(*reply_info).x-(int) (QuantumMargin >> 2);
  for (i=1; i <= Extent((*reply_info).marker); i++)
    if (XTextWidth(font_info,(*reply_info).marker,i) > x)
      break;
  (*reply_info).cursor=(*reply_info).marker+i-1;
  if (event.xbutton.time > (click_time+(unsigned long) DoubleClick))
    (*reply_info).highlight=MagickFalse;
  else
    {
      /*
        Become the XA_PRIMARY selection owner.
      */
      (void) CopyMagickString(primary_selection,(*reply_info).text,
        MagickPathExtent);
      (void) XSetSelectionOwner(display,XA_PRIMARY,windows->widget.id,
        event.xbutton.time);
      (*reply_info).highlight=XGetSelectionOwner(display,XA_PRIMARY) ==
        windows->widget.id ? MagickTrue : MagickFalse;
    }
  XDrawMatteText(display,&windows->widget,reply_info);
  click_time=event.xbutton.time;
}

static void ApplyDialogControlKey(const KeySym key_symbol,const size_t state,
  XWidgetInfo *reply_info)
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
        *(*reply_info).text='\0';
        (*reply_info).cursor=(*reply_info).text;
        (*reply_info).marker=(*reply_info).text;
        (*reply_info).highlight=MagickFalse;
        break;
      }
      default:
        break;
    }
}

#define CancelButtonText  "Cancel"

typedef struct _DialogTargets
{
  XWidgetInfo
    *special_info,
    *action_info,
    *cancel_info,
    *reply_info;

  size_t
    *state;

  XWidgetInfo
    *text_info;
} DialogTargets;

typedef struct _DialogContext
{
  Display
    *display;

  XWindows
    *windows;

  XFontStruct
    *font_info;

  unsigned int
    anomaly;

  char
    *primary_selection;

  const char
    *action,
    *query;

  unsigned int
    height;

  MagickBooleanType
    raised;

  char
    *reply;

  unsigned int
    width;
} DialogContext;

static void PressDialogButton(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    anomaly = context->anomaly;

  char
    *const primary_selection = context->primary_selection;

  XWidgetInfo
    *const special_info = targets->special_info,
    *const action_info = targets->action_info,
    *const cancel_info = targets->cancel_info,
    *const reply_info = targets->reply_info;

  if ((anomaly) && (MatteIsActive((*special_info),event.xbutton)))
    {
      /*
        Option button status changed.
      */
      (*special_info).raised=!(*special_info).raised;
      XDrawBeveledButton(display,&windows->widget,special_info);
      return;
    }
  if (MatteIsActive((*action_info),event.xbutton))
    {
      /*
        User pressed Action button.
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
      DialogPress
        press;

      press.display=display;
      press.windows=windows;
      press.event=event;
      press.font_info=font_info;
      PressDialogReply(&press,primary_selection,reply_info);
      return;
    }
  /*
    Request primary selection.
  */
  (void) XConvertSelection(display,XA_PRIMARY,XA_STRING,XA_STRING,
    windows->widget.id,event.xbutton.time);
}

static void ReleaseDialogButtons(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const action_info = targets->action_info,
    *const cancel_info = targets->cancel_info,
    *const reply_info = targets->reply_info;

  size_t
    *const state = targets->state;

  DialogRelease
    release;

  if (windows->widget.mapped == MagickFalse)
    return;
  release.display=display;
  release.windows=windows;
  release.event=event;
  ReleaseDialogAction(&release,action_info,state);
  release.display=display;
  release.windows=windows;
  release.event=event;
  ReleaseDialogCancel(&release,reply_info,cancel_info,state);
}

static void ClientDialogMessage(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const reply_info = targets->reply_info;

  size_t
    *const state = targets->state;

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

static void KeyDialogPress(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const action_info = targets->action_info,
    *const reply_info = targets->reply_info;

  size_t
    *const state = targets->state;

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
  if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
    {
      (*action_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,action_info);
      (*state)|=ExitState;
      return;
    }
  if (key_symbol == XK_Control_L)
    {
      (*state)|=ControlState;
      return;
    }
  ApplyDialogControlKey(key_symbol,(*state),reply_info);
  XEditText(display,reply_info,key_symbol,command,(*state));
  XDrawMatteText(display,&windows->widget,reply_info);
}

static void KeyDialogRelease(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = targets->state;

  static char
    command[MagickPathExtent];

  static KeySym
    key_symbol;

  /*
    Respond to a user key release.
  */
  if (event.xkey.window != windows->widget.id)
    return;
  (void) XLookupString((XKeyEvent *) &event.xkey,command,
    (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  if (key_symbol == XK_Control_L)
    (*state)&=(unsigned int) (~ControlState);
}

static void MoveDialogButtons(const DialogContext *context,XEvent event,
  const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const action_info = targets->action_info,
    *const cancel_info = targets->cancel_info;

  size_t
    *const state = targets->state;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
  if ((*state) & InactiveWidgetState)
    return;
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

static void SelectDialogReply(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const reply_info = targets->reply_info;

  size_t
    *const state = targets->state;

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
    }
  (void) XFree((void *) data);
}

static void InitializeDialogOptions(const DialogContext *context,
  const DialogTargets *targets)
{
  XFontStruct
    *const font_info = context->font_info;

  const char
    *const action = context->action,
    *const query = context->query;

  const unsigned int
    height = context->height;

  const MagickBooleanType
    raised = context->raised;

  XWidgetInfo
    *const special_info = targets->special_info,
    *const action_info = targets->action_info,
    *const reply_info = targets->reply_info,
    *const text_info = targets->text_info;

  /*
    Initialize option information.
  */
  XGetWidgetInfo("Dither",special_info);
  (*special_info).raised=raised;
  (*special_info).bevel_width--;
  (*special_info).width=(unsigned int) QuantumMargin >> 1;
  (*special_info).height=(unsigned int) QuantumMargin >> 1;
  (*special_info).x=(*reply_info).x;
  (*special_info).y=(*action_info).y+(int) (*action_info).height-(int)
    (*special_info).height;
  if (LocaleCompare(action,"Background") == 0)
    (*special_info).text=(char *) "Backdrop";
  if (LocaleCompare(action,"New") == 0)
    (*special_info).text=(char *) "Gradation";
  if (LocaleCompare(action,"Resize") == 0)
    (*special_info).text=(char *) "Constrain ratio";
  if (LocaleCompare(action,"Save") == 0)
    (*special_info).text=(char *) "Non-progressive";
  if (LocaleCompare(action,"Shade") == 0)
    (*special_info).text=(char *) "Color shading";
  /*
    Initialize text information.
  */
  XGetWidgetInfo(query,text_info);
  (*text_info).width=(*reply_info).width;
  (*text_info).height=height;
  (*text_info).x=(*reply_info).x-(int) (QuantumMargin >> 1);
  (*text_info).y=QuantumMargin;
  (*text_info).center=MagickFalse;
}

static void UpdateDialogLayout(const DialogContext *context,
  const DialogTargets *targets)
{
  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  const char
    *const action = context->action;

  char
    *const reply = context->reply;

  const unsigned int
    width = context->width,
    height = context->height;

  XWidgetInfo
    *const action_info = targets->action_info,
    *const cancel_info = targets->cancel_info,
    *const reply_info = targets->reply_info;

  size_t
    *const state = targets->state;

  if ((*state) & UpdateConfigurationState)
    {
      /*
        Initialize button information.
      */
      XGetWidgetInfo(CancelButtonText,cancel_info);
      (*cancel_info).width=width;
      (*cancel_info).height=(unsigned int) ((3*height) >> 1);
      (*cancel_info).x=(int) windows->widget.width-(int) (*cancel_info).width-
        ((3*QuantumMargin) >> 1);
      (*cancel_info).y=(int) windows->widget.height-(int) (*cancel_info).height-
        ((3*QuantumMargin) >> 1);
      XGetWidgetInfo(action,action_info);
      (*action_info).width=width;
      (*action_info).height=(unsigned int) ((3*height) >> 1);
      (*action_info).x=(*cancel_info).x-((int) (*cancel_info).width+QuantumMargin+
        (int) ((*action_info).bevel_width << 1));
      (*action_info).y=(*cancel_info).y;
      /*
        Initialize reply information.
      */
      XGetWidgetInfo(reply,reply_info);
      (*reply_info).raised=MagickFalse;
      (*reply_info).bevel_width--;
      (*reply_info).width=(unsigned int) ((int) windows->widget.width-
        (3*QuantumMargin));
      (*reply_info).height=height << 1;
      (*reply_info).x=(3*QuantumMargin) >> 1;
      (*reply_info).y=(*action_info).y-(int) (*reply_info).height-QuantumMargin;
      InitializeDialogOptions(context,targets);
      (*state)&=(unsigned int) (~UpdateConfigurationState);
    }
}

static void RedrawDialogWidget(const DialogContext *context,
  const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    anomaly = context->anomaly;

  XWidgetInfo
    *const special_info = targets->special_info,
    *const action_info = targets->action_info,
    *const cancel_info = targets->cancel_info,
    *const reply_info = targets->reply_info;

  size_t
    *const state = targets->state;

  XWidgetInfo
    *const text_info = targets->text_info;

  if ((*state) & RedrawWidgetState)
    {
      /*
        Redraw Dialog widget.
      */
      XDrawWidgetText(display,&windows->widget,text_info);
      XDrawBeveledMatte(display,&windows->widget,reply_info);
      XDrawMatteText(display,&windows->widget,reply_info);
      if (anomaly)
        XDrawBeveledButton(display,&windows->widget,special_info);
      XDrawBeveledButton(display,&windows->widget,action_info);
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
      (*state)&=(unsigned int) (~RedrawWidgetState);
    }
}

static void HandleDialogSelectionEvent(const DialogContext *context,
  XEvent event,const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const primary_selection = context->primary_selection;

  XWidgetInfo
    *const reply_info = targets->reply_info;

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
      SelectDialogReply(context,event,targets);
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
      notify.display=request->display;
      notify.requestor=request->requestor;
      notify.selection=request->selection;
      notify.target=request->target;
      notify.time=request->time;
      if (request->property == None)
        notify.property=request->target;
      else
        notify.property=request->property;
      (void) XSendEvent(request->display,request->requestor,False,0,
        (XEvent *) &notify);
    }
    default:
      break;
  }
}

static void HandleDialogKeyEvent(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = targets->state;

  switch (event.type)
  {
    case KeyPress:
    {
      KeyDialogPress(context,event,targets);
      break;
    }
    case KeyRelease:
    {
      KeyDialogRelease(context,event,targets);
      break;
    }
    case LeaveNotify:
    {
      (*state)=LeaveWidgetWindow(windows,event,(*state));
      break;
    }
    case MotionNotify:
    {
      MoveDialogButtons(context,event,targets);
      break;
    }
    default:
    {
      HandleDialogSelectionEvent(context,event,targets);
      break;
    }
  }
}

static void HandleDialogEvent(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = targets->state;

  switch (event.type)
  {
    case ButtonPress:
    {
      PressDialogButton(context,event,targets);
      break;
    }
    case ButtonRelease:
    {
      ReleaseDialogButtons(context,event,targets);
      break;
    }
    case ClientMessage:
    {
      ClientDialogMessage(context,event,targets);
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
      HandleDialogKeyEvent(context,event,targets);
      break;
    }
  }
}

typedef struct _DialogLayout
{
  Display
    *display;

  XFontStruct
    *font_info;

  unsigned int
    width,
    height;

  const char
    *query;

  char
    *reply;
} DialogLayout;

static void PositionDialogWidget(XWindows *windows,
  const DialogLayout *layout)
{
  Display
    *const display = layout->display;

  XFontStruct
    *const font_info = layout->font_info;

  const unsigned int
    width = layout->width,
    height = layout->height;

  const char
    *const query = layout->query;

  char
    *const reply = layout->reply;

  /*
    Position Dialog widget.
  */
  windows->widget.width=(unsigned int) MagickMax((int) (2*width),(int)
    WidgetTextWidth(font_info,(char *) query));
  if (windows->widget.width < WidgetTextWidth(font_info,reply))
    windows->widget.width=WidgetTextWidth(font_info,reply);
  windows->widget.width+=(unsigned int) (6*QuantumMargin);
  windows->widget.min_width=(unsigned int)
    ((int) width+28*XTextWidth(font_info,"#",1)+4*QuantumMargin);
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int) (7*(int) height+(QuantumMargin << 1));
  windows->widget.min_height=windows->widget.height;
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
}

static void MapDialogWidget(Display *display,XWindows *windows)
{
  Status
    status;

  XTextProperty
    window_name;

  XWindowChanges
    window_changes;

  (void) CopyMagickString(windows->widget.name,"Dialog",MagickPathExtent);
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
    (unsigned int) (CWWidth | CWHeight | CWX | CWY),&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
}

static void RespondToDialogEvents(const DialogContext *context,
  const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = targets->state;

  XEvent
    event;

  do
  {
    UpdateDialogLayout(context,targets);
    RedrawDialogWidget(context,targets);
    /*
      Wait for next event.
    */
    (void) XIfEvent(display,&event,XScreenEvent,(char *) windows);
    HandleDialogEvent(context,event,targets);
  } while (((*state) & ExitState) == 0);
}

static unsigned int GetDialogButtonWidth(XFontStruct *font_info,
  const char *action)
{
  unsigned int
    width;

  width=WidgetTextWidth(font_info,(char *) action);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  width+=(unsigned int) (3*QuantumMargin) >> 1;
  return(width);
}

static MagickBooleanType IsDialogOptionAction(const char *action)
{
  return(((LocaleCompare(action,"Background") == 0) ||
    (LocaleCompare(action,"New") == 0) ||
    (LocaleCompare(action,"Quantize") == 0) ||
    (LocaleCompare(action,"Resize") == 0) ||
    (LocaleCompare(action,"Save") == 0) ||
    (LocaleCompare(action,"Shade") == 0)) ? MagickTrue : MagickFalse);
}

MagickPrivate int XDialogWidget(Display *display,XWindows *windows,
  const char *action,const char *query,char *reply)
{
  char
    primary_selection[MagickPathExtent];

  static MagickBooleanType
    raised = MagickFalse;

  unsigned int
    anomaly,
    height,
    width;

  size_t
    state;

  XFontStruct
    *font_info;

  XWidgetInfo
    action_info,
    cancel_info,
    reply_info,
    special_info,
    text_info;

  const DialogTargets
    targets = { &special_info, &action_info, &cancel_info, &reply_info,
      &state, &text_info };

  DialogContext
    context;

  /*
    Determine Dialog widget attributes.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(action != (char *) NULL);
  assert(query != (char *) NULL);
  assert(reply != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XCheckRefreshWindows(display,windows);
  font_info=windows->widget.font_info;
  width=GetDialogButtonWidth(font_info,action);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  {
    const DialogLayout
      layout = { display, font_info, width, height, query, reply };

    PositionDialogWidget(windows,&layout);
  }
  /*
    Map Dialog widget.
  */
  MapDialogWidget(display,windows);
  /*
    Respond to X events.
  */
  anomaly=(IsDialogOptionAction(action) != MagickFalse);
  state=UpdateConfigurationState;
  XSetCursorState(display,windows,MagickTrue);
  context.display=display;
  context.windows=windows;
  context.font_info=font_info;
  context.anomaly=anomaly;
  context.primary_selection=primary_selection;
  context.action=action;
  context.query=query;
  context.height=height;
  context.raised=raised;
  context.reply=reply;
  context.width=width;
  RespondToDialogEvents(&context,&targets);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
  if ((anomaly) && (special_info.raised))
    if (*reply != '\0')
      raised=MagickTrue;
  return(raised == MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X F i l e B r o w s e r W i d g e t                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XFileBrowserWidget() displays a File Browser widget with a file query to the
%  user.  The user keys a reply and presses the Action or Cancel button to
%  exit.  The typed text is returned as the reply function parameter.
%
%  The format of the XFileBrowserWidget method is:
%
%      void XFileBrowserWidget(Display *display,XWindows *windows,
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
typedef struct _FileBrowserTargets
{
  XWidgetInfo
    *reply_info;

  size_t
    *state;

  XWidgetInfo
    *selection_info,
    *list_info;
} FileBrowserTargets;

typedef struct _FileBrowserPress
{
  Display
    *display;

  XWindows
    *windows;

  char
    **filelist;

  size_t
    files;

  unsigned int
    height;

  XEvent
    event;

  XWidgetInfo
    slider_info;

  char
    *working_path,
    *reply;
} FileBrowserPress;

static void SelectFileBrowserEntry(const FileBrowserPress *press,
  const FileBrowserTargets *targets)
{
  Display
    *const display = press->display;

  XWindows
    *const windows = press->windows;

  char
    **const filelist = press->filelist;

  const size_t
    files = press->files;

  const unsigned int
    height = press->height;

  const XEvent
    event = press->event;

  const XWidgetInfo
    slider_info = press->slider_info;

  char
    *const working_path = press->working_path,
    *const reply = press->reply;

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
    User pressed file matte.
  */
  id=slider_info.id+(event.xbutton.y-((*list_info).y+(int)
    (height >> 1))+1)/(int) (*selection_info).height;
  if (id >= (int) files)
    return;
  (void) CopyMagickString((*reply_info).text,filelist[id],MagickPathExtent);
  (*reply_info).highlight=MagickFalse;
  (*reply_info).marker=(*reply_info).text;
  (*reply_info).cursor=(*reply_info).text+Extent((*reply_info).text);
  XDrawMatteText(display,&windows->widget,reply_info);
  if (id == (*list_info).id)
    {
      char
        *p;

      p=(*reply_info).text+strlen((*reply_info).text)-1;
      if (*p == *DirectorySeparator)
        ChopPathComponents((*reply_info).text,1);
      (void) ConcatenateMagickString(working_path,DirectorySeparator,
        MagickPathExtent);
      (void) ConcatenateMagickString(working_path,(*reply_info).text,
        MagickPathExtent);
      *reply='\0';
      (*state)|=UpdateListState;
    }
  (*selection_info).id=(~0);
  (*list_info).id=id;
  (*state)|=RedrawListState;
}

static void CopyFileBrowserFormat(char *reply,char *format)
{
  char
    *p;

  p=reply+Extent(reply)-1;
  while ((p > (reply+1)) && (*(p-1) != '.'))
    p--;
  if ((p > (reply+1)) && (*(p-1) == '.'))
    (void) CopyMagickString(format,p,MagickPathExtent);
}

static XWidgetInfo
  TrackFontBrowserSlider(XWidgetInfo,const XEvent,const int);

static ssize_t FindFileBrowserCursor(XFontStruct *font_info,
  const XWidgetInfo reply_info,const int x)
{
  ssize_t
    i;

  for (i=1; i <= (ssize_t) Extent(reply_info.marker); i++)
    if (XTextWidth(font_info,reply_info.marker,(int) i) > x)
      break;
  return(i);
}

typedef struct _FileBrowserMatch
{
  char
    **filelist;

  size_t
    files;

  char
    *reply;
} FileBrowserMatch;

static void MatchFileBrowserReply(const FileBrowserMatch *match,
  XWidgetInfo *list_info,ssize_t *i)
{
  char
    **const filelist = match->filelist;

  const size_t
    files = match->files;

  char
    *const reply = match->reply;

  for ((*i)=0; (*i) < (ssize_t) files; (*i)++)
    if (LocaleCompare(filelist[(*i)],reply) >= 0)
      {
        (*list_info).id=(int)
          (LocaleCompare(filelist[(*i)],reply) == 0 ? (*i) : ~0);
        break;
      }
}

typedef struct _FileBrowserNames
{
  Display
    *display;

  XWindows
    *windows;

  char
    **filelist;

  size_t
    files;

  unsigned int
    visible_files;

  XWidgetInfo
    list_info,
    slider_info;
} FileBrowserNames;

static void DrawFileBrowserNames(const FileBrowserNames *names,
  XWidgetInfo *selection_info)
{
  Display
    *const display = names->display;

  XWindows
    *const windows = names->windows;

  char
    **const filelist = names->filelist;

  const size_t
    files = names->files;

  const unsigned int
    visible_files = names->visible_files;

  const XWidgetInfo
    list_info = names->list_info,
    slider_info = names->slider_info;

  ssize_t
    i;

  for (i=0; i < (ssize_t) visible_files; i++)
  {
    (*selection_info).raised=(int) (slider_info.id+i) != list_info.id ?
      MagickTrue : MagickFalse;
    (*selection_info).text=(char *) NULL;
    if ((slider_info.id+i) < (ssize_t) files)
      (*selection_info).text=filelist[slider_info.id+i];
    XDrawWidgetText(display,&windows->widget,selection_info);
    (*selection_info).y+=(int) (*selection_info).height;
  }
}

typedef struct _FileBrowserPathRelease
{
  XWindows
    *windows;

  XEvent
    event;

  char
    *working_path;
} FileBrowserPathRelease;

static size_t ReleaseFileBrowserUp(const FileBrowserPathRelease *up,
  const XWidgetInfo up_info,size_t state)
{
  XWindows
    *const windows = up->windows;

  const XEvent
    event = up->event;

  char
    *const working_path = up->working_path;

  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(up_info,event.xbutton)))
    {
      ChopPathComponents(working_path,1);
      if (*working_path == '\0')
        (void) CopyMagickString(working_path,DirectorySeparator,
          MagickPathExtent);
      state|=UpdateListState;
    }
  return(state);
}

typedef struct _FileBrowserCancel
{
  XWindows
    *windows;

  XEvent
    event;

  char
    *reply;
} FileBrowserCancel;

static size_t ReleaseFileBrowserCancel(const FileBrowserCancel *cancel,
  const XWidgetInfo cancel_info,const XWidgetInfo reply_info,size_t state)
{
  XWindows
    *const windows = cancel->windows;

  const XEvent
    event = cancel->event;

  char
    *const reply = cancel->reply;

  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(cancel_info,event.xbutton)))
    {
      *reply_info.text='\0';
      *reply='\0';
      state|=ExitState;
    }
  return(state);
}

typedef struct _FileBrowserPaths
{
  char
    *glob_pattern,
    *working_path,
    *reply;
} FileBrowserPaths;

static size_t ReadFileBrowserPath(const FileBrowserPaths *paths,
  const XWidgetInfo reply_info,size_t state)
{
  char
    *const glob_pattern = paths->glob_pattern,
    *const working_path = paths->working_path,
    *const reply = paths->reply;

  /*
    Read new directory or glob pattern.
  */
  if (*reply_info.text == '\0')
    return(state);
  if (IsGlob(reply_info.text))
    (void) CopyMagickString(glob_pattern,reply_info.text,
      MagickPathExtent);
  else
    {
      (void) ConcatenateMagickString(working_path,DirectorySeparator,
        MagickPathExtent);
      (void) ConcatenateMagickString(working_path,reply_info.text,
        MagickPathExtent);
      if (*working_path == '~')
        ExpandFilename(working_path);
      *reply='\0';
    }
  state|=UpdateListState;
  return(state);
}

typedef struct _FileBrowserFormatTargets
{
  XWidgetInfo
    *reply_info,
    *special_info;
} FileBrowserFormatTargets;

static void SelectFileBrowserFormat(Display *display,XWindows *windows,
  char *format,const FileBrowserFormatTargets *targets)
{
  XWidgetInfo
    *const reply_info = targets->reply_info,
    *const special_info = targets->special_info;

  char
    **formats;

  ExceptionInfo
    *exception;

  size_t
    number_formats;

  ssize_t
    i;

  /*
    Let user select image format.
  */
  exception=AcquireExceptionInfo();
  formats=GetMagickList("*",&number_formats,exception);
  exception=DestroyExceptionInfo(exception);
  if (formats == (char **) NULL)
    return;
  (void) XCheckDefineCursor(display,windows->widget.id,
    windows->widget.busy_cursor);
  windows->popup.x=windows->widget.x+60;
  windows->popup.y=windows->widget.y+60;
  XListBrowserWidget(display,windows,&windows->popup,
    (const char **) formats,"Select","Select image format type:",
    format);
  XSetCursorState(display,windows,MagickTrue);
  (void) XCheckDefineCursor(display,windows->widget.id,
    windows->widget.cursor);
  LocaleLower(format);
  AppendImageFormat(format,(*reply_info).text);
  (*reply_info).cursor=(*reply_info).text+Extent((*reply_info).text);
  XDrawMatteText(display,&windows->widget,reply_info);
  (*special_info).raised=MagickTrue;
  XDrawBeveledButton(display,&windows->widget,special_info);
  for (i=0; i < (ssize_t) number_formats; i++)
    formats[i]=DestroyString(formats[i]);
  formats=(char **) RelinquishMagickMemory(formats);
}

static size_t ReleaseFileBrowserGrab(const FileBrowserPathRelease *grab,
  const XWidgetInfo special_info,size_t state)
{
  XWindows
    *const windows = grab->windows;

  const XEvent
    event = grab->event;

  char
    *const working_path = grab->working_path;

  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(special_info,event.xbutton)))
    {
      (void) CopyMagickString(working_path,"x:",MagickPathExtent);
      state|=ExitState;
    }
  return(state);
}

typedef struct _FileBrowserSpecial
{
  unsigned int
    anomaly;

  Display
    *display;

  XWindows
    *windows;

  char
    *format;

  FileBrowserFormatTargets
    format_targets;

  XEvent
    event;

  char
    *working_path;
} FileBrowserSpecial;

typedef struct _FileBrowserSpecialTargets
{
  size_t
    *state;

  XWidgetInfo
    *special_info;
} FileBrowserSpecialTargets;

static MagickBooleanType ReleaseFileBrowserSpecial(
  const FileBrowserSpecial *special,const FileBrowserSpecialTargets *targets)
{
  const unsigned int
    anomaly = special->anomaly;

  Display
    *const display = special->display;

  XWindows
    *const windows = special->windows;

  char
    *const format = special->format;

  const FileBrowserFormatTargets
    format_targets = special->format_targets;

  const XEvent
    event = special->event;

  char
    *const working_path = special->working_path;

  size_t
    *const state = targets->state;

  XWidgetInfo
    *const special_info = targets->special_info;

  if ((*special_info).raised == MagickFalse)
    {
      FileBrowserPathRelease
        grab;

      if (anomaly == MagickFalse)
        {
          SelectFileBrowserFormat(display,windows,format,
            &format_targets);
          return(MagickFalse);
        }
      grab.windows=windows;
      grab.event=event;
      grab.working_path=working_path;
      (*state)=ReleaseFileBrowserGrab(&grab,(*special_info),(*state));
      (*special_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,special_info);
    }
  return(MagickTrue);
}

MagickPrivate void XFileBrowserWidget(Display *display,XWindows *windows,
  const char *action,char *reply)
{
#define CancelButtonText  "Cancel"
#define DirectoryText  "Directory:"
#define FilenameText  "File name:"
#define GrabButtonText  "Grab"
#define FormatButtonText  "Format"
#define HomeButtonText  "Home"
#define UpButtonText  "Up"

  char
    *directory,
    **filelist,
    home_directory[MagickPathExtent],
    primary_selection[MagickPathExtent],
    text[MagickPathExtent],
    working_path[MagickPathExtent];

  int
    x,
    y;

  static char
    glob_pattern[MagickPathExtent] = "*",
    format[MagickPathExtent] = "miff";

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  Status
    status;

  size_t
    delay,
    files,
    state;

  ssize_t
    i;

  unsigned int
    anomaly,
    height,
    text_width,
    visible_files,
    width;

  XEvent
    event;

  XFontStruct
    *font_info;

  XTextProperty
    window_name;

  XWidgetInfo
    action_info,
    cancel_info,
    expose_info,
    special_info,
    list_info,
    home_info,
    north_info,
    reply_info,
    scroll_info,
    selection_info,
    slider_info,
    south_info,
    text_info,
    up_info;

  XWindowChanges
    window_changes;

  const FileBrowserTargets
    targets = { &reply_info, &state, &selection_info, &list_info };

  const FileBrowserFormatTargets
    format_targets = { &reply_info, &special_info };

  const FileBrowserSpecialTargets
    special_targets = { &state, &special_info };

  /*
    Read filelist from current directory.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(action != (char *) NULL);
  assert(reply != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  directory=getcwd(home_directory,MagickPathExtent);
  (void) directory;
  (void) CopyMagickString(working_path,home_directory,MagickPathExtent);
  filelist=ListFiles(working_path,glob_pattern,&files);
  if (filelist == (char **) NULL)
    {
      /*
        Directory read failed.
      */
      XNoticeWidget(display,windows,"Unable to read directory:",working_path);
      (void) XDialogWidget(display,windows,action,"Enter filename:",reply);
      return;
    }
  /*
    Determine File Browser widget attributes.
  */
  font_info=windows->widget.font_info;
  text_width=0;
  for (i=0; i < (ssize_t) files; i++)
    if (WidgetTextWidth(font_info,filelist[i]) > text_width)
      text_width=WidgetTextWidth(font_info,filelist[i]);
  width=WidgetTextWidth(font_info,(char *) action);
  if (WidgetTextWidth(font_info,GrabButtonText) > width)
    width=WidgetTextWidth(font_info,GrabButtonText);
  if (WidgetTextWidth(font_info,FormatButtonText) > width)
    width=WidgetTextWidth(font_info,FormatButtonText);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  if (WidgetTextWidth(font_info,HomeButtonText) > width)
    width=WidgetTextWidth(font_info,HomeButtonText);
  if (WidgetTextWidth(font_info,UpButtonText) > width)
    width=WidgetTextWidth(font_info,UpButtonText);
  width+=(unsigned int) QuantumMargin;
  if (WidgetTextWidth(font_info,DirectoryText) > width)
    width=WidgetTextWidth(font_info,DirectoryText);
  if (WidgetTextWidth(font_info,FilenameText) > width)
    width=WidgetTextWidth(font_info,FilenameText);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position File Browser widget.
  */
  windows->widget.width=width+MagickMin(text_width,MaxTextWidth)+
    (unsigned int) (6*QuantumMargin);
  windows->widget.min_width=width+MinTextWidth+(unsigned int) (4*QuantumMargin);
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int)
    (((81*height) >> 2)+(unsigned int) ((13*QuantumMargin) >> 1)+4);
  windows->widget.min_height=(unsigned int)
    (((23*height) >> 1)+(unsigned int) ((13*QuantumMargin) >> 1)+4);
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
  /*
    Map File Browser widget.
  */
  (void) CopyMagickString(windows->widget.name,"Browse and Select a File",
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
  (void) XReconfigureWMWindow(display,windows->widget.id,
    windows->widget.screen,mask,&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
  /*
    Respond to X events.
  */
  XGetWidgetInfo((char *) NULL,&slider_info);
  XGetWidgetInfo((char *) NULL,&north_info);
  XGetWidgetInfo((char *) NULL,&south_info);
  XGetWidgetInfo((char *) NULL,&expose_info);
  visible_files=0;
  anomaly=(LocaleCompare(action,"Composite") == 0) ||
    (LocaleCompare(action,"Open") == 0) || (LocaleCompare(action,"Map") == 0);
  delay=SuspendTime << 2;
  state=UpdateConfigurationState;
  do
  {
    if (state & UpdateConfigurationState)
      {
        int
          id;

        /*
          Initialize button information.
        */
        XGetWidgetInfo(CancelButtonText,&cancel_info);
        cancel_info.width=width;
        cancel_info.height=(unsigned int) ((3*height) >> 1);
        cancel_info.x=(int) windows->widget.width-(int) cancel_info.width-
          QuantumMargin-2;
        cancel_info.y=(int) windows->widget.height-(int) cancel_info.height-
          QuantumMargin;
        XGetWidgetInfo(action,&action_info);
        action_info.width=width;
        action_info.height=(unsigned int) ((3*height) >> 1);
        action_info.x=cancel_info.x-((int) cancel_info.width+
          (QuantumMargin >> 1)+(int) (action_info.bevel_width << 1));
        action_info.y=cancel_info.y;
        XGetWidgetInfo(GrabButtonText,&special_info);
        special_info.width=width;
        special_info.height=(unsigned int) ((3*height) >> 1);
        special_info.x=action_info.x-((int) action_info.width+
          (QuantumMargin >> 1)+(int) (special_info.bevel_width << 1));
        special_info.y=action_info.y;
        if (anomaly == MagickFalse)
          {
            special_info.text=(char *) FormatButtonText;
            CopyFileBrowserFormat(reply,format);
          }
        XGetWidgetInfo(UpButtonText,&up_info);
        up_info.width=width;
        up_info.height=(unsigned int) ((3*height) >> 1);
        up_info.x=QuantumMargin;
        up_info.y=(int) ((5*QuantumMargin) >> 1)+(int) height;
        XGetWidgetInfo(HomeButtonText,&home_info);
        home_info.width=width;
        home_info.height=(unsigned int) ((3*height) >> 1);
        home_info.x=QuantumMargin;
        home_info.y=up_info.y+(int) up_info.height+QuantumMargin;
        /*
          Initialize reply information.
        */
        XGetWidgetInfo(reply,&reply_info);
        reply_info.raised=MagickFalse;
        reply_info.bevel_width--;
        reply_info.width=windows->widget.width-width-(unsigned int)
          ((6*QuantumMargin) >> 1);
        reply_info.height=height << 1;
        reply_info.x=(int) width+(QuantumMargin << 1);
        reply_info.y=action_info.y-(int) reply_info.height-QuantumMargin;
        /*
          Initialize scroll information.
        */
        XGetWidgetInfo((char *) NULL,&scroll_info);
        scroll_info.bevel_width--;
        scroll_info.width=height;
        scroll_info.height=(unsigned int)
          (reply_info.y-up_info.y-(int) (QuantumMargin >> 1));
        scroll_info.x=reply_info.x+(int) (reply_info.width-scroll_info.width);
        scroll_info.y=up_info.y-(int) reply_info.bevel_width;
        scroll_info.raised=MagickFalse;
        scroll_info.trough=MagickTrue;
        north_info=scroll_info;
        north_info.raised=MagickTrue;
        north_info.width-=(north_info.bevel_width << 1);
        north_info.height=north_info.width-1;
        north_info.x+=(int) north_info.bevel_width;
        north_info.y+=(int) north_info.bevel_width;
        south_info=north_info;
        south_info.y=scroll_info.y+(int) scroll_info.height-(int)
          scroll_info.bevel_width-(int) south_info.height;
        id=slider_info.id;
        slider_info=north_info;
        slider_info.id=id;
        slider_info.width-=2;
        slider_info.min_y=north_info.y+(int) north_info.height+(int)
          north_info.bevel_width+(int) slider_info.bevel_width+2;
        slider_info.height=(unsigned int) ((int) scroll_info.height-
          ((slider_info.min_y-scroll_info.y+1) << 1)+4);
        visible_files=(unsigned int) (scroll_info.height*
          MagickSafeReciprocal((double) height+(height >> 3)));
        if (files > visible_files)
          slider_info.height=(unsigned int)
            ((visible_files*slider_info.height)/files);
        slider_info.max_y=south_info.y-(int) south_info.bevel_width-(int)
          slider_info.bevel_width-2;
        slider_info.x=scroll_info.x+(int) slider_info.bevel_width+1;
        slider_info.y=slider_info.min_y;
        expose_info=scroll_info;
        expose_info.y=slider_info.y;
        /*
          Initialize list information.
        */
        XGetWidgetInfo((char *) NULL,&list_info);
        list_info.raised=MagickFalse;
        list_info.bevel_width--;
        list_info.width=(unsigned int)
          (scroll_info.x-reply_info.x-(int) (QuantumMargin >> 1));
        list_info.height=scroll_info.height;
        list_info.x=reply_info.x;
        list_info.y=scroll_info.y;
        if (windows->widget.mapped == MagickFalse)
          state|=JumpListState;
        /*
          Initialize text information.
        */
        *text='\0';
        XGetWidgetInfo(text,&text_info);
        text_info.center=MagickFalse;
        text_info.width=reply_info.width;
        text_info.height=height;
        text_info.x=list_info.x-(int) (QuantumMargin >> 1);
        text_info.y=QuantumMargin;
        /*
          Initialize selection information.
        */
        XGetWidgetInfo((char *) NULL,&selection_info);
        selection_info.center=MagickFalse;
        selection_info.width=list_info.width;
        selection_info.height=(unsigned int) ((9*height) >> 3);
        selection_info.x=list_info.x;
        state&=(unsigned int) (~UpdateConfigurationState);
      }
    if (state & RedrawWidgetState)
      {
        /*
          Redraw File Browser window.
        */
        x=QuantumMargin;
        y=text_info.y+(int) ((text_info.height-height) >> 1)+font_info->ascent;
        (void) XDrawString(display,windows->widget.id,
          windows->widget.annotate_context,x,y,DirectoryText,
          Extent(DirectoryText));
        (void) CopyMagickString(text_info.text,working_path,MagickPathExtent);
        (void) ConcatenateMagickString(text_info.text,DirectorySeparator,
          MagickPathExtent);
        (void) ConcatenateMagickString(text_info.text,glob_pattern,
          MagickPathExtent);
        XDrawWidgetText(display,&windows->widget,&text_info);
        XDrawBeveledButton(display,&windows->widget,&up_info);
        XDrawBeveledButton(display,&windows->widget,&home_info);
        XDrawBeveledMatte(display,&windows->widget,&list_info);
        XDrawBeveledMatte(display,&windows->widget,&scroll_info);
        XDrawTriangleNorth(display,&windows->widget,&north_info);
        XDrawBeveledButton(display,&windows->widget,&slider_info);
        XDrawTriangleSouth(display,&windows->widget,&south_info);
        x=QuantumMargin;
        y=reply_info.y+(int) ((reply_info.height-height) >> 1)+
          font_info->ascent;
        (void) XDrawString(display,windows->widget.id,
          windows->widget.annotate_context,x,y,FilenameText,
          Extent(FilenameText));
        XDrawBeveledMatte(display,&windows->widget,&reply_info);
        XDrawMatteText(display,&windows->widget,&reply_info);
        XDrawBeveledButton(display,&windows->widget,&special_info);
        XDrawBeveledButton(display,&windows->widget,&action_info);
        XDrawBeveledButton(display,&windows->widget,&cancel_info);
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        selection_info.id=(~0);
        state|=RedrawListState;
        state&=(unsigned int) (~RedrawWidgetState);
      }
    if (state & UpdateListState)
      {
        char
          **checklist;

        size_t
          number_files;

        /*
          Update file list.
        */
        checklist=ListFiles(working_path,glob_pattern,&number_files);
        if (checklist == (char **) NULL)
          {
            /*
              Reply is a filename, exit.
            */
            action_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&action_info);
            break;
          }
        for (i=0; i < (ssize_t) files; i++)
          filelist[i]=DestroyString(filelist[i]);
        if (filelist != (char **) NULL)
          filelist=(char **) RelinquishMagickMemory(filelist);
        filelist=checklist;
        files=number_files;
        /*
          Update file list.
        */
        slider_info.height=(unsigned int) ((int) scroll_info.height-
          ((slider_info.min_y-scroll_info.y+1) << 1)+1);
        if (files > visible_files)
          slider_info.height=(unsigned int)
            ((visible_files*slider_info.height)/files);
        slider_info.max_y=south_info.y-(int) south_info.bevel_width-
          (int) slider_info.bevel_width-2;
        slider_info.id=0;
        slider_info.y=slider_info.min_y;
        expose_info.y=slider_info.y;
        selection_info.id=(~0);
        list_info.id=(~0);
        state|=RedrawListState;
        /*
          Redraw directory name & reply.
        */
        if (IsGlob(reply_info.text) == MagickFalse)
          {
            *reply_info.text='\0';
            reply_info.cursor=reply_info.text;
          }
        (void) CopyMagickString(text_info.text,working_path,MagickPathExtent);
        (void) ConcatenateMagickString(text_info.text,DirectorySeparator,
          MagickPathExtent);
        (void) ConcatenateMagickString(text_info.text,glob_pattern,
          MagickPathExtent);
        XDrawWidgetText(display,&windows->widget,&text_info);
        XDrawMatteText(display,&windows->widget,&reply_info);
        XDrawBeveledMatte(display,&windows->widget,&scroll_info);
        XDrawTriangleNorth(display,&windows->widget,&north_info);
        XDrawBeveledButton(display,&windows->widget,&slider_info);
        XDrawTriangleSouth(display,&windows->widget,&south_info);
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        state&=(unsigned int) (~UpdateListState);
      }
    if (state & JumpListState)
      {
        FileBrowserMatch
          match;

        /*
          Jump scroll to match user filename.
        */
        list_info.id=(~0);
        match.filelist=filelist;
        match.files=files;
        match.reply=reply;
        MatchFileBrowserReply(&match,&list_info,&i);
        if ((i < (ssize_t) slider_info.id) ||
            (i >= (slider_info.id+(ssize_t) visible_files)))
          slider_info.id=i-(int) (visible_files >> 1);
        selection_info.id=(~0);
        state|=RedrawListState;
        state&=(unsigned int) (~JumpListState);
      }
    if (state & RedrawListState)
      {
        /*
          Determine slider id and position.
        */
        if (slider_info.id >= (int) (files-visible_files))
          slider_info.id=(int) (files-visible_files);
        if ((slider_info.id < 0) || (files <= visible_files))
          slider_info.id=0;
        slider_info.y=slider_info.min_y;
        if (files > 0)
          slider_info.y+=((int) slider_info.id*(slider_info.max_y-
            slider_info.min_y+1)/(int) files);
        if (slider_info.id != selection_info.id)
          {
            FileBrowserNames
              names;

            /*
              Redraw scroll bar and file names.
            */
            selection_info.id=slider_info.id;
            selection_info.y=list_info.y+(int) (height >> 3)+2;
            names.display=display;
            names.windows=windows;
            names.filelist=filelist;
            names.files=files;
            names.visible_files=visible_files;
            names.list_info=list_info;
            names.slider_info=slider_info;
            DrawFileBrowserNames(&names,&selection_info);
            /*
              Update slider.
            */
            expose_info=PlaceTextViewExpose(expose_info,slider_info);
            XDrawTriangleNorth(display,&windows->widget,&north_info);
            XDrawMatte(display,&windows->widget,&expose_info);
            XDrawBeveledButton(display,&windows->widget,&slider_info);
            XDrawTriangleSouth(display,&windows->widget,&south_info);
            expose_info.y=slider_info.y;
          }
        state&=(unsigned int) (~RedrawListState);
      }
    /*
      Wait for next event.
    */
    if (north_info.raised && south_info.raised)
      (void) XIfEvent(display,&event,XScreenEvent,(char *) windows);
    else
      {
        /*
          Brief delay before advancing scroll bar.
        */
        XDelay(display,delay);
        delay=SuspendTime;
        (void) XCheckIfEvent(display,&event,XScreenEvent,(char *) windows);
        if ((north_info.raised == MagickFalse) && (slider_info.id > 0))
          {
            /*
              Move slider up.
            */
            slider_info.id--;
            state|=RedrawListState;
          }
        if ((south_info.raised == MagickFalse) &&
            (slider_info.id < (int) files))
          {
            /*
              Move slider down.
            */
            slider_info.id++;
            state|=RedrawListState;
          }
        if (event.type != ButtonRelease)
          continue;
      }
    switch (event.type)
    {
      case ButtonPress:
      {
        if (MatteIsActive(slider_info,event.xbutton))
          {
            /*
              Track slider.
            */
            slider_info.active=MagickTrue;
            break;
          }
        if ((MatteIsActive(north_info,event.xbutton)) && (slider_info.id > 0))
          {
            /*
              Move slider up.
            */
            north_info.raised=MagickFalse;
            slider_info.id--;
            state|=RedrawListState;
            break;
          }
        if ((MatteIsActive(south_info,event.xbutton)) &&
            (slider_info.id < (int) files))
          {
            /*
              Move slider down.
            */
            south_info.raised=MagickFalse;
            slider_info.id++;
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(scroll_info,event.xbutton))
          {
            /*
              Move slider.
            */
            slider_info=PageListBrowserSlider(slider_info,event,visible_files);
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(list_info,event.xbutton))
          {
            FileBrowserPress
              entry;

            entry.display=display;
            entry.windows=windows;
            entry.filelist=filelist;
            entry.files=files;
            entry.height=height;
            entry.event=event;
            entry.slider_info=slider_info;
            entry.working_path=working_path;
            entry.reply=reply;
            SelectFileBrowserEntry(&entry,&targets);
            break;
          }
        if (MatteIsActive(up_info,event.xbutton))
          {
            /*
              User pressed Up button.
            */
            up_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&up_info);
            break;
          }
        if (MatteIsActive(home_info,event.xbutton))
          {
            /*
              User pressed Home button.
            */
            home_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&home_info);
            break;
          }
        if (MatteIsActive(special_info,event.xbutton))
          {
            /*
              User pressed Special button.
            */
            special_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&special_info);
            break;
          }
        if (MatteIsActive(action_info,event.xbutton))
          {
            /*
              User pressed action button.
            */
            action_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&action_info);
            break;
          }
        if (MatteIsActive(cancel_info,event.xbutton))
          {
            /*
              User pressed Cancel button.
            */
            cancel_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&cancel_info);
            break;
          }
        if (MatteIsActive(reply_info,event.xbutton) == MagickFalse)
          break;
        if (event.xbutton.button != Button2)
          {
            static Time
              click_time;

            ListBrowserClick
              click;

            /*
              Move text cursor to position of button press.
            */
            x=event.xbutton.x-reply_info.x-(int) (QuantumMargin >> 2);
            i=FindFileBrowserCursor(font_info,reply_info,x);
            reply_info.cursor=reply_info.marker+i-1;
            click.display=display;
            click.window_info=(&windows->widget);
            click.event=event;
            click.click_time=click_time;
            reply_info=HighlightListBrowserReply(&click,primary_selection,
              reply_info);
            XDrawMatteText(display,&windows->widget,&reply_info);
            click_time=event.xbutton.time;
            break;
          }
        /*
          Request primary selection.
        */
        (void) XConvertSelection(display,XA_PRIMARY,XA_STRING,XA_STRING,
          windows->widget.id,event.xbutton.time);
        break;
      }
      case ButtonRelease:
      {
        FileBrowserSpecial
          special;

        ListBrowserRelease
          release;

        if (windows->widget.mapped == MagickFalse)
          break;
        if (north_info.raised == MagickFalse)
          {
            /*
              User released up button.
            */
            delay=SuspendTime << 2;
            north_info.raised=MagickTrue;
            XDrawTriangleNorth(display,&windows->widget,&north_info);
          }
        if (south_info.raised == MagickFalse)
          {
            /*
              User released down button.
            */
            delay=SuspendTime << 2;
            south_info.raised=MagickTrue;
            XDrawTriangleSouth(display,&windows->widget,&south_info);
          }
        if (slider_info.active)
          {
            /*
              Stop tracking slider.
            */
            slider_info.active=MagickFalse;
            break;
          }
        if (up_info.raised == MagickFalse)
          {
            FileBrowserPathRelease
              up;

            up.windows=windows;
            up.event=event;
            up.working_path=working_path;
            state=ReleaseFileBrowserUp(&up,up_info,state);
            up_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&up_info);
          }
        if (home_info.raised == MagickFalse)
          {
            FontBrowserBack
              back;

            back.windows=windows;
            back.event=event;
            back.back_info=home_info;
            back.glob_pattern=working_path;
            back.back_pattern=home_directory;
            state=ReleaseFontBrowserBack(&back,state);
            home_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&home_info);
          }
        special.anomaly=anomaly;
        special.display=display;
        special.windows=windows;
        special.format=format;
        special.format_targets=format_targets;
        special.event=event;
        special.working_path=working_path;
        if (ReleaseFileBrowserSpecial(&special,&special_targets) == MagickFalse)
          break;
        release.display=display;
        release.window_info=(&windows->widget);
        release.event=event;
        ReleaseListBrowserAction(&release,reply_info,&action_info,&state);
        if (cancel_info.raised == MagickFalse)
          {
            FileBrowserCancel
              cancel;

            cancel.windows=windows;
            cancel.event=event;
            cancel.reply=reply;
            state=ReleaseFileBrowserCancel(&cancel,cancel_info,reply_info,state);
            cancel_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&cancel_info);
          }
        break;
      }
      case ClientMessage:
      {
        /*
          If client window delete message, exit.
        */
        if (event.xclient.message_type != windows->wm_protocols)
          break;
        if (*event.xclient.data.l == (int) windows->wm_take_focus)
          {
            (void) XSetInputFocus(display,event.xclient.window,RevertToParent,
              (Time) event.xclient.data.l[1]);
            break;
          }
        if (*event.xclient.data.l != (int) windows->wm_delete_window)
          break;
        if (event.xclient.window == windows->widget.id)
          {
            *reply_info.text='\0';
            state|=ExitState;
            break;
          }
        break;
      }
      case ConfigureNotify:
      {
        state=ConfigureWidgetWindow(windows,event,state);
        break;
      }
      case EnterNotify:
      {
        state=EnterWidgetWindow(windows,event,state);
        break;
      }
      case Expose:
      {
        state=ExposeWidgetWindow(windows,event,state);
        break;
      }
      case KeyPress:
      {
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
          break;
        length=XLookupString((XKeyEvent *) &event.xkey,command,
          (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        *(command+length)='\0';
        if (AreaIsActive(scroll_info,event.xkey))
          {
            /*
              Move slider.
            */
            slider_info=MoveFontBrowserSlider(slider_info,key_symbol,
              visible_files,(int) files);
            state|=RedrawListState;
            break;
          }
        if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
          {
            FileBrowserPaths
              paths;

            paths.glob_pattern=glob_pattern;
            paths.working_path=working_path;
            paths.reply=reply;
            state=ReadFileBrowserPath(&paths,reply_info,state);
            break;
          }
        if (key_symbol == XK_Control_L)
          {
            state|=ControlState;
            break;
          }
        reply_info=ApplyListBrowserControlKey(reply_info,key_symbol,state);
        XEditText(display,&reply_info,key_symbol,command,state);
        XDrawMatteText(display,&windows->widget,&reply_info);
        state|=JumpListState;
        break;
      }
      case KeyRelease:
      {
        static char
          command[MagickPathExtent];

        static KeySym
          key_symbol;

        /*
          Respond to a user key release.
        */
        if (event.xkey.window != windows->widget.id)
          break;
        (void) XLookupString((XKeyEvent *) &event.xkey,command,
          (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        if (key_symbol == XK_Control_L)
          state&=(unsigned int) (~ControlState);
        break;
      }
      case LeaveNotify:
      {
        state=LeaveWidgetWindow(windows,event,state);
        break;
      }
      case MapNotify:
      {
        mask&=(unsigned int) (~CWX);
        mask&=(unsigned int) (~CWY);
        break;
      }
      case MotionNotify:
      {
        /*
          Discard pending button motion events.
        */
        while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
        if (slider_info.active)
          {
            /*
              Move slider matte.
            */
            slider_info=TrackFontBrowserSlider(slider_info,event,(int) files);
            state|=RedrawListState;
            break;
          }
        if (state & InactiveWidgetState)
          break;
        if (up_info.raised == MatteIsActive(up_info,event.xmotion))
          {
            /*
              Up button status changed.
            */
            up_info.raised=!up_info.raised;
            XDrawBeveledButton(display,&windows->widget,&up_info);
            break;
          }
        if (home_info.raised == MatteIsActive(home_info,event.xmotion))
          {
            /*
              Home button status changed.
            */
            home_info.raised=!home_info.raised;
            XDrawBeveledButton(display,&windows->widget,&home_info);
            break;
          }
        if (special_info.raised == MatteIsActive(special_info,event.xmotion))
          {
            /*
              Grab button status changed.
            */
            special_info.raised=!special_info.raised;
            XDrawBeveledButton(display,&windows->widget,&special_info);
            break;
          }
        if (action_info.raised == MatteIsActive(action_info,event.xmotion))
          {
            /*
              Action button status changed.
            */
            action_info.raised=action_info.raised == MagickFalse ?
              MagickTrue : MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&action_info);
            break;
          }
        if (cancel_info.raised == MatteIsActive(cancel_info,event.xmotion))
          {
            /*
              Cancel button status changed.
            */
            cancel_info.raised=cancel_info.raised == MagickFalse ?
              MagickTrue : MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&cancel_info);
            break;
          }
        break;
      }
      case SelectionClear:
      {
        reply_info.highlight=MagickFalse;
        XDrawMatteText(display,&windows->widget,&reply_info);
        break;
      }
      case SelectionNotify:
      {
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
          break;
        status=XGetWindowProperty(display,event.xselection.requestor,
          event.xselection.property,0L,2047L,MagickTrue,XA_STRING,&type,
          &format,&length,&after,&data);
        if (IsUnusableSelection(status,type,format,length) != MagickFalse)
          break;
        if ((Extent(reply_info.text)+(int) length) >= (MagickPathExtent-1))
          (void) XBell(display,0);
        else
          {
            /*
              Insert primary selection in reply text.
            */
            *(data+length)='\0';
            XEditText(display,&reply_info,(KeySym) XK_Insert,(char *) data,
              state);
            XDrawMatteText(display,&windows->widget,&reply_info);
            state|=JumpListState;
            state|=RedrawActionState;
          }
        (void) XFree((void *) data);
        break;
      }
      case SelectionRequest:
      {
        XSelectionEvent
          notify;

        XSelectionRequestEvent
          *request;

        if (reply_info.highlight == MagickFalse)
          break;
        /*
          Set primary selection.
        */
        request=(&(event.xselectionrequest));
        (void) XChangeProperty(request->display,request->requestor,
          request->property,request->target,8,PropModeReplace,
          (unsigned char *) primary_selection,Extent(primary_selection));
        notify.type=SelectionNotify;
        notify.display=request->display;
        notify.requestor=request->requestor;
        notify.selection=request->selection;
        notify.target=request->target;
        notify.time=request->time;
        if (request->property == None)
          notify.property=request->target;
        else
          notify.property=request->property;
        (void) XSendEvent(request->display,request->requestor,False,0,
          (XEvent *) &notify);
      }
      default:
        break;
    }
  } while ((state & ExitState) == 0);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
  /*
    Free file list.
  */
  for (i=0; i < (ssize_t) files; i++)
    filelist[i]=DestroyString(filelist[i]);
  if (filelist != (char **) NULL)
    filelist=(char **) RelinquishMagickMemory(filelist);
  if (*reply != '\0')
    {
      (void) ConcatenateMagickString(working_path,DirectorySeparator,
        MagickPathExtent);
      (void) ConcatenateMagickString(working_path,reply,MagickPathExtent);
    }
  (void) CopyMagickString(reply,working_path,MagickPathExtent);
  if (*reply == '~')
    ExpandFilename(reply);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X F o n t B r o w s e r W i d g e t                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XFontBrowserWidget() displays a Font Browser widget with a font query to the
%  user.  The user keys a reply and presses the Action or Cancel button to
%  exit.  The typed text is returned as the reply function parameter.
%
%  The format of the XFontBrowserWidget method is:
%
%      void XFontBrowserWidget(Display *display,XWindows *windows,
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
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline MagickBooleanType IsSameFontCharacter(const char *p,const char *q)
{
  return(((*p != '\0') && (*q != '\0') && (*p == *q)) ? MagickTrue :
    MagickFalse);
}

static int FontCompare(const void *x,const void *y)
{
  char
    *p,
    *q;

  p=(char *) *((char **) x);
  q=(char *) *((char **) y);
  while (IsSameFontCharacter(p,q) != MagickFalse)
  {
    p++;
    q++;
  }
  return(*p-(*q));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static void SelectFontBrowserEntry(const FontBrowserPress *press,
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

static int FindFontBrowserCursor(XFontStruct *font_info,
  const XWidgetInfo reply_info,const int x,int i)
{
  if (font_info != (XFontStruct *) NULL)
    for (i=1; i <= Extent(reply_info.marker); i++)
      if (XTextWidth(font_info,reply_info.marker,i) > x)
        break;
  return(i);
}

static void DrawFontBrowserNames(const FontBrowserNames *names,
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

static XWidgetInfo PageFontBrowserSlider(XWidgetInfo slider_info,
  const XEvent event,const unsigned int visible_fonts)
{
  if (event.xbutton.y < slider_info.y)
    slider_info.id-=((int) visible_fonts-1);
  else
    slider_info.id+=((int) visible_fonts-1);
  return(slider_info);
}

static XWidgetInfo TrackFontBrowserSlider(XWidgetInfo slider_info,
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

static size_t ReadFontBrowserPattern(const XWidgetInfo reply_info,
  char *back_pattern,char *glob_pattern,size_t state)
{
  /*
    Read new font or glob pattern.
  */
  if (*reply_info.text == '\0')
    return(state);
  (void) CopyMagickString(back_pattern,glob_pattern,MagickPathExtent);
  (void) CopyMagickString(glob_pattern,reply_info.text,MagickPathExtent);
  state|=UpdateListState;
  return(state);
}

static void MoveFontBrowserSliderByPage(const KeySym key_symbol,
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

static XWidgetInfo MoveFontBrowserSlider(XWidgetInfo slider_info,
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

static size_t ReleaseFontBrowserBack(const FontBrowserBack *back,
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

typedef struct _FontBrowserReset
{
  XWindows
    *windows;

  XEvent
    event;

  XWidgetInfo
    reset_info;

  char
    *back_pattern,
    *glob_pattern;

  const char
    *reset_pattern;
} FontBrowserReset;

static size_t ReleaseFontBrowserReset(const FontBrowserReset *reset,
  size_t state)
{
  XWindows
    *const windows = reset->windows;

  const XEvent
    event = reset->event;

  const XWidgetInfo
    reset_info = reset->reset_info;

  char
    *const back_pattern = reset->back_pattern,
    *const glob_pattern = reset->glob_pattern;

  const char
    *const reset_pattern = reset->reset_pattern;

  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(reset_info,event.xbutton)))
    {
      (void) CopyMagickString(back_pattern,glob_pattern,MagickPathExtent);
      (void) CopyMagickString(glob_pattern,reset_pattern,MagickPathExtent);
      state|=UpdateListState;
    }
  return(state);
}

static void MatchFontBrowserReply(const FontBrowserMatch *match,
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

typedef struct _FontBrowserUpdate
{
  Display
    *display;

  XWindows
    *windows;

  char
    *reply,
    *glob_pattern;

  const char
    *back_pattern;

  char
    **listhead;
} FontBrowserUpdate;

typedef struct _FontBrowserUpdateTargets
{
  XWidgetInfo
    *action_info;

  char
    ***fontlist;

  int
    *fonts;
} FontBrowserUpdateTargets;

static MagickBooleanType UpdateFontBrowserList(const FontBrowserUpdate *update,
  const FontBrowserUpdateTargets *targets)
{
  Display
    *const display = update->display;

  XWindows
    *const windows = update->windows;

  char
    *const reply = update->reply,
    *const glob_pattern = update->glob_pattern;

  const char
    *const back_pattern = update->back_pattern;

  char
    **const listhead = update->listhead;

  XWidgetInfo
    *const action_info = targets->action_info;

  char
    ***const fontlist = targets->fontlist;

  int
    *const fonts = targets->fonts;

  char
    **checklist;

  int
    number_fonts;

  checklist=XListFonts(display,glob_pattern,32767,&number_fonts);
  if (checklist == (char **) NULL)
    {
      if ((strchr(glob_pattern,'*') == (char *) NULL) &&
          (strchr(glob_pattern,'?') == (char *) NULL))
        {
          /*
            Might be a scaleable font-- exit.
          */
          (void) CopyMagickString(reply,glob_pattern,MagickPathExtent);
          (void) CopyMagickString(glob_pattern,back_pattern,MagickPathExtent);
          (*action_info).raised=MagickFalse;
          XDrawBeveledButton(display,&windows->widget,action_info);
          return(MagickFalse);
        }
      (void) CopyMagickString(glob_pattern,back_pattern,MagickPathExtent);
      (void) XBell(display,0);
    }
  else
    if (number_fonts == 1)
      {
        /*
          Reply is a single font name-- exit.
        */
        (void) CopyMagickString(reply,checklist[0],MagickPathExtent);
        (void) CopyMagickString(glob_pattern,back_pattern,MagickPathExtent);
        (void) XFreeFontNames(checklist);
        (*action_info).raised=MagickFalse;
        XDrawBeveledButton(display,&windows->widget,action_info);
        return(MagickFalse);
      }
    else
      {
        (void) XFreeFontNames(listhead);
        (*fontlist)=(char **) RelinquishMagickMemory((*fontlist));
        (*fontlist)=checklist;
        (*fonts)=number_fonts;
      }
  return(MagickTrue);
}

MagickPrivate void XFontBrowserWidget(Display *display,XWindows *windows,
  const char *action,char *reply)
{
#define BackButtonText  "Back"
#define CancelButtonText  "Cancel"
#define FontnameText  "Name:"
#define FontPatternText  "Pattern:"
#define ResetButtonText  "Reset"

  char
    back_pattern[MagickPathExtent],
    **fontlist,
    **listhead,
    primary_selection[MagickPathExtent] = "",
    reset_pattern[MagickPathExtent],
    text[MagickPathExtent];

  int
    fonts,
    x,
    y;

  int
    i;

  static char
    glob_pattern[MagickPathExtent] = "*";

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  Status
    status;

  unsigned int
    height,
    text_width,
    visible_fonts,
    width;

  size_t
    delay,
    state;

  XEvent
    event;

  XFontStruct
    *font_info;

  XTextProperty
    window_name;

  XWidgetInfo
    action_info,
    back_info,
    cancel_info,
    expose_info,
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

  XWindowChanges
    window_changes;

  const FontBrowserTargets
    targets = { &reply_info, &state, &selection_info, &list_info };

  /*
    Get font list and sort in ascending order.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(action != (char *) NULL);
  assert(reply != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(back_pattern,glob_pattern,MagickPathExtent);
  (void) CopyMagickString(reset_pattern,"*",MagickPathExtent);
  fontlist=XListFonts(display,glob_pattern,32767,&fonts);
  if (fonts == 0)
    {
      /*
        Pattern failed, obtain all the fonts.
      */
      XNoticeWidget(display,windows,"Unable to obtain fonts names:",
        glob_pattern);
      (void) CopyMagickString(glob_pattern,"*",MagickPathExtent);
      fontlist=XListFonts(display,glob_pattern,32767,&fonts);
      if (fontlist == (char **) NULL)
        {
          XNoticeWidget(display,windows,"Unable to obtain fonts names:",
            glob_pattern);
          return;
        }
    }
  /*
    Sort font list in ascending order.
  */
  listhead=fontlist;
  fontlist=(char **) AcquireQuantumMemory((size_t) fonts,sizeof(*fontlist));
  if (fontlist == (char **) NULL)
    {
      XNoticeWidget(display,windows,"MemoryAllocationFailed",
        "UnableToViewFonts");
      return;
    }
  for (i=0; i < fonts; i++)
    fontlist[i]=listhead[i];
  qsort((void *) fontlist,(size_t) fonts,sizeof(*fontlist),FontCompare);
  /*
    Determine Font Browser widget attributes.
  */
  font_info=windows->widget.font_info;
  text_width=0;
  for (i=0; i < fonts; i++)
    if (WidgetTextWidth(font_info,fontlist[i]) > text_width)
      text_width=WidgetTextWidth(font_info,fontlist[i]);
  width=WidgetTextWidth(font_info,(char *) action);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  if (WidgetTextWidth(font_info,ResetButtonText) > width)
    width=WidgetTextWidth(font_info,ResetButtonText);
  if (WidgetTextWidth(font_info,BackButtonText) > width)
    width=WidgetTextWidth(font_info,BackButtonText);
  width+=(unsigned int) QuantumMargin;
  if (WidgetTextWidth(font_info,FontPatternText) > width)
    width=WidgetTextWidth(font_info,FontPatternText);
  if (WidgetTextWidth(font_info,FontnameText) > width)
    width=WidgetTextWidth(font_info,FontnameText);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position Font Browser widget.
  */
  windows->widget.width=width+MagickMin(text_width,MaxTextWidth)+(unsigned int)
    (6*QuantumMargin);
  windows->widget.min_width=width+MinTextWidth+(unsigned int) (4*QuantumMargin);
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int)
    (((85*(int) height) >> 2)+((13*QuantumMargin) >> 1)+4);
  windows->widget.min_height=(unsigned int)
    (((27*(int) height) >> 1)+((13*QuantumMargin) >> 1)+4);
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
  /*
    Map Font Browser widget.
  */
  (void) CopyMagickString(windows->widget.name,"Browse and Select a Font",
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
  (void) XReconfigureWMWindow(display,windows->widget.id,
    windows->widget.screen,mask,&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
  /*
    Respond to X events.
  */
  XGetWidgetInfo((char *) NULL,&slider_info);
  XGetWidgetInfo((char *) NULL,&north_info);
  XGetWidgetInfo((char *) NULL,&south_info);
  XGetWidgetInfo((char *) NULL,&expose_info);
  XGetWidgetInfo((char *) NULL,&selection_info);
  visible_fonts=0;
  delay=SuspendTime << 2;
  state=UpdateConfigurationState;
  do
  {
    if (state & UpdateConfigurationState)
      {
        int
          id;

        /*
          Initialize button information.
        */
        XGetWidgetInfo(CancelButtonText,&cancel_info);
        cancel_info.width=width;
        cancel_info.height=(unsigned int) ((3*height) >> 1);
        cancel_info.x=(int) windows->widget.width-(int) cancel_info.width-
          QuantumMargin-2;
        cancel_info.y=(int) windows->widget.height-(int) cancel_info.height-
          QuantumMargin;
        XGetWidgetInfo(action,&action_info);
        action_info.width=width;
        action_info.height=(unsigned int) ((3*height) >> 1);
        action_info.x=(int) windows->widget.width-(int) action_info.width-
          (int) cancel_info.width-2*QuantumMargin-2;
        action_info.y=cancel_info.y;
        XGetWidgetInfo(BackButtonText,&back_info);
        back_info.width=width;
        back_info.height=(unsigned int) ((3*height) >> 1);
        back_info.x=QuantumMargin;
        back_info.y=((5*QuantumMargin) >> 1)+(int) height;
        XGetWidgetInfo(ResetButtonText,&reset_info);
        reset_info.width=width;
        reset_info.height=(unsigned int) ((3*height) >> 1);
        reset_info.x=QuantumMargin;
        reset_info.y=back_info.y+(int) back_info.height+QuantumMargin;
        /*
          Initialize reply information.
        */
        XGetWidgetInfo(reply,&reply_info);
        reply_info.raised=MagickFalse;
        reply_info.bevel_width--;
        reply_info.width=(unsigned int) ((int) windows->widget.width-(int)
          width-((6*QuantumMargin) >> 1));
        reply_info.height=height << 1;
        reply_info.x=(int) width+(QuantumMargin << 1);
        reply_info.y=action_info.y-(int) (action_info.height << 1)-
          QuantumMargin;
        /*
          Initialize mode information.
        */
        XGetWidgetInfo(reply,&mode_info);
        mode_info.bevel_width=0;
        mode_info.width=(unsigned int)
          (action_info.x-reply_info.x-QuantumMargin);
        mode_info.height=action_info.height << 1;
        mode_info.x=reply_info.x;
        mode_info.y=action_info.y-(int) action_info.height+(int)
          action_info.bevel_width;
        /*
          Initialize scroll information.
        */
        XGetWidgetInfo((char *) NULL,&scroll_info);
        scroll_info.bevel_width--;
        scroll_info.width=height;
        scroll_info.height=(unsigned int)
          (reply_info.y-back_info.y-(int) (QuantumMargin >> 1));
        scroll_info.x=reply_info.x+(int) (reply_info.width-scroll_info.width);
        scroll_info.y=back_info.y-(int) reply_info.bevel_width;
        scroll_info.raised=MagickFalse;
        scroll_info.trough=MagickTrue;
        north_info=scroll_info;
        north_info.raised=MagickTrue;
        north_info.width-=(north_info.bevel_width << 1);
        north_info.height=north_info.width-1;
        north_info.x+=(int) north_info.bevel_width;
        north_info.y+=(int) north_info.bevel_width;
        south_info=north_info;
        south_info.y=scroll_info.y+(int) scroll_info.height-(int)
          scroll_info.bevel_width-(int) south_info.height;
        id=slider_info.id;
        slider_info=north_info;
        slider_info.id=id;
        slider_info.width-=2;
        slider_info.min_y=north_info.y+(int) north_info.height+(int)
          north_info.bevel_width+(int) slider_info.bevel_width+2;
        slider_info.height=(unsigned int) ((int) scroll_info.height-
          ((slider_info.min_y-scroll_info.y+1) << 1)+4);
        visible_fonts=(unsigned int) (scroll_info.height*
          MagickSafeReciprocal((double) height+(height >> 3)));
        if (fonts > (int) visible_fonts)
          slider_info.height=(visible_fonts*slider_info.height)/(unsigned int)
            fonts;
        slider_info.max_y=south_info.y-(int) south_info.bevel_width-
          (int) slider_info.bevel_width-2;
        slider_info.x=scroll_info.x+(int) slider_info.bevel_width+1;
        slider_info.y=slider_info.min_y;
        expose_info=scroll_info;
        expose_info.y=slider_info.y;
        /*
          Initialize list information.
        */
        XGetWidgetInfo((char *) NULL,&list_info);
        list_info.raised=MagickFalse;
        list_info.bevel_width--;
        list_info.width=(unsigned int) (scroll_info.x-reply_info.x-
          (QuantumMargin >> 1));
        list_info.height=scroll_info.height;
        list_info.x=reply_info.x;
        list_info.y=scroll_info.y;
        if (windows->widget.mapped == MagickFalse)
          state|=JumpListState;
        /*
          Initialize text information.
        */
        *text='\0';
        XGetWidgetInfo(text,&text_info);
        text_info.center=MagickFalse;
        text_info.width=reply_info.width;
        text_info.height=height;
        text_info.x=list_info.x-(QuantumMargin >> 1);
        text_info.y=QuantumMargin;
        /*
          Initialize selection information.
        */
        XGetWidgetInfo((char *) NULL,&selection_info);
        selection_info.center=MagickFalse;
        selection_info.width=list_info.width;
        selection_info.height=(unsigned int) ((9*height) >> 3);
        selection_info.x=list_info.x;
        state&=(unsigned int) (~UpdateConfigurationState);
      }
    if (state & RedrawWidgetState)
      {
        /*
          Redraw Font Browser window.
        */
        x=QuantumMargin;
        y=text_info.y+(int) ((text_info.height-height) >> 1)+font_info->ascent;
        (void) XDrawString(display,windows->widget.id,
          windows->widget.annotate_context,x,y,FontPatternText,
          Extent(FontPatternText));
        (void) CopyMagickString(text_info.text,glob_pattern,MagickPathExtent);
        XDrawWidgetText(display,&windows->widget,&text_info);
        XDrawBeveledButton(display,&windows->widget,&back_info);
        XDrawBeveledButton(display,&windows->widget,&reset_info);
        XDrawBeveledMatte(display,&windows->widget,&list_info);
        XDrawBeveledMatte(display,&windows->widget,&scroll_info);
        XDrawTriangleNorth(display,&windows->widget,&north_info);
        XDrawBeveledButton(display,&windows->widget,&slider_info);
        XDrawTriangleSouth(display,&windows->widget,&south_info);
        x=QuantumMargin;
        y=reply_info.y+(int) ((reply_info.height-height) >> 1)+
          font_info->ascent;
        (void) XDrawString(display,windows->widget.id,
          windows->widget.annotate_context,x,y,FontnameText,
          Extent(FontnameText));
        XDrawBeveledMatte(display,&windows->widget,&reply_info);
        XDrawMatteText(display,&windows->widget,&reply_info);
        XDrawBeveledButton(display,&windows->widget,&action_info);
        XDrawBeveledButton(display,&windows->widget,&cancel_info);
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        selection_info.id=(~0);
        state|=RedrawActionState;
        state|=RedrawListState;
        state&=(unsigned int) (~RedrawWidgetState);
      }
    if (state & UpdateListState)
      {
        const FontBrowserUpdateTargets
          update_targets = { &action_info, &fontlist, &fonts };

        FontBrowserUpdate
          update;

        /*
          Update font list.
        */
        update.display=display;
        update.windows=windows;
        update.reply=reply;
        update.glob_pattern=glob_pattern;
        update.back_pattern=back_pattern;
        update.listhead=listhead;
        if (UpdateFontBrowserList(&update,&update_targets) == MagickFalse)
          break;
        /*
          Sort font list in ascending order.
        */
        listhead=fontlist;
        fontlist=(char **) AcquireQuantumMemory((size_t) fonts,
          sizeof(*fontlist));
        if (fontlist == (char **) NULL)
          {
            XNoticeWidget(display,windows,"MemoryAllocationFailed",
              "UnableToViewFonts");
            return;
          }
        for (i=0; i < fonts; i++)
          fontlist[i]=listhead[i];
        qsort((void *) fontlist,(size_t) fonts,sizeof(*fontlist),FontCompare);
        slider_info.height=(unsigned int) ((int) scroll_info.height-
          ((slider_info.min_y-scroll_info.y+1) << 1)+1);
        if (fonts > (int) visible_fonts)
          slider_info.height=(visible_fonts*slider_info.height)/(unsigned int)
            fonts;
        slider_info.max_y=south_info.y-(int) south_info.bevel_width-(int)
          slider_info.bevel_width-2;
        slider_info.id=0;
        slider_info.y=slider_info.min_y;
        expose_info.y=slider_info.y;
        selection_info.id=(~0);
        list_info.id=(~0);
        state|=RedrawListState;
        /*
          Redraw font name & reply.
        */
        *reply_info.text='\0';
        reply_info.cursor=reply_info.text;
        (void) CopyMagickString(text_info.text,glob_pattern,MagickPathExtent);
        XDrawWidgetText(display,&windows->widget,&text_info);
        XDrawMatteText(display,&windows->widget,&reply_info);
        XDrawBeveledMatte(display,&windows->widget,&scroll_info);
        XDrawTriangleNorth(display,&windows->widget,&north_info);
        XDrawBeveledButton(display,&windows->widget,&slider_info);
        XDrawTriangleSouth(display,&windows->widget,&south_info);
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        state&=(unsigned int) (~UpdateListState);
      }
    if (state & JumpListState)
      {
        FontBrowserMatch
          match;

        /*
          Jump scroll to match user font.
        */
        list_info.id=(~0);
        match.fontlist=fontlist;
        match.fonts=fonts;
        match.reply=reply;
        MatchFontBrowserReply(&match,&list_info,&i);
        if ((i < slider_info.id) || (i >= (slider_info.id+(int) visible_fonts)))
          slider_info.id=i-((int) visible_fonts >> 1);
        selection_info.id=(~0);
        state|=RedrawListState;
        state&=(unsigned int) (~JumpListState);
      }
    if (state & RedrawListState)
      {
        /*
          Determine slider id and position.
        */
        if (slider_info.id >= (fonts-(int) visible_fonts))
          slider_info.id=fonts-(int) visible_fonts;
        if ((slider_info.id < 0) || (fonts <= (int) visible_fonts))
          slider_info.id=0;
        slider_info.y=slider_info.min_y;
        if (fonts > 0)
          slider_info.y+=
            slider_info.id*(slider_info.max_y-slider_info.min_y+1)/fonts;
        if (slider_info.id != selection_info.id)
          {
            FontBrowserNames
              names;

            /*
              Redraw scroll bar and file names.
            */
            selection_info.id=slider_info.id;
            selection_info.y=list_info.y+(int) (height >> 3)+2;
            names.display=display;
            names.windows=windows;
            names.fontlist=fontlist;
            names.fonts=fonts;
            names.visible_fonts=visible_fonts;
            names.list_info=list_info;
            names.slider_info=slider_info;
            DrawFontBrowserNames(&names,&selection_info,&i);
            /*
              Update slider.
            */
            expose_info=PlaceTextViewExpose(expose_info,slider_info);
            XDrawTriangleNorth(display,&windows->widget,&north_info);
            XDrawMatte(display,&windows->widget,&expose_info);
            XDrawBeveledButton(display,&windows->widget,&slider_info);
            XDrawTriangleSouth(display,&windows->widget,&south_info);
            expose_info.y=slider_info.y;
          }
        state&=(unsigned int) (~RedrawListState);
      }
    if (state & RedrawActionState)
      {
        XFontStruct
          *save_info;

        /*
          Display the selected font in a drawing area.
        */
        save_info=windows->widget.font_info;
        font_info=XLoadQueryFont(display,reply_info.text);
        if (font_info != (XFontStruct *) NULL)
          {
            windows->widget.font_info=font_info;
            (void) XSetFont(display,windows->widget.widget_context,
              font_info->fid);
          }
        XDrawBeveledButton(display,&windows->widget,&mode_info);
        windows->widget.font_info=save_info;
        if (font_info != (XFontStruct *) NULL)
          {
            (void) XSetFont(display,windows->widget.widget_context,
              windows->widget.font_info->fid);
            (void) XFreeFont(display,font_info);
          }
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        XDrawMatteText(display,&windows->widget,&reply_info);
        state&=(unsigned int) (~RedrawActionState);
      }
    /*
      Wait for next event.
    */
    if (north_info.raised && south_info.raised)
      (void) XIfEvent(display,&event,XScreenEvent,(char *) windows);
    else
      {
        /*
          Brief delay before advancing scroll bar.
        */
        XDelay(display,delay);
        delay=SuspendTime;
        (void) XCheckIfEvent(display,&event,XScreenEvent,(char *) windows);
        if ((north_info.raised == MagickFalse) && (slider_info.id > 0))
          {
            /*
              Move slider up.
            */
            slider_info.id--;
            state|=RedrawListState;
          }
        if ((south_info.raised == MagickFalse) && (slider_info.id < fonts))
          {
            /*
              Move slider down.
            */
            slider_info.id++;
            state|=RedrawListState;
          }
        if (event.type != ButtonRelease)
          continue;
      }
    switch (event.type)
    {
      case ButtonPress:
      {
        if (MatteIsActive(slider_info,event.xbutton))
          {
            /*
              Track slider.
            */
            slider_info.active=MagickTrue;
            break;
          }
        if ((MatteIsActive(north_info,event.xbutton)) && (slider_info.id > 0))
          {
            /*
              Move slider up.
            */
            north_info.raised=MagickFalse;
            slider_info.id--;
            state|=RedrawListState;
            break;
          }
        if ((MatteIsActive(south_info,event.xbutton)) &&
            (slider_info.id < fonts))
          {
            /*
              Move slider down.
            */
            south_info.raised=MagickFalse;
            slider_info.id++;
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(scroll_info,event.xbutton))
          {
            /*
              Move slider.
            */
            slider_info=PageFontBrowserSlider(slider_info,event,visible_fonts);
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(list_info,event.xbutton))
          {
            FontBrowserPress
              press;

            press.display=display;
            press.windows=windows;
            press.fontlist=fontlist;
            press.fonts=fonts;
            press.height=height;
            press.event=event;
            press.slider_info=slider_info;
            SelectFontBrowserEntry(&press,glob_pattern,&targets);
            break;
          }
        if (MatteIsActive(back_info,event.xbutton))
          {
            /*
              User pressed Back button.
            */
            back_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&back_info);
            break;
          }
        if (MatteIsActive(reset_info,event.xbutton))
          {
            /*
              User pressed Reset button.
            */
            reset_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&reset_info);
            break;
          }
        if (MatteIsActive(action_info,event.xbutton))
          {
            /*
              User pressed action button.
            */
            action_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&action_info);
            break;
          }
        if (MatteIsActive(cancel_info,event.xbutton))
          {
            /*
              User pressed Cancel button.
            */
            cancel_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&cancel_info);
            break;
          }
        if (MatteIsActive(reply_info,event.xbutton) == MagickFalse)
          break;
        if (event.xbutton.button != Button2)
          {
            static Time
              click_time;

            ListBrowserClick
              click;

            /*
              Move text cursor to position of button press.
            */
            x=event.xbutton.x-reply_info.x-(int) (QuantumMargin >> 2);
            i=FindFontBrowserCursor(font_info,reply_info,x,i);
            reply_info.cursor=reply_info.marker+i-1;
            click.display=display;
            click.window_info=(&windows->widget);
            click.event=event;
            click.click_time=click_time;
            reply_info=HighlightListBrowserReply(&click,primary_selection,
              reply_info);
            XDrawMatteText(display,&windows->widget,&reply_info);
            click_time=event.xbutton.time;
            break;
          }
        /*
          Request primary selection.
        */
        (void) XConvertSelection(display,XA_PRIMARY,XA_STRING,XA_STRING,
          windows->widget.id,event.xbutton.time);
        break;
      }
      case ButtonRelease:
      {
        ListBrowserRelease
          release;

        if (windows->widget.mapped == MagickFalse)
          break;
        if (north_info.raised == MagickFalse)
          {
            /*
              User released up button.
            */
            delay=SuspendTime << 2;
            north_info.raised=MagickTrue;
            XDrawTriangleNorth(display,&windows->widget,&north_info);
          }
        if (south_info.raised == MagickFalse)
          {
            /*
              User released down button.
            */
            delay=SuspendTime << 2;
            south_info.raised=MagickTrue;
            XDrawTriangleSouth(display,&windows->widget,&south_info);
          }
        if (slider_info.active)
          {
            /*
              Stop tracking slider.
            */
            slider_info.active=MagickFalse;
            break;
          }
        if (back_info.raised == MagickFalse)
          {
            FontBrowserBack
              back;

            back.windows=windows;
            back.event=event;
            back.back_info=back_info;
            back.glob_pattern=glob_pattern;
            back.back_pattern=back_pattern;
            state=ReleaseFontBrowserBack(&back,state);
            back_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&back_info);
          }
        if (reset_info.raised == MagickFalse)
          {
            FontBrowserReset
              reset;

            reset.windows=windows;
            reset.event=event;
            reset.reset_info=reset_info;
            reset.back_pattern=back_pattern;
            reset.glob_pattern=glob_pattern;
            reset.reset_pattern=reset_pattern;
            state=ReleaseFontBrowserReset(&reset,state);
            reset_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&reset_info);
          }
        release.display=display;
        release.window_info=(&windows->widget);
        release.event=event;
        ReleaseListBrowserAction(&release,reply_info,&action_info,&state);
        release.display=display;
        release.window_info=(&windows->widget);
        release.event=event;
        ReleaseListBrowserCancel(&release,reply_info,&cancel_info,&state);
        break;
      }
      case ClientMessage:
      {
        /*
          If client window delete message, exit.
        */
        if (event.xclient.message_type != windows->wm_protocols)
          break;
        if (*event.xclient.data.l == (int) windows->wm_take_focus)
          {
            (void) XSetInputFocus(display,event.xclient.window,RevertToParent,
              (Time) event.xclient.data.l[1]);
            break;
          }
        if (*event.xclient.data.l != (int) windows->wm_delete_window)
          break;
        if (event.xclient.window == windows->widget.id)
          {
            *reply_info.text='\0';
            state|=ExitState;
            break;
          }
        break;
      }
      case ConfigureNotify:
      {
        state=ConfigureWidgetWindow(windows,event,state);
        break;
      }
      case EnterNotify:
      {
        state=EnterWidgetWindow(windows,event,state);
        break;
      }
      case Expose:
      {
        state=ExposeWidgetWindow(windows,event,state);
        break;
      }
      case KeyPress:
      {
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
          break;
        length=XLookupString((XKeyEvent *) &event.xkey,command,
          (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        *(command+length)='\0';
        if (AreaIsActive(scroll_info,event.xkey))
          {
            /*
              Move slider.
            */
            slider_info=MoveFontBrowserSlider(slider_info,key_symbol,
              visible_fonts,fonts);
            state|=RedrawListState;
            break;
          }
        if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
          {
            state=ReadFontBrowserPattern(reply_info,back_pattern,glob_pattern,
              state);
            break;
          }
        if (key_symbol == XK_Control_L)
          {
            state|=ControlState;
            break;
          }
        reply_info=ApplyListBrowserControlKey(reply_info,key_symbol,state);
        XEditText(display,&reply_info,key_symbol,command,state);
        XDrawMatteText(display,&windows->widget,&reply_info);
        state|=JumpListState;
        break;
      }
      case KeyRelease:
      {
        static char
          command[MagickPathExtent];

        static KeySym
          key_symbol;

        /*
          Respond to a user key release.
        */
        if (event.xkey.window != windows->widget.id)
          break;
        (void) XLookupString((XKeyEvent *) &event.xkey,command,
          (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        if (key_symbol == XK_Control_L)
          state&=(unsigned int) (~ControlState);
        break;
      }
      case LeaveNotify:
      {
        state=LeaveWidgetWindow(windows,event,state);
        break;
      }
      case MapNotify:
      {
        mask&=(unsigned int) (~CWX);
        mask&=(unsigned int) (~CWY);
        break;
      }
      case MotionNotify:
      {
        /*
          Discard pending button motion events.
        */
        while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
        if (slider_info.active)
          {
            /*
              Move slider matte.
            */
            slider_info=TrackFontBrowserSlider(slider_info,event,fonts);
            state|=RedrawListState;
            break;
          }
        if (state & InactiveWidgetState)
          break;
        if (back_info.raised == MatteIsActive(back_info,event.xmotion))
          {
            /*
              Back button status changed.
            */
            back_info.raised=!back_info.raised;
            XDrawBeveledButton(display,&windows->widget,&back_info);
            break;
          }
        if (reset_info.raised == MatteIsActive(reset_info,event.xmotion))
          {
            /*
              Reset button status changed.
            */
            reset_info.raised=!reset_info.raised;
            XDrawBeveledButton(display,&windows->widget,&reset_info);
            break;
          }
        if (action_info.raised == MatteIsActive(action_info,event.xmotion))
          {
            /*
              Action button status changed.
            */
            action_info.raised=action_info.raised == MagickFalse ?
              MagickTrue : MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&action_info);
            break;
          }
        if (cancel_info.raised == MatteIsActive(cancel_info,event.xmotion))
          {
            /*
              Cancel button status changed.
            */
            cancel_info.raised=cancel_info.raised == MagickFalse ?
              MagickTrue : MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&cancel_info);
            break;
          }
        break;
      }
      case SelectionClear:
      {
        reply_info.highlight=MagickFalse;
        XDrawMatteText(display,&windows->widget,&reply_info);
        break;
      }
      case SelectionNotify:
      {
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
          break;
        status=XGetWindowProperty(display,event.xselection.requestor,
          event.xselection.property,0L,2047L,MagickTrue,XA_STRING,&type,
          &format,&length,&after,&data);
        if (IsUnusableSelection(status,type,format,length) != MagickFalse)
          break;
        if ((Extent(reply_info.text)+(int) length) >= (MagickPathExtent-1))
          (void) XBell(display,0);
        else
          {
            /*
              Insert primary selection in reply text.
            */
            *(data+length)='\0';
            XEditText(display,&reply_info,(KeySym) XK_Insert,(char *) data,
              state);
            XDrawMatteText(display,&windows->widget,&reply_info);
            state|=JumpListState;
            state|=RedrawActionState;
          }
        (void) XFree((void *) data);
        break;
      }
      case SelectionRequest:
      {
        XSelectionEvent
          notify;

        XSelectionRequestEvent
          *request;

        /*
          Set XA_PRIMARY selection.
        */
        request=(&(event.xselectionrequest));
        (void) XChangeProperty(request->display,request->requestor,
          request->property,request->target,8,PropModeReplace,
          (unsigned char *) primary_selection,Extent(primary_selection));
        notify.type=SelectionNotify;
        notify.display=request->display;
        notify.requestor=request->requestor;
        notify.selection=request->selection;
        notify.target=request->target;
        notify.time=request->time;
        if (request->property == None)
          notify.property=request->target;
        else
          notify.property=request->property;
        (void) XSendEvent(request->display,request->requestor,False,0,
          (XEvent *) &notify);
      }
      default:
        break;
    }
  } while ((state & ExitState) == 0);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
  /*
    Free font list.
  */
  (void) XFreeFontNames(listhead);
  fontlist=(char **) RelinquishMagickMemory(fontlist);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X I n f o W i d g e t                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XInfoWidget() displays text in the Info widget.  The purpose is to inform
%  the user that what activity is currently being performed (e.g. reading
%  an image, rotating an image, etc.).
%
%  The format of the XInfoWidget method is:
%
%      void XInfoWidget(Display *display,XWindows *windows,const char *activity)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o activity: This character string reflects the current activity and is
%      displayed in the Info widget.
%
*/
MagickPrivate void XInfoWidget(Display *display,XWindows *windows,
  const char *activity)
{
  unsigned int
    height,
    margin,
    width;

  XFontStruct
    *font_info;

  XWindowChanges
    window_changes;

  /*
    Map Info widget.
  */
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(activity != (char *) NULL);
  font_info=windows->info.font_info;
  width=WidgetTextWidth(font_info,(char *) activity)+(unsigned int)
    ((3*QuantumMargin) >> 1)+4;
  height=(unsigned int) (((6*(font_info->ascent+font_info->descent)) >> 2)+4);
  if ((windows->info.width != width) || (windows->info.height != height))
    {
      /*
        Size Info widget to accommodate the activity text.
      */
      windows->info.width=width;
      windows->info.height=height;
      window_changes.width=(int) width;
      window_changes.height=(int) height;
      (void) XReconfigureWMWindow(display,windows->info.id,windows->info.screen,
        (unsigned int) (CWWidth | CWHeight),&window_changes);
    }
  if (windows->info.mapped == MagickFalse)
    {
      (void) XMapRaised(display,windows->info.id);
      windows->info.mapped=MagickTrue;
    }
  /*
    Initialize Info matte information.
  */
  height=(unsigned int) (font_info->ascent+font_info->descent);
  XGetWidgetInfo(activity,&monitor_info);
  monitor_info.bevel_width--;
  margin=monitor_info.bevel_width+((windows->info.height-height) >> 1)-2;
  monitor_info.center=MagickFalse;
  monitor_info.x=(int) margin;
  monitor_info.y=(int) margin;
  monitor_info.width=windows->info.width-(margin << 1);
  monitor_info.height=windows->info.height-(margin << 1)+1;
  /*
    Draw Info widget.
  */
  monitor_info.raised=MagickFalse;
  XDrawBeveledMatte(display,&windows->info,&monitor_info);
  monitor_info.raised=MagickTrue;
  XDrawWidgetText(display,&windows->info,&monitor_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X L i s t B r o w s e r W i d g e t                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XListBrowserWidget() displays a List Browser widget with a query to the
%  user.  The user keys a reply or select a reply from the list.  Finally, the
%  user presses the Action or Cancel button to exit.  The typed text is
%  returned as the reply function parameter.
%
%  The format of the XListBrowserWidget method is:
%
%      void XListBrowserWidget(Display *display,XWindows *windows,
%        XWindowInfo *window_info,const char *const *list,const char *action,
%        const char *query,char *reply)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o list: Specifies a pointer to an array of strings.  The user can
%      select from these strings as a possible reply value.
%
%    o action: Specifies a pointer to the action of this widget.
%
%    o query: Specifies a pointer to the query to present to the user.
%
%    o reply: the response from the user is returned in this parameter.
%
*/
static unsigned int CountListBrowserEntries(const char *const *list)
{
  unsigned int
    entries;

  for (entries=0; ; entries++)
    if (list[entries] == (char *) NULL)
      break;
  return(entries);
}

static unsigned int WidestListBrowserEntry(XFontStruct *font_info,
  const char *const *list,const unsigned int entries,unsigned int text_width)
{
  int
    i;

  for (i=0; i < (int) entries; i++)
    if (WidgetTextWidth(font_info,(char *) list[i]) > text_width)
      text_width=WidgetTextWidth(font_info,(char *) list[i]);
  return(text_width);
}

typedef struct _ListBrowserReplyMatch
{
  const char
    *const *list;

  char
    *reply;

  unsigned int
    entries,
    visible_entries;
} ListBrowserReplyMatch;

static void MatchListBrowserReply(const ListBrowserReplyMatch *match,
  XWidgetInfo *list_info,XWidgetInfo *slider_info)
{
  const char
    *const *const list = match->list;

  char
    *const reply = match->reply;

  const unsigned int
    entries = match->entries,
    visible_entries = match->visible_entries;

  int
    i;

  for (i=0; i < (int) entries; i++)
    if (LocaleCompare(list[i],reply) == 0)
      {
        (*list_info).id=i;
        (*slider_info).id=i-(int) (visible_entries >> 1);
        if ((*slider_info).id < 0)
          (*slider_info).id=0;
      }
}

typedef struct _ListBrowserRedraw
{
  Display
    *display;

  XWindowInfo
    *window_info;

  const char
    *const *list;

  unsigned int
    entries,
    height,
    visible_entries;

  const XWidgetInfo
    *list_info;

  XWidgetInfo
    north_info,
    slider_info,
    south_info;
} ListBrowserRedraw;

static void RedrawListBrowserList(const ListBrowserRedraw *redraw,
  XWidgetInfo *selection_info,XWidgetInfo *expose_info)
{
  Display
    *const display = redraw->display;

  XWindowInfo
    *const window_info = redraw->window_info;

  const char
    *const *const list = redraw->list;

  const unsigned int
    entries = redraw->entries,
    height = redraw->height,
    visible_entries = redraw->visible_entries;

  const XWidgetInfo
    *const list_info = redraw->list_info;

  const XWidgetInfo
    north_info = redraw->north_info,
    slider_info = redraw->slider_info,
    south_info = redraw->south_info;

  int
    i;

  /*
    Redraw scroll bar and file names.
  */
  (*selection_info).id=slider_info.id;
  (*selection_info).y=(*list_info).y+(int) (height >> 3)+2;
  for (i=0; i < (int) visible_entries; i++)
  {
    (*selection_info).raised=(slider_info.id+i) != (*list_info).id ?
      MagickTrue : MagickFalse;
    (*selection_info).text=(char *) NULL;
    if ((slider_info.id+i) < (int) entries)
      (*selection_info).text=(char *) list[slider_info.id+i];
    XDrawWidgetText(display,window_info,&(*selection_info));
    (*selection_info).y+=(int) (*selection_info).height;
  }
  /*
    Update slider.
  */
  if (slider_info.y > (*expose_info).y)
    {
      (*expose_info).height=(unsigned int) (slider_info.y-(*expose_info).y);
      (*expose_info).y=slider_info.y-(int) (*expose_info).height-(int)
        slider_info.bevel_width-1;
    }
  else
    {
      (*expose_info).height=(unsigned int) ((*expose_info).y-slider_info.y);
      (*expose_info).y=slider_info.y+(int) slider_info.height+(int)
        slider_info.bevel_width+1;
    }
  XDrawTriangleNorth(display,window_info,&north_info);
  XDrawMatte(display,window_info,&(*expose_info));
  XDrawBeveledButton(display,window_info,&slider_info);
  XDrawTriangleSouth(display,window_info,&south_info);
  (*expose_info).y=slider_info.y;
}

static XWidgetInfo PageListBrowserSlider(XWidgetInfo slider_info,
  const XEvent event,const unsigned int visible_entries)
{
  if (event.xbutton.y < slider_info.y)
    slider_info.id-=(int) (visible_entries-1);
  else
    slider_info.id+=(int) (visible_entries-1);
  return(slider_info);
}

typedef struct _ListBrowserTargets
{
  XWidgetInfo
    *reply_info,
    *selection_info,
    *action_info;

  size_t
    *state;

  XWidgetInfo
    *list_info;
} ListBrowserTargets;

typedef struct _ListBrowserPress
{
  Display
    *display;

  XWindowInfo
    *window_info;

  const char
    *const *list;

  unsigned int
    entries,
    height;

  XEvent
    event;

  XWidgetInfo
    slider_info;
} ListBrowserPress;

static void SelectListBrowserEntry(const ListBrowserPress *press,
  const ListBrowserTargets *targets)
{
  Display
    *const display = press->display;

  XWindowInfo
    *const window_info = press->window_info;

  const char
    *const *const list = press->list;

  const unsigned int
    entries = press->entries,
    height = press->height;

  const XEvent
    event = press->event;

  const XWidgetInfo
    slider_info = press->slider_info;

  XWidgetInfo
    *const reply_info = targets->reply_info,
    *const selection_info = targets->selection_info,
    *const action_info = targets->action_info;

  size_t
    *const state = targets->state;

  XWidgetInfo
    *const list_info = targets->list_info;

  int
    id;

  /*
    User pressed list matte.
  */
  id=slider_info.id+(event.xbutton.y-((*list_info).y+(int)
    (height >> 1))+1)/(int) (*selection_info).height;
  if (id >= (int) entries)
    return;
  (void) CopyMagickString((*reply_info).text,list[id],MagickPathExtent);
  (*reply_info).highlight=MagickFalse;
  (*reply_info).marker=(*reply_info).text;
  (*reply_info).cursor=(*reply_info).text+Extent((*reply_info).text);
  XDrawMatteText(display,window_info,reply_info);
  (*selection_info).id=(~0);
  if (id == (*list_info).id)
    {
      (*action_info).raised=MagickFalse;
      XDrawBeveledButton(display,window_info,action_info);
      (*state)|=ExitState;
    }
  (*list_info).id=id;
  (*state)|=RedrawListState;
}

static int FindListBrowserCursor(XFontStruct *font_info,
  const XWidgetInfo reply_info,const int x)
{
  int
    i;

  for (i=1; i <= Extent(reply_info.marker); i++)
    if (XTextWidth(font_info,reply_info.marker,i) > x)
      break;
  return(i);
}

static XWidgetInfo HighlightListBrowserReply(const ListBrowserClick *click,
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

static size_t ConfirmListBrowserReply(Display *display,
  const XWidgetInfo reply_info,size_t state)
{
  if (*reply_info.text == '\0')
    (void) XBell(display,0);
  else
    state|=ExitState;
  return(state);
}

static void ReleaseListBrowserAction(const ListBrowserRelease *release,
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

static void ReleaseListBrowserCancel(const ListBrowserRelease *release,
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

static void MoveListBrowserSliderByPage(const KeySym key_symbol,
  const unsigned int visible_entries,const unsigned int entries,
  XWidgetInfo *slider_info)
{
  switch ((int) key_symbol)
  {
    case XK_Prior:
    case XK_KP_Prior:
    {
      (*slider_info).id-=(int) visible_entries;
      break;
    }
    case XK_Next:
    case XK_KP_Next:
    {
      (*slider_info).id+=(int) visible_entries;
      break;
    }
    case XK_End:
    case XK_KP_End:
    {
      (*slider_info).id=(int) entries;
      break;
    }
    default:
      break;
  }
}

static XWidgetInfo MoveListBrowserSlider(XWidgetInfo slider_info,
  const KeySym key_symbol,const unsigned int visible_entries,
  const unsigned int entries)
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
      MoveListBrowserSliderByPage(key_symbol,visible_entries,entries,
        &slider_info);
      break;
    }
  }
  return(slider_info);
}

static void ReadListBrowserEntry(Display *display,XWindowInfo *window_info,
  const XWidgetInfo reply_info,const ListBrowserTargets *targets)
{
  XWidgetInfo
    *const action_info = targets->action_info;

  size_t
    *const state = targets->state;

  /*
    Read new entry.
  */
  if (*reply_info.text == '\0')
    return;
  (*action_info).raised=MagickFalse;
  XDrawBeveledButton(display,window_info,action_info);
  (*state)|=ExitState;
}

static XWidgetInfo ApplyListBrowserControlKey(XWidgetInfo reply_info,
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

static XWidgetInfo TrackListBrowserSlider(XWidgetInfo slider_info,
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

MagickPrivate void XListBrowserWidget(Display *display,XWindows *windows,
  XWindowInfo *window_info,const char *const *list,const char *action,
  const char *query,char *reply)
{
#define CancelButtonText  "Cancel"

  char
    primary_selection[MagickPathExtent];

  int
    x;

  int
    i;

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  Status
    status;

  unsigned int
    entries,
    height,
    text_width,
    visible_entries,
    width;

  size_t
    delay,
    state;

  XEvent
    event;

  XFontStruct
    *font_info;

  XTextProperty
    window_name;

  XWidgetInfo
    action_info,
    cancel_info,
    expose_info,
    list_info,
    north_info,
    reply_info,
    scroll_info,
    selection_info,
    slider_info,
    south_info,
    text_info;

  XWindowChanges
    window_changes;

  const ListBrowserTargets
    targets = { &reply_info, &selection_info, &action_info, &state,
      &list_info };

  /*
    Count the number of entries in the list.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(window_info != (XWindowInfo *) NULL);
  assert(list != (const char **) NULL);
  assert(action != (char *) NULL);
  assert(query != (char *) NULL);
  assert(reply != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  if (list == (const char **) NULL)
    {
      XNoticeWidget(display,windows,"No text to browse:",(char *) NULL);
      return;
    }
  entries=CountListBrowserEntries(list);
  /*
    Determine Font Browser widget attributes.
  */
  font_info=window_info->font_info;
  text_width=WidgetTextWidth(font_info,(char *) query);
  text_width=WidestListBrowserEntry(font_info,list,entries,text_width);
  width=WidgetTextWidth(font_info,(char *) action);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  width+=(unsigned int) QuantumMargin;
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position List Browser widget.
  */
  window_info->width=MagickMin(text_width,MaxTextWidth)+(unsigned int)
    ((9*QuantumMargin) >> 1);
  window_info->min_width=(MinTextWidth+4*(unsigned int) QuantumMargin);
  if (window_info->width < window_info->min_width)
    window_info->width=window_info->min_width;
  window_info->height=(((81*height) >> 2)+(unsigned int)
    ((13*QuantumMargin) >> 1)+4);
  window_info->min_height=(((23*height) >> 1)+(unsigned int)
    ((13*QuantumMargin) >> 1)+4);
  if (window_info->height < window_info->min_height)
    window_info->height=window_info->min_height;
  XConstrainWindowPosition(display,window_info);
  /*
    Map List Browser widget.
  */
  (void) CopyMagickString(window_info->name,"Browse",MagickPathExtent);
  status=XStringListToTextProperty(&window_info->name,1,&window_name);
  if (status != False)
    {
      XSetWMName(display,window_info->id,&window_name);
      XSetWMIconName(display,windows->widget.id,&window_name);
      (void) XFree((void *) window_name.value);
    }
  window_changes.width=(int) window_info->width;
  window_changes.height=(int) window_info->height;
  window_changes.x=window_info->x;
  window_changes.y=window_info->y;
  (void) XReconfigureWMWindow(display,window_info->id,window_info->screen,mask,
    &window_changes);
  (void) XMapRaised(display,window_info->id);
  window_info->mapped=MagickFalse;
  /*
    Respond to X events.
  */
  XGetWidgetInfo((char *) NULL,&slider_info);
  XGetWidgetInfo((char *) NULL,&north_info);
  XGetWidgetInfo((char *) NULL,&south_info);
  XGetWidgetInfo((char *) NULL,&expose_info);
  XGetWidgetInfo((char *) NULL,&selection_info);
  visible_entries=0;
  delay=SuspendTime << 2;
  state=UpdateConfigurationState;
  do
  {
    if (state & UpdateConfigurationState)
      {
        int
          id;

        /*
          Initialize button information.
        */
        XGetWidgetInfo(CancelButtonText,&cancel_info);
        cancel_info.width=width;
        cancel_info.height=(unsigned int) ((3*height) >> 1);
        cancel_info.x=(int) window_info->width-(int) cancel_info.width-
          QuantumMargin-2;
        cancel_info.y=(int) window_info->height-(int) cancel_info.height-
          QuantumMargin;
        XGetWidgetInfo(action,&action_info);
        action_info.width=width;
        action_info.height=(unsigned int) ((3*height) >> 1);
        action_info.x=cancel_info.x-((int) cancel_info.width+
          (QuantumMargin >> 1)+(int) (action_info.bevel_width << 1));
        action_info.y=cancel_info.y;
        /*
          Initialize reply information.
        */
        XGetWidgetInfo(reply,&reply_info);
        reply_info.raised=MagickFalse;
        reply_info.bevel_width--;
        reply_info.width=(unsigned int) ((int) window_info->width-
          (((4*QuantumMargin) >> 1)));
        reply_info.height=height << 1;
        reply_info.x=QuantumMargin;
        reply_info.y=action_info.y-(int) reply_info.height-QuantumMargin;
        /*
          Initialize scroll information.
        */
        XGetWidgetInfo((char *) NULL,&scroll_info);
        scroll_info.bevel_width--;
        scroll_info.width=height;
        scroll_info.height=(unsigned int)
          (reply_info.y-((6*QuantumMargin) >> 1)-(int) height);
        scroll_info.x=reply_info.x+(int) (reply_info.width-scroll_info.width);
        scroll_info.y=((5*QuantumMargin) >> 1)+(int) height-(int)
          reply_info.bevel_width;
        scroll_info.raised=MagickFalse;
        scroll_info.trough=MagickTrue;
        north_info=scroll_info;
        north_info.raised=MagickTrue;
        north_info.width-=(north_info.bevel_width << 1);
        north_info.height=north_info.width-1;
        north_info.x+=(int) north_info.bevel_width;
        north_info.y+=(int) north_info.bevel_width;
        south_info=north_info;
        south_info.y=scroll_info.y+(int) scroll_info.height-(int)
          scroll_info.bevel_width-(int) south_info.height;
        id=slider_info.id;
        slider_info=north_info;
        slider_info.id=id;
        slider_info.width-=2;
        slider_info.min_y=north_info.y+(int) north_info.height+(int)
          north_info.bevel_width+(int) slider_info.bevel_width+2;
        slider_info.height=(unsigned int) ((int) scroll_info.height-
          ((slider_info.min_y-scroll_info.y+1) << 1)+4);
        visible_entries=(unsigned int) (scroll_info.height*
          MagickSafeReciprocal((double) height+(height >> 3)));
        if (entries > visible_entries)
          slider_info.height=(visible_entries*slider_info.height)/entries;
        slider_info.max_y=south_info.y-(int) south_info.bevel_width-(int)
          slider_info.bevel_width-2;
        slider_info.x=scroll_info.x+(int) slider_info.bevel_width+1;
        slider_info.y=slider_info.min_y;
        expose_info=scroll_info;
        expose_info.y=slider_info.y;
        /*
          Initialize list information.
        */
        XGetWidgetInfo((char *) NULL,&list_info);
        list_info.raised=MagickFalse;
        list_info.bevel_width--;
        list_info.width=(unsigned int)
          (scroll_info.x-reply_info.x-(int) (QuantumMargin >> 1));
        list_info.height=scroll_info.height;
        list_info.x=reply_info.x;
        list_info.y=scroll_info.y;
        if (window_info->mapped == MagickFalse)
          {
            ListBrowserReplyMatch
              match;

            match.list=list;
            match.reply=reply;
            match.entries=entries;
            match.visible_entries=visible_entries;
            MatchListBrowserReply(&match,&list_info,&slider_info);
          }
        /*
          Initialize text information.
        */
        XGetWidgetInfo(query,&text_info);
        text_info.width=reply_info.width;
        text_info.height=height;
        text_info.x=list_info.x-(int) (QuantumMargin >> 1);
        text_info.y=QuantumMargin;
        /*
          Initialize selection information.
        */
        XGetWidgetInfo((char *) NULL,&selection_info);
        selection_info.center=MagickFalse;
        selection_info.width=list_info.width;
        selection_info.height=(unsigned int) ((9*height) >> 3);
        selection_info.x=list_info.x;
        state&=(unsigned int) (~UpdateConfigurationState);
      }
    if (state & RedrawWidgetState)
      {
        /*
          Redraw List Browser window.
        */
        XDrawWidgetText(display,window_info,&text_info);
        XDrawBeveledMatte(display,window_info,&list_info);
        XDrawBeveledMatte(display,window_info,&scroll_info);
        XDrawTriangleNorth(display,window_info,&north_info);
        XDrawBeveledButton(display,window_info,&slider_info);
        XDrawTriangleSouth(display,window_info,&south_info);
        XDrawBeveledMatte(display,window_info,&reply_info);
        XDrawMatteText(display,window_info,&reply_info);
        XDrawBeveledButton(display,window_info,&action_info);
        XDrawBeveledButton(display,window_info,&cancel_info);
        XHighlightWidget(display,window_info,BorderOffset,BorderOffset);
        selection_info.id=(~0);
        state|=RedrawActionState;
        state|=RedrawListState;
        state&=(unsigned int) (~RedrawWidgetState);
      }
    if (state & RedrawListState)
      {
        /*
          Determine slider id and position.
        */
        if (slider_info.id >= (int) (entries-visible_entries))
          slider_info.id=(int) (entries-visible_entries);
        if ((slider_info.id < 0) || (entries <= visible_entries))
          slider_info.id=0;
        slider_info.y=slider_info.min_y;
        if (entries > 0)
          slider_info.y+=slider_info.id*(slider_info.max_y-
            slider_info.min_y+1)/(int) entries;
        if (slider_info.id != selection_info.id)
          {
            ListBrowserRedraw
              redraw;

            redraw.display=display;
            redraw.window_info=window_info;
            redraw.list=list;
            redraw.entries=entries;
            redraw.height=height;
            redraw.visible_entries=visible_entries;
            redraw.list_info=(&list_info);
            redraw.north_info=north_info;
            redraw.slider_info=slider_info;
            redraw.south_info=south_info;
            RedrawListBrowserList(&redraw,&selection_info,&expose_info);
          }
        state&=(unsigned int) (~RedrawListState);
      }
    /*
      Wait for next event.
    */
    if (north_info.raised && south_info.raised)
      (void) XIfEvent(display,&event,XScreenEvent,(char *) windows);
    else
      {
        /*
          Brief delay before advancing scroll bar.
        */
        XDelay(display,delay);
        delay=SuspendTime;
        (void) XCheckIfEvent(display,&event,XScreenEvent,(char *) windows);
        if ((north_info.raised == MagickFalse) && (slider_info.id > 0))
          {
            /*
              Move slider up.
            */
            slider_info.id--;
            state|=RedrawListState;
          }
        if ((south_info.raised == MagickFalse) &&
            (slider_info.id < (int) entries))
          {
            /*
              Move slider down.
            */
            slider_info.id++;
            state|=RedrawListState;
          }
        if (event.type != ButtonRelease)
          continue;
      }
    switch (event.type)
    {
      case ButtonPress:
      {
        if (MatteIsActive(slider_info,event.xbutton))
          {
            /*
              Track slider.
            */
            slider_info.active=MagickTrue;
            break;
          }
        if ((MatteIsActive(north_info,event.xbutton)) &&
            (slider_info.id > 0))
          {
            /*
              Move slider up.
            */
            north_info.raised=MagickFalse;
            slider_info.id--;
            state|=RedrawListState;
            break;
          }
        if ((MatteIsActive(south_info,event.xbutton)) &&
            (slider_info.id < (int) entries))
          {
            /*
              Move slider down.
            */
            south_info.raised=MagickFalse;
            slider_info.id++;
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(scroll_info,event.xbutton))
          {
            /*
              Move slider.
            */
            slider_info=PageListBrowserSlider(slider_info,event,
              visible_entries);
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(list_info,event.xbutton))
          {
            ListBrowserPress
              press;

            press.display=display;
            press.window_info=window_info;
            press.list=list;
            press.entries=entries;
            press.height=height;
            press.event=event;
            press.slider_info=slider_info;
            SelectListBrowserEntry(&press,&targets);
            break;
          }
        if (MatteIsActive(action_info,event.xbutton))
          {
            /*
              User pressed action button.
            */
            action_info.raised=MagickFalse;
            XDrawBeveledButton(display,window_info,&action_info);
            break;
          }
        if (MatteIsActive(cancel_info,event.xbutton))
          {
            /*
              User pressed Cancel button.
            */
            cancel_info.raised=MagickFalse;
            XDrawBeveledButton(display,window_info,&cancel_info);
            break;
          }
        if (MatteIsActive(reply_info,event.xbutton) == MagickFalse)
          break;
        if (event.xbutton.button != Button2)
          {
            static Time
              click_time;

            ListBrowserClick
              click;

            /*
              Move text cursor to position of button press.
            */
            x=event.xbutton.x-reply_info.x-(int) (QuantumMargin >> 2);
            i=FindListBrowserCursor(font_info,reply_info,x);
            reply_info.cursor=reply_info.marker+i-1;
            click.display=display;
            click.window_info=window_info;
            click.event=event;
            click.click_time=click_time;
            reply_info=HighlightListBrowserReply(&click,primary_selection,
              reply_info);
            XDrawMatteText(display,window_info,&reply_info);
            click_time=event.xbutton.time;
            break;
          }
        /*
          Request primary selection.
        */
        (void) XConvertSelection(display,XA_PRIMARY,XA_STRING,XA_STRING,
          window_info->id,event.xbutton.time);
        break;
      }
      case ButtonRelease:
      {
        ListBrowserRelease
          release;

        if (window_info->mapped == MagickFalse)
          break;
        if (north_info.raised == MagickFalse)
          {
            /*
              User released up button.
            */
            delay=SuspendTime << 2;
            north_info.raised=MagickTrue;
            XDrawTriangleNorth(display,window_info,&north_info);
          }
        if (south_info.raised == MagickFalse)
          {
            /*
              User released down button.
            */
            delay=SuspendTime << 2;
            south_info.raised=MagickTrue;
            XDrawTriangleSouth(display,window_info,&south_info);
          }
        if (slider_info.active)
          {
            /*
              Stop tracking slider.
            */
            slider_info.active=MagickFalse;
            break;
          }
        release.display=display;
        release.window_info=window_info;
        release.event=event;
        ReleaseListBrowserAction(&release,reply_info,&action_info,&state);
        release.display=display;
        release.window_info=window_info;
        release.event=event;
        ReleaseListBrowserCancel(&release,reply_info,&cancel_info,&state);
        if (MatteIsActive(reply_info,event.xbutton) == MagickFalse)
          break;
        break;
      }
      case ClientMessage:
      {
        /*
          If client window delete message, exit.
        */
        if (event.xclient.message_type != windows->wm_protocols)
          break;
        if (*event.xclient.data.l == (int) windows->wm_take_focus)
          {
            (void) XSetInputFocus(display,event.xclient.window,RevertToParent,
              (Time) event.xclient.data.l[1]);
            break;
          }
        if (*event.xclient.data.l != (int) windows->wm_delete_window)
          break;
        if (event.xclient.window == window_info->id)
          {
            *reply_info.text='\0';
            state|=ExitState;
            break;
          }
        break;
      }
      case ConfigureNotify:
      {
        /*
          Update widget configuration.
        */
        if (event.xconfigure.window != window_info->id)
          break;
        if ((event.xconfigure.width == (int) window_info->width) &&
            (event.xconfigure.height == (int) window_info->height))
          break;
        window_info->width=(unsigned int)
          MagickMax(event.xconfigure.width,(int) window_info->min_width);
        window_info->height=(unsigned int)
          MagickMax(event.xconfigure.height,(int) window_info->min_height);
        state|=UpdateConfigurationState;
        break;
      }
      case EnterNotify:
      {
        if (event.xcrossing.window != window_info->id)
          break;
        state&=(unsigned int) (~InactiveWidgetState);
        break;
      }
      case Expose:
      {
        if (event.xexpose.window != window_info->id)
          break;
        if (event.xexpose.count != 0)
          break;
        state|=RedrawWidgetState;
        break;
      }
      case KeyPress:
      {
        static char
          command[MagickPathExtent];

        static int
          length;

        static KeySym
          key_symbol;

        /*
          Respond to a user key press.
        */
        if (event.xkey.window != window_info->id)
          break;
        length=XLookupString((XKeyEvent *) &event.xkey,command,
          (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        *(command+length)='\0';
        if (AreaIsActive(scroll_info,event.xkey))
          {
            /*
              Move slider.
            */
            slider_info=MoveListBrowserSlider(slider_info,key_symbol,
              visible_entries,entries);
            state|=RedrawListState;
            break;
          }
        if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
          {
            ReadListBrowserEntry(display,window_info,reply_info,&targets);
            break;
          }
        if (key_symbol == XK_Control_L)
          {
            state|=ControlState;
            break;
          }
        reply_info=ApplyListBrowserControlKey(reply_info,key_symbol,state);
        XEditText(display,&reply_info,key_symbol,command,state);
        XDrawMatteText(display,window_info,&reply_info);
        break;
      }
      case KeyRelease:
      {
        static char
          command[MagickPathExtent];

        static KeySym
          key_symbol;

        /*
          Respond to a user key release.
        */
        if (event.xkey.window != window_info->id)
          break;
        (void) XLookupString((XKeyEvent *) &event.xkey,command,
          (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        if (key_symbol == XK_Control_L)
          state&=(unsigned int) (~ControlState);
        break;
      }
      case LeaveNotify:
      {
        if (event.xcrossing.window != window_info->id)
          break;
        state|=InactiveWidgetState;
        break;
      }
      case MapNotify:
      {
        mask&=(unsigned int) (~CWX);
        mask&=(unsigned int) (~CWY);
        break;
      }
      case MotionNotify:
      {
        /*
          Discard pending button motion events.
        */
        while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
        if (slider_info.active)
          {
            /*
              Move slider matte.
            */
            slider_info=TrackListBrowserSlider(slider_info,event,entries);
            state|=RedrawListState;
            break;
          }
        if (state & InactiveWidgetState)
          break;
        if (action_info.raised == MatteIsActive(action_info,event.xmotion))
          {
            /*
              Action button status changed.
            */
            action_info.raised=action_info.raised == MagickFalse ?
              MagickTrue : MagickFalse;
            XDrawBeveledButton(display,window_info,&action_info);
            break;
          }
        if (cancel_info.raised == MatteIsActive(cancel_info,event.xmotion))
          {
            /*
              Cancel button status changed.
            */
            cancel_info.raised=cancel_info.raised == MagickFalse ?
              MagickTrue : MagickFalse;
            XDrawBeveledButton(display,window_info,&cancel_info);
            break;
          }
        break;
      }
      case SelectionClear:
      {
        reply_info.highlight=MagickFalse;
        XDrawMatteText(display,window_info,&reply_info);
        break;
      }
      case SelectionNotify:
      {
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
          break;
        status=XGetWindowProperty(display,
          event.xselection.requestor,event.xselection.property,0L,2047L,
          MagickTrue,XA_STRING,&type,&format,&length,&after,&data);
        if (IsUnusableSelection(status,type,format,length) != MagickFalse)
          break;
        if ((Extent(reply_info.text)+(int) length) >= (MagickPathExtent-1))
          (void) XBell(display,0);
        else
          {
            /*
              Insert primary selection in reply text.
            */
            *(data+length)='\0';
            XEditText(display,&reply_info,(KeySym) XK_Insert,(char *) data,
              state);
            XDrawMatteText(display,window_info,&reply_info);
            state|=RedrawActionState;
          }
        (void) XFree((void *) data);
        break;
      }
      case SelectionRequest:
      {
        XSelectionEvent
          notify;

        XSelectionRequestEvent
          *request;

        if (reply_info.highlight == MagickFalse)
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
        (void) XSendEvent(request->display,request->requestor,False,NoEventMask,
          (XEvent *) &notify);
      }
      default:
        break;
    }
  } while ((state & ExitState) == 0);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,window_info->id,window_info->screen);
  XCheckRefreshWindows(display,windows);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X M e n u W i d g e t                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMenuWidget() maps a menu and returns the command pointed to by the user
%  when the button is released.
%
%  The format of the XMenuWidget method is:
%
%      int XMenuWidget(Display *display,XWindows *windows,const char *title,
%        const char *const *selections,char *item)
%
%  A description of each parameter follows:
%
%    o selection_number: Specifies the number of the selection that the
%      user choose.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o title: Specifies a character string that describes the menu selections.
%
%    o selections: Specifies a pointer to one or more strings that comprise
%      the choices in the menu.
%
%    o item: Specifies a character array.  The item selected from the menu
%      is returned here.
%
*/
static void PutBackMenuEvent(Display *display,XWindows *windows,
  XEvent event)
{
  if (event.xbutton.window == windows->command.id)
    (void) XPutBackEvent(display,&event);
}

typedef struct _MenuSelections
{
  Display
    *display;

  XWindows
    *windows;

  const char
    *const *selections;

  unsigned int
    number_selections;
} MenuSelections;

static void DrawMenuSelections(const MenuSelections *entries,
  XWidgetInfo *selection_info,XWidgetInfo *highlight_info)
{
  Display
    *const display = entries->display;

  XWindows
    *const windows = entries->windows;

  const char
    *const *const selections = entries->selections;

  const unsigned int
    number_selections = entries->number_selections;

  int
    id;

  for (id=0; id < (int) number_selections; id++)
  {
    (*selection_info).text=(char *) selections[id];
    XDrawWidgetText(display,&windows->widget,&(*selection_info));
    (*highlight_info).y=(*selection_info).y+(int) (*highlight_info).bevel_width;
    if (id == (*selection_info).id)
      XDrawBevel(display,&windows->widget,&(*highlight_info));
    (*selection_info).y+=(int) (*selection_info).height;
  }
}

typedef struct _MenuTargets
{
  XWidgetInfo
    *selection_info,
    *highlight_info;

  size_t
    *state;

  XWidgetInfo
    *menu_info;
} MenuTargets;

typedef struct _MenuPointer
{
  Display
    *display;

  XWindows
    *windows;

  const char
    *const *selections;

  unsigned int
    number_selections,
    top_offset;

  int
    id;
} MenuPointer;

static void EnterMenuSelection(const MenuPointer *pointer,
  const MenuTargets *targets)
{
  Display
    *const display = pointer->display;

  XWindows
    *const windows = pointer->windows;

  const char
    *const *const selections = pointer->selections;

  const unsigned int
    number_selections = pointer->number_selections,
    top_offset = pointer->top_offset;

  const int
    id = pointer->id;

  XWidgetInfo
    *const selection_info = targets->selection_info,
    *const highlight_info = targets->highlight_info;

  if (((*selection_info).id >= 0) &&
      ((*selection_info).id < (int) number_selections))
    {
      /*
        Unhighlight last selection.
      */
      if (id == (*selection_info).id)
        return;
      (*selection_info).y=((int) top_offset+(*selection_info).id*(int)
        (*selection_info).height);
      (*selection_info).text=(char *) selections[(*selection_info).id];
      XDrawWidgetText(display,&windows->widget,selection_info);
    }
  if ((id < 0) || (id >= (int) number_selections))
    return;
  /*
    Highlight this selection.
  */
  (*selection_info).id=id;
  (*selection_info).y=((int) top_offset+(*selection_info).id*(int)
    (*selection_info).height);
  (*selection_info).text=(char *) selections[(*selection_info).id];
  XDrawWidgetText(display,&windows->widget,selection_info);
  (*highlight_info).y=(*selection_info).y+(int) (*highlight_info).bevel_width;
  XDrawBevel(display,&windows->widget,highlight_info);
}

static void MoveMenuSelection(const MenuPointer *pointer,
  const MenuTargets *targets)
{
  Display
    *const display = pointer->display;

  XWindows
    *const windows = pointer->windows;

  const char
    *const *const selections = pointer->selections;

  const unsigned int
    number_selections = pointer->number_selections,
    top_offset = pointer->top_offset;

  const int
    id = pointer->id;

  XWidgetInfo
    *const selection_info = targets->selection_info,
    *const highlight_info = targets->highlight_info;

  if (((*selection_info).id >= 0) &&
      ((*selection_info).id < (int) number_selections))
    {
      /*
        Unhighlight last selection.
      */
      if (id == (*selection_info).id)
        return;
      (*selection_info).y=((int) top_offset+(*selection_info).id*(int)
        (*selection_info).height);
      (*selection_info).text=(char *) selections[(*selection_info).id];
      XDrawWidgetText(display,&windows->widget,selection_info);
    }
  (*selection_info).id=id;
  if ((id < 0) || (id >= (int) number_selections))
    return;
  /*
    Highlight this selection.
  */
  (*selection_info).y=((int) top_offset+id*(int) (*selection_info).height);
  (*selection_info).text=(char *) selections[id];
  XDrawWidgetText(display,&windows->widget,selection_info);
  (*highlight_info).y=(*selection_info).y+(int) (*highlight_info).bevel_width;
  XDrawBevel(display,&windows->widget,highlight_info);
}

typedef struct _MenuExitTargets
{
  XWidgetInfo
    *selection_info;

  size_t
    *state;
} MenuExitTargets;

static MagickBooleanType StayInMenuOnCommand(XWindows *windows,
  const XEvent event,char *item,const MenuExitTargets *targets)
{
  XWidgetInfo
    *const selection_info = targets->selection_info;

  size_t
    *const state = targets->state;

  if ((submenu_info.active != 0) &&
      (event.xmotion.window == windows->command.id))
    {
      if (((*state) & InactiveWidgetState) == 0)
        {
          if (MatteIsActive(submenu_info,event.xmotion) == MagickFalse)
            {
              (*selection_info).id=(~0);
              *item='\0';
              (*state)|=ExitState;
              return(MagickFalse);
            }
        }
      else
        if (WindowIsActive(windows->command,event.xmotion))
          {
            (*selection_info).id=(~0);
            *item='\0';
            (*state)|=ExitState;
            return(MagickFalse);
          }
    }
  return(MagickTrue);
}

static void MapMenuWidget(Display *display,XWindows *windows)
{
  XSetWindowAttributes
    window_attributes;

  XWindowChanges
    window_changes;

  window_attributes.override_redirect=MagickTrue;
  (void) XChangeWindowAttributes(display,windows->widget.id,
    (size_t) CWOverrideRedirect,&window_attributes);
  window_changes.width=(int) windows->widget.width;
  window_changes.height=(int) windows->widget.height;
  window_changes.x=windows->widget.x;
  window_changes.y=windows->widget.y;
  (void) XReconfigureWMWindow(display,windows->widget.id,windows->widget.screen,
    (unsigned int) (CWWidth | CWHeight | CWX | CWY),&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
}

typedef struct _MenuEventContext
{
  Display
    *display;

  XWindows
    *windows;

  const char
    *const *selections;

  unsigned int
    number_selections,
    top_offset;

  char
    *item;

  unsigned int
    height;

  const char
    *title;

  unsigned int
    title_height;
} MenuEventContext;

static void PressMenuSelection(const MenuEventContext *context,
  const XEvent event,const MenuTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const selections = context->selections;

  const unsigned int
    number_selections = context->number_selections,
    top_offset = context->top_offset;

  char
    *const item = context->item;

  XWidgetInfo
    *const selection_info = targets->selection_info,
    *const highlight_info = targets->highlight_info;

  size_t
    *const state = targets->state;

  int
    id;

  if (event.xbutton.window != windows->widget.id)
    {
      /*
        exit menu.
      */
      PutBackMenuEvent(display,windows,event);
      (*selection_info).id=(~0);
      *item='\0';
      (*state)|=ExitState;
      return;
    }
  (*state)&=(unsigned int) (~InactiveWidgetState);
  if ((*selection_info).height == 0)
    return;
  id=(event.xbutton.y-(int) top_offset)/(int) (*selection_info).height;
  (*selection_info).id=id;
  if ((id < 0) || (id >= (int) number_selections))
    return;
  /*
    Highlight this selection.
  */
  (*selection_info).y=((int) top_offset+id*(int) (*selection_info).height);
  (*selection_info).text=(char *) selections[id];
  XDrawWidgetText(display,&windows->widget,selection_info);
  (*highlight_info).y=(*selection_info).y+(int) (*highlight_info).bevel_width;
  XDrawBevel(display,&windows->widget,highlight_info);
}


static void ReleaseMenuSelection(const MenuEventContext *context,
  const XEvent event,const MenuTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const item = context->item;

  size_t
    *const state = targets->state;

  if (windows->widget.mapped == MagickFalse)
    return;
  if ((event.xbutton.window == windows->command.id) &&
      (((*state) & InactiveWidgetState) == 0))
    return;
  /*
    exit menu.
  */
  XSetCursorState(display,windows,MagickFalse);
  *item='\0';
  (*state)|=ExitState;
}

static void ConfigureMenuWindow(const MenuEventContext *context,
  const XEvent event,const MenuTargets *targets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = targets->state;

  /*
    Update widget configuration.
  */
  if (event.xconfigure.window != windows->widget.id)
    return;
  if ((event.xconfigure.width == (int) windows->widget.width) &&
      (event.xconfigure.height == (int) windows->widget.height))
    return;
  windows->widget.width=(unsigned int)
    MagickMax(event.xconfigure.width,(int) windows->widget.min_width);
  windows->widget.height=(unsigned int)
    MagickMax(event.xconfigure.height,(int) windows->widget.min_height);
  (*state)|=UpdateConfigurationState;
}

static void ExposeMenuWindow(const MenuEventContext *context,
  const XEvent event,const MenuTargets *targets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = targets->state;

  if (event.xexpose.window != windows->widget.id)
    return;
  if (event.xexpose.count != 0)
    return;
  (*state)|=RedrawWidgetState;
}

static void LeaveMenuWindow(const MenuEventContext *context,
  const XEvent event,const MenuTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const selections = context->selections;

  const unsigned int
    number_selections = context->number_selections,
    top_offset = context->top_offset;

  XWidgetInfo
    *const selection_info = targets->selection_info;

  size_t
    *const state = targets->state;

  int
    id;

  if (event.xcrossing.window != windows->widget.id)
    return;
  (*state)|=InactiveWidgetState;
  id=(*selection_info).id;
  if ((id < 0) || (id >= (int) number_selections))
    return;
  /*
    Unhighlight last selection.
  */
  (*selection_info).y=((int) top_offset+id*(int) (*selection_info).height);
  (*selection_info).id=(~0);
  (*selection_info).text=(char *) selections[id];
  XDrawWidgetText(display,&windows->widget,selection_info);
}

static void EnterMenuWindow(const MenuEventContext *context,
  const XEvent event,const MenuTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const selections = context->selections;

  const unsigned int
    number_selections = context->number_selections,
    top_offset = context->top_offset;

  XWidgetInfo
    *const selection_info = targets->selection_info;

  size_t
    *const state = targets->state;

  int
    id;

  MenuPointer
    pointer;

  if (event.xcrossing.window != windows->widget.id)
    return;
  if (event.xcrossing.state == 0)
    return;
  (*state)&=(unsigned int) (~InactiveWidgetState);
  if ((*selection_info).height == 0)
    return;
  id=((event.xcrossing.y-(int) top_offset)/(int) (*selection_info).height);
  pointer.display=display;
  pointer.windows=windows;
  pointer.selections=selections;
  pointer.number_selections=number_selections;
  pointer.top_offset=top_offset;
  pointer.id=id;
  EnterMenuSelection(&pointer,targets);
}

static void MoveMenuWindow(const MenuEventContext *context,
  XEvent event,const MenuTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const selections = context->selections;

  const unsigned int
    number_selections = context->number_selections,
    top_offset = context->top_offset;

  char
    *const item = context->item;

  XWidgetInfo
    *const selection_info = targets->selection_info;

  size_t
    *const state = targets->state;

  int
    id;

  MenuExitTargets
    exit_targets;

  MenuPointer
    pointer;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
  exit_targets.selection_info=(selection_info);
  exit_targets.state=(state);
  if (StayInMenuOnCommand(windows,event,item,&exit_targets) ==
      MagickFalse)
    return;
  if (event.xmotion.window != windows->widget.id)
    return;
  if ((*state) & InactiveWidgetState)
    return;
  if ((*selection_info).height == 0)
    return;
  id=(event.xmotion.y-(int) top_offset)/(int) (*selection_info).height;
  pointer.display=display;
  pointer.windows=windows;
  pointer.selections=selections;
  pointer.number_selections=number_selections;
  pointer.top_offset=top_offset;
  pointer.id=id;
  MoveMenuSelection(&pointer,targets);
}

static void UpdateMenuLayout(const MenuEventContext *context,
  const MenuTargets *targets)
{
  XWindows
    *const windows = context->windows;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const selection_info = targets->selection_info,
    *const highlight_info = targets->highlight_info;

  size_t
    *const state = targets->state;

  XWidgetInfo
    *const menu_info = targets->menu_info;

  if ((*state) & UpdateConfigurationState)
    {
      /*
        Initialize selection information.
      */
      XGetWidgetInfo((char *) NULL,menu_info);
      (*menu_info).bevel_width--;
      (*menu_info).width=windows->widget.width-
        (((*menu_info).bevel_width) << 1);
      (*menu_info).height=windows->widget.height-
        (((*menu_info).bevel_width) << 1);
      (*menu_info).x=(int) (*menu_info).bevel_width;
      (*menu_info).y=(int) (*menu_info).bevel_width;
      XGetWidgetInfo((char *) NULL,selection_info);
      (*selection_info).center=MagickFalse;
      (*selection_info).width=(*menu_info).width;
      (*selection_info).height=height;
      (*selection_info).x=(*menu_info).x;
      (*highlight_info)=(*selection_info);
      (*highlight_info).bevel_width--;
      (*highlight_info).width-=((*highlight_info).bevel_width << 1);
      (*highlight_info).height-=((*highlight_info).bevel_width << 1);
      (*highlight_info).x+=(int) (*highlight_info).bevel_width;
      (*state)&=(unsigned int) (~UpdateConfigurationState);
    }
}

static void RedrawMenuWidget(const MenuEventContext *context,
  const MenuTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const selections = context->selections;

  const unsigned int
    number_selections = context->number_selections,
    top_offset = context->top_offset;

  const char
    *const title = context->title;

  const unsigned int
    title_height = context->title_height;

  XWidgetInfo
    *const selection_info = targets->selection_info,
    *const highlight_info = targets->highlight_info;

  size_t
    *const state = targets->state;

  XWidgetInfo
    *const menu_info = targets->menu_info;

  int
    y;

  if ((*state) & RedrawWidgetState)
    {
      MenuSelections
        entries;

      /*
        Redraw Menu widget.
      */
      if (submenu_info.active == 0)
        {
          y=(int) title_height;
          XSetBevelColor(display,&windows->widget,MagickFalse);
          (void) XDrawLine(display,windows->widget.id,
            windows->widget.widget_context,(*selection_info).x,y-1,
            (int) (*selection_info).width,y-1);
          XSetBevelColor(display,&windows->widget,MagickTrue);
          (void) XDrawLine(display,windows->widget.id,
            windows->widget.widget_context,(*selection_info).x,y,
            (int) (*selection_info).width,y);
          (void) XSetFillStyle(display,windows->widget.widget_context,
            FillSolid);
        }
      /*
        Draw menu selections.
      */
      (*selection_info).center=MagickTrue;
      (*selection_info).y=(int) (*menu_info).bevel_width;
      (*selection_info).text=(char *) title;
      if (submenu_info.active == 0)
        XDrawWidgetText(display,&windows->widget,selection_info);
      (*selection_info).center=MagickFalse;
      (*selection_info).y=(int) top_offset;
      entries.display=display;
      entries.windows=windows;
      entries.selections=selections;
      entries.number_selections=number_selections;
      DrawMenuSelections(&entries,selection_info,highlight_info);
      XDrawBevel(display,&windows->widget,menu_info);
      (*state)&=(unsigned int) (~RedrawWidgetState);
    }
}

static void DrawMenuLine(const MenuEventContext *context,
  const MenuTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    number_selections = context->number_selections,
    top_offset = context->top_offset;

  XWidgetInfo
    *const selection_info = targets->selection_info;

  int
    y;

  if (number_selections > 2)
    {
      /*
        Redraw Menu line.
      */
      y=((int) top_offset+(int) (*selection_info).height*(int)
        (number_selections-1));
      XSetBevelColor(display,&windows->widget,MagickFalse);
      (void) XDrawLine(display,windows->widget.id,
        windows->widget.widget_context,(*selection_info).x,y-1,
        (int) (*selection_info).width,y-1);
      XSetBevelColor(display,&windows->widget,MagickTrue);
      (void) XDrawLine(display,windows->widget.id,
        windows->widget.widget_context,(*selection_info).x,y,
        (int) (*selection_info).width,y);
      (void) XSetFillStyle(display,windows->widget.widget_context,FillSolid);
    }
}

static void HandleMenuPointerEvent(const MenuEventContext *context,
  const XEvent event,const MenuTargets *targets)
{
  switch (event.type)
  {
    case Expose:
    {
      ExposeMenuWindow(context,event,targets);
      break;
    }
    case LeaveNotify:
    {
      LeaveMenuWindow(context,event,targets);
      break;
    }
    case MotionNotify:
    {
      MoveMenuWindow(context,event,targets);
      break;
    }
    default:
      break;
  }
}

static void HandleMenuEvent(const MenuEventContext *context,const XEvent event,
  const MenuTargets *targets)
{
  switch (event.type)
  {
    case ButtonPress:
    {
      PressMenuSelection(context,event,targets);
      break;
    }
    case ButtonRelease:
    {
      ReleaseMenuSelection(context,event,targets);
      break;
    }
    case ConfigureNotify:
    {
      ConfigureMenuWindow(context,event,targets);
      break;
    }
    case EnterNotify:
    {
      EnterMenuWindow(context,event,targets);
      break;
    }
    default:
    {
      HandleMenuPointerEvent(context,event,targets);
      break;
    }
  }
}

static int CloseMenuWidget(const MenuEventContext *context,
  const MenuTargets *targets,Cursor cursor)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const selections = context->selections;

  const unsigned int
    number_selections = context->number_selections;

  char
    *const item = context->item;

  XWidgetInfo
    *const selection_info = targets->selection_info;

  XSetWindowAttributes
    window_attributes;

  (void) XFreeCursor(display,cursor);
  window_attributes.override_redirect=MagickFalse;
  (void) XChangeWindowAttributes(display,windows->widget.id,
    (size_t) CWOverrideRedirect,&window_attributes);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
  if (submenu_info.active != 0)
    {
      submenu_info.active=MagickFalse;
      toggle_info.raised=MagickFalse;
      XDrawTriangleEast(display,&windows->command,&toggle_info);
    }
  if (((*selection_info).id < 0) ||
    ((*selection_info).id >= (int) number_selections))
    return(~0);
  (void) CopyMagickString(item,selections[(*selection_info).id],
    MagickPathExtent);
  return((*selection_info).id);
}

MagickPrivate int XMenuWidget(Display *display,XWindows *windows,
  const char *title,const char *const *selections,char *item)
{
  Cursor
    cursor;

  int
    id,
    x,
    y;

  unsigned int
    height,
    number_selections,
    title_height,
    top_offset,
    width;

  size_t
    state;

  XEvent
    event;

  XFontStruct
    *font_info;

  XWidgetInfo
    highlight_info,
    menu_info,
    selection_info;

  const MenuTargets
    targets = { &selection_info, &highlight_info, &state, &menu_info };

  MenuEventContext
    event_context;

  /*
    Determine Menu widget attributes.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(title != (char *) NULL);
  assert(selections != (const char **) NULL);
  assert(item != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",title);
  font_info=windows->widget.font_info;
  windows->widget.width=submenu_info.active == 0 ?
    WidgetTextWidth(font_info,(char *) title) : 0;
  for (id=0; selections[id] != (char *) NULL; id++)
  {
    width=WidgetTextWidth(font_info,(char *) selections[id]);
    if (width > windows->widget.width)
      windows->widget.width=width;
  }
  number_selections=(unsigned int) id;
  XGetWidgetInfo((char *) NULL,&menu_info);
  title_height=(unsigned int) (submenu_info.active == 0 ?
    (3*(font_info->descent+font_info->ascent) >> 1)+5 : 2);
  width=WidgetTextWidth(font_info,(char *) title);
  height=(unsigned int) ((3*(font_info->ascent+font_info->descent)) >> 1);
  /*
    Position Menu widget.
  */
  windows->widget.width+=(unsigned int) QuantumMargin+
    (menu_info.bevel_width << 1);
  top_offset=title_height+menu_info.bevel_width-1;
  windows->widget.height=top_offset+number_selections*height+4;
  windows->widget.min_width=windows->widget.width;
  windows->widget.min_height=windows->widget.height;
  XQueryPosition(display,windows->widget.root,&x,&y);
  windows->widget.x=x-(int) (QuantumMargin >> 1);
  if (submenu_info.active != 0)
    {
      windows->widget.x=windows->command.x+(int) windows->command.width-
        QuantumMargin;
      toggle_info.raised=MagickTrue;
      XDrawTriangleEast(display,&windows->command,&toggle_info);
    }
  windows->widget.y=submenu_info.active == 0 ? y-(int) (title_height >> 2) : y;
  if (submenu_info.active != 0)
    windows->widget.y=windows->command.y+submenu_info.y;
  XConstrainWindowPosition(display,&windows->widget);
  /*
    Map Menu widget.
  */
  MapMenuWidget(display,windows);
  /*
    Respond to X events.
  */
  selection_info.height=height;
  cursor=XCreateFontCursor(display,XC_right_ptr);
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  (void) XCheckDefineCursor(display,windows->command.id,cursor);
  (void) XCheckDefineCursor(display,windows->widget.id,cursor);
  state=UpdateConfigurationState;
  event_context.display=display;
  event_context.windows=windows;
  event_context.selections=selections;
  event_context.number_selections=number_selections;
  event_context.top_offset=top_offset;
  event_context.item=item;
  event_context.height=height;
  event_context.title=title;
  event_context.title_height=title_height;
  do
  {
    UpdateMenuLayout(&event_context,&targets);
    RedrawMenuWidget(&event_context,&targets);
    DrawMenuLine(&event_context,&targets);
    /*
      Wait for next event.
    */
    (void) XIfEvent(display,&event,XScreenEvent,(char *) windows);
    HandleMenuEvent(&event_context,event,&targets);
  } while ((state & ExitState) == 0);
  return(CloseMenuWidget(&event_context,&targets,cursor));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X N o t i c e W i d g e t                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XNoticeWidget() displays a Notice widget with a notice to the user.  The
%  function returns when the user presses the "Dismiss" button.
%
%  The format of the XNoticeWidget method is:
%
%      void XNoticeWidget(Display *display,XWindows *windows,
%        const char *reason,const char *description)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o reason: Specifies the message to display before terminating the
%      program.
%
%    o description: Specifies any description to the message.
%
*/
typedef struct _NoticeTargets
{
  XWidgetInfo
    *dismiss_info;

  size_t
    *state;
} NoticeTargets;

static void ReleaseNoticeDismiss(Display *display,XWindows *windows,
  const XEvent event,const NoticeTargets *targets)
{
  XWidgetInfo
    *const dismiss_info = targets->dismiss_info;

  size_t
    *const state = targets->state;

  if ((*dismiss_info).raised == MagickFalse)
    {
      if ((event.xbutton.window == windows->widget.id) &&
          (MatteIsActive((*dismiss_info),event.xbutton)))
        (*state)|=ExitState;
      (*dismiss_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
    }
}

static void PressNoticeDismiss(Display *display,XWindows *windows,
  const XEvent event,const NoticeTargets *targets)
{
  XWidgetInfo
    *const dismiss_info = targets->dismiss_info;

  if (MatteIsActive((*dismiss_info),event.xbutton))
    {
      /*
        User pressed Dismiss button.
      */
      (*dismiss_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      return;
    }
}

static void KeyNoticeDismiss(Display *display,XWindows *windows,
  const XEvent event,const NoticeTargets *targets)
{
  XWidgetInfo
    *const dismiss_info = targets->dismiss_info;

  size_t
    *const state = targets->state;

  static char
    command[MagickPathExtent];

  static KeySym
    key_symbol;

  /*
    Respond to a user key press.
  */
  if (event.xkey.window != windows->widget.id)
    return;
  (void) XLookupString((XKeyEvent *) &event.xkey,command,
    (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
    {
      (*dismiss_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      (*state)|=ExitState;
      return;
    }
}

static void MoveNoticeDismiss(Display *display,XWindows *windows,XEvent event,
  const NoticeTargets *targets)
{
  XWidgetInfo
    *const dismiss_info = targets->dismiss_info;

  size_t
    *const state = targets->state;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
  if ((*state) & InactiveWidgetState)
    return;
  if ((*dismiss_info).raised == MatteIsActive((*dismiss_info),event.xmotion))
    {
      /*
        Dismiss button status changed.
      */
      (*dismiss_info).raised=
        (*dismiss_info).raised == MagickFalse ? MagickTrue : MagickFalse;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      return;
    }
}

MagickPrivate void XNoticeWidget(Display *display,XWindows *windows,
  const char *reason,const char *description)
{
#define DismissButtonText  "Dismiss"
#define Timeout  8

  const char
    *text;

  int
    x,
    y;

  Status
    status;

  time_t
    timer;

  unsigned int
    height,
    width;

  size_t
    state;

  XEvent
    event;

  XFontStruct
    *font_info;

  XTextProperty
    window_name;

  XWidgetInfo
    dismiss_info;

  XWindowChanges
    window_changes;

  const NoticeTargets
    targets = { &dismiss_info, &state };

  /*
    Determine Notice widget attributes.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(reason != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",reason);
  XDelay(display,SuspendTime << 3);  /* avoid surprise with delay */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  font_info=windows->widget.font_info;
  width=WidgetTextWidth(font_info,DismissButtonText);
  text=GetLocaleExceptionMessage(XServerError,reason);
  if (text != (char *) NULL)
    if (WidgetTextWidth(font_info,(char *) text) > width)
      width=WidgetTextWidth(font_info,(char *) text);
  if (description != (char *) NULL)
    {
      text=GetLocaleExceptionMessage(XServerError,description);
      if (text != (char *) NULL)
        if (WidgetTextWidth(font_info,(char *) text) > width)
          width=WidgetTextWidth(font_info,(char *) text);
    }
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position Notice widget.
  */
  windows->widget.width=width+(unsigned int) (4*QuantumMargin);
  windows->widget.min_width=width+(unsigned int) QuantumMargin;
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int) (12*height);
  windows->widget.min_height=(unsigned int) (7*height);
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
  /*
    Map Notice widget.
  */
  (void) CopyMagickString(windows->widget.name,"Notice",MagickPathExtent);
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
    (unsigned int) (CWWidth | CWHeight | CWX | CWY),&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
  (void) XBell(display,0);
  /*
    Respond to X events.
  */
  timer=GetMagickTime()+Timeout;
  state=UpdateConfigurationState;
  do
  {
    if (GetMagickTime() > timer)
      break;
    if (state & UpdateConfigurationState)
      {
        /*
          Initialize Dismiss button information.
        */
        XGetWidgetInfo(DismissButtonText,&dismiss_info);
        dismiss_info.width=(unsigned int) QuantumMargin+
          WidgetTextWidth(font_info,DismissButtonText);
        dismiss_info.height=(unsigned int) ((3*height) >> 1);
        dismiss_info.x=(int)
          ((windows->widget.width >> 1)-(dismiss_info.width >> 1));
        dismiss_info.y=(int)
          (windows->widget.height-(dismiss_info.height << 1));
        state&=(unsigned int) (~UpdateConfigurationState);
      }
    if (state & RedrawWidgetState)
      {
        /*
          Redraw Notice widget.
        */
        width=WidgetTextWidth(font_info,(char *) reason);
        x=(int) ((windows->widget.width >> 1)-(width >> 1));
        y=(int) ((windows->widget.height >> 1)-(height << 1));
        (void) XDrawString(display,windows->widget.id,
          windows->widget.annotate_context,x,y,(char *) reason,Extent(reason));
        if (description != (char *) NULL)
          {
            width=WidgetTextWidth(font_info,(char *) description);
            x=(int) ((windows->widget.width >> 1)-(width >> 1));
            y+=(int) height;
            (void) XDrawString(display,windows->widget.id,
              windows->widget.annotate_context,x,y,(char *) description,
              Extent(description));
          }
        XDrawBeveledButton(display,&windows->widget,&dismiss_info);
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        state&=(unsigned int) (~RedrawWidgetState);
      }
    /*
      Wait for next event.
    */
    if (XCheckIfEvent(display,&event,XScreenEvent,(char *) windows) == MagickFalse)
      {
        /*
          Do not block if delay > 0.
        */
        XDelay(display,SuspendTime << 2);
        continue;
      }
    switch (event.type)
    {
      case ButtonPress:
      {
        PressNoticeDismiss(display,windows,event,&targets);
        break;
      }
      case ButtonRelease:
      {
        if (windows->widget.mapped == MagickFalse)
          break;
        ReleaseNoticeDismiss(display,windows,event,&targets);
        break;
      }
      case ClientMessage:
      {
        state=ClientWidgetMessage(display,windows,event,state);
        break;
      }
      case ConfigureNotify:
      {
        state=ConfigureWidgetWindow(windows,event,state);
        break;
      }
      case EnterNotify:
      {
        state=EnterWidgetWindow(windows,event,state);
        break;
      }
      case Expose:
      {
        state=ExposeWidgetWindow(windows,event,state);
        break;
      }
      case KeyPress:
      {
        KeyNoticeDismiss(display,windows,event,&targets);
        break;
      }
      case LeaveNotify:
      {
        state=LeaveWidgetWindow(windows,event,state);
        break;
      }
      case MotionNotify:
      {
        MoveNoticeDismiss(display,windows,event,&targets);
        break;
      }
      default:
        break;
    }
  } while ((state & ExitState) == 0);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X P r e f e r e n c e s W i d g e t                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XPreferencesWidget() displays a Preferences widget with program preferences.
%  If the user presses the Apply button, the preferences are stored in a
%  configuration file in the users' home directory.
%
%  The format of the XPreferencesWidget method is:
%
%      MagickBooleanType XPreferencesWidget(Display *display,
%        XResourceInfo *resource_info,XWindows *windows)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o window: Specifies a pointer to a XWindows structure.
%
*/
#define ApplyButtonText  "Apply"
#define CacheButtonText  "%lu mega-bytes of memory in the undo edit cache   "
#define CancelButtonText  "Cancel"
#define NumberPreferences  8

static size_t ReleasePreferencesButton(XWindows *windows,const XEvent event,
  const XWidgetInfo button_info,size_t state)
{
  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(button_info,event.xbutton)))
    state|=ExitState;
  return(state);
}

static void TogglePreferencesButton(Display *display,XWindows *windows,
  const XEvent event,XWidgetInfo *preferences_info)
{
  int
    i;

  for (i=0; i < NumberPreferences; i++)
    if (MatteIsActive(preferences_info[i],event.xbutton))
      {
        /*
          User pressed a Preferences button.
        */
        preferences_info[i].raised=preferences_info[i].raised ==
          MagickFalse ? MagickTrue : MagickFalse;
        XDrawBeveledButton(display,&windows->widget,&preferences_info[i]);
        break;
      }
}

static void DoublePreferencesUndoCache(XResourceInfo *resource_info)
{
  resource_info->undo_cache<<=1;
  if (resource_info->undo_cache > 256)
    resource_info->undo_cache=1;
}

MagickPrivate MagickBooleanType XPreferencesWidget(Display *display,
  XResourceInfo *resource_info,XWindows *windows)
{
  static const char
    *Preferences[] =
    {
      "display image centered on a backdrop",
      "confirm on program exit",
      "confirm on image edits",
      "correct image for display gamma",
      "display warning messages",
      "apply Floyd/Steinberg error diffusion to image",
      "use a shared colormap for colormapped X visuals",
      "display images as an X server pixmap"
    };

  char
    cache[MagickPathExtent];

  int
    x,
    y;

  int
    i;

  Status
    status;

  unsigned int
    height,
    text_width,
    width;

  size_t
    state;

  XEvent
    event;

  XFontStruct
    *font_info;

  XTextProperty
    window_name;

  XWidgetInfo
    apply_info,
    cache_info,
    cancel_info,
    preferences_info[NumberPreferences];

  XWindowChanges
    window_changes;

  /*
    Determine Preferences widget attributes.
  */
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(display != (Display *) NULL);
  assert(resource_info != (XResourceInfo *) NULL);
  assert(windows != (XWindows *) NULL);
  XCheckRefreshWindows(display,windows);
  font_info=windows->widget.font_info;
  text_width=WidgetTextWidth(font_info,CacheButtonText);
  for (i=0; i < NumberPreferences; i++)
    if (WidgetTextWidth(font_info,(char *) Preferences[i]) > text_width)
      text_width=WidgetTextWidth(font_info,(char *) Preferences[i]);
  width=WidgetTextWidth(font_info,ApplyButtonText);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  width+=(unsigned int) QuantumMargin;
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position Preferences widget.
  */
  windows->widget.width=(unsigned int) (MagickMax((int) (width << 1),
    (int) text_width)+6*QuantumMargin);
  windows->widget.min_width=(width << 1)+(unsigned int) QuantumMargin;
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int) (7*(int) height+NumberPreferences*
    ((int) height+(QuantumMargin >> 1)));
  windows->widget.min_height=(unsigned int) (7*(int) height+NumberPreferences*
    ((int) height+(QuantumMargin >> 1)));
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
  /*
    Map Preferences widget.
  */
  (void) CopyMagickString(windows->widget.name,"Preferences",MagickPathExtent);
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
    (unsigned int) (CWWidth | CWHeight | CWX | CWY),&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
  /*
    Respond to X events.
  */
  state=UpdateConfigurationState;
  XSetCursorState(display,windows,MagickTrue);
  do
  {
    if (state & UpdateConfigurationState)
      {
        /*
          Initialize button information.
        */
        XGetWidgetInfo(CancelButtonText,&cancel_info);
        cancel_info.width=width;
        cancel_info.height=(unsigned int) (3*height) >> 1;
        cancel_info.x=(int) windows->widget.width-(int) cancel_info.width-
          (QuantumMargin << 1);
        cancel_info.y=(int) windows->widget.height-(int) cancel_info.height-
          QuantumMargin;
        XGetWidgetInfo(ApplyButtonText,&apply_info);
        apply_info.width=width;
        apply_info.height=(unsigned int) (3*height) >> 1;
        apply_info.x=QuantumMargin << 1;
        apply_info.y=cancel_info.y;
        y=(int) (height << 1);
        for (i=0; i < NumberPreferences; i++)
        {
          XGetWidgetInfo(Preferences[i],&preferences_info[i]);
          preferences_info[i].bevel_width--;
          preferences_info[i].width=(unsigned int) QuantumMargin >> 1;
          preferences_info[i].height=(unsigned int) QuantumMargin >> 1;
          preferences_info[i].x=QuantumMargin << 1;
          preferences_info[i].y=y;
          y+=(int) height+(QuantumMargin >> 1);
        }
        preferences_info[0].raised=resource_info->backdrop ==
          MagickFalse ? MagickTrue : MagickFalse;
        preferences_info[1].raised=resource_info->confirm_exit ==
          MagickFalse ? MagickTrue : MagickFalse;
        preferences_info[2].raised=resource_info->confirm_edit ==
          MagickFalse ? MagickTrue : MagickFalse;
        preferences_info[3].raised=resource_info->gamma_correct ==
          MagickFalse ? MagickTrue : MagickFalse;
        preferences_info[4].raised=resource_info->display_warnings ==
          MagickFalse ? MagickTrue : MagickFalse;
        preferences_info[5].raised=
          resource_info->quantize_info->dither_method == NoDitherMethod ?
          MagickTrue : MagickFalse;
        preferences_info[6].raised=resource_info->colormap !=
          SharedColormap ? MagickTrue : MagickFalse;
        preferences_info[7].raised=resource_info->use_pixmap ==
          MagickFalse ? MagickTrue : MagickFalse;
        (void) FormatLocaleString(cache,MagickPathExtent,CacheButtonText,
          (unsigned long) resource_info->undo_cache);
        XGetWidgetInfo(cache,&cache_info);
        cache_info.bevel_width--;
        cache_info.width=(unsigned int) QuantumMargin >> 1;
        cache_info.height=(unsigned int) QuantumMargin >> 1;
        cache_info.x=QuantumMargin << 1;
        cache_info.y=y;
        state&=(unsigned int) (~UpdateConfigurationState);
      }
    if (state & RedrawWidgetState)
      {
        /*
          Redraw Preferences widget.
        */
        XDrawBeveledButton(display,&windows->widget,&apply_info);
        XDrawBeveledButton(display,&windows->widget,&cancel_info);
        for (i=0; i < NumberPreferences; i++)
          XDrawBeveledButton(display,&windows->widget,&preferences_info[i]);
        XDrawTriangleEast(display,&windows->widget,&cache_info);
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        state&=(unsigned int) (~RedrawWidgetState);
      }
    /*
      Wait for next event.
    */
    (void) XIfEvent(display,&event,XScreenEvent,(char *) windows);
    switch (event.type)
    {
      case ButtonPress:
      {
        if (MatteIsActive(apply_info,event.xbutton))
          {
            /*
              User pressed Apply button.
            */
            apply_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&apply_info);
            break;
          }
        if (MatteIsActive(cancel_info,event.xbutton))
          {
            /*
              User pressed Cancel button.
            */
            cancel_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&cancel_info);
            break;
          }
        TogglePreferencesButton(display,windows,event,preferences_info);
        if (MatteIsActive(cache_info,event.xbutton))
          {
            /*
              User pressed Cache button.
            */
            x=cache_info.x+(int) cache_info.width+(int) cache_info.bevel_width+
              (QuantumMargin >> 1);
            y=cache_info.y+(int) ((cache_info.height-height) >> 1);
            width=WidgetTextWidth(font_info,cache);
            (void) XClearArea(display,windows->widget.id,x,y,width,height,
              False);
            DoublePreferencesUndoCache(resource_info);
            (void) FormatLocaleString(cache,MagickPathExtent,CacheButtonText,
              (unsigned long) resource_info->undo_cache);
            cache_info.raised=MagickFalse;
            XDrawTriangleEast(display,&windows->widget,&cache_info);
            break;
          }
        break;
      }
      case ButtonRelease:
      {
        if (windows->widget.mapped == MagickFalse)
          break;
        if (apply_info.raised == MagickFalse)
          {
            state=ReleasePreferencesButton(windows,event,apply_info,state);
            apply_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&apply_info);
            apply_info.raised=MagickFalse;
          }
        if (cancel_info.raised == MagickFalse)
          {
            state=ReleasePreferencesButton(windows,event,cancel_info,state);
            cancel_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&cancel_info);
          }
        if (cache_info.raised == MagickFalse)
          {
            cache_info.raised=MagickTrue;
            XDrawTriangleEast(display,&windows->widget,&cache_info);
          }
        break;
      }
      case ClientMessage:
      {
        state=ClientWidgetMessage(display,windows,event,state);
        break;
      }
      case ConfigureNotify:
      {
        state=ConfigureWidgetWindow(windows,event,state);
        break;
      }
      case EnterNotify:
      {
        state=EnterWidgetWindow(windows,event,state);
        break;
      }
      case Expose:
      {
        state=ExposeWidgetWindow(windows,event,state);
        break;
      }
      case KeyPress:
      {
        static char
          command[MagickPathExtent];

        static KeySym
          key_symbol;

        /*
          Respond to a user key press.
        */
        if (event.xkey.window != windows->widget.id)
          break;
        (void) XLookupString((XKeyEvent *) &event.xkey,command,
          (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
          {
            apply_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&apply_info);
            state|=ExitState;
            break;
          }
        break;
      }
      case LeaveNotify:
      {
        state=LeaveWidgetWindow(windows,event,state);
        break;
      }
      case MotionNotify:
      {
        /*
          Discard pending button motion events.
        */
        while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
        if (state & InactiveWidgetState)
          break;
        if (apply_info.raised == MatteIsActive(apply_info,event.xmotion))
          {
            /*
              Apply button status changed.
            */
            apply_info.raised=
              apply_info.raised == MagickFalse ? MagickTrue : MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&apply_info);
            break;
          }
        if (cancel_info.raised == MatteIsActive(cancel_info,event.xmotion))
          {
            /*
              Cancel button status changed.
            */
            cancel_info.raised=
              cancel_info.raised == MagickFalse ? MagickTrue : MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&cancel_info);
            break;
          }
        break;
      }
      default:
        break;
    }
  } while ((state & ExitState) == 0);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
  if (apply_info.raised)
    return(MagickFalse);
  /*
    Save user preferences to the client configuration file.
  */
  resource_info->backdrop=
    preferences_info[0].raised == MagickFalse ? MagickTrue : MagickFalse;
  resource_info->confirm_exit=
    preferences_info[1].raised == MagickFalse ? MagickTrue : MagickFalse;
  resource_info->confirm_edit=
    preferences_info[2].raised == MagickFalse ? MagickTrue : MagickFalse;
  resource_info->gamma_correct=
    preferences_info[3].raised == MagickFalse ? MagickTrue : MagickFalse;
  resource_info->display_warnings=
     preferences_info[4].raised == MagickFalse ? MagickTrue : MagickFalse;
  resource_info->quantize_info->dither_method=
    preferences_info[5].raised == MagickFalse ?
    RiemersmaDitherMethod : NoDitherMethod;
  resource_info->colormap=SharedColormap;
  if (preferences_info[6].raised)
    resource_info->colormap=PrivateColormap;
  resource_info->use_pixmap=
    preferences_info[7].raised == MagickFalse ? MagickTrue : MagickFalse;
  XUserPreferences(resource_info);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X P r o g r e s s M o n i t o r W i d g e t                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XProgressMonitorWidget() displays the progress a task is making in
%  completing a task.  A span of zero toggles the active status.  An inactive
%  state disables the progress monitor.
%
%  The format of the XProgressMonitorWidget method is:
%
%      void XProgressMonitorWidget(Display *display,XWindows *windows,
%        const char *task,const MagickOffsetType offset,
%        const MagickSizeType span)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o task: Identifies the task in progress.
%
%    o offset: Specifies the offset position within the span which represents
%      how much progress has been made in completing a task.
%
%    o span: Specifies the span relative to completing a task.
%
*/
static void XRefreshExposedWindows(Display *display,XWindows *windows)
{
  XEvent
    event;

  while (XCheckTypedWindowEvent(display,windows->command.id,Expose,&event))
    (void) XCommandWidget(display,windows,(const char *const *) NULL,&event);
  while (XCheckTypedWindowEvent(display,windows->image.id,Expose,&event))
    XRefreshWindow(display,&windows->image,&event);
  while (XCheckTypedWindowEvent(display,windows->info.id,Expose,&event))
    if (monitor_info.text != (char *) NULL)
      XInfoWidget(display,windows,monitor_info.text);
}

MagickPrivate void XProgressMonitorWidget(Display *display,XWindows *windows,
  const char *task,const MagickOffsetType offset,const MagickSizeType span)
{
  unsigned int
    width;

  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(task != (const char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",task);
  if (span == 0)
    return;
  /*
    Update image windows if there is a pending expose event.
  */
  XRefreshExposedWindows(display,windows);
  /*
    Draw progress monitor bar to represent percent completion of a task.
  */
  if ((windows->info.mapped == MagickFalse) || (task != monitor_info.text))
    XInfoWidget(display,windows,task);
  width=(unsigned int) (((offset+1)*((int) windows->info.width-
    (2*monitor_info.x)))/(int) span);
  if (width < monitor_info.width)
    {
      monitor_info.raised=MagickTrue;
      XDrawWidgetText(display,&windows->info,&monitor_info);
      monitor_info.raised=MagickFalse;
    }
  monitor_info.width=width;
  XDrawWidgetText(display,&windows->info,&monitor_info);
  (void) XFlush(display);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X T e x t V i e w W i d g e t                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XTextViewWidget() displays text in a Text View widget.
%
%  The format of the XTextViewWidget method is:
%
%      void XTextViewWidget(Display *display,const XResourceInfo *resource_info,
%        XWindows *windows,const MagickBooleanType mono,const char *title,
%        const char **textlist)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o mono:  Use mono-spaced font when displaying text.
%
%    o title: This character string is displayed at the top of the widget
%      window.
%
%    o textlist: This string list is displayed within the Text View widget.
%
*/
typedef struct _TextViewLines
{
  Display
    *display;

  XWindows
    *windows;

  const char
    **textlist;

  unsigned int
    lines,
    visible_lines;

  const XWidgetInfo
    *list_info;

  XWidgetInfo
    slider_info;
} TextViewLines;

static void DrawTextViewLines(const TextViewLines *view,
  XWidgetInfo *selection_info)
{
  Display
    *const display = view->display;

  XWindows
    *const windows = view->windows;

  const char
    **const textlist = view->textlist;

  const unsigned int
    lines = view->lines,
    visible_lines = view->visible_lines;

  const XWidgetInfo
    *const list_info = view->list_info;

  const XWidgetInfo
    slider_info = view->slider_info;

  int
    i;

  for (i=0; i < (int) visible_lines; i++)
  {
    (*selection_info).raised=
      (slider_info.id+i) != (*list_info).id ? MagickTrue : MagickFalse;
    (*selection_info).text=(char *) NULL;
    if ((slider_info.id+i) < (int) lines)
      (*selection_info).text=(char *) textlist[slider_info.id+i];
    XDrawWidgetText(display,&windows->widget,&(*selection_info));
    (*selection_info).y+=(int) (*selection_info).height;
  }
}

static XWidgetInfo PlaceTextViewExpose(XWidgetInfo expose_info,
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

typedef struct _TextViewTargets
{
  XWidgetInfo
    *list_info,
    *selection_info;

  size_t
    *state;
} TextViewTargets;

typedef struct _TextViewPress
{
  Display
    *display;

  XWindows
    *windows;

  const char
    **textlist;

  unsigned int
    lines,
    height;

  XEvent
    event;

  XWidgetInfo
    slider_info;
} TextViewPress;

static void SelectTextViewLine(const TextViewPress *press,
  char *primary_selection,const TextViewTargets *targets)
{
  Display
    *const display = press->display;

  XWindows
    *const windows = press->windows;

  const char
    **const textlist = press->textlist;

  const unsigned int
    lines = press->lines,
    height = press->height;

  const XEvent
    event = press->event;

  const XWidgetInfo
    slider_info = press->slider_info;

  XWidgetInfo
    *const list_info = targets->list_info,
    *const selection_info = targets->selection_info;

  size_t
    *const state = targets->state;

  int
    id;

  static Time
    click_time;

  /*
    User pressed list matte.
  */
  id=slider_info.id+(event.xbutton.y-((*list_info).y+(int)
    (height >> 1))+1)/(int) (*selection_info).height;
  if (id >= (int) lines)
    return;
  if (id != (*list_info).id)
    {
      (*list_info).id=id;
      click_time=event.xbutton.time;
      return;
    }
  (*list_info).id=id;
  if (event.xbutton.time >= (click_time+(unsigned long) DoubleClick))
    {
      click_time=event.xbutton.time;
      return;
    }
  click_time=event.xbutton.time;
  /*
    Become the XA_PRIMARY selection owner.
  */
  (void) CopyMagickString(primary_selection,textlist[(*list_info).id],
    MagickPathExtent);
  (void) XSetSelectionOwner(display,XA_PRIMARY,windows->widget.id,
    event.xbutton.time);
  if (XGetSelectionOwner(display,XA_PRIMARY) != windows->widget.id)
    return;
  (*selection_info).id=(~0);
  (*list_info).id=id;
  (*state)|=RedrawListState;
}

static void ReleaseTextViewDismiss(const DialogRelease *release,
  XWidgetInfo *dismiss_info,size_t *state)
{
  Display
    *const display = release->display;

  XWindows
    *const windows = release->windows;

  const XEvent
    event = release->event;

  if ((*dismiss_info).raised == MagickFalse)
    {
      if ((event.xbutton.window == windows->widget.id) &&
          (MatteIsActive((*dismiss_info),event.xbutton)))
        (*state)|=ExitState;
      (*dismiss_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,&(*dismiss_info));
    }
}

static void MoveTextViewSliderByPage(const KeySym key_symbol,
  const unsigned int visible_lines,const unsigned int lines,
  XWidgetInfo *slider_info)
{
  switch ((int) key_symbol)
  {
    case XK_Prior:
    case XK_KP_Prior:
    {
      (*slider_info).id-=(int) visible_lines;
      break;
    }
    case XK_Next:
    case XK_KP_Next:
    {
      (*slider_info).id+=(int) visible_lines;
      break;
    }
    case XK_End:
    case XK_KP_End:
    {
      (*slider_info).id=(int) lines;
      break;
    }
    default:
      break;
  }
}

static XWidgetInfo MoveTextViewSlider(XWidgetInfo slider_info,
  const KeySym key_symbol,const unsigned int visible_lines,
  const unsigned int lines)
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
      MoveTextViewSliderByPage(key_symbol,visible_lines,lines,&slider_info);
      break;
    }
  }
  return(slider_info);
}

MagickPrivate void XTextViewWidget(Display *display,
  const XResourceInfo *resource_info,XWindows *windows,
  const MagickBooleanType mono,const char *title,const char **textlist)
{
#define DismissButtonText  "Dismiss"

  char
    primary_selection[MagickPathExtent];

  int
    i;

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  Status
    status;

  unsigned int
    height,
    lines,
    text_width,
    visible_lines,
    width;

  size_t
    delay,
    state;

  XEvent
    event;

  XFontStruct
    *font_info,
    *text_info;

  XTextProperty
    window_name;

  XWidgetInfo
    dismiss_info,
    expose_info,
    list_info,
    north_info,
    scroll_info,
    selection_info,
    slider_info,
    south_info;

  XWindowChanges
    window_changes;

  const TextViewTargets
    targets = { &list_info, &selection_info, &state };

  /*
    Convert text string to a text list.
  */
  assert(display != (Display *) NULL);
  assert(resource_info != (XResourceInfo *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(title != (const char *) NULL);
  assert(textlist != (const char **) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",title);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  if (textlist == (const char **) NULL)
    {
      XNoticeWidget(display,windows,"No text to view:",(char *) NULL);
      return;
    }
  /*
    Determine Text View widget attributes.
  */
  font_info=windows->widget.font_info;
  text_info=(XFontStruct *) NULL;
  if (mono != MagickFalse)
    text_info=XBestFont(display,resource_info,MagickTrue);
  if (text_info == (XFontStruct *) NULL)
    text_info=windows->widget.font_info;
  text_width=0;
  for (i=0; textlist[i] != (char *) NULL; i++)
    if (WidgetTextWidth(text_info,(char *) textlist[i]) > text_width)
      text_width=(unsigned int) XTextWidth(text_info,(char *) textlist[i],
        MagickMin(Extent(textlist[i]),160));
  lines=(unsigned int) i;
  width=WidgetTextWidth(font_info,DismissButtonText);
  width+=(unsigned int) QuantumMargin;
  height=(unsigned int) (text_info->ascent+text_info->descent);
  /*
    Position Text View widget.
  */
  windows->widget.width=(unsigned int) (MagickMin((int) text_width,
    (int) MaxTextWidth)+5*QuantumMargin);
  windows->widget.min_width=(unsigned int) ((int) MinTextWidth+4*QuantumMargin);
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int) (MagickMin(MagickMax((int) lines,3),32)*
    (int) height+(int) ((13*height) >> 1)+((9*QuantumMargin) >> 1));
  windows->widget.min_height=(3*height+((13*height) >> 1)+(unsigned int) ((9*
    QuantumMargin) >> 1));
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
  /*
    Map Text View widget.
  */
  (void) CopyMagickString(windows->widget.name,title,MagickPathExtent);
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
  (void) XReconfigureWMWindow(display,windows->widget.id,
    windows->widget.screen,(unsigned int) mask,&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
  /*
    Respond to X events.
  */
  XGetWidgetInfo((char *) NULL,&slider_info);
  XGetWidgetInfo((char *) NULL,&north_info);
  XGetWidgetInfo((char *) NULL,&south_info);
  XGetWidgetInfo((char *) NULL,&expose_info);
  XGetWidgetInfo((char *) NULL,&selection_info);
  visible_lines=0;
  delay=SuspendTime << 2;
  height=(unsigned int) (font_info->ascent+font_info->descent);
  state=UpdateConfigurationState;
  do
  {
    if (state & UpdateConfigurationState)
      {
        int
          id;

        /*
          Initialize button information.
        */
        XGetWidgetInfo(DismissButtonText,&dismiss_info);
        dismiss_info.width=width;
        dismiss_info.height=(unsigned int) ((3*height) >> 1);
        dismiss_info.x=(int) windows->widget.width-(int) dismiss_info.width-
          QuantumMargin-2;
        dismiss_info.y=(int) windows->widget.height-(int) dismiss_info.height-
          QuantumMargin;
        /*
          Initialize scroll information.
        */
        XGetWidgetInfo((char *) NULL,&scroll_info);
        scroll_info.bevel_width--;
        scroll_info.width=height;
        scroll_info.height=(unsigned int) (dismiss_info.y-((5*QuantumMargin) >>
          1));
        scroll_info.x=(int) windows->widget.width-QuantumMargin-(int)
          scroll_info.width;
        scroll_info.y=(3*QuantumMargin) >> 1;
        scroll_info.raised=MagickFalse;
        scroll_info.trough=MagickTrue;
        north_info=scroll_info;
        north_info.raised=MagickTrue;
        north_info.width-=(north_info.bevel_width << 1);
        north_info.height=north_info.width-1;
        north_info.x+=(int) north_info.bevel_width;
        north_info.y+=(int) north_info.bevel_width;
        south_info=north_info;
        south_info.y=scroll_info.y+(int) scroll_info.height-(int)
          scroll_info.bevel_width-(int) south_info.height;
        id=slider_info.id;
        slider_info=north_info;
        slider_info.id=id;
        slider_info.width-=2;
        slider_info.min_y=north_info.y+(int) north_info.height+(int)
          north_info.bevel_width+(int) slider_info.bevel_width+2;
        slider_info.height=(unsigned int) ((int) scroll_info.height-
          ((slider_info.min_y-scroll_info.y+1) << 1)+4);
        visible_lines=(unsigned int) (scroll_info.height*MagickSafeReciprocal(
          (double) text_info->ascent+text_info->descent+((text_info->ascent+
          text_info->descent) >> 3)));
        if (lines > visible_lines)
          slider_info.height=(unsigned int) (visible_lines*slider_info.height)/
            lines;
        slider_info.max_y=south_info.y-(int) south_info.bevel_width-(int)
          slider_info.bevel_width-2;
        slider_info.x=scroll_info.x+(int) slider_info.bevel_width+1;
        slider_info.y=slider_info.min_y;
        expose_info=scroll_info;
        expose_info.y=slider_info.y;
        /*
          Initialize list information.
        */
        XGetWidgetInfo((char *) NULL,&list_info);
        list_info.raised=MagickFalse;
        list_info.bevel_width--;
        list_info.width=(unsigned int) (scroll_info.x-((3*QuantumMargin) >> 1));
        list_info.height=scroll_info.height;
        list_info.x=QuantumMargin;
        list_info.y=scroll_info.y;
        /*
          Initialize selection information.
        */
        XGetWidgetInfo((char *) NULL,&selection_info);
        selection_info.center=MagickFalse;
        selection_info.width=list_info.width;
        selection_info.height=(unsigned int)
          (9*(text_info->ascent+text_info->descent)) >> 3;
        selection_info.x=list_info.x;
        state&=(unsigned int) (~UpdateConfigurationState);
      }
    if (state & RedrawWidgetState)
      {
        /*
          Redraw Text View window.
        */
        XDrawBeveledMatte(display,&windows->widget,&list_info);
        XDrawBeveledMatte(display,&windows->widget,&scroll_info);
        XDrawTriangleNorth(display,&windows->widget,&north_info);
        XDrawBeveledButton(display,&windows->widget,&slider_info);
        XDrawTriangleSouth(display,&windows->widget,&south_info);
        XDrawBeveledButton(display,&windows->widget,&dismiss_info);
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        selection_info.id=(~0);
        state|=RedrawListState;
        state&=(unsigned int) (~RedrawWidgetState);
      }
    if (state & RedrawListState)
      {
        /*
          Determine slider id and position.
        */
        if (slider_info.id >= (int) (lines-visible_lines))
          slider_info.id=(int) (lines-visible_lines);
        if ((slider_info.id < 0) || (lines <= visible_lines))
          slider_info.id=0;
        slider_info.y=slider_info.min_y;
        if (lines != 0)
          slider_info.y+=slider_info.id*(slider_info.max_y-
            slider_info.min_y+1)/(int) lines;
        if (slider_info.id != selection_info.id)
          {
            TextViewLines
              view;

            /*
              Redraw scroll bar and text.
            */
            windows->widget.font_info=text_info;
            (void) XSetFont(display,windows->widget.annotate_context,
              text_info->fid);
            (void) XSetFont(display,windows->widget.highlight_context,
              text_info->fid);
            selection_info.id=slider_info.id;
            selection_info.y=list_info.y+(int) (height >> 3)+2;
            view.display=display;
            view.windows=windows;
            view.textlist=textlist;
            view.lines=lines;
            view.visible_lines=visible_lines;
            view.list_info=(&list_info);
            view.slider_info=slider_info;
            DrawTextViewLines(&view,&selection_info);
            windows->widget.font_info=font_info;
            (void) XSetFont(display,windows->widget.annotate_context,
              font_info->fid);
            (void) XSetFont(display,windows->widget.highlight_context,
              font_info->fid);
            /*
              Update slider.
            */
            expose_info=PlaceTextViewExpose(expose_info,slider_info);
            XDrawTriangleNorth(display,&windows->widget,&north_info);
            XDrawMatte(display,&windows->widget,&expose_info);
            XDrawBeveledButton(display,&windows->widget,&slider_info);
            XDrawTriangleSouth(display,&windows->widget,&south_info);
            expose_info.y=slider_info.y;
          }
        state&=(unsigned int) (~RedrawListState);
      }
    /*
      Wait for next event.
    */
    if (north_info.raised && south_info.raised)
      (void) XIfEvent(display,&event,XScreenEvent,(char *) windows);
    else
      {
        /*
          Brief delay before advancing scroll bar.
        */
        XDelay(display,delay);
        delay=SuspendTime;
        (void) XCheckIfEvent(display,&event,XScreenEvent,(char *) windows);
        if ((north_info.raised == MagickFalse) && (slider_info.id > 0))
          {
            /*
              Move slider up.
            */
            slider_info.id--;
            state|=RedrawListState;
          }
        if ((south_info.raised == MagickFalse) &&
            (slider_info.id < (int) lines))
          {
            /*
              Move slider down.
            */
            slider_info.id++;
            state|=RedrawListState;
          }
        if (event.type != ButtonRelease)
          continue;
      }
    switch (event.type)
    {
      case ButtonPress:
      {
        if (MatteIsActive(slider_info,event.xbutton))
          {
            /*
              Track slider.
            */
            slider_info.active=MagickTrue;
            break;
          }
        if ((MatteIsActive(north_info,event.xbutton)) &&
            (slider_info.id > 0))
          {
            /*
              Move slider up.
            */
            north_info.raised=MagickFalse;
            slider_info.id--;
            state|=RedrawListState;
            break;
          }
        if ((MatteIsActive(south_info,event.xbutton)) &&
            (slider_info.id < (int) lines))
          {
            /*
              Move slider down.
            */
            south_info.raised=MagickFalse;
            slider_info.id++;
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(scroll_info,event.xbutton))
          {
            /*
              Move slider.
            */
            slider_info=PageListBrowserSlider(slider_info,event,visible_lines);
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(dismiss_info,event.xbutton))
          {
            /*
              User pressed Dismiss button.
            */
            dismiss_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&dismiss_info);
            break;
          }
        if (MatteIsActive(list_info,event.xbutton))
          {
            TextViewPress
              press;

            press.display=display;
            press.windows=windows;
            press.textlist=textlist;
            press.lines=lines;
            press.height=height;
            press.event=event;
            press.slider_info=slider_info;
            SelectTextViewLine(&press,primary_selection,&targets);
            break;
          }
        break;
      }
      case ButtonRelease:
      {
        DialogRelease
          release;

        if (windows->widget.mapped == MagickFalse)
          break;
        if (north_info.raised == MagickFalse)
          {
            /*
              User released up button.
            */
            delay=SuspendTime << 2;
            north_info.raised=MagickTrue;
            XDrawTriangleNorth(display,&windows->widget,&north_info);
          }
        if (south_info.raised == MagickFalse)
          {
            /*
              User released down button.
            */
            delay=SuspendTime << 2;
            south_info.raised=MagickTrue;
            XDrawTriangleSouth(display,&windows->widget,&south_info);
          }
        if (slider_info.active)
          {
            /*
              Stop tracking slider.
            */
            slider_info.active=MagickFalse;
            break;
          }
        release.display=display;
        release.windows=windows;
        release.event=event;
        ReleaseTextViewDismiss(&release,&dismiss_info,&state);
        break;
      }
      case ClientMessage:
      {
        state=ClientWidgetMessage(display,windows,event,state);
        break;
      }
      case ConfigureNotify:
      {
        state=ConfigureWidgetWindow(windows,event,state);
        break;
      }
      case EnterNotify:
      {
        state=EnterWidgetWindow(windows,event,state);
        break;
      }
      case Expose:
      {
        state=ExposeWidgetWindow(windows,event,state);
        break;
      }
      case KeyPress:
      {
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
          break;
        length=XLookupString((XKeyEvent *) &event.xkey,command,
          (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        *(command+length)='\0';
        if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
          {
            dismiss_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&dismiss_info);
            state|=ExitState;
            break;
          }
        if (AreaIsActive(scroll_info,event.xkey))
          {
            /*
              Move slider.
            */
            slider_info=MoveTextViewSlider(slider_info,key_symbol,visible_lines,
              lines);
            state|=RedrawListState;
            break;
          }
        break;
      }
      case KeyRelease:
        break;
      case LeaveNotify:
      {
        state=LeaveWidgetWindow(windows,event,state);
        break;
      }
      case MapNotify:
      {
        mask&=(unsigned int) (~CWX);
        mask&=(unsigned int) (~CWY);
        break;
      }
      case MotionNotify:
      {
        /*
          Discard pending button motion events.
        */
        while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
        if (slider_info.active)
          {
            /*
              Move slider matte.
            */
            slider_info=TrackListBrowserSlider(slider_info,event,lines);
            state|=RedrawListState;
            break;
          }
        if (state & InactiveWidgetState)
          break;
        if (dismiss_info.raised == MatteIsActive(dismiss_info,event.xmotion))
          {
            /*
              Dismiss button status changed.
            */
            dismiss_info.raised=
              dismiss_info.raised == MagickFalse ? MagickTrue : MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&dismiss_info);
            break;
          }
        break;
      }
      case SelectionClear:
      {
        list_info.id=(~0);
        selection_info.id=(~0);
        state|=RedrawListState;
        break;
      }
      case SelectionRequest:
      {
        XSelectionEvent
          notify;

        XSelectionRequestEvent
          *request;

        if (list_info.id == (~0))
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
        (void) XSendEvent(request->display,request->requestor,False,NoEventMask,
          (XEvent *) &notify);
      }
      default:
        break;
    }
  } while ((state & ExitState) == 0);
  if (text_info != windows->widget.font_info)
    (void) XFreeFont(display,text_info);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
}
#endif
