/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               X   X  W   W  IIIII  N   N  DDDD    OOO   W   W               %
%                X X   W   W    I    NN  N  D   D  O   O  W   W               %
%                 X    W W W    I    N N N  D   D  O   O  W W W               %
%                X X   WW WW    I    N  NN  D   D  O   O  WW WW               %
%               X   X  W   W  IIIII  N   N  DDDD    OOO   W   W               %
%                                                                             %
%                  V   V  IIIII  SSSSS  U   U   AAA   L                       %
%                  V   V    I    SS     U   U  A   A  L                       %
%                  V   V    I     SSS   U   U  AAAAA  L                       %
%                   V V     I       SS  U   U  A   A  L                       %
%                    V    IIIII  SSSSS   UUU   A   A  LLLLL                   %
%                                                                             %
%                                                                             %
%            MagickCore X Window Visual and Initialization Methods            %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                  July 1992                                  %
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
#include "MagickCore/animate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/composite.h"
#include "MagickCore/constitute.h"
#include "MagickCore/display.h"
#include "MagickCore/distort.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/identify.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/locale_.h"
#include "MagickCore/log.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/resize.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/widget.h"
#include "MagickCore/widget-private.h"
#include "MagickCore/xwindow.h"
#include "MagickCore/xwindow-private.h"
#include "MagickCore/version.h"
#if defined(__BEOS__)
#include <OS.h>
#endif
#if defined(MAGICKCORE_X11_DELEGATE)
#include <X11/Xproto.h>
#include <X11/Xlocale.h>
#if defined(MAGICK_HAVE_POLL)
# include <sys/poll.h>
#endif
#if defined(MAGICKCORE_HAVE_SHARED_MEMORY)
#if defined(MAGICKCORE_HAVE_MACHINE_PARAM_H)
# include <machine/param.h>
#endif
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif
#if defined(MAGICKCORE_HAVE_SHAPE)
#include <X11/extensions/shape.h>
#endif

/*
  Forward declarations.
*/
static const char
  *XVisualClassName(const int);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X B e s t V i s u a l I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XBestVisualInfo() returns visual information for a visual that is the "best"
%  the server supports.  "Best" is defined as:
%
%    1. Restrict the visual list to those supported by the default screen.
%
%    2. If a visual type is specified, restrict the visual list to those of
%       that type.
%
%    3. If a map type is specified, choose the visual that matches the id
%       specified by the Standard Colormap.
%
%    4  From the list of visuals, choose one that can display the most
%       simultaneous colors.  If more than one visual can display the same
%       number of simultaneous colors, one is chosen based on a rank.
%
%  The format of the XBestVisualInfo method is:
%
%      XVisualInfo *XBestVisualInfo(Display *display,
%        XStandardColormap *map_info,XResourceInfo *resource_info)
%
%  A description of each parameter follows:
%
%    o visual_info: XBestVisualInfo returns a pointer to a X11 XVisualInfo
%      structure.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o map_info: If map_type is specified, this structure is initialized
%      with info from the Standard Colormap.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
*/
#define MaxStandardColormaps  7
#define XVisualColormapSize(visual_info) MagickMin((unsigned long int) (\
  (visual_info->klass == TrueColor) || (visual_info->klass == DirectColor) ? \
   visual_info->red_mask | visual_info->green_mask | visual_info->blue_mask : \
   (unsigned long) visual_info->colormap_size),1UL << visual_info->depth)

typedef struct _StandardColormapTargets
{
  Status
    *status;

  XStandardColormap
    **map_list;

  int
    *number_maps;
} StandardColormapTargets;

static void XGetNamedStandardColormap(Display *display,
  const Window root_window,const char *map_type,
  const StandardColormapTargets *targets)
{
  Atom
    map_property;

  char
    map_name[MagickPathExtent];

  Status
    *const status = targets->status;

  XStandardColormap
    **const map_list = targets->map_list;

  int
    *const number_maps = targets->number_maps;

  /*
    User specified Standard Colormap.
  */
  (void) FormatLocaleString((char *) map_name,MagickPathExtent,
    "RGB_%s_MAP",map_type);
  LocaleUpper(map_name);
  map_property=XInternAtom(display,(char *) map_name,MagickTrue);
  if (map_property != (Atom) NULL)
    (*status)=XGetRGBColormaps(display,root_window,map_list,number_maps,
      map_property);
}

static void XGetListedStandardColormap(Display *display,
  const Window root_window,XResourceInfo *resource_info,
  const StandardColormapTargets *targets)
{
  Atom
    map_property;

  int
    i;

  Status
    *const status = targets->status;

  XStandardColormap
    **const map_list = targets->map_list;

  int
    *const number_maps = targets->number_maps;

  static const char
    *colormap[MaxStandardColormaps]=
    {
      "_HP_RGB_SMOOTH_MAP_LIST",
      "RGB_BEST_MAP",
      "RGB_DEFAULT_MAP",
      "RGB_GRAY_MAP",
      "RGB_RED_MAP",
      "RGB_GREEN_MAP",
      "RGB_BLUE_MAP",
    };

  /*
    Choose a standard colormap from a list.
  */
  for (i=0; i < MaxStandardColormaps; i++)
  {
    map_property=XInternAtom(display,(char *) colormap[i],MagickTrue);
    if (map_property == (Atom) NULL)
      continue;
    (*status)=XGetRGBColormaps(display,root_window,map_list,number_maps,
      map_property);
    if ((*status) != False)
      break;
  }
  resource_info->color_recovery=i == 0 ? MagickTrue : MagickFalse;
}

static XVisualInfo *XPickRichestVisual(XVisualInfo *visual_list,
  const int number_visuals)
{
  static const unsigned int
    rank[]=
      {
        StaticGray,
        GrayScale,
        StaticColor,
        DirectColor,
        TrueColor,
        PseudoColor
      };

  int
    i;

  XVisualInfo
    *p,
    *visual_info;

  /*
    Pick one visual that displays the most simultaneous colors.
  */
  visual_info=visual_list;
  p=visual_list;
  for (i=1; i < number_visuals; i++)
  {
    p++;
    if (XVisualColormapSize(p) > XVisualColormapSize(visual_info))
      visual_info=p;
    else
      if ((XVisualColormapSize(p) == XVisualColormapSize(visual_info)) &&
          (rank[p->klass] > rank[visual_info->klass]))
        visual_info=p;
  }
  return(visual_info);
}

static void XRestrictVisualByID(Display *display,const char *visual_type,
  int *visual_mask,XVisualInfo *visual_template)
{
  if (LocaleCompare("default",visual_type) == 0)
    {
      (*visual_mask)|=VisualIDMask;
      (*visual_template).visualid=XVisualIDFromVisual(
        XDefaultVisual(display,XDefaultScreen(display)));
    }
  else if (isdigit((int) ((unsigned char) *visual_type)) != 0)
    {
      (*visual_mask)|=VisualIDMask;
      (*visual_template).visualid=(size_t)
        strtol(visual_type,(char **) NULL,0);
    }
  else
    ThrowXWindowException(XServerError,
      "UnrecognizedVisualSpecifier",visual_type);
}

static void XRestrictVisualByType(Display *display,const char *visual_type,
  int *visual_mask,XVisualInfo *visual_template)
{
  /*
    Restrict visual search by class or visual id.
  */
  if (LocaleCompare("staticgray",visual_type) == 0)
    {
      (*visual_mask)|=VisualClassMask;
      (*visual_template).klass=StaticGray;
    }
  else if (LocaleCompare("grayscale",visual_type) == 0)
    {
      (*visual_mask)|=VisualClassMask;
      (*visual_template).klass=GrayScale;
    }
  else if (LocaleCompare("staticcolor",visual_type) == 0)
    {
      (*visual_mask)|=VisualClassMask;
      (*visual_template).klass=StaticColor;
    }
  else if (LocaleCompare("pseudocolor",visual_type) == 0)
    {
      (*visual_mask)|=VisualClassMask;
      (*visual_template).klass=PseudoColor;
    }
  else if (LocaleCompare("truecolor",visual_type) == 0)
    {
      (*visual_mask)|=VisualClassMask;
      (*visual_template).klass=TrueColor;
    }
  else if (LocaleCompare("directcolor",visual_type) == 0)
    {
      (*visual_mask)|=VisualClassMask;
      (*visual_template).klass=DirectColor;
    }
  else
    XRestrictVisualByID(display,visual_type,visual_mask,visual_template);
}

static void XRestrictVisualByScreen(Display *display,
  const XResourceInfo *resource_info,int *visual_mask,
  XVisualInfo *visual_template)
{
  size_t
    one;

  (*visual_mask)=VisualScreenMask;
  (*visual_template).screen=XDefaultScreen(display);
  (*visual_template).depth=XDefaultDepth(display,XDefaultScreen(display));
  one=1;
  if ((resource_info->immutable != MagickFalse) && (resource_info->colors != 0))
    if (resource_info->colors <= (one << (size_t) (*visual_template).depth))
      (*visual_mask)|=VisualDepthMask;
}

static MagickBooleanType XGetStandardColormapList(Display *display,
  const char *map_type,XResourceInfo *resource_info,
  const StandardColormapTargets *targets)
{
  Status
    *const status = targets->status;

  int
    *const number_maps = targets->number_maps;

  Window
    root_window;

  /*
    Choose a visual associated with a standard colormap.
  */
  root_window=XRootWindow(display,XDefaultScreen(display));
  (*status)=False;
  (*number_maps)=0;
  if (LocaleCompare(map_type,"list") != 0)
    XGetNamedStandardColormap(display,root_window,map_type,targets);
  else
    XGetListedStandardColormap(display,root_window,resource_info,targets);
  if ((*status) == False)
    {
      ThrowXWindowException(XServerError,"UnableToGetStandardColormap",
        map_type);
      return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _StandardColormapSearch
{
  XVisualInfo
    *visual_list;

  XStandardColormap
    *map_list;

  char
    *map_type;

  int
    number_maps,
    number_visuals;
} StandardColormapSearch;

static MagickBooleanType XMatchStandardColormap(
  const StandardColormapSearch *search,XVisualInfo *visual_template,
  XStandardColormap *map_info)
{
  int
    i,
    j;

  (*visual_template).visualid=XVisualIDFromVisual(
    search->visual_list[0].visual);
  for (i=0; i < search->number_maps; i++)
    for (j=0; j < search->number_visuals; j++)
      if (search->map_list[i].visualid ==
          XVisualIDFromVisual(search->visual_list[j].visual))
        {
          *map_info=search->map_list[i];
          (*visual_template).visualid=XVisualIDFromVisual(
            search->visual_list[j].visual);
          break;
        }
  if (map_info->visualid != (*visual_template).visualid)
    {
      ThrowXWindowException(XServerError,
        "UnableToMatchVisualToStandardColormap",search->map_type);
      return(MagickFalse);
    }
  return(MagickTrue);
}

MagickPrivate XVisualInfo *XBestVisualInfo(Display *display,
  XStandardColormap *map_info,XResourceInfo *resource_info)
{
  char
    *map_type,
    *visual_type;

  int
    visual_mask;

  static int
    number_visuals;

  static XVisualInfo
    visual_template;

  XVisualInfo
    *visual_info,
    *visual_list;

  /*
    Restrict visual search by screen number.
  */
  assert(display != (Display *) NULL);
  assert(map_info != (XStandardColormap *) NULL);
  assert(resource_info != (XResourceInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  map_type=resource_info->map_type;
  visual_type=resource_info->visual_type;
  XRestrictVisualByScreen(display,resource_info,&visual_mask,&visual_template);
  if (visual_type != (char *) NULL)
    XRestrictVisualByType(display,visual_type,&visual_mask,&visual_template);
  /*
    Get all visuals that meet our criteria so far.
  */
  number_visuals=0;
  visual_list=XGetVisualInfo(display,visual_mask,&visual_template,
    &number_visuals);
  visual_mask=VisualScreenMask | VisualIDMask;
  if ((number_visuals == 0) || (visual_list == (XVisualInfo *) NULL))
    {
      /*
        Failed to get visual;  try using the default visual.
      */
      ThrowXWindowException(XServerWarning,"UnableToGetVisual",visual_type);
      visual_template.visualid=XVisualIDFromVisual(XDefaultVisual(display,
        XDefaultScreen(display)));
      visual_list=XGetVisualInfo(display,visual_mask,&visual_template,
        &number_visuals);
      if ((number_visuals == 0) || (visual_list == (XVisualInfo *) NULL))
        return((XVisualInfo *) NULL);
      ThrowXWindowException(XServerWarning,"UsingDefaultVisual",
        XVisualClassName(visual_list->klass));
    }
  resource_info->color_recovery=MagickFalse;
  if ((map_info != (XStandardColormap *) NULL) && (map_type != (char *) NULL))
    {
      int
        number_maps;

      Status
        status;

      XStandardColormap
        *map_list;

      const StandardColormapTargets
        targets = { &status,&map_list,&number_maps };

      if (XGetStandardColormapList(display,map_type,resource_info,&targets) ==
          MagickFalse)
        return((XVisualInfo *) NULL);
      /*
        Search all Standard Colormaps and visuals for ids that match.
      */
      *map_info=map_list[0];
#if !defined(PRE_R4_ICCCM)
      {
        const StandardColormapSearch
          search = { visual_list,map_list,map_type,number_maps,
            number_visuals };

        if (XMatchStandardColormap(&search,&visual_template,map_info) ==
            MagickFalse)
          return((XVisualInfo *) NULL);
      }
#endif
      if (map_info->colormap == (Colormap) NULL)
        {
          ThrowXWindowException(XServerError,"StandardColormapIsNotInitialized",
            map_type);
          return((XVisualInfo *) NULL);
        }
      (void) XFree((void *) map_list);
    }
  else
    {
      visual_info=XPickRichestVisual(visual_list,number_visuals);
      visual_template.visualid=XVisualIDFromVisual(visual_info->visual);
    }
  (void) XFree((void *) visual_list);
  /*
    Retrieve only one visual by its screen & id number.
  */
  visual_info=XGetVisualInfo(display,visual_mask,&visual_template,
    &number_visuals);
  if ((number_visuals == 0) || (visual_info == (XVisualInfo *) NULL))
    return((XVisualInfo *) NULL);
  return(visual_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X D i s p l a y I m a g e I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDisplayImageInfo() displays information about an X image.
%
%  The format of the XDisplayImageInfo method is:
%
%      void XDisplayImageInfo(Display *display,
%        const XResourceInfo *resource_info,XWindows *windows,Image *undo_image,
%        Image *image,ExceptionInfo *exception)
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
%    o undo_image: the undo image.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XWriteVisualInfo(FILE *file,const XResourceInfo *resource_info,
  const XWindows *windows)
{
  (void) FormatLocaleFile(file,"X\n  visual: %s\n",
    XVisualClassName((int) windows->image.storage_class));
  (void) FormatLocaleFile(file,"  depth: %d\n",windows->image.ximage->depth);
  if (windows->visual_info->colormap_size != 0)
    (void) FormatLocaleFile(file,"  colormap size: %d\n",
      windows->visual_info->colormap_size);
  if (resource_info->colormap== SharedColormap)
    (void) FormatLocaleFile(file,"  colormap type: Shared\n");
  else
    (void) FormatLocaleFile(file,"  colormap type: Private\n");
  (void) FormatLocaleFile(file,"  geometry: %dx%d\n",
    windows->image.ximage->width,windows->image.ximage->height);
  if (windows->image.crop_geometry != (char *) NULL)
    (void) FormatLocaleFile(file,"  crop geometry: %s\n",
      windows->image.crop_geometry);
  if (windows->image.pixmap == (Pixmap) NULL)
    (void) FormatLocaleFile(file,"  type: X Image\n");
  else
    (void) FormatLocaleFile(file,"  type: Pixmap\n");
  if (windows->image.shape != MagickFalse)
    (void) FormatLocaleFile(file,"  non-rectangular shape: True\n");
  else
    (void) FormatLocaleFile(file,"  non-rectangular shape: False\n");
  if (windows->image.shared_memory != MagickFalse)
    (void) FormatLocaleFile(file,"  shared memory: True\n");
  else
    (void) FormatLocaleFile(file,"  shared memory: False\n");
  (void) FormatLocaleFile(file,"\n");
}

static void XWriteFontInfo(FILE *file,const XResourceInfo *resource_info)
{
  if (resource_info->font != (char *) NULL)
    (void) FormatLocaleFile(file,"Font: %s\n\n",resource_info->font);
  if (resource_info->text_font != (char *) NULL)
    (void) FormatLocaleFile(file,"Text font: %s\n\n",resource_info->text_font);
}

static void XWriteUndoInfo(FILE *file,const XResourceInfo *resource_info,
  Image *undo_image)
{
  ssize_t
    bytes;

  size_t
    number_pixels;

  unsigned int
    levels;

  bytes=0;
  for (levels=0; undo_image != (Image *) NULL; levels++)
  {
    number_pixels=undo_image->list->columns*undo_image->list->rows;
    bytes+=(ssize_t) (number_pixels*sizeof(PixelInfo));
    undo_image=GetPreviousImageInList(undo_image);
  }
  (void) FormatLocaleFile(file,"Undo Edit Cache\n  levels: %u\n",levels);
  (void) FormatLocaleFile(file,"  bytes: %.17gmb\n",(double)
    ((bytes+(1 << 19)) >> 20));
  (void) FormatLocaleFile(file,"  limit: %.17gmb\n\n",(double)
    resource_info->undo_cache);
}

static void XWriteGammaInfo(FILE *file,const XResourceInfo *resource_info)
{
  if (resource_info->gamma_correct != MagickFalse)
    if (resource_info->display_gamma != (char *) NULL)
      (void) FormatLocaleFile(file,"Display\n  gamma: %s\n\n",
        resource_info->display_gamma);
}

MagickPrivate void XDisplayImageInfo(Display *display,
  const XResourceInfo *resource_info,XWindows *windows,Image *undo_image,
  Image *image,ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent],
    *text,
    **textlist;

  FILE
    *file;

  int
    unique_file;

  ssize_t
    i;

  /*
    Write info about the X server to a file.
  */
  assert(display != (Display *) NULL);
  assert(resource_info != (XResourceInfo *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(image != (Image *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  file=(FILE *) NULL;
  unique_file=AcquireUniqueFileResource(filename);
  if (unique_file != -1)
    file=fdopen(unique_file,"w");
  if ((unique_file == -1) || (file == (FILE *) NULL))
    {
      XNoticeWidget(display,windows,"Unable to display image info",filename);
      return;
    }
  XWriteGammaInfo(file,resource_info);
  /*
    Write info about the X image to a file.
  */
  XWriteVisualInfo(file,resource_info,windows);
  XWriteFontInfo(file,resource_info);
  /*
    Write info about the undo cache to a file.
  */
  XWriteUndoInfo(file,resource_info,undo_image);
  /*
    Write info about the image to a file.
  */
  (void) IdentifyImage(image,file,MagickTrue,exception);
  (void) fclose(file);
  text=FileToString(filename,~0UL,exception);
  (void) RelinquishUniqueFileResource(filename);
  if (text == (char *) NULL)
    {
      XNoticeWidget(display,windows,"MemoryAllocationFailed",
        "UnableToDisplayImageInfo");
      return;
    }
  textlist=StringToList(text);
  if (textlist != (char **) NULL)
    {
      char
        title[MagickPathExtent];

      /*
        Display information about the image in the Text View widget.
      */
      (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
      (void) FormatLocaleString(title,MagickPathExtent,"Image Info: %s",
        image->filename);
      XTextViewWidget(display,resource_info,windows,MagickTrue,title,
        (char const **) textlist);
      for (i=0; textlist[i] != (char *) NULL; i++)
        textlist[i]=DestroyString(textlist[i]);
      textlist=(char **) RelinquishMagickMemory(textlist);
    }
  text=DestroyString(text);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X I n i t i a l i z e W i n d o w s                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XInitializeWindows() initializes the XWindows structure.
%
%  The format of the XInitializeWindows method is:
%
%      XWindows *XInitializeWindows(Display *display,
%        XResourceInfo *resource_info)
%
%  A description of each parameter follows:
%
%    o windows: XInitializeWindows returns a pointer to a XWindows structure.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
*/
static inline MagickBooleanType IsWindowsInfoMissing(const XWindows *windows)
{
  return(((windows->pixel_info == (XPixelInfo *) NULL) ||
    (windows->icon_pixel == (XPixelInfo *) NULL) ||
    (windows->icon_resources == (XResourceInfo *) NULL)) ? MagickTrue :
    MagickFalse);
}

static void LogXWindowProtocols(Display *display,const XWindows *windows)
{
  if ((GetLogEventMask() & X11Event) != 0)
    {
      (void) XSynchronize(display,MagickTrue);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"Version: %s",
        GetMagickVersion((size_t *) NULL));
      (void) LogMagickEvent(X11Event,GetMagickModule(),"Protocols:");
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "  Window Manager: 0x%lx",windows->wm_protocols);
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "    delete window: 0x%lx",windows->wm_delete_window);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"    take focus: 0x%lx",
        windows->wm_take_focus);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"  ImageMagick: 0x%lx",
        windows->im_protocols);
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "    remote command: 0x%lx",windows->im_remote_command);
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "    update widget: 0x%lx",windows->im_update_widget);
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "    update colormap: 0x%lx",windows->im_update_colormap);
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "    former image: 0x%lx",windows->im_former_image);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"    next image: 0x%lx",
        windows->im_next_image);
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "    retain colors: 0x%lx",windows->im_retain_colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"    exit: 0x%lx",
        windows->im_exit);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"  Drag and Drop: 0x%lx",
        windows->dnd_protocols);
    }
}

static void LogXWindowVisual(const XWindows *windows)
{
  if ((GetLogEventMask() & X11Event) != 0)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),"Visual:");
      (void) LogMagickEvent(X11Event,GetMagickModule(),"  visual id: 0x%lx",
        windows->visual_info->visualid);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"  class: %s",
        XVisualClassName(windows->visual_info->klass));
      (void) LogMagickEvent(X11Event,GetMagickModule(),"  depth: %d planes",
        windows->visual_info->depth);
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "  size of colormap: %d entries",windows->visual_info->colormap_size);
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "  red, green, blue masks: 0x%lx 0x%lx 0x%lx",
        windows->visual_info->red_mask,windows->visual_info->green_mask,
        windows->visual_info->blue_mask);
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "  significant bits in color: %d bits",
        windows->visual_info->bits_per_rgb);
    }
}

static void GetXWindowGroupLeader(Display *display,XResourceInfo *resource_info,
  XWindows *windows)
{
  Window
    root_window;

  root_window=XRootWindow(display,windows->visual_info->screen);
  windows->group_leader.id=(Window) NULL;
  if (resource_info->window_group != (char *) NULL)
    {
      if (isdigit((int) ((unsigned char) *resource_info->window_group)) != 0)
        windows->group_leader.id=XWindowByID(display,root_window,(Window)
          strtol((char *) resource_info->window_group,(char **) NULL,0));
      if (windows->group_leader.id == (Window) NULL)
        windows->group_leader.id=
          XWindowByName(display,root_window,resource_info->window_group);
    }
}

static void InitializeXWindowColormaps(XWindows *windows)
{
  windows->map_info=XAllocStandardColormap();
  windows->icon_map=XAllocStandardColormap();
  if ((windows->map_info == (XStandardColormap *) NULL) ||
      (windows->icon_map == (XStandardColormap *) NULL))
    ThrowXWindowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      "...");
  windows->map_info->colormap=(Colormap) NULL;
  windows->icon_map->colormap=(Colormap) NULL;
  windows->pixel_info->pixels=(unsigned long *) NULL;
  windows->pixel_info->annotate_context=(GC) NULL;
  windows->pixel_info->highlight_context=(GC) NULL;
  windows->pixel_info->widget_context=(GC) NULL;
  windows->font_info=(XFontStruct *) NULL;
  windows->icon_pixel->annotate_context=(GC) NULL;
  windows->icon_pixel->pixels=(unsigned long *) NULL;
}

MagickPrivate XWindows *XInitializeWindows(Display *display,
  XResourceInfo *resource_info)
{
  XWindows
    *windows;

  /*
    Allocate windows structure.
  */
  windows=(XWindows *) AcquireMagickMemory(sizeof(*windows));
  if (windows == (XWindows *) NULL)
    {
      ThrowXWindowFatalException(XServerFatalError,"MemoryAllocationFailed",
        "...");
      return((XWindows *) NULL);
    }
  (void) memset(windows,0,sizeof(*windows));
  windows->pixel_info=(XPixelInfo *) AcquireQuantumMemory(1,
    sizeof(*windows->pixel_info));
  windows->icon_pixel=(XPixelInfo *) AcquireQuantumMemory(1,
    sizeof(*windows->icon_pixel));
  windows->icon_resources=(XResourceInfo *) AcquireQuantumMemory(1,
    sizeof(*windows->icon_resources));
  if (IsWindowsInfoMissing(windows) != MagickFalse)
    {
      ThrowXWindowFatalException(XServerFatalError,"MemoryAllocationFailed",
        "...");
      return((XWindows *) NULL);
    }
  (void) memset((void *) windows->pixel_info,0,sizeof(XPixelInfo));
  (void) memset((void *) windows->icon_pixel,0,sizeof(XPixelInfo));
  /*
    Initialize windows structure.
  */
  windows->display=display;
  windows->wm_protocols=XInternAtom(display,"WM_PROTOCOLS",MagickFalse);
  windows->wm_delete_window=XInternAtom(display,"WM_DELETE_WINDOW",MagickFalse);
  windows->wm_take_focus=XInternAtom(display,"WM_TAKE_FOCUS",MagickFalse);
  windows->im_protocols=XInternAtom(display,"IM_PROTOCOLS",MagickFalse);
  windows->im_remote_command=
    XInternAtom(display,"IM_REMOTE_COMMAND",MagickFalse);
  windows->im_update_widget=XInternAtom(display,"IM_UPDATE_WIDGET",MagickFalse);
  windows->im_update_colormap=
    XInternAtom(display,"IM_UPDATE_COLORMAP",MagickFalse);
  windows->im_former_image=XInternAtom(display,"IM_FORMER_IMAGE",MagickFalse);
  windows->im_next_image=XInternAtom(display,"IM_NEXT_IMAGE",MagickFalse);
  windows->im_retain_colors=XInternAtom(display,"IM_RETAIN_COLORS",MagickFalse);
  windows->im_exit=XInternAtom(display,"IM_EXIT",MagickFalse);
  windows->dnd_protocols=XInternAtom(display,"DndProtocol",MagickFalse);
#if defined(MAGICKCORE_WINDOWS_SUPPORT)
  (void) XSynchronize(display,MagickFalse);
#endif
  LogXWindowProtocols(display,windows);
  /*
    Allocate standard colormap.
  */
  InitializeXWindowColormaps(windows);
  /*
    Allocate visual.
  */
  *windows->icon_resources=(*resource_info);
  windows->icon_resources->visual_type=(char *) "default";
  windows->icon_resources->colormap=SharedColormap;
  windows->visual_info=
    XBestVisualInfo(display,windows->map_info,resource_info);
  windows->icon_visual=
    XBestVisualInfo(display,windows->icon_map,windows->icon_resources);
  if ((windows->visual_info == (XVisualInfo *) NULL) ||
      (windows->icon_visual == (XVisualInfo *) NULL))
    ThrowXWindowFatalException(XServerFatalError,"UnableToGetVisual",
      resource_info->visual_type);
  LogXWindowVisual(windows);
  /*
    Allocate class and manager hints.
  */
  windows->class_hints=XAllocClassHint();
  windows->manager_hints=XAllocWMHints();
  if ((windows->class_hints == (XClassHint *) NULL) ||
      (windows->manager_hints == (XWMHints *) NULL))
    ThrowXWindowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      "...");
  /*
    Determine group leader if we have one.
  */
  GetXWindowGroupLeader(display,resource_info,windows);
  return(windows);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X V i s u a l C l a s s N a m e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XVisualClassName() returns the visual class name as a character string.
%
%  The format of the XVisualClassName method is:
%
%      char *XVisualClassName(const int visual_class)
%
%  A description of each parameter follows:
%
%    o visual_type: XVisualClassName returns the visual class as a character
%      string.
%
%    o class: Specifies the visual class.
%
*/
static const char *XVisualClassName(const int visual_class)
{
  switch (visual_class)
  {
    case StaticGray: return("StaticGray");
    case GrayScale: return("GrayScale");
    case StaticColor: return("StaticColor");
    case PseudoColor: return("PseudoColor");
    case TrueColor: return("TrueColor");
    case DirectColor: return("DirectColor");
  }
  return("unknown visual class");
}
#endif
