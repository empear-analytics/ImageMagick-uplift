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
  edit, open, pan, paste, print and region-of-interest commands, from
  XMoveMagnifyPosition to XROIWaitHead; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_MATTE_ROI_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_MATTE_ROI_PRIVATE_H

static void XMoveMagnifyPosition(XWindows *windows,const unsigned int quantum,
  const KeySym key_symbol)
{
  switch ((int) key_symbol)
  {
    case XK_Left:
    case XK_KP_Left:
    {
      if (windows->magnify.x > 0)
        windows->magnify.x-=(int) quantum;
      break;
    }
    case XK_Up:
    case XK_KP_Up:
    {
      if (windows->magnify.y > 0)
        windows->magnify.y-=(int) quantum;
      break;
    }
    default:
    {
      XMoveMagnifyRightDown(windows,quantum,key_symbol);
      break;
    }
  }
}

static void XApplyMagnifyKey(const KeySym key_symbol,Display *display,
  XWindows *windows,unsigned int quantum)
{
  switch ((int) key_symbol)
  {
    case QuitCommand:
    {
      (void) XWithdrawWindow(display,windows->magnify.id,
        windows->magnify.screen);
      break;
    }
    case XK_Home:
    case XK_KP_Home:
    {
      windows->magnify.x=(int) windows->image.width/2;
      windows->magnify.y=(int) windows->image.height/2;
      break;
    }
    default:
    {
      XMoveMagnifyPosition(windows,quantum,key_symbol);
      break;
    }
  }
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X M a g n i f y W i n d o w C o m m a n d                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMagnifyWindowCommand() moves the image within an Magnify window by one
%  pixel as specified by the key symbol.
%
%  The format of the XMagnifyWindowCommand method is:
%
%      void XMagnifyWindowCommand(Display *display,XWindows *windows,
%        const MagickStatusType state,const KeySym key_symbol,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o state: key mask.
%
%    o key_symbol: Specifies a KeySym which indicates which side of the image
%      to trim.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XMagnifyWindowCommand(const MagnifyWindowInput *input,
  const MagickStatusType state,const KeySym key_symbol)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  XWindows
    *const windows = input->windows;

  unsigned int
    quantum;

  /*
    User specified a magnify factor or position.
  */
  quantum=1;
  if ((state & Mod1Mask) != 0)
    quantum=10;
  XApplyMagnifyKey(key_symbol,display,windows,quantum);
  XMakeMagnifyImage(display,windows,exception);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X M a k e P a n I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMakePanImage() creates a thumbnail of the image and displays it in the Pan
%  icon window.
%
%  The format of the XMakePanImage method is:
%
%        void XMakePanImage(Display *display,XResourceInfo *resource_info,
%          XWindows *windows,Image *image,ExceptionInfo *exception)
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
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XMakePanImage(Display *display,XResourceInfo *resource_info,
  XWindows *windows,Image *image,ExceptionInfo *exception)
{
  MagickStatusType
    status;

  /*
    Create and display image for panning icon.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  windows->pan.x=(int) windows->image.x;
  windows->pan.y=(int) windows->image.y;
  status=XMakeImage(display,resource_info,&windows->pan,image,
    windows->pan.width,windows->pan.height,exception);
  if (status == MagickFalse)
    ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  (void) XSetWindowBackgroundPixmap(display,windows->pan.id,
    windows->pan.pixmap);
  (void) XClearWindow(display,windows->pan.id);
  XDrawPanRectangle(display,windows);
  XSetCursorState(display,windows,MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X M a t t a E d i t I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMatteEditImage() allows the user to interactively change the Matte channel
%  of an image.  If the image is PseudoClass it is promoted to DirectClass
%  before the matte information is stored.
%
%  The format of the XMatteEditImage method is:
%
%      MagickBooleanType XMatteEditImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image **image,
%        ExceptionInfo *exception)
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
static void XReplaceRowAlpha(Image **image,Quantum *q,const PixelInfo target,
  const char *matte)
{
  int
    x;

  PixelInfo
    pixel;

  for (x=0; x < (int) (*image)->columns; x++)
  {
    GetPixelInfoPixel(*image,q,&pixel);
    if (IsFuzzyEquivalencePixelInfo(&pixel,&target))
      SetPixelAlpha(*image,(Quantum) StringToLong(matte),q);
    q+=(ptrdiff_t) GetPixelChannels(*image);
  }
}

static void XDiscardMagnifyKeys(Display *display,XWindows *windows,
  XEvent *event)
{
  if ((*event).xkey.window == windows->magnify.id)
    {
      Window
        window;

      window=windows->magnify.id;
      while (XCheckWindowEvent(display,window,KeyPressMask,event)) ;
    }
}

static void XSelectPaintMethod(const EditMenuInput *menu,
  const PaintMethodTargets *targets)
{
  Display
    *const display = menu->display;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const MatteEditMenu = menu->MatteEditMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  PaintMethod
    *const method = targets->method;

  char
    **methods;

  int
    entry;

  /*
    Select a method from the pop-up menu.
  */
  methods=GetCommandOptions(MagickMethodOptions);
  if (methods == (char **) NULL)
    return;
  {
    const XMenuRequest
      menu_request = { MatteEditMenu[id], (const char **) methods, command };

    entry=XMenuWidget(display,windows,&menu_request);
  }
  if (entry >= 0)
    (*method)=(PaintMethod) ParseCommandOption(MagickMethodOptions,
      MagickFalse,methods[entry]);
  methods=DestroyStringList(methods);
}

static void XSelectFuzzFactor(const EditMenuInput *menu,
  const char *const *FuzzMenu,char *fuzz,Image **image)
{
  Display
    *const display = menu->display;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const MatteEditMenu = menu->MatteEditMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  int
    entry;

  /*
    Select a command from the pop-up menu.
  */
  {
    const XMenuRequest
      menu_request = { MatteEditMenu[id], FuzzMenu, command };

    entry=XMenuWidget(display,windows,&menu_request);
  }
  if (entry < 0)
    return;
  if (entry != 5)
    {
      (*image)->fuzz=StringToDoubleInterval(FuzzMenu[entry],(double)
        QuantumRange+1.0);
      return;
    }
  (void) CopyMagickString(fuzz,"20%",MagickPathExtent);
  {
    const XDialogRequest
      dialog_request = { "Ok", "Enter fuzz factor (0.0 - 99.9%):", fuzz };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*fuzz == '\0')
    return;
  (void) ConcatenateMagickString(fuzz,"%",MagickPathExtent);
  (*image)->fuzz=StringToDoubleInterval(fuzz,(double) QuantumRange+
    1.0);
}

typedef struct _MatteEditPixelInput
{
  Image
    **image;

  CacheView
    *image_view;

  int
    x_offset,
    y_offset;

  const char
    *matte;

  ExceptionInfo
    *exception;

  XResourceInfo
    *resource_info;

  PaintMethod
    method;

  XColor
    border_color;
} MatteEditPixelInput;

static void XMattePoint(const MatteEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  CacheView
    *const image_view = pixel_input->image_view;

  const int
    x_offset = pixel_input->x_offset,
    y_offset = pixel_input->y_offset;

  const char
    *const matte = pixel_input->matte;

  ExceptionInfo
    *const exception = pixel_input->exception;

  Quantum
    *q;

  /*
    Update matte information using point algorithm.
  */
  q=GetCacheViewAuthenticPixels(image_view,(ssize_t) x_offset,
    (ssize_t) y_offset,1,1,exception);
  if (q == (Quantum *) NULL)
    return;
  SetPixelAlpha(*image,(Quantum) StringToLong(matte),q);
  (void) SyncCacheViewAuthenticPixels(image_view,exception);
}

static void XMatteReplace(const MatteEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  CacheView
    *const image_view = pixel_input->image_view;

  const int
    x_offset = pixel_input->x_offset,
    y_offset = pixel_input->y_offset;

  const char
    *const matte = pixel_input->matte;

  ExceptionInfo
    *const exception = pixel_input->exception;

  int
    y;

  PixelInfo
    target;

  Quantum
    *q;

  /*
    Update matte information using replace algorithm.
  */
  (void) GetOneCacheViewVirtualPixelInfo(image_view,(ssize_t)
    x_offset,(ssize_t) y_offset,&target,exception);
  for (y=0; y < (int) (*image)->rows; y++)
  {
    q=GetCacheViewAuthenticPixels(image_view,0,(ssize_t) y,
      (*image)->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    XReplaceRowAlpha(image,q,target,matte);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
  }
}

static void XMatteReset(const MatteEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  CacheView
    *const image_view = pixel_input->image_view;

  const char
    *const matte = pixel_input->matte;

  ExceptionInfo
    *const exception = pixel_input->exception;

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
      SetPixelAlpha(*image,(Quantum) StringToLong(matte),q);
      q+=(ptrdiff_t) GetPixelChannels(*image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
  }
  if (StringToLong(matte) == (long) OpaqueAlpha)
    (*image)->alpha_trait=UndefinedPixelTrait;
}

static void XMatteFloodfill(const MatteEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  const int
    x_offset = pixel_input->x_offset,
    y_offset = pixel_input->y_offset;

  const char
    *const matte = pixel_input->matte;

  ExceptionInfo
    *const exception = pixel_input->exception;

  XResourceInfo
    *const resource_info = pixel_input->resource_info;

  const PaintMethod
    method = pixel_input->method;

  const XColor
    border_color = pixel_input->border_color;

  ChannelType
    channel_mask;

  DrawInfo
    *draw_info;

  PixelInfo
    target;

  /*
    Update matte information using floodfill algorithm.
  */
  (void) GetOneVirtualPixelInfo(*image,
    GetPixelCacheVirtualMethod(*image),(ssize_t) x_offset,(ssize_t)
    y_offset,&target,exception);
  if (method == FillToBorderMethod)
    {
      target.red=(double) ScaleShortToQuantum(
        border_color.red);
      target.green=(double) ScaleShortToQuantum(
        border_color.green);
      target.blue=(double) ScaleShortToQuantum(
        border_color.blue);
    }
  draw_info=CloneDrawInfo(resource_info->image_info,
    (DrawInfo *) NULL);
  draw_info->fill.alpha=(double) ClampToQuantum(
    StringToDouble(matte,(char **) NULL));
  channel_mask=SetImageChannelMask(*image,AlphaChannel);
  (void) FloodfillPaintImage(*image,draw_info,&target,(ssize_t)
    x_offset,(ssize_t) y_offset,
    (MagickBooleanType) (method != FloodfillMethod),exception);
  (void) SetPixelChannelMask(*image,channel_mask);
  draw_info=DestroyDrawInfo(draw_info);
}

static MagickBooleanType XMatteEditPixels(
  const MatteEditPixelInput pixel_input,const PaintMethod method,
  Image **image,ExceptionInfo *exception)
{
  switch (method)
  {
    case PointMethod:
    default:
    {
      XMattePoint(&pixel_input);
      break;
    }
    case ReplaceMethod:
    {
      XMatteReplace(&pixel_input);
      break;
    }
    case FloodfillMethod:
    case FillToBorderMethod:
    {
      XMatteFloodfill(&pixel_input);
      break;
    }
    case ResetMethod:
    {
      /*
        Update matte information using reset algorithm.
      */
      if (SetImageStorageClass(*image,DirectClass,exception) == MagickFalse)
        return(MagickFalse);
      XMatteReset(&pixel_input);
      break;
    }
  }
  return(MagickTrue);
}

typedef struct _BorderColorTargets
{
  XColor
    *border_color;
} BorderColorTargets;

static void XSelectBorderColor(const EditMenuInput *menu,
  XResourceInfo *resource_info,const BorderColorTargets *targets)
{
  Display
    *const display = menu->display;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const MatteEditMenu = menu->MatteEditMenu;

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

  int
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
      menu_request = { MatteEditMenu[id], (const char **) ColorMenu, command };

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

static void XSelectMatteValue(const EditMenuInput *menu,
  const char *const *MatteMenu,char *message,char *matte)
{
  Display
    *const display = menu->display;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const MatteEditMenu = menu->MatteEditMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  int
    entry;

  /*
    Select a command from the pop-up menu.
  */
  {
    const XMenuRequest
      menu_request = { MatteEditMenu[id], MatteMenu, command };

    entry=XMenuWidget(display,windows,&menu_request);
  }
  if (entry < 0)
    return;
  if (entry != 2)
    {
      (void) FormatLocaleString(matte,MagickPathExtent,"%g",
        (double) OpaqueAlpha);
      if (LocaleCompare(MatteMenu[entry],"Transparent") == 0)
        (void) FormatLocaleString(matte,MagickPathExtent,"%g",
          (double) TransparentAlpha);
      return;
    }
  (void) FormatLocaleString(message,MagickPathExtent,
    "Enter matte value (0 - " "%g" "):",(double) QuantumRange);
  {
    const XDialogRequest
      dialog_request = { "Matte", message, matte };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*matte == '\0')
    return;
}

typedef struct _MatteCommandInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const ModeType
    *MatteEditCommands;

  Image
    **image;

  ExceptionInfo
    *exception;

  const char
    *const *MatteEditMenu;

  char
    *command,
    *matte;

  PaintMethodTargets
    method_targets;

  BorderColorTargets
    border_targets;

  char
    *text;
} MatteCommandInput;

static void XMatteEditControl(const MatteCommandInput *input,const int id,
  size_t *state)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  const ModeType
    *const MatteEditCommands = input->MatteEditCommands;

  Image
    **const image = input->image;

  ExceptionInfo
    *const exception = input->exception;

  switch (MatteEditCommands[id])
  {
    case MatteEditUndoCommand:
    {
      (void) XMagickCommand(display,resource_info,windows,UndoCommand,
        image,exception);
      break;
    }
    case MatteEditHelpCommand:
    {
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Matte Edit",ImageMatteEditHelp);
      }
      break;
    }
    case MatteEditDismissCommand:
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

static void XMatteEditValues(const MatteCommandInput *input,const int id,
  size_t *state)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  const ModeType
    *const MatteEditCommands = input->MatteEditCommands;

  Image
    **const image = input->image;

  const char
    *const *const MatteEditMenu = input->MatteEditMenu;

  EditMenuInput
    menu_input;

  switch (MatteEditCommands[id])
  {
    case MatteEditFuzzCommand:
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

      menu_input.display=display;
      menu_input.windows=windows;
      menu_input.MatteEditMenu=MatteEditMenu;
      menu_input.id=id;
      menu_input.command=input->command;
      XSelectFuzzFactor(&menu_input,FuzzMenu,fuzz,image);
      break;
    }
    case MatteEditValueCommand:
    {
      const char
        *const MatteMenu[] =
        {
          "Opaque",
          "Transparent",
          "Dialog...",
          (char *) NULL,
        };

      static char
        message[MagickPathExtent];

      menu_input.display=display;
      menu_input.windows=windows;
      menu_input.MatteEditMenu=MatteEditMenu;
      menu_input.id=id;
      menu_input.command=input->command;
      XSelectMatteValue(&menu_input,MatteMenu,message,input->matte);
      break;
    }
    default:
    {
      XMatteEditControl(input,id,state);
      break;
    }
  }
}

static MagickBooleanType XMatteCommandBlock(const MatteCommandInput *input,
  XEvent *event,const Cursor cursor,size_t *state)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  const char
    *const *const MatteEditMenu = input->MatteEditMenu;

  const ModeType
    *const MatteEditCommands = input->MatteEditCommands;

  const PaintMethodTargets
    method_targets = input->method_targets;

  const BorderColorTargets
    border_targets = input->border_targets;

  int
    id;

  if ((*event).xany.window == windows->command.id)
    {
      EditMenuInput
        menu_input;

      /*
        Select a command from the Command widget.
      */
      id=XCommandWidget(display,windows,MatteEditMenu,event);
      if (id < 0)
        {
          (void) XCheckDefineCursor(display,windows->image.id,cursor);
          return(MagickTrue);
        }
      switch (MatteEditCommands[id])
      {
        case MatteEditMethod:
        {
          menu_input.display=display;
          menu_input.windows=windows;
          menu_input.MatteEditMenu=MatteEditMenu;
          menu_input.id=id;
          menu_input.command=input->command;
          XSelectPaintMethod(&menu_input,&method_targets);
          break;
        }
        case MatteEditBorderCommand:
        {
          menu_input.display=display;
          menu_input.windows=windows;
          menu_input.MatteEditMenu=MatteEditMenu;
          menu_input.id=id;
          menu_input.command=input->command;
          XSelectBorderColor(&menu_input,input->resource_info,&border_targets);
          break;
        }
        default:
        {
          XMatteEditValues(input,id,state);
          break;
        }
      }
      (void) XCheckDefineCursor(display,windows->image.id,cursor);
      return(MagickTrue);
    }
  return(MagickFalse);
}

static MagickBooleanType XMatteWritePixels(const MatteCommandInput *input,
  const int x_offset,const int y_offset)
{
  Image
    **const image = input->image;

  ExceptionInfo
    *const exception = input->exception;

  const PaintMethod
    method = *input->method_targets.method;

  const XColor
    border_color = *input->border_targets.border_color;

  CacheView
    *image_view;

  MatteEditPixelInput
    pixel_input;

  if (SetImageStorageClass(*image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if ((*image)->alpha_trait == UndefinedPixelTrait)
    (void) SetImageAlphaChannel(*image,OpaqueAlphaChannel,exception);
  image_view=AcquireAuthenticCacheView(*image,exception);
  pixel_input.image=image;
  pixel_input.image_view=image_view;
  pixel_input.x_offset=x_offset;
  pixel_input.y_offset=y_offset;
  pixel_input.matte=input->matte;
  pixel_input.exception=exception;
  pixel_input.resource_info=input->resource_info;
  pixel_input.method=method;
  pixel_input.border_color=border_color;
  if (XMatteEditPixels(pixel_input,method,image,exception) == MagickFalse)
    return(MagickFalse);
  image_view=DestroyCacheView(image_view);
  return(MagickTrue);
}

static MagickBooleanType XMatteUpdatePixels(const MatteCommandInput *input,
  int x_offset,int y_offset,size_t *state)
{
  XWindows
    *const windows = input->windows;

  Image
    **const image = input->image;

  unsigned int
    height,
    width;

  if (((*state) & UpdateConfigurationState) != 0)
    {
      int
        x,
        y;

      /*
        Matte edit is relative to image configuration.
      */
      (void) XClearArea(input->display,windows->image.id,x_offset,y_offset,1,1,
        MagickTrue);
      XPutPixel(windows->image.ximage,x_offset,y_offset,
        windows->pixel_info->background_color.pixel);
      width=(unsigned int) (*image)->columns;
      height=(unsigned int) (*image)->rows;
      x=0;
      y=0;
      if (windows->image.crop_geometry != (char *) NULL)
        (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,
          &height);
      x_offset=((int) width*(windows->image.x+x_offset)/
        windows->image.ximage->width+x);
      y_offset=((int) height*(windows->image.y+y_offset)/
        windows->image.ximage->height+y);
      if ((x_offset < 0) || (y_offset < 0))
        return(MagickTrue);
      if ((x_offset >= (int) (*image)->columns) ||
          (y_offset >= (int) (*image)->rows))
        return(MagickTrue);
      if (XMatteWritePixels(input,x_offset,y_offset) == MagickFalse)
        return(MagickFalse);
      (*state)&=(unsigned int) (~UpdateConfigurationState);
    }
  return(MagickTrue);
}

static size_t XMatteKeyPress(const MatteCommandInput *input,XEvent *event,
  size_t state)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  XDiscardMagnifyKeys(display,windows,event);
  if ((*event).xkey.window != windows->image.id)
    return(state);
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
      state|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      {
        const XHelpView view = { display, input->resource_info, windows,
          MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Matte Edit",ImageMatteEditHelp);
      }
      break;
    }
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
  return(state);
}

typedef struct _MatteEventTargets
{
  int
    *x,
    *y;

  size_t
    *state;
} MatteEventTargets;

static void XMattePress(const MatteCommandInput *input,const XEvent *event,
  const MatteEventTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    **const image = input->image;

  ExceptionInfo
    *const exception = input->exception;

  int
    *const x = targets->x,
    *const y = targets->y;

  size_t
    *const state = targets->state;

  if ((*event).xbutton.button != Button1)
    return;
  if (((*event).xbutton.window != windows->image.id) &&
      ((*event).xbutton.window != windows->magnify.id))
    return;
  /*
    Update matte data.
  */
  (*x)=(*event).xbutton.x;
  (*y)=(*event).xbutton.y;
  (void) XMagickCommand(display,resource_info,windows,
    SaveToUndoBufferCommand,image,exception);
  (*state)|=UpdateConfigurationState;
}

static void XMatteRelease(const MatteCommandInput *input,const XEvent *event,
  const Cursor cursor,const MatteEventTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    **const image = input->image;

  ExceptionInfo
    *const exception = input->exception;

  int
    *const x = targets->x,
    *const y = targets->y;

  size_t
    *const state = targets->state;

  if ((*event).xbutton.button != Button1)
    return;
  if (((*event).xbutton.window != windows->image.id) &&
      ((*event).xbutton.window != windows->magnify.id))
    return;
  /*
    Update colormap information.
  */
  (*x)=(*event).xbutton.x;
  (*y)=(*event).xbutton.y;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
  XInfoWidget(display,windows,input->text);
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  (*state)&=(unsigned int) (~UpdateConfigurationState);
}

static void XMatteEditEvent(const MatteCommandInput *input,XEvent *event,
  const Cursor cursor,const MatteEventTargets *targets)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  int
    *const x = targets->x,
    *const y = targets->y;

  size_t
    *const state = targets->state;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XMattePress(input,event,targets);
      break;
    }
    case ButtonRelease:
    {
      XMatteRelease(input,event,cursor,targets);
      break;
    }
    case Expose:
      break;
    case KeyPress:
    {
      (*state)=XMatteKeyPress(input,event,(*state));
      break;
    }
    case MotionNotify:
    {
      /*
        Map and unmap Info widget as cursor crosses its boundaries.
      */
      (*x)=(*event).xmotion.x;
      (*y)=(*event).xmotion.y;
      XTrackInfoWidget(display,windows,(*x),(*y));
      break;
    }
    default:
      break;
  }
}

static MagickBooleanType XMatteEditStep(const MatteCommandInput *input,
  const Cursor cursor,const MatteEventTargets *targets)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  XWindows
    *const windows = input->windows;

  int
    *const x = targets->x,
    *const y = targets->y;

  size_t
    *const state = targets->state;

  int
    x_offset,
    y_offset;

  XEvent
    event;

  if (windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(input->text,MagickPathExtent," %+d%+d ",
        (*x)+windows->image.x,(*y)+windows->image.y);
      XInfoWidget(display,windows,input->text);
    }
  /*
    Wait for next event.
  */
  XScreenEvent(display,windows,&event,exception);
  if (XMatteCommandBlock(input,&event,cursor,state) != MagickFalse)
    return(MagickTrue);
  XMatteEditEvent(input,&event,cursor,targets);
  if (event.xany.window == windows->magnify.id)
    {
      (*x)=windows->magnify.x-windows->image.x;
      (*y)=windows->magnify.y-windows->image.y;
    }
  x_offset=(*x);
  y_offset=(*y);
  if (XMatteUpdatePixels(input,x_offset,y_offset,state) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static const ModeType
  MatteEditCommandTable[] =
  {
    MatteEditMethod,
    MatteEditBorderCommand,
    MatteEditFuzzCommand,
    MatteEditValueCommand,
    MatteEditUndoCommand,
    MatteEditHelpCommand,
    MatteEditDismissCommand
  };

static MagickBooleanType XMatteEditImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image **image,
  ExceptionInfo *exception)
{
  const char
    *const MatteEditMenu[] =
    {
      "Method",
      "Border Color",
      "Fuzz",
      "Matte Value",
      "Undo",
      "Help",
      "Dismiss",
      (char *) NULL
    };

  static char
    matte[MagickPathExtent] = "0";

  static PaintMethod
    method = PointMethod;

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

  const PaintMethodTargets
    method_targets = { &method };

  const BorderColorTargets
    border_targets = { &border_color };

  const MatteEventTargets
    event_targets = { &x, &y, &state };

  const MatteCommandInput
    command_input = { display, resource_info, windows, MatteEditCommandTable,
      image, exception, MatteEditMenu, command, matte, method_targets,
      border_targets, text };

  /*
    Map Command widget.
  */
  (void) CloneString(&windows->command.name,"Matte Edit");
  windows->command.data=4;
  (void) XCommandWidget(display,windows,MatteEditMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
  /*
    Make cursor.
  */
  cursor=XMakeCursor(display,windows->image.id,windows->map_info->colormap,
    resource_info->background_color,resource_info->foreground_color);
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  /*
    Track pointer until button 1 is pressed.
  */
  XQueryPosition(display,windows->image.id,&x,&y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  state=DefaultState;
  do
  {
    if (XMatteEditStep(&command_input,cursor,&event_targets) == MagickFalse)
      return(MagickFalse);
  } while ((state & ExitState) == 0);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  XSetCursorState(display,windows,MagickFalse);
  (void) XFreeCursor(display,cursor);
  return(MagickTrue);
}

static MagickBooleanType XSelectCommandLineFile(Display *display,
  XWindows *windows,char *filename)
{
  char
    **filelist,
    **files;

  int
    count,
    status;

  int
    i,
    j;

  /*
    Select next image from the command line.
  */
  status=XGetCommand(display,windows->image.id,&files,&count);
  if (status == 0)
    {
      ThrowXWindowException(XServerError,"UnableToGetProperty","...");
      return(MagickFalse);
    }
  filelist=(char **) AcquireQuantumMemory((size_t) count,sizeof(*filelist));
  if (filelist == (char **) NULL)
    {
      ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
        "...");
      (void) XFreeStringList(files);
      return(MagickFalse);
    }
  j=0;
  for (i=1; i < count; i++)
    if (*files[i] != '-')
      filelist[j++]=files[i];
  filelist[j]=(char *) NULL;
  {
    const XListBrowserRequest
      browse_request = { (const char **) filelist, "Load",
        "Select Image to Load:", filename };

    XListBrowserWidget(display,windows,&windows->widget,&browse_request);
  }
  filelist=(char **) RelinquishMagickMemory(filelist);
  (void) XFreeStringList(files);
  return(MagickTrue);
}

static MagickBooleanType XSelectOpenFile(Display *display,XWindows *windows,
  const MagickBooleanType command,char *filename)
{
  if (command == MagickFalse)
    XFileBrowserWidget(display,windows,"Open",filename);
  else
    {
      if (XSelectCommandLineFile(display,windows,filename) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType XGrabDelay(Display *display,XWindows *windows,
  const ImageInfo *image_info)
{
  if (LocaleCompare(image_info->magick,"X") == 0)
    {
      char
        seconds[MagickPathExtent];

      /*
        User may want to delay the X server screen grab.
      */
      (void) CopyMagickString(seconds,"0",MagickPathExtent);
      {
        const XDialogRequest
          dialog_request = { "Grab", "Enter any delay in seconds:", seconds };

        (void) XDialogWidget(display,windows,&dialog_request);
      }
      if (*seconds == '\0')
        return(MagickFalse);
      XDelay(display,(size_t) (1000*StringToLong(seconds)));
    }
  return(MagickTrue);
}

static void XRequestImageSize(Display *display,XWindows *windows,
  ImageInfo *image_info,const MagickInfo *magick_info)
{
  if ((magick_info != (const MagickInfo *) NULL) &&
      GetMagickRawSupport(magick_info) == MagickTrue)
    {
      char
        geometry[MagickPathExtent];

      /*
        Request image size from the user.
      */
      (void) CopyMagickString(geometry,"512x512",MagickPathExtent);
      if (image_info->size != (char *) NULL)
        (void) CopyMagickString(geometry,image_info->size,MagickPathExtent);
      {
        const XDialogRequest
          dialog_request = { "Load", "Enter the image geometry:", geometry };

        (void) XDialogWidget(display,windows,&dialog_request);
      }
      (void) CloneString(&image_info->size,geometry);
    }
}

static void XDestroyTextList(char **textlist)
{
  int
    i;

  for (i=0; textlist[i] != (char *) NULL; i++)
    textlist[i]=DestroyString(textlist[i]);
  textlist=(char **) RelinquishMagickMemory(textlist);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X O p e n I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XOpenImage() loads an image from a file.
%
%  The format of the XOpenImage method is:
%
%     Image *XOpenImage(Display *display,XResourceInfo *resource_info,
%       XWindows *windows,const unsigned int command)
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
%    o command: A value other than zero indicates that the file is selected
%      from the command line argument list.
%
*/
static Image *XOpenImage(Display *display,XResourceInfo *resource_info,
  XWindows *windows,const MagickBooleanType command)
{
  const MagickInfo
    *magick_info;

  ExceptionInfo
    *exception;

  Image
    *nexus;

  ImageInfo
    *image_info;

  static char
    filename[MagickPathExtent] = "\0";

  /*
    Request file name from user.
  */
  if (XSelectOpenFile(display,windows,command,filename) == MagickFalse)
    return((Image *) NULL);
  if (*filename == '\0')
    return((Image *) NULL);
  image_info=CloneImageInfo(resource_info->image_info);
  (void) SetImageInfoProgressMonitor(image_info,(MagickProgressMonitor) NULL,
    (void *) NULL);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  exception=AcquireExceptionInfo();
  (void) SetImageInfo(image_info,0,exception);
  if (XGrabDelay(display,windows,image_info) == MagickFalse)
    return((Image *) NULL);
  magick_info=GetMagickInfo(image_info->magick,exception);
  XRequestImageSize(display,windows,image_info,magick_info);
  /*
    Load the image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  nexus=ReadImage(image_info,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (nexus != (Image *) NULL)
    XClientMessage(display,windows->image.id,windows->im_protocols,
      windows->im_next_image,CurrentTime);
  else
    {
      char
        *text,
        **textlist;

      /*
        Unknown image format.
      */
      text=FileToString(filename,~0UL,exception);
      if (text == (char *) NULL)
        return((Image *) NULL);
      textlist=StringToList(text);
      if (textlist != (char **) NULL)
        {
          char
            title[MagickPathExtent];

          const XHelpView
            view = { display, resource_info, windows, MagickTrue };

          (void) FormatLocaleString(title,MagickPathExtent,
            "Unknown format: %s",filename);
          XTextViewWidget(&view,title,(const char **) textlist);
          XDestroyTextList(textlist);
        }
      text=DestroyString(text);
    }
  exception=DestroyExceptionInfo(exception);
  image_info=DestroyImageInfo(image_info);
  return(nexus);
}

static Cursor XSelectPanCursor(Display *display,XWindows *windows)
{
  Cursor
    cursor;

  if ((windows->image.ximage->width > (int) windows->image.width) &&
      (windows->image.ximage->height > (int) windows->image.height))
    cursor=XCreateFontCursor(display,XC_fleur);
  else
    if (windows->image.ximage->width > (int) windows->image.width)
      cursor=XCreateFontCursor(display,XC_sb_h_double_arrow);
    else
      if (windows->image.ximage->height > (int) windows->image.height)
        cursor=XCreateFontCursor(display,XC_sb_v_double_arrow);
      else
        cursor=XCreateFontCursor(display,XC_arrow);
  return(cursor);
}

static void XClampPanOffsetY(RectangleInfo *pan_info,XWindows *windows)
{
  if ((*pan_info).y < 0)
    (*pan_info).y=0;
  else
    if ((int) ((*pan_info).y+windows->image.height) >
        windows->image.ximage->height)
      (*pan_info).y=(ssize_t) (windows->image.ximage->height-(int)
        windows->image.height);
}

static void XClampPanOffset(XWindows *windows,RectangleInfo *pan_info,
  const double x_factor,const double y_factor)
{
  if ((*pan_info).x < (ssize_t) ((*pan_info).width/2))
    (*pan_info).x=0;
  else
    (*pan_info).x=(ssize_t) (x_factor*((*pan_info).x-((int)
      (*pan_info).width/2)));
  if ((*pan_info).x < 0)
    (*pan_info).x=0;
  else
    if ((int) ((*pan_info).x+windows->image.width) >
        windows->image.ximage->width)
      (*pan_info).x=(ssize_t) (windows->image.ximage->width-(int)
        windows->image.width);
  if ((*pan_info).y < (ssize_t) ((*pan_info).height/2))
    (*pan_info).y=0;
  else
    (*pan_info).y=(ssize_t) (y_factor*((*pan_info).y-((int)
      (*pan_info).height/2)));
  XClampPanOffsetY(pan_info,windows);
}

static void XDisplayPanOffset(Display *display,XWindows *windows,
  const RectangleInfo pan_info)
{
  char
    text[MagickPathExtent];

  if ((windows->image.x != (int) pan_info.x) ||
      (windows->image.y != (int) pan_info.y))
    {
      /*
        Display image pan offset.
      */
      windows->image.x=(int) pan_info.x;
      windows->image.y=(int) pan_info.y;
      (void) FormatLocaleString(text,MagickPathExtent," %ux%u%+d%+d ",
        windows->image.width,windows->image.height,windows->image.x,
        windows->image.y);
      XInfoWidget(display,windows,text);
      /*
        Refresh Image window.
      */
      XDrawPanRectangle(display,windows);
      XRefreshWindow(display,&windows->image,(XEvent *) NULL);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X P a n I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XPanImage() pans the image until the mouse button is released.
%
%  The format of the XPanImage method is:
%
%      void XPanImage(Display *display,XWindows *windows,XEvent *event,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o event: Specifies a pointer to a XEvent structure.  If it is NULL,
%      the entire image is refreshed.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XTrackPanPointer(XWindows *windows,XEvent *event,Display *display,
  ExceptionInfo *exception)
{
  double
    x_factor,
    y_factor;

  RectangleInfo
    pan_info;

  size_t
    state;

  x_factor=(double) windows->image.ximage->width/windows->pan.width;
  y_factor=(double) windows->image.ximage->height/windows->pan.height;
  pan_info.width=windows->pan.width*windows->image.width/
    (unsigned int) windows->image.ximage->width;
  pan_info.height=windows->pan.height*windows->image.height/
    (unsigned int) windows->image.ximage->height;
  pan_info.x=0;
  pan_info.y=0;
  state=UpdateConfigurationState;
  do
  {
    switch (event->type)
    {
      case ButtonPress:
      {
        /*
          User choose an initial pan location.
        */
        pan_info.x=(ssize_t) event->xbutton.x;
        pan_info.y=(ssize_t) event->xbutton.y;
        state|=UpdateConfigurationState;
        break;
      }
      case ButtonRelease:
      {
        /*
          User has finished panning the image.
        */
        pan_info.x=(ssize_t) event->xbutton.x;
        pan_info.y=(ssize_t) event->xbutton.y;
        state|=UpdateConfigurationState | ExitState;
        break;
      }
      case MotionNotify:
      {
        pan_info.x=(ssize_t) event->xmotion.x;
        pan_info.y=(ssize_t) event->xmotion.y;
        state|=UpdateConfigurationState;
      }
      default:
        break;
    }
    if ((state & UpdateConfigurationState) != 0)
      {
        /*
          Check boundary conditions.
        */
        XClampPanOffset(windows,&pan_info,x_factor,y_factor);
        XDisplayPanOffset(display,windows,pan_info);
        state&=(unsigned int) (~UpdateConfigurationState);
      }
    /*
      Wait for next event.
    */
    if ((state & ExitState) == 0)
      XScreenEvent(display,windows,event,exception);
  } while ((state & ExitState) == 0);
}

static void XRestorePanCursor(Display *display,XWindows *windows,Cursor cursor)
{
  (void) XCheckDefineCursor(display,windows->pan.id,windows->pan.cursor);
  (void) XFreeCursor(display,cursor);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
}

static void XPanImage(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  Cursor
    cursor;

  /*
    Define cursor.
  */
  cursor=XSelectPanCursor(display,windows);
  (void) XCheckDefineCursor(display,windows->pan.id,cursor);
  /*
    Pan image as pointer moves until the mouse button is released.
  */
  XTrackPanPointer(windows,event,display,exception);
  /*
    Restore cursor.
  */
  XRestorePanCursor(display,windows,cursor);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X P a s t e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XPasteImage() pastes an image previously saved with XCropImage in the X
%  window image at a location the user chooses with the pointer.
%
%  The format of the XPasteImage method is:
%
%      MagickBooleanType XPasteImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        ExceptionInfo *exception)
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
static CompositeOperator XSelectPasteOperator(const ComposeMenuInfo *menu_info,
  const char *const *PasteMenu,CompositeOperator compose)
{
  Display
    *const display = menu_info->display;

  XWindows
    *const windows = menu_info->windows;

  const int
    id = menu_info->id;

  char
    command[MagickPathExtent],
    **operators;

  int
    entry;

  /*
    Select a command from the pop-up menu.
  */
  operators=GetCommandOptions(MagickComposeOptions);
  if (operators == (char **) NULL)
    return(compose);
  {
    const XMenuRequest
      menu_request = { PasteMenu[id], (const char **) operators, command };

    entry=XMenuWidget(display,windows,&menu_request);
  }
  if (entry >= 0)
    compose=(CompositeOperator) ParseCommandOption(
      MagickComposeOptions,MagickFalse,operators[entry]);
  operators=DestroyStringList(operators);
  return(compose);
}

typedef struct _PasteCommandInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const char
    *const *PasteMenu;

  const ModeType
    *PasteCommands;

  Image
    *image;

  ExceptionInfo
    *exception;
} PasteCommandInput;

static MagickBooleanType XPasteCommand(const PasteCommandInput *input,
  XEvent *event,CompositeOperator *compose,size_t *state)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  const char
    *const *const PasteMenu = input->PasteMenu;

  const ModeType
    *const PasteCommands = input->PasteCommands;

  int
    id;

  if ((*event).xany.window == windows->command.id)
    {
      /*
        Select a command from the Command widget.
      */
      id=XCommandWidget(display,windows,PasteMenu,event);
      if (id < 0)
        return(MagickTrue);
      switch (PasteCommands[id])
      {
        case PasteOperatorsCommand:
        {
          ComposeMenuInfo
            menu_info;

          menu_info.display=display;
          menu_info.windows=windows;
          menu_info.id=id;
          (*compose)=XSelectPasteOperator(&menu_info,PasteMenu,(*compose));
          break;
        }
        case PasteHelpCommand:
        {
          {
            const XHelpView view = { display, resource_info, windows,
              MagickFalse };

            XTextViewHelp(&view,"Help Viewer - Image Composite",ImagePasteHelp);
          }
          break;
        }
        case PasteDismissCommand:
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
      return(MagickTrue);
    }
  return(MagickFalse);
}

typedef struct _PasteFinishInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    *image;

  ExceptionInfo
    *exception;

  Cursor
    cursor;

  size_t
    state;

  Image
    *paste_image;

  RectangleInfo
    paste_info;

  CompositeOperator
    compose;
} PasteFinishInput;

static MagickBooleanType XPasteFinish(const PasteFinishInput *input)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    *const image = input->image;

  ExceptionInfo
    *const exception = input->exception;

  const Cursor
    cursor = input->cursor;

  const size_t
    state = input->state;

  const CompositeOperator
    compose = input->compose;

  Image
    *paste_image = input->paste_image;

  RectangleInfo
    paste_info = input->paste_info;

  double
    scale_factor;

  int
    x,
    y;

  unsigned int
    height,
    width;

  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
  XSetCursorState(display,windows,MagickFalse);
  (void) XFreeCursor(display,cursor);
  if ((state & EscapeState) != 0)
    return(MagickTrue);
  /*
    Image pasting is relative to image configuration.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  x=0;
  y=0;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,&height);
  scale_factor=(double) width/windows->image.ximage->width;
  paste_info.x+=x;
  paste_info.x=(ssize_t) (scale_factor*paste_info.x+0.5);
  paste_info.width=(unsigned int) (scale_factor*paste_info.width+0.5);
  scale_factor=(double) height/windows->image.ximage->height;
  paste_info.y+=y;
  paste_info.y=(ssize_t) (scale_factor*paste_info.y*scale_factor+0.5);
  paste_info.height=(unsigned int) (scale_factor*paste_info.height+0.5);
  /*
    Paste image with X Image window.
  */
  (void) CompositeImage(image,paste_image,compose,MagickTrue,paste_info.x,
    paste_info.y,exception);
  paste_image=DestroyImage(paste_image);
  XSetCursorState(display,windows,MagickFalse);
  /*
    Update image colormap.
  */
  XConfigureImageColormap(display,resource_info,windows,image,exception);
  (void) XConfigureImage(display,resource_info,windows,image,exception);
  return(MagickTrue);
}

typedef struct _PasteTargets
{
  Image
    **paste_image;

  size_t
    *state;

  RectangleInfo
    *paste_info;

  CompositeOperator
    *compose;
} PasteTargets;

static void XPasteKeyPress(const PasteCommandInput *input,XEvent *event,
  const PasteTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    **const paste_image = targets->paste_image;

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
      "Key press: 0x%lx (%s)",(long) key_symbol,command);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      /*
        Prematurely exit.
      */
      (*paste_image)=DestroyImage((*paste_image));
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

        XTextViewHelp(&view,"Help Viewer - Image Composite",ImagePasteHelp);
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

static void XPastePress(const PasteCommandInput *input,const XEvent *event,
  const Cursor cursor,const PasteTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    *const image = input->image;

  Image
    **const paste_image = targets->paste_image;

  RectangleInfo
    *const paste_info = targets->paste_info;

  double
    scale_factor;

  int
    x,
    y;

  unsigned int
    height,
    width;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Button Press: 0x%lx %u +%d+%d",(*event).xbutton.window,
      (*event).xbutton.button,(*event).xbutton.x,(*event).xbutton.y);
  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != windows->image.id)
    return;
  /*
    Paste rectangle is relative to image configuration.
  */
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  x=0;
  y=0;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,
      &width,&height);
  scale_factor=(double) windows->image.ximage->width/width;
  (*paste_info).width=(unsigned int) (scale_factor*(*paste_image)->columns+
    0.5);
  scale_factor=(double) windows->image.ximage->height/height;
  (*paste_info).height=(unsigned int) (scale_factor*(*paste_image)->rows+
    0.5);
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  (*paste_info).x=(ssize_t) windows->image.x+(*event).xbutton.x;
  (*paste_info).y=(ssize_t) windows->image.y+(*event).xbutton.y;
}

static void XPasteRelease(const PasteCommandInput *input,const XEvent *event,
  const PasteTargets *targets)
{
  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  RectangleInfo
    *const paste_info = targets->paste_info;

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
  if (((*paste_info).width != 0) && ((*paste_info).height != 0))
    {
      /*
        User has selected the location of the paste image.
      */
      (*paste_info).x=(ssize_t) windows->image.x+(*event).xbutton.x;
      (*paste_info).y=(ssize_t) windows->image.y+(*event).xbutton.y;
      (*state)|=ExitState;
    }
}

static void XPasteEvent(const PasteCommandInput *input,XEvent *event,
  const Cursor cursor,const PasteTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  RectangleInfo
    *const paste_info = targets->paste_info;

  int
    x,
    y;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XPastePress(input,event,cursor,targets);
      break;
    }
    case ButtonRelease:
    {
      XPasteRelease(input,event,targets);
      break;
    }
    case Expose:
      break;
    case KeyPress:
    {
      XPasteKeyPress(input,event,targets);
      break;
    }
    case MotionNotify:
    {
      /*
        Map and unmap Info widget as text cursor crosses its boundaries.
      */
      x=(*event).xmotion.x;
      y=(*event).xmotion.y;
      XTrackInfoWidget(display,windows,x,y);
      (*paste_info).x=(ssize_t) windows->image.x+x;
      (*paste_info).y=(ssize_t) windows->image.y+y;
      break;
    }
    default:
    {
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"Event type: %d",
          (*event).type);
      break;
    }
  }
}

static void XPasteStep(const PasteCommandInput *input,const Cursor cursor,
  const PasteTargets *targets)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  XWindows
    *const windows = input->windows;

  CompositeOperator
    *const compose = targets->compose;

  RectangleInfo
    *const paste_info = targets->paste_info;

  size_t
    *const state = targets->state;

  char
    text[MagickPathExtent];

  RectangleInfo
    highlight_info;

  XEvent
    event;

  if (windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(text,MagickPathExtent," %+ld%+ld ",
        (long) (*paste_info).x,(long) (*paste_info).y);
      XInfoWidget(display,windows,text);
    }
  highlight_info=(*paste_info);
  highlight_info.x=(*paste_info).x-windows->image.x;
  highlight_info.y=(*paste_info).y-windows->image.y;
  XHighlightRectangle(display,windows->image.id,
    windows->image.highlight_context,&highlight_info);
  /*
    Wait for next event.
  */
  XScreenEvent(display,windows,&event,exception);
  XHighlightRectangle(display,windows->image.id,
    windows->image.highlight_context,&highlight_info);
  if (XPasteCommand(input,&event,compose,state) != MagickFalse)
    return;
  XPasteEvent(input,&event,cursor,targets);
}

static const ModeType
  PasteCommandTable[] =
  {
    PasteOperatorsCommand,
    PasteHelpCommand,
    PasteDismissCommand
  };

static MagickBooleanType XPasteImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  const char
    *const PasteMenu[] =
    {
      "Operator",
      "Help",
      "Dismiss",
      (char *) NULL
    };

  static CompositeOperator
    compose = CopyCompositeOp;

  Cursor
    cursor;

  Image
    *paste_image;

  int
    x,
    y;

  RectangleInfo
    paste_info;

  size_t
    state;

  const PasteTargets
    paste_targets = { &paste_image, &state, &paste_info, &compose };

  const PasteCommandInput
    command_input = { display, resource_info, windows, PasteMenu,
      PasteCommandTable, image, exception };

  /*
    Copy image.
  */
  if (resource_info->copy_image == (Image *) NULL)
    return(MagickFalse);
  paste_image=CloneImage(resource_info->copy_image,0,0,MagickTrue,exception);
  if (paste_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Map Command widget.
  */
  (void) CloneString(&windows->command.name,"Paste");
  windows->command.data=1;
  (void) XCommandWidget(display,windows,PasteMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
  /*
    Track pointer until button 1 is pressed.
  */
  XSetCursorState(display,windows,MagickFalse);
  XQueryPosition(display,windows->image.id,&x,&y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  paste_info.x=(ssize_t) windows->image.x+x;
  paste_info.y=(ssize_t) windows->image.y+y;
  paste_info.width=0;
  paste_info.height=0;
  cursor=XCreateFontCursor(display,XC_ul_angle);
  (void) XSetFunction(display,windows->image.highlight_context,GXinvert);
  state=DefaultState;
  do
  {
    XPasteStep(&command_input,cursor,&paste_targets);
  } while ((state & ExitState) == 0);
  {
    const PasteFinishInput
      finish = { display, resource_info, windows, image, exception, cursor,
        state, paste_image, paste_info, compose };

    return(XPasteFinish(&finish));
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X P r i n t I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XPrintImage() prints an image to a Postscript printer.
%
%  The format of the XPrintImage method is:
%
%      MagickBooleanType XPrintImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
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
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType XPrintImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent],
    geometry[MagickPathExtent];

  const char
    *const PageSizes[] =
    {
      "Letter",
      "Tabloid",
      "Ledger",
      "Legal",
      "Statement",
      "Executive",
      "A3",
      "A4",
      "A5",
      "B4",
      "B5",
      "Folio",
      "Quarto",
      "10x14",
      (char *) NULL
    };

  Image
    *print_image;

  ImageInfo
    *image_info;

  MagickStatusType
    status;

  /*
    Request Postscript page geometry from user.
  */
  image_info=CloneImageInfo(resource_info->image_info);
  (void) FormatLocaleString(geometry,MagickPathExtent,"Letter");
  if (image_info->page != (char *) NULL)
    (void) CopyMagickString(geometry,image_info->page,MagickPathExtent);
  {
    const XListBrowserRequest
      browse_request = { PageSizes, "Select",
        "Select Postscript Page Geometry:", geometry };

    XListBrowserWidget(display,windows,&windows->widget,&browse_request);
  }
  if (*geometry == '\0')
    return(MagickTrue);
  image_info->page=GetPageGeometry(geometry);
  /*
    Apply image transforms.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  print_image=CloneImage(image,0,0,MagickTrue,exception);
  if (print_image == (Image *) NULL)
    return(MagickFalse);
  (void) FormatLocaleString(geometry,MagickPathExtent,"%dx%d!",
    windows->image.ximage->width,windows->image.ximage->height);
  (void) TransformImage(&print_image,windows->image.crop_geometry,geometry,
    exception);
  /*
    Print image.
  */
  (void) AcquireUniqueFilename(filename);
  (void) FormatLocaleString(print_image->filename,MagickPathExtent,"print:%s",
    filename);
  status=WriteImage(image_info,print_image,exception);
  (void) RelinquishUniqueFileResource(filename);
  print_image=DestroyImage(print_image);
  image_info=DestroyImageInfo(image_info);
  XSetCursorState(display,windows,MagickFalse);
  return((MagickBooleanType) (status != 0));
}
typedef struct _ROIMenuEntry
{
  const char
    *const **Menus;

  const DisplayCommand
    **Commands;

  int
    id,
    entry;
} ROIMenuEntry;

static DisplayCommand XSelectROIMenuEntry(const ROIMenuEntry *menu_entry,
  char *command,DisplayCommand display_command)
{
  const char
    *const **const Menus = menu_entry->Menus;

  const DisplayCommand
    **const Commands = menu_entry->Commands;

  const int
    id = menu_entry->id,
    entry = menu_entry->entry;

  if (entry >= 0)
    {
      (void) CopyMagickString(command,Menus[id][entry],
        MagickPathExtent);
      display_command=Commands[id][entry];
    }
  return(display_command);
}

#define ApplyMenus  7

typedef struct _ROITargets
{
  int
    *x,
    *y;

  RectangleInfo
    *roi_info;

  size_t
    *state;

  XEvent
    *event;

  DisplayCommand
    *display_command;

  RectangleInfo
    *crop_info;
} ROITargets;

static void XROIButtonPress(const CropEventInput *event_input,
  const RectangleInfo crop_info,const ROITargets *targets)
{
  XWindows
    *const windows = event_input->windows;

  const XEvent
    *const event = event_input->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const roi_info = targets->roi_info;

  size_t
    *const state = targets->state;

  (*x)=windows->image.x;
  (*y)=windows->image.y;
  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != windows->image.id)
    return;
  (*x)=windows->image.x+(*event).xbutton.x;
  (*y)=windows->image.y+(*event).xbutton.y;
  if (IsNearTopLeftCorner((*x),(*y),(*roi_info)) != MagickFalse)
    {
      (*roi_info).x=(*roi_info).x+(int) (*roi_info).width;
      (*roi_info).y=(*roi_info).y+(int) (*roi_info).height;
      (*state)|=UpdateConfigurationState;
      return;
    }
  if (IsNearBottomLeftCorner((*x),(*y),(*roi_info)) != MagickFalse)
    {
      (*roi_info).x=(*roi_info).x+(int) (*roi_info).width;
      (*state)|=UpdateConfigurationState;
      return;
    }
  if (IsNearTopRightCorner((*x),(*y),(*roi_info)) != MagickFalse)
    {
      (*roi_info).y=(*roi_info).y+(int) (*roi_info).height;
      (*state)|=UpdateConfigurationState;
      return;
    }
  if (IsNearBottomRightCorner((*x),(*y),(*roi_info)) != MagickFalse)
    {
      (*state)|=UpdateConfigurationState;
      return;
    }
  XCropButtonRelease(event_input,crop_info);
}

typedef struct _ROIApplyInput
{
  Display
    *display;

  XWindows
    *windows;

  const XEvent
    *event;

  const char
    *const *ApplyMenu;
} ROIApplyInput;

static void XROIButtonRelease(const ROIApplyInput *apply_input,
  const ROITargets *targets)
{
  Display
    *const display = apply_input->display;

  XWindows
    *const windows = apply_input->windows;

  const XEvent
    *const event = apply_input->event;

  const char
    *const *const ApplyMenu = apply_input->ApplyMenu;

  RectangleInfo
    *const roi_info = targets->roi_info;

  size_t
    *const state = targets->state;

  /*
    User has committed to region of interest rectangle.
  */
  (*roi_info).x=(ssize_t) windows->image.x+(*event).xbutton.x;
  (*roi_info).y=(ssize_t) windows->image.y+(*event).xbutton.y;
  XSetCursorState(display,windows,MagickFalse);
  (*state)|=ExitState;
  if (LocaleCompare(windows->command.name,"Apply") == 0)
    return;
  (void) CloneString(&windows->command.name,"Apply");
  windows->command.data=ApplyMenus;
  (void) XCommandWidget(display,windows,ApplyMenu,(XEvent *) NULL);
}

typedef struct _ROIKeyInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const XEvent
    *event;

  Image
    **image;

  ExceptionInfo
    *exception;
} ROIKeyInput;

typedef struct _ROIKeyTargets
{
  RectangleInfo
    *roi_info;

  DisplayCommand
    *display_command;

  size_t
    *state;
} ROIKeyTargets;

static void XROIDownHelpKey(const ROIKeyInput *key_input,
  const ROIKeyTargets *targets,const KeySym key_symbol)
{
  Display
    *const display = key_input->display;

  XWindows
    *const windows = key_input->windows;

  XResourceInfo
    *const resource_info = key_input->resource_info;

  const XEvent
    *const event = key_input->event;

  Image
    **const image = key_input->image;

  ExceptionInfo
    *const exception = key_input->exception;

  RectangleInfo
    *const roi_info = targets->roi_info;

  DisplayCommand
    *const display_command = targets->display_command;

  size_t
    *const state = targets->state;

  switch ((int) key_symbol)
  {
    case XK_Prior:
    case XK_Down:
    case XK_KP_Down:
    {
      (*roi_info).y++;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      (void) XSetFunction(display,windows->image.highlight_context,
        GXcopy);
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Region of Interest",ImageROIHelp);
      }
      (void) XSetFunction(display,windows->image.highlight_context,
        GXinvert);
      break;
    }
    default:
    {
      const ImageWindowKeyInput
        input = { display, resource_info, windows, image, exception };

      (*display_command)=XImageWindowCommand(&input,(*event).xkey.state,
        key_symbol);
      if ((*display_command) != NullCommand)
        (*state)|=UpdateRegionState;
      break;
    }
  }
}

static void XROIUpRightKey(const ROIKeyInput *key_input,
  const ROIKeyTargets *targets,const KeySym key_symbol)
{
  RectangleInfo
    *const roi_info = targets->roi_info;

  switch ((int) key_symbol)
  {
    case XK_Up:
    case XK_KP_Up:
    case XK_Next:
    {
      (*roi_info).y--;
      break;
    }
    case XK_Right:
    case XK_KP_Right:
    {
      (*roi_info).x++;
      break;
    }
    default:
    {
      XROIDownHelpKey(key_input,targets,key_symbol);
      break;
    }
  }
}

static void XROIHomeLeftKey(const ROIKeyInput *key_input,
  const ROIKeyTargets *targets,const KeySym key_symbol)
{
  XWindows
    *const windows = key_input->windows;

  RectangleInfo
    *const roi_info = targets->roi_info;

  switch ((int) key_symbol)
  {
    case XK_Home:
    case XK_KP_Home:
    {
      (*roi_info).x=(ssize_t) (windows->image.width/2L-(*roi_info).width/2L);
      (*roi_info).y=(ssize_t) (windows->image.height/2L-
        (*roi_info).height/2L);
      break;
    }
    case XK_Left:
    case XK_KP_Left:
    {
      (*roi_info).x--;
      break;
    }
    default:
    {
      XROIUpRightKey(key_input,targets,key_symbol);
      break;
    }
  }
}

typedef struct _ROIRegionInput
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

  RectangleInfo
    roi_info;

  DisplayCommand
    display_command;

  const char
    *text;
} ROIRegionInput;

static MagickBooleanType XROIApplyRegion(const ROIRegionInput *region_input,
  RectangleInfo *crop_info)
{
  Display
    *const display = region_input->display;

  XResourceInfo
    *const resource_info = region_input->resource_info;

  XWindows
    *const windows = region_input->windows;

  Image
    **const image = region_input->image;

  ExceptionInfo
    *const exception = region_input->exception;

  const RectangleInfo
    roi_info = region_input->roi_info;

  const DisplayCommand
    display_command = region_input->display_command;

  double
    scale_factor;

  Image
    *roi_image;

  int
    x,
    y;

  MagickProgressMonitor
    progress_monitor;

  unsigned int
    height,
    width;

  /*
    Region of interest is relative to image configuration.
  */
  progress_monitor=SetImageProgressMonitor(*image,
    (MagickProgressMonitor) NULL,(*image)->client_data);
  (*crop_info)=roi_info;
  width=(unsigned int) (*image)->columns;
  height=(unsigned int) (*image)->rows;
  x=0;
  y=0;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,
      &width,&height);
  scale_factor=(double) width/windows->image.ximage->width;
  (*crop_info).x+=x;
  (*crop_info).x=(ssize_t) (scale_factor*(*crop_info).x+0.5);
  (*crop_info).width=(unsigned int) (scale_factor*(*crop_info).width+0.5);
  scale_factor=(double) height/windows->image.ximage->height;
  (*crop_info).y+=y;
  (*crop_info).y=(ssize_t) (scale_factor*(*crop_info).y+0.5);
  (*crop_info).height=(unsigned int) (scale_factor*(*crop_info).height+0.5);
  roi_image=CropImage(*image,crop_info,exception);
  (void) SetImageProgressMonitor(*image,progress_monitor,
    (*image)->client_data);
  if (roi_image == (Image *) NULL)
    return(MagickTrue);
  /*
    Apply image processing technique to the region of interest.
  */
  windows->image.orphan=MagickTrue;
  (void) XMagickCommand(display,resource_info,windows,
    display_command,&roi_image,exception);
  progress_monitor=SetImageProgressMonitor(*image,
    (MagickProgressMonitor) NULL,(*image)->client_data);
  (void) XMagickCommand(display,resource_info,windows,
    SaveToUndoBufferCommand,image,exception);
  windows->image.orphan=MagickFalse;
  (void) CompositeImage(*image,roi_image,CopyCompositeOp,
    MagickTrue,(*crop_info).x,(*crop_info).y,exception);
  roi_image=DestroyImage(roi_image);
  (void) SetImageProgressMonitor(*image,progress_monitor,
    (*image)->client_data);
  return(MagickFalse);
}

static MagickBooleanType XROIUpdateRegion(const ROIRegionInput *region_input,
  size_t *state,RectangleInfo *crop_info)
{
  Display
    *const display = region_input->display;

  XResourceInfo
    *const resource_info = region_input->resource_info;

  XWindows
    *const windows = region_input->windows;

  Image
    **const image = region_input->image;

  ExceptionInfo
    *const exception = region_input->exception;

  const DisplayCommand
    display_command = region_input->display_command;

  const char
    *const text = region_input->text;

  if (((*state) & UpdateRegionState) != 0)
    {
      (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
      switch (display_command)
      {
        case UndoCommand:
        case RedoCommand:
        {
          (void) XMagickCommand(display,resource_info,windows,
            display_command,image,exception);
          break;
        }
        default:
        {
          if (XROIApplyRegion(region_input,crop_info) != MagickFalse)
            return(MagickTrue);
          break;
        }
      }
      if (display_command != InfoCommand)
        {
          XConfigureImageColormap(display,resource_info,windows,*image,
            exception);
          (void) XConfigureImage(display,resource_info,windows,*image,
            exception);
        }
      XCheckRefreshWindows(display,windows);
      XInfoWidget(display,windows,text);
      (void) XSetFunction(display,windows->image.highlight_context,
        GXinvert);
      (*state)&=(unsigned int) (~UpdateRegionState);
    }
  return(MagickFalse);
}

static void XROIMotion(const CropEventInput *event_input,
  const ROITargets *targets)
{
  Display
    *const display = event_input->display;

  XWindows
    *const windows = event_input->windows;

  const XEvent
    *const event = event_input->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const roi_info = targets->roi_info;

  if ((*event).xbutton.window != windows->image.id)
    return;
  /*
    Map and unmap Info widget as text cursor crosses its boundaries.
  */
  (*x)=(*event).xmotion.x;
  (*y)=(*event).xmotion.y;
  XTrackInfoWidget(display,windows,(*x),(*y));
  (*roi_info).x=(ssize_t) windows->image.x+(*event).xmotion.x;
  (*roi_info).y=(ssize_t) windows->image.y+(*event).xmotion.y;
}

static void XROIKeyPress(const ROIKeyInput *key_input,XEvent *event,
  const ROIKeyTargets *targets)
{
  Display
    *const display = key_input->display;

  XWindows
    *const windows = key_input->windows;

  size_t
    *const state = targets->state;

  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  if ((*event).xkey.window != windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  switch ((int) key_symbol)
  {
    case XK_Shift_L:
    case XK_Shift_R:
      break;
    case XK_Escape:
    case XK_F20:
    {
      (*state)|=EscapeState;
      magick_fallthrough;
    }
    case XK_Return:
    {
      (*state)|=ExitState;
      break;
    }
    default:
    {
      XROIHomeLeftKey(key_input,targets,key_symbol);
      break;
    }
  }
  (void) XSetSelectionOwner(display,XA_PRIMARY,windows->image.id,
    (*event).xkey.time);
}

typedef struct _ROICommandInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  RectangleInfo
    highlight_info;

  const char
    *const *ApplyMenu;

  const DisplayCommand
    *ApplyCommands;

  const char
    *const **Menus;

  const DisplayCommand
    **Commands;
} ROICommandInput;

static void XSelectROIPopupEntry(const ROICommandInput *command_input,
  const int id,char *command,DisplayCommand *display_command)
{
  Display
    *const display = command_input->display;

  XWindows
    *const windows = command_input->windows;

  const char
    *const *const ApplyMenu = command_input->ApplyMenu;

  const char
    *const **const Menus = command_input->Menus;

  const DisplayCommand
    **const Commands = command_input->Commands;

  int
    entry;

  ROIMenuEntry
    menu_entry;

  /*
    Select a command from a pop-up menu.
  */
  {
    const XMenuRequest
      menu_request = { ApplyMenu[id], (const char **) Menus[id], command };

    entry=XMenuWidget(display,windows,&menu_request);
  }
  menu_entry.Menus=Menus;
  menu_entry.Commands=Commands;
  menu_entry.id=id;
  menu_entry.entry=entry;
  (*display_command)=XSelectROIMenuEntry(&menu_entry,command,
    (*display_command));
}

static void XROICommand(const ROICommandInput *command_input,XEvent *event,
  DisplayCommand *display_command,size_t *state)
{
  Display
    *const display = command_input->display;

  XResourceInfo
    *const resource_info = command_input->resource_info;

  XWindows
    *const windows = command_input->windows;

  const XHelpView
    view = { display, resource_info, windows, MagickFalse };

  const RectangleInfo
    highlight_info = command_input->highlight_info;

  const char
    *const *const ApplyMenu = command_input->ApplyMenu;

  const DisplayCommand
    *const ApplyCommands = command_input->ApplyCommands;

  char
    command[MagickPathExtent];

  int
    id;

  /*
    Select a command from the Command widget.
  */
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
  (*display_command)=NullCommand;
  id=XCommandWidget(display,windows,ApplyMenu,event);
  if (id >= 0)
    {
      (void) CopyMagickString(command,ApplyMenu[id],MagickPathExtent);
      (*display_command)=ApplyCommands[id];
      if (id < ApplyMenus)
        XSelectROIPopupEntry(command_input,id,command,display_command);
    }
  (void) XSetFunction(display,windows->image.highlight_context,
    GXinvert);
  XHighlightRectangle(display,windows->image.id,
    windows->image.highlight_context,&highlight_info);
  if ((*display_command) == HelpCommand)
    {
      (void) XSetFunction(display,windows->image.highlight_context,
        GXcopy);
      XTextViewHelp(&view,"Help Viewer - Region of Interest",ImageROIHelp);
      (void) XSetFunction(display,windows->image.highlight_context,
        GXinvert);
      return;
    }
  if ((*display_command) == QuitCommand)
    {
      /*
        exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      return;
    }
  if ((*display_command) != NullCommand)
    (*state)|=UpdateRegionState;
}

static const char
  *const ROIMenu[] =
  {
    "Help",
    "Dismiss",
    (char *) NULL
  },
  *const ApplyMenu[] =
  {
    "File",
    "Edit",
    "Transform",
    "Enhance",
    "Effects",
    "F/X",
    "Miscellany",
    "Help",
    "Dismiss",
    (char *) NULL
  },
  *const ROIFileMenu[] =
  {
    "Save...",
    "Print...",
    (char *) NULL
  },
  *const ROIEditMenu[] =
  {
    "Undo",
    "Redo",
    (char *) NULL
  },
  *const ROITransformMenu[] =
  {
    "Flop",
    "Flip",
    "Rotate Right",
    "Rotate Left",
    (char *) NULL
  },
  *const ROIEnhanceMenu[] =
  {
    "Hue...",
    "Saturation...",
    "Brightness...",
    "Gamma...",
    "Spiff",
    "Dull",
    "Contrast Stretch...",
    "Sigmoidal Contrast...",
    "Normalize",
    "Equalize",
    "Negate",
    "Grayscale",
    "Map...",
    "Quantize...",
    (char *) NULL
  },
  *const ROIEffectsMenu[] =
  {
    "Despeckle",
    "Emboss",
    "Reduce Noise",
    "Add Noise",
    "Sharpen...",
    "Blur...",
    "Threshold...",
    "Edge Detect...",
    "Spread...",
    "Shade...",
    "Raise...",
    "Segment...",
    (char *) NULL
  },
  *const ROIFXMenu[] =
  {
    "Solarize...",
    "Sepia Tone...",
    "Swirl...",
    "Implode...",
    "Vignette...",
    "Wave...",
    "Oil Paint...",
    "Charcoal Draw...",
    (char *) NULL
  },
  *const ROIMiscellanyMenu[] =
  {
    "Image Info",
    "Zoom Image",
    "Show Preview...",
    "Show Histogram",
    "Show Matte",
    (char *) NULL
  };

static const char
  *const *ROIMenus[ApplyMenus] =
  {
    ROIFileMenu,
    ROIEditMenu,
    ROITransformMenu,
    ROIEnhanceMenu,
    ROIEffectsMenu,
    ROIFXMenu,
    ROIMiscellanyMenu
  };

static const DisplayCommand
  ApplyCommands[] =
  {
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    HelpCommand,
    QuitCommand
  },
  ROIFileCommands[] =
  {
    SaveCommand,
    PrintCommand
  },
  ROIEditCommands[] =
  {
    UndoCommand,
    RedoCommand
  },
  ROITransformCommands[] =
  {
    FlopCommand,
    FlipCommand,
    RotateRightCommand,
    RotateLeftCommand
  },
  ROIEnhanceCommands[] =
  {
    HueCommand,
    SaturationCommand,
    BrightnessCommand,
    GammaCommand,
    SpiffCommand,
    DullCommand,
    ContrastStretchCommand,
    SigmoidalContrastCommand,
    NormalizeCommand,
    EqualizeCommand,
    NegateCommand,
    GrayscaleCommand,
    MapCommand,
    QuantizeCommand
  },
  ROIEffectsCommands[] =
  {
    DespeckleCommand,
    EmbossCommand,
    ReduceNoiseCommand,
    AddNoiseCommand,
    SharpenCommand,
    BlurCommand,
    ThresholdCommand,
    EdgeDetectCommand,
    SpreadCommand,
    ShadeCommand,
    RaiseCommand,
    SegmentCommand
  },
  ROIFXCommands[] =
  {
    SolarizeCommand,
    SepiaToneCommand,
    SwirlCommand,
    ImplodeCommand,
    VignetteCommand,
    WaveCommand,
    OilPaintCommand,
    CharcoalDrawCommand
  },
  ROIMiscellanyCommands[] =
  {
    InfoCommand,
    ZoomCommand,
    ShowPreviewCommand,
    ShowHistogramCommand,
    ShowMatteCommand
  },
  ROICommands[] =
  {
    ROIHelpCommand,
    ROIDismissCommand
  };

static const DisplayCommand
  *ROIMenuCommands[ApplyMenus] =
  {
    ROIFileCommands,
    ROIEditCommands,
    ROITransformCommands,
    ROIEnhanceCommands,
    ROIEffectsCommands,
    ROIFXCommands,
    ROIMiscellanyCommands
  };

typedef struct _ROIContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Cursor
    cursor;

  ExceptionInfo
    *exception;

  char
    *text;

  Image
    **image;

  ROIKeyTargets
    key_targets;
} ROIContext;

static void XROIPointerCommand(const ROIContext *context,
  const ROITargets *targets)
{
  int
    id;

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  /*
    Select a command from the Command widget.
  */
  id=XCommandWidget(context->display,context->windows,ROIMenu,event);
  if (id < 0)
    return;
  switch (ROICommands[id])
  {
    case ROIHelpCommand:
    {
      {
        const XHelpView view = { context->display, context->resource_info,
          context->windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Region of Interest",ImageROIHelp);
      }
      break;
    }
    case ROIDismissCommand:
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

static void XROIPointerPress(const ROIContext *context,
  const ROITargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  RectangleInfo
    *const roi_info = targets->roi_info;

  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != context->windows->image.id)
    return;
  /*
    Note first corner of region of interest rectangle-- exit loop.
  */
  (void) XCheckDefineCursor(context->display,context->windows->image.id,
    context->cursor);
  (*roi_info).x=(ssize_t) context->windows->image.x+(*event).xbutton.x;
  (*roi_info).y=(ssize_t) context->windows->image.y+(*event).xbutton.y;
  (*state)|=ExitState;
}

static void XROIPointerKey(const ROIContext *context,
  const ROITargets *targets)
{
  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

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
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      {
        const XHelpView view = { context->display, context->resource_info,
          context->windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Region of Interest",ImageROIHelp);
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

static void XROIPointerEvent(const ROIContext *context,
  const ROITargets *targets)
{
  XEvent
    *const event = targets->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const roi_info = targets->roi_info;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XROIPointerPress(context,targets);
      break;
    }
    case ButtonRelease:
      break;
    case Expose:
      break;
    case KeyPress:
    {
      XROIPointerKey(context,targets);
      break;
    }
    case MotionNotify:
    {
      /*
        Map and unmap Info widget as text cursor crosses its boundaries.
      */
      (*x)=(*event).xmotion.x;
      (*y)=(*event).xmotion.y;
      XTrackInfoWidget(context->display,context->windows,(*x),(*y));
      (*roi_info).x=(ssize_t) context->windows->image.x+(*x);
      (*roi_info).y=(ssize_t) context->windows->image.y+(*y);
      break;
    }
    default:
      break;
  }
}

static void XROITrackPointer(const ROIContext *context,
  const ROITargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  RectangleInfo
    *const roi_info = targets->roi_info;

  do
  {
    if (context->windows->info.mapped != MagickFalse)
      {
        /*
          Display pointer position.
        */
        (void) FormatLocaleString(context->text,MagickPathExtent," %+ld%+ld ",
          (long) (*roi_info).x,(long) (*roi_info).y);
        XInfoWidget(context->display,context->windows,context->text);
      }
    /*
      Wait for next event.
    */
    XScreenEvent(context->display,context->windows,event,context->exception);
    if ((*event).xany.window == context->windows->command.id)
      {
        XROIPointerCommand(context,targets);
        continue;
      }
    XROIPointerEvent(context,targets);
  } while (((*state) & ExitState) == 0);
}

static void XROISizeEvent(const ROIContext *context,
  const ROITargets *targets)
{
  XEvent
    *const event = targets->event;

  RectangleInfo
    *const roi_info = targets->roi_info;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      (*roi_info).x=(ssize_t) context->windows->image.x+(*event).xbutton.x;
      (*roi_info).y=(ssize_t) context->windows->image.y+(*event).xbutton.y;
      break;
    }
    case ButtonRelease:
    {
      ROIApplyInput
        apply_input;

      apply_input.display=context->display;
      apply_input.windows=context->windows;
      apply_input.event=(event);
      apply_input.ApplyMenu=ApplyMenu;
      XROIButtonRelease(&apply_input,targets);
      break;
    }
    case Expose:
      break;
    case MotionNotify:
    {
      (*roi_info).x=(ssize_t) context->windows->image.x+(*event).xmotion.x;
      (*roi_info).y=(ssize_t) context->windows->image.y+(*event).xmotion.y;
    }
    default:
      break;
  }
}

static void XROIDrawRegion(const ROIContext *context,const ROITargets *targets,
  RectangleInfo *highlight_info)
{
  RectangleInfo
    *const roi_info = targets->roi_info;

  (*highlight_info)=(*roi_info);
  (*highlight_info).x=(*roi_info).x-context->windows->image.x;
  (*highlight_info).y=(*roi_info).y-context->windows->image.y;
  if (((*highlight_info).width > 3) && ((*highlight_info).height > 3))
    {
      /*
        Display info and draw region of interest rectangle.
      */
      XMapInfoWidget(context->display,context->windows);
      (void) FormatLocaleString(context->text,MagickPathExtent,
        " %.17gx%.17g%+.20g%+.20g",(double) (*roi_info).width,(double)
        (*roi_info).height,(double) (*roi_info).x,(double) (*roi_info).y);
      XInfoWidget(context->display,context->windows,context->text);
      XHighlightRectangle(context->display,context->windows->image.id,
        context->windows->image.highlight_context,highlight_info);
    }
  else
    if (context->windows->info.mapped != MagickFalse)
      (void) XWithdrawWindow(context->display,context->windows->info.id,
        context->windows->info.screen);
}

static void XROISizeRegion(const ROIContext *context,const ROITargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const roi_info = targets->roi_info;

  RectangleInfo
    highlight_info;

  do
  {
    XROIDrawRegion(context,targets,&highlight_info);
    /*
      Wait for next event.
    */
    XScreenEvent(context->display,context->windows,event,context->exception);
    if ((highlight_info.width > 3) && (highlight_info.height > 3))
      XHighlightRectangle(context->display,context->windows->image.id,
        context->windows->image.highlight_context,&highlight_info);
    XROISizeEvent(context,targets);
    if (IsSelectionMovedOrDone((*roi_info),(*x),(*y),(*state)) != MagickFalse)
      XCheckSelectionBounds(context->windows,(*x),(*y),roi_info);
  } while (((*state) & ExitState) == 0);
}

static void XROIWaitCommand(const ROIContext *context,const ROITargets *targets,
  const RectangleInfo highlight_info)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  DisplayCommand
    *const display_command = targets->display_command;

  ROICommandInput
    command_input;

  command_input.display=context->display;
  command_input.resource_info=context->resource_info;
  command_input.windows=context->windows;
  command_input.highlight_info=highlight_info;
  command_input.ApplyMenu=ApplyMenu;
  command_input.ApplyCommands=ApplyCommands;
  command_input.Menus=ROIMenus;
  command_input.Commands=ROIMenuCommands;
  XROICommand(&command_input,event,display_command,state);
}

static void XROIWaitEvent(const ROIContext *context,const ROITargets *targets,
  const RectangleInfo highlight_info)
{
  CropEventInput
    event_input;

  ROIKeyInput
    key_input;

  XEvent
    *const event = targets->event;

  RectangleInfo
    *const roi_info = targets->roi_info;

  RectangleInfo
    *const crop_info = targets->crop_info;

  event_input.display=context->display;
  event_input.windows=context->windows;
  event_input.event=(event);
  event_input.highlight_info=highlight_info;
  event_input.text=context->text;
  key_input.display=context->display;
  key_input.resource_info=context->resource_info;
  key_input.windows=context->windows;
  key_input.event=(event);
  key_input.image=context->image;
  key_input.exception=context->exception;
  switch ((*event).type)
  {
    case ButtonPress:
    {
      XROIButtonPress(&event_input,(*crop_info),targets);
      break;
    }
    case ButtonRelease:
    {
      XCropButtonRelease(&event_input,(*crop_info));
      break;
    }
    case Expose:
    {
      XCropExpose(&event_input,event);
      break;
    }
    case KeyPress:
    {
      XROIKeyPress(&key_input,event,&context->key_targets);
      break;
    }
    case MotionNotify:
    {
      XROIMotion(&event_input,targets);
      break;
    }
    case SelectionRequest:
    {
      XCropSelectionRequest(event,(*roi_info),context->text);
    }
    default:
      break;
  }
}

static MagickBooleanType XROIWaitHead(const ROIContext *context,
  const ROITargets *targets,RectangleInfo *highlight_info)
{
  size_t
    *const state = targets->state;

  RectangleInfo
    *const roi_info = targets->roi_info;

  if (context->windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(context->text,MagickPathExtent,
        " %.17gx%.17g%+.20g%+.20g",(double) (*roi_info).width,(double)
        (*roi_info).height,(double) (*roi_info).x,(double) (*roi_info).y);
      XInfoWidget(context->display,context->windows,context->text);
    }
  (*highlight_info)=(*roi_info);
  (*highlight_info).x=(*roi_info).x-context->windows->image.x;
  (*highlight_info).y=(*roi_info).y-context->windows->image.y;
  if (((*highlight_info).width <= 3) || ((*highlight_info).height <= 3))
    {
      (*state)|=EscapeState;
      (*state)|=ExitState;
      return(MagickFalse);
    }
  return(MagickTrue);
}

#endif
