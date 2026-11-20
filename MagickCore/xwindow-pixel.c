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
%                      PPPP   IIIII  X   X  EEEEE  L                          %
%                      P   P    I     X X   E      L                          %
%                      PPPP     I      X    EEE    L                          %
%                      P        I     X X   E      L                          %
%                      P      IIIII  X   X  EEEEE  LLLLL                      %
%                                                                             %
%                                                                             %
%             MagickCore X11 Pixel and Color Information Methods              %
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
  Define declarations.
*/
#define XGammaPacket(map,color)  (size_t) (map->base_pixel+ \
  ((ScaleQuantumToShort(XRedGamma((color)->red))*map->red_max/65535L)* \
    map->red_mult)+ \
  ((ScaleQuantumToShort(XGreenGamma((color)->green))*map->green_max/65535L)* \
    map->green_mult)+ \
  ((ScaleQuantumToShort(XBlueGamma((color)->blue))*map->blue_max/65535L)* \
    map->blue_mult))
#define HighlightModulate  ScaleCharToQuantum(125)
#define ShadowModulate  ScaleCharToQuantum(135)
#define DepthModulate  ScaleCharToQuantum(185)
#define TroughModulate  ScaleCharToQuantum(110)

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X G e t P i x e l I n f o                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XGetPixelInfo() initializes the PixelInfo structure.
%
%  The format of the XGetPixelInfo method is:
%
%      void XGetPixelInfo(Display *display,const XPixelInfoSource *source,
%        Image *image,XPixelInfo *pixel)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o source: the visual, colormap and resources, in these members:
%
%      o visual_info: Specifies a pointer to a X11 XVisualInfo structure;
%        returned from XGetVisualInfo.
%
%      o map_info: If map_type is specified, this structure is initialized
%        with info from the Standard Colormap.
%
%      o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o image: the image.
%
%    o pixel: Specifies a pointer to a XPixelInfo structure.
%
*/
static void XSetDisplayGamma(const XResourceInfo *resource_info,
  const Image *image)
{
  if ((resource_info->gamma_correct != MagickFalse) &&
      (image->gamma != 0.0))
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      /*
        Initialize map relative to display and image gamma.
      */
      flags=ParseGeometry(resource_info->display_gamma,&geometry_info);
      if ((flags & RhoValue) != 0)
        (*XRedGammaReference())=geometry_info.rho;
      (*XGreenGammaReference())=(*XRedGammaReference());
      if ((flags & SigmaValue) != 0)
        (*XGreenGammaReference())=geometry_info.sigma;
      (*XBlueGammaReference())=(*XRedGammaReference());
      if ((flags & XiValue) != 0)
        (*XBlueGammaReference())=geometry_info.xi;
      (*XRedGammaReference())*=image->gamma;
      (*XGreenGammaReference())*=image->gamma;
      (*XBlueGammaReference())*=image->gamma;
    }
}

static void XInitializePseudoPixels(const XStandardColormap *map_info,
  const Image *image,XPixelInfo *pixel)
{
  ssize_t
    i;

  if (image->storage_class == PseudoClass)
    {
      /*
        Initialize pixel array for images of type PseudoClass.
      */
      for (i=0; i < (ssize_t) image->colors; i++)
        pixel->pixels[i]=XGammaPacket(map_info,image->colormap+i);
      for (i=0; i < MaxNumberPens; i++)
        pixel->pixels[image->colors+(size_t) i]=pixel->pen_colors[i].pixel;
      pixel->colors+=MaxNumberPens;
    }
}

static void XGetModulatedColors(const XStandardColormap *map_info,
  XPixelInfo *pixel)
{
  /*
    Set highlight color.
  */
  pixel->highlight_color.red=(unsigned short) (((double)
    pixel->matte_color.red*ScaleQuantumToShort(HighlightModulate))/65535L+
    (ScaleQuantumToShort((Quantum) (QuantumRange-HighlightModulate))));
  pixel->highlight_color.green=(unsigned short) (((double)
    pixel->matte_color.green*ScaleQuantumToShort(HighlightModulate))/65535L+
    (ScaleQuantumToShort((Quantum) (QuantumRange-HighlightModulate))));
  pixel->highlight_color.blue=(unsigned short) (((double)
    pixel->matte_color.blue*ScaleQuantumToShort(HighlightModulate))/65535L+
    (ScaleQuantumToShort((Quantum) (QuantumRange-HighlightModulate))));
  pixel->highlight_color.pixel=XStandardPixel(map_info,&pixel->highlight_color);
  pixel->highlight_color.flags=(char) (DoRed | DoGreen | DoBlue);
  /*
    Set shadow color.
  */
  pixel->shadow_color.red=(unsigned short) (((double)
    pixel->matte_color.red*ScaleQuantumToShort(ShadowModulate))/65535L);
  pixel->shadow_color.green=(unsigned short) (((double)
    pixel->matte_color.green*ScaleQuantumToShort(ShadowModulate))/65535L);
  pixel->shadow_color.blue=(unsigned short) (((double)
    pixel->matte_color.blue*ScaleQuantumToShort(ShadowModulate))/65535L);
  pixel->shadow_color.pixel=XStandardPixel(map_info,&pixel->shadow_color);
  pixel->shadow_color.flags=(char) (DoRed | DoGreen | DoBlue);
  /*
    Set depth color.
  */
  pixel->depth_color.red=(unsigned short) (((double)
    pixel->matte_color.red*ScaleQuantumToShort(DepthModulate))/65535L);
  pixel->depth_color.green=(unsigned short) (((double)
    pixel->matte_color.green*ScaleQuantumToShort(DepthModulate))/65535L);
  pixel->depth_color.blue=(unsigned short) (((double)
    pixel->matte_color.blue*ScaleQuantumToShort(DepthModulate))/65535L);
  pixel->depth_color.pixel=XStandardPixel(map_info,&pixel->depth_color);
  pixel->depth_color.flags=(char) (DoRed | DoGreen | DoBlue);
  /*
    Set trough color.
  */
  pixel->trough_color.red=(unsigned short) (((double)
    pixel->matte_color.red*ScaleQuantumToShort(TroughModulate))/65535L);
  pixel->trough_color.green=(unsigned short) (((double)
    pixel->matte_color.green*ScaleQuantumToShort(TroughModulate))/65535L);
  pixel->trough_color.blue=(unsigned short) (((double)
    pixel->matte_color.blue*ScaleQuantumToShort(TroughModulate))/65535L);
  pixel->trough_color.pixel=XStandardPixel(map_info,&pixel->trough_color);
  pixel->trough_color.flags=(char) (DoRed | DoGreen | DoBlue);
}

typedef struct _XPixelColorContext
{
  Display
    *display;

  Colormap
    colormap;

  const XStandardColormap
    *map_info;

  const XResourceInfo
    *resource_info;

  const char
    **PenColors;
} XPixelColorContext;

static void XGetInterfaceColors(const XPixelColorContext *context,
  XPixelInfo *pixel)
{
  Display
    *const display = context->display;

  const Colormap
    colormap = context->colormap;

  const XStandardColormap
    *const map_info = context->map_info;

  const XResourceInfo
    *const resource_info = context->resource_info;

  Status
    status;

  (void) XParseColor(display,colormap,(char *) ForegroundColor,
    &pixel->foreground_color);
  status=XParseColor(display,colormap,resource_info->foreground_color,
    &pixel->foreground_color);
  if (status == False)
    ThrowXWindowException(XServerError,"ColorIsNotKnownToServer",
      resource_info->foreground_color);
  pixel->foreground_color.pixel=
    XStandardPixel(map_info,&pixel->foreground_color);
  pixel->foreground_color.flags=(char) (DoRed | DoGreen | DoBlue);
  /*
    Set background color.
  */
  (void) XParseColor(display,colormap,"#d6d6d6d6d6d6",&pixel->background_color);
  status=XParseColor(display,colormap,resource_info->background_color,
    &pixel->background_color);
  if (status == False)
    ThrowXWindowException(XServerError,"ColorIsNotKnownToServer",
      resource_info->background_color);
  pixel->background_color.pixel=
    XStandardPixel(map_info,&pixel->background_color);
  pixel->background_color.flags=(char) (DoRed | DoGreen | DoBlue);
  /*
    Set border color.
  */
  (void) XParseColor(display,colormap,(char *) BorderColor,
    &pixel->border_color);
  status=XParseColor(display,colormap,resource_info->border_color,
    &pixel->border_color);
  if (status == False)
    ThrowXWindowException(XServerError,"ColorIsNotKnownToServer",
      resource_info->border_color);
  pixel->border_color.pixel=XStandardPixel(map_info,&pixel->border_color);
  pixel->border_color.flags=(char) (DoRed | DoGreen | DoBlue);
  /*
    Set matte color.
  */
  pixel->matte_color=pixel->background_color;
  if (resource_info->matte_color != (char *) NULL)
    {
      /*
        Matte color is specified as a X resource or command line argument.
      */
      status=XParseColor(display,colormap,resource_info->matte_color,
        &pixel->matte_color);
      if (status == False)
        ThrowXWindowException(XServerError,"ColorIsNotKnownToServer",
          resource_info->matte_color);
      pixel->matte_color.pixel=XStandardPixel(map_info,&pixel->matte_color);
      pixel->matte_color.flags=(char) (DoRed | DoGreen | DoBlue);
    }
}

static void XGetPenColors(const XPixelColorContext *context,
  XPixelInfo *pixel)
{
  Display
    *const display = context->display;

  const Colormap
    colormap = context->colormap;

  const XStandardColormap
    *const map_info = context->map_info;

  const XResourceInfo
    *const resource_info = context->resource_info;

  const char
    **const PenColors = context->PenColors;

  ssize_t
    i;

  Status
    status;

  /*
    Set pen color.
  */
  for (i=0; i < MaxNumberPens; i++)
  {
    (void) XParseColor(display,colormap,(char *) PenColors[i],
      &pixel->pen_colors[i]);
    status=XParseColor(display,colormap,resource_info->pen_colors[i],
      &pixel->pen_colors[i]);
    if (status == False)
      ThrowXWindowException(XServerError,"ColorIsNotKnownToServer",
        resource_info->pen_colors[i]);
    pixel->pen_colors[i].pixel=XStandardPixel(map_info,&pixel->pen_colors[i]);
    pixel->pen_colors[i].flags=(char) (DoRed | DoGreen | DoBlue);
  }
}

MagickPrivate void XGetPixelInfo(Display *display,
  const XPixelInfoSource *source,Image *image,XPixelInfo *pixel)
{
  const XVisualInfo
    *const visual_info = source->visual_info;

  const XStandardColormap
    *const map_info = source->map_info;

  const XResourceInfo
    *const resource_info = source->resource_info;

  static const char
    *PenColors[MaxNumberPens]=
    {
      "#000000000000",  /* black */
      "#00000000ffff",  /* blue */
      "#0000ffffffff",  /* cyan */
      "#0000ffff0000",  /* green */
      "#bdbdbdbdbdbd",  /* gray */
      "#ffff00000000",  /* red */
      "#ffff0000ffff",  /* magenta */
      "#ffffffff0000",  /* yellow */
      "#ffffffffffff",  /* white */
      "#bdbdbdbdbdbd",  /* gray */
      "#bdbdbdbdbdbd"   /* gray */
    };

  Colormap
    colormap;

  unsigned int
    packets;

  XPixelColorContext
    context;

  /*
    Initialize pixel info.
  */
  assert(display != (Display *) NULL);
  assert(visual_info != (XVisualInfo *) NULL);
  assert(map_info != (XStandardColormap *) NULL);
  assert(resource_info != (XResourceInfo *) NULL);
  assert(pixel != (XPixelInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  pixel->colors=0;
  if (image != (Image *) NULL)
    if (image->storage_class == PseudoClass)
      pixel->colors=(ssize_t) image->colors;
  packets=(unsigned int)
    MagickMax((int) pixel->colors,visual_info->colormap_size)+MaxNumberPens;
  if (pixel->pixels != (unsigned long *) NULL)
    pixel->pixels=(unsigned long *) RelinquishMagickMemory(pixel->pixels);
  pixel->pixels=(unsigned long *) AcquireQuantumMemory(packets,
    sizeof(*pixel->pixels));
  if (pixel->pixels == (unsigned long *) NULL)
    ThrowXWindowFatalException(ResourceLimitFatalError,"UnableToGetPixelInfo",
      image->filename);
  /*
    Set foreground color.
  */
  colormap=map_info->colormap;
  context.display=display;
  context.colormap=colormap;
  context.map_info=map_info;
  context.resource_info=resource_info;
  context.PenColors=PenColors;
  XGetInterfaceColors(&context,pixel);
  XGetModulatedColors(map_info,pixel);
  XGetPenColors(&context,pixel);
  pixel->box_color=pixel->background_color;
  pixel->pen_color=pixel->foreground_color;
  pixel->box_index=0;
  pixel->pen_index=1;
  if (image != (Image *) NULL)
    {
      XSetDisplayGamma(resource_info,image);
      XInitializePseudoPixels(map_info,image,pixel);
    }
}
#endif
