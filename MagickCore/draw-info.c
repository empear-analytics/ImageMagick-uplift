/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                         DDDD   RRRR    AAA   W   W                          %
%                         D   D  R   R  A   A  W   W                          %
%                         D   D  RRRR   AAAAA  W W W                          %
%                         D   D  R R    A   A  WW WW                          %
%                         DDDD   R  R   A   A  W   W                          %
%                                                                             %
%                         IIIII  N   N  FFFFF   OOO                           %
%                           I    NN  N  F      O   O                          %
%                           I    N N N  FFF    O   O                          %
%                           I    N  NN  F      O   O                          %
%                         IIIII  N   N  F       OOO                           %
%                                                                             %
%                                                                             %
%                        MagickCore Draw Info Methods                         %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1998                                   %
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
% Bill Radcliffe of Corbis (www.corbis.com) contributed the polygon
% rendering code based on Paul Heckbert's "Concave Polygon Scan Conversion",
% Graphics Gems, 1990.  Leonard Rosenthal and David Harr of Appligent
% (www.appligent.com) contributed the dash pattern, linecap stroking
% algorithm, and minor rendering improvements.
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/annotate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/constitute-private.h"
#include "MagickCore/draw.h"
#include "MagickCore/draw-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/draw-core-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e D r a w I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireDrawInfo() returns a DrawInfo structure properly initialized.
%
%  The format of the AcquireDrawInfo method is:
%
%      DrawInfo *AcquireDrawInfo(void)
%
*/
MagickExport DrawInfo *AcquireDrawInfo(void)
{
  DrawInfo
    *draw_info;

  draw_info=(DrawInfo *) AcquireCriticalMemory(sizeof(*draw_info));
  GetDrawInfo((ImageInfo *) NULL,draw_info);
  return(draw_info);
}

static void CloneDrawInfoGradientStops(const DrawInfo *draw_info,
  DrawInfo *clone_info)
{
  if (draw_info->gradient.stops != (StopInfo *) NULL)
    {
      size_t
        number_stops;

      number_stops=clone_info->gradient.number_stops;
      clone_info->gradient.stops=(StopInfo *) AcquireQuantumMemory((size_t)
        number_stops,sizeof(*clone_info->gradient.stops));
      if (clone_info->gradient.stops == (StopInfo *) NULL)
        ThrowFatalException(ResourceLimitFatalError,
          "UnableToAllocateDashPattern");
      (void) memcpy(clone_info->gradient.stops,draw_info->gradient.stops,
        (size_t) number_stops*sizeof(*clone_info->gradient.stops));
    }
}

static void CloneDrawInfoString(const char *source,char **value)
{
  if (source != (char *) NULL)
    (void) CloneString(&(*value),source);
}

static void CloneDrawInfoDashPattern(const DrawInfo *draw_info,
  DrawInfo *clone_info)
{
  if (draw_info->dash_pattern != (double *) NULL)
    {
      ssize_t
        x;

      for (x=0; fabs(draw_info->dash_pattern[x]) >= MagickEpsilon; x++) ;
      clone_info->dash_pattern=(double *) AcquireQuantumMemory((size_t) (x+1),
        sizeof(*clone_info->dash_pattern));
      if (clone_info->dash_pattern == (double *) NULL)
        ThrowFatalException(ResourceLimitFatalError,
          "UnableToAllocateDashPattern");
      (void) memset(clone_info->dash_pattern,0,(size_t) (x+1)*
        sizeof(*clone_info->dash_pattern));
      (void) memcpy(clone_info->dash_pattern,draw_info->dash_pattern,(size_t)
        x*sizeof(*clone_info->dash_pattern));
    }
}

static void CloneDrawInfoTextAttributes(const DrawInfo *draw_info,
  DrawInfo *clone_info)
{
  CloneDrawInfoString(draw_info->text,&clone_info->text);
  CloneDrawInfoString(draw_info->font,&clone_info->font);
  CloneDrawInfoString(draw_info->metrics,&clone_info->metrics);
  CloneDrawInfoString(draw_info->family,&clone_info->family);
  clone_info->style=draw_info->style;
  clone_info->stretch=draw_info->stretch;
  clone_info->weight=draw_info->weight;
  CloneDrawInfoString(draw_info->encoding,&clone_info->encoding);
  clone_info->pointsize=draw_info->pointsize;
  clone_info->kerning=draw_info->kerning;
  clone_info->interline_spacing=draw_info->interline_spacing;
  clone_info->interword_spacing=draw_info->interword_spacing;
  clone_info->direction=draw_info->direction;
  clone_info->word_break=draw_info->word_break;
  CloneDrawInfoString(draw_info->density,&clone_info->density);
  clone_info->align=draw_info->align;
  clone_info->undercolor=draw_info->undercolor;
  clone_info->border_color=draw_info->border_color;
  CloneDrawInfoString(draw_info->server_name,&clone_info->server_name);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e D r a w I n f o                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneDrawInfo() makes a copy of the given draw_info structure.  If NULL
%  is specified, a new DrawInfo structure is created initialized to default
%  values.
%
%  The format of the CloneDrawInfo method is:
%
%      DrawInfo *CloneDrawInfo(const ImageInfo *image_info,
%        const DrawInfo *draw_info)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o draw_info: the draw info.
%
*/
MagickExport DrawInfo *CloneDrawInfo(const ImageInfo *image_info,
  const DrawInfo *draw_info)
{
  DrawInfo
    *clone_info;

  ExceptionInfo
    *exception;

  clone_info=(DrawInfo *) AcquireCriticalMemory(sizeof(*clone_info));
  GetDrawInfo(image_info,clone_info);
  if (draw_info == (DrawInfo *) NULL)
    return(clone_info);
  exception=AcquireExceptionInfo();
  CloneDrawInfoString(draw_info->id,&clone_info->id);
  CloneDrawInfoString(draw_info->primitive,&clone_info->primitive);
  CloneDrawInfoString(draw_info->geometry,&clone_info->geometry);
  clone_info->compliance=draw_info->compliance;
  clone_info->viewbox=draw_info->viewbox;
  clone_info->affine=draw_info->affine;
  clone_info->gravity=draw_info->gravity;
  clone_info->fill=draw_info->fill;
  clone_info->stroke=draw_info->stroke;
  clone_info->stroke_width=draw_info->stroke_width;
  if (draw_info->fill_pattern != (Image *) NULL)
    clone_info->fill_pattern=CloneImage(draw_info->fill_pattern,0,0,MagickTrue,
      exception);
  if (draw_info->stroke_pattern != (Image *) NULL)
    clone_info->stroke_pattern=CloneImage(draw_info->stroke_pattern,0,0,
      MagickTrue,exception);
  clone_info->stroke_antialias=draw_info->stroke_antialias;
  clone_info->text_antialias=draw_info->text_antialias;
  clone_info->fill_rule=draw_info->fill_rule;
  clone_info->linecap=draw_info->linecap;
  clone_info->linejoin=draw_info->linejoin;
  clone_info->miterlimit=draw_info->miterlimit;
  clone_info->dash_offset=draw_info->dash_offset;
  clone_info->decorate=draw_info->decorate;
  clone_info->compose=draw_info->compose;
  CloneDrawInfoTextAttributes(draw_info,clone_info);
  CloneDrawInfoDashPattern(draw_info,clone_info);
  clone_info->gradient=draw_info->gradient;
  CloneDrawInfoGradientStops(draw_info,clone_info);
  clone_info->bounds=draw_info->bounds;
  clone_info->fill_alpha=draw_info->fill_alpha;
  clone_info->stroke_alpha=draw_info->stroke_alpha;
  clone_info->element_reference=draw_info->element_reference;
  clone_info->clip_path=draw_info->clip_path;
  clone_info->clip_units=draw_info->clip_units;
  CloneDrawInfoString(draw_info->clip_mask,&clone_info->clip_mask);
  if (draw_info->clipping_mask != (Image *) NULL)
    clone_info->clipping_mask=CloneImage(draw_info->clipping_mask,0,0,
      MagickTrue,exception);
  if (draw_info->composite_mask != (Image *) NULL)
    clone_info->composite_mask=CloneImage(draw_info->composite_mask,0,0,
      MagickTrue,exception);
  clone_info->render=draw_info->render;
  clone_info->debug=draw_info->debug;
  clone_info->macro_expansion=draw_info->macro_expansion;
  exception=DestroyExceptionInfo(exception);
  return(clone_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y D r a w I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyDrawInfo() deallocates memory associated with an DrawInfo structure.
%
%  The format of the DestroyDrawInfo method is:
%
%      DrawInfo *DestroyDrawInfo(DrawInfo *draw_info)
%
%  A description of each parameter follows:
%
%    o draw_info: the draw info.
%
*/
MagickExport DrawInfo *DestroyDrawInfo(DrawInfo *draw_info)
{
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  draw_info->id=DestroyString(draw_info->id);
  draw_info->primitive=DestroyString(draw_info->primitive);
  draw_info->text=DestroyString(draw_info->text);
  draw_info->geometry=DestroyString(draw_info->geometry);
  if (draw_info->fill_pattern != (Image *) NULL)
    draw_info->fill_pattern=DestroyImage(draw_info->fill_pattern);
  if (draw_info->stroke_pattern != (Image *) NULL)
    draw_info->stroke_pattern=DestroyImage(draw_info->stroke_pattern);
  draw_info->font=DestroyString(draw_info->font);
  draw_info->metrics=DestroyString(draw_info->metrics);
  draw_info->family=DestroyString(draw_info->family);
  draw_info->encoding=DestroyString(draw_info->encoding);
  draw_info->density=DestroyString(draw_info->density);
  draw_info->server_name=(char *)
   RelinquishMagickMemory(draw_info->server_name);
  draw_info->dash_pattern=(double *) RelinquishMagickMemory(
    draw_info->dash_pattern);
  draw_info->gradient.stops=(StopInfo *) RelinquishMagickMemory(
    draw_info->gradient.stops);
  draw_info->clip_mask=DestroyString(draw_info->clip_mask);
  if (draw_info->clipping_mask != (Image *) NULL)
    draw_info->clipping_mask=DestroyImage(draw_info->clipping_mask);
  if (draw_info->composite_mask != (Image *) NULL)
    draw_info->composite_mask=DestroyImage(draw_info->composite_mask);
  if (draw_info->image_info != (ImageInfo *) NULL)
    draw_info->image_info=DestroyImageInfo(draw_info->image_info);
  draw_info->signature=(~MagickCoreSignature);
  draw_info=(DrawInfo *) RelinquishMagickMemory(draw_info);
  return(draw_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t A f f i n e M a t r i x                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetAffineMatrix() returns an AffineMatrix initialized to the identity
%  matrix.
%
%  The format of the GetAffineMatrix method is:
%
%      void GetAffineMatrix(AffineMatrix *affine_matrix)
%
%  A description of each parameter follows:
%
%    o affine_matrix: the affine matrix.
%
*/
MagickExport void GetAffineMatrix(AffineMatrix *affine_matrix)
{
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(affine_matrix != (AffineMatrix *) NULL);
  (void) memset(affine_matrix,0,sizeof(*affine_matrix));
  affine_matrix->sx=1.0;
  affine_matrix->sy=1.0;
}

static void SetDrawInfoDefaults(DrawInfo *draw_info)
{
  draw_info->stroke_antialias=draw_info->image_info->antialias;
  draw_info->stroke_width=1.0;
  draw_info->fill_rule=EvenOddRule;
  draw_info->alpha=OpaqueAlpha;
  draw_info->fill_alpha=OpaqueAlpha;
  draw_info->stroke_alpha=OpaqueAlpha;
  draw_info->linecap=ButtCap;
  draw_info->linejoin=MiterJoin;
  draw_info->miterlimit=10;
  draw_info->decorate=NoDecoration;
  draw_info->pointsize=12.0;
  draw_info->undercolor.alpha=(MagickRealType) TransparentAlpha;
  draw_info->compose=OverCompositeOp;
  draw_info->render=MagickTrue;
  draw_info->clip_path=MagickFalse;
  draw_info->debug=(GetLogEventMask() & (DrawEvent | AnnotateEvent)) != 0 ?
    MagickTrue : MagickFalse;
  draw_info->macro_expansion=(size_t *) NULL;
}

static void SetDrawImageInfoAttributes(DrawInfo *draw_info)
{
  if (draw_info->image_info->font != (char *) NULL)
    draw_info->font=AcquireString(draw_info->image_info->font);
  if (draw_info->image_info->density != (char *) NULL)
    draw_info->density=AcquireString(draw_info->image_info->density);
  draw_info->text_antialias=draw_info->image_info->antialias;
  if (fabs(draw_info->image_info->pointsize) >= MagickEpsilon)
    draw_info->pointsize=draw_info->image_info->pointsize;
  draw_info->border_color=draw_info->image_info->border_color;
  if (draw_info->image_info->server_name != (char *) NULL)
    draw_info->server_name=AcquireString(draw_info->image_info->server_name);
}

static void SetDrawWeightOption(DrawInfo *draw_info)
{
  const char
    *option;

  option=GetImageOption(draw_info->image_info,"weight");
  if (option != (const char *) NULL)
    {
      ssize_t
        weight;

      weight=ParseCommandOption(MagickWeightOptions,MagickFalse,option);
      if (weight == -1)
        weight=(ssize_t) StringToUnsignedLong(option);
      draw_info->weight=(size_t) weight;
    }
}

static void SetDrawColorOption(const ImageInfo *image_info,const char *key,
  PixelInfo *color,ExceptionInfo *exception)
{
  const char
    *option;

  option=GetImageOption(image_info,key);
  if (option != (const char *) NULL)
    (void) QueryColorCompliance(option,AllCompliance,&(*color),exception);
}

static void SetDrawValueOption(const ImageInfo *image_info,const char *key,
  double *value)
{
  char
    *next_token;

  const char
    *option;

  option=GetImageOption(image_info,key);
  if (option != (const char *) NULL)
    (*value)=GetDrawValue(option,&next_token);
}

static void SetDrawStringOption(const ImageInfo *image_info,const char *key,
  char **string)
{
  const char
    *option;

  option=GetImageOption(image_info,key);
  if (option != (const char *) NULL)
    (void) CloneString(&(*string),option);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t D r a w I n f o                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDrawInfo() initializes draw_info to default values from image_info.
%
%  The format of the GetDrawInfo method is:
%
%      void GetDrawInfo(const ImageInfo *image_info,DrawInfo *draw_info)
%
%  A description of each parameter follows:
%
%    o image_info: the image info..
%
%    o draw_info: the draw info.
%
*/
MagickExport void GetDrawInfo(const ImageInfo *image_info,DrawInfo *draw_info)
{

  const char
    *option;

  ExceptionInfo
    *exception;

  /*
    Initialize draw attributes.
  */
  assert(draw_info != (DrawInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  (void) memset(draw_info,0,sizeof(*draw_info));
  draw_info->image_info=CloneImageInfo(image_info);
  GetAffineMatrix(&draw_info->affine);
  exception=AcquireExceptionInfo();
  (void) QueryColorCompliance("#000F",AllCompliance,&draw_info->fill,
    exception);
  (void) QueryColorCompliance("#FFF0",AllCompliance,&draw_info->stroke,
    exception);
  SetDrawInfoDefaults(draw_info);
  SetDrawImageInfoAttributes(draw_info);
  option=GetImageOption(draw_info->image_info,"direction");
  if (option != (const char *) NULL)
    draw_info->direction=(DirectionType) ParseCommandOption(
      MagickDirectionOptions,MagickFalse,option);
  else
    draw_info->direction=UndefinedDirection;
  SetDrawStringOption(draw_info->image_info,"encoding",
    &draw_info->encoding);
  SetDrawStringOption(draw_info->image_info,"family",
    &draw_info->family);
  SetDrawColorOption(draw_info->image_info,"fill",
    &draw_info->fill,exception);
  option=GetImageOption(draw_info->image_info,"gravity");
  if (option != (const char *) NULL)
    draw_info->gravity=(GravityType) ParseCommandOption(MagickGravityOptions,
      MagickFalse,option);
  SetDrawValueOption(draw_info->image_info,"interline-spacing",
    &draw_info->interline_spacing);
  SetDrawValueOption(draw_info->image_info,"interword-spacing",
    &draw_info->interword_spacing);
  SetDrawValueOption(draw_info->image_info,"kerning",
    &draw_info->kerning);
  SetDrawColorOption(draw_info->image_info,"stroke",
    &draw_info->stroke,exception);
  SetDrawValueOption(draw_info->image_info,"strokewidth",
    &draw_info->stroke_width);
  option=GetImageOption(draw_info->image_info,"style");
  if (option != (const char *) NULL)
    draw_info->style=(StyleType) ParseCommandOption(MagickStyleOptions,
      MagickFalse,option);
  SetDrawColorOption(draw_info->image_info,"undercolor",
    &draw_info->undercolor,exception);
  SetDrawWeightOption(draw_info);
  option=GetImageOption(draw_info->image_info,"word-break");
  if (option != (const char *) NULL)
    draw_info->word_break=(WordBreakType) ParseCommandOption(
      MagickWordBreakOptions,MagickFalse,option);
  exception=DestroyExceptionInfo(exception);
  draw_info->signature=MagickCoreSignature;
}
