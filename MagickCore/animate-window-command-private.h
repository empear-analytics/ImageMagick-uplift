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

  MagickCore private methods of animate-window.c: the background animation
  helpers of XAnimateBackgroundImage, from SceneCompare to
  XReleaseBackgroundImages; included by that file only, after
  animate-window-menu-private.h (the typedefs and the animate commands).
*/
#ifndef MAGICKCORE_ANIMATE_WINDOW_COMMAND_PRIVATE_H
#define MAGICKCORE_ANIMATE_WINDOW_COMMAND_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int SceneCompare(const void *x,const void *y)
{
  const Image
    **image_1,
    **image_2;

  image_1=(const Image **) x;
  image_2=(const Image **) y;
  return((int) ((*image_1)->scene-(*image_2)->scene));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static MagickBooleanType IsTrueColorMap(const char *map_type,
  const XVisualInfo *visual_info)
{
  return(((map_type != (char *) NULL) ||
    (visual_info->klass == TrueColor) ||
    (visual_info->klass == DirectColor)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType IsColormapMismatch(const Image *next,
  const Image *images,const XVisualInfo *visual_info)
{
  return(((next->storage_class == DirectClass) ||
    (next->colors != images->colors) ||
    (next->colors > (size_t) visual_info->colormap_size)) ? MagickTrue :
    MagickFalse);
}

static ssize_t FindColormapDifference(const Image *next,const Image *images)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) images->colors; i++)
    if (IsPixelInfoEquivalent(next->colormap+i,images->colormap+i) == MagickFalse)
      break;
  return(i);
}

static void XRemapBackgroundImages(Image *images,
  const XVisualInfo *visual_info,const XResourceInfo resources,
  ExceptionInfo *exception)
{
  Image
    *next;

  ssize_t
    i;

  /*
    Determine if the sequence of images has the identical colormap.
  */
  for (next=images; next != (Image *) NULL; )
  {
    next->alpha_trait=UndefinedPixelTrait;
    if (IsColormapMismatch(next,images,visual_info) != MagickFalse)
      break;
    i=FindColormapDifference(next,images);
    if (i < (ssize_t) images->colors)
      break;
    next=GetNextImageInList(next);
  }
  if (next != (Image *) NULL)
    (void) RemapImages(resources.quantize_info,images,(Image *) NULL,
      exception);
}

static MagickBooleanType XNextBackgroundEvent(Display *display,XEvent *event)
{
  if (XEventsQueued(display,QueuedAfterFlush) > 0)
    {
      (void) XNextEvent(display,event);
      if ((*event).type == DestroyNotify)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static void XSortSceneList(Image **image_list,const size_t number_scenes)
{
  ssize_t
    scene;

  for (scene=0; scene < (ssize_t) number_scenes; scene++)
    if (image_list[scene]->scene == 0)
      break;
  if (scene == (ssize_t) number_scenes)
    qsort((void *) image_list,number_scenes,sizeof(Image *),SceneCompare);
}

typedef struct _AnimateBackgroundSetup
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  ExceptionInfo
    *exception;

  Image
    *images,
    **image_list;

  size_t
    number_scenes;

  XVisualInfo
    *visual_info;

  XStandardColormap
    *map_info;
} AnimateBackgroundSetup;

typedef struct _AnimateBackgroundTargets
{
  XWindowInfo
    *window_info;

  XResourceInfo
    *resources;

  XWindowAttributes
    *window_attributes;

  unsigned int
    *width,
    *height;

  XPixelInfo
    *pixel;
} AnimateBackgroundTargets;

static void XCreateBackgroundXImage(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets)
{
  char
    geometry[MagickPathExtent];

  Display
    *const display = setup->display;

  Image
    **const image_list = setup->image_list;

  MagickStatusType
    status;

  RectangleInfo
    geometry_info;

  XResourceInfo
    *const resources = targets->resources;

  XWindowAttributes
    *const window_attributes = targets->window_attributes;

  XWindowInfo
    *const window_info = targets->window_info;

  (*window_info).width=(unsigned int) image_list[0]->columns;
  (*window_info).height=(unsigned int) image_list[0]->rows;
  if ((image_list[0]->columns != (*window_info).width) ||
      (image_list[0]->rows != (*window_info).height))
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      image_list[0]->filename);
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
  status=XMakeImage(display,resources,window_info,image_list[0],
    (*window_info).width,(*window_info).height,setup->exception);
  if (status == MagickFalse)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      setup->images->filename);
  (*window_info).x=0;
  (*window_info).y=0;
  if (setup->resource_info->debug != MagickFalse)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "Image: %s[%.17g] %.17gx%.17g ",image_list[0]->filename,(double)
        image_list[0]->scene,(double) image_list[0]->columns,(double)
        image_list[0]->rows);
      if (image_list[0]->colors != 0)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"%.17gc ",(double)
          image_list[0]->colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"%s",
        image_list[0]->magick);
    }
}

static void XAdjustBackgroundSize(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets)
{
  unsigned int
    *const height = targets->height,
    *const width = targets->width;

  XResourceInfo
    *const resources = targets->resources;

  XWindowAttributes
    *const window_attributes = targets->window_attributes;

  XWindowInfo
    *const window_info = targets->window_info;

  (*width)=(*window_info).width;
  (*height)=(*window_info).height;
  if ((*resources).backdrop != MagickFalse)
    {
      /*
        Center image on window.
      */
      (*window_info).x=(int) ((*window_attributes).width/2)-
        ((*window_info).ximage->width/2);
      (*window_info).y=(int) ((*window_attributes).height/2)-
        ((*window_info).ximage->height/2);
      (*width)=(unsigned int) (*window_attributes).width;
      (*height)=(unsigned int) (*window_attributes).height;
    }
  if ((*resources).image_geometry != (char *) NULL)
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
          "MemoryAllocationFailed",setup->images->filename);
      size_hints->flags=0L;
      (void) FormatLocaleString(default_geometry,MagickPathExtent,"%ux%u",
        (*width),(*height));
      flags=XWMGeometry(setup->display,setup->visual_info->screen,
        (*resources).image_geometry,default_geometry,
        (*window_info).border_width,size_hints,&(*window_info).x,
        &(*window_info).y,(int *) width,(int *) height,&gravity);
      if (((flags & (XValue | YValue))) != 0)
        {
          (*width)=(unsigned int) (*window_attributes).width;
          (*height)=(unsigned int) (*window_attributes).height;
        }
      (void) XFree((void *) size_hints);
    }
}

static void XCreateBackgroundPixmaps(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets)
{
  Display
    *const display = setup->display;

  unsigned int
    *const height = targets->height,
    *const width = targets->width;

  XWindowInfo
    *const window_info = targets->window_info;

  (*window_info).pixmap=XCreatePixmap(display,(*window_info).id,
    (unsigned int) (*width),(unsigned int) (*height),(*window_info).depth);
  if ((*window_info).pixmap == (Pixmap) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXPixmap",
      setup->images->filename);
  /*
    Display pixmap on the window.
  */
  if (((unsigned int) (*width) > (*window_info).width) ||
      ((unsigned int) (*height) > (*window_info).height))
    (void) XFillRectangle(display,(*window_info).pixmap,
      (*window_info).annotate_context,0,0,(unsigned int) (*width),
      (unsigned int) (*height));
  (void) XPutImage(display,(*window_info).pixmap,
    (*window_info).annotate_context,(*window_info).ximage,0,0,
    (*window_info).x,(*window_info).y,(*window_info).width,
    (*window_info).height);
  (void) XSetWindowBackgroundPixmap(display,(*window_info).id,
    (*window_info).pixmap);
  (void) XClearWindow(display,(*window_info).id);
  /*
    Initialize image pixmaps structure.
  */
  (*window_info).pixmaps=(Pixmap *) AcquireQuantumMemory(setup->number_scenes,
    sizeof(*(*window_info).pixmaps));
  (*window_info).matte_pixmaps=(Pixmap *) AcquireQuantumMemory(
    setup->number_scenes,sizeof(*(*window_info).matte_pixmaps));
  if (((*window_info).pixmaps == (Pixmap *) NULL) ||
      ((*window_info).matte_pixmaps == (Pixmap *) NULL))
    ThrowXWindowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      setup->images->filename);
  (*window_info).pixmaps[0]=(*window_info).pixmap;
  (*window_info).matte_pixmaps[0]=(*window_info).pixmap;
}

static void XCreateSceneXImage(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets,const int scene)
{
  Image
    **const image_list = setup->image_list;

  MagickStatusType
    status;

  unsigned int
    columns,
    rows;

  XResourceInfo
    *const resources = targets->resources;

  XStandardColormap
    *const map_info = setup->map_info;

  XVisualInfo
    *const visual_info = setup->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  (*window_info).pixmap=(Pixmap) NULL;
  (*window_info).matte_pixmap=(Pixmap) NULL;
  if ((IsTrueColorMap((*resources).map_type,visual_info) != MagickFalse) &&
      (image_list[scene]->storage_class == PseudoClass))
    {
      const XPixelInfoSource
        pixel_source = { visual_info, map_info, resources };

      XGetPixelInfo(setup->display,&pixel_source,image_list[scene],
        (*window_info).pixel_info);
    }
  columns=(unsigned int) image_list[scene]->columns;
  rows=(unsigned int) image_list[scene]->rows;
  if ((image_list[scene]->columns != columns) ||
      (image_list[scene]->rows != rows))
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      image_list[scene]->filename);
  status=XMakeImage(setup->display,resources,window_info,image_list[scene],
    columns,rows,setup->exception);
  if (status == MagickFalse)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      setup->images->filename);
  if (setup->resource_info->debug != MagickFalse)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "Image: [%.17g] %s %.17gx%.17g ",(double) image_list[scene]->scene,
        image_list[scene]->filename,(double) columns,(double) rows);
      if (image_list[scene]->colors != 0)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"%.17gc ",(double)
          image_list[scene]->colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"%s",
        image_list[scene]->magick);
    }
}

static void XCreateScenePixmap(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets,const int scene)
{
  Display
    *const display = setup->display;

  unsigned int
    *const height = targets->height,
    *const width = targets->width;

  XWindowInfo
    *const window_info = targets->window_info;

  (*window_info).pixmap=XCreatePixmap(display,(*window_info).id,(*width),
    (*height),(*window_info).depth);
  if ((*window_info).pixmap == (Pixmap) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXPixmap",
      setup->images->filename);
  /*
    Display pixmap on the window.
  */
  if (((*width) > (*window_info).width) || ((*height) > (*window_info).height))
    (void) XFillRectangle(display,(*window_info).pixmap,
      (*window_info).annotate_context,0,0,(*width),(*height));
  (void) XPutImage(display,(*window_info).pixmap,
    (*window_info).annotate_context,(*window_info).ximage,0,0,
    (*window_info).x,(*window_info).y,(*window_info).width,
    (*window_info).height);
  (void) XSetWindowBackgroundPixmap(display,(*window_info).id,
    (*window_info).pixmap);
  (void) XClearWindow(display,(*window_info).id);
  (*window_info).pixmaps[scene]=(*window_info).pixmap;
  (*window_info).matte_pixmaps[scene]=(*window_info).matte_pixmap;
  if (setup->image_list[scene]->alpha_trait)
    (void) XClearWindow(display,(*window_info).id);
}

static void XRunBackgroundAnimation(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets)
{
  Display
    *const display = setup->display;

  Image
    **const image_list = setup->image_list;

  int
    scene;

  size_t
    delay;

  ssize_t
    iterations;

  XEvent
    event;

  XResourceInfo
    *const resources = targets->resources;

  XWindowInfo
    *const window_info = targets->window_info;

  (void) XSelectInput(display,(*window_info).id,SubstructureNotifyMask);
  event.type=Expose;
  iterations=0;
  do
  {
    for (scene=0; scene < (int) setup->number_scenes; scene++)
    {
      if (XNextBackgroundEvent(display,&event) == MagickFalse)
        break;
      (*window_info).pixmap=(*window_info).pixmaps[scene];
      (*window_info).matte_pixmap=(*window_info).matte_pixmaps[scene];
      (void) XSetWindowBackgroundPixmap(display,(*window_info).id,
        (*window_info).pixmap);
      (void) XClearWindow(display,(*window_info).id);
      (void) XSync(display,MagickFalse);
      delay=1000*image_list[scene]->delay/(size_t) MagickMax(
        image_list[scene]->ticks_per_second,1L);
      XDelay(display,(*resources).delay*(delay == 0 ? 10 : delay));
    }
    iterations++;
    if (iterations == (ssize_t) image_list[0]->iterations)
      break;
  } while (event.type != DestroyNotify);
}

static void XMakeBackgroundColormap(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets)
{
  ExceptionInfo
    *const exception = setup->exception;

  Image
    *display_image,
    **const image_list = setup->image_list;

  int
    scene;

  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resource_info = setup->resource_info,
    *const resources = targets->resources;

  XVisualInfo
    *const visual_info = setup->visual_info;

  (*resources).colormap=SharedColormap;
  display_image=image_list[0];
  for (scene=0; scene < (int) setup->number_scenes; scene++)
  {
    if (IsTrueColorMap(resource_info->map_type,visual_info) != MagickFalse)
      (void) SetImageType(image_list[scene],image_list[scene]->alpha_trait ==
        BlendPixelTrait ? TrueColorType : TrueColorAlphaType,exception);
    if ((display_image->columns < image_list[scene]->columns) &&
        (display_image->rows < image_list[scene]->rows))
      display_image=image_list[scene];
  }
  if (IsTrueColorMap(resource_info->map_type,visual_info) != MagickFalse)
    (void) SetImageType(display_image,display_image->alpha_trait !=
      BlendPixelTrait ? TrueColorType : TrueColorAlphaType,exception);
  XMakeStandardColormap(setup->display,visual_info,resources,display_image,
    setup->map_info,pixel,exception);
}

static void XInitializeBackgroundWindow(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets)
{
  Display
    *const display = setup->display;

  XGCValues
    context_values;

  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resources = targets->resources;

  XWindowInfo
    *const window_info = targets->window_info;

  context_values.background=(*pixel).background_color.pixel;
  context_values.foreground=(*pixel).foreground_color.pixel;
  (*pixel).annotate_context=XCreateGC(display,(*window_info).id,(unsigned long)
    (GCBackground | GCForeground),&context_values);
  if ((*pixel).annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      setup->images->filename);
  /*
    Initialize Image window attributes.
  */
  XGetWindowInfo(display,setup->visual_info,setup->map_info,pixel,
    (XFontStruct *) NULL,resources,window_info);
}

static void XCreateBackgroundScenes(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets)
{
  Image
    **const image_list = setup->image_list;

  int
    scene;

  size_t
    delay;

  XResourceInfo
    *const resources = targets->resources;

  for (scene=1; scene < (int) setup->number_scenes; scene++)
  {
    /*
      Create X image.
    */
    XCreateSceneXImage(setup,targets,scene);
    /*
      Create the X pixmap.
    */
    XCreateScenePixmap(setup,targets,scene);
    delay=1000*image_list[scene]->delay/(size_t) MagickMax(
      image_list[scene]->ticks_per_second,1L);
    XDelay(setup->display,(*resources).delay*(delay == 0 ? 10 : delay));
  }
}

static Image **
XSortAnimateScenes(Image *,const size_t,ExceptionInfo *);

static void XGetBackgroundVisualType(Display *display,
  const AnimateBackgroundTargets *targets,char *visual_type)
{
  MagickStatusType
    status;

  XWindowAttributes
    *const window_attributes = targets->window_attributes;

  XWindowInfo
    *const window_info = targets->window_info;

  (*window_attributes).width=XDisplayWidth(display,XDefaultScreen(display));
  (*window_attributes).height=XDisplayHeight(display,XDefaultScreen(display));
  (void) CopyMagickString(visual_type,"default",MagickPathExtent);
  status=XGetWindowAttributes(display,(*window_info).id,
    window_attributes) != 0 ? MagickTrue : MagickFalse;
  if (status != MagickFalse)
    (void) FormatLocaleString(visual_type,MagickPathExtent,"0x%lx",
      XVisualIDFromVisual((*window_attributes).visual));
}

typedef struct _AnimateBackgroundEarlyTargets
{
  Window
    *root_window;

  XResourceInfo
    *resources;

  XWindowInfo
    *window_info;

  XPixelInfo
    *pixel;

  XStandardColormap
    **map_info;

  XVisualInfo
    **visual_info;
} AnimateBackgroundEarlyTargets;

static void XFindBackgroundWindow(Display *display,
  XResourceInfo *resource_info,const AnimateBackgroundEarlyTargets *targets)
{
  Window
    *const root_window = targets->root_window;

  XResourceInfo
    *const resources = targets->resources;

  XWindowInfo
    *const window_info = targets->window_info;

  (*resources)=(*resource_info);
  (*window_info).id=(Window) NULL;
  (*root_window)=XRootWindow(display,XDefaultScreen(display));
  if (LocaleCompare((*resources).window_id,"root") == 0)
    (*window_info).id=(*root_window);
  else
    {
      if (isdigit((int) ((unsigned char) *(*resources).window_id)) != 0)
        (*window_info).id=XWindowByID(display,(*root_window),
          (Window) strtol((char *) (*resources).window_id,(char **) NULL,0));
      if ((*window_info).id == (Window) NULL)
        (*window_info).id=
          XWindowByName(display,(*root_window),(*resources).window_id);
    }
}

static void XCreateBackgroundVisual(Display *display,const char *filename,
  char *visual_type,const AnimateBackgroundEarlyTargets *targets)
{
  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resources = targets->resources;

  XStandardColormap
    **const map_info = targets->map_info;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  if ((*visual_info) == (XVisualInfo *) NULL)
    {
      /*
        Allocate standard colormap.
      */
      (*map_info)=XAllocStandardColormap();
      if ((*map_info) == (XStandardColormap *) NULL)
        ThrowXWindowFatalException(ResourceLimitFatalError,
          "MemoryAllocationFailed",filename);
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
          filename);
      /*
        Initialize window info.
      */
      (*window_info).ximage=(XImage *) NULL;
      (*window_info).matte_image=(XImage *) NULL;
      (*window_info).pixmap=(Pixmap) NULL;
      (*window_info).matte_pixmap=(Pixmap) NULL;
    }
}

static Image *XPrepareBackgroundImages(Display *display,Image *images,
  ExceptionInfo *exception,const AnimateBackgroundEarlyTargets *targets)
{
  Image
    *coalesce_image;

  Window
    *const root_window = targets->root_window;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  if ((*window_info).id == (*root_window))
    XDestroyWindowColors(display,(*root_window));
  coalesce_image=CoalesceImages(images,exception);
  if (coalesce_image == (Image *) NULL)
    ThrowXWindowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      images->filename);
  images=coalesce_image;
  if ((*resources).map_type == (char *) NULL)
    if (((*visual_info)->klass != TrueColor) &&
        ((*visual_info)->klass != DirectColor))
      XRemapBackgroundImages(images,(*visual_info),(*resources),exception);
  return(images);
}

static void XReleaseBackgroundImages(Display *display,Image *images,
  Image **image_list)
{
  (void) XSync(display,MagickFalse);
  image_list=(Image **) RelinquishMagickMemory(image_list);
  images=DestroyImageList(images);
}

#endif
