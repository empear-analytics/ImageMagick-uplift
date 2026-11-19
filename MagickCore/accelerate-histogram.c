/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     AAA    CCCC   CCCC  EEEEE  L      EEEEE  RRRR    AAA   TTTTT  EEEEE     %
%    A   A  C      C      E      L      E      R   R  A   A    T    E         %
%    AAAAA  C      C      EEE    L      EEE    RRRR   AAAAA    T    EEE       %
%    A   A  C      C      E      L      E      R R    A   A    T    E         %
%    A   A   CCCC   CCCC  EEEEE  LLLLL  EEEEE  R  R   A   A    T    EEEEE     %
%                                                                             %
%        H   H  IIIII  SSSSS  TTTTT   OOO    GGGG  RRRR    AAA   M   M        %
%        H   H    I    SS       T    O   O  G      R   R  A   A  MM MM        %
%        HHHHH    I     SSS     T    O   O  G  GG  RRRR   AAAAA  M M M        %
%        H   H    I       SS    T    O   O  G   G  R R    A   A  M   M        %
%        H   H  IIIII  SSSSS    T     OOO    GGG   R  R   A   A  M   M        %
%                                                                             %
%                                                                             %
%               MagickCore OpenCL Accelerated Histogram Methods               %
%                                                                             %
%                              Software Design                                %
%                                  Cristy                                     %
%                               SiuChi Chan                                   %
%                              Guansong Zhang                                 %
%                               January 2010                                  %
%                               Dirk Lemstra                                  %
%                                April 2016                                   %
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
*/

/*
Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/accelerate-private.h"
#include "MagickCore/artifact.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color-private.h"
#include "MagickCore/delegate-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/opencl.h"
#include "MagickCore/opencl-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/prepress.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/registry.h"
#include "MagickCore/resize.h"
#include "MagickCore/resize-private.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"
#include "MagickCore/accelerate-core-private.h"
#include "MagickCore/accelerate-histogram-private.h"
#if defined(MAGICKCORE_OPENCL_SUPPORT)


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A c c e l e r a t e E q u a l i z e I m a g e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

typedef struct _HistogramBufferInfo
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  cl_mem
    histogramBuffer;

  cl_uint4
    *histogram;
} HistogramBufferInfo;

static MagickBooleanType ReadHistogramBuffer(const HistogramBufferInfo *info,
  ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = info->clEnv;

  const MagickCLDevice
    device = info->device;

  const cl_command_queue
    queue = info->queue;

  const cl_mem
    histogramBuffer = info->histogramBuffer;

  cl_uint4
    *const histogram = info->histogram;

  cl_int
    clStatus;

  size_t
    length;

  /* read from the kernel output */
  if (ALIGNED(histogram,cl_uint4))
  {
    length = (MaxMap+1);
    clEnv->library->clEnqueueMapBuffer(queue, histogramBuffer, CL_TRUE, CL_MAP_READ|CL_MAP_WRITE, 0, length * sizeof(cl_uint4), 0, NULL, NULL, &clStatus);
  }
  else
  {
    length = (MaxMap+1);
    clStatus = clEnv->library->clEnqueueReadBuffer(queue, histogramBuffer, CL_TRUE, 0, length * sizeof(cl_uint4), histogram, 0, NULL, NULL);
  }
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "Reading output image from CL buffer failed.", ".");
    return(MagickFalse);
  }

  /* unmap, don't block gpu to use this buffer again.  */
  if (ALIGNED(histogram,cl_uint4))
  {
    clStatus = clEnv->library->clEnqueueUnmapMemObject(queue, histogramBuffer, histogram, 0, NULL, NULL);
    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clEnqueueUnmapMemObject failed.", ".");
      return(MagickFalse);
    }
  }
  return(MagickTrue);
}

typedef struct _EqualizeLevels
{
  cl_float4
    white,
    black;
} EqualizeLevels;

static void EqualizeColormapSync(Image *image,PixelPacket *equalize_map,
  const cl_uint i,const EqualizeLevels *levels)
{
  const cl_float4
    white = levels->white,
    black = levels->black;

  if (white.x != black.x)
  {
    image->colormap[i].red=equalize_map[
      ScaleQuantumToMap((const Quantum) image->colormap[i].red)].red;
    image->colormap[i].green=equalize_map[
      ScaleQuantumToMap((const Quantum) image->colormap[i].green)].red;
    image->colormap[i].blue=equalize_map[
      ScaleQuantumToMap((const Quantum) image->colormap[i].blue)].red;
    image->colormap[i].alpha=equalize_map[
      ScaleQuantumToMap((const Quantum) image->colormap[i].alpha)].red;
  }
}

static void IntegrateEqualizeHistogram(const Image *image,
  const cl_uint4 *histogram,cl_float4 *map)
{
  cl_float4
    intensity;

  cl_uint
    i;

  (void) memset(&intensity,0,sizeof(intensity));
  for (i=0; i <= (ssize_t) MaxMap; i++)
  {
    if ((image->channel_mask & SyncChannels) != 0)
    {
      intensity.x+=histogram[i].s[2];
      map[i]=intensity;
      continue;
    }
    if ((image->channel_mask & RedChannel) != 0)
      intensity.x+=histogram[i].s[2];
    if ((image->channel_mask & GreenChannel) != 0)
      intensity.y+=histogram[i].s[1];
    if ((image->channel_mask & BlueChannel) != 0)
      intensity.z+=histogram[i].s[0];
    if ((image->channel_mask & AlphaChannel) != 0)
      intensity.w+=histogram[i].s[3];
    map[i]=intensity;
  }
}

static MagickBooleanType EqualizesRedChannel(const Image *image,
  const cl_float4 white,const cl_float4 black)
{
  return((((image->channel_mask & RedChannel) != 0) &&
    (white.x != black.x)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType EqualizesGreenChannel(const Image *image,
  const cl_float4 white,const cl_float4 black)
{
  return((((image->channel_mask & GreenChannel) != 0) &&
    (white.y != black.y)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType EqualizesBlueChannel(const Image *image,
  const cl_float4 white,const cl_float4 black)
{
  return((((image->channel_mask & BlueChannel) != 0) &&
    (white.z != black.z)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType EqualizesAlphaChannel(const Image *image,
  const cl_float4 white,const cl_float4 black)
{
  return((((image->channel_mask & AlphaChannel) != 0) &&
    (white.w != black.w)) ? MagickTrue : MagickFalse);
}

static void EqualizeColormap(Image *image,PixelPacket *equalize_map,
  const cl_float4 white,const cl_float4 black)
{
  cl_uint
    i;

  for (i=0; i < (cl_uint) image->colors; i++)
  {
    if ((image->channel_mask & SyncChannels) != 0)
    {
      EqualizeLevels
        levels;

      levels.white=white;
      levels.black=black;
      EqualizeColormapSync(image,equalize_map,i,&levels);
      continue;
    }
    if (EqualizesRedChannel(image,white,black) != MagickFalse)
      image->colormap[i].red=equalize_map[
        ScaleQuantumToMap((const Quantum) image->colormap[i].red)].red;
    if (EqualizesGreenChannel(image,white,black) != MagickFalse)
      image->colormap[i].green=equalize_map[
        ScaleQuantumToMap((const Quantum) image->colormap[i].green)].green;
    if (EqualizesBlueChannel(image,white,black) != MagickFalse)
      image->colormap[i].blue=equalize_map[
        ScaleQuantumToMap((const Quantum) image->colormap[i].blue)].blue;
    if (EqualizesAlphaChannel(image,white,black) != MagickFalse)
      image->colormap[i].alpha=equalize_map[
        ScaleQuantumToMap((const Quantum) image->colormap[i].alpha)].alpha;
  }
}

typedef struct _EqualizeMapInput
{
  Image
    *image;

  cl_float4
    *map,
    white,
    black;

  PixelPacket
    *equalize_map;
} EqualizeMapInput;

static void SetEqualizeMapEntry(const EqualizeMapInput *input,const cl_uint i)
{
  Image
    *const image = input->image;

  cl_float4
    *const map = input->map;

  const cl_float4
    white = input->white,
    black = input->black;

  PixelPacket
    *const equalize_map = input->equalize_map;

  if ((image->channel_mask & SyncChannels) != 0)
  {
    if (white.x != black.x)
      equalize_map[i].red=(unsigned int) ScaleMapToQuantum(
        (MagickRealType) ((MaxMap*(map[i].x-black.x))/(white.x-black.x)));
    return;
  }
  if (EqualizesRedChannel(image,white,black) != MagickFalse)
    equalize_map[i].red=(unsigned int) ScaleMapToQuantum(
      (MagickRealType) ((MaxMap*(map[i].x-black.x))/(white.x-black.x)));
  if (EqualizesGreenChannel(image,white,black) != MagickFalse)
    equalize_map[i].green=(unsigned int) ScaleMapToQuantum(
      (MagickRealType) ((MaxMap*(map[i].y-black.y))/(white.y-black.y)));
  if (EqualizesBlueChannel(image,white,black) != MagickFalse)
    equalize_map[i].blue=(unsigned int) ScaleMapToQuantum(
      (MagickRealType) ((MaxMap*(map[i].z-black.z))/(white.z-black.z)));
  if (EqualizesAlphaChannel(image,white,black) != MagickFalse)
    equalize_map[i].alpha=(unsigned int) ScaleMapToQuantum(
      (MagickRealType) ((MaxMap*(map[i].w-black.w))/(white.w-black.w)));
}

typedef struct _HostBufferSource
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  void
    *host;

  int
    aligned;

  cl_mem_flags
    aligned_flags;

  size_t
    element_size;
} HostBufferSource;

static MagickBooleanType CreateHostBuffer(const HostBufferSource *source,
  cl_mem *buffer,ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = source->clEnv;

  const MagickCLDevice
    device = source->device;

  void
    *const host = source->host;

  const int
    aligned = source->aligned;

  const cl_mem_flags
    aligned_flags = source->aligned_flags;

  const size_t
    element_size = source->element_size;

  cl_int
    clStatus;

  cl_mem_flags
    mem_flags;

  size_t
    length;

  void
    *hostPtr;

  /* If the host pointer is aligned to the size of cl_uint,
     then use the host buffer directly from the GPU; otherwise,
     create a buffer on the GPU and copy the data over */
  if (aligned)
  {
    mem_flags = aligned_flags;
    hostPtr = host;
  }
  else
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR;
    hostPtr = host;
  }
  /* create a CL buffer for histogram  */
  length = (MaxMap+1);
  *buffer = clEnv->library->clCreateBuffer(device->context, mem_flags, length * element_size, hostPtr, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

static void ReleaseEqualizeKernels(MagickCLDevice device,cl_command_queue queue,
  cl_kernel histogramKernel,cl_kernel equalizeKernel)
{
  if (histogramKernel!=NULL)
    ReleaseOpenCLKernel(histogramKernel);
  if (equalizeKernel!=NULL)
    ReleaseOpenCLKernel(equalizeKernel);
  if (queue != NULL)
    ReleaseOpenCLCommandQueue(device, queue);
  if (device != NULL)
    ReleaseOpenCLDevice(device);
}

typedef struct _EqualizeBuffers
{
  MagickCLEnv
    clEnv;

  cl_mem
    imageBuffer;

  cl_float4
    *map;

  cl_mem
    equalizeMapBuffer;

  PixelPacket
    *equalize_map;

  cl_mem
    histogramBuffer;

  cl_uint4
    *histogram;
} EqualizeBuffers;

static void ReleaseEqualizeBuffers(const EqualizeBuffers *buffers)
{
  const MagickCLEnv
    clEnv = buffers->clEnv;

  const cl_mem
    imageBuffer = buffers->imageBuffer,
    equalizeMapBuffer = buffers->equalizeMapBuffer,
    histogramBuffer = buffers->histogramBuffer;

  cl_float4
    *map = buffers->map;

  PixelPacket
    *equalize_map = buffers->equalize_map;

  cl_uint4
    *histogram = buffers->histogram;

  if (imageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(imageBuffer);
  if (map!=NULL)
    map=(cl_float4 *) RelinquishMagickMemory(map);
  if (equalizeMapBuffer!=NULL)
    clEnv->library->clReleaseMemObject(equalizeMapBuffer);
  if (equalize_map!=NULL)
    equalize_map=(PixelPacket *) RelinquishMagickMemory(equalize_map);
  if (histogramBuffer!=NULL)
    clEnv->library->clReleaseMemObject(histogramBuffer);
  if (histogram!=NULL)
    histogram=(cl_uint4 *) RelinquishMagickMemory(histogram);
}

typedef struct _EqualizeReadback
{
  MagickCLEnv
    clEnv;

  cl_command_queue
    queue;

  cl_mem
    imageBuffer;

  Image
    *image;

  void
    *inputPixels;

  MagickCLDevice
    device;
} EqualizeReadback;

static MagickBooleanType ReadEqualizedImageBuffer(
  const EqualizeReadback *readback,ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = readback->clEnv;

  const cl_command_queue
    queue = readback->queue;

  const cl_mem
    imageBuffer = readback->imageBuffer;

  Image
    *const image = readback->image;

  void
    *const inputPixels = readback->inputPixels;

  const MagickCLDevice
    device = readback->device;

  cl_int
    clStatus;

  size_t
    length;

  if (ALIGNED(inputPixels,CLPixelPacket))
  {
    length = image->columns * image->rows;
    clEnv->library->clEnqueueMapBuffer(queue, imageBuffer, CL_TRUE, CL_MAP_READ|CL_MAP_WRITE, 0, length * sizeof(CLPixelPacket), 0, NULL, NULL, &clStatus);
  }
  else
  {
    length = image->columns * image->rows;
    clStatus = clEnv->library->clEnqueueReadBuffer(queue, imageBuffer, CL_TRUE, 0, length * sizeof(CLPixelPacket), inputPixels, 0, NULL, NULL);
  }
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "Reading output image from CL buffer failed.", ".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _EqualizeKernelInputs
{
  MagickCLDevice
    device;

  MagickCLEnv
    clEnv;

  cl_mem
    imageBuffer;

  cl_int
    channel_mask;

  cl_mem
    equalizeMapBuffer;

  cl_float4
    white,
    black;

  Image
    *image;

  cl_command_queue
    queue;
} EqualizeKernelInputs;

static MagickBooleanType LaunchEqualizeKernel(
  const EqualizeKernelInputs *inputs,cl_kernel *equalizeKernel,
  ExceptionInfo *exception)
{
  const MagickCLDevice
    device = inputs->device;

  const MagickCLEnv
    clEnv = inputs->clEnv;

  const cl_mem
    imageBuffer = inputs->imageBuffer;

  const cl_int
    channel_mask = inputs->channel_mask;

  const cl_mem
    equalizeMapBuffer = inputs->equalizeMapBuffer;

  const cl_float4
    white = inputs->white,
    black = inputs->black;

  Image
    *const image = inputs->image;

  const cl_command_queue
    queue = inputs->queue;

  cl_event
    event;

  cl_int
    clStatus;

  cl_uint
    i;

  size_t
    global_work_size[2];

  /* get the OpenCL kernel */
  *equalizeKernel = AcquireOpenCLKernel(device,"Equalize");
  if (*equalizeKernel == NULL)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "AcquireOpenCLKernel failed.", ".");
    return(MagickFalse);
  }

  /* set the kernel arguments */
  i=0;
  clStatus=clEnv->library->clSetKernelArg(*equalizeKernel,i++,sizeof(cl_mem),(void *)&imageBuffer);
  clStatus|=clEnv->library->clSetKernelArg(*equalizeKernel,i++,sizeof(cl_int),&channel_mask);
  clStatus|=clEnv->library->clSetKernelArg(*equalizeKernel,i++,sizeof(cl_mem),(void *)&equalizeMapBuffer);
  clStatus|=clEnv->library->clSetKernelArg(*equalizeKernel,i++,sizeof(cl_float4),&white);
  clStatus|=clEnv->library->clSetKernelArg(*equalizeKernel,i++,sizeof(cl_float4),&black);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
    return(MagickFalse);
  }

  /* launch the kernel */
  global_work_size[0] = image->columns;
  global_work_size[1] = image->rows;

  clStatus = clEnv->library->clEnqueueNDRangeKernel(queue, *equalizeKernel, 2, NULL, global_work_size, NULL, 0, NULL, &event);

  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clEnqueueNDRangeKernel failed.", ".");
    return(MagickFalse);
  }
  RecordProfileData(device,*equalizeKernel,event);
  return(MagickTrue);
}

typedef struct _EqualizeHistogramInput
{
  Image
    *image;

  cl_uint4
    *histogram;

  cl_float4
    *map;

  PixelPacket
    *equalize_map;
} EqualizeHistogramInput;

static void BuildEqualizeMap(const EqualizeHistogramInput *input,
  cl_float4 *black,cl_float4 *white)
{
  Image
    *const image = input->image;

  cl_uint4
    *const histogram = input->histogram;

  cl_float4
    *const map = input->map;

  PixelPacket
    *const equalize_map = input->equalize_map;

  cl_uint
    i;

  EqualizeMapInput
    map_input;

  /*
    Integrate the histogram to get the equalization map.
  */
  IntegrateEqualizeHistogram(image,histogram,map);
  *black=map[0];
  *white=map[(int) MaxMap];
  (void) memset(equalize_map,0,(MaxMap+1)*sizeof(*equalize_map));
  map_input.image=image;
  map_input.map=map;
  map_input.white=*white;
  map_input.black=*black;
  map_input.equalize_map=equalize_map;
  for (i=0; i <= (ssize_t) MaxMap; i++)
    SetEqualizeMapEntry(&map_input,i);

  if (image->storage_class == PseudoClass)
  {
    /*
       Equalize colormap.
       */
    EqualizeColormap(image,equalize_map,*white,*black);
  }
}

typedef struct _EqualizeQueueTargets
{
  MagickCLDevice
    *device;

  cl_command_queue
    *queue;
} EqualizeQueueTargets;

static MagickBooleanType AcquireEqualizeQueue(MagickCLEnv clEnv,
  const EqualizeQueueTargets *targets)
{
  MagickCLDevice
    *const device = targets->device;

  cl_command_queue
    *const queue = targets->queue;

  *device=RequestOpenCLDevice(clEnv);
  if (*device == (MagickCLDevice) NULL)
    return(MagickFalse);
  *queue=AcquireOpenCLCommandQueue(*device);
  if (*queue == (cl_command_queue) NULL)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _HistogramPassInput
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_uint4
    *histogram;

  cl_command_queue
    queue;

  cl_mem
    imageBuffer;

  Image
    *image;
} HistogramPassInput;

static MagickBooleanType RunHistogramPass(const HistogramPassInput *pass,
  cl_mem *histogramBuffer,ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = pass->clEnv;

  const MagickCLDevice
    device = pass->device;

  cl_uint4
    *const histogram = pass->histogram;

  const cl_command_queue
    queue = pass->queue;

  const cl_mem
    imageBuffer = pass->imageBuffer;

  Image
    *const image = pass->image;

  HistogramBufferInfo
    histogram_info;

  MagickBooleanType
    status;

  {
    const HostBufferSource
      source = { clEnv, device, histogram, ALIGNED(histogram,cl_uint4),
        CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR, sizeof(cl_uint4) };

    if (CreateHostBuffer(&source,histogramBuffer,exception) == MagickFalse)
      return(MagickFalse);
  }

  status = LaunchHistogramKernel(clEnv, device, queue, imageBuffer, *histogramBuffer, image, image->channel_mask, exception);
  if (status == MagickFalse)
    return(MagickFalse);

  histogram_info.clEnv=clEnv;
  histogram_info.device=device;
  histogram_info.queue=queue;
  histogram_info.histogramBuffer=*histogramBuffer;
  histogram_info.histogram=histogram;
  if (ReadHistogramBuffer(&histogram_info,exception) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _EqualizePassInput
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  PixelPacket
    *equalize_map;

  cl_mem
    imageBuffer;

  cl_int
    channel_mask;

  cl_float4
    white,
    black;

  Image
    *image;

  cl_command_queue
    queue;

  void
    *inputPixels;

  CacheView
    *image_view;
} EqualizePassInput;

typedef struct _EqualizePassTargets
{
  cl_mem
    *equalizeMapBuffer;

  cl_kernel
    *equalizeKernel;

  MagickBooleanType
    *outputReady;
} EqualizePassTargets;

static MagickBooleanType RunEqualizePass(const EqualizePassInput *run,
  ExceptionInfo *exception,const EqualizePassTargets *runTargets)
{
  const MagickCLEnv
    clEnv = run->clEnv;

  const MagickCLDevice
    device = run->device;

  PixelPacket
    *const equalize_map = run->equalize_map;

  const cl_mem
    imageBuffer = run->imageBuffer;

  const cl_int
    channel_mask = run->channel_mask;

  const cl_float4
    white = run->white,
    black = run->black;

  Image
    *const image = run->image;

  const cl_command_queue
    queue = run->queue;

  void
    *const inputPixels = run->inputPixels;

  CacheView
    *const image_view = run->image_view;

  cl_mem
    *const equalizeMapBuffer = runTargets->equalizeMapBuffer;

  cl_kernel
    *const equalizeKernel = runTargets->equalizeKernel;

  MagickBooleanType
    *const outputReady = runTargets->outputReady;

  /* Create and initialize OpenCL buffers. */
  {
    const HostBufferSource
      source = { clEnv, device, equalize_map,
        ALIGNED(equalize_map, PixelPacket),
        CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, sizeof(PixelPacket) };

    if (CreateHostBuffer(&source,equalizeMapBuffer,exception) == MagickFalse)
      return(MagickFalse);
  }

  {
    const EqualizeKernelInputs
      inputs = { device, clEnv, imageBuffer, channel_mask, *equalizeMapBuffer,
        white, black, image, queue };

    if (LaunchEqualizeKernel(&inputs,equalizeKernel,exception) == MagickFalse)
      return(MagickFalse);
  }

  /* read the data back */
  {
    const EqualizeReadback
      readback = { clEnv, queue, imageBuffer, image, inputPixels, device };

    if (ReadEqualizedImageBuffer(&readback,exception) == MagickFalse)
      return(MagickFalse);
  }

  *outputReady=SyncCacheViewAuthenticPixels(image_view,exception);
  return(MagickTrue);
}

typedef struct _EqualizeInputSource
{
  Image
    *image;

  MagickCLDevice
    device;

  MagickCLEnv
    clEnv;
} EqualizeInputSource;

typedef struct _EqualizeInputTargets
{
  CacheView
    **image_view;

  void
    **inputPixels;

  cl_mem
    *imageBuffer;
} EqualizeInputTargets;

static MagickBooleanType AcquireEqualizeInput(
  const EqualizeInputSource *source,ExceptionInfo *exception,
  const EqualizeInputTargets *inputTargets)
{
  Image
    *const image = source->image;

  const MagickCLDevice
    device = source->device;

  const MagickCLEnv
    clEnv = source->clEnv;

  CacheView
    **const image_view = inputTargets->image_view;

  void
    **const inputPixels = inputTargets->inputPixels;

  cl_mem
    *const imageBuffer = inputTargets->imageBuffer;

  cl_int
    clStatus;

  cl_mem_flags
    mem_flags;

  size_t
    length;

  /* Create and initialize OpenCL buffers. */
  /* inputPixels = AcquirePixelCachePixels(image, &length, exception); */
  /* assume this  will get a writable image */
  *image_view=AcquireAuthenticCacheView(image,exception);
  *inputPixels=GetCacheViewAuthenticPixels(*image_view,0,0,image->columns,image->rows,exception);

  if (*inputPixels == (void *) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),CacheWarning,"UnableToReadPixelCache.","`%s'",image->filename);
    return(MagickFalse);
  }
  /* If the host pointer is aligned to the size of CLPixelPacket,
     then use the host buffer directly from the GPU; otherwise,
     create a buffer on the GPU and copy the data over */
  mem_flags=GetInputBufferFlags(*inputPixels);
  /* create a CL buffer from image pixel buffer */
  length = image->columns * image->rows;
  *imageBuffer = clEnv->library->clCreateBuffer(device->context, mem_flags, length * sizeof(CLPixelPacket), (void*)*inputPixels, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType ComputeEqualizeImage(Image *image,MagickCLEnv clEnv,
  ExceptionInfo *exception)
{
#define EqualizeImageTag  "Equalize/Image"

  CacheView
    *image_view=NULL;

  cl_command_queue
    queue=NULL;

  cl_int
    channel_mask=get32BitChannelValue(image->channel_mask);

  cl_mem
    equalizeMapBuffer=NULL,
    histogramBuffer=NULL,
    imageBuffer=NULL;

  cl_kernel
    equalizeKernel=NULL,
    histogramKernel=NULL;

  cl_uint4
    *histogram=NULL;

  cl_float4
    white,
    black,
    *map=NULL;

  EqualizeHistogramInput
    histogram_input;

  MagickBooleanType
    outputReady=MagickFalse;

  MagickCLDevice
    device;

  PixelPacket
    *equalize_map=NULL;

  void
    *inputPixels=NULL;

  if (clEnv == (MagickCLEnv) NULL)
    return(MagickFalse);

  TraceAccelerateImage(image);

  /*
   * initialize opencl env
   */
  {
    const EqualizeQueueTargets
      queueTargets = { &device, &queue };

    if (AcquireEqualizeQueue(clEnv,&queueTargets) == MagickFalse)
      goto cleanup;
  }

  /*
    Allocate and initialize histogram arrays.
  */
  histogram=(cl_uint4 *) AcquireQuantumMemory(MaxMap+1UL, sizeof(*histogram));
  if (histogram == (cl_uint4 *) NULL)
      ThrowBinaryException(ResourceLimitWarning,"MemoryAllocationFailed", image->filename);

  /* reset histogram */
  (void) memset(histogram,0,(MaxMap+1)*sizeof(*histogram));

  {
    const EqualizeInputSource
      source = { image, device, clEnv };

    const EqualizeInputTargets
      inputTargets = { &image_view, &inputPixels, &imageBuffer };

    if (AcquireEqualizeInput(&source,exception,&inputTargets) == MagickFalse)
      goto cleanup;
  }

  {
    const HistogramPassInput
      pass = { clEnv, device, histogram, queue, imageBuffer, image };

    if (RunHistogramPass(&pass,&histogramBuffer,exception) == MagickFalse)
      goto cleanup;
  }

  /* recreate input buffer later, in case image updated */
#ifdef RECREATEBUFFER
  if (imageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(imageBuffer);
#endif

  /* CPU stuff */
  equalize_map=(PixelPacket *) AcquireQuantumMemory(MaxMap+1UL, sizeof(*equalize_map));
  if (equalize_map == (PixelPacket *) NULL)
    ThrowBinaryException(ResourceLimitWarning,"MemoryAllocationFailed", image->filename);

  map=(cl_float4 *) AcquireQuantumMemory(MaxMap+1UL,sizeof(*map));
  if (map == (cl_float4 *) NULL)
    ThrowBinaryException(ResourceLimitWarning,"MemoryAllocationFailed", image->filename);

  histogram_input.image=image;
  histogram_input.histogram=histogram;
  histogram_input.map=map;
  histogram_input.equalize_map=equalize_map;
  BuildEqualizeMap(&histogram_input,&black,&white);

  /*
    Equalize image.
  */

  /* GPU can work on this again, image and equalize map as input
    image:        uchar4 (CLPixelPacket)
    equalize_map: uchar4 (PixelPacket)
    black, white: float4 (FloatPixelPacket) */

#ifdef RECREATEBUFFER
  /* If the host pointer is aligned to the size of CLPixelPacket,
     then use the host buffer directly from the GPU; otherwise,
     create a buffer on the GPU and copy the data over */
  if (ALIGNED(inputPixels,CLPixelPacket))
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR;
  }
  else
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR;
  }
  /* create a CL buffer from image pixel buffer */
  length = image->columns * image->rows;
  imageBuffer = clEnv->library->clCreateBuffer(context, mem_flags, length * sizeof(CLPixelPacket), (void*)inputPixels, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    goto cleanup;
  }
#endif

  {
    const EqualizePassInput
      run = { clEnv, device, equalize_map, imageBuffer, channel_mask, white,
        black, image, queue, inputPixels, image_view };

    const EqualizePassTargets
      runTargets = { &equalizeMapBuffer, &equalizeKernel, &outputReady };

    if (RunEqualizePass(&run,exception,&runTargets) == MagickFalse)
      goto cleanup;
  }

cleanup:

  image_view=DestroyCacheView(image_view);

  {
    const EqualizeBuffers
      buffers = { clEnv, imageBuffer, map, equalizeMapBuffer, equalize_map,
        histogramBuffer, histogram };

    ReleaseEqualizeBuffers(&buffers);
  }
  ReleaseEqualizeKernels(device,queue,histogramKernel,equalizeKernel);

  return(outputReady);
}

MagickPrivate MagickBooleanType AccelerateEqualizeImage(Image *image,
  ExceptionInfo *exception)
{
  return(ComputeEqualizeImage(image,getAccelerateEnvironment(image,
    HistogramAccelerateCheck,__func__,exception),exception));
}
#endif /* MAGICKCORE_OPENCL_SUPPORT */
