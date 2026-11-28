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

  MagickCore private colorspace predicates: IsRGBColorspace,
  IssRGBColorspace, IsRGBModelColorspace and IssRGBCompatibleColorspace;
  included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_RGB_PREDICATES_PRIVATE_H
#define MAGICKCORE_COLORSPACE_RGB_PREDICATES_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

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

static inline MagickBooleanType IsRGBModelColorspace(
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
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static inline MagickBooleanType IssRGBCompatibleColorspace(
  const ColorspaceType colorspace)
{
  if (IsRGBModelColorspace(colorspace) != MagickFalse)
    return(MagickTrue);
  switch (colorspace)
  {
    case TransparentColorspace:
    case GRAYColorspace:
    case LinearGRAYColorspace:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
