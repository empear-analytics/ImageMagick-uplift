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
%        TTTTT  RRRR    AAA   N   N  SSSSS  FFFFF   OOO   RRRR   M   M        %
%          T    R   R  A   A  NN  N  SS     F      O   O  R   R  MM MM        %
%          T    RRRR   AAAAA  N N N   SSS   FFF    O   O  RRRR   M M M        %
%          T    R R    A   A  N  NN     SS  F      O   O  R R    M   M        %
%          T    R  R   A   A  N   N  SSSSS  F       OOO   R  R   M   M        %
%                                                                             %
%                                                                             %
%                MagickCore Image Colorspace Transform Methods                %
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
%   S e t I m a g e C o l o r s p a c e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageColorspace() sets the colorspace member of the Image structure.
%
%  The format of the SetImageColorspace method is:
%
%      MagickBooleanType SetImageColorspace(Image *image,
%        const ColorspaceType colorspace,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o colorspace: the colorspace.
%
%   o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsLinearGammaColorspace(
  const ColorspaceType colorspace)
{
  return(((IsRGBColorspace(colorspace) != MagickFalse) ||
    (colorspace == XYZColorspace) || (colorspace == xyYColorspace)) ?
    MagickTrue : MagickFalse);
}

static void SetPerceptualChromaticity(Image *image)
{
  image->rendering_intent=PerceptualIntent;
  image->chromaticity.red_primary.x=0.6400;
  image->chromaticity.red_primary.y=0.3300;
  image->chromaticity.red_primary.z=0.0300;
  image->chromaticity.green_primary.x=0.3000;
  image->chromaticity.green_primary.y=0.6000;
  image->chromaticity.green_primary.z=0.1000;
  image->chromaticity.blue_primary.x=0.1500;
  image->chromaticity.blue_primary.y=0.0600;
  image->chromaticity.blue_primary.z=0.7900;
  image->chromaticity.white_point.x=0.3127;
  image->chromaticity.white_point.y=0.3290;
  image->chromaticity.white_point.z=0.3583;
}

static ImageType SetGrayColorspaceGamma(Image *image,
  const ColorspaceType colorspace)
{
  ImageType
    type;

  if (colorspace == LinearGRAYColorspace)
    image->gamma=1.000;
  type=GrayscaleType;
  return(type);
}

static MagickBooleanType ApplyImageColorspace(Image *image,
  const ColorspaceType colorspace,ExceptionInfo *exception)
{
  ImageType
    type;

  MagickBooleanType
    status;

  image->colorspace=colorspace;
  image->rendering_intent=UndefinedIntent;
  image->gamma=1.000/2.200;
  (void) memset(&image->chromaticity,0,sizeof(image->chromaticity));
  type=image->type;
  if (IsGrayColorspace(colorspace) != MagickFalse)
    type=SetGrayColorspaceGamma(image,colorspace);
  else
    if (IsLinearGammaColorspace(colorspace) != MagickFalse)
      image->gamma=1.000;
    else
      SetPerceptualChromaticity(image);
  status=SyncImagePixelCache(image,exception);
  image->type=type;
  return(status);
}

MagickExport MagickBooleanType SetImageColorspace(Image *image,
  const ColorspaceType colorspace,ExceptionInfo *exception)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->colorspace == colorspace)
    return(MagickTrue);
  return(ApplyImageColorspace(image,colorspace,exception));
}

