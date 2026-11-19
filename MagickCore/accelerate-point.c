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
%                      PPPP    OOO   IIIII  N   N  TTTTT                      %
%                      P   P  O   O    I    NN  N    T                        %
%                      PPPP   O   O    I    N N N    T                        %
%                      P      O   O    I    N  NN    T                        %
%                      P       OOO   IIIII  N   N    T                        %
%                                                                             %
%                                                                             %
%         MagickCore OpenCL Accelerated Function and Modulate Methods         %
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
%     A c c e l e r a t e F u n c t i o n I m a g e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

typedef struct _FunctionKernelInputs
{
  MagickCLDevice
    device;

  size_t
    number_parameters;

  const double
    *parameters;
} FunctionKernelInputs;

typedef struct _FunctionKernelTargets
{
  cl_mem
    *parametersBuffer;

  cl_kernel
    *functionKernel;
} FunctionKernelTargets;

static MagickBooleanType AcquireFunctionKernel(
  const FunctionKernelInputs *inputs,ExceptionInfo *exception,
  const FunctionKernelTargets *targets)
{
  const MagickCLDevice
    device = inputs->device;

  const size_t
    number_parameters = inputs->number_parameters;

  const double
    *const parameters = inputs->parameters;

  cl_mem
    *const parametersBuffer = targets->parametersBuffer;

  cl_kernel
    *const functionKernel = targets->functionKernel;

  float
    *parametersBufferPtr;

  size_t
    i;

  parametersBufferPtr=(float *) AcquireQuantumMemory(number_parameters,
    sizeof(float));
  if (parametersBufferPtr == (float *) NULL)
    return(MagickFalse);
  for (i=0; i<number_parameters; i++)
    parametersBufferPtr[i]=(float) parameters[i];
  *parametersBuffer=CreateOpenCLBuffer(device,CL_MEM_READ_ONLY |
    CL_MEM_COPY_HOST_PTR,number_parameters*sizeof(*parametersBufferPtr),
    parametersBufferPtr);
  parametersBufferPtr=(float *) RelinquishMagickMemory(parametersBufferPtr);
  if (*parametersBuffer == (cl_mem) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"CreateOpenCLBuffer failed.",".");
    return(MagickFalse);
  }

  *functionKernel=AcquireOpenCLKernel(device,"ComputeFunction");
  if (*functionKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _FunctionKernelArgs
{
  Image
    *image;

  cl_kernel
    functionKernel;

  cl_mem
    imageBuffer,
    parametersBuffer;

  cl_int
    channel_mask;

  MagickFunction
    function;

  size_t
    number_parameters;

  MagickCLDevice
    device;
} FunctionKernelArgs;

static MagickBooleanType SetFunctionKernelArgs(const FunctionKernelArgs *args,
  ExceptionInfo *exception)
{
  Image
    *const image = args->image;

  const cl_kernel
    functionKernel = args->functionKernel;

  const cl_mem
    imageBuffer = args->imageBuffer,
    parametersBuffer = args->parametersBuffer;

  const cl_int
    channel_mask = args->channel_mask;

  const MagickFunction
    function = args->function;

  const size_t
    number_parameters = args->number_parameters;

  const MagickCLDevice
    device = args->device;

  cl_int
    status;

  cl_uint
    number_params,
    number_channels;

  size_t
    i;

  number_channels=(cl_uint) image->number_channels;
  number_params=(cl_uint) number_parameters;

  i=0;
  status =SetOpenCLKernelArg(functionKernel,i++,sizeof(cl_mem),(void *)&imageBuffer);
  status|=SetOpenCLKernelArg(functionKernel,i++,sizeof(cl_uint),(void *)&number_channels);
  status|=SetOpenCLKernelArg(functionKernel,i++,sizeof(cl_int),&channel_mask);
  status|=SetOpenCLKernelArg(functionKernel,i++,sizeof(MagickFunction),(void *)&function);
  status|=SetOpenCLKernelArg(functionKernel,i++,sizeof(cl_uint),(void *)&number_params);
  status|=SetOpenCLKernelArg(functionKernel,i++,sizeof(cl_mem),(void *)&parametersBuffer);
  if (status != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"SetOpenCLKernelArg failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType ComputeFunctionImage(Image *image,MagickCLEnv clEnv,
  const MagickFunction function,const size_t number_parameters,
  const double *parameters,ExceptionInfo *exception)
{
  cl_command_queue
    queue;

  cl_int
    channel_mask=get32BitChannelValue(image->channel_mask);

  cl_kernel
    functionKernel;

  cl_mem
    imageBuffer,
    parametersBuffer;

  FunctionKernelInputs
    inputs;

  FunctionKernelTargets
    kernel_targets;

  MagickBooleanType
    outputReady;

  AccelerateImageBufferTargets
    targets;

  MagickCLDevice
    device;

  size_t
    gsize[2];

  TraceAccelerateImage(image);
  
  queue=NULL;
  imageBuffer=NULL;
  functionKernel=NULL;
  parametersBuffer=NULL;
  outputReady=MagickFalse;

  targets.device=(&device);
  targets.queue=(&queue);
  targets.imageBuffer=(&imageBuffer);
  if (AcquireAccelerateImageBuffer(clEnv,image,exception,&targets) == MagickFalse)
    goto cleanup;

  inputs.device=device;
  inputs.number_parameters=number_parameters;
  inputs.parameters=parameters;
  kernel_targets.parametersBuffer=(&parametersBuffer);
  kernel_targets.functionKernel=(&functionKernel);
  if (AcquireFunctionKernel(&inputs,exception,&kernel_targets) == MagickFalse)
    goto cleanup;

  {
    const FunctionKernelArgs args = { image, functionKernel, imageBuffer,
      parametersBuffer, channel_mask, function, number_parameters, device };

    if (SetFunctionKernelArgs(&args,exception) == MagickFalse)
      goto cleanup;
  }

  gsize[0]=image->columns;
  gsize[1]=image->rows;
  outputReady=EnqueueOpenCLKernel(queue,functionKernel,2,(const size_t *) NULL,
    gsize,(const size_t *) NULL,image,(const Image *) NULL,MagickFalse,
    exception);

cleanup:

  ReleaseOpenCLBuffer(imageBuffer);
  ReleaseOpenCLBuffer(parametersBuffer);
  ReleaseKernel(functionKernel);
  ReleaseAccelerateQueue(device,queue);
  return(outputReady);
}

MagickPrivate MagickBooleanType AccelerateFunctionImage(Image *image,
  const MagickFunction function,const size_t number_parameters,
  const double *parameters,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  MagickCLEnv
    clEnv;

  assert(image != NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);

  if (checkAccelerateCondition(image) == MagickFalse)
    return(MagickFalse);

  clEnv=getOpenCLEnvironment(exception);
  if (clEnv == (MagickCLEnv) NULL)
    return(MagickFalse);

  status=ComputeFunctionImage(image,clEnv,function,number_parameters,
    parameters,exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A c c e l e r a t e M o d u l a t e I m a g e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

typedef struct _ModulateResourceTargets
{
  MagickCLDevice
    *device;

  cl_command_queue
    *queue;

  CacheView
    **image_view;

  void
    **inputPixels;

  cl_mem
    *imageBuffer;
} ModulateResourceTargets;

static MagickBooleanType AcquireModulateResources(MagickCLEnv clEnv,
  Image *image,ExceptionInfo *exception,
  const ModulateResourceTargets *targets)
{
  MagickCLDevice
    *const device = targets->device;

  cl_command_queue
    *const queue = targets->queue;

  CacheView
    **const image_view = targets->image_view;

  void
    **const inputPixels = targets->inputPixels;

  cl_mem
    *const imageBuffer = targets->imageBuffer;

  cl_int
    clStatus;

  cl_mem_flags
    mem_flags;

  size_t
    length;

  *device=RequestOpenCLDevice(clEnv);
  if (*device == (MagickCLDevice) NULL)
    return(MagickFalse);
  *queue=AcquireOpenCLCommandQueue(*device);
  if (*queue == (cl_command_queue) NULL)
    return(MagickFalse);

  /* Create and initialize OpenCL buffers.
   inputPixels = AcquirePixelCachePixels(image, &length, exception);
   assume this  will get a writable image
   */
  *image_view=AcquireAuthenticCacheView(image,exception);
  *inputPixels=GetCacheViewAuthenticPixels(*image_view,0,0,image->columns,image->rows,exception);
  if (*inputPixels == (void *) NULL)
  {
    (void) OpenCLThrowMagickException(*device,exception,GetMagickModule(),CacheWarning,"UnableToReadPixelCache.","`%s'",image->filename);
    return(MagickFalse);
  }

  /* If the host pointer is aligned to the size of CLPixelPacket,
   then use the host buffer directly from the GPU; otherwise,
   create a buffer on the GPU and copy the data over
   */
  if (ALIGNED(*inputPixels,CLPixelPacket))
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR;
  }
  else
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR;
  }
  /* create a CL buffer from image pixel buffer */
  length = image->columns * image->rows;
  *imageBuffer = clEnv->library->clCreateBuffer((*device)->context, mem_flags, length * sizeof(CLPixelPacket), (void*)*inputPixels, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(*device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _ModulateKernelInputs
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  cl_mem
    imageBuffer;

  Image
    *image;

  double
    percent_brightness,
    percent_hue,
    percent_saturation;

  ColorspaceType
    colorspace;
} ModulateKernelInputs;

static MagickBooleanType LaunchModulateKernel(
  const ModulateKernelInputs *inputs,cl_kernel *modulateKernel,
  ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = inputs->clEnv;

  const MagickCLDevice
    device = inputs->device;

  const cl_command_queue
    queue = inputs->queue;

  const cl_mem
    imageBuffer = inputs->imageBuffer;

  Image
    *const image = inputs->image;

  const double
    percent_brightness = inputs->percent_brightness,
    percent_hue = inputs->percent_hue,
    percent_saturation = inputs->percent_saturation;

  const ColorspaceType
    colorspace = inputs->colorspace;

  cl_event
    event;

  cl_float
    bright,
    hue,
    saturation;

  cl_int
    color,
    clStatus;

  cl_uint
    i;

  *modulateKernel = AcquireOpenCLKernel(device, "Modulate");
  if (*modulateKernel == NULL)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "AcquireOpenCLKernel failed.", ".");
    return(MagickFalse);
  }

  bright=(cl_float) percent_brightness;
  hue=(cl_float) percent_hue;
  saturation=(cl_float) percent_saturation;
  color=(cl_int) colorspace;

  i=0;
  clStatus=clEnv->library->clSetKernelArg(*modulateKernel,i++,sizeof(cl_mem),(void *)&imageBuffer);
  clStatus|=clEnv->library->clSetKernelArg(*modulateKernel,i++,sizeof(cl_float),&bright);
  clStatus|=clEnv->library->clSetKernelArg(*modulateKernel,i++,sizeof(cl_float),&hue);
  clStatus|=clEnv->library->clSetKernelArg(*modulateKernel,i++,sizeof(cl_float),&saturation);
  clStatus|=clEnv->library->clSetKernelArg(*modulateKernel,i++,sizeof(cl_float),&color);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
    return(MagickFalse);
  }

  {
    size_t global_work_size[2];
    global_work_size[0] = image->columns;
    global_work_size[1] = image->rows;
    /* launch the kernel */
	clStatus = clEnv->library->clEnqueueNDRangeKernel(queue, *modulateKernel, 2, NULL, global_work_size, NULL, 0, NULL, &event);
    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clEnqueueNDRangeKernel failed.", ".");
      return(MagickFalse);
    }
    RecordProfileData(device,*modulateKernel,event);
  }
  return(MagickTrue);
}

typedef struct _ModulateOutputInputs
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  cl_mem
    imageBuffer;

  Image
    *image;

  CacheView
    *image_view;

  void
    *inputPixels;
} ModulateOutputInputs;

static MagickBooleanType ReadModulateOutput(
  const ModulateOutputInputs *inputs,MagickBooleanType *outputReady,
  ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = inputs->clEnv;

  const MagickCLDevice
    device = inputs->device;

  const cl_command_queue
    queue = inputs->queue;

  const cl_mem
    imageBuffer = inputs->imageBuffer;

  Image
    *const image = inputs->image;

  CacheView
    *const image_view = inputs->image_view;

  void
    *const inputPixels = inputs->inputPixels;

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

  *outputReady=SyncCacheViewAuthenticPixels(image_view,exception);
  return(MagickTrue);
}

typedef struct _ModulateReleaseInputs
{
  MagickCLEnv
    clEnv;

  cl_mem
    imageBuffer;

  cl_kernel
    modulateKernel;

  cl_command_queue
    queue;

  MagickCLDevice
    device;
} ModulateReleaseInputs;

static void ReleaseModulateResources(const ModulateReleaseInputs *inputs)
{
  const MagickCLEnv
    clEnv = inputs->clEnv;

  const cl_mem
    imageBuffer = inputs->imageBuffer;

  const cl_kernel
    modulateKernel = inputs->modulateKernel;

  const cl_command_queue
    queue = inputs->queue;

  const MagickCLDevice
    device = inputs->device;

  if (imageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(imageBuffer);
  if (modulateKernel!=NULL)
    ReleaseOpenCLKernel(modulateKernel);
  if (queue != NULL)
    ReleaseOpenCLCommandQueue(device,queue);
  if (device != NULL)
    ReleaseOpenCLDevice(device);
}

static MagickBooleanType ComputeModulateImage(Image *image,MagickCLEnv clEnv,
  const double percent_brightness,const double percent_hue,
  const double percent_saturation,const ColorspaceType colorspace,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  cl_command_queue
    queue;

  cl_kernel
    modulateKernel;

  cl_mem
    imageBuffer;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  void
    *inputPixels;

  TraceAccelerateImage(image);

  queue=NULL;
  image_view=NULL;
  inputPixels=NULL;
  imageBuffer=NULL;
  modulateKernel=NULL;
  outputReady=MagickFalse;

  /*
   * initialize opencl env
   */
  {
    const ModulateResourceTargets targets = { &device, &queue, &image_view,
      &inputPixels, &imageBuffer };

    if (AcquireModulateResources(clEnv,image,exception,&targets) ==
        MagickFalse)
      goto cleanup;
  }

  {
    const ModulateKernelInputs inputs = { clEnv, device, queue, imageBuffer,
      image, percent_brightness, percent_hue, percent_saturation, colorspace };

    if (LaunchModulateKernel(&inputs,&modulateKernel,exception) == MagickFalse)
      goto cleanup;
  }

  {
    const ModulateOutputInputs inputs = { clEnv, device, queue, imageBuffer,
      image, image_view, inputPixels };

    if (ReadModulateOutput(&inputs,&outputReady,exception) == MagickFalse)
      goto cleanup;
  }

cleanup:

  image_view=DestroyCacheView(image_view);

  {
    const ModulateReleaseInputs inputs = { clEnv, imageBuffer, modulateKernel,
      queue, device };

    ReleaseModulateResources(&inputs);
  }

  return outputReady;

}

MagickPrivate MagickBooleanType AccelerateModulateImage(Image *image,
  const double percent_brightness,const double percent_hue,
  const double percent_saturation,const ColorspaceType colorspace,
  ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  MagickCLEnv
    clEnv;

  assert(image != NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);

  if (checkAccelerateConditionRGBA(image) == MagickFalse)
    return(MagickFalse);

  if ((colorspace != HSLColorspace) && (colorspace != UndefinedColorspace))
    return(MagickFalse);

  clEnv=getOpenCLEnvironment(exception);
  if (clEnv == (MagickCLEnv) NULL)
    return(MagickFalse);

  status=ComputeModulateImage(image,clEnv,percent_brightness,percent_hue,
    percent_saturation,colorspace,exception);
  return(status);
}
#endif /* MAGICKCORE_OPENCL_SUPPORT */
