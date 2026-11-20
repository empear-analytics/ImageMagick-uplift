/*
  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.  You may
  obtain a copy of the License at

    https://imagemagick.org/license/

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore image colorspace private methods.
*/
#ifndef MAGICKCORE_COLORSPACE_PRIVATE_H
#define MAGICKCORE_COLORSPACE_PRIVATE_H

#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/colorspace-illuminant-xyz-private.h"
#include "MagickCore/colorspace-xyz-to-rgb-private.h"
#include "MagickCore/colorspace-hcl-to-rgb-private.h"
#include "MagickCore/colorspace-to-rgb-private.h"
#include "MagickCore/colorspace-lab-to-rgb-private.h"
#include "MagickCore/colorspace-luv-lms-to-rgb-private.h"
#include "MagickCore/colorspace-cie-to-rgb-private.h"
#include "MagickCore/colorspace-rgb-to-xyz-cmy-private.h"
#include "MagickCore/colorspace-rgb-to-xyz-private.h"
#include "MagickCore/colorspace-rgb-to-hue-private.h"
#include "MagickCore/colorspace-rgb-to-lab-private.h"
#include "MagickCore/colorspace-rgb-to-lchuv-private.h"
#include "MagickCore/colorspace-rgb-to-cie-private.h"
#include "MagickCore/colorspace-xyz-to-jzazbz-private.h"
#include "MagickCore/colorspace-rgb-to-jzazbz-private.h"
#include "MagickCore/colorspace-jzazbz-to-xyz-private.h"
#include "MagickCore/colorspace-jzazbz-private.h"
#include "MagickCore/colorspace-jzazbz-oklab-private.h"
#include "MagickCore/colorspace-rgb-to-ycc-private.h"
#include "MagickCore/colorspace-rgb-to-luma-private.h"
#include "MagickCore/colorspace-conversions-private.h"
#include "MagickCore/colorspace-predicates-private.h"
#include "MagickCore/colorspace-lab-compatible-private.h"
#include "MagickCore/colorspace-rgb-predicates-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline MagickBooleanType IsYCbCrCompatibleColorspace(
  const ColorspaceType colorspace)
{
  switch (colorspace)
  {
    case YCbCrColorspace:
    case Rec709YCbCrColorspace:
    case Rec601YCbCrColorspace:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

extern MagickPrivate void
  ConvertGenericToRGB(const ColorspaceType,const double,const double,
    const double,const double,const IlluminantType,double *,double *,double *),
  ConvertRGBToGeneric(const ColorspaceType,const double,const double,
    const double,const double,const IlluminantType,double *,double *,double *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
