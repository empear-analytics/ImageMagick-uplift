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
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%                                                                             %
%                   MagickCore Filtered Image Resize Method                   %
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
%   R e s i z e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResizeImage() scales an image to the desired dimensions, using the given
%  filter (see AcquireFilterInfo()).
%
%  If an undefined filter is given the filter defaults to Mitchell for a
%  colormapped image, a image with a matte channel, or if the image is
%  enlarged.  Otherwise the filter defaults to a Lanczos.
%
%  ResizeImage() was inspired by Paul Heckbert's "zoom" program.
%
%  The format of the ResizeImage method is:
%
%      Image *ResizeImage(Image *image,const size_t columns,const size_t rows,
%        const FilterType filter,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o columns: the number of columns in the scaled image.
%
%    o rows: the number of rows in the scaled image.
%
%    o filter: Image filter to use.
%
%    o exception: return any errors or warnings in this structure.
%
*/

typedef struct _ContributionInfo
{
  double
    weight;

  ssize_t
    pixel;
} ContributionInfo;

static ContributionInfo **DestroyContributionTLS(
  ContributionInfo **contribution)
{
  ssize_t
    i;

  assert(contribution != (ContributionInfo **) NULL);
  for (i=0; i < (ssize_t) GetMagickResourceLimit(ThreadResource); i++)
    if (contribution[i] != (ContributionInfo *) NULL)
      contribution[i]=(ContributionInfo *) RelinquishAlignedMemory(
        contribution[i]);
  contribution=(ContributionInfo **) RelinquishMagickMemory(contribution);
  return(contribution);
}

static ContributionInfo **AcquireContributionTLS(const size_t count)
{
  ssize_t
    i;

  ContributionInfo
    **contribution;

  size_t
    number_threads;

  number_threads=(size_t) GetMagickResourceLimit(ThreadResource);
  contribution=(ContributionInfo **) AcquireQuantumMemory(number_threads,
    sizeof(*contribution));
  if (contribution == (ContributionInfo **) NULL)
    return((ContributionInfo **) NULL);
  (void) memset(contribution,0,number_threads*sizeof(*contribution));
  for (i=0; i < (ssize_t) number_threads; i++)
  {
    contribution[i]=(ContributionInfo *) MagickAssumeAligned(
      AcquireAlignedMemory(count,sizeof(**contribution)));
    if (contribution[i] == (ContributionInfo *) NULL)
      return(DestroyContributionTLS(contribution));
  }
  return(contribution);
}

static void NormalizeContributions(ContributionInfo *contribution,
  const ssize_t n,double density)
{
  ssize_t
    i;

  density=MagickSafeReciprocal(density);
  for (i=0; i < n; i++)
    contribution[i].weight*=density;
}

static void FilterHorizontalPixel(const Image *image,
  const Image *resize_image,const ContributionInfo *contribution,
  const ssize_t n,const ssize_t start,const ssize_t stop,const double bisect,
  const ssize_t y,const Quantum *p,Quantum *q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      alpha,
      gamma,
      pixel;

    PixelChannel
      channel;

    PixelTrait
      resize_traits,
      traits;

    ssize_t
      j,
      k;

    channel=GetPixelChannelChannel(image,i);
    traits=GetPixelChannelTraits(image,channel);
    resize_traits=GetPixelChannelTraits(resize_image,channel);
    if ((traits == UndefinedPixelTrait) ||
        (resize_traits == UndefinedPixelTrait))
      continue;
    if (((resize_traits & CopyPixelTrait) != 0) ||
        (GetPixelWriteMask(resize_image,q) <= (QuantumRange/2)))
      {
        j=(ssize_t) (MagickMin(MagickMax(bisect,(double) start),(double)
          stop-1.0)+0.5);
        k=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
          (contribution[j-start].pixel-contribution[0].pixel);
        SetPixelChannel(resize_image,channel,
          p[k*(ssize_t) GetPixelChannels(image)+i],q);
        continue;
      }
    pixel=0.0;
    if ((resize_traits & BlendPixelTrait) == 0)
      {
        /*
          No alpha blending.
        */
        for (j=0; j < n; j++)
        {
          k=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
            (contribution[j].pixel-contribution[0].pixel);
          alpha=contribution[j].weight;
          pixel+=alpha*(double) p[k*(ssize_t) GetPixelChannels(image)+i];
        }
        SetPixelChannel(resize_image,channel,ClampToQuantum(pixel),q);
        continue;
      }
    /*
      Alpha blending.
    */
    gamma=0.0;
    for (j=0; j < n; j++)
    {
      k=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
        (contribution[j].pixel-contribution[0].pixel);
      alpha=contribution[j].weight*QuantumScale*
        (double) GetPixelAlpha(image,p+k*(ssize_t) GetPixelChannels(image));
      pixel+=alpha*(double) p[k*(ssize_t) GetPixelChannels(image)+i];
      gamma+=alpha;
    }
    gamma=MagickSafeReciprocal(gamma);
    SetPixelChannel(resize_image,channel,ClampToQuantum(gamma*pixel),q);
  }
}

static MagickBooleanType HorizontalFilter(
  const ResizeFilter *magick_restrict resize_filter,
  const Image *magick_restrict image,Image *magick_restrict resize_image,
  const double x_factor,const MagickSizeType span,
  MagickOffsetType *magick_restrict progress,ExceptionInfo *exception)
{
#define ResizeImageTag  "Resize/Image"

  CacheView
    *image_view,
    *resize_view;

  ClassType
    storage_class;

  ContributionInfo
    **magick_restrict contributions;

  double
    scale,
    support;

  MagickBooleanType
    status;

  ssize_t
    x;

  /*
    Apply filter to resize horizontally from image to resize image.
  */
  scale=MagickMax(1.0/x_factor+MagickEpsilon,1.0);
  support=scale*GetResizeFilterSupport(resize_filter);
  storage_class=support > 0.5 ? DirectClass : image->storage_class;
  if (SetImageStorageClass(resize_image,storage_class,exception) == MagickFalse)
    return(MagickFalse);
  if (support < 0.5)
    {
      /*
        Support too small even for nearest neighbour: Reduce to point sampling.
      */
      support=(double) 0.5;
      scale=1.0;
    }
  contributions=AcquireContributionTLS((size_t) (2.0*support+3.0));
  if (contributions == (ContributionInfo **) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(MagickFalse);
    }
  status=MagickTrue;
  scale=MagickSafeReciprocal(scale);
  image_view=AcquireVirtualCacheView(image,exception);
  resize_view=AcquireAuthenticCacheView(resize_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,resize_image,resize_image->columns,1)
#endif
  for (x=0; x < (ssize_t) resize_image->columns; x++)
  {
    const int
      id = GetOpenMPThreadId();

    const Quantum
      *magick_restrict p;

    ContributionInfo
      *magick_restrict contribution;

    double
      bisect,
      density;

    Quantum
      *magick_restrict q;

    ssize_t
      n,
      start,
      stop,
      y;

    if (status == MagickFalse)
      continue;
    bisect=(double) (x+0.5)/x_factor+MagickEpsilon;
    start=(ssize_t) MagickMax(bisect-support+0.5,0.0);
    stop=(ssize_t) MagickMin(bisect+support+0.5,(double) image->columns);
    density=0.0;
    contribution=contributions[id];
    for (n=0; n < (stop-start); n++)
    {
      contribution[n].pixel=start+n;
      contribution[n].weight=GetResizeFilterWeight(resize_filter,scale*
        ((double) (start+n)-bisect+0.5));
      density+=contribution[n].weight;
    }
    if (n == 0)
      continue;
    if ((density != 0.0) && (density != 1.0))
      {
        /*
          Normalize.
        */
        NormalizeContributions(contribution,n,density);
      }
    p=GetCacheViewVirtualPixels(image_view,contribution[0].pixel,0,(size_t)
      (contribution[n-1].pixel-contribution[0].pixel+1),image->rows,exception);
    q=QueueCacheViewAuthenticPixels(resize_view,x,0,1,resize_image->rows,
      exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    for (y=0; y < (ssize_t) resize_image->rows; y++)
    {
      FilterHorizontalPixel(image,resize_image,contribution,n,start,stop,bisect,
        y,p,q);
      q+=(ptrdiff_t) GetPixelChannels(resize_image);
    }
    if (SyncCacheViewAuthenticPixels(resize_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        (*progress)++;
        proceed=SetImageProgress(image,ResizeImageTag,*progress,span);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  resize_view=DestroyCacheView(resize_view);
  image_view=DestroyCacheView(image_view);
  contributions=DestroyContributionTLS(contributions);
  return(status);
}

static void FilterVerticalPixel(const Image *image,
  const Image *resize_image,const ContributionInfo *contribution,
  const ssize_t n,const ssize_t start,const ssize_t stop,const double bisect,
  const ssize_t x,const Quantum *p,Quantum *q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      alpha,
      gamma,
      pixel;

    PixelChannel
      channel;

    PixelTrait
      resize_traits,
      traits;

    ssize_t
      j,
      k;

    channel=GetPixelChannelChannel(image,i);
    traits=GetPixelChannelTraits(image,channel);
    resize_traits=GetPixelChannelTraits(resize_image,channel);
    if ((traits == UndefinedPixelTrait) ||
        (resize_traits == UndefinedPixelTrait))
      continue;
    if (((resize_traits & CopyPixelTrait) != 0) ||
        (GetPixelWriteMask(resize_image,q) <= (QuantumRange/2)))
      {
        j=(ssize_t) (MagickMin(MagickMax(bisect,(double) start),(double)
          stop-1.0)+0.5);
        k=(ssize_t) ((contribution[j-start].pixel-contribution[0].pixel)*
          (ssize_t) image->columns+x);
        SetPixelChannel(resize_image,channel,p[k*(ssize_t)
          GetPixelChannels(image)+i],q);
        continue;
      }
    pixel=0.0;
    if ((resize_traits & BlendPixelTrait) == 0)
      {
        /*
          No alpha blending.
        */
        for (j=0; j < n; j++)
        {
          k=(ssize_t) ((contribution[j].pixel-contribution[0].pixel)*
            (ssize_t) image->columns+x);
          alpha=contribution[j].weight;
          pixel+=alpha*(double) p[k*(ssize_t) GetPixelChannels(image)+i];
        }
        SetPixelChannel(resize_image,channel,ClampToQuantum(pixel),q);
        continue;
      }
    gamma=0.0;
    for (j=0; j < n; j++)
    {
      k=(ssize_t) ((contribution[j].pixel-contribution[0].pixel)*
        (ssize_t) image->columns+x);
      alpha=contribution[j].weight*QuantumScale*(double)
       GetPixelAlpha(image,p+k*(ssize_t) GetPixelChannels(image));
      pixel+=alpha*(double) p[k*(ssize_t) GetPixelChannels(image)+i];
      gamma+=alpha;
    }
    gamma=MagickSafeReciprocal(gamma);
    SetPixelChannel(resize_image,channel,ClampToQuantum(gamma*pixel),q);
  }
}

static MagickBooleanType VerticalFilter(
  const ResizeFilter *magick_restrict resize_filter,
  const Image *magick_restrict image,Image *magick_restrict resize_image,
  const double y_factor,const MagickSizeType span,
  MagickOffsetType *magick_restrict progress,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *resize_view;

  ClassType
    storage_class;

  ContributionInfo
    **magick_restrict contributions;

  double
    scale,
    support;

  MagickBooleanType
    status;

  ssize_t
    y;

  /*
    Apply filter to resize vertically from image to resize image.
  */
  scale=MagickMax(1.0/y_factor+MagickEpsilon,1.0);
  support=scale*GetResizeFilterSupport(resize_filter);
  storage_class=support > 0.5 ? DirectClass : image->storage_class;
  if (SetImageStorageClass(resize_image,storage_class,exception) == MagickFalse)
    return(MagickFalse);
  if (support < 0.5)
    {
      /*
        Support too small even for nearest neighbour: Reduce to point sampling.
      */
      support=(double) 0.5;
      scale=1.0;
    }
  contributions=AcquireContributionTLS((size_t) (2.0*support+3.0));
  if (contributions == (ContributionInfo **) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(MagickFalse);
    }
  status=MagickTrue;
  scale=MagickSafeReciprocal(scale);
  image_view=AcquireVirtualCacheView(image,exception);
  resize_view=AcquireAuthenticCacheView(resize_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,resize_image,resize_image->rows,1)
#endif
  for (y=0; y < (ssize_t) resize_image->rows; y++)
  {
    const int
      id = GetOpenMPThreadId();

    const Quantum
      *magick_restrict p;

    ContributionInfo
      *magick_restrict contribution;

    double
      bisect,
      density;

    Quantum
      *magick_restrict q;

    ssize_t
      n,
      start,
      stop,
      x;

    if (status == MagickFalse)
      continue;
    bisect=(double) (y+0.5)/y_factor+MagickEpsilon;
    start=(ssize_t) MagickMax(bisect-support+0.5,0.0);
    stop=(ssize_t) MagickMin(bisect+support+0.5,(double) image->rows);
    density=0.0;
    contribution=contributions[id];
    for (n=0; n < (stop-start); n++)
    {
      contribution[n].pixel=start+n;
      contribution[n].weight=GetResizeFilterWeight(resize_filter,scale*
        ((double) (start+n)-bisect+0.5));
      density+=contribution[n].weight;
    }
    if (n == 0)
      continue;
    if ((density != 0.0) && (density != 1.0))
      {
        /*
          Normalize.
        */
        NormalizeContributions(contribution,n,density);
      }
    p=GetCacheViewVirtualPixels(image_view,0,contribution[0].pixel,
      image->columns,(size_t) (contribution[n-1].pixel-contribution[0].pixel+1),
      exception);
    q=QueueCacheViewAuthenticPixels(resize_view,0,y,resize_image->columns,1,
      exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) resize_image->columns; x++)
    {
      FilterVerticalPixel(image,resize_image,contribution,n,start,stop,bisect,
        x,p,q);
      q+=(ptrdiff_t) GetPixelChannels(resize_image);
    }
    if (SyncCacheViewAuthenticPixels(resize_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        (*progress)++;
        proceed=SetImageProgress(image,ResizeImageTag,*progress,span);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  resize_view=DestroyCacheView(resize_view);
  image_view=DestroyCacheView(image_view);
  contributions=DestroyContributionTLS(contributions);
  return(status);
}

static inline MagickBooleanType IsMitchellResize(const Image *image,
  const double x_factor,const double y_factor)
{
  return(((image->storage_class == PseudoClass) ||
    (image->alpha_trait != UndefinedPixelTrait) ||
    ((x_factor*y_factor) > 1.0)) ? MagickTrue : MagickFalse);
}

static FilterType SelectResizeFilterType(const Image *image,
  const FilterType filter,const double x_factor,const double y_factor)
{
  FilterType
    filter_type;

  filter_type=LanczosFilter;
  if (filter != UndefinedFilter)
    filter_type=filter;
  else
    if ((x_factor == 1.0) && (y_factor == 1.0))
      filter_type=PointFilter;
    else
      if (IsMitchellResize(image,x_factor,y_factor) != MagickFalse)
        filter_type=MitchellFilter;
  return(filter_type);
}

static inline MagickBooleanType IsResizeIdentity(const Image *image,
  const size_t columns,const size_t rows,const FilterType filter)
{
  return(((columns == image->columns) && (rows == image->rows) &&
    (filter == UndefinedFilter)) ? MagickTrue : MagickFalse);
}

static void RecordResizeTransform(const Image *image,Image *resize_image,
  const FilterType filter_type,ExceptionInfo *exception)
{
  char
    transform[MagickPathExtent];

  (void) FormatLocaleString(transform,MagickPathExtent,
    "resize %.17gx%.17g %.17gx%.17g %d",(double) image->columns,
    (double) image->rows,(double) resize_image->columns,
    (double) resize_image->rows,(int) filter_type);
  AppendImageProfileProperty(resize_image,"hdrgm","hdrgm:Transform",
    transform,exception);
}

MagickExport Image *ResizeImage(const Image *image,const size_t columns,
  const size_t rows,const FilterType filter,ExceptionInfo *exception)
{
  double
    x_factor,
    y_factor;

  FilterType
    filter_type;

  Image
    *filter_image,
    *resize_image;

  MagickOffsetType
    offset;

  MagickSizeType
    span;

  MagickStatusType
    status;

  ResizeFilter
    *resize_filter;

  /*
    Acquire resize image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((columns == 0) || (rows == 0))
    ThrowImageException(ImageError,"NegativeOrZeroImageSize");
  if (IsResizeIdentity(image,columns,rows,filter) != MagickFalse)
    return(CloneImage(image,0,0,MagickTrue,exception));
  /*
    Acquire resize filter.
  */
  x_factor=(double) (columns*MagickSafeReciprocal((double) image->columns));
  y_factor=(double) (rows*MagickSafeReciprocal((double) image->rows));
  filter_type=SelectResizeFilterType(image,filter,x_factor,y_factor);
  resize_filter=AcquireResizeFilter(image,filter_type,MagickFalse,exception);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  resize_image=AccelerateResizeImage(image,columns,rows,resize_filter,
    exception);
  if (resize_image != (Image *) NULL)
    {
      resize_filter=DestroyResizeFilter(resize_filter);
      goto record_resize_transform;
    }
#endif
  resize_image=CloneImage(image,columns,rows,MagickTrue,exception);
  if (resize_image == (Image *) NULL)
    {
      resize_filter=DestroyResizeFilter(resize_filter);
      return(resize_image);
    }
  if (x_factor > y_factor)
    filter_image=CloneImage(image,columns,image->rows,MagickTrue,exception);
  else
    filter_image=CloneImage(image,image->columns,rows,MagickTrue,exception);
  if (filter_image == (Image *) NULL)
    {
      resize_filter=DestroyResizeFilter(resize_filter);
      return(DestroyImage(resize_image));
    }
  /*
    Resize image.
  */
  offset=0;
  if (x_factor > y_factor)
    {
      span=(MagickSizeType) (filter_image->columns+rows);
      status=HorizontalFilter(resize_filter,image,filter_image,x_factor,span,
        &offset,exception);
      status&=(MagickStatusType) VerticalFilter(resize_filter,filter_image,
        resize_image,y_factor,span,&offset,exception);
    }
  else
    {
      span=(MagickSizeType) (filter_image->rows+columns);
      status=VerticalFilter(resize_filter,image,filter_image,y_factor,span,
        &offset,exception);
      status&=(MagickStatusType) HorizontalFilter(resize_filter,filter_image,
        resize_image,x_factor,span,&offset,exception);
    }
  /*
    Free resources.
  */
  filter_image=DestroyImage(filter_image);
  resize_filter=DestroyResizeFilter(resize_filter);
  if (status == MagickFalse)
    {
      resize_image=DestroyImage(resize_image);
      return((Image *) NULL);
    }
  resize_image->type=image->type;
#if defined(MAGICKCORE_OPENCL_SUPPORT)
record_resize_transform:
#endif
  RecordResizeTransform(image,resize_image,filter_type,exception);
  return(resize_image);
}
