/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                QQQ   U   U   AAA   N   N  TTTTT  U   U  M   M               %
%               Q   Q  U   U  A   A  NN  N    T    U   U  MM MM               %
%               Q   Q  U   U  AAAAA  N N N    T    U   U  M M M               %
%               Q  QQ  U   U  A   A  N  NN    T    U   U  M   M               %
%                QQQQ   UUU   A   A  N   N    T     UUU   M   M               %
%                                                                             %
%                         TTTTT  Y   Y  PPPP   EEEEE                          %
%                           T     Y Y   P   P  E                              %
%                           T      Y    PPPP   EEE                            %
%                           T      Y    P      E                              %
%                           T      Y    P      EEEEE                          %
%                                                                             %
%                           Quantum Type and Extent                           %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                               October 1998                                  %
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
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/color-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/geometry.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/statistic.h"
#include "MagickCore/stream.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/utility.h"

typedef struct _QuantumChannels
{
  QuantumType
    key;

  size_t
    value;
} QuantumChannels;

static const QuantumChannels
  QuantumChannelCounts[] =
  {
    { GrayAlphaQuantum, 2 },
    { IndexAlphaQuantum, 2 },
    { RGBQuantum, 3 },
    { BGRQuantum, 3 },
    { RGBAQuantum, 4 },
    { RGBOQuantum, 4 },
    { BGRAQuantum, 4 },
    { CMYKQuantum, 4 },
    { CMYKAQuantum, 5 },
    { CbYCrAQuantum, 4 },
    { CbYCrQuantum, 3 },
    { CbYCrYQuantum, 4 }
  };

static const QuantumChannels *FindQuantumChannels(const QuantumType key)
{
  size_t
    i;

  for (i=0; i < (sizeof(QuantumChannelCounts)/sizeof(*QuantumChannelCounts));
       i++)
    if (QuantumChannelCounts[i].key == key)
      return(QuantumChannelCounts+i);
  return((const QuantumChannels *) NULL);
}

static size_t GetQuantumChannels(const Image *image,
  const QuantumType quantum_type)
{
  const QuantumChannels
    *entry;

  size_t
    channels;

  channels=1;
  entry=FindQuantumChannels(quantum_type);
  if (entry != (const QuantumChannels *) NULL)
    channels=entry->value;
  else
    switch (quantum_type)
    {
      case MultispectralQuantum: channels=GetImageChannels(image); break;
      default: break;
    }
  return(channels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t Q u a n t u m E x t e n t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetQuantumExtent() returns the quantum pixel buffer extent.
%
%  The format of the GetQuantumExtent method is:
%
%      size_t GetQuantumExtent(Image *image,const QuantumInfo *quantum_info,
%        const QuantumType quantum_type)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o quantum_info: the quantum info.
%
%    o quantum_type: Declare which pixel components to transfer (red, green,
%      blue, opacity, RGB, or RGBA).
%
*/
MagickExport size_t GetQuantumExtent(const Image *image,
  const QuantumInfo *quantum_info,const QuantumType quantum_type)
{
  size_t
    channels;

  assert(quantum_info != (QuantumInfo *) NULL);
  assert(quantum_info->signature == MagickCoreSignature);
  channels=GetQuantumChannels(image,quantum_type);
  if (quantum_info->pack == MagickFalse)
    return((size_t) (channels*image->columns*((quantum_info->depth+7)/8))+
      (quantum_info->pad*image->columns));
  return((size_t) ((channels*image->columns*quantum_info->depth+7)/8)+
    (quantum_info->pad*image->columns));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t Q u a n t u m T y p e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetQuantumType() returns the quantum type of the image.
%
%  The format of the GetQuantumType method is:
%
%      QuantumType GetQuantumType(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _AlphaQuantumTypes
{
  QuantumType
    opaque_type,
    alpha_type;
} AlphaQuantumTypes;

static QuantumType SelectAlphaQuantumType(const Image *image,
  const AlphaQuantumTypes *types)
{
  const QuantumType
    alpha_type = types->alpha_type,
    opaque_type = types->opaque_type;

  QuantumType
    quantum_type;

  quantum_type=opaque_type;
  if (image->alpha_trait != UndefinedPixelTrait)
    quantum_type=alpha_type;
  return(quantum_type);
}

MagickExport QuantumType GetQuantumType(Image *image,ExceptionInfo *exception)
{
  const AlphaQuantumTypes
    rgb_types = { RGBQuantum, RGBAQuantum };

  QuantumType
    quantum_type;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  (void) exception;
  quantum_type=SelectAlphaQuantumType(image,&rgb_types);
  if (image->colorspace == CMYKColorspace)
    {
      const AlphaQuantumTypes
        cmyk_types = { CMYKQuantum, CMYKAQuantum };

      quantum_type=SelectAlphaQuantumType(image,&cmyk_types);
    }
  if (IsGrayColorspace(image->colorspace) != MagickFalse)
    {
      const AlphaQuantumTypes
        gray_types = { GrayQuantum, GrayAlphaQuantum };

      quantum_type=SelectAlphaQuantumType(image,&gray_types);
    }
  if (image->storage_class == PseudoClass)
    {
      const AlphaQuantumTypes
        index_types = { IndexQuantum, IndexAlphaQuantum };

      quantum_type=SelectAlphaQuantumType(image,&index_types);
    }
  if (image->number_meta_channels != 0)
    quantum_type=MultispectralQuantum;
  return(quantum_type);
}

static MagickBooleanType IsIndexQuantum(const QuantumType quantum_type)
{
  switch (quantum_type)
  {
    case IndexQuantum:
    case IndexAlphaQuantum:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static const QuantumType
  ColorSeparationQuantums[] =
  {
    CyanQuantum,
    MagentaQuantum,
    YellowQuantum,
    BlackQuantum,
    CMYKQuantum,
    CMYKAQuantum,
    MultispectralQuantum
  };

static MagickBooleanType IsColorSeparationQuantum(
  const QuantumType quantum_type)
{
  size_t
    i;

  for (i=0; i < (sizeof(ColorSeparationQuantums)/sizeof(*ColorSeparationQuantums)); i++)
    if (ColorSeparationQuantums[i] == quantum_type)
      return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t Q u a n t u m I m a g e T y p e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetQuantumImageType() sets the image type based on the quantum type.
%
%  The format of the SetQuantumImageType method is:
%
%      void ImageType SetQuantumImageType(Image *image,
%        const QuantumType quantum_type)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o quantum_type: Declare which pixel components to transfer (red, green,
%      blue, opacity, RGB, or RGBA).
%
*/
MagickExport void SetQuantumImageType(Image *image,
  const QuantumType quantum_type)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (IsIndexQuantum(quantum_type) != MagickFalse)
    {
      image->type=PaletteType;
      return;
    }
  if (IsColorSeparationQuantum(quantum_type) != MagickFalse)
    {
      image->type=ColorSeparationType;
      return;
    }
  switch (quantum_type)
  {
    case GrayQuantum:
    case GrayAlphaQuantum:
    {
      image->type=GrayscaleType;
      if (image->depth == 1)
        image->type=BilevelType;
      break;
    }
    default:
    {
      image->type=TrueColorType;
      break;
    }
  }
}
