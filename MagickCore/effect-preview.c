/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  EEEEE  FFFFF  FFFFF  EEEEE   CCCC  TTTTT                   %
%                  E      F      F      E      C        T                     %
%                  EEE    FFF    FFF    EEE    C        T                     %
%                  E      F      F      E      C        T                     %
%                  EEEEE  F      F      EEEEE   CCCC    T                     %
%                                                                             %
%               PPPP   RRRR   EEEEE  V   V  IIIII  EEEEE  W   W               %
%               P   P  R   R  E      V   V    I    E      W   W               %
%               PPPP   RRRR   EEE    V   V    I    EEE    W W W               %
%               P      R R    E       V V     I    E      WW WW               %
%               P      R  R   EEEEE    V    IIIII  EEEEE  W   W               %
%                                                                             %
%                                                                             %
%                      MagickCore Image Preview Methods                       %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                 October 1996                                %
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
#include "MagickCore/blob.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/effect.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/matrix.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/morphology.h"
#include "MagickCore/morphology-private.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/shear.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/threshold.h"
#include "MagickCore/utility-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     P r e v i e w I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PreviewImage() tiles 9 thumbnails of the specified image with an image
%  processing operation applied with varying parameters.  This may be helpful
%  pin-pointing an appropriate parameter for a particular image processing
%  operation.
%
%  The format of the PreviewImages method is:
%
%      Image *PreviewImages(const Image *image,const PreviewType preview,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o preview: the image processing operation.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void FormatQualityLabel(char *label,const char *factor,
  const Image *preview_image,const Image *thumbnail)
{
  if ((GetBlobSize(preview_image)/1024) >= 1024)
    (void) FormatLocaleString(label,MagickPathExtent,"quality %s\n%gmb ",
      factor,(double) ((MagickOffsetType) GetBlobSize(preview_image))/
      1024.0/1024.0);
  else
    if (GetBlobSize(preview_image) >= 1024)
      (void) FormatLocaleString(label,MagickPathExtent,
        "quality %s\n%gkb ",factor,(double) ((MagickOffsetType)
        GetBlobSize(preview_image))/1024.0);
    else
      (void) FormatLocaleString(label,MagickPathExtent,
        "quality %s\n%.17gb ",factor,(double) ((MagickOffsetType)
        GetBlobSize(thumbnail)));
}

static Image *WriteAndReadQualityPreview(ImageInfo *preview_info,
  Image *preview_image,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=WriteImage(preview_info,preview_image,exception);
  if (status != MagickFalse)
    {
      Image
        *quality_image;

      (void) CopyMagickString(preview_info->filename,
        preview_image->filename,MagickPathExtent);
      quality_image=ReadImage(preview_info,exception);
      if (quality_image != (Image *) NULL)
        {
          preview_image=DestroyImage(preview_image);
          preview_image=quality_image;
        }
    }
  return(preview_image);
}

static Image *DespeckleThumbnail(Image *thumbnail,const ssize_t i,
  ExceptionInfo *exception)
{
  Image
    *preview_image;

  ssize_t
    x;

  for (x=0; x < (i-1); x++)
  {
    preview_image=DespeckleImage(thumbnail,exception);
    if (preview_image == (Image *) NULL)
      break;
    thumbnail=DestroyImage(thumbnail);
    thumbnail=preview_image;
  }
  return(thumbnail);
}

static void SharpenContrastRepeatedly(Image *preview_image,const ssize_t i,
  ExceptionInfo *exception)
{
  ssize_t
    x;

  for (x=0; x < i; x++)
    (void) ContrastImage(preview_image,MagickTrue,exception);
}

static void SelectNoiseFactor(const ssize_t i,Image *thumbnail,char *factor)
{
  switch ((int) i)
  {
    case 0:
    {
      (void) CopyMagickString(factor,"uniform",MagickPathExtent);
      break;
    }
    case 1:
    {
      (void) CopyMagickString(factor,"gaussian",MagickPathExtent);
      break;
    }
    case 2:
    {
      (void) CopyMagickString(factor,"multiplicative",MagickPathExtent);
      break;
    }
    case 3:
    {
      (void) CopyMagickString(factor,"impulse",MagickPathExtent);
      break;
    }
    case 5:
    {
      (void) CopyMagickString(factor,"laplacian",MagickPathExtent);
      break;
    }
    case 6:
    {
      (void) CopyMagickString(factor,"Poisson",MagickPathExtent);
      break;
    }
    default:
    {
      (void) CopyMagickString(thumbnail->magick,"NULL",MagickPathExtent);
      break;
    }
  }
}

static void RelinquishMontageDirectory(const Image *image,
  Image *montage_image)
{
  if (montage_image->montage != (char *) NULL)
    {
      /*
        Free image directory.
      */
      montage_image->montage=(char *) RelinquishMagickMemory(
        montage_image->montage);
      if (image->directory != (char *) NULL)
        montage_image->directory=(char *) RelinquishMagickMemory(
          montage_image->directory);
    }
}

typedef struct _PreviewTile
{
  const Image
    *image;

  Image
    *thumbnail;

  ExceptionInfo
    *exception;

  char
    *factor;

  char
    *label;

  ImageInfo
    *preview_info;

  PreviewType
    preview;

  ssize_t
    i;

  double
    percentage;

  double
    radius;

  double
    sigma;
} PreviewTile;

static void ApplyGammaHueSaturationPreview(const PreviewTile *tile,
  Image **preview_image,double *gamma)
{
  Image
    *const thumbnail = tile->thumbnail;

  ExceptionInfo
    *const exception = tile->exception;

  char
    *const factor = tile->factor,
    *const label = tile->label;

  const PreviewType
    preview = tile->preview;

  const double
    percentage = tile->percentage;

  switch (preview)
  {
    case GammaPreview:
    default:
    {
      (*preview_image)=CloneImage(thumbnail,0,0,MagickTrue,exception);
      if ((*preview_image) == (Image *) NULL)
        break;
      (*gamma)+=0.4;
      (void) GammaImage((*preview_image),(*gamma),exception);
      (void) FormatLocaleString(label,MagickPathExtent,"gamma %g",(*gamma));
      break;
    }
    case HuePreview:
    {
      (*preview_image)=CloneImage(thumbnail,0,0,MagickTrue,exception);
      if ((*preview_image) == (Image *) NULL)
        break;
      (void) FormatLocaleString(factor,MagickPathExtent,"100,100,%g",2.0*
        percentage);
      (void) ModulateImage((*preview_image),factor,exception);
      (void) FormatLocaleString(label,MagickPathExtent,"modulate %s",factor);
      break;
    }
    case SaturationPreview:
    {
      (*preview_image)=CloneImage(thumbnail,0,0,MagickTrue,exception);
      if ((*preview_image) == (Image *) NULL)
        break;
      (void) FormatLocaleString(factor,MagickPathExtent,"100,%g",2.0*
        percentage);
      (void) ModulateImage((*preview_image),factor,exception);
      (void) FormatLocaleString(label,MagickPathExtent,"modulate %s",factor);
      break;
    }
  }
}

static void ApplyBrightnessSpiffThresholdPreview(const PreviewTile *tile,
  Image **preview_image,double *gamma)
{
  Image
    *const thumbnail = tile->thumbnail;

  ExceptionInfo
    *const exception = tile->exception;

  char
    *const factor = tile->factor,
    *const label = tile->label;

  const PreviewType
    preview = tile->preview;

  const ssize_t
    i = tile->i;

  const double
    percentage = tile->percentage;

  switch (preview)
  {
    case BrightnessPreview:
    {
      (*preview_image)=CloneImage(thumbnail,0,0,MagickTrue,exception);
      if ((*preview_image) == (Image *) NULL)
        break;
      (void) FormatLocaleString(factor,MagickPathExtent,"%g",2.0*percentage);
      (void) ModulateImage((*preview_image),factor,exception);
      (void) FormatLocaleString(label,MagickPathExtent,"modulate %s",factor);
      break;
    }
    case SpiffPreview:
    {
      (*preview_image)=CloneImage(thumbnail,0,0,MagickTrue,exception);
      if ((*preview_image) != (Image *) NULL)
        SharpenContrastRepeatedly((*preview_image),i,exception);
      (void) FormatLocaleString(label,MagickPathExtent,"contrast (%.17g)",
        (double) i+1);
      break;
    }
    case ThresholdPreview:
    {
      (*preview_image)=CloneImage(thumbnail,0,0,MagickTrue,exception);
      if ((*preview_image) == (Image *) NULL)
        break;
      (void) BilevelImage(thumbnail,(double) (percentage*((double)
        QuantumRange+1.0))/100.0,exception);
      (void) FormatLocaleString(label,MagickPathExtent,"threshold %g",
        (double) (percentage*((double) QuantumRange+1.0))/100.0);
      break;
    }
    default:
    {
      ApplyGammaHueSaturationPreview(tile,&(*preview_image),&(*gamma));
      break;
    }
  }
}

static void ApplyDullSolarizeEdgePreview(const PreviewTile *tile,
  Image **preview_image,double *gamma)
{
  Image
    *const thumbnail = tile->thumbnail;

  ExceptionInfo
    *const exception = tile->exception;

  char
    *const label = tile->label;

  const PreviewType
    preview = tile->preview;

  const ssize_t
    i = tile->i;

  const double
    percentage = tile->percentage,
    radius = tile->radius;

  ssize_t
    x;

  switch (preview)
  {
    case DullPreview:
    {
      (*preview_image)=CloneImage(thumbnail,0,0,MagickTrue,exception);
      if ((*preview_image) == (Image *) NULL)
        break;
      for (x=0; x < i; x++)
        (void) ContrastImage((*preview_image),MagickFalse,exception);
      (void) FormatLocaleString(label,MagickPathExtent,"+contrast (%.17g)",
        (double) i+1);
      break;
    }
    case SolarizePreview:
    {
      (*preview_image)=CloneImage(thumbnail,0,0,MagickTrue,exception);
      if ((*preview_image) == (Image *) NULL)
        break;
      (void) SolarizeImage((*preview_image),(double) QuantumRange*percentage/
        100.0,exception);
      (void) FormatLocaleString(label,MagickPathExtent,"solarize %g",
        ((double) QuantumRange*percentage)/100.0);
      break;
    }
    case EdgeDetectPreview:
    {
      (*preview_image)=EdgeImage(thumbnail,radius,exception);
      (void) FormatLocaleString(label,MagickPathExtent,"edge %g",radius);
      break;
    }
    default:
    {
      ApplyBrightnessSpiffThresholdPreview(tile,&(*preview_image),&(*gamma));
      break;
    }
  }
}

static void WriteJpegQualityPreview(ImageInfo *preview_info,
  Image **preview_image,ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent];

  int
    file;

  file=AcquireUniqueFileResource(filename);
  if (file != -1)
    file=close_utf8(file)-1;
  (void) FormatLocaleString((*preview_image)->filename,MagickPathExtent,
    "jpeg:%s",filename);
  (*preview_image)=WriteAndReadQualityPreview(preview_info,(*preview_image),
    exception);
  (void) RelinquishUniqueFileResource((*preview_image)->filename);
}

static void ApplyRaiseJpegPreview(const PreviewTile *tile,Image **preview_image,
  double *gamma)
{
  Image
    *const thumbnail = tile->thumbnail;

  ExceptionInfo
    *const exception = tile->exception;

  char
    *const factor = tile->factor,
    *const label = tile->label;

  ImageInfo
    *const preview_info = tile->preview_info;

  const PreviewType
    preview = tile->preview;

  const ssize_t
    i = tile->i;

  const double
    percentage = tile->percentage;

  switch (preview)
  {
    case RaisePreview:
    {
      RectangleInfo
        raise;

      (*preview_image)=CloneImage(thumbnail,0,0,MagickTrue,exception);
      if ((*preview_image) == (Image *) NULL)
        break;
      raise.width=(size_t) (2*i+2);
      raise.height=(size_t) (2*i+2);
      raise.x=(i-1)/2;
      raise.y=(i-1)/2;
      (void) RaiseImage((*preview_image),&raise,MagickTrue,exception);
      (void) FormatLocaleString(label,MagickPathExtent,
        "raise %.17gx%.17g%+.20g%+.20g",(double) raise.width,(double)
        raise.height,(double) raise.x,(double) raise.y);
      break;
    }
    case JPEGPreview:
    {
      (*preview_image)=CloneImage(thumbnail,0,0,MagickTrue,exception);
      if ((*preview_image) == (Image *) NULL)
        break;
      preview_info->quality=(size_t) percentage;
      (void) FormatLocaleString(factor,MagickPathExtent,"%.17g",(double)
        preview_info->quality);
      WriteJpegQualityPreview(preview_info,preview_image,exception);
      FormatQualityLabel(label,factor,(*preview_image),thumbnail);
      break;
    }
    default:
    {
      ApplyDullSolarizeEdgePreview(tile,&(*preview_image),&(*gamma));
      break;
    }
  }
}

static void ApplyOilPaintNoiseSharpenBlurPreview(const PreviewTile *tile,
  Image **preview_image,double *gamma)
{
  Image
    *const thumbnail = tile->thumbnail;

  ExceptionInfo
    *const exception = tile->exception;

  char
    *const factor = tile->factor,
    *const label = tile->label;

  const PreviewType
    preview = tile->preview;

  const ssize_t
    i = tile->i;

  const double
    radius = tile->radius,
    sigma = tile->sigma;

  switch (preview)
  {
    case OilPaintPreview:
    {
      (*preview_image)=OilPaintImage(thumbnail,(double) radius,(double) sigma,
        exception);
      (void) FormatLocaleString(label,MagickPathExtent,"charcoal %gx%g",
        radius,sigma);
      break;
    }
    case ReduceNoisePreview:
    {
      (*preview_image)=StatisticImage(thumbnail,NonpeakStatistic,(size_t)
        radius,(size_t) radius,exception);
      (void) FormatLocaleString(label,MagickPathExtent,"noise %g",radius);
      break;
    }
    case AddNoisePreview:
    {
      SelectNoiseFactor(i,thumbnail,factor);
      (*preview_image)=StatisticImage(thumbnail,NonpeakStatistic,(size_t) i,
        (size_t) i,exception);
      (void) FormatLocaleString(label,MagickPathExtent,"+noise %s",factor);
      break;
    }
    case SharpenPreview:
    {
      (*preview_image)=SharpenImage(thumbnail,radius,sigma,exception);
      (void) FormatLocaleString(label,MagickPathExtent,"sharpen %gx%g",
        radius,sigma);
      break;
    }
    case BlurPreview:
    {
      (*preview_image)=BlurImage(thumbnail,radius,sigma,exception);
      (void) FormatLocaleString(label,MagickPathExtent,"blur %gx%g",radius,
        sigma);
      break;
    }
    default:
    {
      ApplyRaiseJpegPreview(tile,&(*preview_image),&(*gamma));
      break;
    }
  }
}

static void ApplySpreadCharcoalPreview(const PreviewTile *tile,
  Image **preview_image,double *gamma)
{
  const Image
    *const image = tile->image;

  Image
    *const thumbnail = tile->thumbnail;

  ExceptionInfo
    *const exception = tile->exception;

  char
    *const label = tile->label;

  const PreviewType
    preview = tile->preview;

  const double
    radius = tile->radius,
    sigma = tile->sigma;

  switch (preview)
  {
    case SpreadPreview:
    {
      (*preview_image)=SpreadImage(thumbnail,image->interpolate,radius,
        exception);
      (void) FormatLocaleString(label,MagickPathExtent,"spread %g",
        radius+0.5);
      break;
    }
    case CharcoalDrawingPreview:
    {
      (*preview_image)=CharcoalImage(thumbnail,(double) radius,(double) sigma,
        exception);
      (void) FormatLocaleString(label,MagickPathExtent,"charcoal %gx%g",
        radius,sigma);
      break;
    }
    default:
    {
      ApplyOilPaintNoiseSharpenBlurPreview(tile,&(*preview_image),&(*gamma));
      break;
    }
  }
}

static void ApplyAnglePreview(const PreviewTile *tile,Image **preview_image,
  double *degrees,double *gamma)
{
  const Image
    *const image = tile->image;

  Image
    *const thumbnail = tile->thumbnail;

  ExceptionInfo
    *const exception = tile->exception;

  char
    *const label = tile->label;

  const PreviewType
    preview = tile->preview;

  switch (preview)
  {
    case RotatePreview:
    {
      (*degrees)+=45.0;
      (*preview_image)=RotateImage(thumbnail,(*degrees),exception);
      (void) FormatLocaleString(label,MagickPathExtent,"rotate %g",(*degrees));
      break;
    }
    case ShearPreview:
    {
      (*degrees)+=5.0;
      (*preview_image)=ShearImage(thumbnail,(*degrees),(*degrees),exception);
      (void) FormatLocaleString(label,MagickPathExtent,"shear %gx%g",(*degrees),
        2.0*(*degrees));
      break;
    }
    case ShadePreview:
    {
      (*degrees)+=10.0;
      (*preview_image)=ShadeImage(thumbnail,MagickTrue,(*degrees),(*degrees),
        exception);
      (void) FormatLocaleString(label,MagickPathExtent,"shade %gx%g",(*degrees),
        (*degrees));
      break;
    }
    case SwirlPreview:
    {
      (*preview_image)=SwirlImage(thumbnail,(*degrees),image->interpolate,
        exception);
      (void) FormatLocaleString(label,MagickPathExtent,"swirl %g",(*degrees));
      (*degrees)+=45.0;
      break;
    }
    case ImplodePreview:
    {
      (*degrees)+=0.1;
      (*preview_image)=ImplodeImage(thumbnail,(*degrees),image->interpolate,
        exception);
      (void) FormatLocaleString(label,MagickPathExtent,"implode %g",(*degrees));
      break;
    }
    default:
    {
      ApplySpreadCharcoalPreview(tile,&(*preview_image),&(*gamma));
      break;
    }
  }
}

static void ApplyWavePreview(const PreviewTile *tile,Image **preview_image,
  double *degrees,double *gamma)
{
  const Image
    *const image = tile->image;

  Image
    *const thumbnail = tile->thumbnail;

  ExceptionInfo
    *const exception = tile->exception;

  char
    *const label = tile->label;

  const PreviewType
    preview = tile->preview;

  switch (preview)
  {
    case WavePreview:
    {
      (*degrees)+=5.0;
      (*preview_image)=WaveImage(thumbnail,0.5*(*degrees),2.0*(*degrees),
        image->interpolate,exception);
      (void) FormatLocaleString(label,MagickPathExtent,"wave %gx%g",0.5*
        (*degrees),2.0*(*degrees));
      break;
    }
    default:
    {
      ApplyAnglePreview(tile,&(*preview_image),&(*degrees),&(*gamma));
      break;
    }
  }
}

#define NumberTiles  9
#define PreviewImageTag  "Preview/Image"
#define DefaultPreviewGeometry  "204x204+10+10"

typedef struct _PreviewTargets
{
  Image
    **preview_image;

  double
    *degrees,
    *gamma;

  size_t
    *colors;

  QuantizeInfo
    *quantize_info;
} PreviewTargets;

static void ApplyRollQuantizePreview(const PreviewTile *tile,
  const PreviewTargets *targets)
{
  Image
    *const thumbnail = tile->thumbnail;

  ExceptionInfo
    *const exception = tile->exception;

  char
    *const label = tile->label;

  const PreviewType
    preview = tile->preview;

  const ssize_t
    i = tile->i;

  Image
    **const preview_image = targets->preview_image;

  double
    *const degrees = targets->degrees,
    *const gamma = targets->gamma;

  size_t
    *const colors = targets->colors;

  QuantizeInfo
    *const quantize_info = targets->quantize_info;

  ssize_t
    x = 0,
    y = 0;

  switch (preview)
  {
    case RollPreview:
    {
      x=((i+1)*(ssize_t) thumbnail->columns)/NumberTiles;
      y=((i+1)*(ssize_t) thumbnail->rows)/NumberTiles;
      (*preview_image)=RollImage(thumbnail,x,y,exception);
      (void) FormatLocaleString(label,MagickPathExtent,"roll %+.20gx%+.20g",
        (double) x,(double) y);
      break;
    }
    default:
    {
      ApplyWavePreview(tile,&(*preview_image),&(*degrees),&(*gamma));
      break;
    }
    case GrayscalePreview:
    {
      (*preview_image)=CloneImage(thumbnail,0,0,MagickTrue,exception);
      if ((*preview_image) == (Image *) NULL)
        break;
      (*colors)<<=1;
      (*quantize_info).number_colors=(*colors);
      (*quantize_info).colorspace=GRAYColorspace;
      (void) QuantizeImage(&(*quantize_info),(*preview_image),exception);
      (void) FormatLocaleString(label,MagickPathExtent,
        "-colorspace gray -colors %.17g",(double) (*colors));
      break;
    }
    case QuantizePreview:
    {
      (*preview_image)=CloneImage(thumbnail,0,0,MagickTrue,exception);
      if ((*preview_image) == (Image *) NULL)
        break;
      (*colors)<<=1;
      (*quantize_info).number_colors=(*colors);
      (void) QuantizeImage(&(*quantize_info),(*preview_image),exception);
      (void) FormatLocaleString(label,MagickPathExtent,"colors %.17g",
        (double) (*colors));
      break;
    }
  }
}

typedef struct _PreviewSelection
{
  PreviewType
    preview;

  ssize_t
    i;

  char
    *label;

  ExceptionInfo
    *exception;

  PreviewTile
    tile;

  PreviewTargets
    targets;
} PreviewSelection;

static void ApplyDespeckleSegmentPreview(const PreviewSelection *selection,
  Image **thumbnail,Image **preview_image,double *threshold)
{
  const PreviewType
    preview = selection->preview;

  const ssize_t
    i = selection->i;

  char
    *const label = selection->label;

  ExceptionInfo
    *const exception = selection->exception;

  const PreviewTile
    tile = selection->tile;

  const PreviewTargets
    targets = selection->targets;

  switch (preview)
  {
    default:
    {
      ApplyRollQuantizePreview(&tile,&targets);
      break;
    }
    case DespecklePreview:
    {
      (*thumbnail)=DespeckleThumbnail((*thumbnail),i,exception);
      (*preview_image)=DespeckleImage((*thumbnail),exception);
      if ((*preview_image) == (Image *) NULL)
        break;
      (void) FormatLocaleString(label,MagickPathExtent,"despeckle (%.17g)",
        (double) i+1);
      break;
    }
    case SegmentPreview:
    {
      (*preview_image)=CloneImage((*thumbnail),0,0,MagickTrue,exception);
      if ((*preview_image) == (Image *) NULL)
        break;
      (*threshold)+=0.4;
      (void) SegmentImage((*preview_image),sRGBColorspace,MagickFalse,
        (*threshold),(*threshold),exception);
      (void) FormatLocaleString(label,MagickPathExtent,"segment %gx%g",
        (*threshold),(*threshold));
      break;
    }
  }
}

static Image *MontagePreviewTiles(const Image *image,ImageInfo *preview_info,
  Image *images,ExceptionInfo *exception)
{
  Image
    *montage_image;

  MontageInfo
    *montage_info;

  montage_info=CloneMontageInfo(preview_info,(MontageInfo *) NULL);
  (void) CopyMagickString(montage_info->filename,image->filename,
    MagickPathExtent);
  montage_info->shadow=MagickTrue;
  (void) CloneString(&montage_info->tile,"3x3");
  (void) CloneString(&montage_info->geometry,DefaultPreviewGeometry);
  (void) CloneString(&montage_info->frame,DefaultTileFrame);
  montage_image=MontageImages(images,montage_info,exception);
  montage_info=DestroyMontageInfo(montage_info);
  return(montage_image);
}

static MagickBooleanType AcquirePreviewThumbnail(const Image *image,
  const RectangleInfo geometry,ExceptionInfo *exception,Image **thumbnail)
{
  (*thumbnail)=ThumbnailImage(image,geometry.width,geometry.height,exception);
  if ((*thumbnail) == (Image *) NULL)
    return(MagickFalse);
  (void) SetImageProgressMonitor((*thumbnail),(MagickProgressMonitor) NULL,
    (void *) NULL);
  (void) SetImageProperty((*thumbnail),"label",DefaultTileLabel,exception);
  return(MagickTrue);
}

static MagickBooleanType AppendMiddlePreviewTile(const ssize_t i,
  Image *thumbnail,ExceptionInfo *exception,Image **images)
{
  if (i == (NumberTiles/2))
    {
      (void) QueryColorCompliance("#dfdfdf",AllCompliance,
        &thumbnail->matte_color,exception);
      AppendImageToList(&(*images),thumbnail);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static Image *AppendPreviewTile(Image *images,Image *preview_image,
  char *label,ExceptionInfo *exception)
{
  preview_image->alpha_trait=UndefinedPixelTrait;
  (void) DeleteImageProperty(preview_image,"label");
  (void) SetImageProperty(preview_image,"label",label,exception);
  AppendImageToList(&images,preview_image);
  return(images);
}

typedef struct _PreviewTileInput
{
  const Image
    *image;

  Image
    *thumbnail;

  ExceptionInfo
    *exception;

  char
    *factor,
    *label;

  ImageInfo
    *preview_info;

  PreviewType
    preview;

  ssize_t
    i;

  PreviewTargets
    targets;
} PreviewTileInput;

typedef struct _PreviewTileTargets
{
  Image
    **preview_image;

  double
    *threshold,
    *percentage,
    *radius,
    *sigma;

  Image
    **images;
} PreviewTileTargets;

static MagickBooleanType RenderPreviewTile(const PreviewTileInput *input,
  const PreviewTileTargets *tile_targets)
{
  Image
    *thumbnail = input->thumbnail;

  Image
    **const preview_image = tile_targets->preview_image,
    **const images = tile_targets->images;

  double
    *const threshold = tile_targets->threshold,
    *const percentage = tile_targets->percentage,
    *const radius = tile_targets->radius,
    *const sigma = tile_targets->sigma;

  MagickBooleanType
    proceed;

  PreviewSelection
    selection;

  PreviewTile
    tile;

  tile.image=input->image;
  tile.thumbnail=thumbnail;
  tile.exception=input->exception;
  tile.factor=input->factor;
  tile.label=input->label;
  tile.preview_info=input->preview_info;
  tile.preview=input->preview;
  tile.i=input->i;
  tile.percentage=(*percentage);
  tile.radius=(*radius);
  tile.sigma=(*sigma);
  selection.preview=input->preview;
  selection.i=input->i;
  selection.label=input->label;
  selection.exception=input->exception;
  selection.tile=tile;
  selection.targets=input->targets;
  ApplyDespeckleSegmentPreview(&selection,&thumbnail,&(*preview_image),
    &(*threshold));
  thumbnail=DestroyImage(thumbnail);
  (*percentage)+=12.5;
  (*radius)+=0.5;
  (*sigma)+=0.25;
  if ((*preview_image) == (Image *) NULL)
    return(MagickFalse);
  (*images)=AppendPreviewTile((*images),(*preview_image),input->label,
    input->exception);
  proceed=SetImageProgress(input->image,PreviewImageTag,(MagickOffsetType)
    input->i,NumberTiles);
  if (proceed == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _PreviewSetup
{
  const Image
    *image;

  PreviewType
    preview;

  ImageInfo
    *preview_info;

  RectangleInfo
    geometry;

  ExceptionInfo
    *exception;

  size_t
    colors;

  double
    degrees,
    gamma;
} PreviewSetup;

static Image *AcquirePreviewTiles(const PreviewSetup *setup)
{
  const Image
    *const image = setup->image;

  const PreviewType
    preview = setup->preview;

  ImageInfo
    *const preview_info = setup->preview_info;

  const RectangleInfo
    geometry = setup->geometry;

  ExceptionInfo
    *const exception = setup->exception;

  char
    factor[MagickPathExtent],
    label[MagickPathExtent];

  double
    degrees = setup->degrees,
    gamma = setup->gamma,
    percentage,
    radius,
    sigma,
    threshold;

  Image
    *images,
    *preview_image,
    *thumbnail;

  QuantizeInfo
    quantize_info;

  size_t
    colors = setup->colors;

  ssize_t
    i;

  const PreviewTargets
    targets = { &preview_image, &degrees, &gamma, &colors, &quantize_info };

  const PreviewTileTargets
    tile_targets = { &preview_image, &threshold, &percentage, &radius, &sigma,
      &images };

  images=NewImageList();
  percentage=12.5;
  GetQuantizeInfo(&quantize_info);
  radius=0.0;
  sigma=1.0;
  threshold=0.0;
  for (i=0; i < NumberTiles; i++)
  {
    PreviewTileInput
      input;

    if (AcquirePreviewThumbnail(image,geometry,exception,&thumbnail) ==
        MagickFalse)
      break;
    if (AppendMiddlePreviewTile(i,thumbnail,exception,&images) == MagickFalse)
      continue;
    input.image=image;
    input.thumbnail=thumbnail;
    input.exception=exception;
    input.factor=factor;
    input.label=label;
    input.preview_info=preview_info;
    input.preview=preview;
    input.i=i;
    input.targets=targets;
    if (RenderPreviewTile(&input,&tile_targets) == MagickFalse)
      break;
  }
  return(images);
}

MagickExport Image *PreviewImage(const Image *image,const PreviewType preview,
  ExceptionInfo *exception)
{
  double
    degrees,
    gamma;

  Image
    *images,
    *montage_image;

  ImageInfo
    *preview_info;

  PreviewSetup
    setup;

  RectangleInfo
    geometry;

  size_t
    colors;

  /*
    Open output image file.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  colors=2;
  degrees=0.0;
  gamma=(-0.2f);
  preview_info=AcquireImageInfo();
  SetGeometry(image,&geometry);
  (void) ParseMetaGeometry(DefaultPreviewGeometry,&geometry.x,&geometry.y,
    &geometry.width,&geometry.height);
  setup.image=image;
  setup.preview=preview;
  setup.preview_info=preview_info;
  setup.geometry=geometry;
  setup.exception=exception;
  setup.colors=colors;
  setup.degrees=degrees;
  setup.gamma=gamma;
  images=AcquirePreviewTiles(&setup);
  if (images == (Image *) NULL)
    {
      preview_info=DestroyImageInfo(preview_info);
      return((Image *) NULL);
    }
  /*
    Create the montage.
  */
  montage_image=MontagePreviewTiles(image,preview_info,images,exception);
  images=DestroyImageList(images);
  if (montage_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  RelinquishMontageDirectory(image,montage_image);
  preview_info=DestroyImageInfo(preview_info);
  return(montage_image);
}
