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
%                         TTTTT  EEEEE  X   X  TTTTT                          %
%                           T    E       X X     T                            %
%                           T    EEE      X      T                            %
%                           T    E       X X     T                            %
%                           T    EEEEE  X   X    T                            %
%                                                                             %
%                                                                             %
%               MagickCore X11 Font and Text Rendering Methods                %
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
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X B e s t F o n t                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XBestFont() returns the "best" font.  "Best" is defined as a font specified
%  in the X resource database or a font such that the text width displayed
%  with the font does not exceed the specified maximum width.
%
%  The format of the XBestFont method is:
%
%      XFontStruct *XBestFont(Display *display,
%        const XResourceInfo *resource_info,const MagickBooleanType text_font)
%
%  A description of each parameter follows:
%
%    o font: XBestFont returns a pointer to a XFontStruct structure.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o text_font:  True is font should be mono-spaced (typewriter style).
%
*/

static inline MagickBooleanType IsFontSeparator(const char *p)
{
  return(((*p == ':') || (*p == ';') || (*p == ',')) ? MagickTrue :
    MagickFalse);
}

static unsigned int CountFonts(const char *font)
{
  const char
    *p;

  unsigned int
    fonts;

  fonts=1U;
  for (p=font; *p != '\0'; p++)
    if (IsFontSeparator(p) != MagickFalse)
      fonts++;
  return(fonts);
}

static char **FontToList(char *font)
{
  char
    **fontlist;

  char
    *p,
    *q;

  int
    i;

  unsigned int
    fonts;

  if (font == (char *) NULL)
    return((char **) NULL);
  /*
    Convert string to an ASCII list.
  */
  fonts=CountFonts(font);
  fontlist=(char **) AcquireQuantumMemory((size_t) fonts+1UL,sizeof(*fontlist));
  if (fontlist == (char **) NULL)
    {
      ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",font);
      return((char **) NULL);
    }
  p=font;
  for (i=0; i < (int) fonts; i++)
  {
    for (q=p; *q != '\0'; q++)
      if (IsFontSeparator(q) != MagickFalse)
        break;
    fontlist[i]=(char *) AcquireQuantumMemory((size_t) (q-p)+1UL,
      sizeof(*fontlist[i]));
    if (fontlist[i] == (char *) NULL)
      {
        ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",font);
        return((char **) NULL);
      }
    (void) CopyMagickString(fontlist[i],p,(size_t) (q-p+1));
    p=q+1;
  }
  fontlist[i]=(char *) NULL;
  return(fontlist);
}

static XFontStruct *XLoadFirstFont(Display *display,char **fontlist,
  XFontStruct *font_info)
{
  int
    i;

  if (fontlist != (char **) NULL)
    {
      for (i=0; fontlist[i] != (char *) NULL; i++)
      {
        if (font_info == (XFontStruct *) NULL)
          font_info=XLoadQueryFont(display,fontlist[i]);
        fontlist[i]=DestroyString(fontlist[i]);
      }
      fontlist=(char **) RelinquishMagickMemory(fontlist);
    }
  return(font_info);
}

static XFontStruct *XLoadListedFont(Display *display,const char **p,
  XFontStruct *font_info)
{
  while (*p != (char *) NULL)
  {
    if (font_info != (XFontStruct *) NULL)
      break;
    font_info=XLoadQueryFont(display,(char *) *p);
    p++;
  }
  return(font_info);
}

MagickPrivate XFontStruct *XBestFont(Display *display,
  const XResourceInfo *resource_info,const MagickBooleanType text_font)
{
  static const char
    *Fonts[]=
    {
      "-*-helvetica-medium-r-normal--12-*-*-*-*-*-iso8859-1",
      "-*-arial-medium-r-normal--12-*-*-*-*-*-iso8859-1",
      "-*-helvetica-medium-r-normal--12-*-*-*-*-*-iso8859-15",
      "-*-arial-medium-r-normal--12-*-*-*-*-*-iso8859-15",
      "-*-helvetica-medium-r-normal--12-*-*-*-*-*-*-*",
      "-*-arial-medium-r-normal--12-*-*-*-*-*-*-*",
      "variable",
      "fixed",
      (char *) NULL
    },
    *TextFonts[]=
    {
      "-*-courier-medium-r-normal-*-12-*-*-*-*-*-iso8859-1",
      "-*-courier-medium-r-normal-*-12-*-*-*-*-*-iso8859-15",
      "-*-fixed-medium-r-normal-*-12-*-*-*-*-*-*-*",
      "fixed",
      (char *) NULL
    };

  char
    *font_name;

  const char
    **p;

  XFontStruct
    *font_info;

  font_info=(XFontStruct *) NULL;
  font_name=resource_info->font;
  if (text_font != MagickFalse)
    font_name=resource_info->text_font;
  if ((font_name != (char *) NULL) && (*font_name != '\0'))
    {
      char
        **fontlist;

      /*
        Load preferred font specified in the X resource database.
      */
      fontlist=FontToList(font_name);
      font_info=XLoadFirstFont(display,fontlist,font_info);
      if (font_info == (XFontStruct *) NULL)
        ThrowXWindowException(XServerError,"UnableToLoadFont",font_name);
    }
  /*
    Load fonts from list of fonts until one is found.
  */
  p=Fonts;
  if (text_font != MagickFalse)
    p=TextFonts;
  if (XDisplayHeight(display,XDefaultScreen(display)) >= 748)
    p++;
  font_info=XLoadListedFont(display,p,font_info);
  return(font_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X G e t A n n o t a t e I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XGetAnnotateInfo() initializes the AnnotateInfo structure.
%
%  The format of the XGetAnnotateInfo method is:
%
%      void XGetAnnotateInfo(XAnnotateInfo *annotate_info)
%
%  A description of each parameter follows:
%
%    o annotate_info: Specifies a pointer to a XAnnotateInfo structure.
%
*/
MagickPrivate void XGetAnnotateInfo(XAnnotateInfo *annotate_info)
{
  /*
    Initialize annotate structure.
  */
  assert(annotate_info != (XAnnotateInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  annotate_info->x=0;
  annotate_info->y=0;
  annotate_info->width=0;
  annotate_info->height=0;
  annotate_info->stencil=ForegroundStencil;
  annotate_info->degrees=0.0;
  annotate_info->font_info=(XFontStruct *) NULL;
  annotate_info->text=(char *) NULL;
  *annotate_info->geometry='\0';
  annotate_info->previous=(XAnnotateInfo *) NULL;
  annotate_info->next=(XAnnotateInfo *) NULL;
  (void) XSupportsLocale();
  (void) XSetLocaleModifiers("");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X R e n d e r I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XRenderImage() renders text on the image with an X11 font.  It also returns
%  the bounding box of the text relative to the image.
%
%  The format of the XRenderImage method is:
%
%      MagickBooleanType XRenderImage(Image *image,const XRenderText *text,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o text: the text to render, in these members:
%
%      o draw_info: the draw info.
%
%      o offset: (x,y) location of text relative to image.
%
%      o metrics: bounding box of text.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XSetRenderMetrics(TypeMetric *metrics,const DrawInfo *draw_info,
  const XFontStruct *font_info,const XAnnotateInfo annotate_info)
{
  metrics->pixels_per_em.x=(double) font_info->max_bounds.width;
  metrics->pixels_per_em.y=(double) font_info->ascent+font_info->descent;
  metrics->ascent=(double) font_info->ascent+4;
  metrics->descent=(double) (-font_info->descent);
  metrics->width=annotate_info.width/ExpandAffine(&draw_info->affine);
  metrics->height=(double) font_info->ascent+font_info->descent;
  metrics->max_advance=(double) font_info->max_bounds.width;
  metrics->bounds.x1=0.0;
  metrics->bounds.y1=metrics->descent;
  metrics->bounds.x2=metrics->ascent+metrics->descent;
  metrics->bounds.y2=metrics->ascent+metrics->descent;
  metrics->underline_position=(-2.0);
  metrics->underline_thickness=1.0;
}

typedef struct _XRenderTargets
{
  XrmDatabase
    *resource_database;

  XResourceInfo
    *resource_info;

  XStandardColormap
    **map_info;

  XVisualInfo
    **visual_info;

  XFontStruct
    **font_info;

  XPixelInfo
    *pixel;

  DrawInfo
    *cache_info;

  Display
    **display;
} XRenderTargets;

static void XInitializeRenderColormap(Display *display,
  const XRenderTargets *targets)
{
  XResourceInfo
    *const resource_info = targets->resource_info;

  XStandardColormap
    **const map_info = targets->map_info;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XPixelInfo
    *const pixel = targets->pixel;

  XGetMapInfo((*visual_info),XDefaultColormap(display,(*visual_info)->screen),
    (*map_info));
  {
    const XPixelInfoSource
      pixel_source = { (*visual_info), (*map_info), resource_info };

    XGetPixelInfo(display,&pixel_source,(Image *) NULL,pixel);
  }
  (*pixel).annotate_context=XDefaultGC(display,(*visual_info)->screen);
}

static MagickBooleanType XInitializeRenderResources(const DrawInfo *draw_info,
  const Image *image,Display *display,const XRenderTargets *targets)
{
  XrmDatabase
    *const resource_database = targets->resource_database;

  XResourceInfo
    *const resource_info = targets->resource_info;

  XStandardColormap
    **const map_info = targets->map_info;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XFontStruct
    **const font_info = targets->font_info;

  XPixelInfo
    *const pixel = targets->pixel;

  DrawInfo
    *const cache_info = targets->cache_info;

  const char
    *client_name;

  ImageInfo
    *image_info;

  (void) XSetErrorHandler(XError);
  image_info=AcquireImageInfo();
  client_name=GetClientName();
  (*resource_database)=XGetResourceDatabase(display,client_name);
  XGetResourceInfo(image_info,(*resource_database),client_name,resource_info);
  (*resource_info).close_server=MagickFalse;
  (*resource_info).colormap=PrivateColormap;
  (*resource_info).font=AcquireString(draw_info->font);
  (*resource_info).background_color=AcquireString("#ffffffffffff");
  (*resource_info).foreground_color=AcquireString("#000000000000");
  (*map_info)=XAllocStandardColormap();
  (*visual_info)=(XVisualInfo *) NULL;
  (*font_info)=(XFontStruct *) NULL;
  (*pixel).pixels=(unsigned long *) NULL;
  if ((*map_info) == (XStandardColormap *) NULL)
    {
      ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
      return(MagickFalse);
    }
  /*
    Initialize visual info.
  */
  (*visual_info)=XBestVisualInfo(display,(*map_info),resource_info);
  if ((*visual_info) == (XVisualInfo *) NULL)
    {
      XFreeResources(display,(*visual_info),(*map_info),pixel,(*font_info),
        resource_info,(XWindowInfo *) NULL);
      ThrowXWindowException(XServerError,"UnableToGetVisual",
        image->filename);
      return(MagickFalse);
    }
  (*map_info)->colormap=(Colormap) NULL;
  /*
    Initialize Standard Colormap info.
  */
  XInitializeRenderColormap(display,targets);
  /*
    Initialize font info.
  */
  (*font_info)=XBestFont(display,resource_info,MagickFalse);
  if ((*font_info) == (XFontStruct *) NULL)
    {
      XFreeResources(display,(*visual_info),(*map_info),pixel,(*font_info),
        resource_info,(XWindowInfo *) NULL);
      ThrowXWindowException(XServerError,"UnableToLoadFont",
        draw_info->font);
      return(MagickFalse);
    }
  (*cache_info)=(*draw_info);
  return(MagickTrue);
}

typedef struct _XRenderRequest
{
  Image
    *image;

  const DrawInfo
    *draw_info;

  const PointInfo
    *offset;

  TypeMetric
    *metrics;

  ExceptionInfo
    *exception;
} XRenderRequest;

static MagickBooleanType XRenderFillColor(const XRenderRequest *request,
  Display *display,XAnnotateInfo *annotate_info,XPixelInfo *pixel)
{
  Image
    *const image = request->image;

  const DrawInfo
    *const draw_info = request->draw_info;

  const PointInfo
    *const offset = request->offset;

  TypeMetric
    *const metrics = request->metrics;

  ExceptionInfo
    *const exception = request->exception;

  const XImageRenderTarget
    target = { display, pixel };

  MagickBooleanType
    status;

  size_t
    height,
    width;

  /*
    Render fill color.
  */
  width=(*annotate_info).width;
  height=(*annotate_info).height;
  if ((fabs(draw_info->affine.rx) >= MagickEpsilon) ||
      (fabs(draw_info->affine.ry) >= MagickEpsilon))
    {
      if ((fabs(draw_info->affine.sx-draw_info->affine.sy) < MagickEpsilon) &&
          (fabs(draw_info->affine.rx+draw_info->affine.ry) < MagickEpsilon))
        (*annotate_info).degrees=(double) (180.0/MagickPI)*
          atan2(draw_info->affine.rx,draw_info->affine.sx);
    }
  (void) FormatLocaleString((*annotate_info).geometry,MagickPathExtent,
    "%.17gx%.17g%+.20g%+.20g",(double) width,(double) height,
    ceil(offset->x-0.5),ceil(offset->y-metrics->ascent-metrics->descent+
    draw_info->interline_spacing-0.5));
  (*pixel).pen_color.red=ScaleQuantumToShort(
    ClampToQuantum(draw_info->fill.red));
  (*pixel).pen_color.green=ScaleQuantumToShort(
    ClampToQuantum(draw_info->fill.green));
  (*pixel).pen_color.blue=ScaleQuantumToShort(
    ClampToQuantum(draw_info->fill.blue));
  status=XAnnotateImage(&target,annotate_info,image,exception);
  if (status == 0)
    {
      ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType XChangeRenderFont(const DrawInfo *draw_info,
  Display *display,char *font,const XRenderTargets *targets)
{
  XResourceInfo
    *const resource_info = targets->resource_info;

  XFontStruct
    **const font_info = targets->font_info;

  if (font != draw_info->font)
    {
      /*
        Type name has changed.
      */
      (void) XFreeFont(display,(*font_info));
      (void) CloneString(&(*resource_info).font,draw_info->font);
      (*font_info)=XBestFont(display,resource_info,MagickFalse);
      if ((*font_info) == (XFontStruct *) NULL)
        {
          ThrowXWindowException(XServerError,"UnableToLoadFont",
            draw_info->font);
          return(MagickFalse);
        }
    }
  return(MagickTrue);
}

static MagickBooleanType XOpenRenderResources(const DrawInfo *draw_info,
  const Image *image,const XRenderTargets *targets)
{
  Display
    **const display = targets->display;

  /*
    Open X server connection.
  */
  (*display)=XOpenDisplay(draw_info->server_name);
  if ((*display) == (Display *) NULL)
    {
      ThrowXWindowException(XServerError,"UnableToOpenXServer",
        draw_info->server_name);
      return(MagickFalse);
    }
  /*
    Get user defaults from X resource database.
  */
  if (XInitializeRenderResources(draw_info,image,(*display),targets) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

MagickPrivate MagickBooleanType XRenderImage(Image *image,
  const XRenderText *text,ExceptionInfo *exception)
{
  const DrawInfo
    *const draw_info = text->draw_info;

  const PointInfo
    *const offset = text->offset;

  TypeMetric
    *const metrics = text->metrics;

  XRenderRequest
    request;

  static Display
    *display = (Display *) NULL;

  static DrawInfo
    cache_info;

  static XAnnotateInfo
    annotate_info;

  static XFontStruct
    *font_info;

  static XPixelInfo
    pixel;

  static XResourceInfo
    resource_info;

  static XrmDatabase
    resource_database;

  static XStandardColormap
    *map_info;

  static XVisualInfo
    *visual_info;

  const XRenderTargets
    targets = { &resource_database, &resource_info, &map_info, &visual_info,
      &font_info, &pixel, &cache_info, &display };

  if (display == (Display *) NULL)
    {
      if (XOpenRenderResources(draw_info,image,&targets) == MagickFalse)
        return(MagickFalse);
    }
  /*
    Initialize annotate info.
  */
  XGetAnnotateInfo(&annotate_info);
  annotate_info.stencil=ForegroundStencil;
  if (XChangeRenderFont(draw_info,display,cache_info.font,&targets) == MagickFalse)
    return(MagickFalse);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(AnnotateEvent,GetMagickModule(),
      "Font %s; pointsize %g",draw_info->font != (char *) NULL ?
      draw_info->font : "none",draw_info->pointsize);
  cache_info=(*draw_info);
  annotate_info.font_info=font_info;
  annotate_info.text=(char *) draw_info->text;
  annotate_info.width=(unsigned int) XTextWidth(font_info,draw_info->text,(int)
    strlen(draw_info->text));
  annotate_info.height=(unsigned int) (font_info->ascent+font_info->descent);
  XSetRenderMetrics(metrics,draw_info,font_info,annotate_info);
  if (draw_info->render == MagickFalse)
    return(MagickTrue);
  if (draw_info->fill.alpha == (double) TransparentAlpha)
    return(MagickTrue);
  request.image=image;
  request.draw_info=draw_info;
  request.offset=offset;
  request.metrics=metrics;
  request.exception=exception;
  return(XRenderFillColor(&request,display,&annotate_info,&pixel));
}

#else

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X R e n d e r X 1 1                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XRenderImage() renders text on the image with an X11 font.  It also returns
%  the bounding box of the text relative to the image.
%
%  The format of the XRenderImage method is:
%
%      MagickBooleanType XRenderImage(Image *image,const XRenderText *text,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o text: the text to render, in these members:
%
%      o draw_info: the draw info.
%
%      o offset: (x,y) location of text relative to image.
%
%      o metrics: bounding box of text.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickPrivate MagickBooleanType XRenderImage(Image *image,
  const XRenderText *text,ExceptionInfo *exception)
{
  const DrawInfo
    *const draw_info = text->draw_info;

  const PointInfo
    *const offset = text->offset;

  TypeMetric
    *const metrics = text->metrics;

  (void) draw_info;
  (void) offset;
  (void) metrics;
  (void) ThrowMagickException(exception,GetMagickModule(),
    MissingDelegateError,"DelegateLibrarySupportNotBuiltIn","'%s' (X11)",
    image->filename);
  return(MagickFalse);
}
#endif
