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
  and-scale and region-of-interest appliers, the command table and the magick
  command, from XApplySaveCommand to XMagickCommand; included by that file
  only.
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
  {
    const XImageCommandInput
      command_input = { display, resource_info, windows, *image, exception };

    status=XSaveImage(&command_input);
  }
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
  {
    const XClientReason
      reason = { windows->im_next_image, CurrentTime };

    XClientMessage(display,windows->image.id,windows->im_protocols,&reason);
  }
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

  const XImageCacheContext
    command_context = { display, resource_info, windows, image, exception };

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

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
  (void) XMagickCommand(&command_context,ApplyCommand);
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
  XConfigureImageColormap(&cmap_target,*image,exception);
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

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  (void) FormatLocaleString(image_geometry,MagickPathExtent,"%dx%d!",
    windows->image.ximage->width,windows->image.ximage->height);
  (void) TransformImage(image,windows->image.crop_geometry,image_geometry,
    exception);
  if (windows->image.crop_geometry != (char *) NULL)
    windows->image.crop_geometry=(char *) RelinquishMagickMemory(
      windows->image.crop_geometry);
  windows->image.x=0;
  windows->image.y=0;
  XConfigureImageColormap(&cmap_target,*image,exception);
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
  const DisplayCommand command)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  switch (command)
  {
    case InfoCommand:
    case SaveToUndoBufferCommand:
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
    case VersionCommand:
    {
      XNoticeWidget(display,windows,GetMagickVersion((size_t *) NULL),
        GetMagickCopyright());
      break;
    }
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
}

typedef struct _XCommandMethod
{
  DisplayCommand
    command;

  void
    (*run)(const XMagickCommandContext *);

  void
    (*run_color)(const XMagickCommandContext *,char *);

  void
    (*run_flags)(const XMagickCommandContext *,MagickStatusType *);

  void
    (*run_quantize)(const XMagickCommandContext *,QuantizeInfo);

  Image
    *(*next_image)(const XMagickCommandContext *,Image *);

  Image
    *(*new_image)(const XMagickCommandContext *);
} XCommandMethod;

static const XCommandMethod
  XCommandMethods[] =
  {
    { OpenCommand, NULL, NULL, NULL, NULL, NULL, XApplyOpenCommand },
    { NextCommand, XApplyNextCommand, NULL, NULL, NULL, NULL, NULL },
    { FormerCommand, XApplyFormerCommand, NULL, NULL, NULL, NULL, NULL },
    { SelectCommand, NULL, NULL, NULL, NULL, NULL, XApplySelectCommand },
    { SaveCommand, XApplySaveCommand, NULL, NULL, NULL, NULL, NULL },
    { PrintCommand, XApplyPrintCommand, NULL, NULL, NULL, NULL, NULL },
    { DeleteCommand, XApplyDeleteCommand, NULL, NULL, NULL, NULL, NULL },
    { NewCommand, NULL, NULL, NULL, NULL, XApplyNewCommand, NULL },
    { VisualDirectoryCommand, NULL, NULL, NULL, NULL, NULL,
      XApplyVisualDirectoryCommand },
    { QuitCommand, XApplyQuitCommand, NULL, NULL, NULL, NULL, NULL },
    { CutCommand, XApplyCutCommand, NULL, NULL, NULL, NULL, NULL },
    { CopyCommand, XApplyCopyCommand, NULL, NULL, NULL, NULL, NULL },
    { PasteCommand, XApplyPasteCommand, NULL, NULL, NULL, NULL, NULL },
    { HalfSizeCommand, XApplyHalfSizeCommand, NULL, NULL, NULL, NULL, NULL },
    { OriginalSizeCommand, XApplyOriginalSizeCommand, NULL, NULL, NULL, NULL,
      NULL },
    { DoubleSizeCommand, XApplyDoubleSizeCommand, NULL, NULL, NULL, NULL,
      NULL },
    { ResizeCommand, XApplyResizeCommand, NULL, NULL, NULL, NULL, NULL },
    { ApplyCommand, XApplyApplyCommand, NULL, NULL, NULL, NULL, NULL },
    { RefreshCommand, XApplyRefreshCommand, NULL, NULL, NULL, NULL, NULL },
    { RestoreCommand, XApplyRestoreCommand, NULL, NULL, NULL, NULL, NULL },
    { CropCommand, XApplyCropCommand, NULL, NULL, NULL, NULL, NULL },
    { ChopCommand, XApplyChopCommand, NULL, NULL, NULL, NULL, NULL },
    { FlopCommand, XApplyFlopCommand, NULL, NULL, NULL, NULL, NULL },
    { FlipCommand, XApplyFlipCommand, NULL, NULL, NULL, NULL, NULL },
    { RotateRightCommand, XApplyRotateRightCommand, NULL, NULL, NULL, NULL,
      NULL },
    { RotateLeftCommand, XApplyRotateLeftCommand, NULL, NULL, NULL, NULL,
      NULL },
    { RotateCommand, XApplyRotateCommand, NULL, NULL, NULL, NULL, NULL },
    { ShearCommand, NULL, XApplyShearCommand, NULL, NULL, NULL, NULL },
    { RollCommand, XApplyRollCommand, NULL, NULL, NULL, NULL, NULL },
    { TrimCommand, XApplyTrimCommand, NULL, NULL, NULL, NULL, NULL },
    { HueCommand, XApplyHueCommand, NULL, NULL, NULL, NULL, NULL },
    { SaturationCommand, XApplySaturationCommand, NULL, NULL, NULL, NULL,
      NULL },
    { BrightnessCommand, XApplyBrightnessCommand, NULL, NULL, NULL, NULL,
      NULL },
    { GammaCommand, XApplyGammaCommand, NULL, NULL, NULL, NULL, NULL },
    { SpiffCommand, XApplySpiffCommand, NULL, NULL, NULL, NULL, NULL },
    { DullCommand, XApplyDullCommand, NULL, NULL, NULL, NULL, NULL },
    { ContrastStretchCommand, XApplyContrastStretchCommand, NULL, NULL, NULL,
      NULL, NULL },
    { SigmoidalContrastCommand, XApplySigmoidalContrastCommand, NULL, NULL,
      NULL, NULL, NULL },
    { NormalizeCommand, XApplyNormalizeCommand, NULL, NULL, NULL, NULL, NULL },
    { EqualizeCommand, XApplyEqualizeCommand, NULL, NULL, NULL, NULL, NULL },
    { NegateCommand, XApplyNegateCommand, NULL, NULL, NULL, NULL, NULL },
    { GrayscaleCommand, XApplyGrayscaleCommand, NULL, NULL, NULL, NULL, NULL },
    { MapCommand, NULL, NULL, NULL, XApplyMapCommand, NULL, NULL },
    { QuantizeCommand, NULL, NULL, NULL, XApplyQuantizeCommand, NULL, NULL },
    { DespeckleCommand, XApplyDespeckleCommand, NULL, NULL, NULL, NULL, NULL },
    { EmbossCommand, XApplyEmbossCommand, NULL, NULL, NULL, NULL, NULL },
    { ReduceNoiseCommand, NULL, NULL, XApplyReduceNoiseCommand, NULL, NULL,
      NULL },
    { AddNoiseCommand, XApplyAddNoiseCommand, NULL, NULL, NULL, NULL, NULL },
    { SharpenCommand, NULL, NULL, XApplySharpenCommand, NULL, NULL, NULL },
    { BlurCommand, NULL, NULL, XApplyBlurCommand, NULL, NULL, NULL },
    { ThresholdCommand, XApplyThresholdCommand, NULL, NULL, NULL, NULL, NULL },
    { EdgeDetectCommand, NULL, NULL, XApplyEdgeDetectCommand, NULL, NULL,
      NULL },
    { SpreadCommand, NULL, NULL, XApplySpreadCommand, NULL, NULL, NULL },
    { ShadeCommand, XApplyShadeCommand, NULL, NULL, NULL, NULL, NULL },
    { RaiseCommand, XApplyRaiseCommand, NULL, NULL, NULL, NULL, NULL },
    { SegmentCommand, XApplySegmentCommand, NULL, NULL, NULL, NULL, NULL },
    { SepiaToneCommand, XApplySepiaToneCommand, NULL, NULL, NULL, NULL, NULL },
    { SolarizeCommand, XApplySolarizeCommand, NULL, NULL, NULL, NULL, NULL },
    { SwirlCommand, NULL, NULL, XApplySwirlCommand, NULL, NULL, NULL },
    { ImplodeCommand, NULL, NULL, XApplyImplodeCommand, NULL, NULL, NULL },
    { VignetteCommand, XApplyVignetteCommand, NULL, NULL, NULL, NULL, NULL },
    { WaveCommand, XApplyWaveCommand, NULL, NULL, NULL, NULL, NULL },
    { OilPaintCommand, NULL, NULL, XApplyOilPaintCommand, NULL, NULL, NULL },
    { CharcoalDrawCommand, XApplyCharcoalDrawCommand, NULL, NULL, NULL, NULL,
      NULL },
    { AnnotateCommand, XApplyAnnotateCommand, NULL, NULL, NULL, NULL, NULL },
    { DrawCommand, XApplyDrawCommand, NULL, NULL, NULL, NULL, NULL },
    { ColorCommand, XApplyColorCommand, NULL, NULL, NULL, NULL, NULL },
    { MatteCommand, XApplyMatteCommand, NULL, NULL, NULL, NULL, NULL },
    { CompositeCommand, XApplyCompositeCommand, NULL, NULL, NULL, NULL, NULL },
    { AddBorderCommand, NULL, XApplyAddBorderCommand, NULL, NULL, NULL, NULL },
    { AddFrameCommand, NULL, XApplyAddFrameCommand, NULL, NULL, NULL, NULL },
    { CommentCommand, XApplyCommentCommand, NULL, NULL, NULL, NULL, NULL },
    { LaunchCommand, NULL, NULL, NULL, NULL, XApplyLaunchCommand, NULL },
    { RegionOfInterestCommand, XApplyRegionOfInterestCommand, NULL, NULL, NULL,
      NULL, NULL },
    { ZoomCommand, XApplyZoomCommand, NULL, NULL, NULL, NULL, NULL },
    { ShowPreviewCommand, XApplyShowPreviewCommand, NULL, NULL, NULL, NULL,
      NULL },
    { ShowHistogramCommand, XApplyShowHistogramCommand, NULL, NULL, NULL, NULL,
      NULL },
    { ShowMatteCommand, XApplyShowMatteCommand, NULL, NULL, NULL, NULL, NULL },
    { BackgroundCommand, NULL, NULL, NULL, NULL, XApplyBackgroundCommand,
      NULL },
    { SlideShowCommand, XApplySlideShowCommand, NULL, NULL, NULL, NULL, NULL },
    { PreferencesCommand, NULL, NULL, NULL, NULL, XApplyPreferencesCommand,
      NULL },
    { BrowseDocumentationCommand, XApplyBrowseDocumentationCommand, NULL, NULL,
      NULL, NULL, NULL }
  };

static const XCommandMethod *GetXCommandMethod(const DisplayCommand command)
{
  size_t
    i;

  for (i=0; i < (sizeof(XCommandMethods)/sizeof(*XCommandMethods)); i++)
    if (XCommandMethods[i].command == command)
      return(XCommandMethods+i);
  return((const XCommandMethod *) NULL);
}

static void XRunCommandMethod(const XCommandMethod *method,
  const XMagickCommandContext *context,MagickStatusType *flags,Image **nexus)
{
  if (method->run != NULL)
    method->run(context);
  else if (method->run_color != NULL)
    method->run_color(context,context->color);
  else if (method->run_flags != NULL)
    method->run_flags(context,flags);
  else if (method->run_quantize != NULL)
    method->run_quantize(context,*context->quantize_info);
  else if (method->next_image != NULL)
    *nexus=method->next_image(context,*nexus);
  else
    *nexus=method->new_image(context);
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
static Image *XMagickCommand(const XImageCacheContext *target,
  const DisplayCommand command)
{
  Display
    *const display = target->display;

  XResourceInfo
    *const resource_info = target->resource_info;

  XWindows
    *const windows = target->windows;

  Image
    **const image = target->image;

  ExceptionInfo
    *const exception = target->exception;

  const XCommandMethod
    *method;

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
  method=GetXCommandMethod(command);
  if (method != (const XCommandMethod *) NULL)
    XRunCommandMethod(method,&context,&flags,&nexus);
  else
    XApplySessionCommands(&context,command);
  image_info=DestroyImageInfo(image_info);
  return(nexus);
}

#endif
