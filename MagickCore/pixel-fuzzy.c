/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      PPPP   IIIII  X   X  EEEEE  L                          %
%                      P   P    I     X X   E      L                          %
%                      PPPP     I      X    EEE    L                          %
%                      P        I     X X   E      L                          %
%                      P      IIIII  X   X  EEEEE  LLLLL                      %
%                                                                             %
%                      FFFFF  U   U  ZZZZZ  ZZZZZ  Y   Y                      %
%                      F      U   U     ZZ     ZZ   Y Y                       %
%                      FFF    U   U   ZZZ    ZZZ     Y                        %
%                      F      U   U  ZZ     ZZ       Y                        %
%                      F       UUU   ZZZZZ  ZZZZZ    Y                        %
%                                                                             %
%                           Fuzzy Pixel Equivalence                           %
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
#include "MagickCore/property.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/pixel-private.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/statistic.h"
#include "MagickCore/stream.h"
#include "MagickCore/string_.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I s F u z z y E q u i v a l e n c e P i x e l                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsFuzzyEquivalencePixel() returns MagickTrue if the distance between two
%  pixels is less than the specified distance in a linear three (or four)
%  dimensional color space.
%
%  The format of the IsFuzzyEquivalencePixel method is:
%
%      void IsFuzzyEquivalencePixel(const Image *image,const Quantum *p,
%        const Image *target_image,const Quantum *q)
%
%  A description of each parameter follows:
%
%    o image: the source image.
%
%    o p: Pixel p.
%
%    o target: the target image.
%
%    o q: Pixel q.
%
*/
MagickExport MagickBooleanType IsFuzzyEquivalencePixel(const Image *image,
  const Quantum *p,const Image *target_image,const Quantum *q)
{
  double
    alpha = QuantumScale*(double) GetPixelAlpha(image,p),
    fuzz = GetFuzzyColorDistance(image,target_image),
    target_alpha = QuantumScale*(double) GetPixelAlpha(target_image,q);

  ssize_t
    i;

  /*
    MSE metric for comparing two pixels.
  */
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  { 
    double
      error;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait target_traits = GetPixelChannelTraits(target_image,channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((target_traits & UpdatePixelTrait) == 0))
      continue;
    if (channel == AlphaPixelChannel)
      error=(double) p[i]-(double) GetPixelChannel(target_image,channel,q);
    else
      error=alpha*(double) p[i]-target_alpha*
        GetPixelChannel(target_image,channel,q);
    if (MagickSafeSignificantError(error*error,fuzz) != MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I s F u z z y E q u i v a l e n c e P i x e l I n f o                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsFuzzyEquivalencePixelInfo() returns true if the distance between two
%  colors is less than the specified distance in a linear three (or four)
%  dimensional color space.
%
%  This implements the equivalent of:
%    fuzz < sqrt(color_distance^2 * u.a*v.a  + alpha_distance^2)
%
%  Which produces a multi-dimensional cone for that colorspace along the
%  transparency vector.
%
%  For example for an RGB:
%    color_distance^2  = ( (u.r-v.r)^2 + (u.g-v.g)^2 + (u.b-v.b)^2 ) / 3
%
%  See https://imagemagick.org/Usage/bugs/fuzz_distance/
%
%  Hue colorspace distances need more work.  Hue is not a distance, it is an
%  angle!
%
%  A check that q is in the same color space as p should be made and the
%  appropriate mapping made.  -- Anthony Thyssen  8 December 2010
%
%  The format of the IsFuzzyEquivalencePixelInfo method is:
%
%      MagickBooleanType IsFuzzyEquivalencePixelInfo(const PixelInfo *p,
%        const PixelInfo *q)
%
%  A description of each parameter follows:
%
%    o p: Pixel p.
%
%    o q: Pixel q.
%
*/
static double GetFuzzyHueDistance(const PixelInfo *p,double pixel)
{
  if (IsHueCompatibleColorspace(p->colorspace) != MagickFalse)
    {
      /*
        This calculates a arc distance for hue-- it should be a vector
        angle of 'S'/'W' length with 'L'/'B' forming appropriate cones.
        In other words this is a hack - Anthony.
      */
      if (fabs((double) pixel) > ((double) QuantumRange/2.0))
        pixel-=(double) QuantumRange;
      pixel*=2.0;
    }
  return(pixel);
}

static double GetFuzzyAlpha(const PixelInfo *p)
{
  if (p->alpha_trait != UndefinedPixelTrait)
    return(p->alpha);
  return((double) OpaqueAlpha);
}

static double GetFuzzyAlphaScale(const PixelInfo *p)
{
  if (p->alpha_trait != UndefinedPixelTrait)
    return(QuantumScale*p->alpha);
  return(1.0);
}

MagickExport MagickBooleanType IsFuzzyEquivalencePixelInfo(const PixelInfo *p,
  const PixelInfo *q)
{
  double
    distance,
    fuzz,
    pixel,
    scale;

  fuzz=p->fuzz*p->fuzz+q->fuzz*q->fuzz;
  scale=1.0;
  distance=0.0;
  if ((p->alpha_trait != UndefinedPixelTrait) ||
      (q->alpha_trait != UndefinedPixelTrait))
    {
      /*
        Transparencies are involved - set alpha distance.
      */
      pixel=GetFuzzyAlpha(p)-GetFuzzyAlpha(q);
      distance=pixel*pixel;
      if (MagickSafeSignificantError(distance,fuzz) != MagickFalse)
        return(MagickFalse);
      /*
        Generate a alpha scaling factor to generate a 4D cone on colorspace.
        If one color is transparent, distance has no color component.
      */
      scale=GetFuzzyAlphaScale(p);
      scale*=GetFuzzyAlphaScale(q);
      if (scale <= MagickEpsilon)
        return(MagickTrue);
    }
  /*
    CMYK create a CMY cube with a multi-dimensional cone toward black.
  */
  if (p->colorspace == CMYKColorspace)
    {
      pixel=p->black-q->black;
      distance+=pixel*pixel*scale;
      if (MagickSafeSignificantError(distance,fuzz) != MagickFalse)
        return(MagickFalse);
      scale*=QuantumScale*((double) QuantumRange-(double) p->black);
      scale*=QuantumScale*((double) QuantumRange-(double) q->black);
    }
  /*
    RGB or CMY color cube.
  */
  distance*=3.0;  /* rescale appropriately */
  fuzz*=3.0;
  pixel=p->red-q->red;
  pixel=GetFuzzyHueDistance(p,pixel);
  distance+=pixel*pixel*scale;
  if (MagickSafeSignificantError(distance,fuzz) != MagickFalse)
    return(MagickFalse);
  pixel=p->green-q->green;
  distance+=pixel*pixel*scale;
  if (MagickSafeSignificantError(distance,fuzz) != MagickFalse)
    return(MagickFalse);
  pixel=p->blue-q->blue;
  distance+=pixel*pixel*scale;
  if (MagickSafeSignificantError(distance,fuzz) != MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}
