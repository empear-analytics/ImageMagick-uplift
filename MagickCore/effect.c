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
    MagickRealType
      v;

    ssize_t
      i,
      x;

    i=(2*y+1)+y*(ssize_t) columns;
    if (polarity > 0)
      for (x=0; x < (ssize_t) columns; x++)
      {
        v=(MagickRealType) p[i];
        v=RaiseHullValue(r,i,v);
        q[i]=(Quantum) v;
        i++;
      }
    else
      for (x=0; x < (ssize_t) columns; x++)
      {
        v=(MagickRealType) p[i];
        v=LowerHullValue(r,i,v);
        q[i]=(Quantum) v;
        i++;
      }
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
    ssize_t
      i,
      x;

    MagickRealType
      v;

    i=(2*y+1)+y*(ssize_t) columns;
    if (polarity > 0)
      for (x=0; x < (ssize_t) columns; x++)
      {
        v=(MagickRealType) q[i];
        v=RaiseHullEdgeValue(r,s,i,v);
        p[i]=(Quantum) v;
        i++;
      }
    else
      for (x=0; x < (ssize_t) columns; x++)
      {
        v=(MagickRealType) q[i];
        v=LowerHullEdgeValue(r,s,i,v);
        p[i]=(Quantum) v;
        i++;
      }
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

MagickExport Image *DespeckleImage(const Image *image,ExceptionInfo *exception)
{
  CacheView
    *despeckle_view,
    *image_view;

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

  ssize_t
    i;

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
      if (buffer_info != (MemoryInfo *) NULL)
        buffer_info=RelinquishVirtualMemory(buffer_info);
      if (pixel_info != (MemoryInfo *) NULL)
        pixel_info=RelinquishVirtualMemory(pixel_info);
      despeckle_image=DestroyImage(despeckle_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  pixels=(Quantum *) GetVirtualMemoryBlob(pixel_info);
  buffer=(Quantum *) GetVirtualMemoryBlob(buffer_info);
  /*
    Reduce speckle in the image.
  */
  status=MagickTrue;
  image_view=AcquireVirtualCacheView(image,exception);
  despeckle_view=AcquireAuthenticCacheView(despeckle_image,exception);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    DespeckleLoad
      load;

    DespeckleStore
      store;

    HullBuffers
      hull;

    PixelChannel
       channel;

    PixelTrait
      despeckle_traits,
      traits;

    ssize_t
      k;

    if (status == MagickFalse)
      continue;
    channel=GetPixelChannelChannel(image,i);
    traits=GetPixelChannelTraits(image,channel);
    despeckle_traits=GetPixelChannelTraits(despeckle_image,channel);
    if ((traits == UndefinedPixelTrait) ||
        (despeckle_traits == UndefinedPixelTrait))
      continue;
    if ((despeckle_traits & CopyPixelTrait) != 0)
      continue;
    (void) memset(pixels,0,length*sizeof(*pixels));
    load.image=image;
    load.image_view=image_view;
    load.pixels=pixels;
    load.i=i;
    status=LoadDespeckleChannel(&load,status,exception);
    (void) memset(buffer,0,length*sizeof(*buffer));
    for (k=0; k < 4; k++)
    {
      hull.image=image;
      hull.columns=image->columns;
      hull.rows=image->rows;
      hull.f=pixels;
      hull.g=buffer;
      Hull(&hull,X[k],Y[k],1);
      hull.image=image;
      hull.columns=image->columns;
      hull.rows=image->rows;
      hull.f=pixels;
      hull.g=buffer;
      Hull(&hull,-X[k],-Y[k],1);
      hull.image=image;
      hull.columns=image->columns;
      hull.rows=image->rows;
      hull.f=pixels;
      hull.g=buffer;
      Hull(&hull,-X[k],-Y[k],-1);
      hull.image=image;
      hull.columns=image->columns;
      hull.rows=image->rows;
      hull.f=pixels;
      hull.g=buffer;
      Hull(&hull,X[k],Y[k],-1);
    }
    store.image=image;
    store.despeckle_image=despeckle_image;
    store.despeckle_view=despeckle_view;
    store.channel=channel;
    store.pixels=pixels;
    status=StoreDespeckleChannel(&store,status,exception);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

        proceed=SetImageProgress(image,DespeckleImageTag,(MagickOffsetType) i,
          GetPixelChannels(image));
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  despeckle_view=DestroyCacheView(despeckle_view);
  image_view=DestroyCacheView(image_view);
  buffer_info=RelinquishVirtualMemory(buffer_info);
  pixel_info=RelinquishVirtualMemory(pixel_info);
  despeckle_image->type=image->type;
  if (status == MagickFalse)
    despeckle_image=DestroyImage(despeckle_image);
  return(despeckle_image);
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
  kuwahara_image=CloneImage(image,0,0,MagickTrue,exception);
  if (kuwahara_image == (Image *) NULL)
    {
      gaussian_image=DestroyImage(gaussian_image);
      return((Image *) NULL);
    }
  if (SetImageStorageClass(kuwahara_image,DirectClass,exception) == MagickFalse)
    {
      gaussian_image=DestroyImage(gaussian_image);
      kuwahara_image=DestroyImage(kuwahara_image);
      return((Image *) NULL);
    }
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
    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=QueueCacheViewAuthenticPixels(kuwahara_view,0,y,kuwahara_image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
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
          status=MagickFalse;
          break;
        }
      status=InterpolatePixelChannels(gaussian_image,image_view,kuwahara_image,
        UndefinedInterpolatePixel,(double) target.x+target.width/2.0,(double)
        target.y+target.height/2.0,q,exception);
      if (status == MagickFalse)
        break;
      q+=(ptrdiff_t) GetPixelChannels(kuwahara_image);
    }
    if (SyncCacheViewAuthenticPixels(kuwahara_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,KuwaharaImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  kuwahara_view=DestroyCacheView(kuwahara_view);
  image_view=DestroyCacheView(image_view);
  gaussian_image=DestroyImage(gaussian_image);
  if (status == MagickFalse)
    kuwahara_image=DestroyImage(kuwahara_image);
  return(kuwahara_image);
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
  contrast_image=CloneImage(image,0,0,MagickTrue,exception);
  if (contrast_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(contrast_image,DirectClass,exception) == MagickFalse)
    {
      contrast_image=DestroyImage(contrast_image);
      return((Image *) NULL);
    }
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
      const int
        id = GetOpenMPThreadId();

      const Quantum
        *magick_restrict p;

      float
        *out,
        *pix,
        *pixels;

      ssize_t
        y;

      if (status == MagickFalse)
        continue;
      pixels=scanline;
      pixels+=id*scanLineSize;
      pix=pixels;
      p=GetCacheViewVirtualPixels(image_view,x,-(ssize_t) width,1,
        image->rows+(size_t) (2*width),exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          continue;
        }
      LoadLocalContrastLuma(image,p,pix,width);
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
      const int
        id = GetOpenMPThreadId();

      const Quantum
        *magick_restrict p;

      float
        *pixels;

      LocalContrastPixel
        contrast;

      Quantum
        *magick_restrict q;

      ssize_t
        x;

      if (status == MagickFalse)
        continue;
      pixels=scanline;
      pixels+=id*scanLineSize;
      p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
      q=GetCacheViewAuthenticPixels(contrast_view,0,y,image->columns,1,
        exception);
      if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
        {
          status=MagickFalse;
          continue;
        }
      memcpy(pixels,interImage+((size_t) y*(image->columns+(size_t) (2*width))),
        (image->columns+(size_t) (2*width))*sizeof(float));
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
      if (SyncCacheViewAuthenticPixels(contrast_view,exception) == MagickFalse)
        status=MagickFalse;
    }
  }
  scanline_info=RelinquishVirtualMemory(scanline_info);
  interImage_info=RelinquishVirtualMemory(interImage_info);
  contrast_view=DestroyCacheView(contrast_view);
  image_view=DestroyCacheView(image_view);
  if (status == MagickFalse)
    contrast_image=DestroyImage(contrast_image);
  return(contrast_image);
}


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

MagickExport Image *PreviewImage(const Image *image,const PreviewType preview,
  ExceptionInfo *exception)
{
#define NumberTiles  9
#define PreviewImageTag  "Preview/Image"
#define DefaultPreviewGeometry  "204x204+10+10"

  char
    factor[MagickPathExtent],
    label[MagickPathExtent];

  double
    degrees,
    gamma,
    percentage,
    radius,
    sigma,
    threshold;

  Image
    *images,
    *montage_image,
    *preview_image,
    *thumbnail;

  ImageInfo
    *preview_info;

  MagickBooleanType
    proceed;

  MontageInfo
    *montage_info;

  QuantizeInfo
    quantize_info;

  RectangleInfo
    geometry;

  size_t
    colors;

  ssize_t
    i,
    x = 0,
    y = 0;

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
  images=NewImageList();
  percentage=12.5;
  GetQuantizeInfo(&quantize_info);
  radius=0.0;
  sigma=1.0;
  threshold=0.0;
  for (i=0; i < NumberTiles; i++)
  {
    thumbnail=ThumbnailImage(image,geometry.width,geometry.height,exception);
    if (thumbnail == (Image *) NULL)
      break;
    (void) SetImageProgressMonitor(thumbnail,(MagickProgressMonitor) NULL,
      (void *) NULL);
    (void) SetImageProperty(thumbnail,"label",DefaultTileLabel,exception);
    if (i == (NumberTiles/2))
      {
        (void) QueryColorCompliance("#dfdfdf",AllCompliance,
          &thumbnail->matte_color,exception);
        AppendImageToList(&images,thumbnail);
        continue;
      }
    switch (preview)
    {
      case RotatePreview:
      {
        degrees+=45.0;
        preview_image=RotateImage(thumbnail,degrees,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"rotate %g",degrees);
        break;
      }
      case ShearPreview:
      {
        degrees+=5.0;
        preview_image=ShearImage(thumbnail,degrees,degrees,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"shear %gx%g",degrees,
          2.0*degrees);
        break;
      }
      case RollPreview:
      {
        x=((i+1)*(ssize_t) thumbnail->columns)/NumberTiles;
        y=((i+1)*(ssize_t) thumbnail->rows)/NumberTiles;
        preview_image=RollImage(thumbnail,x,y,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"roll %+.20gx%+.20g",
          (double) x,(double) y);
        break;
      }
      case HuePreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) FormatLocaleString(factor,MagickPathExtent,"100,100,%g",2.0*
          percentage);
        (void) ModulateImage(preview_image,factor,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"modulate %s",factor);
        break;
      }
      case SaturationPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) FormatLocaleString(factor,MagickPathExtent,"100,%g",2.0*
          percentage);
        (void) ModulateImage(preview_image,factor,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"modulate %s",factor);
        break;
      }
      case BrightnessPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) FormatLocaleString(factor,MagickPathExtent,"%g",2.0*percentage);
        (void) ModulateImage(preview_image,factor,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"modulate %s",factor);
        break;
      }
      case GammaPreview:
      default:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        gamma+=0.4;
        (void) GammaImage(preview_image,gamma,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"gamma %g",gamma);
        break;
      }
      case SpiffPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image != (Image *) NULL)
          SharpenContrastRepeatedly(preview_image,i,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"contrast (%.17g)",
          (double) i+1);
        break;
      }
      case DullPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        for (x=0; x < i; x++)
          (void) ContrastImage(preview_image,MagickFalse,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"+contrast (%.17g)",
          (double) i+1);
        break;
      }
      case GrayscalePreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        colors<<=1;
        quantize_info.number_colors=colors;
        quantize_info.colorspace=GRAYColorspace;
        (void) QuantizeImage(&quantize_info,preview_image,exception);
        (void) FormatLocaleString(label,MagickPathExtent,
          "-colorspace gray -colors %.17g",(double) colors);
        break;
      }
      case QuantizePreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        colors<<=1;
        quantize_info.number_colors=colors;
        (void) QuantizeImage(&quantize_info,preview_image,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"colors %.17g",
          (double) colors);
        break;
      }
      case DespecklePreview:
      {
        thumbnail=DespeckleThumbnail(thumbnail,i,exception);
        preview_image=DespeckleImage(thumbnail,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) FormatLocaleString(label,MagickPathExtent,"despeckle (%.17g)",
          (double) i+1);
        break;
      }
      case ReduceNoisePreview:
      {
        preview_image=StatisticImage(thumbnail,NonpeakStatistic,(size_t)
          radius,(size_t) radius,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"noise %g",radius);
        break;
      }
      case AddNoisePreview:
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
        preview_image=StatisticImage(thumbnail,NonpeakStatistic,(size_t) i,
          (size_t) i,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"+noise %s",factor);
        break;
      }
      case SharpenPreview:
      {
        preview_image=SharpenImage(thumbnail,radius,sigma,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"sharpen %gx%g",
          radius,sigma);
        break;
      }
      case BlurPreview:
      {
        preview_image=BlurImage(thumbnail,radius,sigma,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"blur %gx%g",radius,
          sigma);
        break;
      }
      case ThresholdPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) BilevelImage(thumbnail,(double) (percentage*((double)
          QuantumRange+1.0))/100.0,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"threshold %g",
          (double) (percentage*((double) QuantumRange+1.0))/100.0);
        break;
      }
      case EdgeDetectPreview:
      {
        preview_image=EdgeImage(thumbnail,radius,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"edge %g",radius);
        break;
      }
      case SpreadPreview:
      {
        preview_image=SpreadImage(thumbnail,image->interpolate,radius,
          exception);
        (void) FormatLocaleString(label,MagickPathExtent,"spread %g",
          radius+0.5);
        break;
      }
      case SolarizePreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) SolarizeImage(preview_image,(double) QuantumRange*percentage/
          100.0,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"solarize %g",
          ((double) QuantumRange*percentage)/100.0);
        break;
      }
      case ShadePreview:
      {
        degrees+=10.0;
        preview_image=ShadeImage(thumbnail,MagickTrue,degrees,degrees,
          exception);
        (void) FormatLocaleString(label,MagickPathExtent,"shade %gx%g",degrees,
          degrees);
        break;
      }
      case RaisePreview:
      {
        RectangleInfo
          raise;

        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        raise.width=(size_t) (2*i+2);
        raise.height=(size_t) (2*i+2);
        raise.x=(i-1)/2;
        raise.y=(i-1)/2;
        (void) RaiseImage(preview_image,&raise,MagickTrue,exception);
        (void) FormatLocaleString(label,MagickPathExtent,
          "raise %.17gx%.17g%+.20g%+.20g",(double) raise.width,(double)
          raise.height,(double) raise.x,(double) raise.y);
        break;
      }
      case SegmentPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        threshold+=0.4;
        (void) SegmentImage(preview_image,sRGBColorspace,MagickFalse,threshold,
          threshold,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"segment %gx%g",
          threshold,threshold);
        break;
      }
      case SwirlPreview:
      {
        preview_image=SwirlImage(thumbnail,degrees,image->interpolate,
          exception);
        (void) FormatLocaleString(label,MagickPathExtent,"swirl %g",degrees);
        degrees+=45.0;
        break;
      }
      case ImplodePreview:
      {
        degrees+=0.1;
        preview_image=ImplodeImage(thumbnail,degrees,image->interpolate,
          exception);
        (void) FormatLocaleString(label,MagickPathExtent,"implode %g",degrees);
        break;
      }
      case WavePreview:
      {
        degrees+=5.0;
        preview_image=WaveImage(thumbnail,0.5*degrees,2.0*degrees,
          image->interpolate,exception);
        (void) FormatLocaleString(label,MagickPathExtent,"wave %gx%g",0.5*
          degrees,2.0*degrees);
        break;
      }
      case OilPaintPreview:
      {
        preview_image=OilPaintImage(thumbnail,(double) radius,(double) sigma,
          exception);
        (void) FormatLocaleString(label,MagickPathExtent,"charcoal %gx%g",
          radius,sigma);
        break;
      }
      case CharcoalDrawingPreview:
      {
        preview_image=CharcoalImage(thumbnail,(double) radius,(double) sigma,
          exception);
        (void) FormatLocaleString(label,MagickPathExtent,"charcoal %gx%g",
          radius,sigma);
        break;
      }
      case JPEGPreview:
      {
        char
          filename[MagickPathExtent];

        int
          file;

        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        preview_info->quality=(size_t) percentage;
        (void) FormatLocaleString(factor,MagickPathExtent,"%.17g",(double)
          preview_info->quality);
        file=AcquireUniqueFileResource(filename);
        if (file != -1)
          file=close_utf8(file)-1;
        (void) FormatLocaleString(preview_image->filename,MagickPathExtent,
          "jpeg:%s",filename);
        preview_image=WriteAndReadQualityPreview(preview_info,preview_image,
          exception);
        (void) RelinquishUniqueFileResource(preview_image->filename);
        FormatQualityLabel(label,factor,preview_image,thumbnail);
        break;
      }
    }
    thumbnail=DestroyImage(thumbnail);
    percentage+=12.5;
    radius+=0.5;
    sigma+=0.25;
    if (preview_image == (Image *) NULL)
      break;
    preview_image->alpha_trait=UndefinedPixelTrait;
    (void) DeleteImageProperty(preview_image,"label");
    (void) SetImageProperty(preview_image,"label",label,exception);
    AppendImageToList(&images,preview_image);
    proceed=SetImageProgress(image,PreviewImageTag,(MagickOffsetType) i,
      NumberTiles);
    if (proceed == MagickFalse)
      break;
  }
  if (images == (Image *) NULL)
    {
      preview_info=DestroyImageInfo(preview_info);
      return((Image *) NULL);
    }
  /*
    Create the montage.
  */
  montage_info=CloneMontageInfo(preview_info,(MontageInfo *) NULL);
  (void) CopyMagickString(montage_info->filename,image->filename,
    MagickPathExtent);
  montage_info->shadow=MagickTrue;
  (void) CloneString(&montage_info->tile,"3x3");
  (void) CloneString(&montage_info->geometry,DefaultPreviewGeometry);
  (void) CloneString(&montage_info->frame,DefaultTileFrame);
  montage_image=MontageImages(images,montage_info,exception);
  montage_info=DestroyMontageInfo(montage_info);
  images=DestroyImageList(images);
  if (montage_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
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
  preview_info=DestroyImageInfo(preview_info);
  return(montage_image);
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

MagickExport Image *ShadeImage(const Image *image,const MagickBooleanType gray,
  const double azimuth,const double elevation,ExceptionInfo *exception)
{
#define GetShadeIntensity(image,pixel) \
  ClampPixel(GetPixelIntensity((image),(pixel)))
#define ShadeImageTag  "Shade/Image"

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
  linear_image=CloneImage(image,0,0,MagickTrue,exception);
  shade_image=CloneImage(image,0,0,MagickTrue,exception);
  if ((linear_image == (Image *) NULL) || (shade_image == (Image *) NULL))
    {
      if (linear_image != (Image *) NULL)
        linear_image=DestroyImage(linear_image);
      if (shade_image != (Image *) NULL)
        shade_image=DestroyImage(shade_image);
      return((Image *) NULL);
    }
  if (SetImageStorageClass(shade_image,DirectClass,exception) == MagickFalse)
    {
      linear_image=DestroyImage(linear_image);
      shade_image=DestroyImage(shade_image);
      return((Image *) NULL);
    }
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
    double
      shade;

    PrimaryInfo
      normal;

    const ShadeImages
      shading = { linear_image, shade_image, gray };

    const Quantum
      *magick_restrict center,
      *magick_restrict p,
      *magick_restrict post,
      *magick_restrict pre;

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,-1,y-1,linear_image->columns+2,3,
      exception);
    q=QueueCacheViewAuthenticPixels(shade_view,0,y,shade_image->columns,1,
      exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    /*
      Shade this row of pixels.
    */
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
    if (SyncCacheViewAuthenticPixels(shade_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,ShadeImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  shade_view=DestroyCacheView(shade_view);
  image_view=DestroyCacheView(image_view);
  linear_image=DestroyImage(linear_image);
  if (status == MagickFalse)
    shade_image=DestroyImage(shade_image);
  return(shade_image);
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
  spread_image=CloneImage(image,0,0,MagickTrue,exception);
  if (spread_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(spread_image,DirectClass,exception) == MagickFalse)
    {
      spread_image=DestroyImage(spread_image);
      return((Image *) NULL);
    }
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

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=QueueCacheViewAuthenticPixels(spread_view,0,y,spread_image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      PointInfo
        point;

      point.x=GetPseudoRandomValue(random_info[id]);
      point.y=GetPseudoRandomValue(random_info[id]);
      status=InterpolatePixelChannels(image,image_view,spread_image,method,
        (double) x+width*(point.x-0.5),(double) y+width*(point.y-0.5),q,
        exception);
      if (status == MagickFalse)
        break;
      q+=(ptrdiff_t) GetPixelChannels(spread_image);
    }
    if (SyncCacheViewAuthenticPixels(spread_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,SpreadImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  spread_view=DestroyCacheView(spread_view);
  image_view=DestroyCacheView(image_view);
  random_info=DestroyRandomInfoTLS(random_info);
  if (status == MagickFalse)
    spread_image=DestroyImage(spread_image);
  return(spread_image);
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
    const Quantum
      *magick_restrict p;

    const UnsharpPixels
      unsharp = { image, unsharp_image, quantum_threshold, gain };

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    q=GetCacheViewAuthenticPixels(unsharp_view,0,y,unsharp_image->columns,1,
      exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      UnsharpPixelChannels(&unsharp,p,q);
      p+=(ptrdiff_t) GetPixelChannels(image);
      q+=(ptrdiff_t) GetPixelChannels(unsharp_image);
    }
    if (SyncCacheViewAuthenticPixels(unsharp_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,SharpenImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  unsharp_image->type=image->type;
  unsharp_view=DestroyCacheView(unsharp_view);
  image_view=DestroyCacheView(image_view);
  if (status == MagickFalse)
    unsharp_image=DestroyImage(unsharp_image);
  return(unsharp_image);
}
