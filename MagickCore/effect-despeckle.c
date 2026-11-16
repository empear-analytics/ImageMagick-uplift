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
%        DDDD   EEEEE  SSSSS  PPPP   EEEEE   CCCC  K   K  L      EEEEE        %
%        D   D  E      SS     P   P  E      C      K  K   L      E            %
%        D   D  EEE     SSS   PPPP   EEE    C      KKK    L      EEE          %
%        D   D  E         SS  P      E      C      K  K   L      E            %
%        DDDD   EEEEE  SSSSS  P      EEEEE   CCCC  K   K  LLLLL  EEEEE        %
%                                                                             %
%                                                                             %
%                     MagickCore Image Despeckle Methods                      %
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

typedef MagickRealType
  (*HullValueMethod)(const Quantum *,const ssize_t,MagickRealType);

static inline void HullRow(const HullFirstPass *pass,ssize_t i,
  const HullValueMethod method)
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
    v=method(r,i,v);
    q[i]=(Quantum) v;
    i++;
  }
}

static void RaiseHullRow(const HullFirstPass *pass,ssize_t i)
{
  HullRow(pass,i,RaiseHullValue);
}

static void LowerHullRow(const HullFirstPass *pass,ssize_t i)
{
  HullRow(pass,i,LowerHullValue);
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

static inline MagickBooleanType IsDespeckleSizeOverflow(const Image *image)
{
  return(((image->columns > (MAGICK_SIZE_MAX-2)) ||
    (image->rows > (MAGICK_SIZE_MAX-2))) ? MagickTrue : MagickFalse);
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
  if (IsDespeckleSizeOverflow(image) != MagickFalse)
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
