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
%            AAA   N   N  N   N   OOO   TTTTT   AAA   TTTTT  EEEEE            %
%           A   A  NN  N  NN  N  O   O    T    A   A    T    E                %
%           AAAAA  N N N  N N N  O   O    T    AAAAA    T    EEE              %
%           A   A  N  NN  N  NN  O   O    T    A   A    T    E                %
%           A   A  N   N  N   N   OOO     T    A   A    T    EEEEE            %
%                                                                             %
%                                                                             %
%                  MagickCore X11 Annotate and Draw Methods                   %
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
#include "MagickCore/xwindow-image-private.h"
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
%   X A n n o t a t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XAnnotateImage() annotates the image with text.
%
%  The format of the XAnnotateImage method is:
%
%      MagickBooleanType XAnnotateImage(Display *display,
%        const XPixelInfo *pixel,XAnnotateInfo *annotate_info,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o pixel: Specifies a pointer to a XPixelInfo structure.
%
%    o annotate_info: Specifies a pointer to a XAnnotateInfo structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef enum
{
  AnnotateBoxColor,
  AnnotatePenColor
} AnnotatePixelSide;

typedef struct _AnnotatePixelPaint
{
  AnnotatePixelSide
    side;

  const Image
    *annotate_image;

  const XPixelInfo
    *pixel;

  const XAnnotateInfo
    *annotate_info;
} AnnotatePixelPaint;

static magick_attribute((__always_inline__)) inline void SetAnnotatePixel(
  const AnnotatePixelPaint *paint,Quantum *q)
{
  const AnnotatePixelSide
    side = paint->side;

  const Image
    *const annotate_image = paint->annotate_image;

  const XPixelInfo
    *const pixel = paint->pixel;

  const XAnnotateInfo
    *const annotate_info = paint->annotate_info;

  /*
    Set this pixel to the background color.
  */
  SetPixelRed(annotate_image,ScaleShortToQuantum(((side == AnnotateBoxColor) ?
    pixel->box_color.red : pixel->pen_color.red)),q);
  SetPixelGreen(annotate_image,ScaleShortToQuantum(((side == AnnotateBoxColor) ?
    pixel->box_color.green : pixel->pen_color.green)),q);
  SetPixelBlue(annotate_image,ScaleShortToQuantum(((side == AnnotateBoxColor) ?
    pixel->box_color.blue : pixel->pen_color.blue)),q);
  if (((side == AnnotateBoxColor) ?
      (annotate_info->stencil == ForegroundStencil) ||
      (annotate_info->stencil == OpaqueStencil) :
      annotate_info->stencil == BackgroundStencil))
    SetPixelAlpha(annotate_image,TransparentAlpha,q);
}

typedef struct _AnnotatePixels
{
  Image
    *annotate_image;

  XImage
    *annotate_ximage;

  const XPixelInfo
    *pixel;

  XAnnotateInfo
    *annotate_info;

  ExceptionInfo
    *exception;
} AnnotatePixels;

static void XSetAnnotateRow(const AnnotatePixels *annotate_pixels,
  Quantum *magick_restrict q,const int y)
{
  Image
    *const annotate_image = annotate_pixels->annotate_image;

  XImage
    *const annotate_ximage = annotate_pixels->annotate_ximage;

  const XPixelInfo
    *const pixel = annotate_pixels->pixel;

  XAnnotateInfo
    *const annotate_info = annotate_pixels->annotate_info;

  int
    x;

  for (x=0; x < (int) annotate_image->columns; x++)
  {
    const AnnotatePixelPaint
      box = { AnnotateBoxColor, annotate_image, pixel, annotate_info },
      pen = { AnnotatePenColor, annotate_image, pixel, annotate_info };

    SetPixelAlpha(annotate_image,OpaqueAlpha,q);
    if (XGetPixel(annotate_ximage,x,y) == 0)
      SetAnnotatePixel(&box,q);
    else
      SetAnnotatePixel(&pen,q);
    q+=(ptrdiff_t) GetPixelChannels(annotate_image);
  }
}

static int XTransferAnnotatePixels(const AnnotatePixels *annotate_pixels)
{
  Image
    *const annotate_image = annotate_pixels->annotate_image;

  XImage
    *const annotate_ximage = annotate_pixels->annotate_ximage;

  ExceptionInfo
    *const exception = annotate_pixels->exception;

  CacheView
    *annotate_view;

  int
    y;

  annotate_view=AcquireAuthenticCacheView(annotate_image,exception);
  for (y=0; y < (int) annotate_image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    q=GetCacheViewAuthenticPixels(annotate_view,0,(ssize_t) y,
      annotate_image->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    XSetAnnotateRow(annotate_pixels,q,y);
    if (SyncCacheViewAuthenticPixels(annotate_view,exception) == MagickFalse)
      break;
  }
  annotate_view=DestroyCacheView(annotate_view);
  XDestroyImage(annotate_ximage);
  return(y);
}

static Image *XScaleAnnotateImage(Image *annotate_image,
  const unsigned int width,const unsigned int height,ExceptionInfo *exception)
{
  if ((width != (unsigned int) annotate_image->columns) ||
      (height != (unsigned int) annotate_image->rows))
    {
      char
        image_geometry[MagickPathExtent];

      /*
        Scale image.
      */
      (void) FormatLocaleString(image_geometry,MagickPathExtent,"%ux%u",
        width,height);
      (void) TransformImage(&annotate_image,(char *) NULL,image_geometry,
        exception);
    }
  return(annotate_image);
}

static int XQuarterTurns(const double degrees)
{
  double
    normalized_degrees;

  int
    rotations;

  normalized_degrees=degrees;
  while (normalized_degrees < -45.0)
    normalized_degrees+=360.0;
  for (rotations=0; normalized_degrees > 45.0; rotations++)
    normalized_degrees-=90.0;
  return(rotations);
}

static void XRotateAnnotateOrigin(const Image *annotate_image,
  const int rotations,int *x,int *y)
{
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
      (*x)-=(int) annotate_image->columns/2;
      (*y)+=(int) annotate_image->columns/2;
      break;
    }
    case 2:
    {
      /*
        Rotate 180 degrees.
      */
      (*x)=(*x)-(int) annotate_image->columns;
      break;
    }
    case 3:
    {
      /*
        Rotate 270 degrees.
      */
      (*x)=(*x)-(int) annotate_image->columns/2;
      (*y)=(*y)-(int) (annotate_image->rows-(annotate_image->columns/2));
      break;
    }
  }
}

static MagickBooleanType XAcquireAnnotateImage(
  const XAnnotateInfo *annotate_info,Image **annotate_image,
  ExceptionInfo *exception)
{
  (*annotate_image)=AcquireImage((ImageInfo *) NULL,exception);
  if ((*annotate_image) == (Image *) NULL)
    return(MagickFalse);
  (*annotate_image)->columns=annotate_info->width;
  (*annotate_image)->rows=annotate_info->height;
  return(MagickTrue);
}

static MagickBooleanType XDrawAnnotateText(Display *display,
  const XAnnotateInfo *annotate_info,XImage **annotate_ximage)
{
  GC
    annotate_context;

  Pixmap
    annotate_pixmap;

  unsigned int
    depth;

  Window
    root_window;

  XGCValues
    context_values;

  root_window=XRootWindow(display,XDefaultScreen(display));
  depth=(unsigned int) XDefaultDepth(display,XDefaultScreen(display));
  annotate_pixmap=XCreatePixmap(display,root_window,annotate_info->width,
    annotate_info->height,depth);
  if (annotate_pixmap == (Pixmap) NULL)
    return(MagickFalse);
  /*
    Initialize graphics info.
  */
  context_values.background=0;
  context_values.foreground=(size_t) (~0);
  context_values.font=annotate_info->font_info->fid;
  annotate_context=XCreateGC(display,root_window,(unsigned long)
    (GCBackground | GCFont | GCForeground),&context_values);
  if (annotate_context == (GC) NULL)
    return(MagickFalse);
  /*
    Draw text to pixmap.
  */
  (void) XDrawImageString(display,annotate_pixmap,annotate_context,0,
    (int) annotate_info->font_info->ascent,annotate_info->text,
    (int) strlen(annotate_info->text));
  (void) XFreeGC(display,annotate_context);
  /*
    Initialize annotated X image.
  */
  (*annotate_ximage)=XGetImage(display,annotate_pixmap,0,0,annotate_info->width,
    annotate_info->height,AllPlanes,ZPixmap);
  if ((*annotate_ximage) == (XImage *) NULL)
    return(MagickFalse);
  (void) XFreePixmap(display,annotate_pixmap);
  return(MagickTrue);
}

static void XSetAnnotateAlphaTrait(const XAnnotateInfo *annotate_info,
  Image *annotate_image)
{
  if (annotate_info->stencil == ForegroundStencil)
    annotate_image->alpha_trait=BlendPixelTrait;
}

MagickPrivate MagickBooleanType XAnnotateImage(Display *display,
  const XPixelInfo *pixel,XAnnotateInfo *annotate_info,Image *image,
  ExceptionInfo *exception)
{
  AnnotatePixels
    annotate_pixels;

  Image
    *annotate_image;

  int
    x,
    y;

  PixelTrait
    alpha_trait;

  unsigned int
    height,
    width;

  XImage
    *annotate_ximage;

  /*
    Initialize annotated image.
  */
  assert(display != (Display *) NULL);
  assert(pixel != (XPixelInfo *) NULL);
  assert(annotate_info != (XAnnotateInfo *) NULL);
  assert(image != (Image *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  /*
    Initialize annotated pixmap.
  */
  if (XDrawAnnotateText(display,annotate_info,&annotate_ximage) == MagickFalse)
    return(MagickFalse);
  /*
    Initialize annotated image.
  */
  if (XAcquireAnnotateImage(annotate_info,&annotate_image,exception) ==
      MagickFalse)
    return(MagickFalse);
  /*
    Transfer annotated X image to image.
  */
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  x=0;
  y=0;
  (void) XParseGeometry(annotate_info->geometry,&x,&y,&width,&height);
  (void) GetOneVirtualPixelInfo(image,UndefinedVirtualPixelMethod,(ssize_t) x,
    (ssize_t) y,&annotate_image->background_color,exception);
  XSetAnnotateAlphaTrait(annotate_info,annotate_image);
  annotate_pixels.annotate_image=annotate_image;
  annotate_pixels.annotate_ximage=annotate_ximage;
  annotate_pixels.pixel=pixel;
  annotate_pixels.annotate_info=annotate_info;
  annotate_pixels.exception=exception;
  y=XTransferAnnotatePixels(&annotate_pixels);
  /*
    Determine annotate geometry.
  */
  (void) XParseGeometry(annotate_info->geometry,&x,&y,&width,&height);
  annotate_image=XScaleAnnotateImage(annotate_image,width,height,exception);
  if (annotate_info->degrees != 0.0)
    {
      Image
        *rotate_image;

      int
        rotations;

      /*
        Rotate image.
      */
      rotate_image=RotateImage(annotate_image,annotate_info->degrees,exception);
      if (rotate_image == (Image *) NULL)
        return(MagickFalse);
      annotate_image=DestroyImage(annotate_image);
      annotate_image=rotate_image;
      /*
        Annotation is relative to the degree of rotation.
      */
      rotations=XQuarterTurns(annotate_info->degrees);
      XRotateAnnotateOrigin(annotate_image,rotations,&x,&y);
    }
  /*
    Composite text onto the image.
  */
  (void) XParseGeometry(annotate_info->geometry,&x,&y,&width,&height);
  alpha_trait=image->alpha_trait;
  (void) CompositeImage(image,annotate_image,
    annotate_image->alpha_trait != UndefinedPixelTrait ? OverCompositeOp :
    CopyCompositeOp,MagickTrue,(ssize_t) x,(ssize_t) y,exception);
  image->alpha_trait=alpha_trait;
  annotate_image=DestroyImage(annotate_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X D r a w I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDrawImage() draws a line on the image.
%
%  The format of the XDrawImage method is:
%
%    MagickBooleanType XDrawImage(Display *display,const XPixelInfo *pixel,
%      XDrawInfo *draw_info,Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o pixel: Specifies a pointer to a XPixelInfo structure.
%
%    o draw_info: Specifies a pointer to a XDrawInfo structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XDrawLineOrRectangle(Display *display,const Pixmap draw_pixmap,
  const GC draw_context,const XDrawInfo *draw_info)
{
  switch (draw_info->element)
  {
    case PointElement:
    default:
    {
      (void) XDrawLines(display,draw_pixmap,draw_context,
        draw_info->coordinate_info,(int) draw_info->number_coordinates,
        CoordModeOrigin);
      break;
    }
    case LineElement:
    {
      (void) XDrawLine(display,draw_pixmap,draw_context,draw_info->line_info.x1,
        draw_info->line_info.y1,draw_info->line_info.x2,
        draw_info->line_info.y2);
      break;
    }
    case RectangleElement:
    {
      (void) XDrawRectangle(display,draw_pixmap,draw_context,
        (int) draw_info->rectangle_info.x,(int) draw_info->rectangle_info.y,
        (unsigned int) draw_info->rectangle_info.width,
        (unsigned int) draw_info->rectangle_info.height);
      break;
    }
    case FillRectangleElement:
    {
      (void) XFillRectangle(display,draw_pixmap,draw_context,
        (int) draw_info->rectangle_info.x,(int) draw_info->rectangle_info.y,
        (unsigned int) draw_info->rectangle_info.width,
        (unsigned int) draw_info->rectangle_info.height);
      break;
    }
  }
}

static void XDrawArcElement(Display *display,const Pixmap draw_pixmap,
  const GC draw_context,const XDrawInfo *draw_info)
{
  switch (draw_info->element)
  {
    default:
    {
      XDrawLineOrRectangle(display,draw_pixmap,draw_context,draw_info);
      break;
    }
    case CircleElement:
    case EllipseElement:
    {
      (void) XDrawArc(display,draw_pixmap,draw_context,
        (int) draw_info->rectangle_info.x,(int) draw_info->rectangle_info.y,
        (unsigned int) draw_info->rectangle_info.width,
        (unsigned int) draw_info->rectangle_info.height,0,360*64);
      break;
    }
    case FillCircleElement:
    case FillEllipseElement:
    {
      (void) XFillArc(display,draw_pixmap,draw_context,
        (int) draw_info->rectangle_info.x,(int) draw_info->rectangle_info.y,
        (unsigned int) draw_info->rectangle_info.width,
        (unsigned int) draw_info->rectangle_info.height,0,360*64);
      break;
    }
  }
}

static void XSetDrawStipple(Display *display,const GC draw_context,
  const XDrawInfo *draw_info)
{
  if (draw_info->stipple !=  (Pixmap) NULL)
    {
      (void) XSetFillStyle(display,draw_context,FillOpaqueStippled);
      (void) XSetStipple(display,draw_context,draw_info->stipple);
    }
}

static void XDrawPolygonOutline(Display *display,
  const Pixmap draw_pixmap,const GC draw_context,const XDrawInfo *draw_info)
{
  XPoint
    *coordinate_info;

  coordinate_info=draw_info->coordinate_info;
  (void) XDrawLines(display,draw_pixmap,draw_context,coordinate_info,
    (int) draw_info->number_coordinates,CoordModeOrigin);
  (void) XDrawLine(display,draw_pixmap,draw_context,
    coordinate_info[draw_info->number_coordinates-1].x,
    coordinate_info[draw_info->number_coordinates-1].y,
    coordinate_info[0].x,coordinate_info[0].y);
}

static void XDrawElementToPixmap(Display *display,const Pixmap draw_pixmap,
  const GC draw_context,const XDrawInfo *draw_info)
{
  /*
    Clear pixmap.
  */
  (void) XFillRectangle(display,draw_pixmap,draw_context,0,0,draw_info->width,
    draw_info->height);
  /*
    Draw line to pixmap.
  */
  (void) XSetBackground(display,draw_context,0);
  (void) XSetForeground(display,draw_context,(size_t) (~0));
  XSetDrawStipple(display,draw_context,draw_info);
  switch (draw_info->element)
  {
    default:
    {
      XDrawArcElement(display,draw_pixmap,draw_context,draw_info);
      break;
    }
    case PolygonElement:
    {
      XDrawPolygonOutline(display,draw_pixmap,draw_context,draw_info);
      break;
    }
    case FillPolygonElement:
    {
      (void) XFillPolygon(display,draw_pixmap,draw_context,
        draw_info->coordinate_info,(int) draw_info->number_coordinates,Complex,
        CoordModeOrigin);
      break;
    }
  }
}

typedef struct _DrawRotationTargets
{
  Image
    **draw_image;

  int
    *x,
    *y;
} DrawRotationTargets;

static MagickBooleanType XRotateDrawImage(const XDrawInfo *draw_info,
  ExceptionInfo *exception,const DrawRotationTargets *targets)
{
  Image
    **const draw_image = targets->draw_image;

  int
    *const x = targets->x,
    *const y = targets->y;

  if (draw_info->degrees != 0.0)
    {
      Image
        *rotate_image;

      int
        rotations;

      /*
        Rotate image.
      */
      rotate_image=RotateImage((*draw_image),draw_info->degrees,exception);
      if (rotate_image == (Image *) NULL)
        return(MagickFalse);
      *draw_image=DestroyImage(*draw_image);
      (*draw_image)=rotate_image;
      /*
        Annotation is relative to the degree of rotation.
      */
      rotations=XQuarterTurns(draw_info->degrees);
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
          (*x)=(*x)-(int) (*draw_image)->columns/2;
          (*y)=(*y)+(int) (*draw_image)->columns/2;
          break;
        }
        case 2:
        {
          /*
            Rotate 180 degrees.
          */
          (*x)=(*x)-(int) (*draw_image)->columns;
          break;
        }
        case 3:
        {
          /*
            Rotate 270 degrees.
          */
          (*x)=(*x)-(int) (*draw_image)->columns/2;
          (*y)=(*y)-(int) ((*draw_image)->rows-((*draw_image)->columns/2));
          break;
        }
      }
    }
  return(MagickTrue);
}

static void XSetDrawRowOpacity(const Image *draw_image,const Image *image,
  Quantum *magick_restrict q)
{
  int
    x;

  for (x=0; x < (int) draw_image->columns; x++)
  {
    if (GetPixelAlpha(image,q) != TransparentAlpha)
      SetPixelAlpha(draw_image,OpaqueAlpha,q);
    q+=(ptrdiff_t) GetPixelChannels(draw_image);
  }
}

static int XSetDrawImageOpacity(const Image *draw_image,const Image *image,
  ExceptionInfo *exception)
{
  CacheView
    *draw_view;

  int
    y;

  draw_view=AcquireAuthenticCacheView(draw_image,exception);
  for (y=0; y < (int) draw_image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    q=GetCacheViewAuthenticPixels(draw_view,0,(ssize_t) y,draw_image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      break;
    XSetDrawRowOpacity(draw_image,image,q);
    if (SyncCacheViewAuthenticPixels(draw_view,exception) == MagickFalse)
      break;
  }
  draw_view=DestroyCacheView(draw_view);
  return(y);
}

typedef struct _DrawPixels
{
  Image
    *draw_image;

  XImage
    *draw_ximage;

  const XPixelInfo
    *pixel;

  XDrawInfo
    *draw_info;

  ExceptionInfo
    *exception;
} DrawPixels;

static void XSetDrawRow(const DrawPixels *draw_pixels,
  Quantum *magick_restrict q,const int y)
{
  Image
    *const draw_image = draw_pixels->draw_image;

  XImage
    *const draw_ximage = draw_pixels->draw_ximage;

  const XPixelInfo
    *const pixel = draw_pixels->pixel;

  XDrawInfo
    *const draw_info = draw_pixels->draw_info;

  int
    x;

  for (x=0; x < (int) draw_image->columns; x++)
  {
    if (XGetPixel(draw_ximage,x,y) == 0)
      {
        /*
          Set this pixel to the background color.
        */
        SetPixelViaPixelInfo(draw_image,&draw_image->background_color,q);
        SetPixelAlpha(draw_image,(Quantum) (draw_info->stencil ==
          OpaqueStencil ? TransparentAlpha : OpaqueAlpha),q);
      }
    else
      {
        /*
          Set this pixel to the pen color.
        */
        SetPixelRed(draw_image,ScaleShortToQuantum(
          pixel->pen_color.red),q);
        SetPixelGreen(draw_image,ScaleShortToQuantum(
          pixel->pen_color.green),q);
        SetPixelBlue(draw_image,ScaleShortToQuantum(
          pixel->pen_color.blue),q);
        SetPixelAlpha(draw_image,(Quantum) (draw_info->stencil ==
          OpaqueStencil ? OpaqueAlpha : TransparentAlpha),q);
      }
    q+=(ptrdiff_t) GetPixelChannels(draw_image);
  }
}

static int XTransferDrawPixels(const DrawPixels *draw_pixels)
{
  Image
    *const draw_image = draw_pixels->draw_image;

  XImage
    *const draw_ximage = draw_pixels->draw_ximage;

  ExceptionInfo
    *const exception = draw_pixels->exception;

  CacheView
    *draw_view;

  int
    y;

  draw_view=AcquireAuthenticCacheView(draw_image,exception);
  for (y=0; y < (int) draw_image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    q=QueueCacheViewAuthenticPixels(draw_view,0,(ssize_t) y,draw_image->columns,
      1,exception);
    if (q == (Quantum *) NULL)
      break;
    XSetDrawRow(draw_pixels,q,y);
    if (SyncCacheViewAuthenticPixels(draw_view,exception) == MagickFalse)
      break;
  }
  draw_view=DestroyCacheView(draw_view);
  XDestroyImage(draw_ximage);
  return(y);
}

typedef struct _XDrawSource
{
  XImage
    *draw_ximage;

  const XPixelInfo
    *pixel;

  XDrawInfo
    *draw_info;
} XDrawSource;

static MagickBooleanType XComposeDrawImage(const XDrawSource *source,
  Image *image,ExceptionInfo *exception)
{
  XImage
    *const draw_ximage = source->draw_ximage;

  const XPixelInfo
    *const pixel = source->pixel;

  XDrawInfo
    *const draw_info = source->draw_info;

  DrawPixels
    draw_pixels;

  DrawRotationTargets
    targets;

  Image
    *draw_image;

  int
    x,
    y;

  PixelTrait
    alpha_trait;

  unsigned int
    height,
    width;

  /*
    Initialize draw image.
  */
  draw_image=AcquireImage((ImageInfo *) NULL,exception);
  if (draw_image == (Image *) NULL)
    return(MagickFalse);
  draw_image->columns=draw_info->width;
  draw_image->rows=draw_info->height;
  /*
    Transfer drawn X image to image.
  */
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  x=0;
  y=0;
  (void) XParseGeometry(draw_info->geometry,&x,&y,&width,&height);
  (void) GetOneVirtualPixelInfo(image,UndefinedVirtualPixelMethod,(ssize_t) x,
    (ssize_t) y,&draw_image->background_color,exception);
  if (SetImageStorageClass(draw_image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  draw_image->alpha_trait=BlendPixelTrait;
  draw_pixels.draw_image=draw_image;
  draw_pixels.draw_ximage=draw_ximage;
  draw_pixels.pixel=pixel;
  draw_pixels.draw_info=draw_info;
  draw_pixels.exception=exception;
  y=XTransferDrawPixels(&draw_pixels);
  /*
    Determine draw geometry.
  */
  (void) XParseGeometry(draw_info->geometry,&x,&y,&width,&height);
  draw_image=XScaleAnnotateImage(draw_image,width,height,exception);
  targets.draw_image=(&draw_image);
  targets.x=(&x);
  targets.y=(&y);
  if (XRotateDrawImage(draw_info,exception,&targets) == MagickFalse)
    return(MagickFalse);
  /*
    Composite text onto the image.
  */
  y=XSetDrawImageOpacity(draw_image,image,exception);
  (void) XParseGeometry(draw_info->geometry,&x,&y,&width,&height);
  if (draw_info->stencil == TransparentStencil)
    (void) CompositeImage(image,draw_image,CopyAlphaCompositeOp,MagickTrue,
      (ssize_t) x,(ssize_t) y,exception);
  else
    {
      alpha_trait=image->alpha_trait;
      (void) CompositeImage(image,draw_image,OverCompositeOp,MagickTrue,
        (ssize_t) x,(ssize_t) y,exception);
      image->alpha_trait=alpha_trait;
    }
  draw_image=DestroyImage(draw_image);
  return(MagickTrue);
}

static GC XCreateDrawContext(Display *display,const Window root_window,
  const XDrawInfo *draw_info)
{
  XGCValues
    context_values;

  context_values.background=(size_t) (~0);
  context_values.foreground=0;
  context_values.line_width=(int) draw_info->line_width;
  return(XCreateGC(display,root_window,(size_t)
    (GCBackground | GCForeground | GCLineWidth),&context_values));
}

static Pixmap XCreateDrawPixmap(Display *display,const Window root_window,
  const XDrawInfo *draw_info)
{
  unsigned int
    depth;

  depth=(unsigned int) XDefaultDepth(display,XDefaultScreen(display));
  return(XCreatePixmap(display,root_window,draw_info->width,
    draw_info->height,depth));
}

MagickPrivate MagickBooleanType XDrawImage(const XImageRenderTarget *target,
  XDrawInfo *draw_info,Image *image,ExceptionInfo *exception)
{
  Display
    *const display = target->display;

  const XPixelInfo
    *const pixel = target->pixel;

  GC
    draw_context;

  Pixmap
    draw_pixmap;

  Window
    root_window;

  XDrawSource
    source;

  XImage
    *draw_ximage;

  /*
    Initialize drawd image.
  */
  assert(display != (Display *) NULL);
  assert(pixel != (XPixelInfo *) NULL);
  assert(draw_info != (XDrawInfo *) NULL);
  assert(image != (Image *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  /*
    Initialize drawd pixmap.
  */
  root_window=XRootWindow(display,XDefaultScreen(display));
  draw_pixmap=XCreateDrawPixmap(display,root_window,draw_info);
  if (draw_pixmap == (Pixmap) NULL)
    return(MagickFalse);
  /*
    Initialize graphics info.
  */
  draw_context=XCreateDrawContext(display,root_window,draw_info);
  if (draw_context == (GC) NULL)
    return(MagickFalse);
  XDrawElementToPixmap(display,draw_pixmap,draw_context,draw_info);
  (void) XFreeGC(display,draw_context);
  /*
    Initialize X image.
  */
  draw_ximage=XGetImage(display,draw_pixmap,0,0,draw_info->width,
    draw_info->height,AllPlanes,ZPixmap);
  if (draw_ximage == (XImage *) NULL)
    return(MagickFalse);
  (void) XFreePixmap(display,draw_pixmap);
  source.draw_ximage=draw_ximage;
  source.pixel=pixel;
  source.draw_info=draw_info;
  return(XComposeDrawImage(&source,image,exception));
}
#endif
