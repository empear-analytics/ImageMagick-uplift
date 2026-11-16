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
%                  FFFFF  IIIII  L      TTTTT  EEEEE  RRRR                    %
%                  F        I    L        T    E      R   R                   %
%                  FFF      I    L        T    EEE    RRRR                    %
%                  F        I    L        T    E      R R                     %
%                  F      IIIII  LLLLL    T    EEEEE  R  R                    %
%                                                                             %
%                                                                             %
%                   MagickCore Image Filter Effects Methods                   %
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
#include "MagickCore/effect-private.h"


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     K u w a h a r a I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  KuwaharaImage() is an edge preserving noise reduction filter.
%
%  The format of the KuwaharaImage method is:
%
%      Image *KuwaharaImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the square window radius.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickRealType GetMeanLuma(const Image *magick_restrict image,
  const double *magick_restrict pixel)
{
  return(0.212656*pixel[image->channel_map[RedPixelChannel].offset]+
    0.715158*pixel[image->channel_map[GreenPixelChannel].offset]+
    0.072186*pixel[image->channel_map[BluePixelChannel].offset]);  /* Rec709 */
}

static void GetKuwaharaMean(const Image *gaussian_image,
  const Quantum *magick_restrict p,const size_t width,double *mean)
{
  const Quantum
    *magick_restrict k;

  ssize_t
    j,
    n;

  for (j=0; j < (ssize_t) GetPixelChannels(gaussian_image); j++)
    mean[j]=0.0;
  k=p;
  for (n=0; n < (ssize_t) (width*width); n++)
  {
    for (j=0; j < (ssize_t) GetPixelChannels(gaussian_image); j++)
      mean[j]+=(double) k[j];
    k+=(ptrdiff_t) GetPixelChannels(gaussian_image);
  }
  for (j=0; j < (ssize_t) GetPixelChannels(gaussian_image); j++)
    mean[j]/=(double) (width*width);
}

static double GetKuwaharaVariance(const Image *gaussian_image,
  const Quantum *magick_restrict p,const size_t width,const double *mean)
{
  const Quantum
    *magick_restrict k;

  double
    variance;

  ssize_t
    n;

  k=p;
  variance=0.0;
  for (n=0; n < (ssize_t) (width*width); n++)
  {
    double
      luma;

    luma=GetPixelLuma(gaussian_image,k);
    variance+=(luma-GetMeanLuma(gaussian_image,mean))*
      (luma-GetMeanLuma(gaussian_image,mean));
    k+=(ptrdiff_t) GetPixelChannels(gaussian_image);
  }
  return(variance);
}

typedef struct _KuwaharaCorner
{
  ssize_t
    x,
    y;

  size_t
    width,
    i;
} KuwaharaCorner;

static void OffsetKuwaharaQuadrant(const KuwaharaCorner *corner,
  RectangleInfo *quadrant)
{
  const ssize_t
    x = corner->x,
    y = corner->y;

  const size_t
    width = corner->width,
    i = corner->i;

  (*quadrant).x=x;
  (*quadrant).y=y;
  switch (i)
  {
    case 0:
    {
      (*quadrant).x=x-(ssize_t) (width-1);
      (*quadrant).y=y-(ssize_t) (width-1);
      break;
    }
    case 1:
    {
      (*quadrant).y=y-(ssize_t) (width-1);
      break;
    }
    case 2:
    {
      (*quadrant).x=x-(ssize_t) (width-1);
      break;
    }
    case 3:
    default:
      break;
  }
}

typedef struct _KuwaharaQuadrants
{
  Image
    *gaussian_image;

  CacheView
    *image_view;

  size_t
    width;

  ssize_t
    x,
    y;

  ExceptionInfo
    *exception;
} KuwaharaQuadrants;

static void SelectKuwaharaQuadrant(const KuwaharaQuadrants *quadrants,
  RectangleInfo *target,size_t *i)
{
  Image
    *const gaussian_image = quadrants->gaussian_image;

  CacheView
    *const image_view = quadrants->image_view;

  const size_t
    width = quadrants->width;

  const ssize_t
    x = quadrants->x,
    y = quadrants->y;

  ExceptionInfo
    *const exception = quadrants->exception;

  const Quantum
    *magick_restrict p;

  double
    min_variance;

  RectangleInfo
    quadrant;

  min_variance=MagickMaximumValue;
  SetGeometry(gaussian_image,target);
  quadrant.width=width;
  quadrant.height=width;
  for (*i=0; *i < 4; (*i)++)
  {
    double
      mean[MaxPixelChannels],
      variance;

    KuwaharaCorner
      corner;

    corner.x=x;
    corner.y=y;
    corner.width=width;
    corner.i=(*i);
    OffsetKuwaharaQuadrant(&corner,&quadrant);
    p=GetCacheViewVirtualPixels(image_view,quadrant.x,quadrant.y,
      quadrant.width,quadrant.height,exception);
    if (p == (const Quantum *) NULL)
      break;
    GetKuwaharaMean(gaussian_image,p,width,mean);
    variance=GetKuwaharaVariance(gaussian_image,p,width,mean);
    if (variance < min_variance)
      {
        min_variance=variance;
        *target=quadrant;
      }
  }
}

typedef struct _KuwaharaInfo
{
  Image
    *gaussian_image,
    *kuwahara_image;

  CacheView
    *image_view,
    *kuwahara_view;

  ExceptionInfo
    *exception;

  size_t
    width;

  ssize_t
    y;
} KuwaharaInfo;

static void KuwaharaRow(MagickBooleanType *status,const KuwaharaInfo *kuwahara,
  Quantum *magick_restrict q)
{
  Image
    *const gaussian_image = kuwahara->gaussian_image,
    *const kuwahara_image = kuwahara->kuwahara_image;

  CacheView
    *const image_view = kuwahara->image_view;

  ExceptionInfo
    *const exception = kuwahara->exception;

  const size_t
    width = kuwahara->width;

  const ssize_t
    y = kuwahara->y;

  ssize_t
    x;

  for (x=0; x < (ssize_t) gaussian_image->columns; x++)
  {
    KuwaharaQuadrants
      quadrants;

    RectangleInfo
      target;

    size_t
      i;

    quadrants.gaussian_image=gaussian_image;
    quadrants.image_view=image_view;
    quadrants.width=width;
    quadrants.x=x;
    quadrants.y=y;
    quadrants.exception=exception;
    SelectKuwaharaQuadrant(&quadrants,&target,&i);
    if (i < 4)
      {
        (*status)=MagickFalse;
        break;
      }
    (*status)=InterpolatePixelChannels(gaussian_image,image_view,kuwahara_image,
      UndefinedInterpolatePixel,(double) target.x+target.width/2.0,(double)
      target.y+target.height/2.0,q,exception);
    if ((*status) == MagickFalse)
      break;
    q+=(ptrdiff_t) GetPixelChannels(kuwahara_image);
  }
}

typedef struct _KuwaharaImageTargets
{
  Image
    **gaussian_image,
    **kuwahara_image;
} KuwaharaImageTargets;

static MagickBooleanType AcquireKuwaharaImage(const Image *image,
  ExceptionInfo *exception,const KuwaharaImageTargets *targets)
{
  Image
    **const gaussian_image = targets->gaussian_image,
    **const kuwahara_image = targets->kuwahara_image;

  (*kuwahara_image)=CloneImage(image,0,0,MagickTrue,exception);
  if ((*kuwahara_image) == (Image *) NULL)
    {
      (*gaussian_image)=DestroyImage((*gaussian_image));
      return(MagickFalse);
    }
  if (SetImageStorageClass((*kuwahara_image),DirectClass,exception) == MagickFalse)
    {
      (*gaussian_image)=DestroyImage((*gaussian_image));
      (*kuwahara_image)=DestroyImage((*kuwahara_image));
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType KuwaharaHeadRow(MagickBooleanType *status,
  const KuwaharaInfo *kuwahara)
{
  CacheView
    *const kuwahara_view = kuwahara->kuwahara_view;

  ExceptionInfo
    *const exception = kuwahara->exception;

  const Image
    *const kuwahara_image = kuwahara->kuwahara_image;

  const ssize_t
    y = kuwahara->y;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=QueueCacheViewAuthenticPixels(kuwahara_view,0,y,kuwahara_image->columns,1,
    exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  KuwaharaRow(status,kuwahara,q);
  return(MagickTrue);
}

static void SyncKuwaharaRow(MagickBooleanType *status,CacheView *kuwahara_view,
  ExceptionInfo *exception)
{
  if (SyncCacheViewAuthenticPixels(kuwahara_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

MagickExport Image *KuwaharaImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
#define KuwaharaImageTag  "Kuwahara/Image"

  CacheView
    *image_view,
    *kuwahara_view;

  Image
    *gaussian_image,
    *kuwahara_image;

  MagickBooleanType
    status;

  KuwaharaImageTargets
    targets;

  MagickOffsetType
    progress;

  size_t
    width;

  ssize_t
    y;

  /*
    Initialize Kuwahara image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  width=(size_t) radius+1;
  gaussian_image=BlurImage(image,radius,sigma,exception);
  if (gaussian_image == (Image *) NULL)
    return((Image *) NULL);
  targets.gaussian_image=(&gaussian_image);
  targets.kuwahara_image=(&kuwahara_image);
  if (AcquireKuwaharaImage(image,exception,&targets) == MagickFalse)
    return((Image *) NULL);
  /*
    Edge preserving noise reduction filter.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(gaussian_image,exception);
  kuwahara_view=AcquireAuthenticCacheView(kuwahara_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,kuwahara_image,gaussian_image->rows,1)
#endif
  for (y=0; y < (ssize_t) gaussian_image->rows; y++)
  {
    const KuwaharaInfo
      kuwahara = { gaussian_image, kuwahara_image, image_view, kuwahara_view,
        exception, width, y };

    if (KuwaharaHeadRow(&status,&kuwahara) == MagickFalse)
      continue;
    SyncKuwaharaRow(&status,kuwahara_view,exception);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportRowProgress(image,KuwaharaImageTag,progress,&status);
      }
  }
  kuwahara_view=DestroyCacheView(kuwahara_view);
  image_view=DestroyCacheView(image_view);
  gaussian_image=DestroyImage(gaussian_image);
  return(ReleaseFailedImage(kuwahara_image,status));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     L o c a l C o n t r a s t I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LocalContrastImage() attempts to increase the appearance of large-scale
%  light-dark transitions. Local contrast enhancement works similarly to
%  sharpening with an unsharp mask, however the mask is instead created using
%  an image with a greater blur distance.
%
%  The format of the LocalContrastImage method is:
%
%      Image *LocalContrastImage(const Image *image, const double radius,
%        const double strength,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the Gaussian blur, in percentage with 100%
%      resulting in a blur radius of 20% of largest dimension.
%
%    o strength: the strength of the blur mask in percentage.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void MirrorIntoPadding(const Image *image,float *out,const ssize_t x,
  const ssize_t width)
{
  if ((x <= width) && (x != 0))
    *(out-(x*2))=*out;
  if ((x > (ssize_t) image->columns-width-2) &&
      (x != (ssize_t) image->columns-1))
    *(out+((image->columns-(size_t) x-1)*2))=*out;
}

static double GetTriangleWeightedSum(const float *pixels,const ssize_t offset,
  const ssize_t width)
{
  const float
    *pix;

  double
    sum,
    weight;

  ssize_t
    i;

  weight=1.0;
  sum=0;
  pix=pixels+offset;
  for (i=0; i < width; i++)
  {
    sum+=weight*((double) *pix++);
    weight+=1.0;
  }
  for (i=width+1; i < (2*width); i++)
  {
    sum+=weight*((double) *pix++);
    weight-=1.0;
  }
  return(sum);
}

typedef struct _LocalContrastPixel
{
  const Image
    *image;

  const Quantum
    *p;

  double
    sum,
    totalWeight,
    strength;

  Image
    *contrast_image;
} LocalContrastPixel;

static void ApplyLocalContrastPixel(const LocalContrastPixel *contrast,
  Quantum *magick_restrict q)
{
  const Image
    *const image = contrast->image;

  const Quantum
    *const p = contrast->p;

  const double
    sum = contrast->sum,
    totalWeight = contrast->totalWeight,
    strength = contrast->strength;

  Image
    *const contrast_image = contrast->contrast_image;

  double
    mult,
    srcVal;

  PixelTrait
    traits;

  srcVal=(float) GetPixelLuma(image,p);
  mult=(srcVal-(sum/totalWeight))*(strength/100.0);
  mult=(srcVal+mult)/srcVal;
  traits=GetPixelChannelTraits(image,RedPixelChannel);
  if ((traits & UpdatePixelTrait) != 0)
    SetPixelRed(contrast_image,ClampToQuantum((MagickRealType)
      GetPixelRed(image,p)*mult),q);
  traits=GetPixelChannelTraits(image,GreenPixelChannel);
  if ((traits & UpdatePixelTrait) != 0)
    SetPixelGreen(contrast_image,ClampToQuantum((MagickRealType)
      GetPixelGreen(image,p)*mult),q);
  traits=GetPixelChannelTraits(image,BluePixelChannel);
  if ((traits & UpdatePixelTrait) != 0)
    SetPixelBlue(contrast_image,ClampToQuantum((MagickRealType)
      GetPixelBlue(image,p)*mult),q);
}

static void LoadLocalContrastLuma(const Image *image,const Quantum *p,
  float *pix,const ssize_t width)
{
  ssize_t
    y;

  for (y=0; y < (ssize_t) image->rows+(2*width); y++)
  {
    *pix++=(float)GetPixelLuma(image,p);
    p+=(ptrdiff_t) image->number_channels;
  }
}

typedef struct _LocalContrastRow
{
  const Image
    *image;

  float
    *pixels;

  ssize_t
    width;

  double
    totalWeight,
    strength;

  Image
    *contrast_image;
} LocalContrastRow;

static void ApplyLocalContrastRow(const LocalContrastRow *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = row->image;

  float
    *const pixels = row->pixels;

  const ssize_t
    width = row->width;

  const double
    totalWeight = row->totalWeight,
    strength = row->strength;

  Image
    *const contrast_image = row->contrast_image;

  LocalContrastPixel
    contrast;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    double
      sum;

    sum=GetTriangleWeightedSum(pixels,x,width);
    /*
      Apply and write.
    */
    contrast.image=image;
    contrast.p=p;
    contrast.sum=sum;
    contrast.totalWeight=totalWeight;
    contrast.strength=strength;
    contrast.contrast_image=contrast_image;
    ApplyLocalContrastPixel(&contrast,q);
    p+=(ptrdiff_t) image->number_channels;
    q+=(ptrdiff_t) contrast_image->number_channels;
  }
}

typedef struct _LocalContrastColumn
{
  const Image
    *image;

  float
    *interImage,
    *pixels;

  ssize_t
    width,
    x;

  double
    totalWeight;
} LocalContrastColumn;

static void StoreLocalContrastColumn(const LocalContrastColumn *column)
{
  const Image
    *const image = column->image;

  float
    *const interImage = column->interImage,
    *const pixels = column->pixels;

  const ssize_t
    width = column->width,
    x = column->x;

  const double
    totalWeight = column->totalWeight;

  float
    *out;

  ssize_t
    y;

  out=interImage+x+width;
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    double
      sum;

    sum=GetTriangleWeightedSum(pixels,y,width);
    /* write to output */
    *out=(float) (sum/totalWeight);
    /* mirror into padding */
    MirrorIntoPadding(image,out,x,width);
    out+=image->columns+(size_t) (width*2);
  }
}

static MagickBooleanType AcquireDirectClassClone(const Image *image,
  Image **clone_image,ExceptionInfo *exception)
{
  (*clone_image)=CloneImage(image,0,0,MagickTrue,exception);
  if ((*clone_image) == (Image *) NULL)
    return(MagickFalse);
  if (SetImageStorageClass((*clone_image),DirectClass,exception) == MagickFalse)
    {
      (*clone_image)=DestroyImage((*clone_image));
      return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _LocalContrastColumnPass
{
  const Image
    *image;

  CacheView
    *image_view;

  float
    *interImage,
    *scanline;

  ssize_t
    scanLineSize,
    width;

  double
    totalWeight;

  ExceptionInfo
    *exception;
} LocalContrastColumnPass;

static void RunLocalContrastColumn(MagickBooleanType *status,
  const LocalContrastColumnPass *pass,const ssize_t x)
{
  const Image
    *const image = pass->image;

  CacheView
    *const image_view = pass->image_view;

  float
    *const interImage = pass->interImage,
    *const scanline = pass->scanline;

  const ssize_t
    scanLineSize = pass->scanLineSize,
    width = pass->width;

  const double
    totalWeight = pass->totalWeight;

  ExceptionInfo
    *const exception = pass->exception;

  const int
    id = GetOpenMPThreadId();

  const Quantum
    *magick_restrict p;

  float
    *pix,
    *pixels;

  LocalContrastColumn
    column;

  if ((*status) == MagickFalse)
    return;
  pixels=scanline;
  pixels+=id*scanLineSize;
  pix=pixels;
  p=GetCacheViewVirtualPixels(image_view,x,-(ssize_t) width,1,
    image->rows+(size_t) (2*width),exception);
  if (p == (const Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return;
    }
  LoadLocalContrastLuma(image,p,pix,width);
  column.image=image;
  column.interImage=interImage;
  column.pixels=pixels;
  column.width=width;
  column.x=x;
  column.totalWeight=totalWeight;
  StoreLocalContrastColumn(&column);
}

typedef struct _LocalContrastRowPass
{
  const Image
    *image;

  CacheView
    *image_view,
    *contrast_view;

  float
    *interImage,
    *scanline;

  ssize_t
    scanLineSize,
    width;

  double
    totalWeight,
    strength;

  Image
    *contrast_image;

  ExceptionInfo
    *exception;
} LocalContrastRowPass;

static void RunLocalContrastRow(MagickBooleanType *status,
  const LocalContrastRowPass *pass,const ssize_t y)
{
  const Image
    *const image = pass->image;

  CacheView
    *const image_view = pass->image_view,
    *const contrast_view = pass->contrast_view;

  float
    *const interImage = pass->interImage,
    *const scanline = pass->scanline;

  const ssize_t
    scanLineSize = pass->scanLineSize,
    width = pass->width;

  const double
    totalWeight = pass->totalWeight,
    strength = pass->strength;

  Image
    *const contrast_image = pass->contrast_image;

  ExceptionInfo
    *const exception = pass->exception;

  const int
    id = GetOpenMPThreadId();

  const Quantum
    *magick_restrict p;

  float
    *pixels;

  LocalContrastRow
    row;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return;
  pixels=scanline;
  pixels+=id*scanLineSize;
  p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
  q=GetCacheViewAuthenticPixels(contrast_view,0,y,image->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return;
    }
  memcpy(pixels,interImage+((size_t) y*(image->columns+(size_t) (2*width))),
    (image->columns+(size_t) (2*width))*sizeof(float));
  row.image=image;
  row.pixels=pixels;
  row.width=width;
  row.totalWeight=totalWeight;
  row.strength=strength;
  row.contrast_image=contrast_image;
  ApplyLocalContrastRow(&row,p,q);
  if (SyncCacheViewAuthenticPixels(contrast_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

typedef struct _LocalContrastPasses
{
  const Image
    *image;

  CacheView
    *image_view,
    *contrast_view;

  float
    *interImage,
    *scanline;

  ssize_t
    scanLineSize,
    width;

  double
    totalWeight,
    strength;

  Image
    *contrast_image;

  ExceptionInfo
    *exception;
} LocalContrastPasses;

static MagickBooleanType RunLocalContrastPasses(
  const LocalContrastPasses *passes)
{
  const Image
    *const image = passes->image;

  CacheView
    *const image_view = passes->image_view,
    *const contrast_view = passes->contrast_view;

  float
    *const interImage = passes->interImage,
    *const scanline = passes->scanline;

  const ssize_t
    scanLineSize = passes->scanLineSize,
    width = passes->width;

  const double
    totalWeight = passes->totalWeight,
    strength = passes->strength;

  Image
    *const contrast_image = passes->contrast_image;

  ExceptionInfo
    *const exception = passes->exception;

  MagickBooleanType
    status;

  status=MagickTrue;
  {
    ssize_t
      x;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
#pragma omp parallel for schedule(static) \
    magick_number_threads(image,image,image->columns,1)
#endif
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      const LocalContrastColumnPass
        pass = { image, image_view, interImage, scanline, scanLineSize, width,
          totalWeight, exception };

      RunLocalContrastColumn(&status,&pass,x);
    }
  }
  /*
    Horizontal pass.
  */
  {
    ssize_t
      y;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
#pragma omp parallel for schedule(static) \
    magick_number_threads(image,image,image->rows,1)
#endif
    for (y=0; y < (ssize_t) image->rows; y++)
    {
      const LocalContrastRowPass
        pass = { image, image_view, contrast_view, interImage, scanline,
          scanLineSize, width, totalWeight, strength, contrast_image,
          exception };

      RunLocalContrastRow(&status,&pass,y);
    }
  }
  return(status);
}

typedef struct _LocalContrastFilter
{
  const Image
    *image;

  CacheView
    *image_view,
    *contrast_view;

  float
    *scanline;

  ssize_t
    scanLineSize,
    width;

  double
    strength;

  Image
    *contrast_image;

  ExceptionInfo
    *exception;

  MemoryInfo
    *scanline_info;
} LocalContrastFilter;

static Image *FilterLocalContrast(const LocalContrastFilter *filter)
{
  const Image
    *const image = filter->image;

  CacheView
    *image_view = filter->image_view,
    *contrast_view = filter->contrast_view;

  float
    *const scanline = filter->scanline;

  const ssize_t
    scanLineSize = filter->scanLineSize,
    width = filter->width;

  const double
    strength = filter->strength;

  Image
    *contrast_image = filter->contrast_image;

  ExceptionInfo
    *const exception = filter->exception;

  MemoryInfo
    *scanline_info = filter->scanline_info;

  double
    totalWeight;

  float
    *interImage;

  LocalContrastPasses
    passes;

  MagickBooleanType
    status;

  MemoryInfo
    *interImage_info;

  /*
    Create intermediate buffer.
  */
  interImage_info=AcquireVirtualMemory(image->rows*(image->columns+(size_t)
    (2*width)),sizeof(*interImage));
  if (interImage_info == (MemoryInfo *) NULL)
    {
      scanline_info=RelinquishVirtualMemory(scanline_info);
      contrast_view=DestroyCacheView(contrast_view);
      image_view=DestroyCacheView(image_view);
      contrast_image=DestroyImage(contrast_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  interImage=(float *) GetVirtualMemoryBlob(interImage_info);
  totalWeight=(float) ((width+1)*(width+1));
  /*
    Vertical pass.
  */
  passes.image=image;
  passes.image_view=image_view;
  passes.contrast_view=contrast_view;
  passes.interImage=interImage;
  passes.scanline=scanline;
  passes.scanLineSize=scanLineSize;
  passes.width=width;
  passes.totalWeight=totalWeight;
  passes.strength=strength;
  passes.contrast_image=contrast_image;
  passes.exception=exception;
  status=RunLocalContrastPasses(&passes);
  scanline_info=RelinquishVirtualMemory(scanline_info);
  interImage_info=RelinquishVirtualMemory(interImage_info);
  contrast_view=DestroyCacheView(contrast_view);
  image_view=DestroyCacheView(image_view);
  return(ReleaseFailedImage(contrast_image,status));
}

MagickExport Image *LocalContrastImage(const Image *image,const double radius,
  const double strength,ExceptionInfo *exception)
{
#define LocalContrastImageTag  "LocalContrast/Image"

  CacheView
    *image_view,
    *contrast_view;

  float
    *scanline;

  Image
    *contrast_image;

  LocalContrastFilter
    filter;

  MemoryInfo
    *scanline_info;

  ssize_t
    scanLineSize,
    width;

  /*
    Initialize contrast image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  contrast_image=AccelerateLocalContrastImage(image,radius,strength,exception);
  if (contrast_image != (Image *) NULL)
    return(contrast_image);
#endif
  if (AcquireDirectClassClone(image,&contrast_image,exception) == MagickFalse)
    return((Image *) NULL);
  image_view=AcquireVirtualCacheView(image,exception);
  contrast_view=AcquireAuthenticCacheView(contrast_image,exception);
  scanLineSize=(ssize_t) MagickMax(image->columns,image->rows);
  width=(ssize_t) (scanLineSize*0.002*fabs(radius));
  scanLineSize+=(2*width);
  scanline_info=AcquireVirtualMemory(GetOpenMPMaximumThreads()*
    (size_t) scanLineSize,sizeof(*scanline));
  if (scanline_info == (MemoryInfo *) NULL)
    {
      contrast_view=DestroyCacheView(contrast_view);
      image_view=DestroyCacheView(image_view);
      contrast_image=DestroyImage(contrast_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  scanline=(float *) GetVirtualMemoryBlob(scanline_info);
  filter.image=image;
  filter.image_view=image_view;
  filter.contrast_view=contrast_view;
  filter.scanline=scanline;
  filter.scanLineSize=scanLineSize;
  filter.width=width;
  filter.strength=strength;
  filter.contrast_image=contrast_image;
  filter.exception=exception;
  filter.scanline_info=scanline_info;
  return(FilterLocalContrast(&filter));
}






/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S p r e a d I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SpreadImage() is a special effects method that randomly displaces each
%  pixel in a square area defined by the radius parameter.
%
%  The format of the SpreadImage method is:
%
%      Image *SpreadImage(const Image *image,
%        const PixelInterpolateMethod method,const double radius,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o method:  interpolation method.
%
%    o radius:  choose a random pixel in a neighborhood of this extent.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _SpreadInfo
{
  const Image
    *image;

  Image
    *spread_image;

  CacheView
    *image_view,
    *spread_view;

  ExceptionInfo
    *exception;

  PixelInterpolateMethod
    method;

  size_t
    width;

  ssize_t
    y;
} SpreadInfo;

static void SpreadRow(MagickBooleanType *status,const SpreadInfo *spread,
  RandomInfo *random_info,Quantum *magick_restrict q)
{
  const Image
    *const image = spread->image;

  Image
    *const spread_image = spread->spread_image;

  CacheView
    *const image_view = spread->image_view;

  ExceptionInfo
    *const exception = spread->exception;

  const PixelInterpolateMethod
    method = spread->method;

  const size_t
    width = spread->width;

  const ssize_t
    y = spread->y;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    PointInfo
      point;

    point.x=GetPseudoRandomValue(random_info);
    point.y=GetPseudoRandomValue(random_info);
    (*status)=InterpolatePixelChannels(image,image_view,spread_image,method,
      (double) x+width*(point.x-0.5),(double) y+width*(point.y-0.5),q,
      exception);
    if ((*status) == MagickFalse)
      break;
    q+=(ptrdiff_t) GetPixelChannels(spread_image);
  }
}

static MagickBooleanType SpreadHeadRow(MagickBooleanType *status,
  const SpreadInfo *spread,RandomInfo *random_info)
{
  CacheView
    *const spread_view = spread->spread_view;

  ExceptionInfo
    *const exception = spread->exception;

  const Image
    *const spread_image = spread->spread_image;

  const ssize_t
    y = spread->y;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=QueueCacheViewAuthenticPixels(spread_view,0,y,spread_image->columns,1,
    exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  SpreadRow(status,spread,random_info,q);
  if (SyncCacheViewAuthenticPixels(spread_view,exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

MagickExport Image *SpreadImage(const Image *image,
  const PixelInterpolateMethod method,const double radius,
  ExceptionInfo *exception)
{
#define SpreadImageTag  "Spread/Image"

  CacheView
    *image_view,
    *spread_view;

  Image
    *spread_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  RandomInfo
    **magick_restrict random_info;

  size_t
    width;

  ssize_t
    y;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
  unsigned long
    key;
#endif

  /*
    Initialize spread image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (AcquireDirectClassClone(image,&spread_image,exception) == MagickFalse)
    return((Image *) NULL);
  /*
    Spread image.
  */
  status=MagickTrue;
  progress=0;
  width=GetOptimalKernelWidth1D(radius,0.5);
  random_info=AcquireRandomInfoTLS();
  image_view=AcquireVirtualCacheView(image,exception);
  spread_view=AcquireAuthenticCacheView(spread_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  key=GetRandomSecretKey(random_info[0]);
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,spread_image,image->rows,key == ~0UL)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const int
      id = GetOpenMPThreadId();

    const SpreadInfo
      spread = { image, spread_image, image_view, spread_view, exception,
        method, width, y };

    if (SpreadHeadRow(&status,&spread,random_info[id]) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportRowProgress(image,SpreadImageTag,progress,&status);
      }
  }
  spread_view=DestroyCacheView(spread_view);
  image_view=DestroyCacheView(image_view);
  random_info=DestroyRandomInfoTLS(random_info);
  return(ReleaseFailedImage(spread_image,status));
}

