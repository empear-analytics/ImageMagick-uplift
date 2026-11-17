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
%                      L       OOO    CCCC   AAA   L                          %
%                      L      O   O  C      A   A  L                          %
%                      L      O   O  C      AAAAA  L                          %
%                      L      O   O  C      A   A  L                          %
%                      LLLLL   OOO    CCCC  A   A  LLLLL                      %
%                                                                             %
%            CCCC   OOO   N   N  TTTTT  RRRR    AAA   SSSSS  TTTTT            %
%           C      O   O  NN  N    T    R   R  A   A  SS       T              %
%           C      O   O  N N N    T    RRRR   AAAAA   SSS     T              %
%           C      O   O  N  NN    T    R R    A   A     SS    T              %
%            CCCC   OOO   N   N    T    R  R   A   A  SSSSS    T              %
%                                                                             %
%                                                                             %
%             MagickCore OpenCL Accelerated Local Contrast Method             %
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A c c e l e r a t e L o c a l C o n t r a s t I m a g e                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

typedef struct _LocalContrastLaunchInfo
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  cl_kernel
    blurRowKernel;

  const Image
    *image;

  unsigned int
    passes;
} LocalContrastLaunchInfo;

static MagickBooleanType LaunchLocalContrastBlurRow(
  const LocalContrastLaunchInfo *info,ExceptionInfo *exception)
{
  const Image
    *const image = info->image;

  const MagickCLDevice
    device = info->device;

  const MagickCLEnv
    clEnv = info->clEnv;

  const cl_command_queue
    queue = info->queue;

  const cl_kernel
    blurRowKernel = info->blurRowKernel;

  const unsigned int
    passes = info->passes;

  cl_event
    event;

  cl_int
    clStatus;

  int x;
  for (x = 0; x < (int) passes; ++x) {
    size_t gsize[2];
    size_t wsize[2];
    size_t goffset[2];

    gsize[0] = 256;
    gsize[1] = (image->rows + passes - 1) / passes;
    wsize[0] = 256;
    wsize[1] = 1;
    goffset[0] = 0;
    goffset[1] = x * gsize[1];

    clStatus = clEnv->library->clEnqueueNDRangeKernel(queue, blurRowKernel, 2, goffset, gsize, wsize, 0, NULL, &event);
    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clEnqueueNDRangeKernel failed.", ".");
      return(MagickFalse);
    }
    clEnv->library->clFlush(queue);
    RecordProfileData(device,blurRowKernel,event);
  }
  return(MagickTrue);
}

typedef struct _LocalContrastResourceTargets
{
  MagickCLDevice
    *device;

  cl_command_queue
    *queue;

  CacheView
    **image_view;

  cl_mem
    *imageBuffer;
} LocalContrastResourceTargets;

static MagickBooleanType AcquireLocalContrastResources(MagickCLEnv clEnv,
  const Image *image,ExceptionInfo *exception,
  const LocalContrastResourceTargets *targets)
{
  MagickCLDevice
    *const device = targets->device;

  cl_command_queue
    *const queue = targets->queue;

  CacheView
    **const image_view = targets->image_view;

  cl_mem
    *const imageBuffer = targets->imageBuffer;

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
  *queue=AcquireOpenCLCommandQueue(*device);
  if (*queue == (cl_command_queue) NULL)
    return(MagickFalse);

  /* Create and initialize OpenCL buffers. */
  {
    *image_view=AcquireAuthenticCacheView(image,exception);
    inputPixels=GetCacheViewAuthenticPixels(*image_view,0,0,image->columns,image->rows,exception);
    if (inputPixels == (const void *) NULL)
    {
      (void) OpenCLThrowMagickException(*device,exception,GetMagickModule(),CacheWarning,"UnableToReadPixelCache.","`%s'",image->filename);
      return(MagickFalse);
    }

    /* If the host pointer is aligned to the size of CLPixelPacket,
     then use the host buffer directly from the GPU; otherwise,
     create a buffer on the GPU and copy the data over */
    if (ALIGNED(inputPixels,CLPixelPacket))
    {
      mem_flags = CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR;
    }
    else
    {
      mem_flags = CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR;
    }
    /* create a CL buffer from image pixel buffer */
    length = image->columns * image->rows;
    *imageBuffer = clEnv->library->clCreateBuffer((*device)->context, mem_flags, length * sizeof(CLPixelPacket), (void*)inputPixels, &clStatus);
    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(*device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
      return(MagickFalse);
    }
  }
  return(MagickTrue);
}

typedef struct _LocalContrastOutputInputs
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  const Image
    *image;
} LocalContrastOutputInputs;

typedef struct _LocalContrastOutputTargets
{
  Image
    **filteredImage;

  CacheView
    **filteredImage_view;

  void
    **filteredPixels;

  cl_mem
    *filteredImageBuffer;
} LocalContrastOutputTargets;

static MagickBooleanType CreateLocalContrastOutput(
  const LocalContrastOutputInputs *inputs,ExceptionInfo *exception,
  const LocalContrastOutputTargets *targets)
{
  const MagickCLEnv
    clEnv = inputs->clEnv;

  const MagickCLDevice
    device = inputs->device;

  const Image
    *const image = inputs->image;

  Image
    **const filteredImage = targets->filteredImage;

  CacheView
    **const filteredImage_view = targets->filteredImage_view;

  void
    **const filteredPixels = targets->filteredPixels;

  cl_mem
    *const filteredImageBuffer = targets->filteredImageBuffer;

  cl_int
    clStatus;

  cl_mem_flags
    mem_flags;

  size_t
    length;

  void
    *hostPtr;

  /* create output */
  {
    *filteredImage = CloneImage(image,0,0,MagickTrue,exception);
    assert(*filteredImage != NULL);
    if (SetImageStorageClass(*filteredImage,DirectClass,exception) != MagickTrue)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "CloneImage failed.", ".");
      return(MagickFalse);
    }
    *filteredImage_view=AcquireAuthenticCacheView(*filteredImage,exception);
    *filteredPixels=GetCacheViewAuthenticPixels(*filteredImage_view,0,0,(*filteredImage)->columns,(*filteredImage)->rows,exception);
    if (*filteredPixels == (void *) NULL)
    {
      (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),CacheWarning, "UnableToReadPixelCache.","`%s'",(*filteredImage)->filename);
      return(MagickFalse);
    }

    if (ALIGNED(*filteredPixels,CLPixelPacket))
    {
      mem_flags = CL_MEM_WRITE_ONLY|CL_MEM_USE_HOST_PTR;
      hostPtr = *filteredPixels;
    }
    else
    {
      mem_flags = CL_MEM_WRITE_ONLY;
      hostPtr = NULL;
    }

    /* create a CL buffer from image pixel buffer */
    length = image->columns * image->rows;
    *filteredImageBuffer = clEnv->library->clCreateBuffer(device->context, mem_flags, length * sizeof(CLPixelPacket), hostPtr, &clStatus);
    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
      return(MagickFalse);
    }
  }
  return(MagickTrue);
}

typedef struct _LocalContrastKernelInputs
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  const Image
    *image;
} LocalContrastKernelInputs;

typedef struct _LocalContrastKernelTargets
{
  cl_mem
    *tempImageBuffer;

  cl_kernel
    *blurRowKernel,
    *blurColumnKernel;
} LocalContrastKernelTargets;

static MagickBooleanType AcquireLocalContrastKernels(
  const LocalContrastKernelInputs *inputs,ExceptionInfo *exception,
  const LocalContrastKernelTargets *targets)
{
  const MagickCLEnv
    clEnv = inputs->clEnv;

  const MagickCLDevice
    device = inputs->device;

  const Image
    *const image = inputs->image;

  cl_mem
    *const tempImageBuffer = targets->tempImageBuffer;

  cl_kernel
    *const blurRowKernel = targets->blurRowKernel,
    *const blurColumnKernel = targets->blurColumnKernel;

  cl_int
    clStatus;

  size_t
    length;

    /* create temp buffer */
    {
      length = image->columns * image->rows;
      *tempImageBuffer = clEnv->library->clCreateBuffer(device->context, CL_MEM_READ_WRITE, length * sizeof(float), NULL, &clStatus);
      if (clStatus != CL_SUCCESS)
      {
        (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
        return(MagickFalse);
      }
    }

    /* get the opencl kernel */
    {
      *blurRowKernel = AcquireOpenCLKernel(device,"LocalContrastBlurRow");
      if (*blurRowKernel == NULL)
      {
        (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "AcquireOpenCLKernel failed.", ".");
        return(MagickFalse);
      };

      *blurColumnKernel = AcquireOpenCLKernel(device,"LocalContrastBlurApplyColumn");
      if (*blurColumnKernel == NULL)
      {
        (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "AcquireOpenCLKernel failed.", ".");
        return(MagickFalse);
      };
    }
  return(MagickTrue);
}

typedef struct _LocalContrastColumnInputs
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  cl_kernel
    blurColumnKernel;

  cl_mem
    imageBuffer,
    filteredImageBuffer,
    tempImageBuffer;

  cl_int
    iRadius;

  double
    strength;

  unsigned int
    imageColumns,
    imageRows,
    passes;

  const Image
    *image;
} LocalContrastColumnInputs;

static MagickBooleanType LaunchLocalContrastBlurColumn(
  const LocalContrastColumnInputs *inputs,ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = inputs->clEnv;

  const MagickCLDevice
    device = inputs->device;

  const cl_command_queue
    queue = inputs->queue;

  const cl_kernel
    blurColumnKernel = inputs->blurColumnKernel;

  const unsigned int
    passes = inputs->passes;

  const Image
    *const image = inputs->image;

  cl_event
    event;

  cl_int
    clStatus;

  cl_uint
    i;

    {
      cl_float FStrength = (cl_float) inputs->strength;
      i=0;
      clStatus=clEnv->library->clSetKernelArg(blurColumnKernel,i++,sizeof(cl_mem),(void *)&inputs->imageBuffer);
      clStatus|=clEnv->library->clSetKernelArg(blurColumnKernel,i++,sizeof(cl_mem),(void *)&inputs->filteredImageBuffer);
      clStatus|=clEnv->library->clSetKernelArg(blurColumnKernel,i++,sizeof(cl_mem),(void *)&inputs->tempImageBuffer);
      clStatus|=clEnv->library->clSetKernelArg(blurColumnKernel,i++,sizeof(unsigned int),(void *)&inputs->iRadius);
      clStatus|=clEnv->library->clSetKernelArg(blurColumnKernel,i++,sizeof(cl_float),(void *)&FStrength);
      clStatus|=clEnv->library->clSetKernelArg(blurColumnKernel,i++,sizeof(unsigned int),(void *)&inputs->imageColumns);
      clStatus|=clEnv->library->clSetKernelArg(blurColumnKernel,i++,sizeof(unsigned int),(void *)&inputs->imageRows);

      if (clStatus != CL_SUCCESS)
      {
        (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
        return(MagickFalse);
      }
    }

    /* launch the kernel */
    {
      int x;
      for (x = 0; x < (int) passes; ++x) {
        size_t gsize[2];
        size_t wsize[2];
        size_t goffset[2];

        gsize[0] = ((image->columns + 3) / 4) * 4;
        gsize[1] = ((((image->rows + 63) / 64) + (passes + 1)) / passes) * 64;
        wsize[0] = 4;
        wsize[1] = 64;
        goffset[0] = 0;
        goffset[1] = x * gsize[1];

        clStatus = clEnv->library->clEnqueueNDRangeKernel(queue, blurColumnKernel, 2, goffset, gsize, wsize, 0, NULL, &event);
        if (clStatus != CL_SUCCESS)
        {
          (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clEnqueueNDRangeKernel failed.", ".");
          return(MagickFalse);
        }
        clEnv->library->clFlush(queue);
        RecordProfileData(device,blurColumnKernel,event);
      }
    }
  return(MagickTrue);
}

typedef struct _LocalContrastBufferInputs
{
  MagickCLEnv
    clEnv;

  cl_mem
    imageBuffer,
    filteredImageBuffer,
    tempImageBuffer,
    imageKernelBuffer;
} LocalContrastBufferInputs;

static void ReleaseLocalContrastBuffers(const LocalContrastBufferInputs *inputs)
{
  const MagickCLEnv
    clEnv = inputs->clEnv;

  const cl_mem
    imageBuffer = inputs->imageBuffer,
    filteredImageBuffer = inputs->filteredImageBuffer,
    tempImageBuffer = inputs->tempImageBuffer,
    imageKernelBuffer = inputs->imageKernelBuffer;

  if (imageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(imageBuffer);
  if (filteredImageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(filteredImageBuffer);
  if (tempImageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(tempImageBuffer);
  if (imageKernelBuffer!=NULL)
    clEnv->library->clReleaseMemObject(imageKernelBuffer);
}

static void ReleaseLocalContrastKernels(cl_kernel blurRowKernel,
  cl_kernel blurColumnKernel,cl_command_queue queue,MagickCLDevice device)
{
  if (blurRowKernel!=NULL)
    ReleaseOpenCLKernel(blurRowKernel);
  if (blurColumnKernel!=NULL)
    ReleaseOpenCLKernel(blurColumnKernel);
  if (queue != NULL)
    ReleaseOpenCLCommandQueue(device, queue);
  if (device != NULL)
    ReleaseOpenCLDevice(device);
}

typedef struct _LocalContrastRowInputs
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  cl_kernel
    blurRowKernel;

  cl_mem
    imageBuffer,
    filteredImageBuffer,
    tempImageBuffer;

  const Image
    *image;

  double
    radius;
} LocalContrastRowInputs;

typedef struct _LocalContrastRowTargets
{
  unsigned int
    *imageColumns,
    *imageRows;

  cl_int
    *iRadius;

  unsigned int
    *passes;
} LocalContrastRowTargets;

static MagickBooleanType LaunchLocalContrastRowStage(
  const LocalContrastRowInputs *inputs,ExceptionInfo *exception,
  const LocalContrastRowTargets *targets)
{
  const MagickCLEnv
    clEnv = inputs->clEnv;

  const MagickCLDevice
    device = inputs->device;

  const cl_command_queue
    queue = inputs->queue;

  const cl_kernel
    blurRowKernel = inputs->blurRowKernel;

  const Image
    *const image = inputs->image;

  unsigned int
    *const imageColumns = targets->imageColumns,
    *const imageRows = targets->imageRows;

  cl_int
    *const iRadius = targets->iRadius;

  unsigned int
    *const passes = targets->passes;

  cl_int
    clStatus;

  cl_uint
    i;

    {
      *imageColumns = (unsigned int) image->columns;
      *imageRows = (unsigned int) image->rows;
      /* Normalized radius, 100% gives blur radius of 20% of the largest dimension */
      *iRadius = (cl_int) ((image->rows > image->columns ? image->rows : image->columns) * 0.002f * fabs(inputs->radius)); 

      *passes = (unsigned int) ((((1.0f * *imageRows) * *imageColumns * *iRadius) + 3999999999) / 4000000000.0f);
      *passes = (*passes < 1) ? 1: *passes;

      /* set the kernel arguments */
      i=0;
      clStatus=clEnv->library->clSetKernelArg(blurRowKernel,i++,sizeof(cl_mem),(void *)&inputs->imageBuffer);
      clStatus|=clEnv->library->clSetKernelArg(blurRowKernel,i++,sizeof(cl_mem),(void *)&inputs->filteredImageBuffer);
      clStatus|=clEnv->library->clSetKernelArg(blurRowKernel,i++,sizeof(cl_mem),(void *)&inputs->tempImageBuffer);
      clStatus|=clEnv->library->clSetKernelArg(blurRowKernel,i++,sizeof(cl_int),(void *)iRadius);
      clStatus|=clEnv->library->clSetKernelArg(blurRowKernel,i++,sizeof(unsigned int),(void *)imageColumns);
      clStatus|=clEnv->library->clSetKernelArg(blurRowKernel,i++,sizeof(unsigned int),(void *)imageRows);

      if (clStatus != CL_SUCCESS)
      {
        (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
        return(MagickFalse);
      }
    }

    /* launch the kernel */
    {
      const LocalContrastLaunchInfo
        info = { clEnv, device, queue, blurRowKernel, image, *passes };

      if (LaunchLocalContrastBlurRow(&info,exception) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _LocalContrastStageInputs
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  const Image
    *image;

  cl_mem
    imageBuffer,
    filteredImageBuffer;

  double
    radius,
    strength;
} LocalContrastStageInputs;

typedef struct _LocalContrastStageTargets
{
  cl_mem
    *tempImageBuffer;

  cl_kernel
    *blurRowKernel,
    *blurColumnKernel;
} LocalContrastStageTargets;

static MagickBooleanType RunLocalContrastKernels(
  const LocalContrastStageInputs *stage,ExceptionInfo *exception,
  const LocalContrastStageTargets *kernels)
{
  cl_mem
    *const tempImageBuffer = kernels->tempImageBuffer;

  cl_kernel
    *const blurRowKernel = kernels->blurRowKernel,
    *const blurColumnKernel = kernels->blurColumnKernel;

  cl_int
    iRadius;

  unsigned int
    imageColumns,
    imageRows,
    passes;

  {
    {
      const LocalContrastKernelInputs inputs = { stage->clEnv, stage->device, stage->image };
      const LocalContrastKernelTargets targets = { tempImageBuffer,
        blurRowKernel, blurColumnKernel };

      if (AcquireLocalContrastKernels(&inputs,exception,&targets) ==
          MagickFalse)
        return(MagickFalse);
    }

    {
      const LocalContrastRowInputs inputs = { stage->clEnv, stage->device, stage->queue,
        *blurRowKernel, stage->imageBuffer, stage->filteredImageBuffer, *tempImageBuffer,
        stage->image, stage->radius };
      const LocalContrastRowTargets targets = { &imageColumns, &imageRows,
        &iRadius, &passes };

      if (LaunchLocalContrastRowStage(&inputs,exception,&targets) ==
          MagickFalse)
        return(MagickFalse);
    }

    {
      const LocalContrastColumnInputs inputs = { stage->clEnv, stage->device, stage->queue,
        *blurColumnKernel, stage->imageBuffer, stage->filteredImageBuffer, *tempImageBuffer,
        iRadius, stage->strength, imageColumns, imageRows, passes, stage->image };

      if (LaunchLocalContrastBlurColumn(&inputs,exception) == MagickFalse)
        return(MagickFalse);
    }
  }
  return(MagickTrue);
}

typedef struct _LocalContrastRunInputs
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  const Image
    *image;

  cl_mem
    imageBuffer,
    filteredImageBuffer;

  double
    radius,
    strength;

  void
    *filteredPixels;

  CacheView
    *filteredImage_view;
} LocalContrastRunInputs;

typedef struct _LocalContrastRunTargets
{
  cl_mem
    *tempImageBuffer;

  cl_kernel
    *blurRowKernel,
    *blurColumnKernel;

  MagickBooleanType
    *outputReady;
} LocalContrastRunTargets;

static MagickBooleanType RunLocalContrastImage(
  const LocalContrastRunInputs *run,ExceptionInfo *exception,
  const LocalContrastRunTargets *results)
{
  cl_mem
    *const tempImageBuffer = results->tempImageBuffer;

  cl_kernel
    *const blurRowKernel = results->blurRowKernel,
    *const blurColumnKernel = results->blurColumnKernel;

  MagickBooleanType
    *const outputReady = results->outputReady;

  cl_int
    clStatus;

  size_t
    length;

  {
    const LocalContrastStageInputs inputs = { run->clEnv, run->device, run->queue, run->image,
      run->imageBuffer, run->filteredImageBuffer, run->radius, run->strength };
    const LocalContrastStageTargets targets = { tempImageBuffer,
      blurRowKernel, blurColumnKernel };

    if (RunLocalContrastKernels(&inputs,exception,&targets) == MagickFalse)
      return(MagickFalse);
  }

  /* get result */
  if (ALIGNED(run->filteredPixels,CLPixelPacket))
  {
    length = run->image->columns * run->image->rows;
    run->clEnv->library->clEnqueueMapBuffer(run->queue, run->filteredImageBuffer, CL_TRUE, CL_MAP_READ|CL_MAP_WRITE, 0, length * sizeof(CLPixelPacket), 0, NULL, NULL, &clStatus);
  }
  else
  {
    length = run->image->columns * run->image->rows;
    clStatus = run->clEnv->library->clEnqueueReadBuffer(run->queue, run->filteredImageBuffer, CL_TRUE, 0, length * sizeof(CLPixelPacket), run->filteredPixels, 0, NULL, NULL);
  }
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(run->device,exception, GetMagickModule(), ResourceLimitWarning, "Reading output image from CL buffer failed.", ".");
    return(MagickFalse);
  }

  *outputReady=SyncCacheViewAuthenticPixels(run->filteredImage_view,exception);
  return(MagickTrue);
}

static Image *DiscardLocalContrastOutput(MagickBooleanType outputReady,
  Image *filteredImage)
{
  if ((outputReady == MagickFalse) && (filteredImage != NULL))
  {
    DestroyImage(filteredImage);
    filteredImage = NULL;
  }
  return(filteredImage);
}

typedef struct _LocalContrastCleanupInputs
{
  MagickCLEnv
    clEnv;

  CacheView
    *image_view,
    *filteredImage_view;

  cl_mem
    imageBuffer,
    filteredImageBuffer,
    tempImageBuffer,
    imageKernelBuffer;

  cl_kernel
    blurRowKernel,
    blurColumnKernel;

  cl_command_queue
    queue;

  MagickCLDevice
    device;

  MagickBooleanType
    outputReady;
} LocalContrastCleanupInputs;

static Image *ReleaseLocalContrastResources(
  const LocalContrastCleanupInputs *cleanup,Image *filteredImage)
{
  CacheView
    *image_view = cleanup->image_view,
    *filteredImage_view = cleanup->filteredImage_view;

  image_view=DestroyCacheView(image_view);
  if (filteredImage_view != NULL)
    filteredImage_view=DestroyCacheView(filteredImage_view);

  {
    const LocalContrastBufferInputs inputs = { cleanup->clEnv,
      cleanup->imageBuffer, cleanup->filteredImageBuffer,
      cleanup->tempImageBuffer, cleanup->imageKernelBuffer };

    ReleaseLocalContrastBuffers(&inputs);
  }
  ReleaseLocalContrastKernels(cleanup->blurRowKernel,
    cleanup->blurColumnKernel,cleanup->queue,cleanup->device);
  filteredImage=DiscardLocalContrastOutput(cleanup->outputReady,
    filteredImage);
  return(filteredImage);
}

static Image *ComputeLocalContrastImage(const Image *image,MagickCLEnv clEnv,
  const double radius,const double strength,ExceptionInfo *exception)
{
  CacheView
    *filteredImage_view,
    *image_view;

  cl_command_queue
    queue;

  cl_kernel
    blurRowKernel,
    blurColumnKernel;

  cl_mem
    filteredImageBuffer,
    imageBuffer,
    imageKernelBuffer,
    tempImageBuffer;

  Image
    *filteredImage;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  void
    *filteredPixels;

  if (clEnv == (MagickCLEnv) NULL)
    return((Image *) NULL);

  queue=NULL;
  image_view=NULL;
  filteredImage=NULL;
  filteredImage_view=NULL;
  imageBuffer=NULL;
  filteredImageBuffer=NULL;
  tempImageBuffer=NULL;
  imageKernelBuffer=NULL;
  blurRowKernel=NULL;
  blurColumnKernel=NULL;
  outputReady=MagickFalse;

  {
    const LocalContrastResourceTargets targets = { &device, &queue,
      &image_view, &imageBuffer };

    if (AcquireLocalContrastResources(clEnv,image,exception,&targets) ==
        MagickFalse)
      goto cleanup;
  }

  {
    const LocalContrastOutputInputs inputs = { clEnv, device, image };
    const LocalContrastOutputTargets targets = { &filteredImage,
      &filteredImage_view, &filteredPixels, &filteredImageBuffer };

    if (CreateLocalContrastOutput(&inputs,exception,&targets) == MagickFalse)
      goto cleanup;
  }

  {
    const LocalContrastRunInputs inputs = { clEnv, device, queue, image,
      imageBuffer, filteredImageBuffer, radius, strength, filteredPixels,
      filteredImage_view };
    const LocalContrastRunTargets targets = { &tempImageBuffer,
      &blurRowKernel, &blurColumnKernel, &outputReady };

    if (RunLocalContrastImage(&inputs,exception,&targets) == MagickFalse)
      goto cleanup;
  }

cleanup:

  {
    const LocalContrastCleanupInputs inputs = { clEnv, image_view,
      filteredImage_view, imageBuffer, filteredImageBuffer, tempImageBuffer,
      imageKernelBuffer, blurRowKernel, blurColumnKernel, queue, device,
      outputReady };

    filteredImage=ReleaseLocalContrastResources(&inputs,filteredImage);
  }

  return(filteredImage);
}

MagickPrivate Image *AccelerateLocalContrastImage(const Image *image,
  const double radius,const double strength,ExceptionInfo *exception)
{
  return(ComputeLocalContrastImage(image,getAccelerateEnvironment(image,
    RGBAAccelerateCheck,(const char *) NULL,exception),radius,strength,
    exception));
}
#endif /* MAGICKCORE_OPENCL_SUPPORT */
