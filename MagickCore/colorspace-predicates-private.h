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

  MagickCore private colorspace predicates: IsCMYKColorspace,
  IsGrayColorspace, IsGrayImageType and IsHueCompatibleColorspace; included
  by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_PREDICATES_PRIVATE_H
#define MAGICKCORE_COLORSPACE_PREDICATES_PRIVATE_H

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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
