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
%               BBBB    AAA   L       AAA   N   N   CCCC  EEEEE               %
%               B   B  A   A  L      A   A  NN  N  C      E                   %
%               BBBB   AAAAA  L      AAAAA  N N N  C      EEE                 %
%               B   B  A   A  L      A   A  N  NN  C      E                   %
%               BBBB   A   A  LLLLL  A   A  N   N   CCCC  EEEEE               %
%                                                                             %
%                                                                             %
%                   MagickCore Image White Balance Methods                    %
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
#include "MagickCore/enhance-color-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     W h i t e B a l a n c e I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WhiteBalanceImage() applies white balancing to an image according to a
%  grayworld assumption in the LAB colorspace.
%
%  The format of the WhiteBalanceImage method is:
%
%      MagickBooleanType WhiteBalanceImage(Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image to auto-level
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void ShiftChromaRow(const Image *image,const double a_mean,
  const double b_mean,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    double
      a,
      b;

    /*
      Scale the chroma distance shifted according to amount of luminance.
    */
    a=(double) GetPixela(image,q)-1.1*(double) GetPixelL(image,q)*a_mean;
    b=(double) GetPixelb(image,q)-1.1*(double) GetPixelL(image,q)*b_mean;
    SetPixela(image,ClampToQuantum(a),q);
    SetPixelb(image,ClampToQuantum(b),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickStatusType LevelVibrance(Image *image,const char *artifact,
  MagickStatusType status,ExceptionInfo *exception)
{
  ChannelType
    channel_mask;

  double
    black_point = 0.0;

  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  /*
    Level the a & b channels.
  */
  flags=ParseGeometry(artifact,&geometry_info);
  if ((flags & RhoValue) != 0)
    black_point=geometry_info.rho;
  if ((flags & PercentValue) != 0)
    black_point*=((double) QuantumRange/100.0);
  channel_mask=SetImageChannelMask(image,(ChannelType) (aChannel |
    bChannel));
  status&=(MagickStatusType) LevelImage(image,black_point,(double)
    QuantumRange-black_point,1.0,exception);
  (void) SetImageChannelMask(image,channel_mask);
  return(status);
}

static void AccumulateChromaMeans(const Image *image,
  const Quantum *magick_restrict p,double *a_mean,double *b_mean)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    (*a_mean)+=QuantumScale*(double) GetPixela(image,p)-0.5;
    (*b_mean)+=QuantumScale*(double) GetPixelb(image,p)-0.5;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickStatusType LevelWhiteBalanceVibrance(Image *image,
  MagickStatusType status,ExceptionInfo *exception)
{
  const char
    *artifact;

  artifact=GetImageArtifact(image,"white-balance:vibrance");
  if (artifact != (const char *) NULL)
    status=LevelVibrance(image,artifact,status,exception);
  return(status);
}

static void SumWhiteBalanceChroma(const EnhanceLine *line,
  MagickStatusType *status,double *a_mean,double *b_mean)
{
  ssize_t
    y;

  for (y=0; y < (ssize_t) line->image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    if ((*status) == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(line->image_view,0,y,line->image->columns,1,
      line->exception);
    if (p == (Quantum *) NULL)
      {
        (*status)=MagickFalse;
        continue;
      }
    AccumulateChromaMeans(line->image,p,a_mean,b_mean);
  }
}

static MagickBooleanType FinishWhiteBalance(Image *image,
  MagickStatusType status,ExceptionInfo *exception)
{
  status=LevelWhiteBalanceVibrance(image,status,exception);
  status&=(MagickStatusType) TransformImageColorspace(image,sRGBColorspace,
    exception);
  return(status != 0 ? MagickTrue : MagickFalse);
}

typedef struct _WhiteBalanceHead
{
  const Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;

  double
    a_mean,
    b_mean;
} WhiteBalanceHead;

static MagickBooleanType ShiftWhiteBalanceHead(const WhiteBalanceHead *head,
  const ssize_t y,MagickStatusType *status)
{
  const Image
    *const image = head->image;

  CacheView
    *const image_view = head->image_view;

  const double
    a_mean = head->a_mean,
    b_mean = head->b_mean;

  ExceptionInfo
    *const exception = head->exception;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  ShiftChromaRow(image,a_mean,b_mean,q);
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static void ReportWhiteBalanceProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickStatusType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

MagickExport MagickBooleanType WhiteBalanceImage(Image *image,
  ExceptionInfo *exception)
{
#define WhiteBalanceImageTag  "WhiteBalance/Image"

  CacheView
    *image_view;

  double
    a_mean,
    b_mean;

  EnhanceLine
    line;

  MagickOffsetType
    progress;

  MagickStatusType
    status;

  ssize_t
    y;

  /*
    White balance image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  status=TransformImageColorspace(image,LabColorspace,exception);
  a_mean=0.0;
  b_mean=0.0;
  image_view=AcquireAuthenticCacheView(image,exception);
  line.image=image;
  line.image_view=image_view;
  line.exception=exception;
  SumWhiteBalanceChroma(&line,&status,&a_mean,&b_mean);
  a_mean/=((double) image->columns*image->rows);
  b_mean/=((double) image->columns*image->rows);
  progress=0;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    WhiteBalanceHead
      head;

    head.image=image;
    head.image_view=image_view;
    head.exception=exception;
    head.a_mean=a_mean;
    head.b_mean=b_mean;
    if (ShiftWhiteBalanceHead(&head,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportWhiteBalanceProgress(image,WhiteBalanceImageTag,progress,
          &status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(FinishWhiteBalance(image,status,exception));
}
