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

  MagickCore private methods of display-image.c: the colour-edit command and
  the composite pointer handlers, from XColorFloodfill to XCompositeRelease;
  included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_ANNOTATE_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_ANNOTATE_PRIVATE_H

typedef struct _EditMenuInput
{
  Display
    *display;

  XWindows
    *windows;

  const char
    *const *MatteEditMenu;

  int
    id;

  char
    *command;
} EditMenuInput;

typedef struct _PaintMethodTargets
{
  PaintMethod
    *method;
} PaintMethodTargets;

typedef struct _ColorEditPixelInput
{
  Image
    **image;

  CacheView
    *image_view;

  int
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;

  XResourceInfo
    *resource_info;

  PaintMethod
    method;

  XColor
    color,
    border_color;

  unsigned int
    pen_id;
} ColorEditPixelInput;

static void XColorFloodfill(const ColorEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  const int
    x_offset = pixel_input->x_offset,
    y_offset = pixel_input->y_offset;

  ExceptionInfo
    *const exception = pixel_input->exception;

  XResourceInfo
    *const resource_info = pixel_input->resource_info;

  const PaintMethod
    method = pixel_input->method;

  const XColor
    border_color = pixel_input->border_color;

  const unsigned int
    pen_id = pixel_input->pen_id;

  DrawInfo
    *draw_info;

  PixelInfo
    target;

  /*
    Update color information using floodfill algorithm.
  */
  (void) GetOneVirtualPixelInfo(*image,
    GetPixelCacheVirtualMethod(*image),(ssize_t) x_offset,(ssize_t)
    y_offset,&target,exception);
  if (method == FillToBorderMethod)
    {
      target.red=(double)
        ScaleShortToQuantum(border_color.red);
      target.green=(double)
        ScaleShortToQuantum(border_color.green);
      target.blue=(double)
        ScaleShortToQuantum(border_color.blue);
    }
  draw_info=CloneDrawInfo(resource_info->image_info,
    (DrawInfo *) NULL);
  (void) QueryColorCompliance(resource_info->pen_colors[pen_id],
    AllCompliance,&draw_info->fill,exception);
  (void) FloodfillPaintImage(*image,draw_info,&target,
    (ssize_t)x_offset,(ssize_t)y_offset,
    (MagickBooleanType) (method != FloodfillMethod),exception);
  draw_info=DestroyDrawInfo(draw_info);
}

static void XColorReset(const ColorEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  CacheView
    *const image_view = pixel_input->image_view;

  ExceptionInfo
    *const exception = pixel_input->exception;

  const XColor
    color = pixel_input->color;

  int
    x,
    y;

  Quantum
    *q;

  for (y=0; y < (int) (*image)->rows; y++)
  {
    q=QueueCacheViewAuthenticPixels(image_view,0,(ssize_t) y,
      (*image)->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (int) (*image)->columns; x++)
    {
      SetPixelRed(*image,ScaleShortToQuantum(color.red),q);
      SetPixelGreen(*image,ScaleShortToQuantum(color.green),q);
      SetPixelBlue(*image,ScaleShortToQuantum(color.blue),q);
      q+=(ptrdiff_t) GetPixelChannels(*image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
  }
}

static MagickBooleanType XColorDirectPixels(
  const ColorEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  CacheView
    *const image_view = pixel_input->image_view;

  const int
    x_offset = pixel_input->x_offset,
    y_offset = pixel_input->y_offset;

  ExceptionInfo
    *const exception = pixel_input->exception;

  const PaintMethod
    method = pixel_input->method;

  const XColor
    color = pixel_input->color;

  Quantum
    *q;

  switch (method)
  {
    case PointMethod:
    default:
    {
      /*
        Update color information using point algorithm.
      */
      if (SetImageStorageClass(*image,DirectClass,exception) == MagickFalse)
        return(MagickFalse);
      q=GetCacheViewAuthenticPixels(image_view,(ssize_t)x_offset,
        (ssize_t) y_offset,1,1,exception);
      if (q == (Quantum *) NULL)
        break;
      SetPixelRed(*image,ScaleShortToQuantum(color.red),q);
      SetPixelGreen(*image,ScaleShortToQuantum(color.green),q);
      SetPixelBlue(*image,ScaleShortToQuantum(color.blue),q);
      (void) SyncCacheViewAuthenticPixels(image_view,exception);
      break;
    }
    case ResetMethod:
    {
      /*
        Update color information using reset algorithm.
      */
      if (SetImageStorageClass(*image,DirectClass,exception) == MagickFalse)
        return(MagickFalse);
      XColorReset(pixel_input);
      break;
    }
  }
  return(MagickTrue);
}

static MagickBooleanType XColorEditPixels(
  const ColorEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  CacheView
    *const image_view = pixel_input->image_view;

  const int
    x_offset = pixel_input->x_offset,
    y_offset = pixel_input->y_offset;

  ExceptionInfo
    *const exception = pixel_input->exception;

  const PaintMethod
    method = pixel_input->method;

  const XColor
    color = pixel_input->color;

  switch (method)
  {
    default:
    {
      if (XColorDirectPixels(pixel_input) == MagickFalse)
        return(MagickFalse);
      break;
    }
    case ReplaceMethod:
    {
      PixelInfo
        target;

      XReplaceColorInput
        replace;

      /*
        Update color information using replace algorithm.
      */
      (void) GetOneCacheViewVirtualPixelInfo(image_view,(ssize_t)
        x_offset,(ssize_t) y_offset,&target,exception);
      replace.image=image;
      replace.image_view=image_view;
      replace.target=target;
      replace.color=color;
      replace.exception=exception;
      XReplaceImageColor(&replace);
      break;
    }
    case FloodfillMethod:
    case FillToBorderMethod:
    {
      XColorFloodfill(pixel_input);
      break;
    }
  }
  return(MagickTrue);
}

typedef struct _ColorBorderTargets
{
  XColor
    *border_color;
} ColorBorderTargets;

static void XSelectColorBorder(const EditMenuInput *menu,
  XResourceInfo *resource_info,const ColorBorderTargets *targets)
{
  Display
    *const display = menu->display;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const ColorEditMenu = menu->MatteEditMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  XColor
    *const border_color = targets->border_color;

  const char
    *ColorMenu[MaxNumberPens];

  int
    pen_number;

  ssize_t
    i;

  /*
    Initialize menu selections.
  */
  for (i=0; i < (int) (MaxNumberPens-2); i++)
    ColorMenu[i]=resource_info->pen_colors[i];
  ColorMenu[MaxNumberPens-2]="Browser...";
  ColorMenu[MaxNumberPens-1]=(const char *) NULL;
  /*
    Select a pen color from the pop-up menu.
  */
  {
    const XMenuRequest
      menu_request = { ColorEditMenu[id], (const char **) ColorMenu, command };

    pen_number=XMenuWidget(display,windows,&menu_request);
  }
  if (pen_number < 0)
    return;
  if (pen_number == (MaxNumberPens-2))
    {
      static char
        color_name[MagickPathExtent] = "gray";

      /*
        Select a pen color from a dialog.
      */
      resource_info->pen_colors[pen_number]=color_name;
      XColorBrowserWidget(display,windows,"Select",color_name);
      if (*color_name == '\0')
        return;
    }
  /*
    Set border color.
  */
  (void) XParseColor(display,windows->map_info->colormap,
    resource_info->pen_colors[pen_number],border_color);
}

typedef struct _PenColorTargets
{
  XColor
    *color;

  unsigned int
    *pen_id;
} PenColorTargets;

static void XSelectPenColor(const EditMenuInput *menu,
  XResourceInfo *resource_info,const PenColorTargets *targets)
{
  Display
    *const display = menu->display;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const ColorEditMenu = menu->MatteEditMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  XColor
    *const color = targets->color;

  unsigned int
    *const pen_id = targets->pen_id;

  const char
    *ColorMenu[MaxNumberPens];

  int
    pen_number;

  ssize_t
    i;

  /*
    Initialize menu selections.
  */
  for (i=0; i < (int) (MaxNumberPens-2); i++)
    ColorMenu[i]=resource_info->pen_colors[i];
  ColorMenu[MaxNumberPens-2]="Browser...";
  ColorMenu[MaxNumberPens-1]=(const char *) NULL;
  /*
    Select a pen color from the pop-up menu.
  */
  {
    const XMenuRequest
      menu_request = { ColorEditMenu[id], (const char **) ColorMenu, command };

    pen_number=XMenuWidget(display,windows,&menu_request);
  }
  if (pen_number < 0)
    return;
  if (pen_number == (MaxNumberPens-2))
    {
      static char
        color_name[MagickPathExtent] = "gray";

      /*
        Select a pen color from a dialog.
      */
      resource_info->pen_colors[pen_number]=color_name;
      XColorBrowserWidget(display,windows,"Select",color_name);
      if (*color_name == '\0')
        return;
    }
  /*
    Set pen color.
  */
  (void) XParseColor(display,windows->map_info->colormap,
    resource_info->pen_colors[pen_number],color);
  {
    const XBestPixelTarget
      pixel_target = { display, windows->map_info->colormap };

    XBestPixel(&pixel_target,(XColor *) NULL,(unsigned int) MaxColors,color);
  }
  windows->pixel_info->pen_colors[pen_number]=(*color);
  (*pen_id)=(unsigned int) pen_number;
}

static void
  XSelectPaintMethod(const EditMenuInput *,const PaintMethodTargets *);

static void
  XSelectFuzzFactor(const EditMenuInput *,const char *const *,char *,
    Image **);

static void
  XDiscardMagnifyKeys(Display *,XWindows *,XEvent *);

static const char
  *const ColorEditMenu[] =
  {
    "Method",
    "Pixel Color",
    "Border Color",
    "Fuzz",
    "Undo",
    "Help",
    "Dismiss",
    (char *) NULL
  };

static const ModeType
  ColorEditCommands[] =
  {
    ColorEditMethodCommand,
    ColorEditColorCommand,
    ColorEditBorderCommand,
    ColorEditFuzzCommand,
    ColorEditUndoCommand,
    ColorEditHelpCommand,
    ColorEditDismissCommand
  };

typedef struct _ColorEditContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    **image;

  ExceptionInfo
    *exception;

  const ModeType
    *ColorEditCommands;

  const char
    *const *ColorEditMenu;

  char
    *command;

  Cursor
    cursor;

  PaintMethodTargets
    method_targets;

  PenColorTargets
    pen_targets;

  ColorBorderTargets
    border_targets;

  char
    *text;
} ColorEditContext;

static void XColorEditCommandAction(const ColorEditContext *context,
  const int id,size_t *state)
{
  switch (context->ColorEditCommands[id])
  {
    case ColorEditUndoCommand:
    {
      {
        const XImageCacheContext
          command_context = { context->display, context->resource_info,
            context->windows, context->image, context->exception };

        (void) XMagickCommand(&command_context,UndoCommand);
      }
      break;
    }
    case ColorEditHelpCommand:
    default:
    {
      {
        const XHelpView view = { context->display, context->resource_info,
          context->windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Annotation",
          ImageColorEditHelp);
      }
      break;
    }
    case ColorEditDismissCommand:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
  }
}

static void XColorEditMenuAction(const ColorEditContext *context,
  const int id,size_t *state)
{
  EditMenuInput
    menu_input;

  switch (context->ColorEditCommands[id])
  {
    case ColorEditBorderCommand:
    {
      menu_input.display=context->display;
      menu_input.windows=context->windows;
      menu_input.MatteEditMenu=context->ColorEditMenu;
      menu_input.id=id;
      menu_input.command=context->command;
      XSelectColorBorder(&menu_input,context->resource_info,
        &context->border_targets);
      break;
    }
    case ColorEditFuzzCommand:
    {
      const char
        *const FuzzMenu[] =
        {
          "0%",
          "2%",
          "5%",
          "10%",
          "15%",
          "Dialog...",
          (char *) NULL,
        };

      static char
        fuzz[MagickPathExtent];

      menu_input.display=context->display;
      menu_input.windows=context->windows;
      menu_input.MatteEditMenu=context->ColorEditMenu;
      menu_input.id=id;
      menu_input.command=context->command;
      XSelectFuzzFactor(&menu_input,FuzzMenu,fuzz,context->image);
      break;
    }
    default:
    {
      XColorEditCommandAction(context,id,state);
      break;
    }
  }
}

typedef struct _ColorEditTargets
{
  XEvent
    *event;

  size_t
    *state;

  int
    *x,
    *y;

  PaintMethod
    *method;

  unsigned int
    *pen_id;

  XColor
    *border_color,
    *color;
} ColorEditTargets;

static void XColorEditCommand(const ColorEditContext *context,
  const ColorEditTargets *targets)
{
  EditMenuInput
    menu_input;

  int
    id;

  /*
    Select a command from the Command widget.
  */
  id=XCommandWidget(context->display,context->windows,context->ColorEditMenu,
    targets->event);
  if (id < 0)
    {
      (void) XCheckDefineCursor(context->display,context->windows->image.id,
        context->cursor);
      return;
    }
  switch (context->ColorEditCommands[id])
  {
    case ColorEditMethodCommand:
    {
      menu_input.display=context->display;
      menu_input.windows=context->windows;
      menu_input.MatteEditMenu=context->ColorEditMenu;
      menu_input.id=id;
      menu_input.command=context->command;
      XSelectPaintMethod(&menu_input,&context->method_targets);
      break;
    }
    case ColorEditColorCommand:
    {
      menu_input.display=context->display;
      menu_input.windows=context->windows;
      menu_input.MatteEditMenu=context->ColorEditMenu;
      menu_input.id=id;
      menu_input.command=context->command;
      XSelectPenColor(&menu_input,context->resource_info,&context->pen_targets);
      break;
    }
    default:
    {
      XColorEditMenuAction(context,id,targets->state);
      break;
    }
  }
  (void) XCheckDefineCursor(context->display,context->windows->image.id,
    context->cursor);
}

static void XColorEditPress(const ColorEditContext *context,
  const ColorEditTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  int
    *const x = targets->x,
    *const y = targets->y;

  if ((*event).xbutton.button != Button1)
    return;
  if (((*event).xbutton.window != context->windows->image.id) &&
      ((*event).xbutton.window != context->windows->magnify.id))
    return;
  /*
    exit loop.
  */
  (*x)=(*event).xbutton.x;
  (*y)=(*event).xbutton.y;
  {
    const XImageCacheContext
      command_context = { context->display, context->resource_info,
        context->windows, context->image, context->exception };

    (void) XMagickCommand(&command_context,SaveToUndoBufferCommand);
  }
  (*state)|=UpdateConfigurationState;
}

static void XColorEditRelease(const ColorEditContext *context,
  const ColorEditTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  int
    *const x = targets->x,
    *const y = targets->y;

  if ((*event).xbutton.button != Button1)
    return;
  if (((*event).xbutton.window != context->windows->image.id) &&
      ((*event).xbutton.window != context->windows->magnify.id))
    return;
  /*
    Update colormap information.
  */
  (*x)=(*event).xbutton.x;
  (*y)=(*event).xbutton.y;
  XConfigureImageColormap(context->display,context->resource_info,
    context->windows,*context->image,context->exception);
  (void) XConfigureImage(context->display,context->resource_info,
    context->windows,*context->image,context->exception);
  XInfoWidget(context->display,context->windows,context->text);
  (void) XCheckDefineCursor(context->display,context->windows->image.id,
    context->cursor);
  (*state)&=(size_t) (~UpdateConfigurationState);
}

static void XColorEditKey(const ColorEditContext *context,
  const ColorEditTargets *targets)
{
  char
    command[MagickPathExtent];

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  KeySym
    key_symbol;

  XDiscardMagnifyKeys(context->display,context->windows,event);
  if ((*event).xkey.window != context->windows->image.id)
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
      (*state)|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      {
        const XHelpView view = { context->display, context->resource_info,
          context->windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Annotation",
          ImageColorEditHelp);
      }
      break;
    }
    default:
    {
      (void) XBell(context->display,0);
      break;
    }
  }
}

static void XColorEditEvent(const ColorEditContext *context,
  const ColorEditTargets *targets)
{
  XEvent
    *const event = targets->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XColorEditPress(context,targets);
      break;
    }
    case ButtonRelease:
    {
      XColorEditRelease(context,targets);
      break;
    }
    case Expose:
      break;
    case KeyPress:
    {
      XColorEditKey(context,targets);
      break;
    }
    case MotionNotify:
    {
      /*
        Map and unmap Info widget as cursor crosses its boundaries.
      */
      (*x)=(*event).xmotion.x;
      (*y)=(*event).xmotion.y;
      XTrackInfoWidget(context->display,context->windows,(*x),(*y));
      break;
    }
    default:
      break;
  }
}

static MagickBooleanType XColorEditUpdate(const ColorEditContext *context,
  const ColorEditTargets *targets,int x_offset,int y_offset)
{
  PaintMethod
    *const method = targets->method;

  size_t
    *const state = targets->state;

  unsigned int
    *const pen_id = targets->pen_id;

  unsigned int
    height,
    width;

  XColor
    *const border_color = targets->border_color;

  XColor
    *const color = targets->color;

  if (((*state) & UpdateConfigurationState) != 0)
    {
      CacheView
        *image_view;

      ColorEditPixelInput
        pixel_input;

      int
        x,
        y;

      /*
        Pixel edit is relative to image configuration.
      */
      (void) XClearArea(context->display,context->windows->image.id,x_offset,
        y_offset,1,1,MagickTrue);
      (*color)=context->windows->pixel_info->pen_colors[(*pen_id)];
      XPutPixel(context->windows->image.ximage,x_offset,y_offset,
        (*color).pixel);
      width=(unsigned int) (*context->image)->columns;
      height=(unsigned int) (*context->image)->rows;
      x=0;
      y=0;
      if (context->windows->image.crop_geometry != (char *) NULL)
        (void) XParseGeometry(context->windows->image.crop_geometry,&x,&y,
          &width,&height);
      x_offset=((int) width*(context->windows->image.x+x_offset)/(int)
        context->windows->image.ximage->width+x);
      y_offset=((int) height*(context->windows->image.y+y_offset)/(int)
        context->windows->image.ximage->height+y);
      if ((x_offset < 0) || (y_offset < 0))
        return(MagickTrue);
      if ((x_offset >= (int) (*context->image)->columns) ||
          (y_offset >= (int) (*context->image)->rows))
        return(MagickTrue);
      image_view=AcquireAuthenticCacheView(*context->image,context->exception);
      pixel_input.image=context->image;
      pixel_input.image_view=image_view;
      pixel_input.x_offset=x_offset;
      pixel_input.y_offset=y_offset;
      pixel_input.exception=context->exception;
      pixel_input.resource_info=context->resource_info;
      pixel_input.method=(*method);
      pixel_input.color=(*color);
      pixel_input.border_color=(*border_color);
      pixel_input.pen_id=(*pen_id);
      if (XColorEditPixels(&pixel_input) == MagickFalse)
        return(MagickFalse);
      image_view=DestroyCacheView(image_view);
      (*state)&=(~0x0080U);
    }
  return(MagickTrue);
}

static void XColorEditMagnifyOffset(XEvent *event,
  const ColorEditContext *context,int *x,int *y)
{
  if ((*event).xany.window == context->windows->magnify.id)
    {
      (*x)=context->windows->magnify.x-context->windows->image.x;
      (*y)=context->windows->magnify.y-context->windows->image.y;
    }
}

static MagickBooleanType XColorEditStep(const ColorEditContext *context,
  const ColorEditTargets *targets)
{
  XEvent
    *const event = targets->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  int
    x_offset,
    y_offset;

  if (context->windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(context->text,MagickPathExtent," %+d%+d ",
        (*x)+context->windows->image.x,(*y)+context->windows->image.y);
      XInfoWidget(context->display,context->windows,context->text);
    }
  /*
    Wait for next event.
  */
  XScreenEvent(context->display,context->windows,event,context->exception);
  if ((*event).xany.window == context->windows->command.id)
    {
      XColorEditCommand(context,targets);
      return(MagickTrue);
    }
  XColorEditEvent(context,targets);
  XColorEditMagnifyOffset(event,context,x,y);
  x_offset=(*x);
  y_offset=(*y);
  if (XColorEditUpdate(context,targets,x_offset,y_offset) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static void XMapColorEditCommand(Display *display,XWindows *windows)
{
  (void) CloneString(&windows->command.name,"Color Edit");
  windows->command.data=4;
  (void) XCommandWidget(display,windows,ColorEditMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
}

static void XReleaseColorEditCursor(Display *display,XWindows *windows,
  Cursor cursor)
{
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  XSetCursorState(display,windows,MagickFalse);
  (void) XFreeCursor(display,cursor);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X C o l o r E d i t I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XColorEditImage() allows the user to interactively change the color of one
%  pixel for a DirectColor image or one colormap entry for a PseudoClass image.
%
%  The format of the XColorEditImage method is:
%
%      MagickBooleanType XColorEditImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image **image,
%          ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image; returned from ReadImage.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType XColorEditImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image **image,
  ExceptionInfo *exception)
{
  static PaintMethod
    method = PointMethod;

  static unsigned int
    pen_id = 0;

  static XColor
    border_color = { 0, 0, 0, 0, 0, 0 };

  char
    command[MagickPathExtent],
    text[MagickPathExtent];

  Cursor
    cursor;

  int
    x,
    y;

  size_t
    state;

  XColor
    color;

  XEvent
    event;

  ColorEditContext
    context;

  const PaintMethodTargets
    method_targets = { &method };

  const PenColorTargets
    pen_targets = { &color, &pen_id };

  const ColorBorderTargets
    border_targets = { &border_color };

  const ColorEditTargets
    targets = { &event, &state, &x, &y, &method, &pen_id, &border_color,
      &color };

  /*
    Map Command widget.
  */
  XMapColorEditCommand(display,windows);
  /*
    Make cursor.
  */
  {
    const XCursorColors
      cursor_colors = { resource_info->background_color,
        resource_info->foreground_color };

    cursor=XMakeCursor(display,windows->image.id,windows->map_info->colormap,
      &cursor_colors);
  }
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  /*
    Track pointer until button 1 is pressed.
  */
  XQueryPosition(display,windows->image.id,&x,&y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  state=DefaultState;
  context.display=display;
  context.resource_info=resource_info;
  context.windows=windows;
  context.image=image;
  context.exception=exception;
  context.ColorEditCommands=ColorEditCommands;
  context.ColorEditMenu=ColorEditMenu;
  context.command=command;
  context.cursor=cursor;
  context.method_targets=method_targets;
  context.pen_targets=pen_targets;
  context.border_targets=border_targets;
  context.text=text;
  do
  {
    if (XColorEditStep(&context,&targets) == MagickFalse)
      return(MagickFalse);
  } while ((state & ExitState) == 0);
  XReleaseColorEditCursor(display,windows,cursor);
  return(MagickTrue);
}
typedef struct _ComposeMenuInfo
{
  Display
    *display;

  XWindows
    *windows;

  int
    id;
} ComposeMenuInfo;

static CompositeOperator
  XSelectPasteOperator(const ComposeMenuInfo *,const char *const *,
    CompositeOperator);

typedef struct _CompositeTargets
{
  double
    *blend;

  CompositeOperator
    *compose;
} CompositeTargets;

static void XSelectDissolveBlend(Display *display,XWindows *windows,
  const CompositeTargets *targets)
{
  double
    *const blend = targets->blend;

  CompositeOperator
    *const compose = targets->compose;

  static char
    factor[MagickPathExtent] = "20.0";

  /*
    Dissolve the two images a given percent.
  */
  (void) XSetFunction(display,windows->image.highlight_context,
    GXcopy);
  {
    const XDialogRequest
      dialog_request = { "Dissolve", "Enter the blend factor (0.0 - 99.9%):",
        factor };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  (void) XSetFunction(display,windows->image.highlight_context,
    GXinvert);
  if (*factor == '\0')
    return;
  (*blend)=StringToDouble(factor,(char **) NULL);
  (*compose)=DissolveCompositeOp;
}

static void XSelectDisplaceGeometry(Display *display,XWindows *windows,
  char *displacement_geometry,const CompositeTargets *targets)
{
  CompositeOperator
    *const compose = targets->compose;

  /*
    Get horizontal and vertical scale displacement geometry.
  */
  (void) XSetFunction(display,windows->image.highlight_context,
    GXcopy);
  {
    const XDialogRequest
      dialog_request = { "Displace", "Enter the horizontal and vertical scale:",
        displacement_geometry };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  (void) XSetFunction(display,windows->image.highlight_context,
    GXinvert);
  if (*displacement_geometry == '\0')
    return;
  (*compose)=DisplaceCompositeOp;
}

static void XSetBlendAlpha(Image *image,CacheView *image_view,
  const Quantum opacity,ExceptionInfo *exception)
{
  int
    x,
    y;

  Quantum
    *q;

  for (y=0; y < (int) image->rows; y++)
  {
    q=GetCacheViewAuthenticPixels(image_view,0,(ssize_t) y,image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (int) image->columns; x++)
    {
      SetPixelAlpha(image,opacity,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
  }
}

static MagickBooleanType XCreateBlendMattes(Image *image,Image *composite_image,
  const double blend,ExceptionInfo *exception)
{
  if (blend != 0.0)
    {
      CacheView
        *image_view;

      Quantum
        opacity;

      /*
        Create mattes for blending.
      */
      (void) SetImageAlphaChannel(composite_image,OpaqueAlphaChannel,exception);
      opacity=(Quantum) (ScaleQuantumToChar(QuantumRange)-
        ((ssize_t) ScaleQuantumToChar(QuantumRange)*blend)/100);
      if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
        return(MagickFalse);
      image->alpha_trait=BlendPixelTrait;
      image_view=AcquireAuthenticCacheView(image,exception);
      XSetBlendAlpha(image,image_view,opacity,exception);
      image_view=DestroyCacheView(image_view);
    }
  return(MagickTrue);
}

typedef struct _CompositeCommandInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const ModeType
    *CompositeCommands;

  const char
    *const *CompositeMenu;

  char
    *displacement_geometry;

  CompositeTargets
    targets;

  ExceptionInfo
    *exception;
} CompositeCommandInput;

static void XCompositeControl(const CompositeCommandInput *input,const int id,
  size_t *state)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  const ModeType
    *const CompositeCommands = input->CompositeCommands;

  switch (CompositeCommands[id])
  {
    case CompositeHelpCommand:
    {
      (void) XSetFunction(display,windows->image.highlight_context,
        GXcopy);
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Composite",ImageCompositeHelp);
      }
      (void) XSetFunction(display,windows->image.highlight_context,
        GXinvert);
      break;
    }
    case CompositeDismissCommand:
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

static MagickBooleanType XCompositeCommandBlock(
  const CompositeCommandInput *input,XEvent *event,CompositeOperator *compose,
  size_t *state)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  const char
    *const *const CompositeMenu = input->CompositeMenu;

  const ModeType
    *const CompositeCommands = input->CompositeCommands;

  const CompositeTargets
    targets = input->targets;

  int
    id;

  if ((*event).xany.window == windows->command.id)
    {
      /*
        Select a command from the Command widget.
      */
      id=XCommandWidget(display,windows,CompositeMenu,event);
      if (id < 0)
        return(MagickTrue);
      switch (CompositeCommands[id])
      {
        case CompositeOperatorsCommand:
        {
          ComposeMenuInfo
            menu_info;

          menu_info.display=display;
          menu_info.windows=windows;
          menu_info.id=id;
          (*compose)=XSelectPasteOperator(&menu_info,CompositeMenu,(*compose));
          break;
        }
        case CompositeDissolveCommand:
        {
          XSelectDissolveBlend(display,windows,&targets);
          break;
        }
        case CompositeDisplaceCommand:
        {
          XSelectDisplaceGeometry(display,windows,input->displacement_geometry,
            &targets);
          break;
        }
        default:
        {
          XCompositeControl(input,id,state);
          break;
        }
      }
      return(MagickTrue);
    }
  return(MagickFalse);
}

typedef struct _CompositeEventTargets
{
  Image
    **composite_image;

  size_t
    *state;

  RectangleInfo
    *composite_info;

  CompositeOperator
    *compose;
} CompositeEventTargets;

static void XCompositeKeyPress(const CompositeCommandInput *input,
  XEvent *event,const CompositeEventTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    **const composite_image = targets->composite_image;

  size_t
    *const state = targets->state;

  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  int
    length;

  if ((*event).xkey.window != windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  length=XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  *(command+length)='\0';
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Key press: 0x%lx (%s)",(unsigned long) key_symbol,command);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      /*
        Prematurely exit.
      */
      (*composite_image)=DestroyImage((*composite_image));
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      (void) XSetFunction(display,windows->image.highlight_context,
        GXcopy);
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Composite",ImageCompositeHelp);
      }
      (void) XSetFunction(display,windows->image.highlight_context,
        GXinvert);
      break;
    }
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
}

static void XCompositePress(const CompositeCommandInput *input,
  const XEvent *event,const Cursor cursor,const CompositeEventTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    **const composite_image = targets->composite_image;

  RectangleInfo
    *const composite_info = targets->composite_info;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Button Press: 0x%lx %u +%d+%d",(*event).xbutton.window,
      (*event).xbutton.button,(*event).xbutton.x,(*event).xbutton.y);
  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != windows->image.id)
    return;
  /*
    Change cursor.
  */
  (*composite_info).width=(*composite_image)->columns;
  (*composite_info).height=(*composite_image)->rows;
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  (*composite_info).x=(ssize_t) windows->image.x+(*event).xbutton.x;
  (*composite_info).y=(ssize_t) windows->image.y+(*event).xbutton.y;
}

static void XCompositeRelease(const CompositeCommandInput *input,
  const XEvent *event,const CompositeEventTargets *targets)
{
  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  RectangleInfo
    *const composite_info = targets->composite_info;

  size_t
    *const state = targets->state;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Button Release: 0x%lx %u +%d+%d",(*event).xbutton.window,
      (*event).xbutton.button,(*event).xbutton.x,(*event).xbutton.y);
  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != windows->image.id)
    return;
  if (((*composite_info).width != 0) && ((*composite_info).height != 0))
    {
      /*
        User has selected the location of the composite image.
      */
      (*composite_info).x=(ssize_t) windows->image.x+(*event).xbutton.x;
      (*composite_info).y=(ssize_t) windows->image.y+(*event).xbutton.y;
      (*state)|=ExitState;
    }
}

#endif
