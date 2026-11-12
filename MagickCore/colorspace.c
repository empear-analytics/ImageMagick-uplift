/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     CCCC   OOO   L       OOO   RRRR   SSSSS  PPPP    AAA    CCCC  EEEEE     %
%    C      O   O  L      O   O  R   R  SS     P   P  A   A  C      E         %
%    C      O   O  L      O   O  RRRR    SSS   PPPP   AAAAA  C      EEE       %
%    C      O   O  L      O   O  R R       SS  P      A   A  C      E         %
%     CCCC   OOO   LLLLL   OOO   R  R   SSSSS  P      A   A   CCCC  EEEEE     %
%                                                                             %
%                                                                             %
%                     MagickCore Image Colorspace Methods                     %
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
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/property.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/utility.h"


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e C o l o r s p a c e T y p e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageColorspaceType() returns the potential type of image:
%  sRGBColorspaceType, RGBColorspaceType, GRAYColorspaceType, etc.
%
%  To ensure the image type matches its potential, use SetImageColorspaceType():
%
%    (void) SetImageColorspaceType(image,GetImageColorspaceType(image),
%      exception);
%
%  The format of the GetImageColorspaceType method is:
%
%      ColorspaceType GetImageColorspaceType(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static ColorspaceType IdentifyColorspaceType(const Image *image,
  ExceptionInfo *exception)
{
  ColorspaceType
    colorspace;

  ImageType
    type;

  colorspace=image->colorspace;
  type=IdentifyImageType(image,exception);
  if (IsGrayImageType(type))
    colorspace=GRAYColorspace;
  return(colorspace);
}

MagickExport ColorspaceType GetImageColorspaceType(const Image *image,
  ExceptionInfo *exception)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  return(IdentifyColorspaceType(image,exception));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S e t I m a g e G r a y                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageGray() returns MagickTrue if all the pixels in the image have the
%  same red, green, and blue intensities and changes the type of the image to
%  bi-level or grayscale.
%
%  The format of the SetImageGray method is:
%
%      MagickBooleanType SetImageGray(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType SetGrayImageType(Image *image,const ImageType type,
  ExceptionInfo *exception)
{
  image->colorspace=GRAYColorspace;
  if (SyncImagePixelCache(image,exception) == MagickFalse)
    return(MagickFalse);
  image->type=type;
  return(MagickTrue);
}

static MagickBooleanType SetIdentifiedGrayType(Image *image,
  ExceptionInfo *exception)
{
  ImageType
    type;

  type=IdentifyImageGray(image,exception);
  if (type == UndefinedType)
    return(MagickFalse);
  return(SetGrayImageType(image,type,exception));
}

MagickExport MagickBooleanType SetImageGray(Image *image,
  ExceptionInfo *exception)
{
  const char
    *value;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (IsImageGray(image) != MagickFalse)
    return(MagickTrue);
  if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
    return(MagickFalse);
  value=GetImageProperty(image,"colorspace:auto-grayscale",exception);
  if (IsStringFalse(value) != MagickFalse)
    return(MagickFalse);
  return(SetIdentifiedGrayType(image,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e M o n o c h r o m e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageMonochrome() returns MagickTrue if all the pixels in the image have
%  the same red, green, and blue intensities and the intensity is either
%  0 or QuantumRange and changes the type of the image to bi-level.
%
%  The format of the SetImageMonochrome method is:
%
%      MagickBooleanType SetImageMonochrome(Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SetImageMonochrome(Image *image,
  ExceptionInfo *exception)
{
  MagickBooleanType
    is_bilevel;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (IsImageMonochrome(image) != MagickFalse)
    return(MagickTrue);
  if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
    return(MagickFalse);
  is_bilevel=IdentifyImageMonochrome(image,exception);
  if (is_bilevel == MagickFalse)
    return(MagickFalse);
  return(SetGrayImageType(image,BilevelType,exception));
}

