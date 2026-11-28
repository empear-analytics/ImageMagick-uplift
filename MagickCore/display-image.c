/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               DDDD   IIIII  SSSSS  PPPP   L       AAA   Y   Y               %
%               D   D    I    SS     P   P  L      A   A   Y Y                %
%               D   D    I     SSS   PPPP   L      AAAAA    Y                 %
%               D   D    I       SS  P      L      A   A    Y                 %
%               DDDD   IIIII  SSSSS  P      LLLLL  A   A    Y                 %
%                                                                             %
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%                                                                             %
%                  MagickCore X Window Image Display Methods                  %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                                July 1992                                    %
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
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/composite.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/delegate.h"
#include "MagickCore/display.h"
#include "MagickCore/display-private.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/locale-private.h"
#include "MagickCore/log.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/shear.h"
#include "MagickCore/segment.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/visual-effects.h"
#include "MagickCore/widget.h"
#include "MagickCore/widget-private.h"
#include "MagickCore/xwindow.h"
#include "MagickCore/xwindow-private.h"
#if defined(MAGICKCORE_X11_DELEGATE)

#include "MagickCore/display-image-annotate-chop-private.h"
#include "MagickCore/display-image-annotate-private.h"

#include "MagickCore/display-image-composite-crop-private.h"
#include "MagickCore/display-image-crop-private.h"

#include "MagickCore/display-image-draw-undo-private.h"
#include "MagickCore/display-image-draw-private.h"

#include "MagickCore/display-image-apply-filters-private.h"
#include "MagickCore/display-image-apply-effects-private.h"
#include "MagickCore/display-image-apply-private.h"

#include "MagickCore/display-image-matte-roi-private.h"
#include "MagickCore/display-image-matte-private.h"

#include "MagickCore/display-image-save-private.h"
#include "MagickCore/display-image-screen-button-private.h"
#include "MagickCore/display-image-screen-configure-private.h"
#include "MagickCore/display-image-screen-event-private.h"
#include "MagickCore/display-image-tile-load-private.h"
#include "MagickCore/display-image-tile-private.h"


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X D i s p l a y B a c k g r o u n d I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDisplayBackgroundImage() displays an image in the background of a window.
%
%  The format of the XDisplayBackgroundImage method is:
%
%      MagickBooleanType XDisplayBackgroundImage(Display *display,
%        XResourceInfo *resource_info,Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType XDisplayBackgroundImage(Display *display,
  XResourceInfo *resource_info,Image *image,ExceptionInfo *exception)
{
  char
    visual_type[MagickPathExtent];

  int
    height,
    width;

  static XPixelInfo
    pixel;

  static XStandardColormap
    *map_info;

  static XVisualInfo
    *visual_info = (XVisualInfo *) NULL;

  static XWindowInfo
    window_info;

  Window
    root_window;

  XResourceInfo
    resources;

  BackgroundInput
    input;

  const BackgroundTargets
    targets = { &map_info, &pixel, &resources, &visual_info, &window_info };

  XWindowAttributes
    window_attributes;

  /*
    Determine target window.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  resources=(*resource_info);
  window_info.id=(Window) NULL;
  root_window=XRootWindow(display,XDefaultScreen(display));
  if (XFindBackgroundWindow(display,resources.window_id,root_window,
      &window_info) == MagickFalse)
    return(MagickFalse);
  /*
    Determine window visual id.
  */
  XGetBackgroundVisualType(display,&window_info,&window_attributes,visual_type);
  if (visual_info == (XVisualInfo *) NULL)
    XInitializeBackgroundVisual(display,image,visual_type,&targets);
  /*
    Free previous root colors.
  */
  if (window_info.id == root_window)
    (void) XDestroyWindowColors(display,root_window);
  XCreateBackgroundWindowInfo(display,image,exception,&targets);
  /*
    Create the X image.
  */
  XSizeBackgroundWindow(image,&window_info);
  XFitBackgroundWindow(&window_attributes,&window_info);
  XMakeBackgroundXImage(display,image,exception,&targets);
  if (resource_info->debug != MagickFalse)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "Image: %s[%.17g] %.17gx%.17g ",image->filename,(double) image->scene,
        (double) image->columns,(double) image->rows);
      if (image->colors != 0)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"%.17gc ",(double)
          image->colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"%s",image->magick);
    }
  /*
    Adjust image dimensions as specified by backdrop or geometry options.
  */
  input.display=display;
  input.image=image;
  input.window_attributes=(&window_attributes);
  XAdjustBackgroundSize(&input,&targets,&width,&height);
  XDisplayBackgroundPixmap(&input,&targets,width,height);
  return((MagickBooleanType) (window_info.id == root_window));
}

#include "MagickCore/display-image-window-setup-private.h"
#include "MagickCore/display-image-window-private.h"

static DisplayCommand
  CommandMenus[] =
  {
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
  },
  FileCommands[] =
  {
    OpenCommand,
    NextCommand,
    FormerCommand,
    SelectCommand,
    SaveCommand,
    PrintCommand,
    DeleteCommand,
    NewCommand,
    VisualDirectoryCommand,
    QuitCommand
  },
  EditCommands[] =
  {
    UndoCommand,
    RedoCommand,
    CutCommand,
    CopyCommand,
    PasteCommand
  },
  ViewCommands[] =
  {
    HalfSizeCommand,
    OriginalSizeCommand,
    DoubleSizeCommand,
    ResizeCommand,
    ApplyCommand,
    RefreshCommand,
    RestoreCommand
  },
  TransformCommands[] =
  {
    CropCommand,
    ChopCommand,
    FlopCommand,
    FlipCommand,
    RotateRightCommand,
    RotateLeftCommand,
    RotateCommand,
    ShearCommand,
    RollCommand,
    TrimCommand
  },
  EnhanceCommands[] =
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
  EffectsCommands[] =
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
  FXCommands[] =
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
  ImageEditCommands[] =
  {
    AnnotateCommand,
    DrawCommand,
    ColorCommand,
    MatteCommand,
    CompositeCommand,
    AddBorderCommand,
    AddFrameCommand,
    CommentCommand,
    LaunchCommand,
    RegionOfInterestCommand
  },
  MiscellanyCommands[] =
  {
    InfoCommand,
    ZoomCommand,
    ShowPreviewCommand,
    ShowHistogramCommand,
    ShowMatteCommand,
    BackgroundCommand,
    SlideShowCommand,
    PreferencesCommand
  },
  HelpCommands[] =
  {
    HelpCommand,
    BrowseDocumentationCommand,
    VersionCommand
  },
  ShortCutsCommands[] =
  {
    NextCommand,
    FormerCommand,
    OpenCommand,
    SaveCommand,
    PrintCommand,
    UndoCommand,
    RestoreCommand,
    InfoCommand,
    QuitCommand
  },
  VirtualCommands[] =
  {
    InfoCommand,
    PrintCommand,
    NextCommand,
    QuitCommand
  };

static DisplayCommand
  *Commands[MagickMenus] =
  {
    FileCommands,
    EditCommands,
    ViewCommands,
    TransformCommands,
    EnhanceCommands,
    EffectsCommands,
    FXCommands,
    ImageEditCommands,
    MiscellanyCommands,
    HelpCommands
  };

#include "MagickCore/display-image-event-private.h"


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D i s p l a y I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDisplayImage() displays an image via X11.  A new image is created and
%  returned if the user interactively transforms the displayed image.
%
%  The format of the XDisplayImage method is:
%
%      Image *XDisplayImage(Display *display,XResourceInfo *resource_info,
%        char **argv,int argc,Image **image,size_t *state,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o nexus:  Method XDisplayImage returns an image when the
%      user chooses 'Open Image' from the command menu or picks a tile
%      from the image directory.  Otherwise a null image is returned.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o argv: Specifies the application's argument list.
%
%    o argc: Specifies the number of arguments.
%
%    o image: Specifies an address to an address of an Image structure;
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *XDisplayImage(Display *display,XResourceInfo *resource_info,
  char **argv,int argc,Image **image,size_t *state,ExceptionInfo *exception)
{
  char
    command[MagickPathExtent];

  Image
    *display_image,
    *nexus;

  MagickStatusType
    status;

  static char
    working_directory[MagickPathExtent];

  static XPoint
    vid_info;

  static XWindowInfo
    *magick_windows[MaxXWindows];

  static unsigned int
    number_windows;

  time_t
    timer;

  WarningHandler
    warning_handler;

  XEvent
    event;

  XGCValues
    context_values;

  XWindows
    *windows;

  DisplayEventContext
    event_context;

  const DisplayEventTargets
    event_targets = { &nexus, &display_image, &event, &vid_info, &timer,
      &context_values, &status, &warning_handler, &windows, &number_windows };

  assert(image != (Image **) NULL);
  assert((*image)->signature == MagickCoreSignature);
  XTraceDisplayImage(image);
  display_image=(*image);
  {
    const DisplayImageInput input = { display, resource_info, exception, image,
      working_directory, magick_windows, state, command, argv, argc };

    XInitializeDisplayImage(&input,&event_context,&event_targets);
  }
  do
  {
    /*
      Handle a window event.
    */
    if (XNextDisplayEvent(&event_context,&event_targets) != MagickFalse)
      continue;
    switch (event.type)
    {
      case KeyPress:
        XLookupKeyPress(&event_context,&event_targets,command,
          sizeof(command));
        break;
      case KeyRelease:
        XLookupKeyRelease(resource_info,&event,command,sizeof(command));
        break;
      default:
      {
        XDispatchButtonEvent(&event_context,&event_targets);
        break;
      }
    }
  } while (!(*state & ExitState));
  {
    const DisplayFinishInput input = { state, display, resource_info,
      exception, working_directory, image };

    return(XFinishDisplayImage(&input,&event_context,&event_targets));
  }
}
#endif
