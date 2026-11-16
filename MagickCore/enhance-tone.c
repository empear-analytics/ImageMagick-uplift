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
#include "MagickCore/enhance-private.h"

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

static void SetClutColorspace(Image *image,const Image *clut_image,
  ExceptionInfo *exception)
{
  if (IsGrayImageWithColorClut(image,clut_image) != MagickFalse)
    (void) SetImageColorspace(image,sRGBColorspace,exception);
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
  SetClutColorspace(image,clut_image,exception);
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

    const ToneLine
      line = { image,image_view,exception };

    if (GetToneRow(&line,y,&q,&status) == MagickFalse)
      continue;
    ClutImageRow(image,clut_map,q);
    SyncToneRow(&line,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetToneProgress(image,ClutImageTag,progress,&status);
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

static MagickBooleanType LoadColorCorrection(
  const char *color_correction_collection,ColorCorrection *color_correction,
  ExceptionInfo *exception)
{
  XMLTreeInfo
    *cc,
    *ccc;

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
  *color_correction=ParseColorCorrection(cc);
  ccc=DestroyXMLTree(ccc);
  return(MagickTrue);
}

typedef struct _ColorCorrectionTargets
{
  ColorCorrection
    *color_correction;

  PixelInfo
    **cdl_map;
} ColorCorrectionTargets;

static MagickBooleanType PrepareColorCorrection(const Image *image,
  const char *color_correction_collection,
  const ColorCorrectionTargets *targets,ExceptionInfo *exception)
{
  ColorCorrection
    *const color_correction = targets->color_correction;

  PixelInfo
    **const cdl_map = targets->cdl_map;

  if (LoadColorCorrection(color_correction_collection,color_correction,
        exception) == MagickFalse)
    return(MagickFalse);
  LogColorCorrection(image,(*color_correction));
  (*cdl_map)=(PixelInfo *) AcquireQuantumMemory(MaxMap+1UL,
    sizeof(*(*cdl_map)));
  if ((*cdl_map) == (PixelInfo *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  BuildCDLMap((*color_correction),(*cdl_map));
  return(MagickTrue);
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

  ColorCorrectionTargets
    targets;

  ssize_t
    y;

  /*
    Allocate and initialize cdl maps.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  targets.color_correction=(&color_correction);
  targets.cdl_map=(&cdl_map);
  if (PrepareColorCorrection(image,color_correction_collection,&targets,
        exception) == MagickFalse)
    return(MagickFalse);
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

    const ToneLine
      line = { image,image_view,exception };

    if (GetToneRow(&line,y,&q,&status) == MagickFalse)
      continue;
    ApplyCDLToPixels(image,color_correction,cdl_map,q);
    SyncToneRow(&line,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetToneProgress(image,ColorDecisionListCorrectImageTag,progress,
          &status);
      }
  }
  image_view=DestroyCacheView(image_view);
  cdl_map=(PixelInfo *) RelinquishMagickMemory(cdl_map);
  return(status);
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

static MagickBooleanType PrepareHaldClutImage(Image *image,
  const Image *hald_image,ExceptionInfo *exception)
{
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
  if (image->colorspace != hald_image->colorspace)
    (void) SetImageColorspace(image,hald_image->colorspace,exception);
  return(MagickTrue);
}

static double GetHaldArea(const Image *hald_image,const double point)
{
  double
    area;

  area=point;
  if (hald_image->interpolate == NearestInterpolatePixel)
    area=(point < 0.5) ? 0.0 : 1.0;
  return(area);
}

typedef struct _HaldClutRowInfo
{
  Image
    *image;

  const Image
    *hald_image;

  CacheView
    *hald_view;

  ExceptionInfo
    *exception;

  PixelInfo
    zero;

  size_t
    level;

  size_t
    cube_size;

  double
    width;
} HaldClutRowInfo;

static void HaldClutRow(MagickBooleanType *status,
  const HaldClutRowInfo *clut,Quantum *magick_restrict q)
{
  Image
    *const image = clut->image;

  const Image
    *const hald_image = clut->hald_image;

  CacheView
    *const hald_view = clut->hald_view;

  ExceptionInfo
    *const exception = clut->exception;

  const PixelInfo
    zero = clut->zero;

  const size_t
    level = clut->level;

  const size_t
    cube_size = clut->cube_size;

  const double
    width = clut->width;

  ssize_t
    x;

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
    (*status)=InterpolatePixelInfo(hald_image,hald_view,hald_image->interpolate,
      fmod(offset,width),floor(offset/width),&pixel1,exception);
    if ((*status) == MagickFalse)
      break;
    (*status)=InterpolatePixelInfo(hald_image,hald_view,hald_image->interpolate,
      fmod(offset+level,width),floor((offset+level)/width),&pixel2,exception);
    if ((*status) == MagickFalse)
      break;
    area=GetHaldArea(hald_image,point.y);
    CompositePixelInfoAreaBlend(&pixel1,pixel1.alpha,&pixel2,pixel2.alpha,
      area,&pixel3);
    offset+=cube_size;
    (*status)=InterpolatePixelInfo(hald_image,hald_view,hald_image->interpolate,
      fmod(offset,width),floor(offset/width),&pixel1,exception);
    if ((*status) == MagickFalse)
      break;
    (*status)=InterpolatePixelInfo(hald_image,hald_view,hald_image->interpolate,
      fmod(offset+level,width),floor((offset+level)/width),&pixel2,exception);
    if ((*status) == MagickFalse)
      break;
    CompositePixelInfoAreaBlend(&pixel1,pixel1.alpha,&pixel2,pixel2.alpha,
      area,&pixel4);
    area=GetHaldArea(hald_image,point.z);
    CompositePixelInfoAreaBlend(&pixel3,pixel3.alpha,&pixel4,pixel4.alpha,
      area,&pixel);
    SetHaldPixel(image,pixel,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static size_t GetHaldLevel(const Image *hald_image)
{
  size_t
    length,
    level;

  length=(size_t) MagickMin((MagickRealType) hald_image->columns,
    (MagickRealType) hald_image->rows);
  for (level=2; (level*level*level) < length; level++) ;
  level*=level;
  return(level);
}

#define HaldClutImageTag  "Clut/Image"

typedef struct _HaldClutPass
{
  Image
    *image;

  const Image
    *hald_image;

  ExceptionInfo
    *exception;
} HaldClutPass;

static MagickBooleanType HaldClutImagePixels(const HaldClutPass *pass)
{
  Image
    *const image = pass->image;

  const Image
    *const hald_image = pass->hald_image;

  ExceptionInfo
    *const exception = pass->exception;

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
    level;

  ssize_t
    y;

  status=MagickTrue;
  progress=0;
  level=GetHaldLevel(hald_image);
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
    HaldClutRowInfo
      clut;

    Quantum
      *magick_restrict q;

    const ToneLine
      line = { image,image_view,exception };

    if (GetToneRow(&line,y,&q,&status) == MagickFalse)
      continue;
    clut.image=image;
    clut.hald_image=hald_image;
    clut.hald_view=hald_view;
    clut.exception=exception;
    clut.zero=zero;
    clut.level=level;
    clut.cube_size=cube_size;
    clut.width=width;
    HaldClutRow(&status,&clut,q);
    SyncToneRow(&line,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetToneProgress(image,HaldClutImageTag,progress,&status);
      }
  }
  hald_view=DestroyCacheView(hald_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

MagickExport MagickBooleanType HaldClutImage(Image *image,
  const Image *hald_image,ExceptionInfo *exception)
{
  HaldClutPass
    pass;

  MagickBooleanType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(hald_image != (Image *) NULL);
  assert(hald_image->signature == MagickCoreSignature);
  if (PrepareHaldClutImage(image,hald_image,exception) == MagickFalse)
    return(MagickFalse);
  /*
    Hald clut image.
  */
  pass.image=image;
  pass.hald_image=hald_image;
  pass.exception=exception;
  status=HaldClutImagePixels(&pass);
  return(status);
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

#define NegateImageTag  "Negate/Image"

typedef struct _NegateInfo
{
  Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;
} NegateInfo;

static void NegateGrayscaleRow(const NegateInfo *negate,const ssize_t y,
  MagickBooleanType *status,MagickOffsetType *progress)
{
  Image
    *image = negate->image;

  MagickBooleanType
    proceed,
    sync;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return;
  q=GetCacheViewAuthenticPixels(negate->image_view,0,y,image->columns,1,
    negate->exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return;
    }
  NegateGrayImageRow(image,q);
  sync=SyncCacheViewAuthenticPixels(negate->image_view,negate->exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
  if (image->progress_monitor == (MagickProgressMonitor) NULL)
    return;
  (*progress)++;
  proceed=SetImageProgress(image,NegateImageTag,(*progress),image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

static MagickBooleanType NegateGrayscaleImage(const NegateInfo *negate,
  MagickBooleanType status,MagickOffsetType progress)
{
  CacheView
    *image_view = negate->image_view;

  Image
    *const image = negate->image;

  ssize_t
    y;

  for (y=0; y < (ssize_t) image->rows; y++)
    NegateGrayscaleRow(negate,y,&status,&progress);
  image_view=DestroyCacheView(image_view);
  return(MagickTrue);
}

static MagickBooleanType NegateImageLine(const NegateInfo *negate,
  const ssize_t y,MagickBooleanType *status)
{
  Image
    *image = negate->image;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(negate->image_view,0,y,image->columns,1,
    negate->exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  NegateImageRow(image,q);
  if (SyncCacheViewAuthenticPixels(negate->image_view,negate->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

MagickExport MagickBooleanType NegateImage(Image *image,
  const MagickBooleanType grayscale,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  NegateInfo
    negate;

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
  negate.image=image;
  negate.image_view=image_view;
  negate.exception=exception;
  if( grayscale != MagickFalse )
    return(NegateGrayscaleImage(&negate,status,progress));
  /*
    Negate image.
  */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    if (NegateImageLine(&negate,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetToneProgress(image,NegateImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}
