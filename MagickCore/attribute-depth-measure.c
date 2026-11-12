/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%         AAA   TTTTT  TTTTT  RRRR   IIIII  BBBB   U   U  TTTTT  EEEEE        %
%        A   A    T      T    R   R    I    B   B  U   U    T    E            %
%        AAAAA    T      T    RRRR     I    BBBB   U   U    T    EEE          %
%        A   A    T      T    R R      I    B   B  U   U    T    E            %
%        A   A    T      T    R  R   IIIII  BBBB    UUU     T    EEEEE        %
%                                                                             %
%                      DDDD   EEEEE  PPPP   TTTTT  H   H                      %
%                      D   D  E      P   P    T    H   H                      %
%                      D   D  EEE    PPPP     T    HHHHH                      %
%                      D   D  E      P        T    H   H                      %
%                      DDDD   EEEEE  P        T    H   H                      %
%                                                                             %
%               M   M  EEEEE   AAA   SSSSS  U   U  RRRR   EEEEE               %
%               MM MM  E      A   A  SS     U   U  R   R  E                   %
%               M M M  EEE    AAAAA   SSS   U   U  RRRR   EEE                 %
%               M   M  E      A   A     SS  U   U  R R    E                   %
%               M   M  EEEEE  A   A  SSSSS   UUU   R  R   EEEEE               %
%                                                                             %
%                                                                             %
%                     MagickCore Image Depth Measurement                      %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                October 2002                                 %
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
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colormap-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/draw-private.h"
#include "MagickCore/effect.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/histogram.h"
#include "MagickCore/identify.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/magick.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/segment.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e D e p t h                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageDepth() returns the depth of a particular image channel.
%
%  The format of the GetImageDepth method is:
%
%      size_t GetImageDepth(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static size_t GetMaximumDepth(const size_t *current_depth,
  const size_t number_threads)
{
  size_t
    depth;

  ssize_t
    i;

  depth=current_depth[0];
  for (i=1; i < (ssize_t) number_threads; i++)
    if (depth < current_depth[i])
      depth=current_depth[i];
  return(depth);
}

static MagickBooleanType CheckColormapValueDepth(const MagickRealType value,
  const QuantumAny range,MagickBooleanType atDepth)
{
  if (IsPixelAtDepth(ClampToQuantum(value),range) == MagickFalse)
    atDepth=MagickFalse;
  return(atDepth);
}

static void RaiseColormapEntryDepth(const Image *image,size_t *current_depth,
  const int id,const ssize_t i)
{
  while (current_depth[id] < MAGICKCORE_QUANTUM_DEPTH)
  {
    MagickBooleanType
      atDepth;

    QuantumAny
      range;

    atDepth=MagickTrue;
    range=GetQuantumRange(current_depth[id]);
    if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
      atDepth=CheckColormapValueDepth(image->colormap[i].red,range,atDepth);
    if ((atDepth != MagickFalse) &&
        (GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
      atDepth=CheckColormapValueDepth(image->colormap[i].green,range,atDepth);
    if ((atDepth != MagickFalse) &&
        (GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
      atDepth=CheckColormapValueDepth(image->colormap[i].blue,range,atDepth);
    if ((atDepth != MagickFalse))
      break;
    current_depth[id]++;
  }
}

static size_t GetColormapDepth(const Image *image,size_t *current_depth,
  const size_t number_threads)
{
  size_t
    depth;

  ssize_t
    i;

  for (i=0; i < (ssize_t) image->colors; i++)
  {
    const int
      id = GetOpenMPThreadId();

    RaiseColormapEntryDepth(image,current_depth,id,i);
  }
  depth=GetMaximumDepth(current_depth,number_threads);
  current_depth=(size_t *) RelinquishMagickMemory(current_depth);
  return(depth);
}

static void RaiseSampleDepth(const Quantum *p,const ssize_t j,
  size_t *current_depth,const int id)
{
  while (current_depth[id] < MAGICKCORE_QUANTUM_DEPTH)
  {
    QuantumAny
      range;

    range=GetQuantumRange(current_depth[id]);
    if (p[j] == ScaleAnyToQuantum(ScaleQuantumToAny(p[j],range),range))
      break;
    current_depth[id]++;
  }
}

static void RaisePixelDepth(const Image *image,const Quantum *p,
  size_t *current_depth,const int id)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
  {
    PixelChannel
      channel;

    PixelTrait
      traits;

    channel=GetPixelChannelChannel(image,j);
    traits=GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    RaiseSampleDepth(p,j,current_depth,id);
  }
}

typedef struct _DepthScan
{
  const Image
    *image;

  CacheView
    *image_view;

  ssize_t
    y;

  ExceptionInfo
    *exception;

  size_t
    *current_depth;

  int
    id;
} DepthScan;

static MagickBooleanType RaiseImageRowDepth(const DepthScan *scan)
{
  const Quantum
    *magick_restrict p;

  MagickBooleanType
    status;

  ssize_t
    x;

  status=MagickTrue;
  p=GetCacheViewVirtualPixels(scan->image_view,0,scan->y,scan->image->columns,
    1,scan->exception);
  if (p == (const Quantum *) NULL)
    return(status);
  for (x=0; x < (ssize_t) scan->image->columns; x++)
  {
    RaisePixelDepth(scan->image,p,scan->current_depth,scan->id);
    p+=(ptrdiff_t) GetPixelChannels(scan->image);
  }
  if (scan->current_depth[scan->id] == MAGICKCORE_QUANTUM_DEPTH)
    status=MagickFalse;
  return(status);
}

static inline MagickBooleanType IsOpaqueColormapImage(const Image *image)
{
  return(((image->storage_class == PseudoClass) &&
    ((image->alpha_trait & BlendPixelTrait) == 0)) ? MagickTrue :
    MagickFalse);
}

static void RaiseImageRowDepthStatus(MagickBooleanType *status,
  const DepthScan *scan)
{
  if ((*status) == MagickFalse)
    return;
  if (RaiseImageRowDepth(scan) == MagickFalse)
    (*status)=MagickFalse;
}

#if !defined(MAGICKCORE_HDRI_SUPPORT)
typedef struct _DepthMapScan
{
  const Image
    *image;

  CacheView
    *image_view;

  size_t
    *depth_map;

  ExceptionInfo
    *exception;

  size_t
    *current_depth;
} DepthMapScan;

static void RaiseMappedPixelDepth(const DepthMapScan *scan,const Quantum *p,
  const int id)
{
  const Image
    *const image = scan->image;

  size_t
    *const current_depth = scan->current_depth,
    *const depth_map = scan->depth_map;

  ssize_t
    j;

  for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,j);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    if (depth_map[ScaleQuantumToMap(p[j])] > current_depth[id])
      current_depth[id]=depth_map[ScaleQuantumToMap(p[j])];
  }
}

static size_t *AcquireDepthMap(void)
{
  size_t
    *depth_map,
    depth;

  ssize_t
    i;

  depth_map=(size_t *) AcquireQuantumMemory(MaxMap+1,sizeof(*depth_map));
  if (depth_map == (size_t *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  for (i=0; i <= (ssize_t) MaxMap; i++)
  {
    for (depth=1; depth < (size_t) MAGICKCORE_QUANTUM_DEPTH; depth++)
    {
      Quantum
        pixel;

      QuantumAny
        range;

      range=GetQuantumRange(depth);
      pixel=(Quantum) i;
      if (pixel == ScaleAnyToQuantum(ScaleQuantumToAny(pixel,range),range))
        break;
    }
    depth_map[i]=depth;
  }
  return(depth_map);
}

static void RaiseMappedRowDepth(MagickBooleanType *status,
  const DepthMapScan *scan,const ssize_t y)
{
  const int
    id = GetOpenMPThreadId();

  const Quantum
    *magick_restrict p;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(scan->image_view,0,y,scan->image->columns,1,
    scan->exception);
  if (p == (const Quantum *) NULL)
    return;
  for (x=0; x < (ssize_t) scan->image->columns; x++)
  {
    RaiseMappedPixelDepth(scan,p,id);
    p+=(ptrdiff_t) GetPixelChannels(scan->image);
  }
  if (scan->current_depth[id] == MAGICKCORE_QUANTUM_DEPTH)
    (*status)=MagickFalse;
}

typedef struct _DepthMapImage
{
  const Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;

  size_t
    *current_depth,
    number_threads;

  MagickBooleanType
    status;
} DepthMapImage;

static size_t GetMappedImageDepth(const DepthMapImage *map_image)
{
  CacheView
    *image_view = map_image->image_view;

  const Image
    *const image = map_image->image;

  ExceptionInfo
    *const exception = map_image->exception;

  MagickBooleanType
    status = map_image->status;

  const size_t
    number_threads = map_image->number_threads;

  size_t
    *current_depth = map_image->current_depth,
    *depth_map,
    depth;

  ssize_t
    i,
    y;

  /*
    Scale pixels to desired (optimized with depth map).
  */
  depth_map=AcquireDepthMap();
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const DepthMapScan
      scan = { image, image_view, depth_map, exception, current_depth };

    RaiseMappedRowDepth(&status,&scan,y);
  }
  image_view=DestroyCacheView(image_view);
  depth=current_depth[0];
  for (i=1; i < (ssize_t) number_threads; i++)
    if (depth < current_depth[i])
      depth=current_depth[i];
  depth_map=(size_t *) RelinquishMagickMemory(depth_map);
  current_depth=(size_t *) RelinquishMagickMemory(current_depth);
  return(depth);
}
#endif

MagickExport size_t GetImageDepth(const Image *image,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  ssize_t
    i;

  size_t
    *current_depth,
    depth,
    number_threads;

  ssize_t
    y;

  /*
    Compute image depth.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  number_threads=(size_t) GetMagickResourceLimit(ThreadResource);
  current_depth=(size_t *) AcquireQuantumMemory(number_threads,
    sizeof(*current_depth));
  if (current_depth == (size_t *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  status=MagickTrue;
  for (i=0; i < (ssize_t) number_threads; i++)
    current_depth[i]=1;
  if (IsOpaqueColormapImage(image) != MagickFalse)
    {
      return(GetColormapDepth(image,current_depth,number_threads));
    }
  image_view=AcquireVirtualCacheView(image,exception);
#if !defined(MAGICKCORE_HDRI_SUPPORT)
  DisableMSCWarning(4127)
  if ((1UL*QuantumRange) <= MaxMap)
  RestoreMSCWarning
    {
      DepthMapImage
        map_image;

      map_image.image=image;
      map_image.image_view=image_view;
      map_image.exception=exception;
      map_image.current_depth=current_depth;
      map_image.number_threads=number_threads;
      map_image.status=status;
      return(GetMappedImageDepth(&map_image));
    }
#endif
  /*
    Compute pixel depth.
  */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const int
      id = GetOpenMPThreadId();

    DepthScan
      scan;

    scan.image=image;
    scan.image_view=image_view;
    scan.y=y;
    scan.exception=exception;
    scan.current_depth=current_depth;
    scan.id=id;
    RaiseImageRowDepthStatus(&status,&scan);
  }
  image_view=DestroyCacheView(image_view);
  depth=GetMaximumDepth(current_depth,number_threads);
  current_depth=(size_t *) RelinquishMagickMemory(current_depth);
  return(depth);
}
