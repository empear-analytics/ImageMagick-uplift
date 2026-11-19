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
%                  M   M   OOO   TTTTT  IIIII   OOO   N   N                   %
%                  MM MM  O   O    T      I    O   O  NN  N                   %
%                  M M M  O   O    T      I    O   O  N N N                   %
%                  M   M  O   O    T      I    O   O  N  NN                   %
%                  M   M   OOO     T    IIIII   OOO   N   N                   %
%                                                                             %
%                         BBBB   L      U   U  RRRR                           %
%                         B   B  L      U   U  R   R                          %
%                         BBBB   L      U   U  RRRR                           %
%                         B   B  L      U   U  R R                            %
%                         BBBB   LLLLL   UUU   R  R                           %
%                                                                             %
%                                                                             %
%              MagickCore OpenCL Accelerated Motion Blur Method               %
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
#if defined(MAGICKCORE_OPENCL_SUPPORT)

/* pad the global workgroup size to the next multiple of
   the local workgroup size */
inline static unsigned int padGlobalWorkgroupSizeToLocalWorkgroupSize(
  const unsigned int orgGlobalSize,const unsigned int localGroupSize)
{
  return ((orgGlobalSize+(localGroupSize-1))/localGroupSize*localGroupSize);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A c c e l e r a t e M o t i o n B l u r I m a g e                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

typedef struct _MotionBlurFilterInputs
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  const double
    *kernel;

  size_t
    width;
} MotionBlurFilterInputs;

typedef struct _MotionBlurFilterTargets
{
  cl_mem
    *imageKernelBuffer;

  cl_command_queue
    *queue;
} MotionBlurFilterTargets;

static MagickBooleanType CreateMotionBlurFilterBuffer(
  const MotionBlurFilterInputs *inputs,ExceptionInfo *exception,
  const MotionBlurFilterTargets *targets)
{
  const MagickCLEnv
    clEnv = inputs->clEnv;

  const MagickCLDevice
    device = inputs->device;

  const double
    *const kernel = inputs->kernel;

  const size_t
    width = inputs->width;

  cl_mem
    *const imageKernelBuffer = targets->imageKernelBuffer;

  cl_command_queue
    *const queue = targets->queue;

  cl_int
    clStatus;

  float
    *kernelBufferPtr;

  unsigned int
    i;

  *imageKernelBuffer = clEnv->library->clCreateBuffer(device->context,
    CL_MEM_READ_ONLY|CL_MEM_ALLOC_HOST_PTR, width * sizeof(float), NULL,
    &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception, GetMagickModule(),
      ResourceLimitError, "clEnv->library->clCreateBuffer failed.",".");
    return(MagickFalse);
  }

  *queue=AcquireOpenCLCommandQueue(device);
  if (*queue == (cl_command_queue) NULL)
    return(MagickFalse);
  kernelBufferPtr = (float*)clEnv->library->clEnqueueMapBuffer(*queue, *imageKernelBuffer,
    CL_TRUE, CL_MAP_WRITE, 0, width * sizeof(float), 0, NULL, NULL, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception, GetMagickModule(),
      ResourceLimitError, "clEnv->library->clEnqueueMapBuffer failed.",".");
    return(MagickFalse);
  }
  for (i = 0; i < width; i++)
  {
    kernelBufferPtr[i] = (float) kernel[i];
  }
  clStatus = clEnv->library->clEnqueueUnmapMemObject(*queue, *imageKernelBuffer, kernelBufferPtr,
    0, NULL, NULL);
 if (clStatus != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception, GetMagickModule(), ModuleFatalError,
      "clEnv->library->clEnqueueUnmapMemObject failed.", ".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _MotionBlurKernelInputs
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_mem
    imageBuffer,
    filteredImageBuffer,
    imageKernelBuffer,
    offsetBuffer;

  size_t
    width;

  cl_int
    channel_mask;

  const Image
    *image;
} MotionBlurKernelInputs;

static MagickBooleanType SetMotionBlurKernelArgs(
  const MotionBlurKernelInputs *inputs,cl_kernel *motionBlurKernel,
  ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = inputs->clEnv;

  const MagickCLDevice
    device = inputs->device;

  const Image
    *const image = inputs->image;

  cl_float4
    biasPixel;

  cl_int
    clStatus;

  PixelInfo
    bias;

  unsigned int
    i,
    imageHeight,
    imageWidth,
    matte;

  /*
    Get the OpenCL kernel
  */
  *motionBlurKernel = AcquireOpenCLKernel(device,"MotionBlur");
  if (*motionBlurKernel == NULL)
  {
    (void) ThrowMagickException(exception, GetMagickModule(), ModuleFatalError,
      "AcquireOpenCLKernel failed.", ".");
    return(MagickFalse);
  }

  /*
    Set the kernel arguments.
  */
  i = 0;
  clStatus=clEnv->library->clSetKernelArg(*motionBlurKernel,i++,sizeof(cl_mem),
    (void *)&inputs->imageBuffer);
  clStatus|=clEnv->library->clSetKernelArg(*motionBlurKernel,i++,sizeof(cl_mem),
    (void *)&inputs->filteredImageBuffer);
  imageWidth = (unsigned int) image->columns;
  imageHeight = (unsigned int) image->rows;
  clStatus|=clEnv->library->clSetKernelArg(*motionBlurKernel,i++,sizeof(unsigned int),
    &imageWidth);
  clStatus|=clEnv->library->clSetKernelArg(*motionBlurKernel,i++,sizeof(unsigned int),
    &imageHeight);
  clStatus|=clEnv->library->clSetKernelArg(*motionBlurKernel,i++,sizeof(cl_mem),
    (void *)&inputs->imageKernelBuffer);
  clStatus|=clEnv->library->clSetKernelArg(*motionBlurKernel,i++,sizeof(unsigned int),
    &inputs->width);
  clStatus|=clEnv->library->clSetKernelArg(*motionBlurKernel,i++,sizeof(cl_mem),
    (void *)&inputs->offsetBuffer);

  GetPixelInfo(image,&bias);
  biasPixel.s[0] = (cl_float) bias.red;
  biasPixel.s[1] = (cl_float) bias.green;
  biasPixel.s[2] = (cl_float) bias.blue;
  biasPixel.s[3] = (cl_float) bias.alpha;
  clStatus|=clEnv->library->clSetKernelArg(*motionBlurKernel,i++,sizeof(cl_float4), &biasPixel);

  clStatus|=clEnv->library->clSetKernelArg(*motionBlurKernel,i++,sizeof(cl_int),&inputs->channel_mask);
  matte = (image->alpha_trait > CopyPixelTrait)?1:0;
  clStatus|=clEnv->library->clSetKernelArg(*motionBlurKernel,i++,sizeof(unsigned int), &matte);
  if (clStatus != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception, GetMagickModule(), ModuleFatalError,
      "clEnv->library->clSetKernelArg failed.", ".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _MotionBlurLaunchInputs
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  cl_kernel
    motionBlurKernel;

  cl_mem
    filteredImageBuffer;

  const Image
    *image;

  void
    *filteredPixels;

  CacheView
    *filteredImage_view;
} MotionBlurLaunchInputs;

static MagickBooleanType LaunchMotionBlurKernel(
  const MotionBlurLaunchInputs *inputs,MagickBooleanType *outputReady,
  ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = inputs->clEnv;

  const cl_command_queue
    queue = inputs->queue;

  const Image
    *const image = inputs->image;

  cl_event
    event;

  cl_int
    clStatus;

  size_t
    global_work_size[2],
    length,
    local_work_size[2];

  /*
    Launch the kernel.
  */
  local_work_size[0] = 16;
  local_work_size[1] = 16;
  global_work_size[0] = (size_t)padGlobalWorkgroupSizeToLocalWorkgroupSize(
                                (unsigned int) image->columns,(unsigned int) local_work_size[0]);
  global_work_size[1] = (size_t)padGlobalWorkgroupSizeToLocalWorkgroupSize(
                                (unsigned int) image->rows,(unsigned int) local_work_size[1]);
  clStatus = clEnv->library->clEnqueueNDRangeKernel(queue, inputs->motionBlurKernel, 2, NULL,
	  global_work_size, local_work_size, 0, NULL, &event);

  if (clStatus != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception, GetMagickModule(), ModuleFatalError,
      "clEnv->library->clEnqueueNDRangeKernel failed.", ".");
    return(MagickFalse);
  }
  RecordProfileData(inputs->device,inputs->motionBlurKernel,event);

  if (ALIGNED(inputs->filteredPixels,CLPixelPacket))
  {
    length = image->columns * image->rows;
    clEnv->library->clEnqueueMapBuffer(queue, inputs->filteredImageBuffer, CL_TRUE,
      CL_MAP_READ|CL_MAP_WRITE, 0, length * sizeof(CLPixelPacket), 0, NULL,
      NULL, &clStatus);
  }
  else
  {
    length = image->columns * image->rows;
    clStatus = clEnv->library->clEnqueueReadBuffer(queue, inputs->filteredImageBuffer, CL_TRUE, 0,
      length * sizeof(CLPixelPacket), inputs->filteredPixels, 0, NULL, NULL);
  }
  if (clStatus != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception, GetMagickModule(), ModuleFatalError,
      "Reading output image from CL buffer failed.", ".");
    return(MagickFalse);
  }
  *outputReady=SyncCacheViewAuthenticPixels(inputs->filteredImage_view,exception);
  return(MagickTrue);
}

typedef struct _MotionBlurReleaseInputs
{
  MagickCLEnv
    clEnv;

  cl_mem
    filteredImageBuffer,
    imageBuffer,
    imageKernelBuffer;

  cl_kernel
    motionBlurKernel;

  cl_command_queue
    queue;

  MagickCLDevice
    device;
} MotionBlurReleaseInputs;

static void ReleaseMotionBlurResources(const MotionBlurReleaseInputs *inputs)
{
  const MagickCLEnv
    clEnv = inputs->clEnv;

  const cl_mem
    filteredImageBuffer = inputs->filteredImageBuffer,
    imageBuffer = inputs->imageBuffer,
    imageKernelBuffer = inputs->imageKernelBuffer;

  const cl_kernel
    motionBlurKernel = inputs->motionBlurKernel;

  const cl_command_queue
    queue = inputs->queue;

  const MagickCLDevice
    device = inputs->device;

  if (filteredImageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(filteredImageBuffer);
  if (imageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(imageBuffer);
  if (imageKernelBuffer!=NULL)
    clEnv->library->clReleaseMemObject(imageKernelBuffer);
  if (motionBlurKernel!=NULL)
    ReleaseOpenCLKernel(motionBlurKernel);
  if (queue != NULL)
    ReleaseOpenCLCommandQueue(device,queue);
  if (device != NULL)
    ReleaseOpenCLDevice(device);
}

typedef struct _MotionBlurOffsetInputs
{
  MagickCLEnv
    clEnv;

  cl_command_queue
    queue;

  cl_mem
    offsetBuffer;

  const OffsetInfo
    *offset;

  size_t
    width;
} MotionBlurOffsetInputs;

static MagickBooleanType FillMotionBlurOffsets(
  const MotionBlurOffsetInputs *inputs,ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = inputs->clEnv;

  const cl_command_queue
    queue = inputs->queue;

  const cl_mem
    offsetBuffer = inputs->offsetBuffer;

  const OffsetInfo
    *const offset = inputs->offset;

  const size_t
    width = inputs->width;

  cl_int
    clStatus;

  int
    *offsetBufferPtr;

  unsigned int
    i;

  offsetBufferPtr = (int*)clEnv->library->clEnqueueMapBuffer(queue, offsetBuffer, CL_TRUE,
    CL_MAP_WRITE, 0, width * sizeof(cl_int2), 0, NULL, NULL, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception, GetMagickModule(),
      ResourceLimitError, "clEnv->library->clEnqueueMapBuffer failed.",".");
    return(MagickFalse);
  }
  for (i = 0; i < width; i++)
  {
    offsetBufferPtr[2*i] = (int)offset[i].x;
    offsetBufferPtr[2*i+1] = (int)offset[i].y;
  }
  clStatus = clEnv->library->clEnqueueUnmapMemObject(queue, offsetBuffer, offsetBufferPtr, 0,
    NULL, NULL);
 if (clStatus != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception, GetMagickModule(), ModuleFatalError,
      "clEnv->library->clEnqueueUnmapMemObject failed.", ".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _MotionBlurOutputSource
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  const Image
    *image;

  void
    *filteredPixels;
} MotionBlurOutputSource;

typedef struct _MotionBlurViewTargets
{
  CacheView
    **image_view;

  const void
    **inputPixels;
} MotionBlurViewTargets;

static MagickBooleanType AcquireMotionBlurPixels(const Image *image,
  ExceptionInfo *exception,const MotionBlurViewTargets *targets)
{
  CacheView
    **const image_view = targets->image_view;

  const void
    **const inputPixels = targets->inputPixels;

  *image_view=AcquireAuthenticCacheView(image,exception);
  *inputPixels=GetCacheViewAuthenticPixels(*image_view,0,0,image->columns,
    image->rows,exception);
  if (*inputPixels == (const void *) NULL)
  {
    (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
      "UnableToReadPixelCache.","`%s'",image->filename);
    return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _MotionBlurImageTargets
{
  Image
    **filteredImage;

  CacheView
    **filteredImage_view;

  void
    **filteredPixels;
} MotionBlurImageTargets;

static MagickBooleanType CloneMotionBlurImage(const Image *image,
  ExceptionInfo *exception,const MotionBlurImageTargets *targets)
{
  Image
    **const filteredImage = targets->filteredImage;

  CacheView
    **const filteredImage_view = targets->filteredImage_view;

  void
    **const filteredPixels = targets->filteredPixels;

  *filteredImage = CloneImage(image,image->columns,image->rows,
    MagickTrue,exception);
  assert(*filteredImage != NULL);
  if (SetImageStorageClass(*filteredImage,DirectClass,exception) != MagickTrue)
  {
    (void) ThrowMagickException(exception, GetMagickModule(),
      ResourceLimitError, "CloneImage failed.", ".");
    return(MagickFalse);
  }
  *filteredImage_view=AcquireAuthenticCacheView(*filteredImage,exception);
  *filteredPixels=GetCacheViewAuthenticPixels(*filteredImage_view,0,0,(*filteredImage)->columns,(*filteredImage)->rows,exception);
  if (*filteredPixels == (void *) NULL)
  {
    (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
      "UnableToReadPixelCache.","`%s'",(*filteredImage)->filename);
    return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType CreateMotionBlurOutputBuffer(
  const MotionBlurOutputSource *source,cl_mem *filteredImageBuffer,
  ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = source->clEnv;

  const MagickCLDevice
    device = source->device;

  const Image
    *const image = source->image;

  void
    *const filteredPixels = source->filteredPixels;

  cl_int
    clStatus;

  cl_mem_flags
    mem_flags;

  size_t
    length;

  void
    *hostPtr;

  if (ALIGNED(filteredPixels,CLPixelPacket))
  {
    mem_flags = CL_MEM_WRITE_ONLY|CL_MEM_USE_HOST_PTR;
    hostPtr = filteredPixels;
  }
  else
  {
    mem_flags = CL_MEM_WRITE_ONLY;
    hostPtr = NULL;
  }
  /*
    Create a CL buffer from image pixel buffer.
  */
  length = image->columns * image->rows;
  *filteredImageBuffer = clEnv->library->clCreateBuffer(device->context, mem_flags,
    length * sizeof(CLPixelPacket), hostPtr, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception, GetMagickModule(),
      ResourceLimitError, "clEnv->library->clCreateBuffer failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _MotionBlurOffsetSource
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  const OffsetInfo
    *offset;

  size_t
    width;
} MotionBlurOffsetSource;

static MagickBooleanType CreateMotionBlurOffsetBuffer(
  const MotionBlurOffsetSource *source,cl_mem *offsetBuffer,
  ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = source->clEnv;

  const MagickCLDevice
    device = source->device;

  const cl_command_queue
    queue = source->queue;

  const OffsetInfo
    *const offset = source->offset;

  const size_t
    width = source->width;

  cl_int
    clStatus;

  *offsetBuffer = clEnv->library->clCreateBuffer(device->context,
    CL_MEM_READ_ONLY|CL_MEM_ALLOC_HOST_PTR, width * sizeof(cl_int2), NULL,
    &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception, GetMagickModule(),
      ResourceLimitError, "clEnv->library->clCreateBuffer failed.",".");
    return(MagickFalse);
  }

  {
    const MotionBlurOffsetInputs inputs = { clEnv, queue, *offsetBuffer,
      offset, width };

    if (FillMotionBlurOffsets(&inputs,exception) == MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _MotionBlurInputTargets
{
  MagickCLDevice
    *device;

  CacheView
    **image_view;

  cl_mem
    *imageBuffer;
} MotionBlurInputTargets;

static MagickBooleanType AcquireMotionBlurInput(MagickCLEnv clEnv,
  const Image *image,ExceptionInfo *exception,
  const MotionBlurInputTargets *inputTargets)
{
  MagickCLDevice
    *const device = inputTargets->device;

  CacheView
    **const image_view = inputTargets->image_view;

  cl_mem
    *const imageBuffer = inputTargets->imageBuffer;

  cl_int
    clStatus;

  cl_mem_flags
    mem_flags;

  const void
    *inputPixels;

  size_t
    length;

  *device=RequestOpenCLDevice(clEnv);
  if (*device == (MagickCLDevice) NULL)
    return(MagickFalse);

  /* Create and initialize OpenCL buffers. */

  {
    const MotionBlurViewTargets
      targets = { image_view, &inputPixels };

    if (AcquireMotionBlurPixels(image,exception,&targets) == MagickFalse)
      return(MagickFalse);
  }

  /*
    If the host pointer is aligned to the size of CLPixelPacket, then use
    the host buffer directly from the GPU; otherwise, create a buffer on
    the GPU and copy the data over
  */
  if (ALIGNED(inputPixels,CLPixelPacket))
  {
    mem_flags = CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR;
  }
  else
  {
    mem_flags = CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR;
  }
  /*
    create a CL buffer from image pixel buffer
  */
  length = image->columns * image->rows;
  *imageBuffer = clEnv->library->clCreateBuffer((*device)->context, mem_flags,
    length * sizeof(CLPixelPacket), (void*)inputPixels, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception, GetMagickModule(),
      ResourceLimitError, "clEnv->library->clCreateBuffer failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _MotionBlurOutputInput
{
  const Image
    *image;

  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;
} MotionBlurOutputInput;

typedef struct _MotionBlurOutputTargets
{
  Image
    **filteredImage;

  CacheView
    **filteredImage_view;

  void
    **filteredPixels;

  cl_mem
    *filteredImageBuffer;
} MotionBlurOutputTargets;

static MagickBooleanType AcquireMotionBlurOutput(
  const MotionBlurOutputInput *input,ExceptionInfo *exception,
  const MotionBlurOutputTargets *outputTargets)
{
  const Image
    *const image = input->image;

  const MagickCLEnv
    clEnv = input->clEnv;

  const MagickCLDevice
    device = input->device;

  Image
    **const filteredImage = outputTargets->filteredImage;

  CacheView
    **const filteredImage_view = outputTargets->filteredImage_view;

  void
    **const filteredPixels = outputTargets->filteredPixels;

  cl_mem
    *const filteredImageBuffer = outputTargets->filteredImageBuffer;

  {
    const MotionBlurImageTargets
      targets = { filteredImage, filteredImage_view, filteredPixels };

    if (CloneMotionBlurImage(image,exception,&targets) == MagickFalse)
      return(MagickFalse);
  }

  {
    const MotionBlurOutputSource
      source = { clEnv, device, image, *filteredPixels };

    if (CreateMotionBlurOutputBuffer(&source,filteredImageBuffer,exception) ==
        MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _MotionBlurRunInput
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  const double
    *kernel;

  size_t
    width;

  const OffsetInfo
    *offset;

  cl_mem
    imageBuffer,
    filteredImageBuffer;

  cl_int
    channel_mask;

  const Image
    *image;

  void
    *filteredPixels;

  CacheView
    *filteredImage_view;
} MotionBlurRunInput;

typedef struct _MotionBlurRunTargets
{
  cl_mem
    *imageKernelBuffer;

  cl_command_queue
    *queue;

  cl_mem
    *offsetBuffer;

  cl_kernel
    *motionBlurKernel;

  MagickBooleanType
    *outputReady;
} MotionBlurRunTargets;

static MagickBooleanType RunMotionBlurKernel(const MotionBlurRunInput *run,
  ExceptionInfo *exception,const MotionBlurRunTargets *runTargets)
{
  const MagickCLEnv
    clEnv = run->clEnv;

  const MagickCLDevice
    device = run->device;

  const double
    *const kernel = run->kernel;

  const size_t
    width = run->width;

  const OffsetInfo
    *const offset = run->offset;

  const cl_mem
    imageBuffer = run->imageBuffer,
    filteredImageBuffer = run->filteredImageBuffer;

  const cl_int
    channel_mask = run->channel_mask;

  const Image
    *const image = run->image;

  void
    *const filteredPixels = run->filteredPixels;

  CacheView
    *const filteredImage_view = run->filteredImage_view;

  cl_mem
    *const imageKernelBuffer = runTargets->imageKernelBuffer;

  cl_command_queue
    *const queue = runTargets->queue;

  cl_mem
    *const offsetBuffer = runTargets->offsetBuffer;

  cl_kernel
    *const motionBlurKernel = runTargets->motionBlurKernel;

  MagickBooleanType
    *const outputReady = runTargets->outputReady;

  {
    const MotionBlurFilterInputs inputs = { clEnv, device, kernel, width };
    const MotionBlurFilterTargets targets = { imageKernelBuffer, queue };

    if (CreateMotionBlurFilterBuffer(&inputs,exception,&targets) ==
        MagickFalse)
      return(MagickFalse);
  }

  {
    const MotionBlurOffsetSource
      source = { clEnv, device, *queue, offset, width };

    if (CreateMotionBlurOffsetBuffer(&source,offsetBuffer,exception) ==
        MagickFalse)
      return(MagickFalse);
  }


  {
    const MotionBlurKernelInputs inputs = { clEnv, device, imageBuffer,
      filteredImageBuffer, *imageKernelBuffer, *offsetBuffer, width,
      channel_mask, image };

    if (SetMotionBlurKernelArgs(&inputs,motionBlurKernel,exception) ==
        MagickFalse)
      return(MagickFalse);
  }

  {
    const MotionBlurLaunchInputs inputs = { clEnv, device, *queue,
      *motionBlurKernel, filteredImageBuffer, image, filteredPixels,
      filteredImage_view };

    if (LaunchMotionBlurKernel(&inputs,outputReady,exception) == MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

static void DestroyMotionBlurViews(CacheView *image_view,
  CacheView *filteredImage_view)
{
  image_view=DestroyCacheView(image_view);
  if (filteredImage_view != NULL)
    filteredImage_view=DestroyCacheView(filteredImage_view);
}

typedef struct _MotionBlurFinish
{
  MagickCLEnv
    clEnv;

  cl_mem
    filteredImageBuffer,
    imageBuffer,
    imageKernelBuffer;

  cl_kernel
    motionBlurKernel;

  cl_command_queue
    queue;

  MagickCLDevice
    device;

  MagickBooleanType
    outputReady;
} MotionBlurFinish;

static Image *FinishMotionBlurImage(const MotionBlurFinish *finish,
  CacheView *image_view,CacheView *filteredImage_view,Image *filteredImage)
{
  const MagickCLEnv
    clEnv = finish->clEnv;

  const cl_mem
    filteredImageBuffer = finish->filteredImageBuffer,
    imageBuffer = finish->imageBuffer,
    imageKernelBuffer = finish->imageKernelBuffer;

  const cl_kernel
    motionBlurKernel = finish->motionBlurKernel;

  const cl_command_queue
    queue = finish->queue;

  const MagickCLDevice
    device = finish->device;

  const MagickBooleanType
    outputReady = finish->outputReady;

  DestroyMotionBlurViews(image_view,filteredImage_view);

  {
    const MotionBlurReleaseInputs inputs = { clEnv, filteredImageBuffer,
      imageBuffer, imageKernelBuffer, motionBlurKernel, queue, device };

    ReleaseMotionBlurResources(&inputs);
  }
  if (outputReady == MagickFalse && filteredImage != NULL)
    filteredImage=DestroyImage(filteredImage);

  return(filteredImage);
}

typedef struct _MotionBlurRequest
{
  const double
    *kernel;

  size_t
    width;

  const OffsetInfo
    *offset;
} MotionBlurRequest;

static Image* ComputeMotionBlurImage(const Image *image,MagickCLEnv clEnv,
  const MotionBlurRequest *request,ExceptionInfo *exception)
{
  CacheView
    *filteredImage_view,
    *image_view;

  cl_command_queue
    queue;

  cl_int
    channel_mask=get32BitChannelValue(image->channel_mask);

  cl_kernel
    motionBlurKernel;

  cl_mem
    filteredImageBuffer,
    imageBuffer,
    imageKernelBuffer,
    offsetBuffer;

  Image
    *filteredImage;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  void
    *filteredPixels;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);

  queue=NULL;
  image_view=NULL;
  filteredImage=NULL;
  filteredImage_view=NULL;
  imageBuffer=NULL;
  filteredImageBuffer=NULL;
  imageKernelBuffer=NULL;
  motionBlurKernel=NULL;
  outputReady=MagickFalse;

  {
    const MotionBlurInputTargets
      inputTargets = { &device, &image_view, &imageBuffer };

    if (AcquireMotionBlurInput(clEnv,image,exception,&inputTargets) ==
        MagickFalse)
      goto cleanup;
  }


  {
    const MotionBlurOutputInput
      input = { image, clEnv, device };

    const MotionBlurOutputTargets
      outputTargets = { &filteredImage, &filteredImage_view, &filteredPixels,
        &filteredImageBuffer };

    if (AcquireMotionBlurOutput(&input,exception,&outputTargets) ==
        MagickFalse)
      goto cleanup;
  }


  {
    const MotionBlurRunInput
      run = { clEnv, device, request->kernel, request->width,
        request->offset, imageBuffer, filteredImageBuffer, channel_mask,
        image, filteredPixels, filteredImage_view };

    const MotionBlurRunTargets
      runTargets = { &imageKernelBuffer, &queue, &offsetBuffer,
        &motionBlurKernel, &outputReady };

    if (RunMotionBlurKernel(&run,exception,&runTargets) == MagickFalse)
      goto cleanup;
  }

cleanup:

  {
    const MotionBlurFinish
      finish = { clEnv, filteredImageBuffer, imageBuffer, imageKernelBuffer,
        motionBlurKernel, queue, device, outputReady };

    return(FinishMotionBlurImage(&finish,image_view,filteredImage_view,
      filteredImage));
  }
}

MagickPrivate Image *AccelerateMotionBlurImage(const Image *image,
  const double* kernel,const size_t width,const OffsetInfo *offset,
  ExceptionInfo *exception)
{
  const MotionBlurRequest
    request = { kernel, width, offset };

  Image
    *filteredImage;

  MagickCLEnv
    clEnv;

  assert(image != NULL);
  assert(kernel != (double *) NULL);
  assert(offset != (OffsetInfo *) NULL);
  assert(exception != (ExceptionInfo *) NULL);

  if (checkAccelerateConditionRGBA(image) == MagickFalse)
    return((Image *) NULL);

  clEnv=getOpenCLEnvironment(exception);
  if (clEnv == (MagickCLEnv) NULL)
    return((Image *) NULL);

  filteredImage=ComputeMotionBlurImage(image,clEnv,&request,exception);
  return(filteredImage);
}
#endif /* MAGICKCORE_OPENCL_SUPPORT */
