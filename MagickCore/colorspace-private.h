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
#include "MagickCore/colorspace-to-rgb-private.h"
#include "MagickCore/colorspace-cie-to-rgb-private.h"
#include "MagickCore/colorspace-rgb-to-hue-private.h"
#include "MagickCore/colorspace-conversions-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline MagickBooleanType IsCMYKColorspace(
  const ColorspaceType colorspace)
{
  if (colorspace == CMYKColorspace)
    return(MagickTrue);
  return(MagickFalse);
}

static inline MagickBooleanType IsGrayColorspace(
  const ColorspaceType colorspace)
{
  if ((colorspace == LinearGRAYColorspace) || (colorspace == GRAYColorspace))
    return(MagickTrue);
  return(MagickFalse);
}

static inline MagickBooleanType IsGrayImageType(const ImageType type)
{
  switch (type)
  {
    case GrayscaleType:
    case GrayscaleAlphaType:
    case BilevelType:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static inline MagickBooleanType IsHueCompatibleColorspace(
  const ColorspaceType colorspace)
{
  if ((colorspace == HCLColorspace) || (colorspace == HCLpColorspace) ||
      (colorspace == HSBColorspace) || (colorspace == HSIColorspace) ||
      (colorspace == HSLColorspace) || (colorspace == HSVColorspace))
    return(MagickTrue);
  return(MagickFalse);
}

static inline MagickBooleanType IsLabCompatibleColorspace(
  const ColorspaceType colorspace)
{
  if ((colorspace == LabColorspace) || (colorspace == LCHColorspace) ||
      (colorspace == LCHabColorspace) || (colorspace == LCHuvColorspace) ||
      (colorspace == OklabColorspace))
    return(MagickTrue);
  return(MagickFalse);
}

static inline MagickBooleanType IsRGBColorspace(const ColorspaceType colorspace)
{
  switch (colorspace)
  {
    case RGBColorspace:
    case scRGBColorspace:
    case LinearGRAYColorspace:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static inline MagickBooleanType IssRGBColorspace(
  const ColorspaceType colorspace)
{
  if ((colorspace == sRGBColorspace) || (colorspace == TransparentColorspace))
    return(MagickTrue);
  return(MagickFalse);
}

static inline MagickBooleanType IssRGBCompatibleColorspace(
  const ColorspaceType colorspace)
{
  switch (colorspace)
  {
    case sRGBColorspace:
    case RGBColorspace:
    case Adobe98Colorspace:
    case ProPhotoColorspace:
    case DisplayP3Colorspace:
    case scRGBColorspace:
    case TransparentColorspace:
    case GRAYColorspace:
    case LinearGRAYColorspace:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

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
