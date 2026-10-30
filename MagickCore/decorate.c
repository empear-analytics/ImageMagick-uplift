/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%            DDDD   EEEEE   CCCC   OOO   RRRR    AAA   TTTTT  EEEEE           %
%            D   D  E      C      O   O  R   R  A   A    T    E               %
%            D   D  EEE    C      O   O  RRRR   AAAAA    T    EEE             %
%            D   D  E      C      O   O  R R    A   A    T    E               %
%            DDDD   EEEEE   CCCC   OOO   R  R   A   A    T    EEEEE           %
%                                                                             %
%                                                                             %
%                     MagickCore Image Decoration Methods                     %
%                                                                             %
%                                Software Design                              %
%                                     Cristy                                  %
%                                   July 1992                                 %
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
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/decorate.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/transform.h"

/*
  Define declarations.
*/
#define AccentuateModulate  ScaleCharToQuantum(80)
#define HighlightModulate  ScaleCharToQuantum(125)
#define ShadowModulate  ScaleCharToQuantum(135)
#define DepthModulate  ScaleCharToQuantum(185)
#define TroughModulate  ScaleCharToQuantum(110)

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B o r d e r I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BorderImage() surrounds the image with a border of the color defined by
%  the bordercolor member of the image structure.  The width and height
%  of the border are defined by the corresponding members of the border_info
%  structure.
%
%  The format of the BorderImage method is:
%
%      Image *BorderImage(const Image *image,const RectangleInfo *border_info,
%        const CompositeOperator compose,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o border_info:  define the width and height of the border.
%
%    o compose:  the composite operator.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *BorderImage(const Image *image,
  const RectangleInfo *border_info,const CompositeOperator compose,
  ExceptionInfo *exception)
{
  Image
    *border_image,
    *clone_image;

  FrameInfo
    frame_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(border_info != (RectangleInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  frame_info.width=image->columns+(border_info->width << 1);
  frame_info.height=image->rows+(border_info->height << 1);
  frame_info.x=(ssize_t) border_info->width;
  frame_info.y=(ssize_t) border_info->height;
  frame_info.inner_bevel=0;
  frame_info.outer_bevel=0;
  clone_image=CloneImage(image,0,0,MagickTrue,exception);
  if (clone_image == (Image *) NULL)
    return((Image *) NULL);
  clone_image->matte_color=image->border_color;
  border_image=FrameImage(clone_image,&frame_info,compose,exception);
  clone_image=DestroyImage(clone_image);
  if (border_image != (Image *) NULL)
    border_image->matte_color=image->matte_color;
  return(border_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F r a m e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FrameImage() adds a simulated three-dimensional border around the image.
%  The color of the border is defined by the matte_color member of image.
%  Members width and height of frame_info specify the border width of the
%  vertical and horizontal sides of the frame.  Members inner and outer
%  indicate the width of the inner and outer shadows of the frame.
%
%  The format of the FrameImage method is:
%
%      Image *FrameImage(const Image *image,const FrameInfo *frame_info,
%        const CompositeOperator compose,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o frame_info: Define the width and height of the frame and its bevels.
%
%    o compose: the composite operator.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Quantum *SetFramePixels(const Image *image,const PixelInfo *color,
  const ssize_t count,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < count; x++)
  {
    SetPixelViaPixelInfo(image,color,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static PixelInfo LightenFrameColor(const PixelInfo matte,const Quantum modulate)
{
  PixelInfo
    color;

  color=matte;
  color.red=(QuantumScale*(((double) QuantumRange-(double)
    modulate)*matte.red+((double) QuantumRange*(double)
    modulate)));
  color.green=(QuantumScale*(((double) QuantumRange-(double)
    modulate)*matte.green+((double) QuantumRange*(double)
    modulate)));
  color.blue=(QuantumScale*(((double) QuantumRange-(double)
    modulate)*matte.blue+((double) QuantumRange*(double)
    modulate)));
  color.black=(QuantumScale*(((double) QuantumRange-(double)
    modulate)*matte.black+((double) QuantumRange*(double)
    modulate)));
  color.alpha=matte.alpha;
  return(color);
}

static PixelInfo DarkenFrameColor(const PixelInfo matte,const Quantum modulate)
{
  PixelInfo
    color;

  color=matte;
  color.red=QuantumScale*matte.red*(double) modulate;
  color.green=QuantumScale*matte.green*(double) modulate;
  color.blue=QuantumScale*matte.blue*(double) modulate;
  color.black=QuantumScale*matte.black*(double) modulate;
  color.alpha=matte.alpha;
  return(color);
}

typedef struct _FrameTopBevel
{
  Image
    *frame_image;

  const FrameInfo
    *frame_info;

  PixelInfo
    accentuate,
    highlight,
    shadow;
} FrameTopBevel;

static Quantum *DrawFrameTopOuterBevel(const FrameTopBevel *top_bevel,
  Quantum *magick_restrict q)
{
  Image
    *const frame_image = top_bevel->frame_image;

  const FrameInfo
    *const frame_info = top_bevel->frame_info;

  const PixelInfo
    accentuate = top_bevel->accentuate,
    highlight = top_bevel->highlight,
    shadow = top_bevel->shadow;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) frame_info->outer_bevel; y++)
  {
    for (x=0; x < ((ssize_t) frame_image->columns-y); x++)
    {
      if (x < y)
        SetPixelViaPixelInfo(frame_image,&highlight,q);
      else
        SetPixelViaPixelInfo(frame_image,&accentuate,q);
      q+=(ptrdiff_t) GetPixelChannels(frame_image);
    }
    for ( ; x < (ssize_t) frame_image->columns; x++)
    {
      SetPixelViaPixelInfo(frame_image,&shadow,q);
      q+=(ptrdiff_t) GetPixelChannels(frame_image);
    }
  }
  return(q);
}

typedef struct _FrameTopInnerBevel
{
  Image
    *frame_image;

  const Image
    *image;

  const FrameInfo
    *frame_info;

  PixelInfo
    highlight,
    matte,
    shadow,
    trough;

  size_t
    bevel_width;
} FrameTopInnerBevel;

static Quantum *DrawFrameTopInnerBevel(const FrameTopInnerBevel *top_inner,
  Quantum *magick_restrict q)
{
  Image
    *const frame_image = top_inner->frame_image;

  const Image
    *const image = top_inner->image;

  const FrameInfo
    *const frame_info = top_inner->frame_info;

  const PixelInfo
    highlight = top_inner->highlight,
    matte = top_inner->matte,
    shadow = top_inner->shadow,
    trough = top_inner->trough;

  const size_t
    bevel_width = top_inner->bevel_width;

  size_t
    width;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) frame_info->inner_bevel; y++)
  {
    q=SetFramePixels(frame_image,&highlight,(ssize_t) frame_info->outer_bevel,q);
    q=SetFramePixels(frame_image,&matte,(frame_info->x-(ssize_t) bevel_width),q);
    width=image->columns+((size_t) frame_info->inner_bevel << 1)-
      (size_t) y;
    for (x=0; x < (ssize_t) width; x++)
    {
      if (x < y)
        SetPixelViaPixelInfo(frame_image,&shadow,q);
      else
        SetPixelViaPixelInfo(frame_image,&trough,q);
      q+=(ptrdiff_t) GetPixelChannels(frame_image);
    }
    for ( ; x < ((ssize_t) image->columns+2*frame_info->inner_bevel); x++)
    {
      SetPixelViaPixelInfo(frame_image,&highlight,q);
      q+=(ptrdiff_t) GetPixelChannels(frame_image);
    }
    width=frame_info->width-(size_t) frame_info->x-
      image->columns-bevel_width;
    q=SetFramePixels(frame_image,&matte,(ssize_t) width,q);
    q=SetFramePixels(frame_image,&shadow,(ssize_t) frame_info->outer_bevel,q);
  }
  return(q);
}

typedef struct _FrameBottomInnerBevel
{
  Image
    *frame_image;

  const Image
    *image;

  const FrameInfo
    *frame_info;

  PixelInfo
    accentuate,
    highlight,
    matte,
    shadow;

  size_t
    bevel_width;
} FrameBottomInnerBevel;

static Quantum *DrawFrameBottomInnerRow(
  const FrameBottomInnerBevel *bottom_inner,const ssize_t y,
  Quantum *magick_restrict q)
{
  Image
    *const frame_image = bottom_inner->frame_image;

  const Image
    *const image = bottom_inner->image;

  const FrameInfo
    *const frame_info = bottom_inner->frame_info;

  const PixelInfo
    accentuate = bottom_inner->accentuate,
    highlight = bottom_inner->highlight,
    matte = bottom_inner->matte,
    shadow = bottom_inner->shadow;

  const size_t
    bevel_width = bottom_inner->bevel_width;

  size_t
    width;

  ssize_t
    x;

  q=SetFramePixels(frame_image,&highlight,(ssize_t) frame_info->outer_bevel,q);
  q=SetFramePixels(frame_image,&matte,(frame_info->x-(ssize_t) bevel_width),q);
  for (x=0; x < y; x++)
  {
    SetPixelViaPixelInfo(frame_image,&shadow,q);
    q+=(ptrdiff_t) GetPixelChannels(frame_image);
  }
  for ( ; x < ((ssize_t) image->columns+2*frame_info->inner_bevel); x++)
  {
    if (x >= ((ssize_t) image->columns+2*frame_info->inner_bevel-y))
      SetPixelViaPixelInfo(frame_image,&highlight,q);
    else
      SetPixelViaPixelInfo(frame_image,&accentuate,q);
    q+=(ptrdiff_t) GetPixelChannels(frame_image);
  }
  width=(size_t) ((ssize_t) frame_info->width-frame_info->x-
    (ssize_t) image->columns-(ssize_t) bevel_width);
  q=SetFramePixels(frame_image,&matte,(ssize_t) width,q);
  q=SetFramePixels(frame_image,&shadow,(ssize_t) frame_info->outer_bevel,q);
  return(q);
}

typedef struct _FrameBottomOuterBevel
{
  Image
    *frame_image;

  PixelInfo
    highlight,
    shadow,
    trough;
} FrameBottomOuterBevel;

static Quantum *DrawFrameBottomOuterRow(
  const FrameBottomOuterBevel *bottom_outer,const ssize_t y,
  Quantum *magick_restrict q)
{
  Image
    *const frame_image = bottom_outer->frame_image;

  const PixelInfo
    highlight = bottom_outer->highlight,
    shadow = bottom_outer->shadow,
    trough = bottom_outer->trough;

  ssize_t
    x;

  for (x=0; x < y; x++)
  {
    SetPixelViaPixelInfo(frame_image,&highlight,q);
    q+=(ptrdiff_t) GetPixelChannels(frame_image);
  }
  for ( ; x < (ssize_t) frame_image->columns; x++)
  {
    if (x >= ((ssize_t) frame_image->columns-y))
      SetPixelViaPixelInfo(frame_image,&shadow,q);
    else
      SetPixelViaPixelInfo(frame_image,&trough,q);
    q+=(ptrdiff_t) GetPixelChannels(frame_image);
  }
  return(q);
}

typedef struct _FrameTopBorder
{
  CacheView
    *frame_view;

  Image
    *frame_image;

  const Image
    *image;

  const FrameInfo
    *frame_info;

  PixelInfo
    accentuate,
    highlight,
    matte,
    shadow,
    trough;

  size_t
    bevel_width,
    height;

  ExceptionInfo
    *exception;
} FrameTopBorder;

static void DrawFrameTopBorder(const FrameTopBorder *top_border)
{
  CacheView
    *const frame_view = top_border->frame_view;

  Image
    *const frame_image = top_border->frame_image;

  const Image
    *const image = top_border->image;

  const FrameInfo
    *const frame_info = top_border->frame_info;

  const PixelInfo
    accentuate = top_border->accentuate,
    highlight = top_border->highlight,
    matte = top_border->matte,
    shadow = top_border->shadow,
    trough = top_border->trough;

  const size_t
    bevel_width = top_border->bevel_width,
    height = top_border->height;

  ExceptionInfo
    *const exception = top_border->exception;

  ssize_t
    y;

  FrameTopBevel
    top_bevel;

  FrameTopInnerBevel
    top_inner;

  Quantum
    *magick_restrict q;

  size_t
    width;

  /*
    Draw top of ornamental border.
  */
  q=QueueCacheViewAuthenticPixels(frame_view,0,0,frame_image->columns,
    height,exception);
  if (q != (Quantum *) NULL)
    {
      /*
        Draw top of ornamental border.
      */
      top_bevel.frame_image=frame_image;
      top_bevel.frame_info=frame_info;
      top_bevel.accentuate=accentuate;
      top_bevel.highlight=highlight;
      top_bevel.shadow=shadow;
      q=DrawFrameTopOuterBevel(&top_bevel,q);
      for (y=0; y < (frame_info->y-(ssize_t) bevel_width); y++)
      {
        q=SetFramePixels(frame_image,&highlight,(ssize_t) frame_info->outer_bevel,q);
        width=frame_image->columns-2*(size_t) frame_info->outer_bevel;
        q=SetFramePixels(frame_image,&matte,(ssize_t) width,q);
        q=SetFramePixels(frame_image,&shadow,(ssize_t) frame_info->outer_bevel,q);
      }
      top_inner.frame_image=frame_image;
      top_inner.image=image;
      top_inner.frame_info=frame_info;
      top_inner.highlight=highlight;
      top_inner.matte=matte;
      top_inner.shadow=shadow;
      top_inner.trough=trough;
      top_inner.bevel_width=bevel_width;
      q=DrawFrameTopInnerBevel(&top_inner,q);
      (void) SyncCacheViewAuthenticPixels(frame_view,exception);
    }
}

MagickExport Image *FrameImage(const Image *image,const FrameInfo *frame_info,
  const CompositeOperator compose,ExceptionInfo *exception)
{
#define FrameImageTag  "Frame/Image"

  CacheView
    *image_view,
    *frame_view;

  Image
    *frame_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    accentuate,
    highlight,
    matte,
    shadow,
    trough;

  size_t
    bevel_width,
    height;

  ssize_t
    x_offset,
    y,
    y_offset;

  /*
    Check frame geometry.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(frame_info != (FrameInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((frame_info->outer_bevel < 0) || (frame_info->inner_bevel < 0))
    ThrowImageException(OptionError,"FrameIsLessThanImageSize");
  bevel_width=(size_t) (frame_info->outer_bevel+frame_info->inner_bevel);
  x_offset=(ssize_t) frame_info->width-frame_info->x-(ssize_t) bevel_width;
  y_offset=(ssize_t) frame_info->height-frame_info->y-(ssize_t) bevel_width;
  if ((x_offset < (ssize_t) image->columns) ||
      (y_offset < (ssize_t) image->rows))
    ThrowImageException(OptionError,"FrameIsLessThanImageSize");
  /*
    Initialize framed image attributes.
  */
  frame_image=CloneImage(image,frame_info->width,frame_info->height,MagickTrue,
    exception);
  if (frame_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(frame_image,DirectClass,exception) == MagickFalse)
    {
      frame_image=DestroyImage(frame_image);
      return((Image *) NULL);
    }
  if ((IsPixelInfoGray(&frame_image->border_color) == MagickFalse) &&
      (IsGrayColorspace(frame_image->colorspace) != MagickFalse))
    (void) SetImageColorspace(frame_image,sRGBColorspace,exception);
  if ((frame_image->matte_color.alpha_trait != UndefinedPixelTrait) &&
      (frame_image->alpha_trait == UndefinedPixelTrait))
    (void) SetImageAlpha(frame_image,OpaqueAlpha,exception);
  frame_image->page=image->page;
  if ((image->page.width != 0) && (image->page.height != 0))
    {
      frame_image->page.width+=frame_image->columns-image->columns;
      frame_image->page.height+=frame_image->rows-image->rows;
    }
  /*
    Initialize 3D effects color.
  */
  matte=image->matte_color;
  accentuate=LightenFrameColor(matte,AccentuateModulate);
  highlight=LightenFrameColor(matte,HighlightModulate);
  shadow=DarkenFrameColor(matte,ShadowModulate);
  trough=DarkenFrameColor(matte,TroughModulate);
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  frame_view=AcquireAuthenticCacheView(frame_image,exception);
  height=(size_t) (frame_info->outer_bevel+(frame_info->y-(ssize_t)
    bevel_width)+frame_info->inner_bevel);
  if (height != 0)
    {
      FrameTopBorder
        top_border;

      top_border.frame_view=frame_view;
      top_border.frame_image=frame_image;
      top_border.image=image;
      top_border.frame_info=frame_info;
      top_border.accentuate=accentuate;
      top_border.highlight=highlight;
      top_border.matte=matte;
      top_border.shadow=shadow;
      top_border.trough=trough;
      top_border.bevel_width=bevel_width;
      top_border.height=height;
      top_border.exception=exception;
      DrawFrameTopBorder(&top_border);
    }
  /*
    Draw sides of ornamental border.
  */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,frame_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    size_t
      width;

    /*
      Initialize scanline with matte color.
    */
    if (status == MagickFalse)
      continue;
    q=QueueCacheViewAuthenticPixels(frame_view,0,frame_info->y+y,
      frame_image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    q=SetFramePixels(frame_image,&highlight,(ssize_t) frame_info->outer_bevel,q);
    q=SetFramePixels(frame_image,&matte,(frame_info->x-(ssize_t) bevel_width),q);
    q=SetFramePixels(frame_image,&shadow,(ssize_t) frame_info->inner_bevel,q);
    /*
      Set frame interior pixels.
    */
    q=SetFramePixels(frame_image,&frame_image->border_color,(ssize_t)
      image->columns,q);
    q=SetFramePixels(frame_image,&highlight,(ssize_t) frame_info->inner_bevel,q);
    width=frame_info->width-(size_t) frame_info->x-image->columns-bevel_width;
    q=SetFramePixels(frame_image,&matte,(ssize_t) width,q);
    q=SetFramePixels(frame_image,&shadow,(ssize_t) frame_info->outer_bevel,q);
    if (SyncCacheViewAuthenticPixels(frame_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,FrameImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  height=(size_t) (frame_info->inner_bevel+(ssize_t) frame_info->height-
    frame_info->y-(ssize_t) image->rows-(ssize_t) bevel_width+
    frame_info->outer_bevel);
  if (height != 0)
    {
      size_t
        width;

      Quantum
        *magick_restrict q;

      /*
        Draw bottom of ornamental border.
      */
      q=QueueCacheViewAuthenticPixels(frame_view,0,(ssize_t) (frame_image->rows-
        height),frame_image->columns,height,exception);
      if (q != (Quantum *) NULL)
        {
          /*
            Draw bottom of ornamental border.
          */
          for (y=frame_info->inner_bevel-1; y >= 0; y--)
          {
            FrameBottomInnerBevel
              bottom_inner;

            bottom_inner.frame_image=frame_image;
            bottom_inner.image=image;
            bottom_inner.frame_info=frame_info;
            bottom_inner.accentuate=accentuate;
            bottom_inner.highlight=highlight;
            bottom_inner.matte=matte;
            bottom_inner.shadow=shadow;
            bottom_inner.bevel_width=bevel_width;
            q=DrawFrameBottomInnerRow(&bottom_inner,y,q);
          }
          height=(size_t) ((ssize_t) frame_info->height-frame_info->y-(ssize_t)
            image->rows-(ssize_t) bevel_width);
          for (y=0; y < (ssize_t) height; y++)
          {
            q=SetFramePixels(frame_image,&highlight,(ssize_t) frame_info->outer_bevel,q);
            width=(size_t) ((ssize_t) frame_image->columns-2*(ssize_t)
              frame_info->outer_bevel);
            q=SetFramePixels(frame_image,&matte,(ssize_t) width,q);
            q=SetFramePixels(frame_image,&shadow,(ssize_t) frame_info->outer_bevel,q);
          }
          for (y=frame_info->outer_bevel-1; y >= 0; y--)
          {
            FrameBottomOuterBevel
              bottom_outer;

            bottom_outer.frame_image=frame_image;
            bottom_outer.highlight=highlight;
            bottom_outer.shadow=shadow;
            bottom_outer.trough=trough;
            q=DrawFrameBottomOuterRow(&bottom_outer,y,q);
          }
          (void) SyncCacheViewAuthenticPixels(frame_view,exception);
        }
    }
  frame_view=DestroyCacheView(frame_view);
  image_view=DestroyCacheView(image_view);
  x_offset=frame_info->outer_bevel+(frame_info->x-(ssize_t) bevel_width)+
    frame_info->inner_bevel;
  y_offset=frame_info->outer_bevel+(frame_info->y-(ssize_t) bevel_width)+
    frame_info->inner_bevel;
  if (status != MagickFalse)
    status=CompositeImage(frame_image,image,compose,MagickTrue,x_offset,
      y_offset,exception);
  if (status == MagickFalse)
    frame_image=DestroyImage(frame_image);
  return(frame_image);
}

