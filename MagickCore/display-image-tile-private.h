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

  MagickCore private methods of display-image.c: the trim and visual-directory
  commands and the background helpers, from XTrimLeftEdge to
  XDisplayBackgroundPixmap; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_TILE_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_TILE_PRIVATE_H

#include "MagickCore/display-image-translate-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X T r i m I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XTrimImage() trims the edges from the Image window.
%
%  The format of the XTrimImage method is:
%
%      MagickBooleanType XTrimImage(Display *display,
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
static void XTrimLeftEdge(const XWindows *windows,RectangleInfo *trim_info)
{
  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Crop the left edge.
  */
  background=XGetPixel(windows->image.ximage,0,0);
  (*trim_info).width=(size_t) windows->image.ximage->width;
  for (x=0; x < windows->image.ximage->width; x++)
  {
    for (y=0; y < windows->image.ximage->height; y++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (y < windows->image.ximage->height)
      break;
  }
  (*trim_info).x=(ssize_t) x;
}

static void XTrimRightEdge(const XWindows *windows,RectangleInfo *trim_info)
{
  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Crop the right edge.
  */
  background=XGetPixel(windows->image.ximage,windows->image.ximage->width-1,0);
  for (x=windows->image.ximage->width-1; x != 0; x--)
  {
    for (y=0; y < windows->image.ximage->height; y++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (y < windows->image.ximage->height)
      break;
  }
  (*trim_info).width=(size_t) (x-(*trim_info).x+1);
}

static void XTrimTopEdge(const XWindows *windows,RectangleInfo *trim_info)
{
  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Crop the top edge.
  */
  background=XGetPixel(windows->image.ximage,0,0);
  (*trim_info).height=(size_t) windows->image.ximage->height;
  for (y=0; y < windows->image.ximage->height; y++)
  {
    for (x=0; x < windows->image.ximage->width; x++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (x < windows->image.ximage->width)
      break;
  }
  (*trim_info).y=(ssize_t) y;
}

static MagickBooleanType XTrimImage(const XImageCommandInput *command_input)
{
  Display
    *const display = command_input->display;

  XResourceInfo
    *const resource_info = command_input->resource_info;

  XWindows
    *const windows = command_input->windows;

  Image
    *const image = command_input->image;

  ExceptionInfo
    *const exception = command_input->exception;

  RectangleInfo
    trim_info;

  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Trim edges from image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  /*
    Crop the left edge.
  */
  XTrimLeftEdge(windows,&trim_info);
  if (trim_info.x == (ssize_t) windows->image.ximage->width)
    {
      XSetCursorState(display,windows,MagickFalse);
      return(MagickFalse);
    }
  /*
    Crop the right edge.
  */
  XTrimRightEdge(windows,&trim_info);
  /*
    Crop the top edge.
  */
  XTrimTopEdge(windows,&trim_info);
  /*
    Crop the bottom edge.
  */
  background=XGetPixel(windows->image.ximage,0,windows->image.ximage->height-1);
  for (y=windows->image.ximage->height-1; y != 0; y--)
  {
    for (x=0; x < windows->image.ximage->width; x++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (x < windows->image.ximage->width)
      break;
  }
  trim_info.height=(size_t) (y-trim_info.y+1);
  if (((unsigned int) trim_info.width != windows->image.width) ||
      ((unsigned int) trim_info.height != windows->image.height))
    {
      /*
        Reconfigure Image window as defined by the trimming rectangle.
      */
      XSetCropGeometry(display,windows,&trim_info,image);
      windows->image.window_changes.width=(int) trim_info.width;
      windows->image.window_changes.height=(int) trim_info.height;
      (void) XConfigureImage(display,resource_info,windows,image,exception);
    }
  XSetCursorState(display,windows,MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X V i s u a l D i r e c t o r y I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XVisualDirectoryImage() creates a Visual Image Directory.
%
%  The format of the XVisualDirectoryImage method is:
%
%      Image *XVisualDirectoryImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,
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
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _DirectoryInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  char
    *filename;

  ExceptionInfo
    *exception;
} DirectoryInput;

static Image *XMontageImageDirectory(const DirectoryInput *input,
  ImageInfo *read_info,Image *images)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  Image
    *montage_image;

  MontageInfo
    *montage_info;

  char
    *const filename = input->filename;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  /*
    Create the Visual Image Directory.
  */
  montage_info=CloneMontageInfo(read_info,(MontageInfo *) NULL);
  montage_info->pointsize=10;
  if (resource_info->font != (char *) NULL)
    (void) CloneString(&montage_info->font,resource_info->font);
  (void) CopyMagickString(montage_info->filename,filename,MagickPathExtent);
  montage_image=MontageImageList(read_info,montage_info,GetFirstImageInList(
    images),exception);
  images=DestroyImageList(images);
  montage_info=DestroyMontageInfo(montage_info);
  read_info=DestroyImageInfo(read_info);
  XSetCursorState(display,windows,MagickFalse);
  if (montage_image == (Image *) NULL)
    return(montage_image);
  {
    const XClientReason
      reason = { windows->im_next_image, CurrentTime };

    XClientMessage(display,windows->image.id,windows->im_protocols,&reason);
  }
  return(montage_image);
}

typedef struct _DirectoryLoadInput
{
  Display
    *display;

  XWindows
    *windows;

  ExceptionInfo
    *exception;

  ImageInfo
    *read_info;

  char
    **filelist;

  int
    number_files;

  MagickBooleanType
    backdrop;

  XResourceInfo
    *background_resources;
} DirectoryLoadInput;

static void XLoadBackdropImage(const DirectoryLoadInput *load_input,
  Image *next_image)
{
  Display
    *const display = load_input->display;

  ExceptionInfo
    *const exception = load_input->exception;

  XWindows
    *const windows = load_input->windows;

  XResourceInfo
    *const background_resources = load_input->background_resources;

  (void) XDisplayBackgroundImage(display,background_resources,next_image,
    exception);
  XSetCursorState(display,windows,MagickTrue);
}

static MagickBooleanType IsDirectoryLoadCancelled(Image *images,const int i,
  const int number_files)
{
  if (images->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(images,LoadImageTag,(MagickOffsetType) i,
        (MagickSizeType) number_files);
      if (proceed == MagickFalse)
        return(MagickTrue);
    }
  return(MagickFalse);
}

static Image *XLoadDirectoryImages(const DirectoryLoadInput *load_input,
  Image *images)
{
  ExceptionInfo
    *const exception = load_input->exception;

  ImageInfo
    *const read_info = load_input->read_info;

  char
    **const filelist = load_input->filelist;

  const int
    number_files = load_input->number_files;

  const MagickBooleanType
    backdrop = load_input->backdrop;

  Image
    *next_image,
    *thumbnail_image;

  RectangleInfo
    geometry;

  int
    i;

  for (i=0; i < (int) number_files; i++)
  {
    (void) CopyMagickString(read_info->filename,filelist[i],MagickPathExtent);
    filelist[i]=DestroyString(filelist[i]);
    *read_info->magick='\0';
    next_image=ReadImage(read_info,exception);
    CatchException(exception);
    if (next_image == (Image *) NULL)
      continue;
    (void) DeleteImageProperty(next_image,"label");
    (void) SetImageProperty(next_image,"label",InterpretImageProperties(
      read_info,next_image,DefaultTileLabel,exception),exception);
    (void) ParseRegionGeometry(next_image,read_info->size,&geometry,
      exception);
    thumbnail_image=ThumbnailImage(next_image,geometry.width,
      geometry.height,exception);
    if (thumbnail_image != (Image *) NULL)
      {
        next_image=DestroyImage(next_image);
        next_image=thumbnail_image;
      }
    if (backdrop)
      XLoadBackdropImage(load_input,next_image);
    AppendImageToList(&images,next_image);
    if (IsDirectoryLoadCancelled(images,i,number_files) != MagickFalse)
      break;
  }
  return(images);
}

typedef struct _DirectoryListTargets
{
  char
    ***filelist;

  int
    *number_files;
} DirectoryListTargets;

static MagickBooleanType XExpandDirectoryFiles(char *filenames,
  const DirectoryListTargets *targets)
{
  char
    ***const filelist = targets->filelist;

  int
    *const number_files = targets->number_files;

  MagickStatusType
    status;

  /*
    Expand the filenames.
  */
  (*filelist)=(char **) AcquireMagickMemory(sizeof(**filelist));
  if ((*filelist) == (char **) NULL)
    {
      ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
        filenames);
      return(MagickFalse);
    }
  (*number_files)=1;
  (*filelist)[0]=filenames;
  status=ExpandFilenames(number_files,filelist);
  if ((status == MagickFalse) || ((*number_files) == 0))
    {
      if ((*number_files) == 0)
        ThrowXWindowException(ImageError,"NoImagesWereFound",filenames)
      else
        ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
          filenames);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static Image *XVisualDirectoryImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,ExceptionInfo *exception)
{
#define TileImageTag  "Scale/Image"
#define XClientName  "montage"

  char
    **filelist;

  Image
    *images;

  ImageInfo
    *read_info;

  int
    number_files;

  MagickBooleanType
    backdrop;

  static char
    filename[MagickPathExtent] = "\0",
    filenames[MagickPathExtent] = "*";

  XResourceInfo
    background_resources;

  const DirectoryInput
    input = { display, resource_info, windows, filename, exception };

  const DirectoryListTargets
    list_targets = { &filelist, &number_files };

  /*
    Request file name from user.
  */
  XFileBrowserWidget(display,windows,"Directory",filenames);
  if (*filenames == '\0')
    return((Image *) NULL);
  if (XExpandDirectoryFiles(filenames,&list_targets) == MagickFalse)
    return((Image *) NULL);
  /*
    Set image background resources.
  */
  background_resources=(*resource_info);
  background_resources.window_id=AcquireString("");
  (void) FormatLocaleString(background_resources.window_id,MagickPathExtent,
    "0x%lx",windows->image.id);
  background_resources.backdrop=MagickTrue;
  /*
    Read each image and convert them to a tile.
  */
  backdrop=(MagickBooleanType) ((windows->visual_info->klass == TrueColor) ||
    (windows->visual_info->klass == DirectColor));
  read_info=CloneImageInfo(resource_info->image_info);
  (void) SetImageOption(read_info,"jpeg:size","120x120");
  (void) CloneString(&read_info->size,DefaultTileGeometry);
  (void) SetImageInfoProgressMonitor(read_info,(MagickProgressMonitor) NULL,
    (void *) NULL);
  images=NewImageList();
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  {
    const DirectoryLoadInput
      load_input = { display, windows, exception, read_info, filelist,
        number_files, backdrop, &background_resources };

    images=XLoadDirectoryImages(&load_input,images);
  }
  filelist=(char **) RelinquishMagickMemory(filelist);
  if (images == (Image *) NULL)
    {
      read_info=DestroyImageInfo(read_info);
      XSetCursorState(display,windows,MagickFalse);
      ThrowXWindowException(ImageError,"NoImagesWereLoaded",filenames);
      return((Image *) NULL);
    }
  /*
    Create the Visual Image Directory.
  */
  return(XMontageImageDirectory(&input,read_info,images));
}
static void XLookupBackgroundWindowId(char *window_id,XWindowInfo *window_info,
  const Window root_window,Display *display)
{
  if (LocaleCompare(window_id,"root") == 0)
    (*window_info).id=root_window;
  else
    {
      if (isdigit((int) ((unsigned char) *window_id)) != 0)
        (*window_info).id=XWindowByID(display,root_window,
          (Window) strtol((char *) window_id,(char **) NULL,0));
      if ((*window_info).id == (Window) NULL)
        (*window_info).id=XWindowByName(display,root_window,window_id);
    }
}

static MagickBooleanType XFindBackgroundWindow(Display *display,
  char *window_id,const Window root_window,XWindowInfo *window_info)
{
  XLookupBackgroundWindowId(window_id,window_info,root_window,display);
  if ((*window_info).id == (Window) NULL)
    {
      ThrowXWindowException(XServerError,"NoWindowWithSpecifiedIDExists",
        window_id);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void XGetBackgroundVisualType(Display *display,
  const XWindowInfo *window_info,XWindowAttributes *window_attributes,
  char *visual_type)
{
  int
    status;

  (*window_attributes).width=XDisplayWidth(display,XDefaultScreen(display));
  (*window_attributes).height=XDisplayHeight(display,XDefaultScreen(display));
  (void) CopyMagickString(visual_type,"default",MagickPathExtent);
  status=XGetWindowAttributes(display,(*window_info).id,window_attributes);
  if (status != 0)
    (void) FormatLocaleString(visual_type,MagickPathExtent,"0x%lx",
      XVisualIDFromVisual((*window_attributes).visual));
}

static void XFitBackgroundWindow(const XWindowAttributes *window_attributes,
  XWindowInfo *window_info)
{
  char
    geometry[MagickPathExtent];

  RectangleInfo
    geometry_info;

  (void) FormatLocaleString(geometry,MagickPathExtent,"%ux%u+0+0>",
    (*window_attributes).width,(*window_attributes).height);
  geometry_info.width=(*window_info).width;
  geometry_info.height=(*window_info).height;
  geometry_info.x=(ssize_t) (*window_info).x;
  geometry_info.y=(ssize_t) (*window_info).y;
  (void) ParseMetaGeometry(geometry,&geometry_info.x,&geometry_info.y,
    &geometry_info.width,&geometry_info.height);
  (*window_info).width=(unsigned int) geometry_info.width;
  (*window_info).height=(unsigned int) geometry_info.height;
  (*window_info).x=(int) geometry_info.x;
  (*window_info).y=(int) geometry_info.y;
}

typedef struct _BackgroundTargets
{
  XStandardColormap
    **map_info;

  XPixelInfo
    *pixel;

  XResourceInfo
    *resources;

  XVisualInfo
    **visual_info;

  XWindowInfo
    *window_info;
} BackgroundTargets;

static void XInitializeBackgroundVisual(Display *display,const Image *image,
  char *visual_type,const BackgroundTargets *targets)
{
  XStandardColormap
    **const map_info = targets->map_info;

  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  /*
    Allocate standard colormap.
  */
  (*map_info)=XAllocStandardColormap();
  if ((*map_info) == (XStandardColormap *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"MemoryAllocationFailed",
      image->filename);
  (*map_info)->colormap=(Colormap) NULL;
  (*pixel).pixels=(unsigned long *) NULL;
  /*
    Initialize visual info.
  */
  (*resources).map_type=(char *) NULL;
  (*resources).visual_type=visual_type;
  (*visual_info)=XBestVisualInfo(display,(*map_info),resources);
  if ((*visual_info) == (XVisualInfo *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToGetVisual",
      (*resources).visual_type);
  /*
    Initialize window info.
  */
  (*window_info).ximage=(XImage *) NULL;
  (*window_info).matte_image=(XImage *) NULL;
  (*window_info).pixmap=(Pixmap) NULL;
  (*window_info).matte_pixmap=(Pixmap) NULL;
}

static void XInitializeBackgroundColormap(Display *display,Image *image,
  ExceptionInfo *exception,const BackgroundTargets *targets)
{
  XStandardColormap
    **const map_info = targets->map_info;

  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  (*resources).colormap=SharedColormap;
  {
    const XStandardColormapRequest
      colormap_request = { (*visual_info), resources, image, (*map_info),
        pixel };

    XMakeStandardColormap(display,&colormap_request,exception);
  }
}

static void XInitializeBackgroundContext(XPixelInfo *pixel,Display *display,
  XWindowInfo *window_info,Image *image)
{
  XGCValues
    context_values;

  context_values.background=(*pixel).foreground_color.pixel;
  context_values.foreground=(*pixel).background_color.pixel;
  (*pixel).annotate_context=XCreateGC(display,(*window_info).id,
    (size_t) (GCBackground | GCForeground),&context_values);
  if ((*pixel).annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      image->filename);
}

static void XInitializeBackgroundAttributes(Display *display,
  const BackgroundTargets *targets)
{
  XStandardColormap
    **const map_info = targets->map_info;

  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  (*window_info).name=AcquireString("\0");
  (*window_info).icon_name=AcquireString("\0");
  {
    const XWindowInfoSource
      source = { (*visual_info), (*map_info), pixel, (XFontStruct *) NULL };

    XGetWindowInfo(display,&source,resources,window_info);
  }
}

static void XCreateBackgroundWindowInfo(Display *display,Image *image,
  ExceptionInfo *exception,const BackgroundTargets *targets)
{
  XPixelInfo
    *const pixel = targets->pixel;

  XWindowInfo
    *const window_info = targets->window_info;

  /*
    Initialize Standard Colormap.
  */
  XInitializeBackgroundColormap(display,image,exception,targets);
  /*
    Graphic context superclass.
  */
  XInitializeBackgroundContext(pixel,display,window_info,image);
  /*
    Initialize Image window attributes.
  */
  XInitializeBackgroundAttributes(display,targets);
}

static void XSizeBackgroundWindow(const Image *image,XWindowInfo *window_info)
{
  (*window_info).width=(unsigned int) image->columns;
  (*window_info).height=(unsigned int) image->rows;
  if ((image->columns != (*window_info).width) ||
      (image->rows != (*window_info).height))
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      image->filename);
}

static void XMakeBackgroundXImage(Display *display,Image *image,
  ExceptionInfo *exception,const BackgroundTargets *targets)
{
  XResourceInfo
    *const resources = targets->resources;

  XWindowInfo
    *const window_info = targets->window_info;

  int
    status;

  {
    const XMakeImageRequest
      image_request = { window_info, image, (*window_info).width,
        (*window_info).height };

    status=XMakeImage(display,resources,&image_request,
      exception) == MagickFalse ? 0 : 1;
  }
  if (status == MagickFalse)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      image->filename);
  (*window_info).x=0;
  (*window_info).y=0;
}

typedef struct _BackgroundInput
{
  Display
    *display;

  Image
    *image;

  const XWindowAttributes
    *window_attributes;
} BackgroundInput;

static void XCenterBackdropImage(const BackgroundInput *input,
  const BackgroundTargets *targets,int *width,int *height)
{
  const XWindowAttributes
    *const window_attributes = input->window_attributes;

  XResourceInfo
    *const resources = targets->resources;

  XWindowInfo
    *const window_info = targets->window_info;

  if ((*resources).backdrop != MagickFalse)
    {
      /*
        Center image on window.
      */
      (*window_info).x=((*window_attributes).width/2)-
        ((*window_info).ximage->width/2);
      (*window_info).y=((*window_attributes).height/2)-
        ((*window_info).ximage->height/2);
      (*width)=(*window_attributes).width;
      (*height)=(*window_attributes).height;
    }
}

static void XAdjustBackgroundSize(const BackgroundInput *input,
  const BackgroundTargets *targets,int *width,int *height)
{
  Display
    *const display = input->display;

  Image
    *const image = input->image;

  const XWindowAttributes
    *const window_attributes = input->window_attributes;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  (*width)=(int) (*window_info).width;
  (*height)=(int) (*window_info).height;
  XCenterBackdropImage(input,targets,width,height);
  if (((*resources).image_geometry != (char *) NULL) &&
      (*(*resources).image_geometry != '\0'))
    {
      char
        default_geometry[MagickPathExtent];

      int
        flags,
        gravity;

      XSizeHints
        *size_hints;

      /*
        User specified geometry.
      */
      size_hints=XAllocSizeHints();
      if (size_hints == (XSizeHints *) NULL)
        ThrowXWindowFatalException(ResourceLimitFatalError,
          "MemoryAllocationFailed",image->filename);
      size_hints->flags=0L;
      (void) FormatLocaleString(default_geometry,MagickPathExtent,"%dx%d",
        (*width),(*height));
      flags=XWMGeometry(display,(*visual_info)->screen,
        (*resources).image_geometry,default_geometry,
        (*window_info).border_width,size_hints,&(*window_info).x,
        &(*window_info).y,width,height,&gravity);
      if (flags & (XValue | YValue))
        {
          (*width)=(*window_attributes).width;
          (*height)=(*window_attributes).height;
        }
      (void) XFree((void *) size_hints);
    }
}

static void XCreateBackgroundPixmap(const BackgroundInput *input,
  const BackgroundTargets *targets,const int width,const int height)
{
  Display
    *const display = input->display;

  Image
    *const image = input->image;

  XWindowInfo
    *const window_info = targets->window_info;

  (*window_info).pixmap=XCreatePixmap(display,(*window_info).id,
    (unsigned int) width,(unsigned int) height,(*window_info).depth);
  if ((*window_info).pixmap == (Pixmap) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXPixmap",
      image->filename);
}

static void XShowBackgroundPixmap(const BackgroundInput *input,
  const BackgroundTargets *targets,const int width,const int height)
{
  Display
    *const display = input->display;

  Image
    *const image = input->image;

  XWindowInfo
    *const window_info = targets->window_info;

  size_t
    delay;

  if (((unsigned int) width > (*window_info).width) ||
      ((unsigned int) height > (*window_info).height))
    (void) XFillRectangle(display,(*window_info).pixmap,
      (*window_info).annotate_context,0,0,(unsigned int) width,
      (unsigned int) height);
  (void) XPutImage(display,(*window_info).pixmap,
    (*window_info).annotate_context,(*window_info).ximage,0,0,
    (*window_info).x,(*window_info).y,(unsigned int) (*window_info).width,
    (unsigned int) (*window_info).height);
  (void) XSetWindowBackgroundPixmap(display,(*window_info).id,
    (*window_info).pixmap);
  (void) XClearWindow(display,(*window_info).id);
  delay=1000*image->delay/(size_t) MagickMax(image->ticks_per_second,1L);
  XDelay(display,delay == 0UL ? 10UL : delay);
  (void) XSync(display,MagickFalse);
}

static void XDisplayBackgroundPixmap(const BackgroundInput *input,
  const BackgroundTargets *targets,const int width,const int height)
{
  /*
    Create the X pixmap.
  */
  XCreateBackgroundPixmap(input,targets,width,height);
  /*
    Display pixmap on the window.
  */
  XShowBackgroundPixmap(input,targets,width,height);
}

#endif
