/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                   EEEEE  FFFFF  FFFFF  EEEEE  CCCC  TTTTT                   %
%                   E      F      F      E     C        T                     %
%                   EEE    FFF    FFF    EEE   C        T                     %
%                   E      F      F      E     C        T                     %
%                   EEEEE  F      F      EEEEE  CCCC    T                     %
%                                                                             %
%                                                                             %
%                       MagickCore Image Effects Methods                      %
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
  Typedef declarations.
*/
typedef struct _HullBuffers
{
  const Image
    *image;

  size_t
    columns,
    rows;

  Quantum
    *f,
    *g;
} HullBuffers;

typedef struct _UnsharpPixels
{
  const Image
    *image,
    *unsharp_image;

  double
    quantum_threshold,
    gain;
} UnsharpPixels;

typedef struct _ShadeImages
{
  const Image
    *linear_image,
    *shade_image;

  MagickBooleanType
    gray;
} ShadeImages;


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     D e s p e c k l e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DespeckleImage() reduces the speckle noise in an image while preserving the
%  edges of the original image.  A speckle removing filter uses a complementary
%  hulling technique (raising pixels that are darker than their surrounding
%  neighbors, then complementarily lowering pixels that are brighter than their
%  surrounding neighbors) to reduce the speckle index of that image (reference
%  Crimmins speckle removal).
%
%  The format of the DespeckleImage method is:
%
%      Image *DespeckleImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickRealType RaiseHullValue(const Quantum *r,const ssize_t i,
  MagickRealType v)
{
  if ((MagickRealType) r[i] >= (v+(double) ScaleCharToQuantum(2)))
    v+=(double) ScaleCharToQuantum(1);
  return(v);
}

static MagickRealType LowerHullValue(const Quantum *r,const ssize_t i,
  MagickRealType v)
{
  if ((MagickRealType) r[i] <= (v-(double) ScaleCharToQuantum(2)))
    v-=(double) ScaleCharToQuantum(1);
  return(v);
}

static MagickRealType RaiseHullEdgeValue(const Quantum *r,const Quantum *s,
  const ssize_t i,MagickRealType v)
{
  if (((MagickRealType) s[i] >= (v+(double) ScaleCharToQuantum(2))) &&
      ((MagickRealType) r[i] > v))
    v+=(double) ScaleCharToQuantum(1);
  return(v);
}

static MagickRealType LowerHullEdgeValue(const Quantum *r,const Quantum *s,
  const ssize_t i,MagickRealType v)
{
  if (((MagickRealType) s[i] <= (v-(double) ScaleCharToQuantum(2))) &&
      ((MagickRealType) r[i] < v))
    v-=(double) ScaleCharToQuantum(1);
  return(v);
}

typedef struct _HullFirstPass
{
  size_t
    columns;

  Quantum
    *p,
    *q,
    *r;
} HullFirstPass;

static void RaiseHullRow(const HullFirstPass *pass,ssize_t i)
{
  const size_t
    columns = pass->columns;

  Quantum
    *const p = pass->p,
    *const q = pass->q,
    *const r = pass->r;

  MagickRealType
    v;

  ssize_t
    x;

  for (x=0; x < (ssize_t) columns; x++)
  {
    v=(MagickRealType) p[i];
    v=RaiseHullValue(r,i,v);
    q[i]=(Quantum) v;
    i++;
  }
}

static void LowerHullRow(const HullFirstPass *pass,ssize_t i)
{
  const size_t
    columns = pass->columns;

  Quantum
    *const p = pass->p,
    *const q = pass->q,
    *const r = pass->r;

  MagickRealType
    v;

  ssize_t
    x;

  for (x=0; x < (ssize_t) columns; x++)
  {
    v=(MagickRealType) p[i];
    v=LowerHullValue(r,i,v);
    q[i]=(Quantum) v;
    i++;
  }
}

typedef struct _HullSecondPass
{
  size_t
    columns;

  Quantum
    *p,
    *q,
    *r,
    *s;
} HullSecondPass;

static void RaiseHullEdgeRow(const HullSecondPass *pass,ssize_t i)
{
  const size_t
    columns = pass->columns;

  Quantum
    *const p = pass->p,
    *const q = pass->q,
    *const r = pass->r,
    *const s = pass->s;

  MagickRealType
    v;

  ssize_t
    x;

  for (x=0; x < (ssize_t) columns; x++)
  {
    v=(MagickRealType) q[i];
    v=RaiseHullEdgeValue(r,s,i,v);
    p[i]=(Quantum) v;
    i++;
  }
}

static void LowerHullEdgeRow(const HullSecondPass *pass,ssize_t i)
{
  const size_t
    columns = pass->columns;

  Quantum
    *const p = pass->p,
    *const q = pass->q,
    *const r = pass->r,
    *const s = pass->s;

  MagickRealType
    v;

  ssize_t
    x;

  for (x=0; x < (ssize_t) columns; x++)
  {
    v=(MagickRealType) q[i];
    v=LowerHullEdgeValue(r,s,i,v);
    p[i]=(Quantum) v;
    i++;
  }
}

static void HullFirstPassRow(const HullFirstPass *pass,const ssize_t i,
  const int polarity)
{
  if (polarity > 0)
    RaiseHullRow(pass,i);
  else
    LowerHullRow(pass,i);
}

static void HullSecondPassRow(const HullSecondPass *pass,const ssize_t i,
  const int polarity)
{
  if (polarity > 0)
    RaiseHullEdgeRow(pass,i);
  else
    LowerHullEdgeRow(pass,i);
}

static void Hull(const HullBuffers *hull,const ssize_t x_offset,
  const ssize_t y_offset,const int polarity)
{
  const Image
    *const image = hull->image;

  const size_t
    columns = hull->columns,
    rows = hull->rows;

  Quantum
    *magick_restrict const f = hull->f,
    *magick_restrict const g = hull->g;

  Quantum
    *p,
    *q,
    *r,
    *s;

  ssize_t
    y;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(f != (Quantum *) NULL);
  assert(g != (Quantum *) NULL);
  assert(columns <= (size_t) (MAGICK_SSIZE_MAX-2));
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  p=f+(ptrdiff_t) (columns+2);
  q=g+(ptrdiff_t) (columns+2);
  r=p+(ptrdiff_t) (y_offset*((ssize_t) columns+2)+x_offset);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) \
    magick_number_threads(image,image,rows,2)
#endif
  for (y=0; y < (ssize_t) rows; y++)
  {
    HullFirstPass
      pass;

    ssize_t
      i;

    pass.columns=columns;
    pass.p=p;
    pass.q=q;
    pass.r=r;
    i=(2*y+1)+y*(ssize_t) columns;
    HullFirstPassRow(&pass,i,polarity);
  }
  p=f+(ptrdiff_t) (columns+2);
  q=g+(ptrdiff_t) (columns+2);
  r=q+(ptrdiff_t) (y_offset*((ssize_t) columns+2)+x_offset);
  s=q-(ptrdiff_t) (y_offset*((ssize_t) columns+2)+x_offset);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) \
    magick_number_threads(image,image,rows,2)
#endif
  for (y=0; y < (ssize_t) rows; y++)
  {
    HullSecondPass
      pass;

    ssize_t
      i;

    pass.columns=columns;
    pass.p=p;
    pass.q=q;
    pass.r=r;
    pass.s=s;
    i=(2*y+1)+y*(ssize_t) columns;
    HullSecondPassRow(&pass,i,polarity);
  }
}

typedef struct _DespeckleLoad
{
  const Image
    *image;

  CacheView
    *image_view;

  Quantum
    *pixels;

  ssize_t
    i;
} DespeckleLoad;

static MagickBooleanType LoadDespeckleChannel(const DespeckleLoad *load,
  MagickBooleanType status,ExceptionInfo *exception)
{
  const Image
    *const image = load->image;

  CacheView
    *const image_view = load->image_view;

  Quantum
    *magick_restrict const pixels = load->pixels;

  const ssize_t
    i = load->i;

  ssize_t
    j,
    x,
    y;

  j=(ssize_t) image->columns+2;
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    j++;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      pixels[j++]=p[i];
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
    j++;
  }
  return(status);
}

typedef struct _DespeckleStore
{
  const Image
    *image;

  Image
    *despeckle_image;

  CacheView
    *despeckle_view;

  PixelChannel
    channel;

  Quantum
    *pixels;
} DespeckleStore;

static MagickBooleanType StoreDespeckleChannel(const DespeckleStore *store,
  MagickBooleanType status,ExceptionInfo *exception)
{
  const Image
    *const image = store->image;

  Image
    *const despeckle_image = store->despeckle_image;

  CacheView
    *const despeckle_view = store->despeckle_view;

  const PixelChannel
    channel = store->channel;

  Quantum
    *magick_restrict const pixels = store->pixels;

  ssize_t
    j,
    x,
    y;

  j=(ssize_t) image->columns+2;
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    MagickBooleanType
      sync;

    Quantum
      *magick_restrict q;

    q=GetCacheViewAuthenticPixels(despeckle_view,0,y,despeckle_image->columns,
      1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    j++;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      SetPixelChannel(despeckle_image,channel,pixels[j++],q);
      q+=(ptrdiff_t) GetPixelChannels(despeckle_image);
    }
    sync=SyncCacheViewAuthenticPixels(despeckle_view,exception);
    if (sync == MagickFalse)
      status=MagickFalse;
    j++;
  }
  return(status);
}

#define DespeckleImageTag  "Despeckle/Image"

static MagickBooleanType ReportDespeckleProgress(const Image *image,
  const ssize_t i,MagickBooleanType status)
{
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(image,DespeckleImageTag,(MagickOffsetType) i,
        GetPixelChannels(image));
      if (proceed == MagickFalse)
        status=MagickFalse;
    }
  return(status);
}

static void RelinquishDespeckleMemory(MemoryInfo *buffer_info,
  MemoryInfo *pixel_info)
{
  if (buffer_info != (MemoryInfo *) NULL)
    buffer_info=RelinquishVirtualMemory(buffer_info);
  if (pixel_info != (MemoryInfo *) NULL)
    pixel_info=RelinquishVirtualMemory(pixel_info);
}

typedef struct _DespeckleHull
{
  const Image
    *image;

  Quantum
    *pixels,
    *buffer;
} DespeckleHull;

static void HullAllPolarities(const DespeckleHull *despeckle_hull,
  const ssize_t x,const ssize_t y)
{
  const Image
    *const image = despeckle_hull->image;

  HullBuffers
    hull;

  Quantum
    *magick_restrict const buffer = despeckle_hull->buffer,
    *magick_restrict const pixels = despeckle_hull->pixels;

  hull.image=image;
  hull.columns=image->columns;
  hull.rows=image->rows;
  hull.f=pixels;
  hull.g=buffer;
  Hull(&hull,x,y,1);
  hull.image=image;
  hull.columns=image->columns;
  hull.rows=image->rows;
  hull.f=pixels;
  hull.g=buffer;
  Hull(&hull,-x,-y,1);
  hull.image=image;
  hull.columns=image->columns;
  hull.rows=image->rows;
  hull.f=pixels;
  hull.g=buffer;
  Hull(&hull,-x,-y,-1);
  hull.image=image;
  hull.columns=image->columns;
  hull.rows=image->rows;
  hull.f=pixels;
  hull.g=buffer;
  Hull(&hull,x,y,-1);
}

typedef struct _DespeckleChannelPass
{
  const Image
    *image;

  Image
    *despeckle_image;

  CacheView
    *image_view,
    *despeckle_view;

  ExceptionInfo
    *exception;

  Quantum
    *buffer,
    *pixels;

  size_t
    length;

  const ssize_t
    *X,
    *Y;
} DespeckleChannelPass;

static void DespeckleChannel(const DespeckleChannelPass *despeckle,
  const ssize_t i,MagickBooleanType *status)
{
  const Image
    *const image = despeckle->image;

  Image
    *const despeckle_image = despeckle->despeckle_image;

  CacheView
    *const image_view = despeckle->image_view,
    *const despeckle_view = despeckle->despeckle_view;

  ExceptionInfo
    *const exception = despeckle->exception;

  Quantum
    *magick_restrict const buffer = despeckle->buffer,
    *magick_restrict const pixels = despeckle->pixels;

  const size_t
    length = despeckle->length;

  const ssize_t
    *const X = despeckle->X,
    *const Y = despeckle->Y;

  DespeckleLoad
    load;

  DespeckleStore
    store;

  DespeckleHull
    despeckle_hull;

  PixelChannel
     channel;

  PixelTrait
    despeckle_traits,
    traits;

  ssize_t
    k;

  if ((*status) == MagickFalse)
    return;
  channel=GetPixelChannelChannel(image,i);
  traits=GetPixelChannelTraits(image,channel);
  despeckle_traits=GetPixelChannelTraits(despeckle_image,channel);
  if ((traits == UndefinedPixelTrait) ||
      (despeckle_traits == UndefinedPixelTrait))
    return;
  if ((despeckle_traits & CopyPixelTrait) != 0)
    return;
  (void) memset(pixels,0,length*sizeof(*pixels));
  load.image=image;
  load.image_view=image_view;
  load.pixels=pixels;
  load.i=i;
  (*status)=LoadDespeckleChannel(&load,(*status),exception);
  (void) memset(buffer,0,length*sizeof(*buffer));
  for (k=0; k < 4; k++)
  {
    despeckle_hull.image=image;
    despeckle_hull.pixels=pixels;
    despeckle_hull.buffer=buffer;
    HullAllPolarities(&despeckle_hull,X[k],Y[k]);
  }
  store.image=image;
  store.despeckle_image=despeckle_image;
  store.despeckle_view=despeckle_view;
  store.channel=channel;
  store.pixels=pixels;
  (*status)=StoreDespeckleChannel(&store,(*status),exception);
  (*status)=ReportDespeckleProgress(image,i,(*status));
}

static Image *ReleaseFailedImage(Image *image,const MagickBooleanType status)
{
  if (status == MagickFalse)
    image=DestroyImage(image);
  return(image);
}

typedef struct _DespeckleReduction
{
  const Image
    *image;

  Image
    *despeckle_image;

  ExceptionInfo
    *exception;

  size_t
    length;

  const ssize_t
    *X,
    *Y;

  MemoryInfo
    *pixel_info,
    *buffer_info;
} DespeckleReduction;

static Image *ReduceSpeckle(const DespeckleReduction *reduction)
{
  const Image
    *const image = reduction->image;

  Image
    *const despeckle_image = reduction->despeckle_image;

  ExceptionInfo
    *const exception = reduction->exception;

  const size_t
    length = reduction->length;

  const ssize_t
    *const X = reduction->X,
    *const Y = reduction->Y;

  MemoryInfo
    *pixel_info = reduction->pixel_info,
    *buffer_info = reduction->buffer_info;

  CacheView
    *despeckle_view,
    *image_view;

  DespeckleChannelPass
    despeckle;

  MagickBooleanType
    status;

  Quantum
    *magick_restrict buffer,
    *magick_restrict pixels;

  ssize_t
    i;

  pixels=(Quantum *) GetVirtualMemoryBlob(pixel_info);
  buffer=(Quantum *) GetVirtualMemoryBlob(buffer_info);
  /*
    Reduce speckle in the image.
  */
  status=MagickTrue;
  image_view=AcquireVirtualCacheView(image,exception);
  despeckle_view=AcquireAuthenticCacheView(despeckle_image,exception);
  despeckle.image=image;
  despeckle.despeckle_image=despeckle_image;
  despeckle.image_view=image_view;
  despeckle.despeckle_view=despeckle_view;
  despeckle.exception=exception;
  despeckle.buffer=buffer;
  despeckle.pixels=pixels;
  despeckle.length=length;
  despeckle.X=X;
  despeckle.Y=Y;
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    DespeckleChannel(&despeckle,i,&status);
  }
  despeckle_view=DestroyCacheView(despeckle_view);
  image_view=DestroyCacheView(image_view);
  buffer_info=RelinquishVirtualMemory(buffer_info);
  pixel_info=RelinquishVirtualMemory(pixel_info);
  despeckle_image->type=image->type;
  return(ReleaseFailedImage(despeckle_image,status));
}

MagickExport Image *DespeckleImage(const Image *image,ExceptionInfo *exception)
{
  DespeckleReduction
    reduction;

  Image
    *despeckle_image;

  MagickBooleanType
    status;

  MemoryInfo
    *buffer_info,
    *pixel_info;

  Quantum
    *magick_restrict buffer,
    *magick_restrict pixels;

  size_t
    length;

  static const ssize_t
    X[4] = {0, 1, 1,-1},
    Y[4] = {1, 0, 1, 1};

  /*
    Allocate despeckled image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  despeckle_image=AccelerateDespeckleImage(image,exception);
  if (despeckle_image != (Image *) NULL)
    return(despeckle_image);
#endif
  despeckle_image=CloneImage(image,0,0,MagickTrue,exception);
  if (despeckle_image == (Image *) NULL)
    return((Image *) NULL);
  status=SetImageStorageClass(despeckle_image,DirectClass,exception);
  if (status == MagickFalse)
    {
      despeckle_image=DestroyImage(despeckle_image);
      return((Image *) NULL);
    }
  /*
    Allocate image buffer.
  */
  if ((image->columns > (MAGICK_SIZE_MAX-2)) ||
      (image->rows > (MAGICK_SIZE_MAX-2)))
    {
      despeckle_image=DestroyImage(despeckle_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  length=(image->columns+2)*(image->rows+2);
  pixel_info=AcquireVirtualMemory(length,sizeof(*pixels));
  buffer_info=AcquireVirtualMemory(length,sizeof(*buffer));
  if ((pixel_info == (MemoryInfo *) NULL) ||
      (buffer_info == (MemoryInfo *) NULL))
    {
      RelinquishDespeckleMemory(buffer_info,pixel_info);
      despeckle_image=DestroyImage(despeckle_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  reduction.image=image;
  reduction.despeckle_image=despeckle_image;
  reduction.exception=exception;
  reduction.length=length;
  reduction.X=X;
  reduction.Y=Y;
  reduction.pixel_info=pixel_info;
  reduction.buffer_info=buffer_info;
  return(ReduceSpeckle(&reduction));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     E d g e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EdgeImage() finds edges in an image.  Radius defines the radius of the
%  convolution filter.  Use a radius of 0 and EdgeImage() selects a suitable
%  radius for you.
%
%  The format of the EdgeImage method is:
%
%      Image *EdgeImage(const Image *image,const double radius,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the pixel neighborhood.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *EdgeImage(const Image *image,const double radius,
  ExceptionInfo *exception)
{
  Image
    *edge_image;

  KernelInfo
    *kernel_info;

  ssize_t
    i;

  size_t
    width;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  width=GetOptimalKernelWidth1D(radius,0.5);
  kernel_info=AcquireKernelInfo((const char *) NULL,exception);
  if (kernel_info == (KernelInfo *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  (void) memset(kernel_info,0,sizeof(*kernel_info));
  kernel_info->width=width;
  kernel_info->height=width;
  kernel_info->x=(ssize_t) (kernel_info->width-1)/2;
  kernel_info->y=(ssize_t) (kernel_info->height-1)/2;
  kernel_info->signature=MagickCoreSignature;
  kernel_info->values=(MagickRealType *) MagickAssumeAligned(
    AcquireAlignedMemory(kernel_info->width,kernel_info->height*
    sizeof(*kernel_info->values)));
  if (kernel_info->values == (MagickRealType *) NULL)
    {
      kernel_info=DestroyKernelInfo(kernel_info);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  for (i=0; i < (ssize_t) (kernel_info->width*kernel_info->height); i++)
    kernel_info->values[i]=(-1.0);
  kernel_info->values[i/2]=(double) kernel_info->width*kernel_info->height-1.0;
  edge_image=ConvolveImage(image,kernel_info,exception);
  kernel_info=DestroyKernelInfo(kernel_info);
  return(edge_image);
}


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

static void ReportRowProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
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

MagickExport Image *LocalContrastImage(const Image *image,const double radius,
  const double strength,ExceptionInfo *exception)
{
#define LocalContrastImageTag  "LocalContrast/Image"

  CacheView
    *image_view,
    *contrast_view;

  double
    totalWeight;

  float
    *interImage,
    *scanline;

  Image
    *contrast_image;

  MagickBooleanType
    status;

  MemoryInfo
    *scanline_info,
    *interImage_info;

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
  scanline_info=RelinquishVirtualMemory(scanline_info);
  interImage_info=RelinquishVirtualMemory(interImage_info);
  contrast_view=DestroyCacheView(contrast_view);
  image_view=DestroyCacheView(image_view);
  return(ReleaseFailedImage(contrast_image,status));
}




/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S h a d e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShadeImage() shines a distant light on an image to create a
%  three-dimensional effect. You control the positioning of the light with
%  azimuth and elevation; azimuth is measured in degrees off the x axis
%  and elevation is measured in pixels above the Z axis.
%
%  The format of the ShadeImage method is:
%
%      Image *ShadeImage(const Image *image,const MagickBooleanType gray,
%        const double azimuth,const double elevation,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o gray: A value other than zero shades the intensity of each pixel.
%
%    o azimuth, elevation:  Define the light source direction.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline double GetSurfaceShade(const PrimaryInfo *normal,
  const PrimaryInfo *light)
{
  double
    distance,
    normal_distance,
    shade;

  if ((fabs(normal->x) <= MagickEpsilon) &&
      (fabs(normal->y) <= MagickEpsilon))
    shade=light->z;
  else
    {
      shade=0.0;
      distance=normal->x*light->x+normal->y*light->y+normal->z*light->z;
      if (distance > MagickEpsilon)
        {
          normal_distance=normal->x*normal->x+normal->y*normal->y+
            normal->z*normal->z;
          if (normal_distance > (MagickEpsilon*MagickEpsilon))
            shade=distance/sqrt((double) normal_distance);
        }
    }
  return(shade);
}

typedef struct _ShadeChannel
{
  const Image
    *linear_image,
    *shade_image;

  const Quantum
    *center;

  MagickBooleanType
    gray;

  double
    shade;
} ShadeChannel;

static inline void ShadePixelChannel(const ShadeChannel *shade_channel,
  const ssize_t i,Quantum *magick_restrict q)
{
  const Image
    *const linear_image = shade_channel->linear_image,
    *const shade_image = shade_channel->shade_image;

  const Quantum
    *const center = shade_channel->center;

  const MagickBooleanType
    gray = shade_channel->gray;

  const double
    shade = shade_channel->shade;

  PixelChannel
    channel;

  PixelTrait
    shade_traits,
    traits;

  channel=GetPixelChannelChannel(linear_image,i);
  traits=GetPixelChannelTraits(linear_image,channel);
  shade_traits=GetPixelChannelTraits(shade_image,channel);
  if ((traits == UndefinedPixelTrait) ||
      (shade_traits == UndefinedPixelTrait))
    return;
  if ((shade_traits & CopyPixelTrait) != 0)
    {
      SetPixelChannel(shade_image,channel,center[i],q);
      return;
    }
  if ((traits & UpdatePixelTrait) == 0)
    {
      SetPixelChannel(shade_image,channel,center[i],q);
      return;
    }
  if (gray != MagickFalse)
    {
      SetPixelChannel(shade_image,channel,ClampToQuantum(shade),q);
      return;
    }
  SetPixelChannel(shade_image,channel,ClampToQuantum(QuantumScale*
    shade*(double) center[i]),q);
}

static inline void ShadePixelChannels(const ShadeImages *shading,
  const Quantum *magick_restrict center,Quantum *magick_restrict q,
  const double shade)
{
  const Image
    *const linear_image = shading->linear_image,
    *const shade_image = shading->shade_image;

  const MagickBooleanType
    gray = shading->gray;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(linear_image); i++)
  {
    ShadeChannel
      shade_channel;

    shade_channel.linear_image=linear_image;
    shade_channel.shade_image=shade_image;
    shade_channel.center=center;
    shade_channel.gray=gray;
    shade_channel.shade=shade;
    ShadePixelChannel(&shade_channel,i,q);
  }
}

#define GetShadeIntensity(image,pixel) \
  ClampPixel(GetPixelIntensity((image),(pixel)))
#define ShadeImageTag  "Shade/Image"

typedef struct _ShadeRow
{
  const Image
    *linear_image,
    *shade_image;

  PrimaryInfo
    light;

  ShadeImages
    shading;

  CacheView
    *image_view,
    *shade_view;

  ExceptionInfo
    *exception;

  ssize_t
    y;
} ShadeRow;

static void ShadeRowPixels(const ShadeRow *row,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  const Image
    *const linear_image = row->linear_image,
    *const shade_image = row->shade_image;

  const PrimaryInfo
    light = row->light;

  const ShadeImages
    shading = row->shading;

  const Quantum
    *magick_restrict center,
    *magick_restrict post,
    *magick_restrict pre;

  double
    shade;

  PrimaryInfo
    normal;

  ssize_t
    x;

  normal.z=2.0*(double) QuantumRange;  /* constant Z of surface normal */
  for (x=0; x < (ssize_t) linear_image->columns; x++)
  {
    /*
      Determine the surface normal and compute shading.
    */
    pre=p+GetPixelChannels(linear_image);
    center=pre+(linear_image->columns+2)*GetPixelChannels(linear_image);
    post=center+(linear_image->columns+2)*GetPixelChannels(linear_image);
    normal.x=(double) (
      GetShadeIntensity(linear_image,pre-GetPixelChannels(linear_image))+
      GetShadeIntensity(linear_image,center-GetPixelChannels(linear_image))+
      GetShadeIntensity(linear_image,post-GetPixelChannels(linear_image))-
      GetShadeIntensity(linear_image,pre+GetPixelChannels(linear_image))-
      GetShadeIntensity(linear_image,center+GetPixelChannels(linear_image))-
      GetShadeIntensity(linear_image,post+GetPixelChannels(linear_image)));
    normal.y=(double) (
      GetShadeIntensity(linear_image,post-GetPixelChannels(linear_image))+
      GetShadeIntensity(linear_image,post)+
      GetShadeIntensity(linear_image,post+GetPixelChannels(linear_image))-
      GetShadeIntensity(linear_image,pre-GetPixelChannels(linear_image))-
      GetShadeIntensity(linear_image,pre)-
      GetShadeIntensity(linear_image,pre+GetPixelChannels(linear_image)));
    shade=GetSurfaceShade(&normal,&light);
    ShadePixelChannels(&shading,center,q,shade);
    p+=(ptrdiff_t) GetPixelChannels(linear_image);
    q+=(ptrdiff_t) GetPixelChannels(shade_image);
  }
}

typedef struct _ShadeImageTargets
{
  Image
    **linear_image,
    **shade_image;
} ShadeImageTargets;

static MagickBooleanType AcquireShadeImages(const Image *image,
  ExceptionInfo *exception,const ShadeImageTargets *targets)
{
  Image
    **const linear_image = targets->linear_image,
    **const shade_image = targets->shade_image;

  (*linear_image)=CloneImage(image,0,0,MagickTrue,exception);
  (*shade_image)=CloneImage(image,0,0,MagickTrue,exception);
  if (((*linear_image) == (Image *) NULL) || ((*shade_image) == (Image *) NULL))
    {
      if ((*linear_image) != (Image *) NULL)
        (*linear_image)=DestroyImage((*linear_image));
      if ((*shade_image) != (Image *) NULL)
        (*shade_image)=DestroyImage((*shade_image));
      return(MagickFalse);
    }
  if (SetImageStorageClass((*shade_image),DirectClass,exception) == MagickFalse)
    {
      (*linear_image)=DestroyImage((*linear_image));
      (*shade_image)=DestroyImage((*shade_image));
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType ShadeHeadRow(MagickBooleanType *status,
  const ShadeRow *row)
{
  const Image
    *const linear_image = row->linear_image,
    *const shade_image = row->shade_image;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(row->image_view,-1,row->y-1,
    linear_image->columns+2,3,row->exception);
  q=QueueCacheViewAuthenticPixels(row->shade_view,0,row->y,shade_image->columns,
    1,row->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  ShadeRowPixels(row,p,q);
  return(MagickTrue);
}

MagickExport Image *ShadeImage(const Image *image,const MagickBooleanType gray,
  const double azimuth,const double elevation,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *shade_view;

  Image
    *linear_image,
    *shade_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PrimaryInfo
    light;

  ShadeImageTargets
    targets;

  ssize_t
    y;

  /*
    Initialize shaded image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  targets.linear_image=(&linear_image);
  targets.shade_image=(&shade_image);
  if (AcquireShadeImages(image,exception,&targets) == MagickFalse)
    return((Image *) NULL);
  /*
    Compute the light vector.
  */
  light.x=(double) QuantumRange*cos(DegreesToRadians(azimuth))*
    cos(DegreesToRadians(elevation));
  light.y=(double) QuantumRange*sin(DegreesToRadians(azimuth))*
    cos(DegreesToRadians(elevation));
  light.z=(double) QuantumRange*sin(DegreesToRadians(elevation));
  /*
    Shade image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(linear_image,exception);
  shade_view=AcquireAuthenticCacheView(shade_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(linear_image,shade_image,linear_image->rows,1)
#endif
  for (y=0; y < (ssize_t) linear_image->rows; y++)
  {
    const ShadeImages
      shading = { linear_image, shade_image, gray };

    const ShadeRow
      row = { linear_image,shade_image,light,shading,image_view,shade_view,
        exception,y };

    if (ShadeHeadRow(&status,&row) == MagickFalse)
      continue;
    if (SyncCacheViewAuthenticPixels(shade_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportRowProgress(image,ShadeImageTag,progress,&status);
      }
  }
  shade_view=DestroyCacheView(shade_view);
  image_view=DestroyCacheView(image_view);
  linear_image=DestroyImage(linear_image);
  return(ReleaseFailedImage(shade_image,status));
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     U n s h a r p M a s k I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnsharpMaskImage() sharpens one or more image channels.  We convolve the
%  image with a Gaussian operator of the given radius and standard deviation
%  (sigma).  For reasonable results, radius should be larger than sigma.  Use a
%  radius of 0 and UnsharpMaskImage() selects a suitable radius for you.
%
%  The format of the UnsharpMaskImage method is:
%
%    Image *UnsharpMaskImage(const Image *image,const double radius,
%      const double sigma,const double amount,const double threshold,
%      ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o gain: the percentage of the difference between the original and the
%      blur image that is added back into the original.
%
%    o threshold: the threshold in pixels needed to apply the difference gain.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline void UnsharpPixelChannels(const UnsharpPixels *unsharp,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = unsharp->image,
    *const unsharp_image = unsharp->unsharp_image;

  const double
    quantum_threshold = unsharp->quantum_threshold,
    gain = unsharp->gain;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      pixel;

    PixelChannel
      channel;

    PixelTrait
      traits,
      unsharp_traits;

    channel=GetPixelChannelChannel(image,i);
    traits=GetPixelChannelTraits(image,channel);
    unsharp_traits=GetPixelChannelTraits(unsharp_image,channel);
    if ((traits == UndefinedPixelTrait) ||
        (unsharp_traits == UndefinedPixelTrait))
      continue;
    if ((unsharp_traits & CopyPixelTrait) != 0)
      {
        SetPixelChannel(unsharp_image,channel,p[i],q);
        continue;
      }
    pixel=(double) p[i]-(double) GetPixelChannel(unsharp_image,channel,q);
    if (fabs(2.0*pixel) < quantum_threshold)
      pixel=(double) p[i];
    else
      pixel=(double) p[i]+gain*pixel;
    SetPixelChannel(unsharp_image,channel,ClampToQuantum(pixel),q);
  }
}

static void UnsharpRow(const UnsharpPixels *unsharp,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = unsharp->image,
    *const unsharp_image = unsharp->unsharp_image;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    UnsharpPixelChannels(unsharp,p,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(unsharp_image);
  }
}

typedef struct _UnsharpHead
{
  UnsharpPixels
    unsharp;

  CacheView
    *image_view,
    *unsharp_view;

  ExceptionInfo
    *exception;
} UnsharpHead;

static MagickBooleanType UnsharpHeadRow(const UnsharpHead *head,const ssize_t y,
  MagickBooleanType *status)
{
  CacheView
    *const image_view = head->image_view,
    *const unsharp_view = head->unsharp_view;

  const Image
    *const image = head->unsharp.image,
    *const unsharp_image = head->unsharp.unsharp_image;

  const Quantum
    *magick_restrict p;

  const UnsharpPixels
    unsharp = head->unsharp;

  ExceptionInfo
    *const exception = head->exception;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
  q=GetCacheViewAuthenticPixels(unsharp_view,0,y,unsharp_image->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  UnsharpRow(&unsharp,p,q);
  if (SyncCacheViewAuthenticPixels(unsharp_view,exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

MagickExport Image *UnsharpMaskImage(const Image *image,const double radius,
  const double sigma,const double gain,const double threshold,
  ExceptionInfo *exception)
{
#define SharpenImageTag  "Sharpen/Image"

  CacheView
    *image_view,
    *unsharp_view;

  Image
    *unsharp_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  double
    quantum_threshold;

  ssize_t
    y;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
/* This kernel appears to be broken.
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  unsharp_image=AccelerateUnsharpMaskImage(image,radius,sigma,gain,threshold,
    exception);
  if (unsharp_image != (Image *) NULL)
    return(unsharp_image);
#endif
*/
  unsharp_image=BlurImage(image,radius,sigma,exception);
  if (unsharp_image == (Image *) NULL)
    return((Image *) NULL);
  quantum_threshold=(double) QuantumRange*threshold;
  /*
    Unsharp-mask image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  unsharp_view=AcquireAuthenticCacheView(unsharp_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,unsharp_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const UnsharpHead
      head = { { image, unsharp_image, quantum_threshold, gain }, image_view,
        unsharp_view, exception };

    if (UnsharpHeadRow(&head,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportRowProgress(image,SharpenImageTag,progress,&status);
      }
  }
  unsharp_image->type=image->type;
  unsharp_view=DestroyCacheView(unsharp_view);
  image_view=DestroyCacheView(image_view);
  return(ReleaseFailedImage(unsharp_image,status));
}
