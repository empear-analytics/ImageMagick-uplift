/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  V   V  IIIII  SSSSS  U   U   AAA   L                       %
%                  V   V    I    SS     U   U  A   A  L                       %
%                  V   V    I     SSS   U   U  AAAAA  L                       %
%                   V V     I       SS  U   U  A   A  L                       %
%                    V    IIIII  SSSSS   UUU   A   A  LLLLL                   %
%                                                                             %
%               EEEEE  FFFFF  FFFFF  EEEEE   CCCC  TTTTT  SSSSS               %
%               E      F      F      E      C        T    SS                  %
%               EEE    FFF    FFF    EEE    C        T     SSS                %
%               E      F      F      E      C        T       SS               %
%               EEEEE  F      F      EEEEE   CCCC    T    SSSSS               %
%                                                                             %
%            CCCC   OOO   L       OOO   RRRR   IIIII  ZZZZZ  EEEEE            %
%           C      O   O  L      O   O  R   R    I       ZZ  E                %
%           C      O   O  L      O   O  RRRR     I     ZZZ   EEE              %
%           C      O   O  L      O   O  R R      I    ZZ     E                %
%            CCCC   OOO   LLLLL   OOO   R  R   IIIII  ZZZZZ  EEEEE            %
%                                                                             %
%                                                                             %
%                         MagickCore Colorize Method                          %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                 October 1996                                %
%                                                                             %
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
#include "MagickCore/accelerate-private.h"
#include "MagickCore/annotate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/decorate.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/layer.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/visual-effects.h"
#include "MagickCore/visual-effects-pixel-methods-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o l o r i z e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ColorizeImage() blends the fill color with each pixel in the image.
%  A percentage blend is specified with opacity.  Control the application
%  of different color components by specifying a different percentage for
%  each component (e.g. 90/100/10 is 90% red, 100% green, and 10% blue).
%
%  The format of the ColorizeImage method is:
%
%      Image *ColorizeImage(const Image *image,const char *blend,
%        const PixelInfo *colorize,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o blend:  A character string indicating the level of blending as a
%      percentage.
%
%    o colorize: A color value.
%
%    o exception: return any errors or warnings in this structure.
%
*/
#define Colorize(pixel,blend_percentage,colorize)  \
  ((((double) pixel)*(100.0-(blend_percentage))+(colorize)*(blend_percentage))/100.0)

static void ColorizePixelChannels(const Image *colorize_image,
  const PixelInfo *blend_percentage,const PixelInfo *colorize,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(colorize_image); i++)
  {
    PixelTrait traits = GetPixelChannelTraits(colorize_image,
      (PixelChannel) i);
    if (traits == UndefinedPixelTrait)
      continue;
    if ((traits & CopyPixelTrait) != 0)
      continue;
    SetPixelChannel(colorize_image,(PixelChannel) i,ClampToQuantum(
      Colorize(q[i],GetPixelInfoChannel(blend_percentage,(PixelChannel) i),
      GetPixelInfoChannel(colorize,(PixelChannel) i))),q);
  }
}

static PixelInfo ParseColorizeBlendPercentage(const Image *colorize_image,
  const char *blend)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  PixelInfo
    blend_percentage;

  GetPixelInfo(colorize_image,&blend_percentage);
  flags=ParseGeometry(blend,&geometry_info);
  blend_percentage.red=geometry_info.rho;
  blend_percentage.green=geometry_info.rho;
  blend_percentage.blue=geometry_info.rho;
  blend_percentage.black=geometry_info.rho;
  blend_percentage.alpha=(MagickRealType) TransparentAlpha;
  if ((flags & SigmaValue) != 0)
    blend_percentage.green=geometry_info.sigma;
  if ((flags & XiValue) != 0)
    blend_percentage.blue=geometry_info.xi;
  if ((flags & PsiValue) != 0)
    blend_percentage.alpha=geometry_info.psi;
  if (blend_percentage.colorspace == CMYKColorspace)
    {
      if ((flags & PsiValue) != 0)
        blend_percentage.black=geometry_info.psi;
      if ((flags & ChiValue) != 0)
        blend_percentage.alpha=geometry_info.chi;
    }
  return(blend_percentage);
}

typedef struct _ColorizeRow
{
  CacheView
    *image_view;

  Image
    *colorize_image;

  PixelInfo
    blend_percentage;

  const PixelInfo
    *colorize;

  ExceptionInfo
    *exception;
} ColorizeRow;

static MagickBooleanType ColorizeImageRow(const ColorizeRow *colorize_row,
  const ssize_t y,MagickBooleanType *status)
{
  CacheView
    *const image_view = colorize_row->image_view;

  Image
    *const colorize_image = colorize_row->colorize_image;

  const PixelInfo
    blend_percentage = colorize_row->blend_percentage;

  const PixelInfo
    *const colorize = colorize_row->colorize;

  ExceptionInfo
    *const exception = colorize_row->exception;

  MagickBooleanType
    sync;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(image_view,0,y,colorize_image->columns,1,
    exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  for (x=0; x < (ssize_t) colorize_image->columns; x++)
  {
    ColorizePixelChannels(colorize_image,&blend_percentage,colorize,q);
    q+=(ptrdiff_t) GetPixelChannels(colorize_image);
  }
  sync=SyncCacheViewAuthenticPixels(image_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static void ConformColorizeImage(Image *colorize_image,
  const PixelInfo *colorize,ExceptionInfo *exception)
{
  if ((IsGrayColorspace(colorize_image->colorspace) != MagickFalse) ||
      (IsPixelInfoGray(colorize) != MagickFalse))
    (void) SetImageColorspace(colorize_image,sRGBColorspace,exception);
  if ((colorize_image->alpha_trait == UndefinedPixelTrait) &&
      (colorize->alpha_trait != UndefinedPixelTrait))
    (void) SetImageAlpha(colorize_image,OpaqueAlpha,exception);
}

MagickExport Image *ColorizeImage(const Image *image,const char *blend,
  const PixelInfo *colorize,ExceptionInfo *exception)
{
#define ColorizeImageTag  "Colorize/Image"

  CacheView
    *image_view;

  Image
    *colorize_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    blend_percentage;

  ssize_t
    y;

  /*
    Allocate colorized image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (CloneEffectImage(image,&colorize_image,exception) == MagickFalse)
    return((Image *) NULL);
  ConformColorizeImage(colorize_image,colorize,exception);
  if (blend == (const char *) NULL)
    return(colorize_image);
  blend_percentage=ParseColorizeBlendPercentage(colorize_image,blend);
  /*
    Colorize DirectClass image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(colorize_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(colorize_image,colorize_image,colorize_image->rows,2)
#endif
  for (y=0; y < (ssize_t) colorize_image->rows; y++)
  {
    ColorizeRow
      colorize_row;

    colorize_row.image_view=image_view;
    colorize_row.colorize_image=colorize_image;
    colorize_row.blend_percentage=blend_percentage;
    colorize_row.colorize=colorize;
    colorize_row.exception=exception;
    if (ColorizeImageRow(&colorize_row,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(image,ColorizeImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(ReleaseFailedImage(colorize_image,status));
}
