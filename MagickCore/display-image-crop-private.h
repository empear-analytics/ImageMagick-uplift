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

  MagickCore private methods of display-image.c: the draw stipple, draw-
  element and preview handlers, from XCreateStippleBitmap to
  XErasePointLineBox; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_CROP_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_CROP_PRIVATE_H

static Pixmap XCreateStippleBitmap(Display *display,const Window root_window,
  const int entry)
{
  Pixmap
    stipple;

  switch (entry)
  {
    case 0:
    {
      stipple=XCreateBitmapFromData(display,root_window,
        (char *) BricksBitmap,BricksWidth,BricksHeight);
      break;
    }
    case 1:
    {
      stipple=XCreateBitmapFromData(display,root_window,
        (char *) DiagonalBitmap,DiagonalWidth,DiagonalHeight);
      break;
    }
    case 2:
    {
      stipple=XCreateBitmapFromData(display,root_window,
        (char *) ScalesBitmap,ScalesWidth,ScalesHeight);
      break;
    }
    case 3:
    {
      stipple=XCreateBitmapFromData(display,root_window,
        (char *) VerticalBitmap,VerticalWidth,VerticalHeight);
      break;
    }
    default:
    {
      XCreateStippleBitmapTail(display,root_window,entry,&stipple);
      break;
    }
  }
  return(stipple);
}

static void XReadStippleFile(const DrawEditContext *context,
  const DrawStyleTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const Window
    root_window = context->root_window;

  ExceptionInfo
    *const exception = context->exception;

  Pixmap
    *const stipple = targets->stipple;

  int
    *const x = targets->x;

  int
    *const y = targets->y;

  Image
    *stipple_image;

  ImageInfo
    *image_info;

  int
    status;

  static char
    filename[MagickPathExtent] = "\0";

  unsigned int
    height,
    width;

  XFileBrowserWidget(display,windows,"Stipple",filename);
  if (*filename == '\0')
    return;
  /*
    Read image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  image_info=AcquireImageInfo();
  (void) CopyMagickString(image_info->filename,filename,
    MagickPathExtent);
  stipple_image=ReadImage(image_info,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (stipple_image == (Image *) NULL)
    return;
  (void) AcquireUniqueFileResource(filename);
  (void) FormatLocaleString(stipple_image->filename,
    MagickPathExtent,"xbm:%s",filename);
  (void) WriteImage(image_info,stipple_image,exception);
  stipple_image=DestroyImage(stipple_image);
  image_info=DestroyImageInfo(image_info);
  status=XReadBitmapFile(display,root_window,filename,&width,
    &height,stipple,x,y);
  (void) RelinquishUniqueFileResource(filename);
  if ((status != BitmapSuccess) != 0)
    XNoticeWidget(display,windows,"Unable to read X bitmap image:",
      filename);
}

static void XSelectDrawStipple(const DrawEditContext *context,const int id,
  const DrawStyleTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const DrawMenu = context->DrawMenu;

  char
    *const command = context->command;

  const Window
    root_window = context->root_window;

  Pixmap
    *const stipple = targets->stipple;

  const char
    *StipplesMenu[] =
    {
      "Brick",
      "Diagonal",
      "Scales",
      "Vertical",
      "Wavy",
      "Translucent",
      "Opaque",
      (char *) NULL,
      (char *) NULL,
    };

  int
    entry;

  /*
    Select a command from the pop-up menu.
  */
  StipplesMenu[7]="Open...";
  {
    const XMenuRequest
      menu_request = { DrawMenu[id], StipplesMenu, command };

    entry=XMenuWidget(display,windows,&menu_request);
  }
  if (entry < 0)
    return;
  if ((*stipple) != (Pixmap) NULL)
    (void) XFreePixmap(display,(*stipple));
  (*stipple)=(Pixmap) NULL;
  if (entry != 7)
    {
      (*stipple)=XCreateStippleBitmap(display,root_window,entry);
      return;
    }
  XReadStippleFile(context,targets);
}

typedef struct _DrawPointerTargets
{
  int
    *x;

  int
    *y;

  size_t
    *state;

  XEvent
    *event;

  ElementType
    *element;
} DrawPointerTargets;

static void XDrawButtonPress(const DrawEditContext *context,
  const XEvent event,const DrawPointerTargets *targets)
{
  XWindows
    *const windows = context->windows;

  int
    *const x = targets->x;

  int
    *const y = targets->y;

  size_t
    *const state = targets->state;

  if (event.xbutton.button != Button1)
    return;
  if (event.xbutton.window != windows->image.id)
    return;
  /*
    exit loop.
  */
  (*x)=event.xbutton.x;
  (*y)=event.xbutton.y;
  (*state)|=ExitState;
}

static void XDrawKeyPress(const DrawEditContext *context,
  const DrawPointerTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = targets->state;

  XEvent
    *const event = targets->event;

  KeySym
    key_symbol;

  char
    command[MagickPathExtent];

  if ((*event).xkey.window != windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Rotation",ImageDrawHelp);
      }
      break;
    }
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
}

static RectangleInfo XFitSelectionRectangle(XWindows *windows,const int x,
  const int y,RectangleInfo rectangle_info)
{
  if (rectangle_info.x < 0)
    rectangle_info.x=0;
  else
    if (rectangle_info.x > (ssize_t) windows->image.width)
      rectangle_info.x=(ssize_t) windows->image.width;
  if ((int) rectangle_info.x < x)
    rectangle_info.width=(unsigned int) (x-rectangle_info.x);
  else
    {
      rectangle_info.width=(unsigned int) (rectangle_info.x-x);
      rectangle_info.x=(ssize_t) x;
    }
  if (rectangle_info.y < 0)
    rectangle_info.y=0;
  else
    if (rectangle_info.y > (ssize_t) windows->image.height)
      rectangle_info.y=(ssize_t) windows->image.height;
  if ((int) rectangle_info.y < y)
    rectangle_info.height=(unsigned int) (y-rectangle_info.y);
  else
    {
      rectangle_info.height=(unsigned int) (rectangle_info.y-y);
      rectangle_info.y=(ssize_t) y;
    }
  return(rectangle_info);
}

static RectangleInfo XDrawPolygonBounds(XPoint *coordinate_info,
  const int number_coordinates,RectangleInfo rectangle_info)
{
  int
    i,
    x,
    y;

  /*
    Determine polygon bounding box.
  */
  rectangle_info.x=(ssize_t) coordinate_info->x;
  rectangle_info.y=(ssize_t) coordinate_info->y;
  x=coordinate_info->x;
  y=coordinate_info->y;
  for (i=1; i < number_coordinates; i++)
  {
    if (coordinate_info[i].x > x)
      x=coordinate_info[i].x;
    if (coordinate_info[i].y > y)
      y=coordinate_info[i].y;
    if ((ssize_t) coordinate_info[i].x < rectangle_info.x)
      rectangle_info.x=MagickMax((ssize_t) coordinate_info[i].x,0);
    if ((ssize_t) coordinate_info[i].y < rectangle_info.y)
      rectangle_info.y=MagickMax((ssize_t) coordinate_info[i].y,0);
  }
  rectangle_info.width=(size_t) (x-rectangle_info.x);
  rectangle_info.height=(size_t) (y-rectangle_info.y);
  for (i=0; i < number_coordinates; i++)
  {
    coordinate_info[i].x-=rectangle_info.x;
    coordinate_info[i].y-=rectangle_info.y;
  }
  return(rectangle_info);
}

typedef struct _DrawShapeInput
{
  ElementType
    element;

  unsigned int
    distance;
} DrawShapeInput;

typedef struct _DrawShapeTargets
{
  XSegment
    *line_info;

  RectangleInfo
    *rectangle_info;

  XPoint
    **coordinate_info;

  size_t
    *state;

  int
    *number_coordinates;

  unsigned int
    *max_coordinates;
} DrawShapeTargets;

static void XDrawButtonRelease(const DrawEditContext *context,
  const DrawShapeInput *shape,const XEvent event,
  const DrawShapeTargets *targets)
{
  ExceptionInfo
    *const exception = context->exception;

  const ElementType
    element = shape->element;

  const unsigned int
    distance = shape->distance;

  XSegment
    *const line_info = targets->line_info;

  RectangleInfo
    *const rectangle_info = targets->rectangle_info;

  XPoint
    **const coordinate_info = targets->coordinate_info;

  size_t
    *const state = targets->state;

  int
    *const number_coordinates = targets->number_coordinates;

  unsigned int
    *const max_coordinates = targets->max_coordinates;

  /*
    User has committed to element.
  */
  (*line_info).x2=event.xbutton.x;
  (*line_info).y2=event.xbutton.y;
  (*rectangle_info).x=(ssize_t) event.xbutton.x;
  (*rectangle_info).y=(ssize_t) event.xbutton.y;
  (*coordinate_info)[(*number_coordinates)].x=event.xbutton.x;
  (*coordinate_info)[(*number_coordinates)].y=event.xbutton.y;
  if (IsElementComplete(element,distance) != MagickFalse)
    {
      (*state)|=ExitState;
      return;
    }
  (*number_coordinates)++;
  if ((*number_coordinates) < (int) (*max_coordinates))
    {
      (*line_info).x1=event.xbutton.x;
      (*line_info).y1=event.xbutton.y;
      return;
    }
  (*max_coordinates)<<=1;
  (*coordinate_info)=(XPoint *) ResizeQuantumMemory((*coordinate_info),
    (*max_coordinates),sizeof(*(*coordinate_info)));
  if ((*coordinate_info) == (XPoint *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),
      ResourceLimitError,"MemoryAllocationFailed","`%s'","...");
}

static void XDrawMotion(const DrawEditContext *context,
  const DrawShapeInput *shape,const XEvent event,
  const DrawShapeTargets *targets)
{
  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  const ElementType
    element = shape->element;

  XSegment
    *const line_info = targets->line_info;

  RectangleInfo
    *const rectangle_info = targets->rectangle_info;

  XPoint
    **const coordinate_info = targets->coordinate_info;

  int
    *const number_coordinates = targets->number_coordinates;

  unsigned int
    *const max_coordinates = targets->max_coordinates;

  if (event.xmotion.window != windows->image.id)
    return;
  if (element != PointElement)
    {
      (*line_info).x2=event.xmotion.x;
      (*line_info).y2=event.xmotion.y;
      (*rectangle_info).x=(ssize_t) event.xmotion.x;
      (*rectangle_info).y=(ssize_t) event.xmotion.y;
      return;
    }
  (*coordinate_info)[(*number_coordinates)].x=event.xbutton.x;
  (*coordinate_info)[(*number_coordinates)].y=event.xbutton.y;
  (*number_coordinates)++;
  if ((*number_coordinates) < (int) (*max_coordinates))
    return;
  (*max_coordinates)<<=1;
  (*coordinate_info)=(XPoint *) ResizeQuantumMemory((*coordinate_info),
    (*max_coordinates),sizeof(*(*coordinate_info)));
  if ((*coordinate_info) == (XPoint *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),
      ResourceLimitError,"MemoryAllocationFailed","`%s'","...");
}

typedef struct _DrawPreviewInput
{
  ElementType
    element;

  int
    number_coordinates;

  XPoint
    *coordinate_info;

  unsigned int
    distance;

  XSegment
    line_info;

  RectangleInfo
    rectangle_info;
} DrawPreviewInput;

static void XPreviewLine(const DrawEditContext *context,
  const DrawPreviewInput *preview,double *degrees)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    distance = preview->distance;

  const XSegment
    line_info = preview->line_info;

  char
    text[MagickPathExtent];

  if (distance > 9)
    {
      /*
        Display angle of the line.
      */
      (*degrees)=RadiansToDegrees(-atan2((double) (line_info.y2-
        line_info.y1),(double) (line_info.x2-line_info.x1)));
      (void) FormatLocaleString(text,MagickPathExtent," %g",
        (double) (*degrees));
      XInfoWidget(display,windows,text);
      XHighlightLine(display,windows->image.id,
        windows->image.highlight_context,&line_info);
    }
  else
    if (windows->info.mapped != MagickFalse)
      (void) XWithdrawWindow(display,windows->info.id,
        windows->info.screen);
}

static void XPreviewPolyline(const DrawEditContext *context,
  const DrawPreviewInput *preview)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const int
    number_coordinates = preview->number_coordinates;

  XPoint
    *const coordinate_info = preview->coordinate_info;

  if (number_coordinates > 1)
    (void) XDrawLines(display,windows->image.id,
      windows->image.highlight_context,coordinate_info,
      number_coordinates,CoordModeOrigin);
}

static void XPreviewLongLine(const DrawEditContext *context,
  const DrawPreviewInput *preview)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    distance = preview->distance;

  const XSegment
    line_info = preview->line_info;

  if (distance > 9)
    XHighlightLine(display,windows->image.id,
      windows->image.highlight_context,&line_info);
}

static void XPreviewPoint(const DrawEditContext *context,
  const DrawPreviewInput *preview)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const int
    number_coordinates = preview->number_coordinates;

  XPoint
    *const coordinate_info = preview->coordinate_info;

  char
    text[MagickPathExtent];

  if (number_coordinates > 1)
    {
      (void) XDrawLines(display,windows->image.id,
        windows->image.highlight_context,coordinate_info,
        number_coordinates,CoordModeOrigin);
      (void) FormatLocaleString(text,MagickPathExtent," %+d%+d",
        coordinate_info[number_coordinates-1].x,
        coordinate_info[number_coordinates-1].y);
      XInfoWidget(display,windows,text);
    }
}

static void XPreviewRectangle(const DrawEditContext *context,
  const DrawPreviewInput *preview)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const RectangleInfo
    rectangle_info = preview->rectangle_info;

  char
    text[MagickPathExtent];

  if ((rectangle_info.width > 3) && (rectangle_info.height > 3))
    {
      /*
        Display info and draw drawing rectangle.
      */
      (void) FormatLocaleString(text,MagickPathExtent,
        " %.17gx%.17g%+.20g%+.20g",(double) rectangle_info.width,
        (double) rectangle_info.height,(double) rectangle_info.x,
        (double) rectangle_info.y);
      XInfoWidget(display,windows,text);
      XHighlightRectangle(display,windows->image.id,
        windows->image.highlight_context,&rectangle_info);
    }
  else
    if (windows->info.mapped != MagickFalse)
      (void) XWithdrawWindow(display,windows->info.id,
        windows->info.screen);
}

static void XPreviewEllipse(const DrawEditContext *context,
  const DrawPreviewInput *preview)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const RectangleInfo
    rectangle_info = preview->rectangle_info;

  char
    text[MagickPathExtent];

  if ((rectangle_info.width > 3) && (rectangle_info.height > 3))
    {
      /*
        Display info and draw drawing rectangle.
      */
      (void) FormatLocaleString(text,MagickPathExtent,
        " %.17gx%.17g%+.20g%+.20g",(double) rectangle_info.width,
        (double) rectangle_info.height,(double) rectangle_info.x,
        (double) rectangle_info.y);
      XInfoWidget(display,windows,text);
      XHighlightEllipse(display,windows->image.id,
        windows->image.highlight_context,&rectangle_info);
    }
  else
    if (windows->info.mapped != MagickFalse)
      (void) XWithdrawWindow(display,windows->info.id,
        windows->info.screen);
}

static void XPreviewRectangleErase(const DrawEditContext *context,
  const DrawPreviewInput *preview)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const RectangleInfo
    rectangle_info = preview->rectangle_info;

  if ((rectangle_info.width > 3) && (rectangle_info.height > 3))
    XHighlightRectangle(display,windows->image.id,
      windows->image.highlight_context,&rectangle_info);
}

static void XPreviewEllipseErase(const DrawEditContext *context,
  const DrawPreviewInput *preview)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const RectangleInfo
    rectangle_info = preview->rectangle_info;

  if ((rectangle_info.width > 3) && (rectangle_info.height > 3))
    XHighlightEllipse(display,windows->image.id,
      windows->image.highlight_context,&rectangle_info);
}

static void XDrawCommandAction(const DrawEditContext *context,const int id,
  const DrawPointerTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const image = context->image;

  const Cursor
    cursor = context->cursor;

  const ModeType
    *const DrawCommands = context->DrawCommands;

  size_t
    *const state = targets->state;

  switch (DrawCommands[id])
  {
    case DrawUndoCommand:
    {
      (void) XMagickCommand(display,resource_info,windows,UndoCommand,
        image,exception);
      break;
    }
    case DrawHelpCommand:
    {
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Rotation",ImageDrawHelp);
      }
      (void) XCheckDefineCursor(display,windows->image.id,cursor);
      break;
    }
    case DrawDismissCommand:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    default:
      break;
  }
}

static void XDrawCommand(const DrawEditContext *context,
  const DrawPointerTargets *pointer_targets,
  const DrawStyleTargets *style_targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const DrawMenu = context->DrawMenu;

  XEvent
    *const event = pointer_targets->event;

  ElementType
    *const element = pointer_targets->element;

  int
    id;

  /*
    Select a command from the Command widget.
  */
  id=XCommandWidget(display,windows,DrawMenu,event);
  if (id < 0)
    return;
  switch (context->DrawCommands[id])
  {
    case DrawElementCommand:
    {
      const char
        *const Elements[] =
        {
          "point",
          "line",
          "rectangle",
          "fill rectangle",
          "circle",
          "fill circle",
          "ellipse",
          "fill ellipse",
          "polygon",
          "fill polygon",
          (char *) NULL,
        };

      /*
        Select a command from the pop-up menu.
      */
      {
        const XMenuRequest
          menu_request = { DrawMenu[id], Elements, context->command };

        (*element)=(ElementType) (XMenuWidget(display,windows,&menu_request)+1);
      }
      break;
    }
    case DrawColorCommand:
    {
      XSelectDrawColor(context,id,style_targets);
      break;
    }
    case DrawStippleCommand:
    {
      XSelectDrawStipple(context,id,style_targets);
      break;
    }
    case DrawWidthCommand:
    {
      XSelectDrawWidth(context,id,style_targets);
      break;
    }
    default:
    {
      XDrawCommandAction(context,id,pointer_targets);
      break;
    }
  }
  (void) XCheckDefineCursor(display,windows->image.id,context->cursor);
}

static void XDrawShapeEvent(const DrawEditContext *context,
  const DrawShapeInput *shape,const XEvent event,
  const DrawShapeTargets *targets)
{
  switch (event.type)
  {
    case ButtonPress:
      break;
    case ButtonRelease:
    {
      XDrawButtonRelease(context,shape,event,targets);
      break;
    }
    case Expose:
      break;
    case MotionNotify:
    {
      XDrawMotion(context,shape,event,targets);
      break;
    }
    default:
      break;
  }
}

static void XClampLineEnd(const XWindows *windows,XSegment *line_info)
{
  if ((*line_info).x2 < 0)
    (*line_info).x2=0;
  else
    if ((*line_info).x2 > (int) windows->image.width)
      (*line_info).x2=(short) windows->image.width;
  if ((*line_info).y2 < 0)
    (*line_info).y2=0;
  else
    if ((*line_info).y2 > (int) windows->image.height)
      (*line_info).y2=(short) windows->image.height;
}

static void XSizeDrawRegion(const DrawEditContext *context,
  XDrawInfo *draw_info,const unsigned int line_width,
  const RectangleInfo *rectangle_info)
{
  Image
    **const image = context->image;

  XWindows
    *const windows = context->windows;

  int
    x,
    y;

  unsigned int
    height,
    width;

  width=(unsigned int) (*image)->columns;
  height=(unsigned int) (*image)->rows;
  x=0;
  y=0;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,&height);
  (*draw_info).x+=windows->image.x-((int) line_width/2);
  if ((*draw_info).x < 0)
    (*draw_info).x=0;
  (*draw_info).x=(int) width*(*draw_info).x/windows->image.ximage->width;
  (*draw_info).y+=windows->image.y-((int) line_width/2);
  if ((*draw_info).y < 0)
    (*draw_info).y=0;
  (*draw_info).y=(int) height*(*draw_info).y/windows->image.ximage->height;
  (*draw_info).width=(unsigned int) (*rectangle_info).width+(line_width << 1);
  if ((*draw_info).width > (unsigned int) (*image)->columns)
    (*draw_info).width=(unsigned int) (*image)->columns;
  (*draw_info).height=(unsigned int) (*rectangle_info).height+(line_width << 1);
  if ((*draw_info).height > (unsigned int) (*image)->rows)
    (*draw_info).height=(unsigned int) (*image)->rows;
  (void) FormatLocaleString((*draw_info).geometry,MagickPathExtent,"%ux%u%+d%+d",
    width*(*draw_info).width/(unsigned int) windows->image.ximage->width,
    height*(*draw_info).height/(unsigned int) windows->image.ximage->height,
    (*draw_info).x+x,(*draw_info).y+y);
}

static void XFlipDrawLineVertical(XDrawInfo *draw_info)
{
  if ((*draw_info).line_info.y2 < 0)
    {
      (*draw_info).line_info.y2=(-(*draw_info).line_info.y2);
      Swap((*draw_info).line_info.y1,(*draw_info).line_info.y2);
    }
}

static void XSetDrawLineInfo(XDrawInfo *draw_info,const XSegment *line_info,
  const unsigned int line_width)
{
  (*draw_info).line_info=(*line_info);
  if ((*line_info).x1 > (int) (line_width/2))
    (*draw_info).line_info.x1=(short) line_width/2;
  if ((*line_info).y1 > (int) (line_width/2))
    (*draw_info).line_info.y1=(short) line_width/2;
  (*draw_info).line_info.x2=(short) ((*line_info).x2-(*line_info).x1+
    ((int) line_width/2));
  (*draw_info).line_info.y2=(short) ((*line_info).y2-(*line_info).y1+
    ((int) line_width/2));
  if (((*draw_info).line_info.x2 < 0) && ((*draw_info).line_info.y2 < 0))
    {
      (*draw_info).line_info.x2=(-(*draw_info).line_info.x2);
      (*draw_info).line_info.y2=(-(*draw_info).line_info.y2);
    }
  if ((*draw_info).line_info.x2 < 0)
    {
      (*draw_info).line_info.x2=(-(*draw_info).line_info.x2);
      Swap((*draw_info).line_info.x1,(*draw_info).line_info.x2);
    }
  XFlipDrawLineVertical(draw_info);
}

static void XDrawStartEvent(const DrawEditContext *context,
  const DrawPointerTargets *pointer_targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  int
    *const x = pointer_targets->x;

  int
    *const y = pointer_targets->y;

  XEvent
    *const event = pointer_targets->event;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XDrawButtonPress(context,(*event),pointer_targets);
      break;
    }
    case ButtonRelease:
      break;
    case Expose:
      break;
    case KeyPress:
    {
      XDrawKeyPress(context,pointer_targets);
      break;
    }
    case MotionNotify:
    {
      /*
        Map and unmap Info widget as text cursor crosses its boundaries.
      */
      (*x)=(*event).xmotion.x;
      (*y)=(*event).xmotion.y;
      XTrackInfoWidget(display,windows,(*x),(*y));
      break;
    }
  }
}

static void XWaitForDrawStart(const DrawEditContext *context,
  const DrawPointerTargets *pointer_targets,
  const DrawStyleTargets *style_targets)
{
  const Cursor
    cursor = context->cursor;

  Display
    *const display = context->display;

  ExceptionInfo
    *const exception = context->exception;

  XWindows
    *const windows = context->windows;

  int
    *const x = pointer_targets->x;

  int
    *const y = pointer_targets->y;

  size_t
    *const state = pointer_targets->state;

  XEvent
    *const event = pointer_targets->event;

  char
    text[MagickPathExtent];

  XQueryPosition(display,windows->image.id,x,y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  (*state)=DefaultState;
  do
  {
    if (windows->info.mapped != MagickFalse)
      {
        /*
          Display pointer position.
        */
        (void) FormatLocaleString(text,MagickPathExtent," %+d%+d ",
          (*x)+windows->image.x,(*y)+windows->image.y);
        XInfoWidget(display,windows,text);
      }
    /*
      Wait for next event.
    */
    XScreenEvent(display,windows,event,exception);
    if ((*event).xany.window == windows->command.id)
      {
        XDrawCommand(context,pointer_targets,style_targets);
        continue;
      }
    XDrawStartEvent(context,pointer_targets);
  } while (((*state) & ExitState) == 0);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
}

static void XPreviewPointLineBox(const DrawEditContext *context,
  const DrawPreviewInput *preview,const ElementType element,double *degrees)
{
  switch (element)
  {
    case PointElement:
    default:
    {
      XPreviewPoint(context,preview);
      break;
    }
    case LineElement:
    {
      XPreviewLine(context,preview,degrees);
      break;
    }
    case RectangleElement:
    case FillRectangleElement:
    {
      XPreviewRectangle(context,preview);
      break;
    }
  }
}

static void XPreviewElement(const DrawEditContext *context,
  const DrawPreviewInput *preview,const ElementType element,double *degrees)
{
  switch (element)
  {
    default:
    {
      XPreviewPointLineBox(context,preview,element,degrees);
      break;
    }
    case CircleElement:
    case FillCircleElement:
    case EllipseElement:
    case FillEllipseElement:
    {
      XPreviewEllipse(context,preview);
      break;
    }
    case PolygonElement:
    case FillPolygonElement:
    {
      XPreviewPolyline(context,preview);
      XPreviewLine(context,preview,degrees);
      break;
    }
  }
}

static void XErasePointLineBox(const DrawEditContext *context,
  const DrawPreviewInput *preview,const ElementType element)
{
  switch (element)
  {
    case PointElement:
    default:
    {
      XPreviewPolyline(context,preview);
      break;
    }
    case LineElement:
    {
      XPreviewLongLine(context,preview);
      break;
    }
    case RectangleElement:
    case FillRectangleElement:
    {
      XPreviewRectangleErase(context,preview);
      break;
    }
  }
}

#endif
