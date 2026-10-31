/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%           DDDD   EEEEE   CCCC   OOO   RRRR    AAA   TTTTT  EEEEE            %
%           D   D  E      C      O   O  R   R  A   A    T    E                %
%           D   D  EEE    C      O   O  RRRR   AAAAA    T    EEE              %
%           D   D  E      C      O   O  R R    A   A    T    E                %
%           DDDD   EEEEE   CCCC   OOO   R  R   A   A    T    EEEEE            %
%                                                                             %
%                      RRRR    AAA   IIIII  SSSSS  EEEEE                      %
%                      R   R  A   A    I    SS     E                          %
%                      RRRR   AAAAA    I     SSS   EEE                        %
%                      R R    A   A    I       SS  E                          %
%                      R  R   A   A  IIIII  SSSSS  EEEEE                      %
%                                                                             %
%                                                                             %
%                        MagickCore Image Raise Method                        %
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
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R a i s e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RaiseImage() creates a simulated three-dimensional button-like effect
%  by lightening and darkening the edges of the image.  Members width and
%  height of raise_info define the width of the vertical and horizontal
%  edge of the effect.
%
%  The format of the RaiseImage method is:
%
%      MagickBooleanType RaiseImage(const Image *image,
%        const RectangleInfo *raise_info,const MagickBooleanType raise,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o raise_info: Define the width and height of the raise area.
%
%    o raise: A value other than zero creates a 3-D raise effect,
%      otherwise it has a lowered effect.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void BlendPixelChannels(const Image *image,const Quantum factor,
  const Quantum color,Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    q[i]=ClampToQuantum(QuantumScale*((double) q[i]*(double) factor+
      (double) color*((double) QuantumRange-(double) factor)));
  }
}

#define AccentuateFactor  ScaleCharToQuantum(135)
#define HighlightFactor  ScaleCharToQuantum(190)
#define ShadowFactor  ScaleCharToQuantum(190)
#define RaiseImageTag  "Raise/Image"
#define TroughFactor  ScaleCharToQuantum(135)

typedef struct _RaiseSide
{
  Image
    *image;

  const RectangleInfo
    *raise_info;

  Quantum
    foreground,
    background;
} RaiseSide;

static void DrawRaiseSideRow(const RaiseSide *raise_side,
  Quantum *magick_restrict q)
{
  Image
    *const image = raise_side->image;

  const RectangleInfo
    *const raise_info = raise_side->raise_info;

  const Quantum
    foreground = raise_side->foreground,
    background = raise_side->background;

  ssize_t
    x;

  for (x=0; x < (ssize_t) raise_info->width; x++)
  {
    BlendPixelChannels(image,HighlightFactor,foreground,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  for ( ; x < (ssize_t) (image->columns-raise_info->width); x++)
    q+=(ptrdiff_t) GetPixelChannels(image);
  for ( ; x < (ssize_t) image->columns; x++)
  {
    BlendPixelChannels(image,ShadowFactor,background,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

MagickExport MagickBooleanType RaiseImage(Image *image,
  const RectangleInfo *raise_info,const MagickBooleanType raise,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  Quantum
    foreground,
    background;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(raise_info != (RectangleInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((image->columns <= (raise_info->width << 1)) ||
      (image->rows <= (raise_info->height << 1)))
    ThrowBinaryException(OptionError,"ImageSizeMustExceedBevelWidth",
      image->filename);
  foreground=QuantumRange;
  background=(Quantum) 0;
  if (raise == MagickFalse)
    {
      foreground=(Quantum) 0;
      background=QuantumRange;
    }
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  /*
    Raise image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,raise_info->height,1)
#endif
  for (y=0; y < (ssize_t) raise_info->height; y++)
  {
    ssize_t
      x;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < y; x++)
    {
      BlendPixelChannels(image,HighlightFactor,foreground,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    for ( ; x < ((ssize_t) image->columns-y); x++)
    {
      BlendPixelChannels(image,AccentuateFactor,foreground,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    for ( ; x < (ssize_t) image->columns; x++)
    {
      BlendPixelChannels(image,ShadowFactor,background,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,RaiseImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows-2*raise_info->height,1)
#endif
  for (y=(ssize_t) raise_info->height; y < (ssize_t) (image->rows-raise_info->height); y++)
  {
    Quantum
      *magick_restrict q;

    RaiseSide
      raise_side;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    raise_side.image=image;
    raise_side.raise_info=raise_info;
    raise_side.foreground=foreground;
    raise_side.background=background;
    DrawRaiseSideRow(&raise_side,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,RaiseImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows-raise_info->height,1)
#endif
  for (y=(ssize_t) (image->rows-raise_info->height); y < (ssize_t) image->rows; y++)
  {
    ssize_t
      x;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < ((ssize_t) image->rows-y); x++)
    {
      BlendPixelChannels(image,HighlightFactor,foreground,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    for ( ; x < ((ssize_t) image->columns-((ssize_t) image->rows-y)); x++)
    {
      BlendPixelChannels(image,TroughFactor,background,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    for ( ; x < (ssize_t) image->columns; x++)
    {
      BlendPixelChannels(image,ShadowFactor,background,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,RaiseImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}
