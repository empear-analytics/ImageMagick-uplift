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

  MagickCore private methods of display-image.c: the draw-edit command, the
  undo cache, the image-window key commands and the command appliers from
  XApplyOpenCommand to XApplyFlopToImage; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_DRAW_UNDO_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_DRAW_UNDO_PRIVATE_H

static void XErasePreviewElement(const DrawEditContext *context,
  const DrawPreviewInput *preview,const ElementType element)
{
  switch (element)
  {
    default:
    {
      XErasePointLineBox(context,preview,element);
      break;
    }
    case CircleElement:
    case FillCircleElement:
    case EllipseElement:
    case FillEllipseElement:
    {
      XPreviewEllipseErase(context,preview);
      break;
    }
    case PolygonElement:
    case FillPolygonElement:
    {
      XPreviewPolyline(context,preview);
      XPreviewLongLine(context,preview);
      break;
    }
  }
}

static void XStartDrawElement(const DrawPointerTargets *pointer_targets,
  const DrawShapeTargets *shape_targets)
{
  int
    *const x = pointer_targets->x;

  int
    *const y = pointer_targets->y;

  XSegment
    *const line_info = shape_targets->line_info;

  RectangleInfo
    *const rectangle_info = shape_targets->rectangle_info;

  XPoint
    **const coordinate_info = shape_targets->coordinate_info;

  int
    *const number_coordinates = shape_targets->number_coordinates;

  (*line_info).x1=(*x);
  (*line_info).y1=(*y);
  (*line_info).x2=(*x);
  (*line_info).y2=(*y);
  (*rectangle_info).x=(ssize_t) (*x);
  (*rectangle_info).y=(ssize_t) (*y);
  (*rectangle_info).width=0;
  (*rectangle_info).height=0;
  (*number_coordinates)=1;
  (*coordinate_info)->x=(*x);
  (*coordinate_info)->y=(*y);
}

static void XTrackDrawElement(const DrawEditContext *context,
  const DrawPointerTargets *pointer_targets,
  const DrawShapeTargets *shape_targets,unsigned int *distance)
{
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

  ElementType
    *const element = pointer_targets->element;

  XSegment
    *const line_info = shape_targets->line_info;

  RectangleInfo
    *const rectangle_info = shape_targets->rectangle_info;

  XPoint
    **const coordinate_info = shape_targets->coordinate_info;

  int
    *const number_coordinates = shape_targets->number_coordinates;

  double
    degrees;

  DrawPreviewInput
    preview_input;

  DrawShapeInput
    shape_input;

  degrees=0.0;
  (void) XSetFunction(display,windows->image.highlight_context,GXinvert);
  (*state)=DefaultState;
  do
  {
    preview_input.element=(*element);
    preview_input.number_coordinates=(*number_coordinates);
    preview_input.coordinate_info=(*coordinate_info);
    preview_input.distance=(*distance);
    preview_input.line_info=(*line_info);
    preview_input.rectangle_info=(*rectangle_info);
    XPreviewElement(context,&preview_input,(*element),&degrees);
    /*
      Wait for next event.
    */
    XScreenEvent(display,windows,event,exception);
    XErasePreviewElement(context,&preview_input,(*element));
    shape_input.element=(*element);
    shape_input.distance=(*distance);
    XDrawShapeEvent(context,&shape_input,(*event),shape_targets);
    /*
      Check boundary conditions.
    */
    XClampLineEnd(windows,line_info);
    (*distance)=(unsigned int)
      ((((*line_info).x2-(*line_info).x1+1)*
        ((*line_info).x2-(*line_info).x1+1))+
       (((*line_info).y2-(*line_info).y1+1)*
        ((*line_info).y2-(*line_info).y1+1)));
    if (IsSelectionMovedOrDone((*rectangle_info),(*x),(*y),(*state)) !=
        MagickFalse)
      {
        (*rectangle_info)=XFitSelectionRectangle(windows,(*x),(*y),
          (*rectangle_info));
      }
  } while (((*state) & ExitState) == 0);
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
}

static MagickStatusType XDrawElementImage(const DrawEditContext *context,
  XDrawInfo *draw_info)
{
  Display
    *const display = context->display;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const image = context->image;

  MagickStatusType
    status;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  {
    const XImageRenderTarget
      target = { display, windows->pixel_info };

    status=XDrawImage(&target,draw_info,*image,exception);
  }
  XSetCursorState(display,windows,MagickFalse);
  /*
    Update image colormap and return to image drawing.
  */
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
  return(status);
}

static MagickBooleanType XFitDrawBounds(
  const DrawPointerTargets *pointer_targets,
  const DrawShapeTargets *shape_targets,const unsigned int distance)
{
  ElementType
    *const element = pointer_targets->element;

  RectangleInfo
    *const rectangle_info = shape_targets->rectangle_info;

  XPoint
    **const coordinate_info = shape_targets->coordinate_info;

  int
    *const number_coordinates = shape_targets->number_coordinates;

  if (IsPolygonElement((*element)) != MagickFalse)
    {
      (*rectangle_info)=XDrawPolygonBounds((*coordinate_info),
        (*number_coordinates),(*rectangle_info));
    }
  else
    if (distance <= 9)
      return(MagickTrue);
    else
      if (IsBoxedElement((*element)) != MagickFalse)
        {
          (*rectangle_info).width--;
          (*rectangle_info).height--;
        }
  return(MagickFalse);
}

static void XPrepareDrawInfo(const DrawEditContext *context,
  const DrawPointerTargets *pointer_targets,
  const DrawShapeTargets *shape_targets,const DrawStyleTargets *style_targets)
{
  Display
    *const display = context->display;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const image = context->image;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ElementType
    *const element = pointer_targets->element;

  XSegment
    *const line_info = shape_targets->line_info;

  RectangleInfo
    *const rectangle_info = shape_targets->rectangle_info;

  XPoint
    **const coordinate_info = shape_targets->coordinate_info;

  int
    *const number_coordinates = shape_targets->number_coordinates;

  XDrawInfo
    *const draw_info = style_targets->draw_info;

  unsigned int
    *const pen_id = style_targets->pen_id;

  unsigned int
    *const line_width = style_targets->line_width;

  Pixmap
    *const stipple = style_targets->stipple;

  const XImageCacheContext
    command_context = { display, resource_info, windows, image, exception };

  (*draw_info).x=(int) (*rectangle_info).x;
  (*draw_info).y=(int) (*rectangle_info).y;
  (void) XMagickCommand(&command_context,SaveToUndoBufferCommand);
  XSizeDrawRegion(context,draw_info,(*line_width),rectangle_info);
  /*
    Initialize drawing attributes.
  */
  (*draw_info).degrees=0.0;
  (*draw_info).element=(*element);
  (*draw_info).stipple=(*stipple);
  (*draw_info).line_width=(*line_width);
  XSetDrawLineInfo(draw_info,line_info,(*line_width));
  (*draw_info).rectangle_info=(*rectangle_info);
  if ((*draw_info).rectangle_info.x > (ssize_t) ((*line_width)/2))
    (*draw_info).rectangle_info.x=(ssize_t) (*line_width)/2;
  if ((*draw_info).rectangle_info.y > (ssize_t) ((*line_width)/2))
    (*draw_info).rectangle_info.y=(ssize_t) (*line_width)/2;
  (*draw_info).number_coordinates=(unsigned int) (*number_coordinates);
  (*draw_info).coordinate_info=(*coordinate_info);
  windows->pixel_info->pen_color=windows->pixel_info->pen_colors[(*pen_id)];
}

static void XMapDrawCommandWidget(Display *display,XWindows *windows,
  const char *const *DrawMenu)
{
  (void) CloneString(&windows->command.name,"Draw");
  windows->command.data=4;
  (void) XCommandWidget(display,windows,DrawMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
}

static MagickBooleanType XAcquireDrawCoordinates(ExceptionInfo *exception,
  const DrawShapeTargets *targets)
{
  XPoint
    **const coordinate_info = targets->coordinate_info;

  unsigned int
    *const max_coordinates = targets->max_coordinates;

  (*max_coordinates)=2048;
  (*coordinate_info)=(XPoint *) AcquireQuantumMemory((size_t) (*max_coordinates),
    sizeof(*(*coordinate_info)));
  if ((*coordinate_info) == (XPoint *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","...");
      return(MagickFalse);
    }
  return(MagickTrue);
}

static const char
  *const DrawEditMenu[] =
  {
    "Element",
    "Color",
    "Stipple",
    "Width",
    "Undo",
    "Help",
    "Dismiss",
    (char *) NULL
  };

static const ModeType
  DrawEditCommands[] =
  {
    DrawElementCommand,
    DrawColorCommand,
    DrawStippleCommand,
    DrawWidthCommand,
    DrawUndoCommand,
    DrawHelpCommand,
    DrawDismissCommand
  };

static MagickBooleanType XTrackDrawStroke(const DrawEditContext *context,
  const DrawPointerTargets *pointer_targets,
  const DrawShapeTargets *shape_targets,const DrawStyleTargets *style_targets)
{
  unsigned int
    distance;

  /*
    Draw element as pointer moves until the button is released.
  */
  distance=0;
  XStartDrawElement(pointer_targets,shape_targets);
  XTrackDrawElement(context,pointer_targets,shape_targets,&distance);
  if (XFitDrawBounds(pointer_targets,shape_targets,distance) != MagickFalse)
    return(MagickTrue);
  /*
    Drawing is relative to image configuration.
  */
  XPrepareDrawInfo(context,pointer_targets,shape_targets,style_targets);
  return(MagickFalse);
}

static MagickBooleanType XDrawEditStep(const DrawEditContext *context,
  const DrawPointerTargets *pointer_targets,
  const DrawShapeTargets *shape_targets,const DrawStyleTargets *style_targets)
{
  MagickStatusType
    *const status = style_targets->status;

  size_t
    *const state = pointer_targets->state;

  XDrawInfo
    *const draw_info = style_targets->draw_info;

  XWaitForDrawStart(context,pointer_targets,style_targets);
  if (((*state) & EscapeState) != 0)
    return(MagickTrue);
  if (XTrackDrawStroke(context,pointer_targets,shape_targets,style_targets) !=
      MagickFalse)
    return(MagickFalse);
  /*
    Draw element on image.
  */
  (*status)=XDrawElementImage(context,draw_info);
  return(MagickFalse);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D r a w I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDrawEditImage() draws a graphic element (point, line, rectangle, etc.) on
%  the image.
%
%  The format of the XDrawEditImage method is:
%
%      MagickBooleanType XDrawEditImage(Display *display,
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
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType XDrawEditImage(const XImageCacheContext *edit)
{
  static ElementType
    element = PointElement;

  static Pixmap
    stipple = (Pixmap) NULL;

  static unsigned int
    pen_id = 0,
    line_width = 1;

  char
    command[MagickPathExtent];

  Cursor
    cursor;

  int
    number_coordinates,
    x,
    y;

  MagickStatusType
    status;

  RectangleInfo
    rectangle_info;

  unsigned int
    max_coordinates;

  size_t
    state;

  Window
    root_window;

  XDrawInfo
    draw_info;

  XEvent
    event;

  XPoint
    *coordinate_info;

  XSegment
    line_info;

  const DrawShapeTargets
    shape_targets = { &line_info, &rectangle_info, &coordinate_info, &state,
      &number_coordinates, &max_coordinates };

  const DrawPointerTargets
    pointer_targets = { &x, &y, &state, &event, &element };

  const DrawStyleTargets
    style_targets = { &draw_info, &pen_id, &line_width, &stipple, &x, &y,
      &status };

  /*
    Allocate polygon info.
  */
  if (XAcquireDrawCoordinates(edit->exception,&shape_targets) ==
      MagickFalse)
    return(MagickFalse);
  /*
    Map Command widget.
  */
  XMapDrawCommandWidget(edit->display,edit->windows,DrawEditMenu);
  /*
    Wait for first button press.
  */
  root_window=XRootWindow(edit->display,XDefaultScreen(edit->display));
  draw_info.stencil=OpaqueStencil;
  status=MagickTrue;
  cursor=XCreateFontCursor(edit->display,XC_tcross);
  {
    const DrawEditContext
      draw_context = { edit->display, edit->resource_info, edit->windows,
        DrawEditMenu, command, root_window, edit->exception, edit->image,
        cursor, DrawEditCommands };

    for ( ; ; )
    {
      if (XDrawEditStep(&draw_context,&pointer_targets,&shape_targets,
          &style_targets) != MagickFalse)
        break;
    }
  }
  XSetCursorState(edit->display,edit->windows,MagickFalse);
  coordinate_info=(XPoint *) RelinquishMagickMemory(coordinate_info);
  return((MagickBooleanType) (status != 0));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D r a w P a n R e c t a n g l e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDrawPanRectangle() draws a rectangle in the pan window.  The pan window
%  displays a zoom image and the rectangle shows which portion of the image is
%  displayed in the Image window.
%
%  The format of the XDrawPanRectangle method is:
%
%      XDrawPanRectangle(Display *display,XWindows *windows)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
*/
static void XDrawPanRectangle(Display *display,XWindows *windows)
{
  double
    scale_factor;

  RectangleInfo
    highlight_info;

  /*
    Determine dimensions of the panning rectangle.
  */
  scale_factor=(double) windows->pan.width/windows->image.ximage->width;
  highlight_info.x=(ssize_t) (scale_factor*windows->image.x+0.5);
  highlight_info.width=(unsigned int) (scale_factor*windows->image.width+0.5);
  scale_factor=(double)
    windows->pan.height/windows->image.ximage->height;
  highlight_info.y=(ssize_t) (scale_factor*windows->image.y+0.5);
  highlight_info.height=(unsigned int) (scale_factor*windows->image.height+0.5);
  /*
    Display the panning rectangle.
  */
  (void) XClearWindow(display,windows->pan.id);
  XHighlightRectangle(display,windows->pan.id,windows->pan.annotate_context,
    &highlight_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X I m a g e C a c h e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XImageCache() handles the creation, manipulation, and destruction of the
%  image cache (undo and redo buffers).
%
%  The format of the XImageCache method is:
%
%      void XImageCache(Display *display,XResourceInfo *resource_info,
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
%    o image: the image;  XImageCache may transform the image and return a new
%      image pointer.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XTrimUndoCache(const XResourceInfo *resource_info,size_t *bytes,
  Image **undo_image)
{
  Image
    *cache_image,
    *previous_image;

  if (*undo_image != (Image *) NULL)
    {
      /*
        Ensure the undo cache has enough memory available.
      */
      previous_image=*undo_image;
      while (previous_image != (Image *) NULL)
      {
        *bytes+=previous_image->list->columns*previous_image->list->rows*
          sizeof(PixelInfo);
        if (*bytes <= (resource_info->undo_cache << 20))
          {
            previous_image=GetPreviousImageInList(previous_image);
            continue;
          }
        *bytes-=previous_image->list->columns*previous_image->list->rows*
          sizeof(PixelInfo);
        if (previous_image == *undo_image)
          *undo_image=NewImageList();
        else
          previous_image->next->previous=NewImageList();
        break;
      }
      while (previous_image != (Image *) NULL)
      {
        /*
          Delete any excess memory from undo cache.
        */
        cache_image=previous_image;
        previous_image=GetPreviousImageInList(previous_image);
        cache_image->list=DestroyImage(cache_image->list);
        cache_image=DestroyImage(cache_image);
      }
    }
}

static void XCopyCropGeometryToCache(XWindows *windows,Image *cache_image)
{
  if (windows->image.crop_geometry != (char *) NULL)
    {
      cache_image->geometry=AcquireString((char *) NULL);
      (void) CopyMagickString(cache_image->geometry,
        windows->image.crop_geometry,MagickPathExtent);
    }
}

static Image *XSaveUndoImage(const XImageCacheContext *cache_context,
  Image *undo_image)
{
  Display
    *const display = cache_context->display;

  XResourceInfo
    *const resource_info = cache_context->resource_info;

  XWindows
    *const windows = cache_context->windows;

  Image
    **const image = cache_context->image;

  ExceptionInfo
    *const exception = cache_context->exception;

  Image
    *cache_image;

  size_t
    bytes;

  bytes=(*image)->columns*(*image)->rows*sizeof(PixelInfo);
  XTrimUndoCache(resource_info,&bytes,&undo_image);
  if (bytes > (resource_info->undo_cache << 20))
    return(undo_image);
  /*
    Save image before transformations are applied.
  */
  cache_image=AcquireImage((ImageInfo *) NULL,exception);
  if (cache_image == (Image *) NULL)
    return(undo_image);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  cache_image->list=CloneImage(*image,0,0,MagickTrue,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (cache_image->list == (Image *) NULL)
    {
      cache_image=DestroyImage(cache_image);
      return(undo_image);
    }
  cache_image->columns=(size_t) windows->image.ximage->width;
  cache_image->rows=(size_t) windows->image.ximage->height;
  cache_image->geometry=windows->image.crop_geometry;
  XCopyCropGeometryToCache(windows,cache_image);
  if (undo_image == (Image *) NULL)
    {
      undo_image=cache_image;
      return(undo_image);
    }
  undo_image->next=cache_image;
  undo_image->next->previous=undo_image;
  undo_image=undo_image->next;
  return(undo_image);
}

static const DisplayCommand
  UndoCacheCommands[] =
  {
    CutCommand,
    PasteCommand,
    ApplyCommand,
    HalfSizeCommand,
    OriginalSizeCommand,
    DoubleSizeCommand,
    ResizeCommand,
    TrimCommand,
    CropCommand,
    ChopCommand,
    FlipCommand,
    FlopCommand,
    RotateRightCommand,
    RotateLeftCommand,
    RotateCommand,
    ShearCommand,
    RollCommand,
    NegateCommand,
    ContrastStretchCommand,
    SigmoidalContrastCommand,
    NormalizeCommand,
    EqualizeCommand,
    HueCommand,
    SaturationCommand,
    BrightnessCommand,
    GammaCommand,
    SpiffCommand,
    DullCommand,
    GrayscaleCommand,
    MapCommand,
    QuantizeCommand,
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
    SegmentCommand,
    SolarizeCommand,
    SepiaToneCommand,
    SwirlCommand,
    ImplodeCommand,
    VignetteCommand,
    WaveCommand,
    OilPaintCommand,
    CharcoalDrawCommand,
    AnnotateCommand,
    AddBorderCommand,
    AddFrameCommand,
    CompositeCommand,
    CommentCommand,
    LaunchCommand,
    RegionOfInterestCommand,
    SaveToUndoBufferCommand,
    RedoCommand
  };

static MagickBooleanType IsUndoCacheCommand(const DisplayCommand command)
{
  size_t
    i;

  for (i=0; i < (sizeof(UndoCacheCommands)/sizeof(*UndoCacheCommands)); i++)
    if (UndoCacheCommands[i] == command)
      return(MagickTrue);
  return(MagickFalse);
}

static void XUndoImageCache(const XImageCacheContext *cache_context,
  Image **undo_image,Image **redo_image)
{
  Display
    *const display = cache_context->display;

  XResourceInfo
    *const resource_info = cache_context->resource_info;

  XWindows
    *const windows = cache_context->windows;

  Image
    **const image = cache_context->image;

  ExceptionInfo
    *const exception = cache_context->exception;

  char
    image_geometry[MagickPathExtent];

  Image
    *cache_image;

  /*
    Undo the last image transformation.
  */
  if ((*undo_image) == (Image *) NULL)
    {
      (void) XBell(display,0);
      ThrowXWindowException(ImageError,"NoImagesWereFound",
        (*image)->filename);
      return;
    }
  cache_image=(*undo_image);
  (*undo_image)=GetPreviousImageInList((*undo_image));
  windows->image.window_changes.width=(int) cache_image->columns;
  windows->image.window_changes.height=(int) cache_image->rows;
  (void) FormatLocaleString(image_geometry,MagickPathExtent,"%dx%d!",
    windows->image.ximage->width,windows->image.ximage->height);
  (void) TransformImage(image,windows->image.crop_geometry,image_geometry,
    exception);
  if (windows->image.crop_geometry != (char *) NULL)
    windows->image.crop_geometry=(char *) RelinquishMagickMemory(
      windows->image.crop_geometry);
  windows->image.crop_geometry=cache_image->geometry;
  if ((*redo_image) != (Image *) NULL)
    (*redo_image)=DestroyImage((*redo_image));
  (*redo_image)=(*image);
  *image=cache_image->list;
  cache_image=DestroyImage(cache_image);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XShowImageCacheInfo(const XImageCacheContext *cache_context,
  Image *undo_image)
{
  Display
    *const display = cache_context->display;

  XWindows
    *const windows = cache_context->windows;

  XResourceInfo
    *const resource_info = cache_context->resource_info;

  Image
    **const image = cache_context->image;

  ExceptionInfo
    *const exception = cache_context->exception;

  const XDisplayImageInfoTarget
    target = { display, resource_info, windows };

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  XDisplayImageInfo(&target,undo_image,*image,exception);
  XSetCursorState(display,windows,MagickFalse);
}

static void XRedoOrShowImageCache(const XImageCacheContext *cache_context,
  const DisplayCommand command,Image **redo_image,Image *undo_image)
{
  Display
    *const display = cache_context->display;

  XResourceInfo
    *const resource_info = cache_context->resource_info;

  XWindows
    *const windows = cache_context->windows;

  Image
    **const image = cache_context->image;

  ExceptionInfo
    *const exception = cache_context->exception;

  if (command == RedoCommand)
    {
      /*
        Redo the last image transformation.
      */
      if ((*redo_image) == (Image *) NULL)
        {
          (void) XBell(display,0);
          return;
        }
      windows->image.window_changes.width=(int) (*redo_image)->columns;
      windows->image.window_changes.height=(int) (*redo_image)->rows;
      if (windows->image.crop_geometry != (char *) NULL)
        windows->image.crop_geometry=(char *)
          RelinquishMagickMemory(windows->image.crop_geometry);
      windows->image.crop_geometry=(*redo_image)->geometry;
      *image=DestroyImage(*image);
      *image=(*redo_image);
      (*redo_image)=NewImageList();
      if (windows->image.orphan != MagickFalse)
        return;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      return;
    }
  if (command != InfoCommand)
    return;
  /*
    Display image info.
  */
  XShowImageCacheInfo(cache_context,undo_image);
}

static void XImageCache(const XImageCacheContext *cache_context,
  const DisplayCommand command)
{
  Image
    *cache_image;

  static Image
    *redo_image = (Image *) NULL,
    *undo_image = (Image *) NULL;

  switch (command)
  {
    case FreeBuffersCommand:
    {
      /*
        Free memory from the undo and redo cache.
      */
      while (undo_image != (Image *) NULL)
      {
        cache_image=undo_image;
        undo_image=GetPreviousImageInList(undo_image);
        cache_image->list=DestroyImage(cache_image->list);
        cache_image=DestroyImage(cache_image);
      }
      undo_image=NewImageList();
      if (redo_image != (Image *) NULL)
        redo_image=DestroyImage(redo_image);
      redo_image=NewImageList();
      return;
    }
    case UndoCommand:
    {
      XUndoImageCache(cache_context,&undo_image,&redo_image);
      return;
    }
    default:
      break;
  }
  if (IsUndoCacheCommand(command) != MagickFalse)
    undo_image=XSaveUndoImage(cache_context,undo_image);
  XRedoOrShowImageCache(cache_context,command,&redo_image,undo_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X I m a g e W i n d o w C o m m a n d                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XImageWindowCommand() makes a transform to the image or Image window as
%  specified by a user menu button or keyboard command.
%
%  The format of the XImageWindowCommand method is:
%
%      DisplayCommand XImageWindowCommand(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,
%        const MagickStatusType state,KeySym key_symbol,Image **image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o nexus:  Method XImageWindowCommand returns an image when the
%      user chooses 'Open Image' from the command menu.  Otherwise a null
%      image is returned.
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o state: key mask.
%
%    o key_symbol: Specifies a command to perform.
%
%    o image: the image;  XImageWIndowCommand may transform the image and
%      return a new image pointer.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XClampTrimTopQuantum(XResourceInfo *resource_info,
  const RectangleInfo crop_info)
{
  if (resource_info->quantum >= (int) crop_info.height)
    resource_info->quantum=(int) crop_info.height-1;
}

static void XClampTrimBottomQuantum(XResourceInfo *resource_info,
  const RectangleInfo crop_info)
{
  if (resource_info->quantum >= (int) (crop_info.height-(ssize_t) crop_info.y))
    resource_info->quantum=(int) (crop_info.height-(ssize_t) crop_info.y-1);
}

static void XClampTrimLeftQuantum(XResourceInfo *resource_info,
  const RectangleInfo crop_info)
{
  if (resource_info->quantum >= (int) crop_info.width)
    resource_info->quantum=(int) crop_info.width-1;
}

static void XClampTrimRightQuantum(XResourceInfo *resource_info,
  const RectangleInfo crop_info)
{
  if (resource_info->quantum >= (int) (crop_info.width-(ssize_t) crop_info.x))
    resource_info->quantum=(int) (crop_info.width-(ssize_t) crop_info.x-1);
}

typedef struct _XKeyCommandInfo
{
  int
    key;

  MagickStatusType
    first_mask;

  DisplayCommand
    first_command;

  MagickStatusType
    second_mask;

  DisplayCommand
    second_command,
    plain_command,
    control_command;
} XKeyCommandInfo;

static const XKeyCommandInfo
  ImageWindowKeyCommands[] =
  {
    { XK_o, ControlMask, OpenCommand, ControlMask, OpenCommand, NullCommand,
      OpenCommand },
    { XK_space, ControlMask, NextCommand, ControlMask, NextCommand,
      NextCommand, NextCommand },
    { XK_BackSpace, ControlMask, FormerCommand, ControlMask, FormerCommand,
      FormerCommand, FormerCommand },
    { XK_s, Mod1Mask, SwirlCommand, ControlMask, SaveCommand, ShearCommand,
      SaveCommand },
    { XK_p, Mod1Mask, OilPaintCommand, Mod4Mask, ColorCommand, NullCommand,
      PrintCommand },
    { XK_Print, Mod1Mask, OilPaintCommand, Mod4Mask, ColorCommand,
      NullCommand, PrintCommand },
    { XK_d, Mod4Mask, DrawCommand, ControlMask, DeleteCommand, NullCommand,
      DeleteCommand },
    { XK_Select, ControlMask, SelectCommand, ControlMask, SelectCommand,
      NullCommand, SelectCommand },
    { XK_n, ControlMask, NewCommand, ControlMask, NewCommand, NullCommand,
      NewCommand },
    { XK_q, ControlMask, QuitCommand, ControlMask, QuitCommand, QuitCommand,
      QuitCommand },
    { XK_Escape, ControlMask, QuitCommand, ControlMask, QuitCommand,
      QuitCommand, QuitCommand },
    { XK_z, ControlMask, UndoCommand, ControlMask, UndoCommand, NullCommand,
      UndoCommand },
    { XK_Undo, ControlMask, UndoCommand, ControlMask, UndoCommand,
      NullCommand, UndoCommand },
    { XK_r, ControlMask, RedoCommand, ControlMask, RedoCommand, RollCommand,
      RedoCommand },
    { XK_Redo, ControlMask, RedoCommand, ControlMask, RedoCommand,
      RollCommand, RedoCommand },
    { XK_x, ControlMask, CutCommand, ControlMask, CutCommand, NullCommand,
      CutCommand },
    { XK_c, Mod1Mask, CharcoalDrawCommand, ControlMask, CopyCommand,
      CropCommand, CopyCommand },
    { XK_v, Mod4Mask, CompositeCommand, ControlMask, PasteCommand,
      FlipCommand, PasteCommand },
    { XK_Insert, Mod4Mask, CompositeCommand, ControlMask, PasteCommand,
      FlipCommand, PasteCommand },
    { XK_less, ControlMask, HalfSizeCommand, ControlMask, HalfSizeCommand,
      HalfSizeCommand, HalfSizeCommand },
    { XK_minus, ControlMask, OriginalSizeCommand, ControlMask,
      OriginalSizeCommand, OriginalSizeCommand, OriginalSizeCommand },
    { XK_greater, ControlMask, DoubleSizeCommand, ControlMask,
      DoubleSizeCommand, DoubleSizeCommand, DoubleSizeCommand },
    { XK_percent, ControlMask, ResizeCommand, ControlMask, ResizeCommand,
      ResizeCommand, ResizeCommand },
    { XK_at, ControlMask, RefreshCommand, ControlMask, RefreshCommand,
      RefreshCommand, RefreshCommand },
    { XK_bracketleft, ControlMask, ChopCommand, ControlMask, ChopCommand,
      ChopCommand, ChopCommand },
    { XK_h, ControlMask, FlopCommand, ControlMask, FlopCommand, FlopCommand,
      FlopCommand },
    { XK_slash, ControlMask, RotateRightCommand, ControlMask,
      RotateRightCommand, RotateRightCommand, RotateRightCommand },
    { XK_backslash, ControlMask, RotateLeftCommand, ControlMask,
      RotateLeftCommand, RotateLeftCommand, RotateLeftCommand },
    { XK_asterisk, ControlMask, RotateCommand, ControlMask, RotateCommand,
      RotateCommand, RotateCommand },
    { XK_t, ControlMask, TrimCommand, ControlMask, TrimCommand, TrimCommand,
      TrimCommand },
    { XK_H, ControlMask, HueCommand, ControlMask, HueCommand, HueCommand,
      HueCommand },
    { XK_S, ControlMask, SaturationCommand, ControlMask, SaturationCommand,
      SaturationCommand, SaturationCommand },
    { XK_L, ControlMask, BrightnessCommand, ControlMask, BrightnessCommand,
      BrightnessCommand, BrightnessCommand },
    { XK_G, ControlMask, GammaCommand, ControlMask, GammaCommand,
      GammaCommand, GammaCommand },
    { XK_C, ControlMask, SpiffCommand, ControlMask, SpiffCommand,
      SpiffCommand, SpiffCommand },
    { XK_Z, ControlMask, DullCommand, ControlMask, DullCommand, DullCommand,
      DullCommand },
    { XK_N, ControlMask, NormalizeCommand, ControlMask, NormalizeCommand,
      NormalizeCommand, NormalizeCommand },
    { XK_equal, ControlMask, EqualizeCommand, ControlMask, EqualizeCommand,
      EqualizeCommand, EqualizeCommand },
    { XK_asciitilde, ControlMask, NegateCommand, ControlMask, NegateCommand,
      NegateCommand, NegateCommand },
    { XK_period, ControlMask, GrayscaleCommand, ControlMask, GrayscaleCommand,
      GrayscaleCommand, GrayscaleCommand },
    { XK_numbersign, ControlMask, QuantizeCommand, ControlMask,
      QuantizeCommand, QuantizeCommand, QuantizeCommand },
    { XK_F2, ControlMask, DespeckleCommand, ControlMask, DespeckleCommand,
      DespeckleCommand, DespeckleCommand },
    { XK_F3, ControlMask, EmbossCommand, ControlMask, EmbossCommand,
      EmbossCommand, EmbossCommand },
    { XK_F4, ControlMask, ReduceNoiseCommand, ControlMask, ReduceNoiseCommand,
      ReduceNoiseCommand, ReduceNoiseCommand },
    { XK_F5, ControlMask, AddNoiseCommand, ControlMask, AddNoiseCommand,
      AddNoiseCommand, AddNoiseCommand },
    { XK_F6, ControlMask, SharpenCommand, ControlMask, SharpenCommand,
      SharpenCommand, SharpenCommand },
    { XK_F7, ControlMask, BlurCommand, ControlMask, BlurCommand, BlurCommand,
      BlurCommand },
    { XK_F8, ControlMask, ThresholdCommand, ControlMask, ThresholdCommand,
      ThresholdCommand, ThresholdCommand },
    { XK_F9, ControlMask, EdgeDetectCommand, ControlMask, EdgeDetectCommand,
      EdgeDetectCommand, EdgeDetectCommand },
    { XK_F10, ControlMask, SpreadCommand, ControlMask, SpreadCommand,
      SpreadCommand, SpreadCommand },
    { XK_F11, ControlMask, ShadeCommand, ControlMask, ShadeCommand,
      ShadeCommand, ShadeCommand },
    { XK_F12, ControlMask, RaiseCommand, ControlMask, RaiseCommand,
      RaiseCommand, RaiseCommand },
    { XK_F13, ControlMask, SegmentCommand, ControlMask, SegmentCommand,
      SegmentCommand, SegmentCommand },
    { XK_i, Mod1Mask, ImplodeCommand, ControlMask, NullCommand, NullCommand,
      NullCommand },
    { XK_w, Mod1Mask, WaveCommand, ControlMask, NullCommand, NullCommand,
      NullCommand },
    { XK_m, Mod4Mask, MatteCommand, ControlMask, NullCommand, NullCommand,
      NullCommand },
    { XK_b, Mod4Mask, AddBorderCommand, ControlMask, NullCommand, NullCommand,
      NullCommand },
    { XK_f, Mod4Mask, AddFrameCommand, ControlMask, NullCommand, NullCommand,
      NullCommand },
    { XK_exclam, Mod4Mask, CommentCommand, ControlMask, NullCommand,
      NullCommand, NullCommand },
    { XK_a, Mod1Mask, ApplyCommand, Mod4Mask, AnnotateCommand, NullCommand,
      RegionOfInterestCommand },
    { XK_question, ControlMask, InfoCommand, ControlMask, InfoCommand,
      InfoCommand, InfoCommand },
    { XK_plus, ControlMask, ZoomCommand, ControlMask, ZoomCommand,
      ZoomCommand, ZoomCommand },
    { XK_Execute, ControlMask, LaunchCommand, ControlMask, LaunchCommand,
      LaunchCommand, LaunchCommand },
    { XK_F1, ControlMask, HelpCommand, ControlMask, HelpCommand, HelpCommand,
      HelpCommand },
    { XK_Find, ControlMask, BrowseDocumentationCommand, ControlMask,
      BrowseDocumentationCommand, BrowseDocumentationCommand,
      BrowseDocumentationCommand }
  };

static DisplayCommand XLookupKeyCommand(const int key,
  const MagickStatusType state)
{
  size_t
    i;

  for (i=0; i < (sizeof(ImageWindowKeyCommands)/
       sizeof(*ImageWindowKeyCommands)); i++)
  {
    if (ImageWindowKeyCommands[i].key != key)
      continue;
    if ((state & ImageWindowKeyCommands[i].first_mask) != 0)
      return(ImageWindowKeyCommands[i].first_command);
    if ((state & ImageWindowKeyCommands[i].second_mask) != 0)
      return(ImageWindowKeyCommands[i].second_command);
    if ((state & ControlMask) == 0)
      return(ImageWindowKeyCommands[i].plain_command);
    return(ImageWindowKeyCommands[i].control_command);
  }
  return(NullCommand);
}

static DisplayCommand XImmutableWindowCommand(const KeySym key_symbol)
{
  /*
    Virtual image window has a restricted command set.
  */
  switch (key_symbol)
  {
    case XK_question:
      return(InfoCommand);
    case XK_p:
    case XK_Print:
      return(PrintCommand);
    case XK_space:
      return(NextCommand);
    case XK_q:
    case XK_Escape:
      return(QuitCommand);
    default:
      break;
  }
  return(NullCommand);
}

static void XTrimBottomEdge(const KeySym key_symbol,
  XResourceInfo *resource_info,RectangleInfo *crop_info)
{
  if ((key_symbol == XK_Down) || (key_symbol == XK_KP_Down))
    {
      XClampTrimBottomQuantum(resource_info,(*crop_info));
      (*crop_info).y+=resource_info->quantum;
      (*crop_info).height-=(size_t) resource_info->quantum;
    }
}

static void XTrimVerticalEdges(XResourceInfo *resource_info,
  const KeySym key_symbol,RectangleInfo *crop_info)
{
  if ((key_symbol == XK_Up) || (key_symbol == XK_KP_Up))
    {
      XClampTrimTopQuantum(resource_info,(*crop_info));
      (*crop_info).height-=(size_t) resource_info->quantum;
    }
  XTrimBottomEdge(key_symbol,resource_info,crop_info);
}

static DisplayCommand XTrimImageWindowEdge(const ImageWindowKeyInput *input,
  const KeySym key_symbol)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  Image
    **const image = input->image;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  RectangleInfo
    crop_info;

  /*
    Trim one pixel from edge of image.
  */
  crop_info.x=0;
  crop_info.y=0;
  crop_info.width=(size_t) windows->image.ximage->width;
  crop_info.height=(size_t) windows->image.ximage->height;
  XTrimVerticalEdges(resource_info,key_symbol,&crop_info);
  if ((key_symbol == XK_Left) || (key_symbol == XK_KP_Left))
    {
      XClampTrimLeftQuantum(resource_info,crop_info);
      crop_info.width-=(size_t) resource_info->quantum;
    }
  if ((key_symbol == XK_Right) || (key_symbol == XK_KP_Right))
    {
      XClampTrimRightQuantum(resource_info,crop_info);
      crop_info.x+=resource_info->quantum;
      crop_info.width-=(size_t) resource_info->quantum;
    }
  if ((windows->image.x+(int) windows->image.width) > (int) crop_info.width)
    windows->image.x=(int) (crop_info.width-windows->image.width);
  if ((windows->image.y+(int) windows->image.height) > (int) crop_info.height)
    windows->image.y=(int) (crop_info.height-windows->image.height);
  XSetCropGeometry(display,windows,&crop_info,*image);
  windows->image.window_changes.width=(int) crop_info.width;
  windows->image.window_changes.height=(int) crop_info.height;
  (void) XSetWindowBackgroundPixmap(display,windows->image.id,None);
  (void) XConfigureImage(display,resource_info,windows,*image,
    exception);
  return(NullCommand);
}

static const char
  Digits[] = "01234567890";

static void XEnterWindowDigit(XResourceInfo *resource_info,
  const KeySym key_symbol,char *delta,KeySym *last_symbol)
{
  size_t
    length;

  if ((((*last_symbol) < XK_0) || ((*last_symbol) > XK_9)))
    {
      *delta='\0';
      resource_info->quantum=1;
    }
  (*last_symbol)=key_symbol;
  length=strlen(delta);
  if (length < MagickPathExtent)
    {
      delta[length]=Digits[key_symbol-XK_0];
      delta[length+1]='\0';
    }
  resource_info->quantum=StringToLong(delta);
}

static MagickBooleanType IsImageWindowTranslateKey(const KeySym key_symbol)
{
  switch ((int) key_symbol)
  {
    case XK_Next:
    case XK_Prior:
    case XK_Home:
    case XK_KP_Home:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static DisplayCommand XImageWindowArrowCommand(const ImageWindowKeyInput *input,
  const MagickStatusType state,const KeySym key_symbol)
{
  Display
    *const display = input->display;

  Image
    **const image = input->image;

  XWindows
    *const windows = input->windows;

  if ((state & Mod1Mask) != 0)
    return(XTrimImageWindowEdge(input,key_symbol));
  XTranslateImage(display,windows,*image,key_symbol);
  return(NullCommand);
}

static MagickBooleanType IsImageWindowHorizontalKey(const KeySym key_symbol)
{
  switch ((int) key_symbol)
  {
    case XK_Left:
    case XK_KP_Left:
    case XK_Right:
    case XK_KP_Right:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static MagickBooleanType IsImageWindowArrowKey(const KeySym key_symbol)
{
  switch ((int) key_symbol)
  {
    case XK_Up:
    case XK_KP_Up:
    case XK_Down:
    case XK_KP_Down:
      return(MagickTrue);
    default:
      break;
  }
  return(IsImageWindowHorizontalKey(key_symbol));
}

static DisplayCommand XImageWindowKeyCommand(const ImageWindowKeyInput *input,
  const MagickStatusType state,const KeySym key_symbol)
{
  Display
    *const display = input->display;

  Image
    **const image = input->image;

  XWindows
    *const windows = input->windows;

  switch ((int) key_symbol)
  {
    case XK_P:
    {
      if ((state & ShiftMask) == 0)
        return(NullCommand);
      return(ShowPreviewCommand);
    }
    case XK_Menu:
    {
      (void) XMapRaised(display,windows->command.id);
      return(NullCommand);
    }
    default:
      break;
  }
  if (IsImageWindowArrowKey(key_symbol) != MagickFalse)
    return(XImageWindowArrowCommand(input,state,key_symbol));
  if (IsImageWindowTranslateKey(key_symbol) != MagickFalse)
    {
      XTranslateImage(display,windows,*image,key_symbol);
      return(NullCommand);
    }
  return(XLookupKeyCommand((int) key_symbol,state));
}

static DisplayCommand XImageWindowCommand(const ImageWindowKeyInput *input,
  const MagickStatusType state,KeySym key_symbol)
{
  XResourceInfo
    *const resource_info = input->resource_info;

  static char
    delta[MagickPathExtent+1] = "";

  static KeySym
    last_symbol = XK_0;

  if ((key_symbol >= XK_0) && (key_symbol <= XK_9))
    {
      XEnterWindowDigit(resource_info,key_symbol,delta,&last_symbol);
      return(NullCommand);
    }
  last_symbol=key_symbol;
  if (resource_info->immutable)
    return(XImmutableWindowCommand(key_symbol));
  return(XImageWindowKeyCommand(input,state,key_symbol));
}
static inline MagickBooleanType IsImageWindowUncropped(
  const XWindows *windows,Image **image)
{
  return((MagickBooleanType) ((windows->image.crop_geometry == (char *) NULL) &&
    ((int) (*image)->columns == windows->image.ximage->width) &&
    ((int) (*image)->rows == windows->image.ximage->height)));
}

static inline MagickBooleanType IsImageWindowOriginalSize(
  const XWindows *windows,Image **image)
{
  return((MagickBooleanType) ((windows->image.width == (unsigned int)
    (*image)->columns) && (windows->image.height == (unsigned int)
    (*image)->rows) && (windows->image.crop_geometry == (char *) NULL)));
}

typedef struct _XMagickCommandContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    **image;

  ImageInfo
    *image_info;

  ExceptionInfo
    *exception;

  char
    *color;

  QuantizeInfo
    *quantize_info;
} XMagickCommandContext;

static Image *XApplyOpenCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    *nexus;

  /*
    Load image.
  */
  nexus=XOpenImage(display,resource_info,windows,MagickFalse);
  return(nexus);
}

static void XApplyNextCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  int
    i;

  /*
    Display next image.
  */
  for (i=0; i < resource_info->quantum; i++)
    XClientMessage(display,windows->image.id,windows->im_protocols,
      windows->im_next_image,CurrentTime);
}

static void XApplyFormerCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  int
    i;

  /*
    Display former image.
  */
  for (i=0; i < resource_info->quantum; i++)
    XClientMessage(display,windows->image.id,windows->im_protocols,
      windows->im_former_image,CurrentTime);
}

static Image *XApplySelectCommand(const XMagickCommandContext *context)
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
    *nexus;

  int
    status;

  /*
    Select image.
  */
  if (*resource_info->home_directory == '\0')
    (void) CopyMagickString(resource_info->home_directory,".",
      MagickPathExtent);
  status=chdir(resource_info->home_directory);
  if (status == -1)
    (void) ThrowMagickException(exception,GetMagickModule(),FileOpenError,
      "UnableToOpenFile","%s",resource_info->home_directory);
  nexus=XOpenImage(display,resource_info,windows,MagickTrue);
  return(nexus);
}

static void XApplyPrintCommand(const XMagickCommandContext *context)
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
    Print image.
  */
  status=XPrintImage(display,resource_info,windows,*image,exception);
  if (status == MagickFalse)
    {
      char
        message[MagickPathExtent];

      (void) FormatLocaleString(message,MagickPathExtent,"%s:%s",
        exception->reason != (char *) NULL ? exception->reason : "",
        exception->description != (char *) NULL ? exception->description :
        "");
      XNoticeWidget(display,windows,"Unable to print file:",message);
      return;
    }
}

static void XApplyDeleteCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  MagickStatusType
    status;

  static char
    filename[MagickPathExtent] = "\0";

  /*
    Delete image file.
  */
  XFileBrowserWidget(display,windows,"Delete",filename);
  if (*filename == '\0')
    return;
  status=ShredFile(filename);
  if (remove_utf8(filename) < 0)
    status=MagickTrue;
  if (status != MagickFalse)
    XNoticeWidget(display,windows,"Unable to delete image file:",filename);
}

static Image *XApplyVisualDirectoryCommand(const XMagickCommandContext *context)
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
    *nexus;

  /*
    Visual Image directory.
  */
  nexus=XVisualDirectoryImage(display,resource_info,windows,exception);
  return(nexus);
}

static void XApplyCutCommand(const XMagickCommandContext *context)
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
    Cut image.
  */
  (void) XCropImage(display,resource_info,windows,*image,CutMode,exception);
}

static void XApplyCopyCommand(const XMagickCommandContext *context)
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
    Copy image.
  */
  (void) XCropImage(display,resource_info,windows,*image,CopyMode,
    exception);
}

static void XApplyPasteCommand(const XMagickCommandContext *context)
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
    Paste image.
  */
  status=XPasteImage(display,resource_info,windows,*image,exception);
  if (status == MagickFalse)
    {
      XNoticeWidget(display,windows,"Unable to paste X image",
        (*image)->filename);
      return;
    }
}

static void XApplyHalfSizeCommand(const XMagickCommandContext *context)
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
    Half image size.
  */
  windows->image.window_changes.width=windows->image.ximage->width/2;
  windows->image.window_changes.height=windows->image.ximage->height/2;
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyOriginalSizeCommand(const XMagickCommandContext *context)
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
    Original image size.
  */
  windows->image.window_changes.width=(int) (*image)->columns;
  windows->image.window_changes.height=(int) (*image)->rows;
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyDoubleSizeCommand(const XMagickCommandContext *context)
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
    Double the image size.
  */
  windows->image.window_changes.width=windows->image.ximage->width << 1;
  windows->image.window_changes.height=windows->image.ximage->height << 1;
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyRefreshCommand(const XMagickCommandContext *context)
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

  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyCropCommand(const XMagickCommandContext *context)
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
    Crop image.
  */
  (void) XCropImage(display,resource_info,windows,*image,CropMode,
    exception);
}

static void XApplyChopCommand(const XMagickCommandContext *context)
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
    Chop image.
  */
  status=XChopImage(display,resource_info,windows,image,exception);
  if (status == MagickFalse)
    {
      XNoticeWidget(display,windows,"Unable to cut X image",
        (*image)->filename);
      return;
    }
}

static void XApplyFlopToImage(Display *display,XWindows *windows,Image **image,
  ExceptionInfo *exception)
{
  Image
    *flop_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  flop_image=FlopImage(*image,exception);
  if (flop_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=flop_image;
    }
}

#endif
