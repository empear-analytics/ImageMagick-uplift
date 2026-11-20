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

  MagickCore private methods of display-image.c: the save, new, shear, crop-
  and-scale and region-of-interest appliers, the command dispatch, the magick
  command and the magnify handlers, from XApplySaveCommand to
  XMoveMagnifyRightDown; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_APPLY_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_APPLY_PRIVATE_H

static void XApplySaveCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  /*
    Save image.
  */
  status=XSaveImage(display,resource_info,windows,*image,exception);
  if (status == MagickFalse)
    {
      char
        message[MagickPathExtent];

      (void) FormatLocaleString(message,MagickPathExtent,"%s:%s",
        exception->reason != (char *) NULL ? exception->reason : "",
        exception->description != (char *) NULL ? exception->description :
        "");
      XNoticeWidget(display,windows,"Unable to save file:",message);
      return;
    }
}

static Image *XApplyNewCommand(const XMagickCommandContext *context,
  Image *nexus)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  ImageInfo
    *const image_info = context->image_info;

  ExceptionInfo
    *const exception = context->exception;

  int
    status;

  static char
    color[MagickPathExtent] = "gray",
    geometry[MagickPathExtent] = "640x480";

  static const char
    *format = "gradient";

  /*
    Query user for canvas geometry.
  */
  {
    const XDialogRequest
      dialog_request = { "New", "Enter image geometry:", geometry };

    status=XDialogWidget(display,windows,&dialog_request);
  }
  if (*geometry == '\0')
    return(nexus);
  if (status == 0)
    format="xc";
  XColorBrowserWidget(display,windows,"Select",color);
  if (*color == '\0')
    return(nexus);
  /*
    Create canvas.
  */
  (void) FormatLocaleString(image_info->filename,MagickPathExtent,
    "%s:%s",format,color);
  (void) CloneString(&image_info->size,geometry);
  nexus=ReadImage(image_info,exception);
  CatchException(exception);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_next_image,CurrentTime);
  return(nexus);
}

static void XApplyShearCommand(const XMagickCommandContext *context,char *color)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  Image
    *shear_image;

  static char
    geometry[MagickPathExtent] = "45.0x45.0";

  /*
    Query user for shear color and geometry.
  */
  XColorBrowserWidget(display,windows,"Select",color);
  if (*color == '\0')
    return;
  {
    const XDialogRequest
      dialog_request = { "Shear", "Enter shear geometry:", geometry };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*geometry == '\0')
    return;
  /*
    Shear image.
  */
  (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
    exception);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) QueryColorCompliance(color,AllCompliance,
    &(*image)->background_color,exception);
  flags=ParseGeometry(geometry,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=geometry_info.rho;
  shear_image=ShearImage(*image,geometry_info.rho,geometry_info.sigma,
    exception);
  if (shear_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=shear_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  windows->image.window_changes.width=(int) (*image)->columns;
  windows->image.window_changes.height=(int) (*image)->rows;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyCropAndScale(char *image_geometry,
  const XMagickCommandContext *context)
{
  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  ExceptionInfo
    *const exception = context->exception;

  (void) FormatLocaleString(image_geometry,MagickPathExtent,"%dx%d!",
    windows->image.ximage->width,windows->image.ximage->height);
  (void) TransformImage(image,windows->image.crop_geometry,image_geometry,
    exception);
  if (windows->image.crop_geometry != (char *) NULL)
    windows->image.crop_geometry=(char *) RelinquishMagickMemory(
      windows->image.crop_geometry);
  windows->image.x=0;
  windows->image.y=0;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyApplyCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  char
    image_geometry[MagickPathExtent];

  if (IsImageWindowUncropped(windows,image) != MagickFalse)
    return;
  /*
    Apply size transforms to image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  /*
    Crop and/or scale displayed image.
  */
  XApplyCropAndScale(image_geometry,context);
}

static void XApplyRegionOfInterestCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  /*
    Apply an image processing technique to a region of interest.
  */
  (void) XROIImage(display,resource_info,windows,image,exception);
}

static void XApplySessionCommands(const XMagickCommandContext *context,
  const DisplayCommand command,Image **nexus)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  switch (command)
  {
    case SlideShowCommand:
      XApplySlideShowCommand(context);
      break;
    case PreferencesCommand:
      (*nexus)=XApplyPreferencesCommand(context,(*nexus));
      break;
    case HelpCommand:
    {
      /*
        User requested help.
      */
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Display",DisplayHelp);
      }
      break;
    }
    case BrowseDocumentationCommand:
      XApplyBrowseDocumentationCommand(context);
      break;
    case VersionCommand:
    {
      XNoticeWidget(display,windows,GetMagickVersion((size_t *) NULL),
        GetMagickCopyright());
      break;
    }
    case SaveToUndoBufferCommand:
      break;
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
}

static void XApplyMiscellanyCommands(const XMagickCommandContext *context,
  const DisplayCommand command,Image **nexus)
{
  switch (command)
  {
    case InfoCommand:
      break;
    case ZoomCommand:
      XApplyZoomCommand(context);
      break;
    case ShowPreviewCommand:
      XApplyShowPreviewCommand(context);
      break;
    case ShowHistogramCommand:
      XApplyShowHistogramCommand(context);
      break;
    case ShowMatteCommand:
      XApplyShowMatteCommand(context);
      break;
    case BackgroundCommand:
      (*nexus)=XApplyBackgroundCommand(context,(*nexus));
      break;
    default:
    {
      XApplySessionCommands(context,command,&(*nexus));
      break;
    }
  }
}

static void XApplyImageEditCommands(const XMagickCommandContext *context,
  const DisplayCommand command,Image **nexus)
{
  char
    *const color = context->color;

  switch (command)
  {
    case CompositeCommand:
      XApplyCompositeCommand(context);
      break;
    case AddBorderCommand:
      XApplyAddBorderCommand(context,color);
      break;
    case AddFrameCommand:
      XApplyAddFrameCommand(context,color);
      break;
    case CommentCommand:
      XApplyCommentCommand(context);
      break;
    case LaunchCommand:
      (*nexus)=XApplyLaunchCommand(context,(*nexus));
      break;
    case RegionOfInterestCommand:
      XApplyRegionOfInterestCommand(context);
      break;
    default:
    {
      XApplyMiscellanyCommands(context,command,&(*nexus));
      break;
    }
  }
}

static void XApplyPaintCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case OilPaintCommand:
      XApplyOilPaintCommand(context,&(*flags));
      break;
    case CharcoalDrawCommand:
      XApplyCharcoalDrawCommand(context);
      break;
    case AnnotateCommand:
      XApplyAnnotateCommand(context);
      break;
    case DrawCommand:
      XApplyDrawCommand(context);
      break;
    case ColorCommand:
      XApplyColorCommand(context);
      break;
    case MatteCommand:
      XApplyMatteCommand(context);
      break;
    default:
    {
      XApplyImageEditCommands(context,command,&(*nexus));
      break;
    }
  }
}

static void XApplyFXCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case SepiaToneCommand:
      XApplySepiaToneCommand(context);
      break;
    case SolarizeCommand:
      XApplySolarizeCommand(context);
      break;
    case SwirlCommand:
      XApplySwirlCommand(context,&(*flags));
      break;
    case ImplodeCommand:
      XApplyImplodeCommand(context,&(*flags));
      break;
    case VignetteCommand:
      XApplyVignetteCommand(context);
      break;
    case WaveCommand:
      XApplyWaveCommand(context);
      break;
    default:
    {
      XApplyPaintCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyEffectsCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case ThresholdCommand:
      XApplyThresholdCommand(context);
      break;
    case EdgeDetectCommand:
      XApplyEdgeDetectCommand(context,&(*flags));
      break;
    case SpreadCommand:
      XApplySpreadCommand(context,&(*flags));
      break;
    case ShadeCommand:
      XApplyShadeCommand(context);
      break;
    case RaiseCommand:
      XApplyRaiseCommand(context);
      break;
    case SegmentCommand:
      XApplySegmentCommand(context);
      break;
    default:
    {
      XApplyFXCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyFilterCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case DespeckleCommand:
      XApplyDespeckleCommand(context);
      break;
    case EmbossCommand:
      XApplyEmbossCommand(context);
      break;
    case ReduceNoiseCommand:
      XApplyReduceNoiseCommand(context,&(*flags));
      break;
    case AddNoiseCommand:
      XApplyAddNoiseCommand(context);
      break;
    case SharpenCommand:
      XApplySharpenCommand(context,&(*flags));
      break;
    case BlurCommand:
      XApplyBlurCommand(context,&(*flags));
      break;
    default:
    {
      XApplyEffectsCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyColorMapCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  QuantizeInfo
    *const quantize_info = context->quantize_info;

  switch (command)
  {
    case NegateCommand:
      XApplyNegateCommand(context);
      break;
    case GrayscaleCommand:
      XApplyGrayscaleCommand(context);
      break;
    case MapCommand:
      XApplyMapCommand(context,(*quantize_info));
      break;
    case QuantizeCommand:
      XApplyQuantizeCommand(context,(*quantize_info));
      break;
    default:
    {
      XApplyFilterCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyContrastCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case SpiffCommand:
      XApplySpiffCommand(context);
      break;
    case DullCommand:
      XApplyDullCommand(context);
      break;
    case ContrastStretchCommand:
      XApplyContrastStretchCommand(context);
      break;
    case SigmoidalContrastCommand:
      XApplySigmoidalContrastCommand(context);
      break;
    case NormalizeCommand:
      XApplyNormalizeCommand(context);
      break;
    case EqualizeCommand:
      XApplyEqualizeCommand(context);
      break;
    default:
    {
      XApplyColorMapCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyToneCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case HueCommand:
      XApplyHueCommand(context);
      break;
    case SaturationCommand:
      XApplySaturationCommand(context);
      break;
    case BrightnessCommand:
      XApplyBrightnessCommand(context);
      break;
    case GammaCommand:
      XApplyGammaCommand(context);
      break;
    default:
    {
      XApplyContrastCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyTransformCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  char
    *const color = context->color;

  switch (command)
  {
    case RotateRightCommand:
      XApplyRotateRightCommand(context);
      break;
    case RotateLeftCommand:
      XApplyRotateLeftCommand(context);
      break;
    case RotateCommand:
      XApplyRotateCommand(context);
      break;
    case ShearCommand:
      XApplyShearCommand(context,color);
      break;
    case RollCommand:
      XApplyRollCommand(context);
      break;
    case TrimCommand:
      XApplyTrimCommand(context);
      break;
    default:
    {
      XApplyToneCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyCropFlipCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case CropCommand:
      XApplyCropCommand(context);
      break;
    case ChopCommand:
      XApplyChopCommand(context);
      break;
    case FlopCommand:
      XApplyFlopCommand(context);
      break;
    case FlipCommand:
      XApplyFlipCommand(context);
      break;
    default:
    {
      XApplyTransformCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyViewCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case ApplyCommand:
      XApplyApplyCommand(context);
      break;
    case RefreshCommand:
      XApplyRefreshCommand(context);
      break;
    case RestoreCommand:
      XApplyRestoreCommand(context);
      break;
    default:
    {
      XApplyCropFlipCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyResizeCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case HalfSizeCommand:
      XApplyHalfSizeCommand(context);
      break;
    case OriginalSizeCommand:
      XApplyOriginalSizeCommand(context);
      break;
    case DoubleSizeCommand:
      XApplyDoubleSizeCommand(context);
      break;
    case ResizeCommand:
      XApplyResizeCommand(context);
      break;
    default:
    {
      XApplyViewCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyEditCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case CutCommand:
      XApplyCutCommand(context);
      break;
    case CopyCommand:
      XApplyCopyCommand(context);
      break;
    case PasteCommand:
      XApplyPasteCommand(context);
      break;
    default:
    {
      XApplyResizeCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyFileCommands(const XMagickCommandContext *context,
  const DisplayCommand command,Image **nexus,MagickStatusType *flags)
{
  switch (command)
  {
    case SaveCommand:
      XApplySaveCommand(context);
      break;
    case PrintCommand:
      XApplyPrintCommand(context);
      break;
    case DeleteCommand:
      XApplyDeleteCommand(context);
      break;
    case NewCommand:
      (*nexus)=XApplyNewCommand(context,(*nexus));
      break;
    case VisualDirectoryCommand:
      (*nexus)=XApplyVisualDirectoryCommand(context);
      break;
    case QuitCommand:
      XApplyQuitCommand(context);
      break;
    default:
    {
      XApplyEditCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X M a g i c k C o m m a n d                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMagickCommand() makes a transform to the image or Image window as
%  specified by a user menu button or keyboard command.
%
%  The format of the XMagickCommand method is:
%
%      Image *XMagickCommand(Display *display,XResourceInfo *resource_info,
%        XWindows *windows,const DisplayCommand command,Image **image,
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
%    o command: Specifies a command to perform.
%
%    o image: the image;  XMagickCommand may transform the image and return a
%      new image pointer.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *XMagickCommand(Display *display,XResourceInfo *resource_info,
  XWindows *windows,const DisplayCommand command,Image **image,
  ExceptionInfo *exception)
{
  GeometryInfo
    geometry_info;

  Image
    *nexus;

  ImageInfo
    *image_info;

  MagickStatusType
    flags;

  QuantizeInfo
    quantize_info;

  static char
    color[MagickPathExtent] = "gray";

  XMagickCommandContext
    context;

  const XImageCacheContext
    cache_context = { display, resource_info, windows, image, exception };

  /*
    Process user command.
  */
  XCheckRefreshWindows(display,windows);
  XImageCache(&cache_context,command);
  nexus=NewImageList();
  windows->image.window_changes.width=windows->image.ximage->width;
  windows->image.window_changes.height=windows->image.ximage->height;
  image_info=CloneImageInfo(resource_info->image_info);
  SetGeometryInfo(&geometry_info);
  GetQuantizeInfo(&quantize_info);
  context.display=display;
  context.resource_info=resource_info;
  context.windows=windows;
  context.image=image;
  context.image_info=image_info;
  context.exception=exception;
  context.color=color;
  context.quantize_info=&quantize_info;
  switch (command)
  {
    case OpenCommand:
      nexus=XApplyOpenCommand(&context);
      break;
    case NextCommand:
      XApplyNextCommand(&context);
      break;
    case FormerCommand:
      XApplyFormerCommand(&context);
      break;
    case SelectCommand:
      nexus=XApplySelectCommand(&context);
      break;
    default:
    {
      XApplyFileCommands(&context,command,&nexus,&flags);
      break;
    }
  }
  image_info=DestroyImageInfo(image_info);
  return(nexus);
}

static void XUpdateMagnifyInfo(Display *display,XWindows *windows,const int x,
  const int y)
{
  char
    text[MagickPathExtent];

  if (windows->info.mapped != MagickFalse)
    {
      if ((x < (windows->info.x+(int) windows->info.width)) &&
          (y < (windows->info.y+(int) windows->info.height)))
        (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
    }
  else
    if ((x > (windows->info.x+(int) windows->info.width)) ||
        (y > (windows->info.y+(int) windows->info.height)))
      (void) XMapWindow(display,windows->info.id);
  if (windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(text,MagickPathExtent," %+d%+d ",
        windows->magnify.x,windows->magnify.y);
      XInfoWidget(display,windows,text);
    }
}

static void XClampMagnifyPosition(XWindows *windows,int *x,int *y)
{
  if ((*x) < 0)
    (*x)=0;
  else
    if ((*x) >= (int) windows->image.width)
      (*x)=(int) windows->image.width-1;
  if ((*y) < 0)
    (*y)=0;
  else
   if ((*y) >= (int) windows->image.height)
     (*y)=(int) windows->image.height-1;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X M a g n i f y I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMagnifyImage() magnifies portions of the image as indicated by the pointer.
%  The magnified portion is displayed in a separate window.
%
%  The format of the XMagnifyImage method is:
%
%      void XMagnifyImage(Display *display,XWindows *windows,XEvent *event,
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
static void XTrackMagnifyButton(Display *display,XWindows *windows,
  XEvent *event,ExceptionInfo *exception)
{
  int
    x,
    y;

  size_t
    state;

  (void) XCheckDefineCursor(display,windows->image.id,windows->magnify.cursor);
  state=DefaultState;
  x=event->xbutton.x;
  y=event->xbutton.y;
  windows->magnify.x=(int) windows->image.x+x;
  windows->magnify.y=(int) windows->image.y+y;
  do
  {
    /*
      Map and unmap Info widget as text cursor crosses its boundaries.
    */
    XUpdateMagnifyInfo(display,windows,x,y);
    /*
      Wait for next event.
    */
    XScreenEvent(display,windows,event,exception);
    switch (event->type)
    {
      case ButtonPress:
        break;
      case ButtonRelease:
      {
        /*
          User has finished magnifying image.
        */
        x=event->xbutton.x;
        y=event->xbutton.y;
        state|=ExitState;
        break;
      }
      case Expose:
        break;
      case MotionNotify:
      {
        x=event->xmotion.x;
        y=event->xmotion.y;
        break;
      }
      default:
        break;
    }
    /*
      Check boundary conditions.
    */
    XClampMagnifyPosition(windows,&x,&y);
  } while ((state & ExitState) == 0);
}

static void XMagnifyImage(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  /*
    Update magnified image until the mouse button is released.
  */
  XTrackMagnifyButton(display,windows,event,exception);
  /*
    Display magnified image.
  */
  XSetCursorState(display,windows,MagickFalse);
}
typedef struct _MagnifyDigitKey
{
  int
    key,
    base;
} MagnifyDigitKey;

static const MagnifyDigitKey
  MagnifyDigitKeys[] =
  {
    { XK_0, XK_0 },
    { XK_1, XK_0 },
    { XK_2, XK_0 },
    { XK_3, XK_0 },
    { XK_4, XK_0 },
    { XK_5, XK_0 },
    { XK_6, XK_0 },
    { XK_7, XK_0 },
    { XK_8, XK_0 },
    { XK_9, XK_0 },
    { XK_KP_0, XK_KP_0 },
    { XK_KP_1, XK_KP_0 },
    { XK_KP_2, XK_KP_0 },
    { XK_KP_3, XK_KP_0 },
    { XK_KP_4, XK_KP_0 },
    { XK_KP_5, XK_KP_0 },
    { XK_KP_6, XK_KP_0 },
    { XK_KP_7, XK_KP_0 },
    { XK_KP_8, XK_KP_0 },
    { XK_KP_9, XK_KP_0 }
  };

static void XSelectMagnifyDigit(XWindows *windows,const KeySym key_symbol)
{
  const int
    key = (int) key_symbol;

  size_t
    i;

  for (i=0; i < (sizeof(MagnifyDigitKeys)/sizeof(*MagnifyDigitKeys)); i++)
    if (MagnifyDigitKeys[i].key == key)
      {
        windows->magnify.data=(key_symbol-MagnifyDigitKeys[i].base);
        break;
      }
}

static void XMoveMagnifyRightDown(XWindows *windows,const unsigned int quantum,
  const KeySym key_symbol)
{
  switch ((int) key_symbol)
  {
    case XK_Right:
    case XK_KP_Right:
    {
      if (windows->magnify.x < (int) (windows->image.ximage->width-1))
        windows->magnify.x+=(int) quantum;
      break;
    }
    case XK_Down:
    case XK_KP_Down:
    {
      if (windows->magnify.y < (int) (windows->image.ximage->height-1))
        windows->magnify.y+=(int) quantum;
      break;
    }
    default:
    {
      XSelectMagnifyDigit(windows,key_symbol);
      break;
    }
  }
}

#endif
