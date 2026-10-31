/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               EEEEE  N   N  H   H   AAA   N   N   CCCC  EEEEE               %
%               E      NN  N  H   H  A   A  NN  N  C      E                   %
%               EEE    N N N  HHHHH  AAAAA  N N N  C      EEE                 %
%               E      N  NN  H   H  A   A  N  NN  C      E                   %
%               EEEEE  N   N  H   H  A   A  N   N   CCCC  EEEEE               %
%                                                                             %
%                         TTTTT   OOO   N   N  EEEEE                          %
%                           T    O   O  NN  N  E                              %
%                           T    O   O  N N N  EEE                            %
%                           T    O   O  N  NN  E                              %
%                           T     OOO   N   N  EEEEE                          %
%                                                                             %
%                                                                             %
%                   MagickCore Image Tone and Color Methods                   %
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
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/accelerate-private.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/histogram.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/pixel-private.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/token.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C l u t I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClutImage() replaces each color value in the given image, by using it as an
%  index to lookup a replacement color value in a Color Look UP Table in the
%  form of an image.  The values are extracted along a diagonal of the CLUT
%  image so either a horizontal or vertical gradient image can be used.
%
%  Typically this is used to either re-color a gray-scale image according to a
%  color gradient in the CLUT image, or to perform a freeform histogram
%  (level) adjustment according to the (typically gray-scale) gradient in the
%  CLUT image.
%
%  When the 'channel' mask includes the matte/alpha transparency channel but
%  one image has no such channel it is assumed that image is a simple
%  gray-scale image that will effect the alpha channel values, either for
%  gray-scale coloring (with transparent or semi-transparent colors), or
%  a histogram adjustment of existing alpha channel values.   If both images
%  have matte channels, direct and normal indexing is applied, which is rarely
%  used.
%
%  The format of the ClutImage method is:
%
%      MagickBooleanType ClutImage(Image *image,Image *clut_image,
%        const PixelInterpolateMethod method,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image, which is replaced by indexed CLUT values
%
%    o clut_image: the color lookup table image for replacement color values.
%
%    o method: the pixel interpolation method.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void ClutImageRow(const Image *image,const PixelInfo *clut_map,
  Quantum *q)
{
  PixelInfo
    pixel;

  ssize_t
    x;

  GetPixelInfo(image,&pixel);
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    PixelTrait
      traits;

    GetPixelInfoPixel(image,q,&pixel);
    traits=GetPixelChannelTraits(image,RedPixelChannel);
    if ((traits & UpdatePixelTrait) != 0)
      pixel.red=clut_map[ScaleQuantumToMap(ClampToQuantum(
        pixel.red))].red;
    traits=GetPixelChannelTraits(image,GreenPixelChannel);
    if ((traits & UpdatePixelTrait) != 0)
      pixel.green=clut_map[ScaleQuantumToMap(ClampToQuantum(
        pixel.green))].green;
    traits=GetPixelChannelTraits(image,BluePixelChannel);
    if ((traits & UpdatePixelTrait) != 0)
      pixel.blue=clut_map[ScaleQuantumToMap(ClampToQuantum(
        pixel.blue))].blue;
    traits=GetPixelChannelTraits(image,BlackPixelChannel);
    if ((traits & UpdatePixelTrait) != 0)
      pixel.black=clut_map[ScaleQuantumToMap(ClampToQuantum(
        pixel.black))].black;
    traits=GetPixelChannelTraits(image,AlphaPixelChannel);
    if ((traits & UpdatePixelTrait) != 0)
      pixel.alpha=clut_map[ScaleQuantumToMap(ClampToQuantum(
        pixel.alpha))].alpha;
    SetPixelViaPixelInfo(image,&pixel,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType BuildClutMap(const Image *clut_image,
  const PixelInterpolateMethod method,PixelInfo *clut_map,
  ExceptionInfo *exception)
{
  CacheView
    *clut_view;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    adjust,
    i;

  adjust=(ssize_t) (method == IntegerInterpolatePixel ? 0 : 1);
  clut_view=AcquireVirtualCacheView(clut_image,exception);
  for (i=0; i <= (ssize_t) MaxMap; i++)
  {
    GetPixelInfo(clut_image,clut_map+i);
    status=InterpolatePixelInfo(clut_image,clut_view,method,(double) i*
      ((double) clut_image->columns-adjust)/MaxMap,(double) i*
      ((double) clut_image->rows-adjust)/MaxMap,clut_map+i,exception);
    if (status == MagickFalse)
      break;
  }
  clut_view=DestroyCacheView(clut_view);
  return(status);
}

static inline MagickBooleanType IsGrayImageWithColorClut(const Image *image,
  const Image *clut_image)
{
  return(((IsGrayColorspace(image->colorspace) != MagickFalse) &&
    (IsGrayColorspace(clut_image->colorspace) == MagickFalse)) ? MagickTrue :
    MagickFalse);
}

static void ActivateClutAlpha(Image *image,const Image *clut_image,
  ExceptionInfo *exception)
{
  if ((clut_image->alpha_trait != UndefinedPixelTrait) &&
      ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0))
    (void) SetImageAlphaChannel(image,ActivateAlphaChannel,exception);
}

MagickExport MagickBooleanType ClutImage(Image *image,const Image *clut_image,
  const PixelInterpolateMethod method,ExceptionInfo *exception)
{
#define ClutImageTag  "Clut/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    *clut_map;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(clut_image != (Image *) NULL);
  assert(clut_image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if (IsGrayImageWithColorClut(image,clut_image) != MagickFalse)
    (void) SetImageColorspace(image,sRGBColorspace,exception);
  clut_map=(PixelInfo *) AcquireQuantumMemory(MaxMap+1UL,sizeof(*clut_map));
  if (clut_map == (PixelInfo *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  /*
    Clut image.
  */
  status=MagickTrue;
  progress=0;
  status=BuildClutMap(clut_image,method,clut_map,exception);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    ClutImageRow(image,clut_map,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,ClutImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  clut_map=(PixelInfo *) RelinquishMagickMemory(clut_map);
  ActivateClutAlpha(image,clut_image,exception);
  return(status);
}

static void ParseCDLTriple(XMLTreeInfo *node,double *red,double *green,
  double *blue)
{
  char
    token[MagickPathExtent];

  const char
    *content,
    *p;

  ssize_t
    i;

  content=GetXMLTreeContent(node);
  p=(const char *) content;
  for (i=0; (*p != '\0') && (i < 3); i++)
  {
    (void) GetNextToken(p,&p,MagickPathExtent,token);
    if (*token == ',')
      (void) GetNextToken(p,&p,MagickPathExtent,token);
    switch (i)
    {
      case 0:
      {
        *red=StringToDouble(token,(char **) NULL);
        break;
      }
      case 1:
      {
        *green=StringToDouble(token,
          (char **) NULL);
        break;
      }
      case 2:
      {
        *blue=StringToDouble(token,
          (char **) NULL);
        break;
      }
    }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o l o r D e c i s i o n L i s t I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ColorDecisionListImage() accepts a lightweight Color Correction Collection
%  (CCC) file which solely contains one or more color corrections and applies
%  the correction to the image.  Here is a sample CCC file:
%
%    <ColorCorrectionCollection xmlns="urn:ASC:CDL:v1.2">
%          <ColorCorrection id="cc03345">
%                <SOPNode>
%                     <Slope> 0.9 1.2 0.5 </Slope>
%                     <Offset> 0.4 -0.5 0.6 </Offset>
%                     <Power> 1.0 0.8 1.5 </Power>
%                </SOPNode>
%                <SATNode>
%                     <Saturation> 0.85 </Saturation>
%                </SATNode>
%          </ColorCorrection>
%    </ColorCorrectionCollection>
%
%  which includes the slop, offset, and power for each of the RGB channels
%  as well as the saturation.
%
%  The format of the ColorDecisionListImage method is:
%
%      MagickBooleanType ColorDecisionListImage(Image *image,
%        const char *color_correction_collection,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o color_correction_collection: the color correction collection in XML.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _Correction
{
  double
    slope,
    offset,
    power;
} Correction;

typedef struct _ColorCorrection
{
  Correction
    red,
    green,
    blue;

  double
    saturation;
} ColorCorrection;

static ColorCorrection ParseCDLSOPNode(XMLTreeInfo *sop,
  ColorCorrection color_correction)
{
  XMLTreeInfo
    *offset,
    *power,
    *slope;

  slope=GetXMLTreeChild(sop,"Slope");
  if (slope != (XMLTreeInfo *) NULL)
    ParseCDLTriple(slope,&color_correction.red.slope,
      &color_correction.green.slope,&color_correction.blue.slope);
  offset=GetXMLTreeChild(sop,"Offset");
  if (offset != (XMLTreeInfo *) NULL)
    ParseCDLTriple(offset,&color_correction.red.offset,
      &color_correction.green.offset,&color_correction.blue.offset);
  power=GetXMLTreeChild(sop,"Power");
  if (power != (XMLTreeInfo *) NULL)
    ParseCDLTriple(power,&color_correction.red.power,
      &color_correction.green.power,&color_correction.blue.power);
  return(color_correction);
}

static ColorCorrection ParseCDLSATNode(XMLTreeInfo *sat,
  ColorCorrection color_correction)
{
  char
    token[MagickPathExtent];

  const char
    *content,
    *p;

  XMLTreeInfo
    *saturation;

  saturation=GetXMLTreeChild(sat,"Saturation");
  if (saturation != (XMLTreeInfo *) NULL)
    {
      content=GetXMLTreeContent(saturation);
      p=(const char *) content;
      (void) GetNextToken(p,&p,MagickPathExtent,token);
      color_correction.saturation=StringToDouble(token,(char **) NULL);
    }
  return(color_correction);
}

static void BuildCDLMap(const ColorCorrection color_correction,
  PixelInfo *cdl_map)
{
  ssize_t
    i;

  for (i=0; i <= (ssize_t) MaxMap; i++)
  {
    cdl_map[i].red=(double) ScaleMapToQuantum((double)
      (MaxMap*(pow(color_correction.red.slope*i/MaxMap+
      color_correction.red.offset,color_correction.red.power))));
    cdl_map[i].green=(double) ScaleMapToQuantum((double)
      (MaxMap*(pow(color_correction.green.slope*i/MaxMap+
      color_correction.green.offset,color_correction.green.power))));
    cdl_map[i].blue=(double) ScaleMapToQuantum((double)
      (MaxMap*(pow(color_correction.blue.slope*i/MaxMap+
      color_correction.blue.offset,color_correction.blue.power))));
  }
}

static void ApplyCDLToColormap(Image *image,
  const ColorCorrection color_correction,const PixelInfo *cdl_map)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) image->colors; i++)
  {
    /*
      Apply transfer function to colormap.
    */
    double
      luma;

    luma=0.21267*image->colormap[i].red+0.71526*image->colormap[i].green+
      0.07217*image->colormap[i].blue;
    image->colormap[i].red=luma+color_correction.saturation*cdl_map[
      ScaleQuantumToMap(ClampToQuantum(image->colormap[i].red))].red-luma;
    image->colormap[i].green=luma+color_correction.saturation*cdl_map[
      ScaleQuantumToMap(ClampToQuantum(image->colormap[i].green))].green-
      luma;
    image->colormap[i].blue=luma+color_correction.saturation*cdl_map[
      ScaleQuantumToMap(ClampToQuantum(image->colormap[i].blue))].blue-luma;
  }
}

static void ApplyCDLToPixels(const Image *image,
  const ColorCorrection color_correction,const PixelInfo *cdl_map,
  Quantum *magick_restrict q)
{
  double
    luma;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    luma=0.21267*(double) GetPixelRed(image,q)+0.71526*(double)
      GetPixelGreen(image,q)+0.07217*(double) GetPixelBlue(image,q);
    SetPixelRed(image,ClampToQuantum(luma+color_correction.saturation*
      (cdl_map[ScaleQuantumToMap(GetPixelRed(image,q))].red-luma)),q);
    SetPixelGreen(image,ClampToQuantum(luma+color_correction.saturation*
      (cdl_map[ScaleQuantumToMap(GetPixelGreen(image,q))].green-luma)),q);
    SetPixelBlue(image,ClampToQuantum(luma+color_correction.saturation*
      (cdl_map[ScaleQuantumToMap(GetPixelBlue(image,q))].blue-luma)),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static ColorCorrection ParseColorCorrection(XMLTreeInfo *cc)
{
  ColorCorrection
    color_correction;

  XMLTreeInfo
    *sat,
    *sop;

  color_correction.red.slope=1.0;
  color_correction.red.offset=0.0;
  color_correction.red.power=1.0;
  color_correction.green.slope=1.0;
  color_correction.green.offset=0.0;
  color_correction.green.power=1.0;
  color_correction.blue.slope=1.0;
  color_correction.blue.offset=0.0;
  color_correction.blue.power=1.0;
  color_correction.saturation=0.0;
  sop=GetXMLTreeChild(cc,"SOPNode");
  if (sop != (XMLTreeInfo *) NULL)
    color_correction=ParseCDLSOPNode(sop,color_correction);
  sat=GetXMLTreeChild(cc,"SATNode");
  if (sat != (XMLTreeInfo *) NULL)
    color_correction=ParseCDLSATNode(sat,color_correction);
  return(color_correction);
}

static void LogColorCorrection(const Image *image,
  const ColorCorrection color_correction)
{
  if (image->debug != MagickFalse)
    {
      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  Color Correction Collection:");
      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  color_correction.red.slope: %g",color_correction.red.slope);
      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  color_correction.red.offset: %g",color_correction.red.offset);
      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  color_correction.red.power: %g",color_correction.red.power);
      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  color_correction.green.slope: %g",color_correction.green.slope);
      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  color_correction.green.offset: %g",color_correction.green.offset);
      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  color_correction.green.power: %g",color_correction.green.power);
      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  color_correction.blue.slope: %g",color_correction.blue.slope);
      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  color_correction.blue.offset: %g",color_correction.blue.offset);
      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  color_correction.blue.power: %g",color_correction.blue.power);
      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  color_correction.saturation: %g",color_correction.saturation);
    }
}

MagickExport MagickBooleanType ColorDecisionListImage(Image *image,
  const char *color_correction_collection,ExceptionInfo *exception)
{
#define ColorDecisionListCorrectImageTag  "ColorDecisionList/Image"

  CacheView
    *image_view;

  ColorCorrection
    color_correction;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    *cdl_map;

  ssize_t
    y;

  XMLTreeInfo
    *cc,
    *ccc;

  /*
    Allocate and initialize cdl maps.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (color_correction_collection == (const char *) NULL)
    return(MagickFalse);
  ccc=NewXMLTree((const char *) color_correction_collection,exception);
  if (ccc == (XMLTreeInfo *) NULL)
    return(MagickFalse);
  cc=GetXMLTreeChild(ccc,"ColorCorrection");
  if (cc == (XMLTreeInfo *) NULL)
    {
      ccc=DestroyXMLTree(ccc);
      return(MagickFalse);
    }
  color_correction=ParseColorCorrection(cc);
  ccc=DestroyXMLTree(ccc);
  LogColorCorrection(image,color_correction);
  cdl_map=(PixelInfo *) AcquireQuantumMemory(MaxMap+1UL,sizeof(*cdl_map));
  if (cdl_map == (PixelInfo *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  BuildCDLMap(color_correction,cdl_map);
  if (image->storage_class == PseudoClass)
    {
      ApplyCDLToColormap(image,color_correction,cdl_map);
      cdl_map=(PixelInfo *) RelinquishMagickMemory(cdl_map);
      (void) SyncImage(image, exception);
      return(MagickTrue);
    }
  /*
    Apply transfer function to image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    ApplyCDLToPixels(image,color_correction,cdl_map,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,ColorDecisionListCorrectImageTag,
          progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  cdl_map=(PixelInfo *) RelinquishMagickMemory(cdl_map);
  return(status);
}

static inline MagickBooleanType IsStretchMemoryMissing(const Quantum *black,
  const Quantum *white,const Quantum *stretch_map,const double *histogram)
{
  return(((black == (Quantum *) NULL) || (white == (Quantum *) NULL) ||
    (stretch_map == (Quantum *) NULL) || (histogram == (double *) NULL)) ?
    MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o n t r a s t S t r e t c h I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ContrastStretchImage() is a simple image enhancement technique that attempts
%  to improve the contrast in an image by 'stretching' the range of intensity
%  values it contains to span a desired range of values. It differs from the
%  more sophisticated histogram equalization in that it can only apply a
%  linear scaling function to the image pixel values.  As a result the
%  'enhancement' is less harsh.
%
%  The format of the ContrastStretchImage method is:
%
%      MagickBooleanType ContrastStretchImage(Image *image,
%        const char *levels,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o black_point: the black point.
%
%    o white_point: the white point.
%
%    o levels: Specify the levels where the black and white points have the
%      range of 0 to number-of-pixels (e.g. 1%, 10x90%, etc.).
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void AccumulateStretchHistogram(const Image *image,
  const Quantum *p,double *histogram)
{
  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    double
      pixel;

    pixel=GetPixelIntensity(image,p);
    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      if (image->channel_mask != AllChannels)
        pixel=(double) p[i];
      histogram[GetPixelChannels(image)*ScaleQuantumToMap(
        ClampToQuantum(pixel))+(size_t) i]++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void BuildStretchMap(const Image *image,const Quantum *black,
  const Quantum *white,Quantum *stretch_map)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    ssize_t
      j;

    for (j=0; j <= (ssize_t) MaxMap; j++)
    {
      double
        gamma;

      gamma=MagickSafeReciprocal(white[i]-black[i]);
      if (j < (ssize_t) black[i])
        stretch_map[(ssize_t) GetPixelChannels(image)*j+i]=(Quantum) 0;
      else
        if (j > (ssize_t) white[i])
          stretch_map[(ssize_t) GetPixelChannels(image)*j+i]=QuantumRange;
        else
          if (black[i] != white[i])
            stretch_map[(ssize_t) GetPixelChannels(image)*j+i]=
              ScaleMapToQuantum((double) (MaxMap*gamma*(j-(double) black[i])));
    }
  }
}

static void StretchColormap(Image *image,const Quantum *stretch_map)
{
  ssize_t
    i,
    j;

  for (j=0; j < (ssize_t) image->colors; j++)
  {
    if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
      {
        i=GetPixelChannelOffset(image,RedPixelChannel);
        image->colormap[j].red=(MagickRealType) stretch_map[
          GetPixelChannels(image)*ScaleQuantumToMap(ClampToQuantum(
          image->colormap[j].red))+(size_t) i];
      }
    if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
      {
        i=GetPixelChannelOffset(image,GreenPixelChannel);
        image->colormap[j].green=(MagickRealType) stretch_map[
          GetPixelChannels(image)*ScaleQuantumToMap(ClampToQuantum(
          image->colormap[j].green))+(size_t) i];
      }
    if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
      {
        i=GetPixelChannelOffset(image,BluePixelChannel);
        image->colormap[j].blue=(MagickRealType) stretch_map[
          GetPixelChannels(image)*ScaleQuantumToMap(ClampToQuantum(
          image->colormap[j].blue))+(size_t) i];
      }
    if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
      {
        i=GetPixelChannelOffset(image,AlphaPixelChannel);
        image->colormap[j].alpha=(MagickRealType) stretch_map[
          GetPixelChannels(image)*ScaleQuantumToMap(ClampToQuantum(
          image->colormap[j].alpha))+(size_t) i];
      }
  }
}

typedef struct _StretchMaps
{
  const Quantum
    *black,
    *white,
    *stretch_map;
} StretchMaps;

static void StretchImageRow(const Image *image,const StretchMaps *maps,
  Quantum *q)
{
  const Quantum
    *black = maps->black,
    *white = maps->white,
    *stretch_map = maps->stretch_map;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      j;

    for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,j);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if ((traits & UpdatePixelTrait) == 0)
        continue;
      if (black[j] == white[j])
        continue;
      q[j]=ClampToQuantum(stretch_map[GetPixelChannels(image)*
        ScaleQuantumToMap(q[j])+(size_t) j]);
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static ssize_t FindStretchBlackLevel(const Image *image,
  const double *histogram,const ssize_t i,const double black_point)
{
  double
    intensity;

  ssize_t
    j;

  intensity=0.0;
  for (j=0; j <= (ssize_t) MaxMap; j++)
  {
    intensity+=histogram[(ssize_t) GetPixelChannels(image)*j+i];
    if (intensity > black_point)
      break;
  }
  return(j);
}

static ssize_t FindStretchWhiteLevel(const Image *image,
  const double *histogram,const ssize_t i,const double white_point)
{
  double
    intensity;

  ssize_t
    j;

  intensity=0.0;
  for (j=(ssize_t) MaxMap; j != 0; j--)
  {
    intensity+=histogram[(ssize_t) GetPixelChannels(image)*j+i];
    if (intensity > ((double) image->columns*image->rows-white_point))
      break;
  }
  return(j);
}

static void RelinquishStretchMemory(double *histogram,Quantum *stretch_map,
  Quantum *white,Quantum *black)
{
  if (histogram != (double *) NULL)
    histogram=(double *) RelinquishMagickMemory(histogram);
  if (stretch_map != (Quantum *) NULL)
    stretch_map=(Quantum *) RelinquishMagickMemory(stretch_map);
  if (white != (Quantum *) NULL)
    white=(Quantum *) RelinquishMagickMemory(white);
  if (black != (Quantum *) NULL)
    black=(Quantum *) RelinquishMagickMemory(black);
}

MagickExport MagickBooleanType ContrastStretchImage(Image *image,
  const double black_point,const double white_point,ExceptionInfo *exception)
{
#define ContrastStretchImageTag  "ContrastStretch/Image"

  CacheView
    *image_view;

  char
    property[MagickPathExtent];

  double
    *histogram;

  ImageType
    type;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  Quantum
    *black,
    *stretch_map,
    *white;

  ssize_t
    i;

  ssize_t
    y;

  /*
    Allocate histogram and stretch map.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  type=IdentifyImageType(image,exception);
  if (IsGrayImageType(type) != MagickFalse)
    (void) SetImageColorspace(image,GRAYColorspace,exception);
  black=(Quantum *) AcquireQuantumMemory(MaxPixelChannels,sizeof(*black));
  white=(Quantum *) AcquireQuantumMemory(MaxPixelChannels,sizeof(*white));
  stretch_map=(Quantum *) AcquireQuantumMemory(MaxMap+1UL,MaxPixelChannels*
    sizeof(*stretch_map));
  histogram=(double *) AcquireQuantumMemory(MaxMap+1UL,MaxPixelChannels*
    sizeof(*histogram));
  if (IsStretchMemoryMissing(black,white,stretch_map,histogram) != MagickFalse)
    {
      RelinquishStretchMemory(histogram,stretch_map,white,black);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  /*
    Form histogram.
  */
  status=MagickTrue;
  (void) memset(histogram,0,(MaxMap+1)*GetPixelChannels(image)*
    sizeof(*histogram));
  image_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    AccumulateStretchHistogram(image,p,histogram);
  }
  image_view=DestroyCacheView(image_view);
  /*
    Find the histogram boundaries by locating the black/white levels.
  */
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    ssize_t
      j;

    black[i]=(Quantum) 0;
    white[i]=(Quantum) ScaleQuantumToMap(QuantumRange);
    j=FindStretchBlackLevel(image,histogram,i,black_point);
    black[i]=(Quantum) j;
    j=FindStretchWhiteLevel(image,histogram,i,white_point);
    white[i]=(Quantum) j;
  }
  histogram=(double *) RelinquishMagickMemory(histogram);
  /*
    Stretch the histogram to create the stretched image mapping.
  */
  (void) memset(stretch_map,0,(MaxMap+1)*GetPixelChannels(image)*
    sizeof(*stretch_map));
  BuildStretchMap(image,black,white,stretch_map);
  /*
    Stretch-contrast colormap.
  */
  if (image->storage_class == PseudoClass)
    StretchColormap(image,stretch_map);
  /*
    Stretch-contrast image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    const StretchMaps
      maps = { black, white, stretch_map };

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    StretchImageRow(image,&maps,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,ContrastStretchImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  (void) FormatLocaleString(property,MagickPathExtent,"%gx%g%%",100.0*
    QuantumScale*GetPixelIntensity(image,black),100.0*QuantumScale*
    GetPixelIntensity(image,white));
  (void) SetImageProperty(image,"histogram:contrast-stretch",property,
    exception);
  white=(Quantum *) RelinquishMagickMemory(white);
  black=(Quantum *) RelinquishMagickMemory(black);
  stretch_map=(Quantum *) RelinquishMagickMemory(stretch_map);
  return(status);
}

static inline MagickBooleanType IsEqualizeMemoryMissing(
  const double *equalize_map,const double *histogram,const double *map)
{
  return(((equalize_map == (double *) NULL) || (histogram == (double *) NULL) ||
    (map == (double *) NULL)) ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     E q u a l i z e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EqualizeImage() applies a histogram equalization to the image.
%
%  The format of the EqualizeImage method is:
%
%      MagickBooleanType EqualizeImage(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void AccumulateEqualizeHistogram(const Image *image,
  const Quantum *p,double *histogram)
{
  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      double
        intensity;

      intensity=(double) p[i];
      if ((image->channel_mask & SyncChannels) != 0)
        intensity=GetPixelIntensity(image,p);
      histogram[GetPixelChannels(image)*ScaleQuantumToMap(
        ClampToQuantum(intensity))+(size_t) i]++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void EqualizeColormap(Image *image,const double *black,
  const double *white,const double *equalize_map)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) image->colors; j++)
  {
    if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
      {
        PixelChannel channel = GetPixelChannelChannel(image,
          RedPixelChannel);
        if (black[channel] != white[channel])
          image->colormap[j].red=equalize_map[(ssize_t)
            GetPixelChannels(image)*ScaleQuantumToMap(
            ClampToQuantum(image->colormap[j].red))+channel];
      }
    if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
      {
        PixelChannel channel = GetPixelChannelChannel(image,
          GreenPixelChannel);
        if (black[channel] != white[channel])
          image->colormap[j].green=equalize_map[(ssize_t)
            GetPixelChannels(image)*ScaleQuantumToMap(
            ClampToQuantum(image->colormap[j].green))+channel];
      }
    if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
      {
        PixelChannel channel = GetPixelChannelChannel(image,
          BluePixelChannel);
        if (black[channel] != white[channel])
          image->colormap[j].blue=equalize_map[(ssize_t)
            GetPixelChannels(image)*ScaleQuantumToMap(
            ClampToQuantum(image->colormap[j].blue))+channel];
      }
    if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
      {
        PixelChannel channel = GetPixelChannelChannel(image,
          AlphaPixelChannel);
        if (black[channel] != white[channel])
          image->colormap[j].alpha=equalize_map[(ssize_t)
            GetPixelChannels(image)*ScaleQuantumToMap(
            ClampToQuantum(image->colormap[j].alpha))+channel];
      }
  }
}

static void EqualizeImageRow(const Image *image,const double *black,
  const double *white,const double *equalize_map,Quantum *q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      j;

    for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,j);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if (((traits & UpdatePixelTrait) == 0) || (black[j] == white[j]))
        continue;
      q[j]=ClampToQuantum(equalize_map[GetPixelChannels(image)*
        ScaleQuantumToMap(q[j])+(size_t) j]);
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void IntegrateEqualizeHistogram(const Image *image,
  const double *histogram,double *map)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      intensity;

    ssize_t
      j;

    intensity=0.0;
    for (j=0; j <= (ssize_t) MaxMap; j++)
    {
      intensity+=histogram[(ssize_t) GetPixelChannels(image)*j+i];
      map[(ssize_t) GetPixelChannels(image)*j+i]=intensity;
    }
  }
}

static MagickBooleanType FormEqualizeHistogram(const Image *image,
  double *histogram,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  status=MagickTrue;
  (void) memset(histogram,0,(MaxMap+1)*GetPixelChannels(image)*
    sizeof(*histogram));
  image_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    AccumulateEqualizeHistogram(image,p,histogram);
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

typedef struct _EqualizeChannel
{
  Image
    *image;

  double
    *map;

  ssize_t
    i;
} EqualizeChannel;

static void BuildEqualizeChannelMap(const EqualizeChannel *equalize,
  const double *black,const double *white,double *equalize_map)
{
  Image
    *const image = equalize->image;

  double
    *const map = equalize->map;

  const ssize_t
    i = equalize->i;

  ssize_t
    j;

  if (black[i] != white[i])
    for (j=0; j <= (ssize_t) MaxMap; j++)
      equalize_map[GetPixelChannels(image)*(size_t) j+(size_t) i]=(double)
        ScaleMapToQuantum((double) ((MaxMap*(map[GetPixelChannels(image)*
        (size_t) j+(size_t) i]-black[i]))/(white[i]-black[i])));
}

static void RelinquishEqualizeMemory(double *map,double *histogram,
  double *equalize_map)
{
  if (map != (double *) NULL)
    map=(double *) RelinquishMagickMemory(map);
  if (histogram != (double *) NULL)
    histogram=(double *) RelinquishMagickMemory(histogram);
  if (equalize_map != (double *) NULL)
    equalize_map=(double *) RelinquishMagickMemory(equalize_map);
}

MagickExport MagickBooleanType EqualizeImage(Image *image,
  ExceptionInfo *exception)
{
#define EqualizeImageTag  "Equalize/Image"

  CacheView
    *image_view;

  double
    black[2*CompositePixelChannel+1],
    *equalize_map,
    *histogram,
    *map,
    white[2*CompositePixelChannel+1];

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    i;

  ssize_t
    y;

  /*
    Allocate and initialize histogram arrays.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  if (AccelerateEqualizeImage(image,exception) != MagickFalse)
    return(MagickTrue);
#endif
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  equalize_map=(double *) AcquireQuantumMemory(MaxMap+1UL,MaxPixelChannels*
    sizeof(*equalize_map));
  histogram=(double *) AcquireQuantumMemory(MaxMap+1UL,MaxPixelChannels*
    sizeof(*histogram));
  map=(double *) AcquireQuantumMemory(MaxMap+1UL,MaxPixelChannels*sizeof(*map));
  if (IsEqualizeMemoryMissing(equalize_map,histogram,map) != MagickFalse)
    {
      RelinquishEqualizeMemory(map,histogram,equalize_map);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  /*
    Form histogram.
  */
  status=FormEqualizeHistogram(image,histogram,exception);
  /*
    Integrate the histogram to get the equalization map.
  */
  IntegrateEqualizeHistogram(image,histogram,map);
  (void) memset(equalize_map,0,(MaxMap+1)*GetPixelChannels(image)*
    sizeof(*equalize_map));
  (void) memset(black,0,sizeof(*black));
  (void) memset(white,0,sizeof(*white));
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    EqualizeChannel
      equalize;

    black[i]=map[i];
    white[i]=map[GetPixelChannels(image)*MaxMap+(size_t) i];
    equalize.image=image;
    equalize.map=map;
    equalize.i=i;
    BuildEqualizeChannelMap(&equalize,black,white,equalize_map);
  }
  histogram=(double *) RelinquishMagickMemory(histogram);
  map=(double *) RelinquishMagickMemory(map);
  /*
    Equalize colormap.
  */
  if (image->storage_class == PseudoClass)
    EqualizeColormap(image,black,white,equalize_map);
  /*
    Equalize image.
  */
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    EqualizeImageRow(image,black,white,equalize_map,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,EqualizeImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  equalize_map=(double *) RelinquishMagickMemory(equalize_map);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     G a m m a I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GammaImage() gamma-corrects a particular image channel.  The same
%  image viewed on different devices will have perceptual differences in the
%  way the image's intensities are represented on the screen.  Specify
%  individual gamma levels for the red, green, and blue channels, or adjust
%  all three with the gamma parameter.  Values typically range from 0.8 to 2.3.
%
%  You can also reduce the influence of a particular channel with a gamma
%  value of 0.
%
%  The format of the GammaImage method is:
%
%      MagickBooleanType GammaImage(Image *image,const double gamma,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o level: the image gamma as a string (e.g. 1.6,1.2,1.0).
%
%    o gamma: the image gamma.
%
*/

static inline double gamma_pow(const double value,const double gamma)
{
  return(value < 0.0 ? value : pow(value,gamma));
}

static void GammaImageRow(const Image *image,const Quantum *gamma_map,
  Quantum *q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      j;

    for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,j);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if ((traits & UpdatePixelTrait) == 0)
        continue;
      q[j]=gamma_map[ScaleQuantumToMap(ClampToQuantum((MagickRealType)
        q[j]))];
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void BuildGammaMap(Quantum *gamma_map,const double gamma)
{
  ssize_t
    i;

  if (gamma != 0.0)
    for (i=0; i <= (ssize_t) MaxMap; i++)
      gamma_map[i]=ScaleMapToQuantum((double) (MaxMap*pow((double) i/
        MaxMap,MagickSafeReciprocal(gamma))));
}

static void GammaColormap(Image *image,const Quantum *gamma_map)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) image->colors; i++)
  {
    /*
      Gamma-correct colormap.
    */
    if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].red=(double) gamma_map[ScaleQuantumToMap(
        ClampToQuantum(image->colormap[i].red))];
    if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].green=(double) gamma_map[ScaleQuantumToMap(
        ClampToQuantum(image->colormap[i].green))];
    if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].blue=(double) gamma_map[ScaleQuantumToMap(
        ClampToQuantum(image->colormap[i].blue))];
    if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].alpha=(double) gamma_map[ScaleQuantumToMap(
        ClampToQuantum(image->colormap[i].alpha))];
  }
}

MagickExport MagickBooleanType GammaImage(Image *image,const double gamma,
  ExceptionInfo *exception)
{
#define GammaImageTag  "Gamma/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  Quantum
    *gamma_map;

  ssize_t
    y;

  /*
    Allocate and initialize gamma maps.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (gamma == 1.0)
    return(MagickTrue);
  gamma_map=(Quantum *) AcquireQuantumMemory(MaxMap+1UL,sizeof(*gamma_map));
  if (gamma_map == (Quantum *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  (void) memset(gamma_map,0,(MaxMap+1)*sizeof(*gamma_map));
  BuildGammaMap(gamma_map,gamma);
  if (image->storage_class == PseudoClass)
    GammaColormap(image,gamma_map);
  /*
    Gamma-correct image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    GammaImageRow(image,gamma_map,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,GammaImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  gamma_map=(Quantum *) RelinquishMagickMemory(gamma_map);
  if (image->gamma != 0.0)
    image->gamma*=gamma;
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     G r a y s c a l e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GrayscaleImage() converts the image to grayscale.
%
%  The format of the GrayscaleImage method is:
%
%      MagickBooleanType GrayscaleImage(Image *image,
%        const PixelIntensityMethod method ,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o method: the pixel intensity method.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickRealType GetGrayscaleIntensity(const Image *image,
  const PixelIntensityMethod method,const Quantum *q)
{
  MagickRealType
    blue,
    green,
    red,
    intensity;

  red=(MagickRealType) GetPixelRed(image,q);
  green=(MagickRealType) GetPixelGreen(image,q);
  blue=(MagickRealType) GetPixelBlue(image,q);
  intensity=0.0;
  switch (method)
  {
    case AveragePixelIntensityMethod:
    {
      intensity=(red+green+blue)/3.0;
      break;
    }
    case BrightnessPixelIntensityMethod:
    {
      intensity=MagickMax(MagickMax(red,green),blue);
      break;
    }
    case LightnessPixelIntensityMethod:
    {
      intensity=(MagickMin(MagickMin(red,green),blue)+
        MagickMax(MagickMax(red,green),blue))/2.0;
      break;
    }
    case MSPixelIntensityMethod:
    {
      intensity=(MagickRealType) (((double) red*red+green*green+
        blue*blue)/3.0);
      break;
    }
    case Rec601LumaPixelIntensityMethod:
    {
      if (image->colorspace == RGBColorspace)
        {
          red=EncodePixelGamma(red);
          green=EncodePixelGamma(green);
          blue=EncodePixelGamma(blue);
        }
      intensity=0.298839*red+0.586811*green+0.114350*blue;
      break;
    }
    case Rec601LuminancePixelIntensityMethod:
    {
      if (image->colorspace == sRGBColorspace)
        {
          red=DecodePixelGamma(red);
          green=DecodePixelGamma(green);
          blue=DecodePixelGamma(blue);
        }
      intensity=0.298839*red+0.586811*green+0.114350*blue;
      break;
    }
    case Rec709LumaPixelIntensityMethod:
    default:
    {
      if (image->colorspace == RGBColorspace)
        {
          red=EncodePixelGamma(red);
          green=EncodePixelGamma(green);
          blue=EncodePixelGamma(blue);
        }
      intensity=0.212656*red+0.715158*green+0.072186*blue;
      break;
    }
    case Rec709LuminancePixelIntensityMethod:
    {
      if (image->colorspace == sRGBColorspace)
        {
          red=DecodePixelGamma(red);
          green=DecodePixelGamma(green);
          blue=DecodePixelGamma(blue);
        }
      intensity=0.212656*red+0.715158*green+0.072186*blue;
      break;
    }
    case RMSPixelIntensityMethod:
    {
      intensity=(MagickRealType) (sqrt((double) red*red+green*green+
        blue*blue)/sqrt(3.0));
      break;
    }
  }
  return(intensity);
}

static void GrayscaleImageRow(const Image *image,
  const PixelIntensityMethod method,Quantum *q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    MagickRealType
      intensity;

    intensity=GetGrayscaleIntensity(image,method,q);
    SetPixelGray(image,ClampToQuantum(intensity),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType SyncGrayscaleImageClass(Image *image,
  ExceptionInfo *exception)
{
  if (image->storage_class == PseudoClass)
    {
      if (SyncImage(image,exception) == MagickFalse)
        return(MagickFalse);
      if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType SetGrayscaleImageColorspace(Image *image,
  const PixelIntensityMethod method,ExceptionInfo *exception)
{
  image->intensity=method;
  image->type=GrayscaleType;
  if ((method == Rec601LuminancePixelIntensityMethod) ||
      (method == Rec709LuminancePixelIntensityMethod))
    return(SetImageColorspace(image,LinearGRAYColorspace,exception));
  return(SetImageColorspace(image,GRAYColorspace,exception));
}

MagickExport MagickBooleanType GrayscaleImage(Image *image,
  const PixelIntensityMethod method,ExceptionInfo *exception)
{
#define GrayscaleImageTag  "Grayscale/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SyncGrayscaleImageClass(image,exception) == MagickFalse)
    return(MagickFalse);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  if (AccelerateGrayscaleImage(image,method,exception) != MagickFalse)
    {
      image->intensity=method;
      image->type=GrayscaleType;
      if ((method == Rec601LuminancePixelIntensityMethod) ||
          (method == Rec709LuminancePixelIntensityMethod))
        return(SetImageColorspace(image,LinearGRAYColorspace,exception));
      return(SetImageColorspace(image,GRAYColorspace,exception));
    }
#endif
  /*
    Grayscale image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    GrayscaleImageRow(image,method,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,GrayscaleImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  return(SetGrayscaleImageColorspace(image,method,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     H a l d C l u t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  HaldClutImage() applies a Hald color lookup table to the image.  A Hald
%  color lookup table is a 3-dimensional color cube mapped to 2 dimensions.
%  Create it with the HALD coder.  You can apply any color transformation to
%  the Hald image and then use this method to apply the transform to the
%  image.
%
%  The format of the HaldClutImage method is:
%
%      MagickBooleanType HaldClutImage(Image *image,Image *hald_image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image, which is replaced by indexed CLUT values
%
%    o hald_image: the color lookup table image for replacement color values.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _HaldInfo
{
  double
    x,
    y,
    z;
} HaldInfo;

static HaldInfo GetHaldPoint(const Image *image,const size_t level,
  const Quantum *magick_restrict q)
{
  HaldInfo
    point;

  point.x=QuantumScale*(level-1.0)*(double) GetPixelRed(image,q);
  point.y=QuantumScale*(level-1.0)*(double) GetPixelGreen(image,q);
  point.z=QuantumScale*(level-1.0)*(double) GetPixelBlue(image,q);
  return(point);
}

static void SetHaldPixel(const Image *image,const PixelInfo pixel,
  Quantum *magick_restrict q)
{
  if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
    SetPixelRed(image,ClampToQuantum(pixel.red),q);
  if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
    SetPixelGreen(image,ClampToQuantum(pixel.green),q);
  if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
    SetPixelBlue(image,ClampToQuantum(pixel.blue),q);
  if (((GetPixelBlackTraits(image) & UpdatePixelTrait) != 0) &&
      (image->colorspace == CMYKColorspace))
    SetPixelBlack(image,ClampToQuantum(pixel.black),q);
  if (((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0) &&
      (image->alpha_trait != UndefinedPixelTrait))
    SetPixelAlpha(image,ClampToQuantum(pixel.alpha),q);
}

MagickExport MagickBooleanType HaldClutImage(Image *image,
  const Image *hald_image,ExceptionInfo *exception)
{
#define HaldClutImageTag  "Clut/Image"

  CacheView
    *hald_view,
    *image_view;

  double
    width;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    zero;

  size_t
    cube_size,
    length,
    level;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(hald_image != (Image *) NULL);
  assert(hald_image->signature == MagickCoreSignature);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
  if (image->colorspace != hald_image->colorspace)
    (void) SetImageColorspace(image,hald_image->colorspace,exception);
  /*
    Hald clut image.
  */
  status=MagickTrue;
  progress=0;
  length=(size_t) MagickMin((MagickRealType) hald_image->columns,
    (MagickRealType) hald_image->rows);
  for (level=2; (level*level*level) < length; level++) ;
  level*=level;
  cube_size=level*level;
  width=(double) hald_image->columns;
  GetPixelInfo(hald_image,&zero);
  hald_view=AcquireVirtualCacheView(hald_image,exception);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      double
        area = 0.0,
        offset = 0.0;

      HaldInfo
        point = { 0, 0, 0 };

      PixelInfo
        pixel = zero,
        pixel1 = zero,
        pixel2 = zero,
        pixel3 = zero,
        pixel4 = zero;

      point=GetHaldPoint(image,level,q);
      offset=point.x+level*floor(point.y)+cube_size*floor(point.z);
      point.x-=floor(point.x);
      point.y-=floor(point.y);
      point.z-=floor(point.z);
      status=InterpolatePixelInfo(hald_image,hald_view,hald_image->interpolate,
        fmod(offset,width),floor(offset/width),&pixel1,exception);
      if (status == MagickFalse)
        break;
      status=InterpolatePixelInfo(hald_image,hald_view,hald_image->interpolate,
        fmod(offset+level,width),floor((offset+level)/width),&pixel2,exception);
      if (status == MagickFalse)
        break;
      area=point.y;
      if (hald_image->interpolate == NearestInterpolatePixel)
        area=(point.y < 0.5) ? 0.0 : 1.0;
      CompositePixelInfoAreaBlend(&pixel1,pixel1.alpha,&pixel2,pixel2.alpha,
        area,&pixel3);
      offset+=cube_size;
      status=InterpolatePixelInfo(hald_image,hald_view,hald_image->interpolate,
        fmod(offset,width),floor(offset/width),&pixel1,exception);
      if (status == MagickFalse)
        break;
      status=InterpolatePixelInfo(hald_image,hald_view,hald_image->interpolate,
        fmod(offset+level,width),floor((offset+level)/width),&pixel2,exception);
      if (status == MagickFalse)
        break;
      CompositePixelInfoAreaBlend(&pixel1,pixel1.alpha,&pixel2,pixel2.alpha,
        area,&pixel4);
      area=point.z;
      if (hald_image->interpolate == NearestInterpolatePixel)
        area=(point.z < 0.5)? 0.0 : 1.0;
      CompositePixelInfoAreaBlend(&pixel3,pixel3.alpha,&pixel4,pixel4.alpha,
        area,&pixel);
      SetHaldPixel(image,pixel,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,HaldClutImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  hald_view=DestroyCacheView(hald_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     L e v e l I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LevelImage() adjusts the levels of a particular image channel by
%  scaling the colors falling between specified white and black points to
%  the full available quantum range.
%
%  The parameters provided represent the black, and white points.  The black
%  point specifies the darkest color in the image. Colors darker than the
%  black point are set to zero.  White point specifies the lightest color in
%  the image.  Colors brighter than the white point are set to the maximum
%  quantum value.
%
%  If a '!' flag is given, map black and white colors to the given levels
%  rather than mapping those levels to black and white.  See
%  LevelizeImage() below.
%
%  Gamma specifies a gamma correction to apply to the image.
%
%  The format of the LevelImage method is:
%
%      MagickBooleanType LevelImage(Image *image,const double black_point,
%        const double white_point,const double gamma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o black_point: The level to map zero (black) to.
%
%    o white_point: The level to map QuantumRange (white) to.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline double LevelPixel(const double black_point,
  const double white_point,const double gamma,const double pixel)
{
  double
    level_pixel,
    scale;

  scale=MagickSafeReciprocal(white_point-black_point);
  level_pixel=(double) QuantumRange*gamma_pow(scale*((double) pixel-(double)
    black_point),MagickSafeReciprocal(gamma));
  return(level_pixel);
}

static void LevelColormap(Image *image,const double black_point,
  const double white_point,const double gamma)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) image->colors; i++)
  {
    /*
      Level colormap.
    */
    if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].red=(double) ClampToQuantum(LevelPixel(black_point,
        white_point,gamma,image->colormap[i].red));
    if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].green=(double) ClampToQuantum(LevelPixel(black_point,
        white_point,gamma,image->colormap[i].green));
    if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].blue=(double) ClampToQuantum(LevelPixel(black_point,
        white_point,gamma,image->colormap[i].blue));
    if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].alpha=(double) ClampToQuantum(LevelPixel(black_point,
        white_point,gamma,image->colormap[i].alpha));
  }
}

typedef struct _LevelPoints
{
  double
    black_point,
    white_point,
    gamma;
} LevelPoints;

static void LevelImageRow(const Image *image,const LevelPoints *level,
  Quantum *q)
{
  const double
    black_point = level->black_point,
    white_point = level->white_point,
    gamma = level->gamma;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      j;

    for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,j);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if ((traits & UpdatePixelTrait) == 0)
        continue;
      q[j]=ClampToQuantum(LevelPixel(black_point,white_point,gamma,
        (double) q[j]));
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

MagickExport MagickBooleanType LevelImage(Image *image,const double black_point,
  const double white_point,const double gamma,ExceptionInfo *exception)
{
#define LevelImageTag  "Level/Image"

  const LevelPoints
    level = { black_point, white_point, gamma };

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Allocate and initialize levels map.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->storage_class == PseudoClass)
    LevelColormap(image,black_point,white_point,gamma);
  /*
    Level image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    LevelImageRow(image,&level,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,LevelImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  (void) ClampImage(image,exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     L e v e l i z e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LevelizeImage() applies the reversed LevelImage() operation to just
%  the specific channels specified.  It compresses the full range of color
%  values, so that they lie between the given black and white points. Gamma is
%  applied before the values are mapped.
%
%  LevelizeImage() can be called with by using a +level command line
%  API option, or using a '!' on a -level or LevelImage() geometry string.
%
%  It can be used to de-contrast a greyscale image to the exact levels
%  specified.  Or by using specific levels for each channel of an image you
%  can convert a gray-scale image to any linear color gradient, according to
%  those levels.
%
%  The format of the LevelizeImage method is:
%
%      MagickBooleanType LevelizeImage(Image *image,const double black_point,
%        const double white_point,const double gamma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o black_point: The level to map zero (black) to.
%
%    o white_point: The level to map QuantumRange (white) to.
%
%    o gamma: adjust gamma by this factor before mapping values.
%
%    o exception: return any errors or warnings in this structure.
%
*/
#define LevelizeValue(x) ClampToQuantum(((MagickRealType) gamma_pow((double) \
  (QuantumScale*((double) x)),gamma))*(white_point-black_point)+black_point)

static void LevelizeImageRow(const Image *image,const LevelPoints *level,
  Quantum *q)
{
  const double
    black_point = level->black_point,
    white_point = level->white_point,
    gamma = level->gamma;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      j;

    for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,j);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if ((traits & UpdatePixelTrait) == 0)
        continue;
      q[j]=LevelizeValue(q[j]);
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

MagickExport MagickBooleanType LevelizeImage(Image *image,
  const double black_point,const double white_point,const double gamma,
  ExceptionInfo *exception)
{
#define LevelizeImageTag  "Levelize/Image"

  const LevelPoints
    level = { black_point, white_point, gamma };

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    i;

  ssize_t
    y;

  /*
    Allocate and initialize levels map.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->storage_class == PseudoClass)
    for (i=0; i < (ssize_t) image->colors; i++)
    {
      /*
        Level colormap.
      */
      if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].red=(double) LevelizeValue(image->colormap[i].red);
      if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].green=(double) LevelizeValue(
          image->colormap[i].green);
      if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].blue=(double) LevelizeValue(image->colormap[i].blue);
      if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].alpha=(double) LevelizeValue(
          image->colormap[i].alpha);
    }
  /*
    Level image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    LevelizeImageRow(image,&level,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,LevelizeImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

static inline MagickBooleanType IsGrayImageWithColorLevels(const Image *image,
  const PixelInfo *black_color,const PixelInfo *white_color)
{
  return(((IsGrayColorspace(image->colorspace) != MagickFalse) &&
    ((IsGrayColorspace(black_color->colorspace) == MagickFalse) ||
     (IsGrayColorspace(white_color->colorspace) == MagickFalse))) ?
    MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     L e v e l I m a g e C o l o r s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LevelImageColors() maps the given color to "black" and "white" values,
%  linearly spreading out the colors, and level values on a channel by channel
%  bases, as per LevelImage().  The given colors allows you to specify
%  different level ranges for each of the color channels separately.
%
%  If the boolean 'invert' is set true the image values will modified in the
%  reverse direction. That is any existing "black" and "white" colors in the
%  image will become the color values given, with all other values compressed
%  appropriately.  This effectively maps a greyscale gradient into the given
%  color gradient.
%
%  The format of the LevelImageColors method is:
%
%    MagickBooleanType LevelImageColors(Image *image,
%      const PixelInfo *black_color,const PixelInfo *white_color,
%      const MagickBooleanType invert,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o black_color: The color to map black to/from
%
%    o white_point: The color to map white to/from
%
%    o invert: if true map the colors (levelize), rather than from (level)
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType LevelImageColors(Image *image,
  const PixelInfo *black_color,const PixelInfo *white_color,
  const MagickBooleanType invert,ExceptionInfo *exception)
{
  ChannelType
    channel_mask;

  MagickStatusType
    status;

  /*
    Allocate and initialize levels map.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (IsGrayImageWithColorLevels(image,black_color,white_color) != MagickFalse)
    (void) SetImageColorspace(image,sRGBColorspace,exception);
  status=MagickTrue;
  if (invert == MagickFalse)
    {
      if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
        {
          channel_mask=SetImageChannelMask(image,RedChannel);
          status&=(MagickStatusType) LevelImage(image,black_color->red,
            white_color->red,1.0,exception);
          (void) SetImageChannelMask(image,channel_mask);
        }
      if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
        {
          channel_mask=SetImageChannelMask(image,GreenChannel);
          status&=(MagickStatusType) LevelImage(image,black_color->green,
            white_color->green,1.0,exception);
          (void) SetImageChannelMask(image,channel_mask);
        }
      if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
        {
          channel_mask=SetImageChannelMask(image,BlueChannel);
          status&=(MagickStatusType) LevelImage(image,black_color->blue,
            white_color->blue,1.0,exception);
          (void) SetImageChannelMask(image,channel_mask);
        }
      if (((GetPixelBlackTraits(image) & UpdatePixelTrait) != 0) &&
          (image->colorspace == CMYKColorspace))
        {
          channel_mask=SetImageChannelMask(image,BlackChannel);
          status&=(MagickStatusType) LevelImage(image,black_color->black,
            white_color->black,1.0,exception);
          (void) SetImageChannelMask(image,channel_mask);
        }
      if (((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0) &&
          (image->alpha_trait != UndefinedPixelTrait))
        {
          channel_mask=SetImageChannelMask(image,AlphaChannel);
          status&=(MagickStatusType) LevelImage(image,black_color->alpha,
            white_color->alpha,1.0,exception);
          (void) SetImageChannelMask(image,channel_mask);
        }
    }
  else
    {
      if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
        {
          channel_mask=SetImageChannelMask(image,RedChannel);
          status&=(MagickStatusType) LevelizeImage(image,black_color->red,
            white_color->red,1.0,exception);
          (void) SetImageChannelMask(image,channel_mask);
        }
      if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
        {
          channel_mask=SetImageChannelMask(image,GreenChannel);
          status&=(MagickStatusType) LevelizeImage(image,black_color->green,
            white_color->green,1.0,exception);
          (void) SetImageChannelMask(image,channel_mask);
        }
      if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
        {
          channel_mask=SetImageChannelMask(image,BlueChannel);
          status&=(MagickStatusType) LevelizeImage(image,black_color->blue,
            white_color->blue,1.0,exception);
          (void) SetImageChannelMask(image,channel_mask);
        }
      if (((GetPixelBlackTraits(image) & UpdatePixelTrait) != 0) &&
          (image->colorspace == CMYKColorspace))
        {
          channel_mask=SetImageChannelMask(image,BlackChannel);
          status&=(MagickStatusType) LevelizeImage(image,black_color->black,
            white_color->black,1.0,exception);
          (void) SetImageChannelMask(image,channel_mask);
        }
      if (((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0) &&
          (image->alpha_trait != UndefinedPixelTrait))
        {
          channel_mask=SetImageChannelMask(image,AlphaChannel);
          status&=(MagickStatusType) LevelizeImage(image,black_color->alpha,
            white_color->alpha,1.0,exception);
          (void) SetImageChannelMask(image,channel_mask);
        }
    }
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     N e g a t e I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NegateImage() negates the colors in the reference image.  The grayscale
%  option means that only grayscale values within the image are negated.
%
%  The format of the NegateImage method is:
%
%      MagickBooleanType NegateImage(Image *image,
%        const MagickBooleanType grayscale,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o grayscale: If MagickTrue, only negate grayscale pixels within the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void NegatePixelChannels(const Image *image,Quantum *q)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,j);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    q[j]=QuantumRange-q[j];
  }
}

static void NegateGrayImageRow(const Image *image,Quantum *q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (IsPixelGray(image,q) == MagickFalse)
      {
        q+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    NegatePixelChannels(image,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void NegateImageRow(const Image *image,Quantum *q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    NegatePixelChannels(image,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static inline MagickBooleanType IsNonGrayColormapEntry(const Image *image,
  const ssize_t i)
{
  return(((image->colormap[i].red != image->colormap[i].green) ||
    (image->colormap[i].green != image->colormap[i].blue)) ? MagickTrue :
    MagickFalse);
}

static void NegateColormap(Image *image,const MagickBooleanType grayscale)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) image->colors; i++)
  {
    /*
      Negate colormap.
    */
    if (grayscale != MagickFalse)
      if (IsNonGrayColormapEntry(image,i) != MagickFalse)
        continue;
    if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].red=(double) QuantumRange-image->colormap[i].red;
    if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].green=(double) QuantumRange-image->colormap[i].green;
    if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].blue=(double) QuantumRange-image->colormap[i].blue;
  }
}

MagickExport MagickBooleanType NegateImage(Image *image,
  const MagickBooleanType grayscale,ExceptionInfo *exception)
{
#define NegateImageTag  "Negate/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->storage_class == PseudoClass)
    NegateColormap(image,grayscale);
  /*
    Negate image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
  if( grayscale != MagickFalse )
    {
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        MagickBooleanType
          proceed,
          sync;

        Quantum
          *magick_restrict q;

        if (status == MagickFalse)
          continue;
        q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
          exception);
        if (q == (Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        NegateGrayImageRow(image,q);
        sync=SyncCacheViewAuthenticPixels(image_view,exception);
        if (sync == MagickFalse)
          status=MagickFalse;
        if (image->progress_monitor == (MagickProgressMonitor) NULL)
          continue;
        progress++;
        proceed=SetImageProgress(image,NegateImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
      image_view=DestroyCacheView(image_view);
      return(MagickTrue);
    }
  /*
    Negate image.
  */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    NegateImageRow(image,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,NegateImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}
