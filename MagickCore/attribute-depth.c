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
%                                                                             %
%                       MagickCore Image Depth Methods                        %
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
      if (IsPixelAtDepth(ClampToQuantum(image->colormap[i].red),range) == MagickFalse)
        atDepth=MagickFalse;
    if ((atDepth != MagickFalse) &&
        (GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
      if (IsPixelAtDepth(ClampToQuantum(image->colormap[i].green),range) == MagickFalse)
        atDepth=MagickFalse;
    if ((atDepth != MagickFalse) &&
        (GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
      if (IsPixelAtDepth(ClampToQuantum(image->colormap[i].blue),range) == MagickFalse)
        atDepth=MagickFalse;
    if ((atDepth != MagickFalse))
      break;
    current_depth[id]++;
  }
}

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
  if ((image->storage_class == PseudoClass) &&
      ((image->alpha_trait & BlendPixelTrait) == 0))
    {
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
  image_view=AcquireVirtualCacheView(image,exception);
#if !defined(MAGICKCORE_HDRI_SUPPORT)
  DisableMSCWarning(4127)
  if ((1UL*QuantumRange) <= MaxMap)
  RestoreMSCWarning
    {
      size_t
        *depth_map;

      /*
        Scale pixels to desired (optimized with depth map).
      */
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
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(image,image,image->rows,1)
#endif
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        const int
          id = GetOpenMPThreadId();

        const Quantum
          *magick_restrict p;

        ssize_t
          x;

        if (status == MagickFalse)
          continue;
        p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
        if (p == (const Quantum *) NULL)
          continue;
        for (x=0; x < (ssize_t) image->columns; x++)
        {
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
          p+=(ptrdiff_t) GetPixelChannels(image);
        }
        if (current_depth[id] == MAGICKCORE_QUANTUM_DEPTH)
          status=MagickFalse;
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

    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      continue;
    for (x=0; x < (ssize_t) image->columns; x++)
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
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (current_depth[id] == MAGICKCORE_QUANTUM_DEPTH)
      status=MagickFalse;
  }
  image_view=DestroyCacheView(image_view);
  depth=GetMaximumDepth(current_depth,number_threads);
  current_depth=(size_t *) RelinquishMagickMemory(current_depth);
  return(depth);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e D e p t h                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageDepth() sets the depth of the image.
%
%  The format of the SetImageDepth method is:
%
%      MagickBooleanType SetImageDepth(Image *image,const size_t depth,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o channel: the channel.
%
%    o depth: the image depth.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickBooleanType FloydSteinbergImageDepth(Image *image,
  const size_t depth,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  double
    *distortion;

  MagickBooleanType
    status;

  QuantumAny
    range;

  size_t
    channels;

  ssize_t
    y;

  /*
    Dither pixels with Floyd Steinberg algorithm.
  */
  status=SetImageStorageClass(image,DirectClass,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  channels=GetPixelChannels(image);
  distortion=(double *) AcquireQuantumMemory(image->columns,3*channels*
    sizeof(*distortion));
  if (distortion == (double *) NULL)
    return(MagickFalse);
  (void) memset(distortion,0,3*image->columns*channels*sizeof(*distortion));
  range=GetQuantumRange(depth);
  image_view=AcquireAuthenticCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    ssize_t
      u,
      v,
      x;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    /*
      Reset pixel distortion for current row.
    */
    u=(y % 3)*(ssize_t) (image->columns*channels);
    (void) memset(distortion+u,0,image->columns*channels*sizeof(*distortion));
    v=((y+1) % 3)*(ssize_t) (image->columns*channels);
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      ssize_t
        i;

      for (i=0; i < (ssize_t) channels; i++)
      {
        double
          error,
          pixel;

        PixelChannel
          channel;

        PixelTrait
          traits;

        /*
          Add distortion to current pixel then distribute new distortion.
        */
        channel=GetPixelChannelChannel(image,i);
        traits=GetPixelChannelTraits(image,channel);
        if ((traits & UpdatePixelTrait) == 0)
          {
            u++;
            v++;
            continue;
          }
        pixel=(double) q[i]+distortion[u];
        q[i]=ScaleAnyToQuantum(ScaleQuantumToAny(ClampPixel((MagickRealType)
          pixel),range),range);
        /*
          Distribute distortion for right.
        */
        error=pixel-(double) q[i];
        if ((x+1) < (ssize_t) image->columns)
          distortion[u+(ssize_t) channels]+=7.0*error/16.0;
        if ((y+1) < (ssize_t) image->rows)
          {
            /*
              Distribute distortion for bottom left, bottom, and bottom right.
            */
            if (x > 0)
              distortion[v-(ssize_t) channels]+=3.0*error/16.0;
            distortion[v]+=5.0*error/16.0;
            if ((x+1) < (ssize_t) image->columns)
              distortion[v+(ssize_t) channels]+=1.0*error/16.0;
          }
        u++;
        v++;
      }
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      {
        status=MagickFalse;
        continue;
      }
  }
  image_view=DestroyCacheView(image_view);
  distortion=(double *) RelinquishMagickMemory(distortion);
  if (status != MagickFalse)
    image->depth=depth;
  return(status);
}

static void SetColormapEntryDepth(Image *image,const ssize_t i,
  const QuantumAny range)
{
  if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
    image->colormap[i].red=(double) ScaleAnyToQuantum(ScaleQuantumToAny(
      ClampPixel(image->colormap[i].red),range),range);
  if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
    image->colormap[i].green=(double) ScaleAnyToQuantum(ScaleQuantumToAny(
      ClampPixel(image->colormap[i].green),range),range);
  if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
    image->colormap[i].blue=(double) ScaleAnyToQuantum(ScaleQuantumToAny(
      ClampPixel(image->colormap[i].blue),range),range);
  if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
    image->colormap[i].alpha=(double) ScaleAnyToQuantum(ScaleQuantumToAny(
      ClampPixel(image->colormap[i].alpha),range),range);
}

MagickExport MagickBooleanType SetImageDepth(Image *image,
  const size_t depth,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  const char
    *artifact;

  MagickBooleanType
    status;

  QuantumAny
    range;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickCoreSignature);
  if (depth >= MAGICKCORE_QUANTUM_DEPTH)
    {
      image->depth=depth;
      return(MagickTrue);
    }
  artifact=GetImageArtifact(image,"dither");
  if ((artifact != (const char *) NULL) &&
      (LocaleCompare(artifact,"FloydSteinberg") == 0))
    return(FloydSteinbergImageDepth(image,depth,exception));
  range=GetQuantumRange(depth);
  if (image->storage_class == PseudoClass)
    {
      ssize_t
        i;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(image,image,image->colors,1)
#endif
      for (i=0; i < (ssize_t) image->colors; i++)
      {
        SetColormapEntryDepth(image,i,range);
      }
    }
  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
#if !defined(MAGICKCORE_HDRI_SUPPORT)
  DisableMSCWarning(4127)
  if ((1UL*QuantumRange) <= MaxMap)
  RestoreMSCWarning
    {
      Quantum
        *depth_map;

      ssize_t
        i;

      /*
        Scale pixels to desired (optimized with depth map).
      */
      depth_map=(Quantum *) AcquireQuantumMemory(MaxMap+1,sizeof(*depth_map));
      if (depth_map == (Quantum *) NULL)
        ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
      for (i=0; i <= (ssize_t) MaxMap; i++)
        depth_map[i]=ScaleAnyToQuantum(ScaleQuantumToAny((Quantum) i,range),
          range);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(image,image,image->rows,2)
#endif
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        ssize_t
          x;

        Quantum
          *magick_restrict q;

        if (status == MagickFalse)
          continue;
        q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
          exception);
        if (q == (Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        for (x=0; x < (ssize_t) image->columns; x++)
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
            q[j]=depth_map[ScaleQuantumToMap(q[j])];
          }
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
          {
            status=MagickFalse;
            continue;
          }
      }
      image_view=DestroyCacheView(image_view);
      depth_map=(Quantum *) RelinquishMagickMemory(depth_map);
      if (status != MagickFalse)
        image->depth=depth;
      return(status);
    }
#endif
  /*
    Scale pixels to desired depth.
  */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ssize_t
      x;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      ssize_t
        i;

      for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
      {
        PixelChannel
          channel;

        PixelTrait
          traits;

        channel=GetPixelChannelChannel(image,i);
        traits=GetPixelChannelTraits(image,channel);
        if ((traits & UpdatePixelTrait) == 0)
          continue;
        q[i]=ScaleAnyToQuantum(ScaleQuantumToAny(ClampPixel((MagickRealType)
          q[i]),range),range);
      }
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      {
        status=MagickFalse;
        continue;
      }
  }
  image_view=DestroyCacheView(image_view);
  if (status != MagickFalse)
    image->depth=depth;
  return(status);
}
