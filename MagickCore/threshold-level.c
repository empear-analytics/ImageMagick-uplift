/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        TTTTT  H   H  RRRR   EEEEE  SSSSS  H   H   OOO   L      DDDD         %
%          T    H   H  R   R  E      SS     H   H  O   O  L      D   D        %
%          T    HHHHH  RRRR   EEE     SSS   HHHHH  O   O  L      D   D        %
%          T    H   H  R R    E         SS  H   H  O   O  L      D   D        %
%          T    H   H  R  R   EEEEE  SSSSS  H   H   OOO   LLLLL  DDDD         %
%                                                                             %
%                      L      EEEEE  V   V  EEEEE  L                          %
%                      L      E      V   V  E      L                          %
%                      L      EEE    V   V  EEE    L                          %
%                      L      E       V V   E      L                          %
%                      LLLLL  EEEEE    V    EEEEE  LLLLL                      %
%                                                                             %
%                                                                             %
%                  MagickCore Image Level Threshold Methods                   %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                 October 1996                                %
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
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/configure.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/effect.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/shear.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"
#include "MagickCore/threshold-private.h"
#include "MagickCore/threshold-level-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     B l a c k T h r e s h o l d I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BlackThresholdImage() is like ThresholdImage() but forces all pixels below
%  the threshold into black while leaving all pixels at or above the threshold
%  unchanged.
%
%  The format of the BlackThresholdImage method is:
%
%      MagickBooleanType BlackThresholdImage(Image *image,
%        const char *threshold,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o threshold: define the threshold value.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void BlackThresholdPixel(const Image *image,const PixelInfo *threshold,
  double pixel,Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    if (image->channel_mask != AllChannels)
      pixel=(double) q[i];
    if (pixel < GetPixelInfoChannel(threshold,channel))
      q[i]=(Quantum) 0;
  }
}

static void BlackThresholdIntensity(const Image *image,const void *arguments,
  Quantum *magick_restrict q)
{
  const PixelInfo
    *threshold = (const PixelInfo *) arguments;

  ThresholdIntensity(image,BlackThresholdPixel,threshold,q);
}

static void BlackThresholdRow(const Image *image,const void *arguments,
  Quantum *magick_restrict q)
{
  ThresholdPixels(image,BlackThresholdIntensity,arguments,q);
}

static MagickBooleanType SetDirectColorImage(Image *image,
  ExceptionInfo *exception)
{
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if (IsGrayColorspace(image->colorspace) != MagickFalse)
    (void) SetImageColorspace(image,sRGBColorspace,exception);
  return(MagickTrue);
}

MagickExport MagickBooleanType BlackThresholdImage(Image *image,
  const char *thresholds,ExceptionInfo *exception)
{
#define ThresholdImageTag  "Threshold/Image"

  PixelInfo
    threshold;

  const ThresholdRows
    rows = { ThresholdImageTag,BlackThresholdRow,&threshold };

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (thresholds == (const char *) NULL)
    return(MagickTrue);
  if (SetDirectColorImage(image,exception) == MagickFalse)
    return(MagickFalse);
  threshold=GetThresholdLevels(image,thresholds);
  /*
    White threshold image.
  */
  return(ThresholdImageRows(image,&rows,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     P e r c e p t i b l e I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PerceptibleImage() set each pixel whose value is less than |epsilon| to
%  epsilon or -epsilon (whichever is closer) otherwise the pixel value remains
%  unchanged.
%
%  The format of the PerceptibleImage method is:
%
%      MagickBooleanType PerceptibleImage(Image *image,const double epsilon,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o epsilon: the epsilon threshold (e.g. 1.0e-9).
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline Quantum PerceptibleThreshold(const Quantum quantum,
  const double epsilon)
{
  double
    sign;

  sign=(double) quantum < 0.0 ? -1.0 : 1.0;
  if ((sign*(double) quantum) >= epsilon)
    return(quantum);
  return((Quantum) (sign*epsilon));
}

static void PerceptiblePixel(const Image *image,const void *arguments,
  Quantum *magick_restrict q)
{
  const double
    epsilon = (*(const double *) arguments);

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) != 0)
      q[i]=PerceptibleThreshold(q[i],epsilon);
  }
}

static void PerceptibleColormapEntry(const Image *image,const double epsilon,
  PixelInfo *magick_restrict q)
{
  if ((GetPixelChannelTraits(image,RedPixelChannel) & UpdatePixelTrait) != 0)
    q->red=(MagickRealType) PerceptibleThreshold(ClampToQuantum(q->red),
      epsilon);
  if ((GetPixelChannelTraits(image,GreenPixelChannel) & UpdatePixelTrait) != 0)
    q->green=(MagickRealType) PerceptibleThreshold(ClampToQuantum(q->green),
      epsilon);
  if ((GetPixelChannelTraits(image,BluePixelChannel) & UpdatePixelTrait) != 0)
    q->blue=(MagickRealType) PerceptibleThreshold(ClampToQuantum(q->blue),
      epsilon);
  if ((GetPixelChannelTraits(image,AlphaPixelChannel) & UpdatePixelTrait) != 0)
    q->alpha=(MagickRealType) PerceptibleThreshold(ClampToQuantum(q->alpha),
      epsilon);
}

static void PerceptibleRow(const Image *image,const void *arguments,
  Quantum *magick_restrict q)
{
  ThresholdPixels(image,PerceptiblePixel,arguments,q);
}

static MagickBooleanType PerceptibleImageColormap(Image *image,
  const double epsilon,ExceptionInfo *exception)
{
  ssize_t
    i;

  PixelInfo
    *magick_restrict q;

  q=image->colormap;
  for (i=0; i < (ssize_t) image->colors; i++)
  {
    PerceptibleColormapEntry(image,epsilon,q);
    q++;
  }
  return(SyncImage(image,exception));
}

MagickExport MagickBooleanType PerceptibleImage(Image *image,
  const double epsilon,ExceptionInfo *exception)
{
#define PerceptibleImageTag  "Perceptible/Image"

  const ThresholdRows
    rows = { PerceptibleImageTag,PerceptibleRow,&epsilon };

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->storage_class == PseudoClass)
    return(PerceptibleImageColormap(image,epsilon,exception));
  /*
    Perceptible image.
  */
  return(ThresholdImageRows(image,&rows,exception));
}

