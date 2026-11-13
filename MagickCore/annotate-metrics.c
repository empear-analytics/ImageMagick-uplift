/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%            AAA   N   N  N   N   OOO   TTTTT   AAA   TTTTT  EEEEE            %
%           A   A  NN  N  NN  N  O   O    T    A   A    T    E                %
%           AAAAA  N N N  N N N  O   O    T    AAAAA    T    EEE              %
%           A   A  N  NN  N  NN  O   O    T    A   A    T    E                %
%           A   A  N   N  N   N   OOO     T    A   A    T    EEEEE            %
%                                                                             %
%               M   M  EEEEE  TTTTT  RRRR   IIIII   CCCC  SSSSS               %
%               MM MM  E        T    R   R    I    C      SS                  %
%               M M M  EEE      T    RRRR     I    C       SSS                %
%               M   M  E        T    R R      I    C         SS               %
%               M   M  EEEEE    T    R  R   IIIII   CCCC  SSSSS               %
%                                                                             %
%                                                                             %
%                           Multiline Type Metrics                            %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
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
% Digital Applications (www.digapp.com) contributed the stroked text algorithm.
% It was written by Leonard Rosenthol.
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/annotate.h"
#include "MagickCore/annotate-private.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/draw-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/log.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/policy.h"
#include "MagickCore/property.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/type.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/xwindow.h"
#include "MagickCore/xwindow-private.h"
#include "coders/ghostscript-private.h"
#if defined(MAGICKCORE_FREETYPE_DELEGATE)
#if defined(__MINGW32__)
#  undef interface
#endif
#include <ft2build.h>
#if defined(FT_FREETYPE_H)
#  include FT_FREETYPE_H
#else
#  include <freetype/freetype.h>
#endif
#if defined(FT_GLYPH_H)
#  include FT_GLYPH_H
#else
#  include <freetype/ftglyph.h>
#endif
#if defined(FT_OUTLINE_H)
#  include FT_OUTLINE_H
#else
#  include <freetype/ftoutln.h>
#endif
#if defined(FT_BBOX_H)
#  include FT_BBOX_H
#else
#  include <freetype/ftbbox.h>
#endif
#if defined(FT_MODULE_H)
#  include FT_MODULE_H
#else
#  include <freetype/ftmodapi.h>
#endif
#endif
#if defined(MAGICKCORE_RAQM_DELEGATE)
#include <raqm.h>
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M u l t i l i n e T y p e M e t r i c s                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMultilineTypeMetrics() returns the following information for the
%  specified font and text:
%
%    character width
%    character height
%    ascender
%    descender
%    text width
%    text height
%    maximum horizontal advance
%    bounds: x1
%    bounds: y1
%    bounds: x2
%    bounds: y2
%    origin: x
%    origin: y
%    underline position
%    underline thickness
%
%  This method is like GetTypeMetrics() but it returns the maximum text width
%  and height for multiple lines of text.
%
%  The format of the GetMultilineTypeMetrics method is:
%
%      MagickBooleanType GetMultilineTypeMetrics(Image *image,
%        const DrawInfo *draw_info,TypeMetric *metrics,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o metrics: Return the font metrics in this structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void DestroyTextLines(char **textlist,const size_t count)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) count; i++)
    textlist[i]=DestroyString(textlist[i]);
}

static MagickBooleanType CheckWidthLimit(Image *image,const TypeMetric extent,
  ExceptionInfo *exception,MagickBooleanType *status)
{
  MagickSizeType
    size;

  size=(MagickSizeType) fabs(extent.width);
  if (AcquireMagickResource(WidthResource,size) == MagickFalse)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "WidthOrHeightExceedsLimit","`%s'",image->filename);
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static inline void KeepWidestMetrics(TypeMetric *metrics,
  const TypeMetric extent)
{
  if (extent.width > metrics->width)
    *metrics=extent;
}

static void DestroyMultilineText(DrawInfo *annotate_info,char **textlist,
  const size_t count)
{
  annotate_info->text=(char *) NULL;
  annotate_info=DestroyDrawInfo(annotate_info);
  DestroyTextLines(textlist,count);
  textlist=(char **) RelinquishMagickMemory(textlist);
}

static void ResetMultilineMetrics(DrawInfo *annotate_info,TypeMetric *metrics)
{
  annotate_info->render=MagickFalse;
  annotate_info->direction=UndefinedDirection;
  (void) memset(metrics,0,sizeof(*metrics));
}

static double GetMultilineHeight(const DrawInfo *draw_info,
  const TypeMetric *metrics,const size_t count)
{
  return(count*(size_t) (metrics->ascent-metrics->descent+
    0.5)+(count-1)*draw_info->interline_spacing);
}

static DrawInfo *CloneMultilineDrawInfo(const DrawInfo *draw_info)
{
  DrawInfo
    *annotate_info;

  annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  annotate_info->text=DestroyString(annotate_info->text);
  return(annotate_info);
}

MagickExport MagickBooleanType GetMultilineTypeMetrics(Image *image,
  const DrawInfo *draw_info,TypeMetric *metrics,ExceptionInfo *exception)
{
  char
    **textlist;

  double
    height;

  DrawInfo
    *annotate_info;

  MagickBooleanType
    status;

  MagickSizeType
    size;

  ssize_t
    i;

  size_t
    count;

  TypeMetric
    extent;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->text != (char *) NULL);
  assert(draw_info->signature == MagickCoreSignature);
  if (*draw_info->text == '\0')
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "LabelExpected","`%s'",image->filename);
      return(MagickFalse);
    }
  annotate_info=CloneMultilineDrawInfo(draw_info);
  /*
    Convert newlines to multiple lines of text.
  */
  textlist=StringToStrings(draw_info->text,&count);
  if (textlist == (char **) NULL)
    {
      annotate_info=DestroyDrawInfo(annotate_info);
      return(MagickFalse);
    }
  ResetMultilineMetrics(annotate_info,metrics);
  (void) memset(&extent,0,sizeof(extent));
  /*
    Find the widest of the text lines.
  */
  annotate_info->text=textlist[0];
  status=GetTypeMetrics(image,annotate_info,&extent,exception);
  *metrics=extent;
  height=GetMultilineHeight(draw_info,metrics,count);
  size=(MagickSizeType) fabs(height);
  if (AcquireMagickResource(HeightResource,size) == MagickFalse)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "WidthOrHeightExceedsLimit","`%s'",image->filename);
      status=MagickFalse;
    }
  else
    {
      for (i=1; i < (ssize_t) count; i++)
      {
        annotate_info->text=textlist[i];
        status=GetTypeMetrics(image,annotate_info,&extent,exception);
        if (status == MagickFalse)
          break;
        KeepWidestMetrics(metrics,extent);
        if (CheckWidthLimit(image,extent,exception,&status) == MagickFalse)
          break;
      }
      metrics->height=(double) height;
    }
  /*
    Relinquish resources.
  */
  DestroyMultilineText(annotate_info,textlist,count);
  return(status);
}
