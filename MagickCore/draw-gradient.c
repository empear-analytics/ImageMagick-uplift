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
%            GGGG  RRRR    AAA   DDDD   IIIII  EEEEE  N   N  TTTTT            %
%           G      R   R  A   A  D   D    I    E      NN  N    T              %
%           G  GG  RRRR   AAAAA  D   D    I    EEE    N N N    T              %
%           G   G  R R    A   A  D   D    I    E      N  NN    T              %
%            GGG   R  R   A   A  DDDD   IIIII  EEEEE  N   N    T              %
%                                                                             %
%                                                                             %
%                     MagickCore Gradient Drawing Methods                     %
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     D r a w G r a d i e n t I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGradientImage() draws a linear gradient on the image.
%
%  The format of the DrawGradientImage method is:
%
%      MagickBooleanType DrawGradientImage(Image *image,
%        const DrawInfo *draw_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline double GetStopColorOffset(const GradientInfo *gradient,
  const ssize_t x,const ssize_t y)
{
  switch (gradient->type)
  {
    case UndefinedGradient:
    case LinearGradient:
    {
      double
        gamma,
        length,
        offset,
        scale;

      PointInfo
        p,
        q;

      const SegmentInfo
        *gradient_vector;

      gradient_vector=(&gradient->gradient_vector);
      p.x=gradient_vector->x2-gradient_vector->x1;
      p.y=gradient_vector->y2-gradient_vector->y1;
      q.x=(double) x-gradient_vector->x1;
      q.y=(double) y-gradient_vector->y1;
      length=sqrt(q.x*q.x+q.y*q.y);
      gamma=sqrt(p.x*p.x+p.y*p.y)*length;
      gamma=MagickSafeReciprocal(gamma);
      scale=p.x*q.x+p.y*q.y;
      offset=gamma*scale*length;
      return(offset);
    }
    case RadialGradient:
    {
      PointInfo
        v;

      if (gradient->spread == RepeatSpread)
        {
          v.x=(double) x-gradient->center.x;
          v.y=(double) y-gradient->center.y;
          return(sqrt(v.x*v.x+v.y*v.y));
        }
      v.x=(double) (((x-gradient->center.x)*cos(DegreesToRadians(
        gradient->angle)))+((y-gradient->center.y)*sin(DegreesToRadians(
        gradient->angle))))*MagickSafeReciprocal(gradient->radii.x);
      v.y=(double) (((x-gradient->center.x)*sin(DegreesToRadians(
        gradient->angle)))-((y-gradient->center.y)*cos(DegreesToRadians(
        gradient->angle))))*MagickSafeReciprocal(gradient->radii.y);
      return(sqrt(v.x*v.x+v.y*v.y));
    }
  }
  return(0.0);
}

static int StopInfoCompare(const void *x,const void *y)
{
  StopInfo
    *stop_1,
    *stop_2;

  stop_1=(StopInfo *) x;
  stop_2=(StopInfo *) y;
  if (stop_1->offset > stop_2->offset)
    return(1);
  if (fabs(stop_1->offset-stop_2->offset) <= MagickEpsilon)
    return(0);
  return(-1);
}

static PixelInfo GetPadGradientColor(const GradientInfo *gradient,
  const double offset,PixelInfo composite)
{
  double
    alpha;

  ssize_t
    i,
    j;

  for (i=0; i < (ssize_t) gradient->number_stops; i++)
    if (offset < gradient->stops[i].offset)
      break;
  if ((offset < 0.0) || (i == 0))
    composite=gradient->stops[0].color;
  else
    if ((offset > 1.0) || (i == (ssize_t) gradient->number_stops))
      composite=gradient->stops[gradient->number_stops-1].color;
    else
      {
        j=i;
        i--;
        alpha=(offset-gradient->stops[i].offset)/
          (gradient->stops[j].offset-gradient->stops[i].offset);
        CompositePixelInfoBlend(&gradient->stops[i].color,1.0-alpha,
          &gradient->stops[j].color,alpha,&composite);
      }
  return(composite);
}

static PixelInfo GetReflectGradientColor(const GradientInfo *gradient,
  const double offset,PixelInfo composite)
{
  double
    alpha;

  ssize_t
    i,
    j;

  for (i=0; i < (ssize_t) gradient->number_stops; i++)
    if (offset < gradient->stops[i].offset)
      break;
  if (i == 0)
    composite=gradient->stops[0].color;
  else
    if (i == (ssize_t) gradient->number_stops)
      composite=gradient->stops[gradient->number_stops-1].color;
    else
      {
        j=i;
        i--;
        alpha=(offset-gradient->stops[i].offset)/
          (gradient->stops[j].offset-gradient->stops[i].offset);
        CompositePixelInfoBlend(&gradient->stops[i].color,1.0-alpha,
          &gradient->stops[j].color,alpha,&composite);
      }
  return(composite);
}

static double GetRepeatOffset(const double offset,const double extent)
{
  double
    repeat;

  repeat=fmod(offset,extent);
  if (repeat < 0.0)
    repeat=extent-fmod(-repeat,extent);
  else
    repeat=fmod(offset,extent);
  return(repeat);
}

typedef struct _RepeatGradientInfo
{
  const GradientInfo
    *gradient;

  double
    length,
    offset,
    repeat;

  MagickBooleanType
    antialias;
} RepeatGradientInfo;

static PixelInfo BlendRepeatGradientStops(const RepeatGradientInfo *repeat_info,
  ssize_t i,PixelInfo composite)
{
  const GradientInfo
    *gradient = repeat_info->gradient;

  const double
    length = repeat_info->length,
    offset = repeat_info->offset,
    repeat = repeat_info->repeat;

  const MagickBooleanType
    antialias = repeat_info->antialias;

  double
    alpha;

  ssize_t
    j;

  j=i;
  i--;
  alpha=(offset-gradient->stops[i].offset)/
    (gradient->stops[j].offset-gradient->stops[i].offset);
  if (antialias != MagickFalse)
    {
      if (gradient->type == LinearGradient)
        alpha=length-repeat;
      else
        alpha=gradient->radius-repeat;
      i=0;
      j=(ssize_t) gradient->number_stops-1L;
    }
  CompositePixelInfoBlend(&gradient->stops[i].color,1.0-alpha,
    &gradient->stops[j].color,alpha,&composite);
  return(composite);
}

static PixelInfo GetRepeatGradientColor(const RepeatGradientInfo *repeat_info,
  PixelInfo composite)
{
  const GradientInfo
    *gradient = repeat_info->gradient;

  const double
    offset = repeat_info->offset;

  ssize_t
    i;

  for (i=0; i < (ssize_t) gradient->number_stops; i++)
    if (offset < gradient->stops[i].offset)
      break;
  if (i == 0)
    composite=gradient->stops[0].color;
  else
    if (i == (ssize_t) gradient->number_stops)
      composite=gradient->stops[gradient->number_stops-1].color;
    else
      composite=BlendRepeatGradientStops(repeat_info,i,composite);
  return(composite);
}

static inline MagickBooleanType IsOffGradientOrigin(
  const SegmentInfo *gradient_vector,const ssize_t x,const ssize_t y)
{
  return(((x != CastDoubleToSsizeT(ceil(gradient_vector->x1-0.5))) ||
    (y != CastDoubleToSsizeT(ceil(gradient_vector->y1-0.5)))) ? MagickTrue :
    MagickFalse);
}

static double GetScaledStopOffset(const GradientInfo *gradient,
  const double length,const ssize_t x,const ssize_t y)
{
  double
    offset;

  offset=GetStopColorOffset(gradient,x,y);
  if (gradient->type != RadialGradient)
    offset*=MagickSafeReciprocal(length);
  return(offset);
}

typedef struct _GradientOffsetInfo
{
  const GradientInfo
    *gradient;

  const SegmentInfo
    *gradient_vector;

  double
    length;

  ssize_t
    x,
    y;
} GradientOffsetInfo;

static void GetRepeatSpreadOffset(const GradientOffsetInfo *offset_info,
  double *offset,double *repeat,MagickBooleanType *antialias)
{
  const GradientInfo
    *gradient = offset_info->gradient;

  const SegmentInfo
    *gradient_vector = offset_info->gradient_vector;

  const double
    length = offset_info->length;

  const ssize_t
    x = offset_info->x,
    y = offset_info->y;

  if (IsOffGradientOrigin(gradient_vector,x,y) != MagickFalse)
    {
      (*offset)=GetStopColorOffset(gradient,x,y);
      if (gradient->type == LinearGradient)
        {
          (*repeat)=GetRepeatOffset((*offset),length);
          (*antialias)=((*repeat) < length) && (((*repeat)+1.0) > length) ?
            MagickTrue : MagickFalse;
          (*offset)=MagickSafeReciprocal(length)*(*repeat);
        }
      else
        {
          (*repeat)=GetRepeatOffset((*offset),gradient->radius);
          (*antialias)=(*repeat)+1.0 > gradient->radius ? MagickTrue :
            MagickFalse;
          (*offset)=(*repeat)*MagickSafeReciprocal(gradient->radius);
        }
    }
}

static double GetPadSpreadOffset(const GradientOffsetInfo *offset_info,
  double offset)
{
  const GradientInfo
    *gradient = offset_info->gradient;

  const SegmentInfo
    *gradient_vector = offset_info->gradient_vector;

  const double
    length = offset_info->length;

  const ssize_t
    x = offset_info->x,
    y = offset_info->y;

  if (IsOffGradientOrigin(gradient_vector,x,y) != MagickFalse)
    {
      offset=GetScaledStopOffset(gradient,length,x,y);
    }
  return(offset);
}

static double GetReflectSpreadOffset(const GradientOffsetInfo *offset_info,
  double offset)
{
  offset=GetPadSpreadOffset(offset_info,offset);
  if (offset < 0.0)
    offset=(-offset);
  if ((ssize_t) fmod(offset,2.0) == 0)
    offset=fmod(offset,1.0);
  else
    offset=1.0-fmod(offset,1.0);
  return(offset);
}

typedef struct _GradientSpreadInput
{
  const GradientInfo
    *gradient;

  const SegmentInfo
    *gradient_vector;

  double
    length;

  ssize_t
    x,
    y;
} GradientSpreadInput;

static void GetSpreadGradientColor(const GradientSpreadInput *spread,
  double *offset,PixelInfo *composite)
{
  const GradientInfo
    *gradient = spread->gradient;

  const SegmentInfo
    *gradient_vector = spread->gradient_vector;

  const double
    length = spread->length;

  const ssize_t
    x = spread->x,
    y = spread->y;

  switch (gradient->spread)
  {
    case UndefinedSpread:
    case PadSpread:
    {
      GradientOffsetInfo
        offset_info;

      offset_info.gradient=gradient;
      offset_info.gradient_vector=gradient_vector;
      offset_info.length=length;
      offset_info.x=x;
      offset_info.y=y;
      (*offset)=GetPadSpreadOffset(&offset_info,(*offset));
      (*composite)=GetPadGradientColor(gradient,(*offset),(*composite));
      break;
    }
    case ReflectSpread:
    {
      GradientOffsetInfo
        offset_info;

      offset_info.gradient=gradient;
      offset_info.gradient_vector=gradient_vector;
      offset_info.length=length;
      offset_info.x=x;
      offset_info.y=y;
      (*offset)=GetReflectSpreadOffset(&offset_info,(*offset));
      (*composite)=GetReflectGradientColor(gradient,(*offset),(*composite));
      break;
    }
    case RepeatSpread:
    {
      double
        repeat;

      MagickBooleanType
        antialias;

      RepeatGradientInfo
        repeat_info;

      GradientOffsetInfo
        offset_info;

      antialias=MagickFalse;
      repeat=0.0;
      offset_info.gradient=gradient;
      offset_info.gradient_vector=gradient_vector;
      offset_info.length=length;
      offset_info.x=x;
      offset_info.y=y;
      GetRepeatSpreadOffset(&offset_info,offset,&repeat,&antialias);
      repeat_info.gradient=gradient;
      repeat_info.length=length;
      repeat_info.offset=(*offset);
      repeat_info.repeat=repeat;
      repeat_info.antialias=antialias;
      (*composite)=GetRepeatGradientColor(&repeat_info,(*composite));
      break;
    }
  }
}

MagickExport MagickBooleanType DrawGradientImage(Image *image,
  const DrawInfo *draw_info,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  const GradientInfo
    *gradient;

  const SegmentInfo
    *gradient_vector;

  double
    length;

  MagickBooleanType
    status;

  PixelInfo
    zero;

  PointInfo
    point;

  RectangleInfo
    bounding_box;

  size_t
    height;

  ssize_t
    y;

  /*
    Draw linear or radial gradient on image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(draw_info != (const DrawInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  gradient=(&draw_info->gradient);
  qsort(gradient->stops,gradient->number_stops,sizeof(StopInfo),
    StopInfoCompare);
  gradient_vector=(&gradient->gradient_vector);
  point.x=gradient_vector->x2-gradient_vector->x1;
  point.y=gradient_vector->y2-gradient_vector->y1;
  length=sqrt(point.x*point.x+point.y*point.y);
  bounding_box=gradient->bounding_box;
  status=MagickTrue;
  GetPixelInfo(image,&zero);
  image_view=AcquireAuthenticCacheView(image,exception);
  height=(size_t) (bounding_box.y+(ssize_t) bounding_box.height);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,height,1)
#endif
  for (y=bounding_box.y; y < (ssize_t) height; y++)
  {
    double
      offset;

    PixelInfo
      composite,
      pixel;

    Quantum
      *magick_restrict q;

    size_t
      width;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,bounding_box.x,y,(size_t)
      bounding_box.width,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    pixel=zero;
    composite=zero;
    offset=GetScaledStopOffset(gradient,length,0,y);
    width=(size_t) (bounding_box.x+(ssize_t) bounding_box.width);
    for (x=bounding_box.x; x < (ssize_t) width; x++)
    {
      GetPixelInfoPixel(image,q,&pixel);
      {
        const GradientSpreadInput
          spread = { gradient, gradient_vector, length, x, y };

        GetSpreadGradientColor(&spread,&offset,&composite);
      }
      CompositePixelInfoOver(&composite,composite.alpha,&pixel,pixel.alpha,
        &pixel);
      SetPixelViaPixelInfo(image,&pixel,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}
