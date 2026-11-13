/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                       CCCC   OOO   L       OOO   RRRR                       %
%                      C      O   O  L      O   O  R   R                      %
%                      C      O   O  L      O   O  RRRR                       %
%                      C      O   O  L      O   O  R R                        %
%                       CCCC   OOO   LLLLL   OOO   R  R                       %
%                                                                             %
%                      TTTTT  U   U  PPPP   L      EEEEE                      %
%                        T    U   U  P   P  L      E                          %
%                        T    U   U  PPPP   L      EEE                        %
%                        T    U   U  P      L      E                          %
%                        T     UUU   P      LLLLL  EEEEE                      %
%                                                                             %
%                                                                             %
%                           Color Tuple Formatting                            %
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
%  We use linked-lists because splay-trees do not currently support duplicate
%  key / value pairs (.e.g X11 green compliance and SVG green compliance).
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/cache.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/client.h"
#include "MagickCore/configure.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/linked-list-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C o n c a t e n a t e C o l o r C o m p o n e n t                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConcatenateColorComponent() returns the pixel as a canonical string.
%
%  The format of the ConcatenateColorComponent() method is:
%
%      void ConcatenateColorComponent(const PixelInfo *pixel,
%        const PixelChannel channel,const ComplianceType compliance,
%        char *tuple)
%
%  A description of each parameter follows.
%
%    o pixel:  The pixel.
%
%    o channel:  The pixel channel.
%
%    o compliance: Adhere to this color standard: SVG, X11, or XPM.
%
%    o tuple:  The color tuple.
%
*/

static inline MagickBooleanType IsSVGCompliant(const PixelInfo *pixel)
{
#define SVGCompliant(component) ((double) \
   ScaleCharToQuantum(ScaleQuantumToChar(ClampToQuantum(component))))
#define SVGEpsilon  1.0e-6

  /*
    SVG requires color depths > 8 expressed as percentages.
  */
  if (fabs((double) (SVGCompliant(pixel->red)-pixel->red)) >= SVGEpsilon)
    return(MagickFalse);
  if (fabs((double) (SVGCompliant(pixel->green)-pixel->green)) >= SVGEpsilon)
    return(MagickFalse);
  if (fabs((double) (SVGCompliant(pixel->blue)-pixel->blue)) >= SVGEpsilon)
    return(MagickFalse);
  if ((pixel->colorspace == CMYKColorspace) &&
      (fabs((double) (SVGCompliant(pixel->black)-pixel->black)) >= SVGEpsilon))
    return(MagickFalse);
  return(MagickTrue);
}

static inline MagickBooleanType IsPercentColorComponent(
  const ComplianceType compliance,const PixelInfo *pixel)
{
  return((MagickBooleanType) ((compliance != NoCompliance) &&
    (IssRGBCompatibleColorspace(pixel->colorspace) != MagickFalse) &&
    (IsSVGCompliant(pixel) == MagickFalse)));
}

static inline MagickBooleanType IsLCHColorspace(
  const ColorspaceType colorspace)
{
  return((MagickBooleanType) ((colorspace == LCHColorspace) ||
    (colorspace == LCHabColorspace) || (colorspace == LCHuvColorspace)));
}

static void SetRedComponent(const PixelInfo *pixel,
  const ComplianceType compliance,double *color,double *scale)
{
  *color=pixel->red;
  if (IsHueCompatibleColorspace(pixel->colorspace) != MagickFalse)
    *scale=360.0;
  if ((compliance != NoCompliance) &&
      (IsLabCompatibleColorspace(pixel->colorspace) != MagickFalse))
    *scale=100.0;
  if (pixel->colorspace == XYZColorspace)
    (*color)/=2.55;
  if (pixel->colorspace == OklchColorspace)
    *scale=1.0;
}

static void FormatScaledComponent(const PixelInfo *pixel,const double scale,
  const double color,char *component)
{
  if ((scale != 100.0) ||
      (IsLabCompatibleColorspace(pixel->colorspace) != MagickFalse))
    (void) FormatLocaleString(component,MagickPathExtent,"%.*g",
      GetMagickPrecision(),scale*QuantumScale*color);
  else
    (void) FormatLocaleString(component,MagickPathExtent,"%.*g%%",
      GetMagickPrecision(),scale*QuantumScale*color);
}

static void SelectIndexComponent(const PixelInfo *pixel,
  const PixelChannel channel,double *color)
{
  switch (channel)
  {
    case BlackPixelChannel:
    {
      *color=pixel->black;
      break;
    }
    case IndexPixelChannel:
    {
      *color=pixel->index;
      break;
    }
    default:
      break;
  }
}

static void SelectBlueComponent(const PixelInfo *pixel,
  const PixelChannel channel,double *color,double *scale)
{
  switch (channel)
  {
    case BluePixelChannel:
    {
      *color=pixel->blue;
      if (IsHueCompatibleColorspace(pixel->colorspace) != MagickFalse)
        *scale=100.0f;
      if (pixel->colorspace == LabColorspace)
        (*color)-=QuantumRange/2.0;
      if (IsLCHColorspace(pixel->colorspace) != MagickFalse)
        (*color)*=360.0/255.0;
      if (pixel->colorspace == XYZColorspace)
        (*color)/=2.55;
      if (pixel->colorspace == OklchColorspace)
        *scale=1.0;
      break;
    }
    default:
    {
      SelectIndexComponent(pixel,channel,color);
      break;
    }
  }
}

static double GetColorComponentScale(const PixelInfo *pixel,
  const ComplianceType compliance)
{
  double
    scale;

  scale=QuantumRange;
  if ((compliance != NoCompliance) || (pixel->depth <= 8))
    scale=255.0;
  if (IsPercentColorComponent(compliance,pixel) != MagickFalse)
    scale=100.0;
  return(scale);
}

static void AdjustGreenComponent(const PixelInfo *pixel,
  const ComplianceType compliance,double *color,double *scale)
{
  if (IsHueCompatibleColorspace(pixel->colorspace) != MagickFalse)
    *scale=100.0;
  if ((compliance != NoCompliance) &&
      (IsLabCompatibleColorspace(pixel->colorspace) != MagickFalse))
    (*color)-=QuantumRange/2.0;
}

static void SetAlphaComponent(const PixelInfo *pixel,
  const ComplianceType compliance,double *color,double *scale)
{
  *color=pixel->alpha;
  if (compliance != NoCompliance)
    *scale=1.0;
}

static inline MagickBooleanType IsOklchHueChannel(const PixelInfo *pixel,
  const PixelChannel channel)
{
  return((MagickBooleanType) ((pixel->colorspace == OklchColorspace) &&
    (channel == BluePixelChannel)));
}

MagickExport void ConcatenateColorComponent(const PixelInfo *pixel,
  const PixelChannel channel,const ComplianceType compliance,char *tuple)
{
#define IsColorComponentFactional(color) \
  ((color)-ScaleCharToQuantum(ScaleQuantumToChar(color)))

  char
    component[MagickPathExtent];

  double
    color,
    scale;

  color=0.0;
  scale=GetColorComponentScale(pixel,compliance);
  switch (channel)
  {
    case RedPixelChannel:
    {
      SetRedComponent(pixel,compliance,&color,&scale);
      break;
    }
    case GreenPixelChannel:
    {
      color=pixel->green;
      AdjustGreenComponent(pixel,compliance,&color,&scale);
      if (pixel->colorspace == XYZColorspace)
        color/=2.55;
      if (pixel->colorspace == OklchColorspace)
        scale=1.0;
      break;
    }
    case AlphaPixelChannel:
    {
      SetAlphaComponent(pixel,compliance,&color,&scale);
      break;
    }
    default:
    {
      SelectBlueComponent(pixel,channel,&color,&scale);
      break;
    }
  }
  if (IsOklchHueChannel(pixel,channel) != MagickFalse)
    (void) FormatLocaleString(component,MagickPathExtent,"%.*g",
      GetMagickPrecision(),RadiansToDegrees(scale*QuantumScale*color));
  else
    FormatScaledComponent(pixel,scale,color,component);
  (void) ConcatenateMagickString(tuple,component,MagickPathExtent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t C o l o r T u p l e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetColorTuple() returns a color as a color tuple string (e.g. rgba(255,0,0))
%  or hex string (e.g. #FF0000).
%
%  The format of the GetColorTuple method is:
%
%      GetColorTuple(const PixelInfo *pixel,const MagickBooleanType hex,
%        char *tuple)
%
%  A description of each parameter follows.
%
%    o pixel: the pixel.
%
%    o hex: A value other than zero returns the tuple in a hexadecimal format.
%
%    o tuple: Return the color tuple as this string.
%
*/

static double GetHexChannelValue(const PixelInfo *pixel,
  const PixelChannel channel)
{
  double
    color;

  color=0.0;
  switch (channel)
  {
    case RedPixelChannel:
    {
      color=pixel->red;
      break;
    }
    case GreenPixelChannel:
    {
      color=pixel->green;
      break;
    }
    case BluePixelChannel:
    {
      color=pixel->blue;
      break;
    }
    case AlphaPixelChannel:
    {
      color=pixel->alpha;
      break;
    }
    case BlackPixelChannel:
    {
      color=pixel->black;
      break;
    }
    default:
      break;
  }
  return(color);
}

static void ConcatenateHexColorComponent(const PixelInfo *pixel,
  const PixelChannel channel,char *tuple)
{
  char
    component[MagickPathExtent];

  double
    color;

  color=GetHexChannelValue(pixel,channel);
  if (pixel->depth > 32)
    {
      (void) FormatLocaleString(component,MagickPathExtent,"%08lX%08lX",
        (unsigned long) ScaleQuantumToLong(ClampToQuantum(color)),
        (unsigned long) ScaleQuantumToLong(ClampToQuantum(color)));
      (void) ConcatenateMagickString(tuple,component,MagickPathExtent);
      return;
    }
  if (pixel->depth > 16)
    {
      (void) FormatLocaleString(component,MagickPathExtent,"%08X",
        (unsigned int) ScaleQuantumToLong(ClampToQuantum(color)));
      (void) ConcatenateMagickString(tuple,component,MagickPathExtent);
      return;
    }
  if (pixel->depth > 8)
    {
      (void) FormatLocaleString(component,MagickPathExtent,"%04X",
        ScaleQuantumToShort(ClampToQuantum(color)));
      (void) ConcatenateMagickString(tuple,component,MagickPathExtent);
      return;
    }
  (void) FormatLocaleString(component,MagickPathExtent,"%02X",
    ScaleQuantumToChar(ClampToQuantum(color)));
  (void) ConcatenateMagickString(tuple,component,MagickPathExtent);
  return;
}

static void ConcatenateHexColor(const PixelInfo *pixel,char *tuple)
{
  /*
    Convert pixel to hex color.
  */
  (void) ConcatenateMagickString(tuple,"#",MagickPathExtent);
  ConcatenateHexColorComponent(pixel,RedPixelChannel,tuple);
  ConcatenateHexColorComponent(pixel,GreenPixelChannel,tuple);
  ConcatenateHexColorComponent(pixel,BluePixelChannel,tuple);
  if (pixel->colorspace == CMYKColorspace)
    ConcatenateHexColorComponent(pixel,BlackPixelChannel,tuple);
  if (pixel->alpha_trait != UndefinedPixelTrait)
    ConcatenateHexColorComponent(pixel,AlphaPixelChannel,tuple);
}

static void ConcatenateColorComponents(const PixelInfo *color,char *tuple)
{
  if ((color->colorspace == LinearGRAYColorspace) ||
      (color->colorspace == GRAYColorspace))
    ConcatenateColorComponent(color,GrayPixelChannel,SVGCompliance,tuple);
  else
    {
      ConcatenateColorComponent(color,RedPixelChannel,SVGCompliance,tuple);
      (void) ConcatenateMagickString(tuple,",",MagickPathExtent);
      ConcatenateColorComponent(color,GreenPixelChannel,SVGCompliance,tuple);
      (void) ConcatenateMagickString(tuple,",",MagickPathExtent);
      ConcatenateColorComponent(color,BluePixelChannel,SVGCompliance,tuple);
    }
  if (color->colorspace == CMYKColorspace)
    {
      (void) ConcatenateMagickString(tuple,",",MagickPathExtent);
      ConcatenateColorComponent(color,BlackPixelChannel,SVGCompliance,tuple);
    }
  if (color->alpha_trait != UndefinedPixelTrait)
    {
      (void) ConcatenateMagickString(tuple,",",MagickPathExtent);
      ConcatenateColorComponent(color,AlphaPixelChannel,SVGCompliance,tuple);
    }
}

MagickExport void GetColorTuple(const PixelInfo *pixel,
  const MagickBooleanType hex,char *tuple)
{
  PixelInfo
    color;

  assert(pixel != (const PixelInfo *) NULL);
  assert(tuple != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",tuple);
  *tuple='\0';
  if (hex != MagickFalse)
    {
      ConcatenateHexColor(pixel,tuple);
      return;
    }
  /*
    Convert pixel to rgb() or cmyk() color.
  */
  color=(*pixel);
  if ((color.depth > 8) && (IsSVGCompliant(pixel) != MagickFalse))
    color.depth=8;
  (void) ConcatenateMagickString(tuple,CommandOptionToMnemonic(
    MagickColorspaceOptions,(ssize_t) color.colorspace),MagickPathExtent);
  if (color.alpha_trait != UndefinedPixelTrait)
    (void) ConcatenateMagickString(tuple,"a",MagickPathExtent);
  (void) ConcatenateMagickString(tuple,"(",MagickPathExtent);
  ConcatenateColorComponents(&color,tuple);
  (void) ConcatenateMagickString(tuple,")",MagickPathExtent);
  LocaleLower(tuple);
  return;
}
