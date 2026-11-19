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
%        DDDD   EEEEE  SSSSS  PPPP   EEEEE   CCCC  K   K  L      EEEEE        %
%        D   D  E      SS     P   P  E      C      K  K   L      E            %
%        D   D  EEE     SSS   PPPP   EEE    C      KKK    L      EEE          %
%        D   D  E         SS  P      E      C      K  K   L      E            %
%        DDDD   EEEEE  SSSSS  P      EEEEE   CCCC  K   K  LLLLL  EEEEE        %
%                                                                             %
%                                                                             %
%               MagickCore OpenCL Accelerated Despeckle Method                %
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
%     A c c e l e r a t e D e s p e c k l e I m a g e                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

typedef struct _TempBufferInfo
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_mem_flags
    mem_flags;

  size_t
    length;
} TempBufferInfo;

static MagickBooleanType CreateTempImageBuffers(const TempBufferInfo *info,
  cl_mem *tempImageBuffer,ExceptionInfo *exception)
{
  const MagickCLDevice
    device = info->device;

  const MagickCLEnv
    clEnv = info->clEnv;

  const cl_mem_flags
    mem_flags = info->mem_flags;

  const size_t
    length = info->length;

  cl_int
    clStatus;

  int
    k;

  for (k = 0; k < 2; k++)
  {
    tempImageBuffer[k] = clEnv->library->clCreateBuffer(device->context, mem_flags, length * sizeof(CLPixelPacket), NULL, &clStatus);
    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
      return(MagickFalse);
    }
  }
  return(MagickTrue);
}

static void ReleaseTempImageBuffers(MagickCLEnv clEnv,cl_mem *tempImageBuffer)
{
  int
    k;

  for (k = 0; k < 2; k++)
  {
    if (tempImageBuffer[k]!=NULL)
      clEnv->library->clReleaseMemObject(tempImageBuffer[k]);
  }
}

typedef struct _HullPassLaunch
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  cl_kernel
    hullPass1,
    hullPass2;

  size_t
    *global_work_size;
} HullPassLaunch;

static MagickBooleanType EnqueueHullPasses(const HullPassLaunch *launch,
  ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = launch->clEnv;

  const MagickCLDevice
    device = launch->device;

  const cl_command_queue
    queue = launch->queue;

  const cl_kernel
    hullPass1 = launch->hullPass1,
    hullPass2 = launch->hullPass2;

  size_t
    *const global_work_size = launch->global_work_size;

  cl_event
    event;

  cl_int
    clStatus;

  /* launch the kernel */
  clStatus = clEnv->library->clEnqueueNDRangeKernel(queue, hullPass1, 2, NULL, global_work_size, NULL, 0, NULL, &event);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clEnqueueNDRangeKernel failed.", ".");
    return(MagickFalse);
  }
  RecordProfileData(device,hullPass1,event);

  /* launch the kernel */
  clStatus = clEnv->library->clEnqueueNDRangeKernel(queue, hullPass2, 2, NULL, global_work_size, NULL, 0, NULL, &event);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clEnqueueNDRangeKernel failed.", ".");
    return(MagickFalse);
  }
  RecordProfileData(device,hullPass2,event);
  return(MagickTrue);
}

typedef struct _DespeckleInputTargets
{
  MagickCLDevice
    *device;

  cl_command_queue
    *queue;

  CacheView
    **image_view;

  cl_mem
    *imageBuffer;
} DespeckleInputTargets;

static MagickBooleanType AcquireDespeckleInput(MagickCLEnv clEnv,
  const Image *image,ExceptionInfo *exception,
  const DespeckleInputTargets *targets)
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

  *image_view=AcquireAuthenticCacheView(image,exception);
  inputPixels=GetCacheViewAuthenticPixels(*image_view,0,0,image->columns,image->rows,exception);
  if (inputPixels == (void *) NULL)
  {
    (void) OpenCLThrowMagickException(*device,exception,GetMagickModule(),CacheWarning,"UnableToReadPixelCache.","`%s'",image->filename);
    return(MagickFalse);
  }

  mem_flags=GetInputBufferFlags(inputPixels);
  /* create a CL buffer from image pixel buffer */
  length = image->columns * image->rows;
  *imageBuffer = clEnv->library->clCreateBuffer((*device)->context, mem_flags, length * sizeof(CLPixelPacket), (void*)inputPixels, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(*device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _DespeckleOutputSource
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  const Image
    *image;

  cl_mem
    *tempImageBuffer;
} DespeckleOutputSource;

typedef struct _DespeckleOutputTargets
{
  Image
    **filteredImage;

  CacheView
    **filteredImage_view;

  void
    **filteredPixels;

  cl_mem
    *filteredImageBuffer;
} DespeckleOutputTargets;

static MagickBooleanType AcquireDespeckleOutput(
  const DespeckleOutputSource *source,ExceptionInfo *exception,
  const DespeckleOutputTargets *targets)
{
  const MagickCLEnv
    clEnv = source->clEnv;

  const MagickCLDevice
    device = source->device;

  const Image
    *const image = source->image;

  cl_mem
    *const tempImageBuffer = source->tempImageBuffer;

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

  mem_flags = CL_MEM_READ_WRITE;
  length = image->columns * image->rows;
  {
    const TempBufferInfo
      info = { clEnv, device, mem_flags, length };

    if (CreateTempImageBuffers(&info,tempImageBuffer,exception) == MagickFalse)
      return(MagickFalse);
  }

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
  return(MagickTrue);
}

static const int
  X[4] = {0, 1, 1,-1},
  Y[4] = {1, 0, 1, 1};

typedef struct _HullPassArgsInput
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_kernel
    *hullPass1,
    *hullPass2;

  cl_mem
    imageBuffer,
    *tempImageBuffer;

  const Image
    *image;
} HullPassArgsInput;

static MagickBooleanType SetHullPassArgs(const HullPassArgsInput *input,
  ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = input->clEnv;

  const MagickCLDevice
    device = input->device;

  cl_kernel
    *const hullPass1 = input->hullPass1,
    *const hullPass2 = input->hullPass2;

  cl_mem
    imageBuffer = input->imageBuffer,
    *const tempImageBuffer = input->tempImageBuffer;

  const Image
    *const image = input->image;

  cl_int
    clStatus;

  int
    matte;

  unsigned int
    imageHeight,
    imageWidth;

  clStatus =clEnv->library->clSetKernelArg(*hullPass1,0,sizeof(cl_mem),(void *)&imageBuffer);
  clStatus |=clEnv->library->clSetKernelArg(*hullPass1,1,sizeof(cl_mem),(void *)(tempImageBuffer+1));
  imageWidth = (unsigned int) image->columns;
  clStatus |=clEnv->library->clSetKernelArg(*hullPass1,2,sizeof(unsigned int),(void *)&imageWidth);
  imageHeight = (unsigned int) image->rows;
  clStatus |=clEnv->library->clSetKernelArg(*hullPass1,3,sizeof(unsigned int),(void *)&imageHeight);
  matte = (image->alpha_trait > CopyPixelTrait)?1:0;
  clStatus |=clEnv->library->clSetKernelArg(*hullPass1,6,sizeof(int),(void *)&matte);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
    return(MagickFalse);
  }

  clStatus = clEnv->library->clSetKernelArg(*hullPass2,0,sizeof(cl_mem),(void *)(tempImageBuffer+1));
  clStatus |=clEnv->library->clSetKernelArg(*hullPass2,1,sizeof(cl_mem),(void *)tempImageBuffer);
  imageWidth = (unsigned int) image->columns;
  clStatus |=clEnv->library->clSetKernelArg(*hullPass2,2,sizeof(unsigned int),(void *)&imageWidth);
  imageHeight = (unsigned int) image->rows;
  clStatus |=clEnv->library->clSetKernelArg(*hullPass2,3,sizeof(unsigned int),(void *)&imageHeight);
  matte = (image->alpha_trait > CopyPixelTrait)?1:0;
  clStatus |=clEnv->library->clSetKernelArg(*hullPass2,6,sizeof(int),(void *)&matte);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
    return(MagickFalse);
  }

  return(MagickTrue);
}

typedef struct _DespeckleRunInput
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  cl_mem
    imageBuffer,
    *tempImageBuffer,
    filteredImageBuffer;

  void
    *filteredPixels;

  const Image
    *image;
} DespeckleRunInput;

typedef struct _DespeckleRunTargets
{
  cl_kernel
    *hullPass1,
    *hullPass2;
} DespeckleRunTargets;

static MagickBooleanType RunDespeckleKernels(const DespeckleRunInput *run,
  ExceptionInfo *exception,const DespeckleRunTargets *targets)
{
  const MagickCLEnv
    clEnv = run->clEnv;

  const MagickCLDevice
    device = run->device;

  const cl_command_queue
    queue = run->queue;

  cl_mem
    imageBuffer = run->imageBuffer,
    *const tempImageBuffer = run->tempImageBuffer,
    filteredImageBuffer = run->filteredImageBuffer;

  void
    *const filteredPixels = run->filteredPixels;

  const Image
    *const image = run->image;

  cl_kernel
    *const hullPass1 = targets->hullPass1,
    *const hullPass2 = targets->hullPass2;

  cl_int
    clStatus;

  int
    k;

  size_t
    global_work_size[2],
    length;

  *hullPass1 = AcquireOpenCLKernel(device,"HullPass1");
  *hullPass2 = AcquireOpenCLKernel(device,"HullPass2");

  {
    const HullPassArgsInput
      args = { clEnv, device, hullPass1, hullPass2, imageBuffer,
        tempImageBuffer, image };

    if (SetHullPassArgs(&args,exception) == MagickFalse)
      return(MagickFalse);
  }

  global_work_size[0] = image->columns;
  global_work_size[1] = image->rows;


  for (k = 0; k < 4; k++)
  {
    cl_int2 offset;
    int polarity;


    offset.s[0] = X[k];
    offset.s[1] = Y[k];
    polarity = 1;
    clStatus = clEnv->library->clSetKernelArg(*hullPass1,4,sizeof(cl_int2),(void *)&offset);
    clStatus|= clEnv->library->clSetKernelArg(*hullPass1,5,sizeof(int),(void *)&polarity);
    clStatus|=clEnv->library->clSetKernelArg(*hullPass2,4,sizeof(cl_int2),(void *)&offset);
    clStatus|=clEnv->library->clSetKernelArg(*hullPass2,5,sizeof(int),(void *)&polarity);
    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
      return(MagickFalse);
    }
    {
      const HullPassLaunch
        launch = { clEnv, device, queue, *hullPass1, *hullPass2,
          global_work_size };

      if (EnqueueHullPasses(&launch,exception) == MagickFalse)
        return(MagickFalse);
    }

    if (k == 0)
      clStatus =clEnv->library->clSetKernelArg(*hullPass1,0,sizeof(cl_mem),(void *)(tempImageBuffer));
    offset.s[0] = -X[k];
    offset.s[1] = -Y[k];
    polarity = 1;
    clStatus = clEnv->library->clSetKernelArg(*hullPass1,4,sizeof(cl_int2),(void *)&offset);
    clStatus|= clEnv->library->clSetKernelArg(*hullPass1,5,sizeof(int),(void *)&polarity);
    clStatus|=clEnv->library->clSetKernelArg(*hullPass2,4,sizeof(cl_int2),(void *)&offset);
    clStatus|=clEnv->library->clSetKernelArg(*hullPass2,5,sizeof(int),(void *)&polarity);
    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
      return(MagickFalse);
    }
    {
      const HullPassLaunch
        launch = { clEnv, device, queue, *hullPass1, *hullPass2,
          global_work_size };

      if (EnqueueHullPasses(&launch,exception) == MagickFalse)
        return(MagickFalse);
    }

    offset.s[0] = -X[k];
    offset.s[1] = -Y[k];
    polarity = -1;
    clStatus = clEnv->library->clSetKernelArg(*hullPass1,4,sizeof(cl_int2),(void *)&offset);
    clStatus|= clEnv->library->clSetKernelArg(*hullPass1,5,sizeof(int),(void *)&polarity);
    clStatus|=clEnv->library->clSetKernelArg(*hullPass2,4,sizeof(cl_int2),(void *)&offset);
    clStatus|=clEnv->library->clSetKernelArg(*hullPass2,5,sizeof(int),(void *)&polarity);
    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
      return(MagickFalse);
    }
    {
      const HullPassLaunch
        launch = { clEnv, device, queue, *hullPass1, *hullPass2,
          global_work_size };

      if (EnqueueHullPasses(&launch,exception) == MagickFalse)
        return(MagickFalse);
    }

    offset.s[0] = X[k];
    offset.s[1] = Y[k];
    polarity = -1;
    clStatus = clEnv->library->clSetKernelArg(*hullPass1,4,sizeof(cl_int2),(void *)&offset);
    clStatus|= clEnv->library->clSetKernelArg(*hullPass1,5,sizeof(int),(void *)&polarity);
    clStatus|=clEnv->library->clSetKernelArg(*hullPass2,4,sizeof(cl_int2),(void *)&offset);
    clStatus|=clEnv->library->clSetKernelArg(*hullPass2,5,sizeof(int),(void *)&polarity);

    if (k == 3)
      clStatus |=clEnv->library->clSetKernelArg(*hullPass2,1,sizeof(cl_mem),(void *)&filteredImageBuffer);

    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
      return(MagickFalse);
    }
    {
      const HullPassLaunch
        launch = { clEnv, device, queue, *hullPass1, *hullPass2,
          global_work_size };

      if (EnqueueHullPasses(&launch,exception) == MagickFalse)
        return(MagickFalse);
    }
  }

  if (ALIGNED(filteredPixels,CLPixelPacket))
  {
    length = image->columns * image->rows;
    clEnv->library->clEnqueueMapBuffer(queue, filteredImageBuffer, CL_TRUE, CL_MAP_READ|CL_MAP_WRITE, 0, length * sizeof(CLPixelPacket), 0, NULL, NULL, &clStatus);
  }
  else
  {
    length = image->columns * image->rows;
    clStatus = clEnv->library->clEnqueueReadBuffer(queue, filteredImageBuffer, CL_TRUE, 0, length * sizeof(CLPixelPacket), filteredPixels, 0, NULL, NULL);
  }
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "Reading output image from CL buffer failed.", ".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

static Image *ComputeDespeckleImage(const Image *image,MagickCLEnv clEnv,
  ExceptionInfo*exception)
{
  CacheView
    *filteredImage_view=NULL,
    *image_view=NULL;

  cl_command_queue
    queue=NULL;

  cl_kernel
    hullPass1=NULL,
    hullPass2=NULL;

  cl_mem
    filteredImageBuffer=NULL,
    imageBuffer=NULL,
    tempImageBuffer[2];

  Image
    *filteredImage=NULL;

  MagickBooleanType
    outputReady=MagickFalse;

  MagickCLDevice
    device;

  void
    *filteredPixels=NULL;

  if (clEnv == (MagickCLEnv) NULL)
    return((Image *) NULL);

  tempImageBuffer[0]=NULL;
  tempImageBuffer[1]=NULL;

  {
    const DespeckleInputTargets
      inputTargets = { &device, &queue, &image_view, &imageBuffer };

    if (AcquireDespeckleInput(clEnv,image,exception,&inputTargets) ==
        MagickFalse)
      goto cleanup;
  }

  {
    const DespeckleOutputSource
      source = { clEnv, device, image, tempImageBuffer };

    const DespeckleOutputTargets
      outputTargets = { &filteredImage, &filteredImage_view, &filteredPixels,
        &filteredImageBuffer };

    if (AcquireDespeckleOutput(&source,exception,&outputTargets) ==
        MagickFalse)
      goto cleanup;
  }

  {
    const DespeckleRunInput
      run = { clEnv, device, queue, imageBuffer, tempImageBuffer,
        filteredImageBuffer, filteredPixels, image };

    const DespeckleRunTargets
      runTargets = { &hullPass1, &hullPass2 };

    if (RunDespeckleKernels(&run,exception,&runTargets) == MagickFalse)
      goto cleanup;
  }

  outputReady=SyncCacheViewAuthenticPixels(filteredImage_view,exception);

cleanup:

  DestroyAccelerateViews(image_view,filteredImage_view);

  if (queue != NULL)
    ReleaseOpenCLCommandQueue(device,queue);
  if (device != NULL)
    ReleaseOpenCLDevice(device);
  if (imageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(imageBuffer);
  ReleaseTempImageBuffers(clEnv,tempImageBuffer);
  if (filteredImageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(filteredImageBuffer);
  if (hullPass1!=NULL)
    ReleaseOpenCLKernel(hullPass1);
  if (hullPass2!=NULL)
    ReleaseOpenCLKernel(hullPass2);
  if (outputReady == MagickFalse && filteredImage != NULL)
    filteredImage=DestroyImage(filteredImage);

  return(filteredImage);
}

MagickPrivate Image *AccelerateDespeckleImage(const Image* image,
  ExceptionInfo* exception)
{
  return(ComputeDespeckleImage(image,getAccelerateEnvironment(image,
    RGBAAccelerateCheck,(const char *) NULL,exception),exception));
}
#endif /* MAGICKCORE_OPENCL_SUPPORT */
