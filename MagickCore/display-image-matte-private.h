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

  MagickCore private methods of display-image.c: the magnify window, matte-
  edit, pan, paste, print, region-of-interest and rotate commands, from
  XMoveMagnifyPosition to XRotateImage; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_MATTE_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_MATTE_PRIVATE_H

static void XROIWaitCorner(const ROIContext *context,const ROITargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  RectangleInfo
    *const roi_info = targets->roi_info;

  RectangleInfo
    *const crop_info = targets->crop_info;

  DisplayCommand
    *const display_command = targets->display_command;

  RectangleInfo
    highlight_info;

  do
  {
    ROIRegionInput
      region_input;

    if (XROIWaitHead(context,targets,&highlight_info) == MagickFalse)
      break;
    region_input.display=context->display;
    region_input.resource_info=context->resource_info;
    region_input.windows=context->windows;
    region_input.image=context->image;
    region_input.exception=context->exception;
    region_input.roi_info=(*roi_info);
    region_input.display_command=(*display_command);
    region_input.text=context->text;
    if (XROIUpdateRegion(&region_input,state,crop_info) != MagickFalse)
      continue;
    XHighlightRectangle(context->display,context->windows->image.id,
      context->windows->image.highlight_context,&highlight_info);
    XScreenEvent(context->display,context->windows,event,context->exception);
    if ((*event).xany.window == context->windows->command.id)
      {
        XROIWaitCommand(context,targets,highlight_info);
        continue;
      }
    XHighlightRectangle(context->display,context->windows->image.id,
      context->windows->image.highlight_context,&highlight_info);
    XROIWaitEvent(context,targets,highlight_info);
    if (((*state) & UpdateConfigurationState) != 0)
      {
        (void) XPutBackEvent(context->display,event);
        (void) XCheckDefineCursor(context->display,context->windows->image.id,
          context->cursor);
        break;
      }
  } while (((*state) & ExitState) == 0);
}

static void XROISelectRegion(const ROIContext *context,const ROITargets *targets)
{
  size_t
    *const state = targets->state;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const roi_info = targets->roi_info;

  RectangleInfo
    *const crop_info = targets->crop_info;

  DisplayCommand
    *const display_command = targets->display_command;

  do
  {
    /*
      Size rectangle as pointer moves until the mouse button is released.
    */
    (*x)=(int) (*roi_info).x;
    (*y)=(int) (*roi_info).y;
    (*roi_info).width=0;
    (*roi_info).height=0;
    (*state)=DefaultState;
    XROISizeRegion(context,targets);
    /*
      Wait for user to grab a corner of the rectangle or press return.
    */
    (*state)=DefaultState;
    (*display_command)=NullCommand;
    (*crop_info).x=0;
    (*crop_info).y=0;
    (void) XMapWindow(context->display,context->windows->info.id);
    XROIWaitCorner(context,targets);
  } while (((*state) & ExitState) == 0);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X R O I I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XROIImage() applies an image processing technique to a region of interest.
%
%  The format of the XROIImage method is:
%
%      MagickBooleanType XROIImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image **image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
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
static MagickBooleanType XROIImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image **image,
  ExceptionInfo *exception)
{
  char
    text[MagickPathExtent];

  DisplayCommand
    display_command;

  Cursor
    cursor;

  int
    x,
    y;

  RectangleInfo
    crop_info,
    roi_info;

  size_t
    state;

  XEvent
    event;

  ROIContext
    context;

  const ROITargets
    targets = { &x, &y, &roi_info, &state, &event, &display_command,
      &crop_info };

  const ROIKeyTargets
    key_targets = { &roi_info, &display_command, &state };

  /*
    Map Command widget.
  */
  (void) CloneString(&windows->command.name,"ROI");
  windows->command.data=0;
  (void) XCommandWidget(display,windows,ROIMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
  /*
    Track pointer until button 1 is pressed.
  */
  XQueryPosition(display,windows->image.id,&x,&y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  roi_info.x=(ssize_t) windows->image.x+x;
  roi_info.y=(ssize_t) windows->image.y+y;
  roi_info.width=0;
  roi_info.height=0;
  cursor=XCreateFontCursor(display,XC_fleur);
  state=DefaultState;
  context.display=display;
  context.resource_info=resource_info;
  context.windows=windows;
  context.cursor=cursor;
  context.exception=exception;
  context.text=text;
  context.image=image;
  context.key_targets=key_targets;
  XROITrackPointer(&context,&targets);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  if ((state & EscapeState) != 0)
    {
      /*
        User want to exit without region of interest.
      */
      (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
      (void) XFreeCursor(display,cursor);
      return(MagickTrue);
    }
  (void) XSetFunction(display,windows->image.highlight_context,GXinvert);
  XROISelectRegion(&context,&targets);
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
  XSetCursorState(display,windows,MagickFalse);
  if ((state & EscapeState) != 0)
    return(MagickTrue);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X R o t a t e I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XRotateImage() rotates the X image.  If the degrees parameter if zero, the
%  rotation angle is computed from the slope of a line drawn by the user.
%
%  The format of the XRotateImage method is:
%
%      MagickBooleanType XRotateImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,double degrees,
%        Image **image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o degrees: Specifies the number of degrees to rotate the image.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _RotatePenMenu
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const char
    *const *RotateMenu;

  int
    id;

  char
    *command;
} RotatePenMenu;

typedef struct _RotateTargets
{
  unsigned int
    *pen_id;
} RotateTargets;

static void XSelectRotatePenColor(const RotatePenMenu *menu,
  const RotateTargets *targets)
{
  Display
    *const display = menu->display;

  XResourceInfo
    *const resource_info = menu->resource_info;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const RotateMenu = menu->RotateMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  unsigned int
    *const pen_id = targets->pen_id;

  const char
    *ColorMenu[MaxNumberPens];

  int
    i,
    pen_number;

  XColor
    color;

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
  pen_number=XMenuWidget(display,windows,RotateMenu[id],
    (const char **) ColorMenu,command);
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
    resource_info->pen_colors[pen_number],&color);
  XBestPixel(display,windows->map_info->colormap,(XColor *) NULL,
    (unsigned int) MaxColors,&color);
  windows->pixel_info->pen_colors[pen_number]=color;
  (*pen_id)=(unsigned int) pen_number;
}

typedef struct _RotateCommandInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  XEvent
    event;

  const char
    *const *RotateMenu;

  const ModeType
    *RotateCommands,
    *DirectionCommands;

  char
    *command;

  RotateTargets
    targets;
} RotateCommandInput;

static void XRotateCommand(const RotateCommandInput *input,
  ModeType *direction,size_t *state)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  XEvent
    event = input->event;

  const char
    *const *const RotateMenu = input->RotateMenu;

  int
    id;

  /*
    Select a command from the Command widget.
  */
  id=XCommandWidget(display,windows,RotateMenu,&event);
  if (id < 0)
    return;
  (void) XSetFunction(display,windows->image.highlight_context,
    GXcopy);
  switch (input->RotateCommands[id])
  {
    case RotateColorCommand:
    {
      RotatePenMenu
        menu;

      menu.display=display;
      menu.resource_info=resource_info;
      menu.windows=windows;
      menu.RotateMenu=RotateMenu;
      menu.id=id;
      menu.command=input->command;
      XSelectRotatePenColor(&menu,&input->targets);
      break;
    }
    case RotateDirectionCommand:
    {
      const char
        *Directions[] =
        {
          "horizontal",
          "vertical",
          (char *) NULL,
        };

      /*
        Select a command from the pop-up menu.
      */
      id=XMenuWidget(display,windows,RotateMenu[id],
        Directions,input->command);
      *direction=XSelectChopDirection(id,input->DirectionCommands,*direction);
      break;
    }
    case RotateHelpCommand:
    {
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Rotation",ImageRotateHelp);
      }
      break;
    }
    case RotateDismissCommand:
    {
      /*
        Prematurely exit.
      */
      *state|=EscapeState;
      *state|=ExitState;
      break;
    }
    default:
      break;
  }
  (void) XSetFunction(display,windows->image.highlight_context,
    GXinvert);
}

typedef struct _RotateLineTargets
{
  XSegment
    *rotate_info;

  size_t
    *state;

  unsigned int
    *distance;

  double
    *degrees;
} RotateLineTargets;

static void XStartRotateLine(Display *display,XWindows *windows,
  const XEvent event,const RotateLineTargets *targets)
{
  XSegment
    *const rotate_info = targets->rotate_info;

  size_t
    *const state = targets->state;

  if (event.xbutton.button != Button1)
    return;
  if (event.xbutton.window != windows->image.id)
    return;
  /*
    exit loop.
  */
  (void) XSetFunction(display,windows->image.highlight_context,
    GXcopy);
  (*rotate_info).x1=event.xbutton.x;
  (*rotate_info).y1=event.xbutton.y;
  (*state)|=ExitState;
}

typedef struct _RotateKeyInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  XEvent
    event;
} RotateKeyInput;

static void XRotateKeyPress(const RotateKeyInput *key_input,
  const RotateLineTargets *targets)
{
  Display
    *const display = key_input->display;

  XResourceInfo
    *const resource_info = key_input->resource_info;

  XWindows
    *const windows = key_input->windows;

  XEvent
    event = key_input->event;

  size_t
    *const state = targets->state;

  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  if (event.xkey.window != windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &event.xkey,command,(int)
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
      (void) XSetFunction(display,windows->image.highlight_context,
        GXcopy);
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Rotation",ImageRotateHelp);
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

typedef struct _RotateInfoInput
{
  Display
    *display;

  XWindows
    *windows;

  char
    *text;

  ModeType
    direction;

  double
    degrees;

  XSegment
    rotate_info;
} RotateInfoInput;

static void XShowRotateInfo(const RotateInfoInput *show)
{
  Display
    *const display = show->display;

  XWindows
    *const windows = show->windows;

  char
    *const text = show->text;

  const ModeType
    direction = show->direction;

  const double
    degrees = show->degrees;

  const XSegment
    rotate_info = show->rotate_info;

  /*
    Display info and draw rotation line.
  */
  if (windows->info.mapped == MagickFalse)
    (void) XMapWindow(display,windows->info.id);
  (void) FormatLocaleString(text,MagickPathExtent," %g",
    direction == VerticalRotateCommand ? degrees-90.0 : degrees);
  XInfoWidget(display,windows,text);
  XHighlightLine(display,windows->image.id,
    windows->image.highlight_context,&rotate_info);
}

typedef struct _RotateLoopInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const char
    *const *RotateMenu;

  const ModeType
    *RotateCommands,
    *DirectionCommands;

  char
    *command;

  RotateTargets
    targets;

  ExceptionInfo
    *exception;
} RotateLoopInput;

static MagickBooleanType XRotateLoopCommand(
  const RotateLoopInput *loop_input,const XEvent event,ModeType *direction,
  size_t *state)
{
  Display
    *const display = loop_input->display;

  XResourceInfo
    *const resource_info = loop_input->resource_info;

  XWindows
    *const windows = loop_input->windows;

  const char
    *const *const RotateMenu = loop_input->RotateMenu;

  const ModeType
    *const RotateCommands = loop_input->RotateCommands,
    *const DirectionCommands = loop_input->DirectionCommands;

  char
    *const command = loop_input->command;

  const RotateTargets
    targets = loop_input->targets;

  if (event.xany.window == windows->command.id)
    {
      RotateCommandInput
        input;

      input.display=display;
      input.resource_info=resource_info;
      input.windows=windows;
      input.event=event;
      input.RotateMenu=RotateMenu;
      input.RotateCommands=RotateCommands;
      input.DirectionCommands=DirectionCommands;
      input.command=command;
      input.targets=targets;
      XRotateCommand(&input,direction,state);
      return(MagickTrue);
    }
  return(MagickFalse);
}

static void XRotateSelectStep(const RotateLoopInput *loop_input,
  const RotateLineTargets *line_targets,ModeType *direction)
{
  Display
    *const display = loop_input->display;

  XResourceInfo
    *const resource_info = loop_input->resource_info;

  XWindows
    *const windows = loop_input->windows;

  ExceptionInfo
    *const exception = loop_input->exception;

  XSegment
    *const rotate_info = line_targets->rotate_info;

  size_t
    *const state = line_targets->state;

  XEvent
    event;

  XHighlightLine(display,windows->image.id,
    windows->image.highlight_context,rotate_info);
  /*
    Wait for next event.
  */
  XScreenEvent(display,windows,&event,exception);
  XHighlightLine(display,windows->image.id,
    windows->image.highlight_context,rotate_info);
  if (XRotateLoopCommand(loop_input,event,direction,state) !=
      MagickFalse)
    return;
  switch (event.type)
  {
    case ButtonPress:
    {
      XStartRotateLine(display,windows,event,line_targets);
      break;
    }
    case ButtonRelease:
      break;
    case Expose:
      break;
    case KeyPress:
    {
      RotateKeyInput
        key_input;

      key_input.display=display;
      key_input.resource_info=resource_info;
      key_input.windows=windows;
      key_input.event=event;
      XRotateKeyPress(&key_input,line_targets);
      break;
    }
    case MotionNotify:
    {
      (*rotate_info).x1=event.xmotion.x;
      (*rotate_info).y1=event.xmotion.y;
    }
  }
  (*rotate_info).x2=(*rotate_info).x1;
  (*rotate_info).y2=(*rotate_info).y1;
  if ((*direction) == HorizontalRotateCommand)
    (*rotate_info).x2+=32;
  else
    (*rotate_info).y2-=32;
}

static void XRotateDragEvent(const XEvent event,
  const RotateLineTargets *targets)
{
  XSegment
    *const rotate_info = targets->rotate_info;

  size_t
    *const state = targets->state;

  switch (event.type)
  {
    case ButtonPress:
      break;
    case ButtonRelease:
    {
      /*
        User has committed to rotation line.
      */
      (*rotate_info).x2=event.xbutton.x;
      (*rotate_info).y2=event.xbutton.y;
      (*state)|=ExitState;
      break;
    }
    case Expose:
      break;
    case MotionNotify:
    {
      (*rotate_info).x2=event.xmotion.x;
      (*rotate_info).y2=event.xmotion.y;
    }
    default:
      break;
  }
}

static void XClampRotateSegment(const XWindows *windows,
  XSegment *rotate_info)
{
  /*
    Check boundary conditions.
  */
  if ((*rotate_info).x2 < 0)
    (*rotate_info).x2=0;
  else
    if ((*rotate_info).x2 > (int) windows->image.width)
      (*rotate_info).x2=(short) windows->image.width;
  if ((*rotate_info).y2 < 0)
    (*rotate_info).y2=0;
  else
    if ((*rotate_info).y2 > (int) windows->image.height)
      (*rotate_info).y2=(short) windows->image.height;
}

static void XRotateAngle(const XSegment *rotate_info,double *degrees,
  unsigned int *distance)
{
  /*
    Compute rotation angle from the slope of the line.
  */
  (*degrees)=0.0;
  (*distance)=(unsigned int)
    ((((*rotate_info).x2-(*rotate_info).x1+1)*
      ((*rotate_info).x2-(*rotate_info).x1+1))+
     (((*rotate_info).y2-(*rotate_info).y1+1)*
      ((*rotate_info).y2-(*rotate_info).y1+1)));
  if ((*distance) > 9)
    (*degrees)=RadiansToDegrees(-atan2((double) ((*rotate_info).y2-
      (*rotate_info).y1),(double) ((*rotate_info).x2-(*rotate_info).x1)));
}

static void XRotateDragLine(const RotateLoopInput *loop_input,
  const RotateLineTargets *line_targets,const ModeType direction)
{
  Display
    *const display = loop_input->display;

  XWindows
    *const windows = loop_input->windows;

  ExceptionInfo
    *const exception = loop_input->exception;

  XSegment
    *const rotate_info = line_targets->rotate_info;

  size_t
    *const state = line_targets->state;

  unsigned int
    *const distance = line_targets->distance;

  double
    *const degrees = line_targets->degrees;

  char
    text[MagickPathExtent];

  XEvent
    event;

  (*distance)=0;
  (void) XSetFunction(display,windows->image.highlight_context,GXinvert);
  (*state)=DefaultState;
  do
  {
    if ((*distance) > 9)
      {
        RotateInfoInput
          show;

        show.display=display;
        show.windows=windows;
        show.text=text;
        show.direction=direction;
        show.degrees=(*degrees);
        show.rotate_info=(*rotate_info);
        XShowRotateInfo(&show);
      }
    else
      if (windows->info.mapped != MagickFalse)
        (void) XWithdrawWindow(display,windows->info.id,
          windows->info.screen);
    /*
      Wait for next event.
    */
    XScreenEvent(display,windows,&event,exception);
    if ((*distance) > 9)
      XHighlightLine(display,windows->image.id,
        windows->image.highlight_context,rotate_info);
    XRotateDragEvent(event,line_targets);
    XClampRotateSegment(windows,rotate_info);
    XRotateAngle(rotate_info,degrees,distance);
  } while (((*state) & ExitState) == 0);
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
}

typedef struct _RotateAngleTargets
{
  int
    *x,
    *y;

  ModeType
    *direction;

  double
    *degrees;
} RotateAngleTargets;

static MagickBooleanType XSelectRotateAngle(
  const RotateLoopInput *loop_input,const RotateAngleTargets *targets)
{
  Display
    *const display = loop_input->display;

  XWindows
    *const windows = loop_input->windows;

  const char
    *const *const RotateMenu = loop_input->RotateMenu;

  int
    *const x = targets->x,
    *const y = targets->y;

  ModeType
    *const direction = targets->direction;

  double
    *const degrees = targets->degrees;

  if ((*degrees) == 0.0)
    {
      unsigned int
        distance;

      size_t
        state;

      XSegment
        rotate_info;

      const RotateLineTargets
        line_targets = { &rotate_info, &state, &distance, degrees };

      /*
        Map Command widget.
      */
      (void) CloneString(&windows->command.name,"Rotate");
      windows->command.data=2;
      (void) XCommandWidget(display,windows,RotateMenu,(XEvent *) NULL);
      (void) XMapRaised(display,windows->command.id);
      XClientMessage(display,windows->image.id,windows->im_protocols,
        windows->im_update_widget,CurrentTime);
      /*
        Wait for first button press.
      */
      (void) XSetFunction(display,windows->image.highlight_context,GXinvert);
      XQueryPosition(display,windows->image.id,x,y);
      rotate_info.x1=(*x);
      rotate_info.y1=(*y);
      rotate_info.x2=(*x);
      rotate_info.y2=(*y);
      state=DefaultState;
      do
      {
        XRotateSelectStep(loop_input,&line_targets,direction);
      } while ((state & ExitState) == 0);
      (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
      (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
      if ((state & EscapeState) != 0)
        return(MagickFalse);
      /*
        Draw line as pointer moves until the mouse button is released.
      */
      XRotateDragLine(loop_input,&line_targets,(*direction));
      if (distance <= 9)
        return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _RotateCropInput
{
  XWindows
    *windows;

  Image
    **image;

  unsigned int
    rotations;

  int
    x,
    y;
} RotateCropInput;

static void XRotateCropGeometry(const RotateCropInput *crop,
  unsigned int *width,unsigned int *height)
{
  XWindows
    *const windows = crop->windows;

  Image
    **const image = crop->image;

  const unsigned int
    rotations = crop->rotations;

  int
    x = crop->x,
    y = crop->y;

  if (windows->image.crop_geometry != (char *) NULL)
    {
      /*
        Rotate crop geometry.
      */
      (*width)=(unsigned int) (*image)->columns;
      (*height)=(unsigned int) (*image)->rows;
      (void) XParseGeometry(windows->image.crop_geometry,&x,&y,width,height);
      switch (rotations % 4)
      {
        default:
        case 0:
          break;
        case 1:
        {
          /*
            Rotate 90 degrees.
          */
          (void) FormatLocaleString(windows->image.crop_geometry,
            MagickPathExtent,"%ux%u%+d%+d",(*height),(*width),
            (int) (*image)->columns-(int) (*height)-y,x);
          break;
        }
        case 2:
        {
          /*
            Rotate 180 degrees.
          */
          (void) FormatLocaleString(windows->image.crop_geometry,
            MagickPathExtent,"%ux%u%+d%+d",(*width),(*height),
            (int) (*width)-x,(int) (*height)-y);
          break;
        }
        case 3:
        {
          /*
            Rotate 270 degrees.
          */
          (void) FormatLocaleString(windows->image.crop_geometry,
            MagickPathExtent,"%ux%u%+d%+d",(*height),(*width),y,
            (int) (*image)->rows-(int) (*width)-x);
          break;
        }
      }
    }
}

typedef struct _RotateUpdateInput
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

  double
    normalized_degrees;

  unsigned int
    rotations;
} RotateUpdateInput;

static MagickBooleanType XRotateUpdate(const RotateUpdateInput *update,
  unsigned int *width,unsigned int *height)
{
  Display
    *const display = update->display;

  XResourceInfo
    *const resource_info = update->resource_info;

  XWindows
    *const windows = update->windows;

  Image
    **const image = update->image;

  ExceptionInfo
    *const exception = update->exception;

  const double
    normalized_degrees = update->normalized_degrees;

  const unsigned int
    rotations = update->rotations;

  int
    x,
    y;

  if (windows->image.orphan != MagickFalse)
    return(MagickTrue);
  if (normalized_degrees != 0.0)
    {
      /*
        Update image colormap.
      */
      windows->image.window_changes.width=(int) (*image)->columns;
      windows->image.window_changes.height=(int) (*image)->rows;
      if (windows->image.crop_geometry != (char *) NULL)
        {
          /*
            Obtain dimensions of image from crop geometry.
          */
          (void) XParseGeometry(windows->image.crop_geometry,&x,&y,
            width,height);
          windows->image.window_changes.width=(int) (*width);
          windows->image.window_changes.height=(int) (*height);
        }
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
    }
  else
    if (((rotations % 4) == 1) || ((rotations % 4) == 3))
      {
        windows->image.window_changes.width=windows->image.ximage->height;
        windows->image.window_changes.height=windows->image.ximage->width;
      }
  /*
    Update image configuration.
  */
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
  return(MagickTrue);
}

typedef struct _RotateApplyInput
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

  double
    degrees,
    normalized_degrees;

  unsigned int
    pen_id;
} RotateApplyInput;

static MagickBooleanType XRotateApply(const RotateApplyInput *apply)
{
  Display
    *const display = apply->display;

  XResourceInfo
    *const resource_info = apply->resource_info;

  XWindows
    *const windows = apply->windows;

  Image
    **const image = apply->image;

  ExceptionInfo
    *const exception = apply->exception;

  const double
    degrees = apply->degrees,
    normalized_degrees = apply->normalized_degrees;

  const unsigned int
    pen_id = apply->pen_id;

  Image
    *rotate_image;

  if (normalized_degrees != 0.0)
    (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
      exception);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (*image)->background_color.red=(double) ScaleShortToQuantum(
    windows->pixel_info->pen_colors[pen_id].red);
  (*image)->background_color.green=(double) ScaleShortToQuantum(
    windows->pixel_info->pen_colors[pen_id].green);
  (*image)->background_color.blue=(double) ScaleShortToQuantum(
    windows->pixel_info->pen_colors[pen_id].blue);
  rotate_image=RotateImage(*image,degrees,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (rotate_image == (Image *) NULL)
    return(MagickFalse);
  *image=DestroyImage(*image);
  *image=rotate_image;
  return(MagickTrue);
}

static const ModeType
  RotateDirectionTable[] =
  {
    HorizontalRotateCommand,
    VerticalRotateCommand
  },
  RotateCommandTable[] =
  {
    RotateColorCommand,
    RotateDirectionCommand,
    RotateHelpCommand,
    RotateDismissCommand
  };

static MagickBooleanType XRotateImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,double degrees,Image **image,
  ExceptionInfo *exception)
{
  const char
    *const RotateMenu[] =
    {
      "Pixel Color",
      "Direction",
      "Help",
      "Dismiss",
      (char *) NULL
    };

  static ModeType
    direction = HorizontalRotateCommand;

  static unsigned int
    pen_id = 0;

  char
    command[MagickPathExtent];

  int
    x,
    y;

  double
    normalized_degrees;

  unsigned int
    height,
    rotations,
    width;

  const RotateTargets
    targets = { &pen_id };

  const RotateLoopInput
    loop_input = { display, resource_info, windows, RotateMenu,
      RotateCommandTable, RotateDirectionTable, command, targets, exception };

  const RotateAngleTargets
    angle_targets = { &x, &y, &direction, &degrees };

  if (XSelectRotateAngle(&loop_input,&angle_targets) == MagickFalse)
    return(MagickTrue);
  if (direction == VerticalRotateCommand)
    degrees-=90.0;
  if (degrees == 0.0)
    return(MagickTrue);
  /*
    Rotate image.
  */
  normalized_degrees=degrees;
  while (normalized_degrees < -45.0)
    normalized_degrees+=360.0;
  for (rotations=0; normalized_degrees > 45.0; rotations++)
    normalized_degrees-=90.0;
  {
    const RotateApplyInput
      apply = { display, resource_info, windows, image, exception, degrees,
        normalized_degrees, pen_id };

    if (XRotateApply(&apply) == MagickFalse)
      return(MagickFalse);
  }
  {
    const RotateCropInput
      crop = { windows, image, rotations, x, y };

    XRotateCropGeometry(&crop,&width,&height);
  }
  {
    const RotateUpdateInput
      update = { display, resource_info, windows, image, exception,
        normalized_degrees, rotations };

    return(XRotateUpdate(&update,&width,&height));
  }
}

#endif
