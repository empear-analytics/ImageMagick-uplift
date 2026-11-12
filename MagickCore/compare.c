/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               CCCC   OOO   M   M  PPPP    AAA   RRRR    EEEEE               %
%              C      O   O  MM MM  P   P  A   A  R   R   E                   %
%              C      O   O  M M M  PPPP   AAAAA  RRRR    EEE                 %
%              C      O   O  M   M  P      A   A  R R     E                   %
%               CCCC   OOO   M   M  P      A   A  R  R    EEEEE               %
%                                                                             %
%                                                                             %
%                      MagickCore Image Comparison Methods                    %
%                                                                             %
%                              Software Design                                %
%                                  Cristy                                     %
%                               December 2003                                 %
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
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/compare.h"
#include "MagickCore/compare-private.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/distort.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/fourier.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/registry.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/statistic.h"
#include "MagickCore/statistic-private.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"
#include "MagickCore/version.h"


static inline MagickBooleanType IsCompareRowMissing(const Quantum *p,
  const Quantum *q,const Quantum *r)
{
  return(((p == (const Quantum *) NULL) || (q == (const Quantum *) NULL) ||
    (r == (Quantum *) NULL)) ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p a r e I m a g e s                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompareImages() compares one or more pixel channels of an image to a
%  reconstructed image and returns the difference image.
%
%  The format of the CompareImages method is:
%
%      Image *CompareImages(const Image *image,const Image *reconstruct_image,
%        const MetricType metric,double *distortion,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o reconstruct_image: the reconstruction image.
%
%    o metric: the metric.
%
%    o distortion: the computed distortion between the images.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _CompareHighlightRow
{
  Image
    *image;

  const Image
    *reconstruct_image;

  Image
    *highlight_image;

  PixelInfo
    highlight,
    lowlight,
    masklight;

  size_t
    columns;
} CompareHighlightRow;

static void HighlightCompareRow(const CompareHighlightRow *row,
  const Quantum *magick_restrict p,const Quantum *magick_restrict q,
  Quantum *magick_restrict r)
{
  Image
    *const image = row->image;

  const Image
    *const reconstruct_image = row->reconstruct_image;

  Image
    *const highlight_image = row->highlight_image;

  const PixelInfo
    highlight = row->highlight,
    lowlight = row->lowlight,
    masklight = row->masklight;

  const size_t
    columns = row->columns;

  ssize_t
    x;

  for (x=0; x < (ssize_t) columns; x++)
  {
    if ((GetPixelReadMask(image,p) <= (QuantumRange/2)) ||
        (GetPixelReadMask(reconstruct_image,q) <= (QuantumRange/2)))
      {
        SetPixelViaPixelInfo(highlight_image,&masklight,r);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
        r+=(ptrdiff_t) GetPixelChannels(highlight_image);
        continue;
      }
    if (IsFuzzyEquivalencePixel(image,p,reconstruct_image,q) == MagickFalse)
      SetPixelViaPixelInfo(highlight_image,&highlight,r);
    else
      SetPixelViaPixelInfo(highlight_image,&lowlight,r);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
    r+=(ptrdiff_t) GetPixelChannels(highlight_image);
  }
}

typedef struct _CompareImageTargets
{
  size_t
    *columns,
    *rows;

  Image
    **difference_image,
    **highlight_image;

  MagickBooleanType
    *status;
} CompareImageTargets;

static MagickBooleanType AcquireCompareImages(const Image *image,
  const Image *reconstruct_image,ExceptionInfo *exception,
  const CompareImageTargets *targets)
{
  Image
    *clone_image,
    **const difference_image = targets->difference_image,
    **const highlight_image = targets->highlight_image;

  MagickBooleanType
    *const status = targets->status;

  RectangleInfo
    geometry;

  size_t
    *const columns = targets->columns,
    *const rows = targets->rows;

  SetImageCompareBounds(image,reconstruct_image,columns,rows);
  SetGeometry(image,&geometry);
  geometry.width=*columns;
  geometry.height=*rows;
  clone_image=CloneImage(image,0,0,MagickTrue,exception);
  if (clone_image == (Image *) NULL)
    return(MagickFalse);
  (void) SetImageMask(clone_image,ReadPixelMask,(Image *) NULL,exception);
  *difference_image=ExtentImage(clone_image,&geometry,exception);
  clone_image=DestroyImage(clone_image);
  if (*difference_image == (Image *) NULL)
    return(MagickFalse);
  (void) ResetImagePage(*difference_image,"0x0+0+0");
  (void) SetImageAlphaChannel(*difference_image,OpaqueAlphaChannel,exception);
  *highlight_image=CloneImage(image,*columns,*rows,MagickTrue,exception);
  if (*highlight_image == (Image *) NULL)
    {
      *difference_image=DestroyImage(*difference_image);
      return(MagickFalse);
    }
  *status=SetImageStorageClass(*highlight_image,DirectClass,exception);
  if (*status == MagickFalse)
    {
      *difference_image=DestroyImage(*difference_image);
      *highlight_image=DestroyImage(*highlight_image);
      return(MagickFalse);
    }
  (void) SetImageMask(*highlight_image,ReadPixelMask,(Image *) NULL,exception);
  (void) SetImageAlphaChannel(*highlight_image,OpaqueAlphaChannel,exception);
  return(MagickTrue);
}

typedef struct _CompareColorTargets
{
  PixelInfo
    *highlight,
    *lowlight,
    *masklight;
} CompareColorTargets;

static void GetCompareColors(const Image *image,ExceptionInfo *exception,
  const CompareColorTargets *targets)
{
  const char
    *artifact;

  PixelInfo
    *const highlight = targets->highlight,
    *const lowlight = targets->lowlight,
    *const masklight = targets->masklight;

  (void) QueryColorCompliance("#f1001ecc",AllCompliance,highlight,exception);
  artifact=GetImageArtifact(image,"compare:highlight-color");
  if (artifact != (const char *) NULL)
    (void) QueryColorCompliance(artifact,AllCompliance,highlight,exception);
  (void) QueryColorCompliance("#ffffffcc",AllCompliance,lowlight,exception);
  artifact=GetImageArtifact(image,"compare:lowlight-color");
  if (artifact != (const char *) NULL)
    (void) QueryColorCompliance(artifact,AllCompliance,lowlight,exception);
  (void) QueryColorCompliance("#888888cc",AllCompliance,masklight,exception);
  artifact=GetImageArtifact(image,"compare:masklight-color");
  if (artifact != (const char *) NULL)
    (void) QueryColorCompliance(artifact,AllCompliance,masklight,exception);
}

typedef struct _CompareRowsInfo
{
  Image
    *image;

  const Image
    *reconstruct_image;

  Image
    *highlight_image;

  PixelInfo
    highlight,
    lowlight,
    masklight;

  size_t
    columns,
    rows;

  ExceptionInfo
    *exception;
} CompareRowsInfo;

static void SyncCompareRow(CacheView *highlight_view,
  MagickBooleanType *status,ExceptionInfo *exception)
{
  MagickBooleanType
    sync;

  sync=SyncCacheViewAuthenticPixels(highlight_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
}

static MagickBooleanType HighlightCompareRows(const CompareRowsInfo *info,
  MagickBooleanType status)
{
  CacheView
    *highlight_view,
    *image_view,
    *reconstruct_view;

  Image
    *const image = info->image,
    *const highlight_image = info->highlight_image;

  const Image
    *const reconstruct_image = info->reconstruct_image;

  const PixelInfo
    highlight = info->highlight,
    lowlight = info->lowlight,
    masklight = info->masklight;

  const size_t
    columns = info->columns,
    rows = info->rows;

  ExceptionInfo
    *const exception = info->exception;

  ssize_t
    y;

  image_view=AcquireVirtualCacheView(image,exception);
  reconstruct_view=AcquireVirtualCacheView(reconstruct_image,exception);
  highlight_view=AcquireAuthenticCacheView(highlight_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,highlight_image,rows,1)
#endif
  for (y=0; y < (ssize_t) rows; y++)
  {
    const Quantum
      *magick_restrict p,
      *magick_restrict q;

    Quantum
      *magick_restrict r;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,columns,1,exception);
    q=GetCacheViewVirtualPixels(reconstruct_view,0,y,columns,1,exception);
    r=QueueCacheViewAuthenticPixels(highlight_view,0,y,columns,1,exception);
    if (IsCompareRowMissing(p,q,r) != MagickFalse)
      {
        status=MagickFalse;
        continue;
      }
    {
      const CompareHighlightRow
        row = { image, reconstruct_image, highlight_image, highlight, lowlight,
          masklight, columns };

      HighlightCompareRow(&row,p,q,r);
    }
    SyncCompareRow(highlight_view,&status,exception);
  }
  highlight_view=DestroyCacheView(highlight_view);
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

MagickExport Image *CompareImages(Image *image,const Image *reconstruct_image,
  const MetricType metric,double *distortion,ExceptionInfo *exception)
{
  CompareRowsInfo
    info;

  Image
    *difference_image,
    *highlight_image;

  MagickBooleanType
    status = MagickTrue;

  PixelInfo
    highlight,
    lowlight,
    masklight;

  size_t
    columns,
    rows;

  const CompareImageTargets
    targets = { &columns, &rows, &difference_image, &highlight_image, &status };

  const CompareColorTargets
    colors = { &highlight, &lowlight, &masklight };

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(reconstruct_image != (const Image *) NULL);
  assert(reconstruct_image->signature == MagickCoreSignature);
  assert(distortion != (double *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  *distortion=0.0;
  status=GetImageDistortion(image,reconstruct_image,metric,distortion,
    exception);
  if (status == MagickFalse)
    return((Image *) NULL);
  if (AcquireCompareImages(image,reconstruct_image,exception,&targets) ==
      MagickFalse)
    return((Image *) NULL);
  GetCompareColors(image,exception,&colors);
  /*
    Generate difference image.
  */
  info.image=image;
  info.reconstruct_image=reconstruct_image;
  info.highlight_image=highlight_image;
  info.highlight=highlight;
  info.lowlight=lowlight;
  info.masklight=masklight;
  info.columns=columns;
  info.rows=rows;
  info.exception=exception;
  status=HighlightCompareRows(&info,status);
  if ((status != MagickFalse) && (difference_image != (Image *) NULL))
    status=CompositeImage(difference_image,highlight_image,image->compose,
      MagickTrue,0,0,exception);
  highlight_image=DestroyImage(highlight_image);
  if (status == MagickFalse)
    difference_image=DestroyImage(difference_image);
  return(difference_image);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  I s I m a g e s E q u a l                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsImagesEqual() compare the pixels of two images and returns immediately
%  if any pixel is not identical.
%
%  The format of the IsImagesEqual method is:
%
%      MagickBooleanType IsImagesEqual(const Image *image,
%        const Image *reconstruct_image,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o reconstruct_image: the reconstruction image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static ssize_t FindUnequalPixelChannel(const Image *image,
  const Image *reconstruct_image,const Quantum *magick_restrict p,
  const Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      distance;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    distance=fabs((double) p[i]-(double) GetPixelChannel(reconstruct_image,
      channel,q));
    if (distance >= MagickEpsilon)
      break;
  }
  return(i);
}

typedef struct _CompareColumnScan
{
  const Image
    *image,
    *reconstruct_image;

  size_t
    columns;
} CompareColumnScan;

static MagickBooleanType IsCompareRowEqual(const CompareColumnScan *scan,
  const Quantum *magick_restrict p,const Quantum *magick_restrict q)
{
  const Image
    *const image = scan->image,
    *const reconstruct_image = scan->reconstruct_image;

  const size_t
    columns = scan->columns;

  ssize_t
    x;

  for (x=0; x < (ssize_t) columns; x++)
  {
    ssize_t
      i;

    i=FindUnequalPixelChannel(image,reconstruct_image,p,q);
    if (i < (ssize_t) GetPixelChannels(image))
      return(MagickFalse);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
  }
  return(MagickTrue);
}

MagickExport MagickBooleanType IsImagesEqual(const Image *image,
  const Image *reconstruct_image,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *reconstruct_view;

  size_t
    columns,
    rows;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(reconstruct_image != (const Image *) NULL);
  assert(reconstruct_image->signature == MagickCoreSignature);
  SetImageCompareBounds(image,reconstruct_image,&columns,&rows);
  image_view=AcquireVirtualCacheView(image,exception);
  reconstruct_view=AcquireVirtualCacheView(reconstruct_image,exception);
  for (y=0; y < (ssize_t) rows; y++)
  {
    const Quantum
      *magick_restrict p,
      *magick_restrict q;

    CompareColumnScan
      scan;

    p=GetCacheViewVirtualPixels(image_view,0,y,columns,1,exception);
    q=GetCacheViewVirtualPixels(reconstruct_view,0,y,columns,1,exception);
    if ((p == (const Quantum *) NULL) || (q == (const Quantum *) NULL))
      break;
    scan.image=image;
    scan.reconstruct_image=reconstruct_image;
    scan.columns=columns;
    if (IsCompareRowEqual(&scan,p,q) == MagickFalse)
      break;
  }
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  return(y < (ssize_t) rows ? MagickFalse : MagickTrue);
}

