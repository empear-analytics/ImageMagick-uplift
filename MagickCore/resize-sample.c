/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  RRRR   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                   %
%                  R   R  E      SS       I       ZZ  E                       %
%                  RRRR   EEE     SSS     I     ZZZ   EEE                     %
%                  R R    E         SS    I    ZZ     E                       %
%                  R  R   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                   %
%                                                                             %
%                  SSSSS   AAA   M   M  PPPP   L      EEEEE                   %
%                  SS     A   A  MM MM  P   P  L      E                       %
%                   SSS   AAAAA  M M M  PPPP   L      EEE                     %
%                     SS  A   A  M   M  P      L      E                       %
%                  SSSSS  A   A  M   M  P      LLLLL  EEEEE                   %
%                                                                             %
%                                                                             %
%                       MagickCore Image Sample Method                        %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/accelerate-private.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/magick.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/profile-private.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resize-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#if defined(MAGICKCORE_LQR_DELEGATE)
#include <lqr.h>
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S a m p l e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SampleImage() scales an image to the desired dimensions with pixel
%  sampling.  Unlike other scaling methods, this method does not introduce
%  any additional color into the scaled image.
%
%  The format of the SampleImage method is:
%
%      Image *SampleImage(const Image *image,const size_t columns,
%        const size_t rows,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o columns: the number of columns in the sampled image.
%
%    o rows: the number of rows in the sampled image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void CopySamplePixel(const Image *image,const Image *sample_image,
  const Quantum *p,Quantum *q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(sample_image); i++)
  {
    PixelChannel
      channel;

    PixelTrait
      image_traits,
      traits;

    channel=GetPixelChannelChannel(sample_image,i);
    traits=GetPixelChannelTraits(sample_image,channel);
    image_traits=GetPixelChannelTraits(image,channel);
    if ((traits == UndefinedPixelTrait) ||
        (image_traits == UndefinedPixelTrait))
      continue;
    SetPixelChannel(sample_image,channel,p[i],q);
  }
}

static MagickBooleanType SampleImageRow(const Image *image,
  const Image *sample_image,CacheView *image_view,const PointInfo sample_offset,
  const ssize_t y,Quantum *q,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  ssize_t
    x;

  status=MagickTrue;
  for (x=0; x < (ssize_t) sample_image->columns; x++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x_offset,
      y_offset;

    if (GetPixelWriteMask(sample_image,q) <= (QuantumRange/2))
      {
        q+=(ptrdiff_t) GetPixelChannels(sample_image);
        continue;
      }
    x_offset=(ssize_t) ((((double) x+sample_offset.x)*image->columns)/
      sample_image->columns);
    y_offset=(ssize_t) ((((double) y+sample_offset.y)*image->rows)/
      sample_image->rows);
    p=GetCacheViewVirtualPixels(image_view,x_offset,y_offset,1,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        break;
      }
    CopySamplePixel(image,sample_image,p,q);
    q+=(ptrdiff_t) GetPixelChannels(sample_image);
  }
  return(status);
}

static PointInfo GetSampleOffset(const Image *image)
{
  PointInfo
    sample_offset;

  sample_offset.x=0.5-MagickEpsilon;
  sample_offset.y=sample_offset.x;
  {
    const char
      *value;

    value=GetImageArtifact(image,"sample:offset");
    if (value != (char *) NULL)
      {
        GeometryInfo
          geometry_info;

        MagickStatusType
          flags;

        (void) ParseGeometry(value,&geometry_info);
        flags=ParseGeometry(value,&geometry_info);
        sample_offset.x=sample_offset.y=geometry_info.rho/100.0-MagickEpsilon;
        if ((flags & SigmaValue) != 0)
          sample_offset.y=geometry_info.sigma/100.0-MagickEpsilon;
      }
  }
  return(sample_offset);
}

MagickExport Image *SampleImage(const Image *image,const size_t columns,
  const size_t rows,ExceptionInfo *exception)
{
#define SampleImageTag  "Sample/Image"

  CacheView
    *image_view,
    *sample_view;

  Image
    *sample_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PointInfo
    sample_offset;

  ssize_t
    y;

  /*
    Initialize sampled image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((columns == 0) || (rows == 0))
    ThrowImageException(ImageError,"NegativeOrZeroImageSize");
  if ((columns == image->columns) && (rows == image->rows))
    return(CloneImage(image,0,0,MagickTrue,exception));
  sample_image=CloneImage(image,columns,rows,MagickTrue,exception);
  if (sample_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Set the sampling offset, default is in the mid-point of sample regions.
  */
  sample_offset=GetSampleOffset(image);
  /*
    Sample each row.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  sample_view=AcquireAuthenticCacheView(sample_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,sample_image,sample_image->rows,2)
#endif
  for (y=0; y < (ssize_t) sample_image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=QueueCacheViewAuthenticPixels(sample_view,0,y,sample_image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    /*
      Sample each column.
    */
    if (SampleImageRow(image,sample_image,image_view,sample_offset,y,q,
          exception) == MagickFalse)
      status=MagickFalse;
    if (SyncCacheViewAuthenticPixels(sample_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

        proceed=SetImageProgress(image,SampleImageTag,progress++,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  sample_view=DestroyCacheView(sample_view);
  sample_image->type=image->type;
  if (status == MagickFalse)
    sample_image=DestroyImage(sample_image);
  return(sample_image);
}
