/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     AAA     CCCC    CCCC  EEEEE  L      EEEEE  RRRR    AAA   TTTTT  EEEEE   %
%    A   A   C       C      E      L      E      R   R  A   A    T    E       %
%    AAAAA   C       C      EEE    L      EEE    RRRR   AAAAA    T    EEE     %
%    A   A   C       C      E      L      E      R R    A   A    T    E       %
%    A   A    CCCC    CCCC  EEEEE  LLLLL  EEEEE  R  R   A   A    T    EEEEE   %
%                                                                             %
%                                                                             %
%                       MagickCore Acceleration Methods                       %
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

#define MAGICK_MIN(x,y) (((x) <= (y))?(x):(y))

#if defined(MAGICKCORE_OPENCL_SUPPORT)

static MagickBooleanType IsRGBAChannelMask(const Image *image)
{
  return((((image->channel_mask & RedChannel) != 0) &&
      ((image->channel_mask & GreenChannel) != 0) &&
      ((image->channel_mask & BlueChannel) != 0) &&
      ((image->channel_mask & AlphaChannel) != 0)) ? MagickTrue : MagickFalse);
}

static Image *cloneImage(const Image* image,ExceptionInfo *exception)
{
  Image
    *clone;

  if (IsRGBAChannelMask(image) != MagickFalse)
    clone=CloneImage(image,0,0,MagickTrue,exception);
  else
    {
      clone=CloneImage(image,0,0,MagickTrue,exception);
      if (clone != (Image *) NULL)
        SyncImagePixelCache(clone,exception);
    }
  return(clone);
}

/* pad the global workgroup size to the next multiple of
   the local workgroup size */
inline static unsigned int padGlobalWorkgroupSizeToLocalWorkgroupSize(
  const unsigned int orgGlobalSize,const unsigned int localGroupSize)
{
  return ((orgGlobalSize+(localGroupSize-1))/localGroupSize*localGroupSize);
}

static cl_mem createKernelInfo(MagickCLDevice device,const double radius,
  const double sigma,cl_uint *width,ExceptionInfo *exception)
{
  char
    geometry[MagickPathExtent];

  cl_mem
    imageKernelBuffer;

  float
    *kernelBufferPtr;

  KernelInfo
    *kernel;

  ssize_t
    i;

  (void) FormatLocaleString(geometry,MagickPathExtent,
    "blur:%.17gx%.17g;blur:%.17gx%.17g+90",radius,sigma,radius,sigma);
  kernel=AcquireKernelInfo(geometry,exception);
  if (kernel == (KernelInfo *) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireKernelInfo failed.",".");
    return((cl_mem) NULL);
  }
  kernelBufferPtr=(float *) AcquireMagickMemory(kernel->width*
    sizeof(*kernelBufferPtr));
  if (kernelBufferPtr == (float *) NULL)
    {
      kernel=DestroyKernelInfo(kernel);
      (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
        ResourceLimitWarning,"MemoryAllocationFailed.",".");
      return((cl_mem) NULL);
    }
  for (i = 0; i < (ssize_t) kernel->width; i++)
    kernelBufferPtr[i]=(float) kernel->values[i];
  imageKernelBuffer=CreateOpenCLBuffer(device,CL_MEM_COPY_HOST_PTR |
    CL_MEM_READ_ONLY,kernel->width*sizeof(*kernelBufferPtr),kernelBufferPtr);
  *width=(cl_uint) kernel->width;
  kernelBufferPtr=(float *) RelinquishMagickMemory(kernelBufferPtr);
  kernel=DestroyKernelInfo(kernel);
  if (imageKernelBuffer == (cl_mem) NULL)
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"CreateOpenCLBuffer failed.",".");
  return(imageKernelBuffer);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A c c e l e r a t e B l u r I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

typedef struct _AccelerateBufferTargets
{
  MagickCLDevice
    *device;

  cl_command_queue
    *queue;

  Image
    **filteredImage;

  cl_mem
    *imageBuffer,
    *filteredImageBuffer;
} AccelerateBufferTargets;

static MagickBooleanType AcquireAccelerateBuffers(MagickCLEnv clEnv,
  const Image *image,ExceptionInfo *exception,
  const AccelerateBufferTargets *targets)
{
  MagickCLDevice
    *const device = targets->device;

  cl_command_queue
    *const queue = targets->queue;

  Image
    **const filteredImage = targets->filteredImage;

  cl_mem
    *const imageBuffer = targets->imageBuffer,
    *const filteredImageBuffer = targets->filteredImageBuffer;

  *device=RequestOpenCLDevice(clEnv);
  if (*device == (MagickCLDevice) NULL)
    return(MagickFalse);
  *queue=AcquireOpenCLCommandQueue(*device);
  if (*queue == (cl_command_queue) NULL)
    return(MagickFalse);
  *filteredImage=cloneImage(image,exception);
  if (*filteredImage == (Image *) NULL)
    return(MagickFalse);
  if ((*filteredImage)->number_channels != image->number_channels)
    return(MagickFalse);
  *imageBuffer=GetAuthenticOpenCLBuffer(image,*device,exception);
  if (*imageBuffer == (cl_mem) NULL)
    return(MagickFalse);
  *filteredImageBuffer=GetAuthenticOpenCLBuffer(*filteredImage,*device,exception);
  if (*filteredImageBuffer == (cl_mem) NULL)
    return(MagickFalse);
  return(MagickTrue);
}

static void
  TraceAccelerateImage(const Image *);

static Image *ComputeBlurImage(const Image* image,MagickCLEnv clEnv,
  const double radius,const double sigma,ExceptionInfo *exception)
{
  cl_command_queue
    queue;

  cl_int
    channel_mask=get32BitChannelValue(image->channel_mask),
    status;

  cl_kernel
    blurColumnKernel,
    blurRowKernel;

  cl_mem
    filteredImageBuffer,
    imageBuffer,
    imageKernelBuffer,
    tempImageBuffer;

  cl_uint
    imageColumns,
    imageRows,
    kernelWidth,
    number_channels;

  Image
    *filteredImage;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  size_t
    chunkSize=256,
    gsize[2],
    i,
    length,
    lsize[2];

  const AccelerateBufferTargets
    targets = { &device, &queue, &filteredImage, &imageBuffer,
      &filteredImageBuffer };

  if (clEnv == (MagickCLEnv) NULL)
    return((Image *) NULL);

  queue=NULL;
  filteredImage=NULL;
  imageBuffer=NULL;
  filteredImageBuffer=NULL;
  tempImageBuffer=NULL;
  imageKernelBuffer=NULL;
  blurRowKernel=NULL;
  blurColumnKernel=NULL;
  outputReady=MagickFalse;

  TraceAccelerateImage(image);

  if (AcquireAccelerateBuffers(clEnv,image,exception,&targets) == MagickFalse)
    goto cleanup;

  imageKernelBuffer=createKernelInfo(device,radius,sigma,&kernelWidth,
    exception);
  if (imageKernelBuffer == (cl_mem) NULL)
    goto cleanup;

  length=image->columns*image->rows;
  tempImageBuffer=CreateOpenCLBuffer(device,CL_MEM_READ_WRITE,length*
    sizeof(cl_float4),(void *) NULL);
  if (tempImageBuffer == (cl_mem) NULL)
    goto cleanup;

  blurRowKernel=AcquireOpenCLKernel(device,"BlurRow");
  if (blurRowKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    goto cleanup;
  }

  number_channels=(cl_uint) image->number_channels;
  imageColumns=(cl_uint) image->columns;
  imageRows=(cl_uint) image->rows;

  i=0;
  status =SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_mem),(void *)&imageBuffer);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_uint),&number_channels);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_int),&channel_mask);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_mem),(void *)&imageKernelBuffer);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_uint),(void *)&kernelWidth);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_uint),(void *)&imageColumns);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_uint),(void *)&imageRows);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_float4)*(chunkSize+kernelWidth),(void *) NULL);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_mem),(void *)&tempImageBuffer);
  if (status != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"SetOpenCLKernelArg failed.",".");
    goto cleanup;
  }

  gsize[0]=chunkSize*((image->columns+chunkSize-1)/chunkSize);
  gsize[1]=image->rows;
  lsize[0]=chunkSize;
  lsize[1]=1;

  outputReady=EnqueueOpenCLKernel(queue,blurRowKernel,2,(size_t *) NULL,gsize,
    lsize,image,filteredImage,MagickFalse,exception);
  if (outputReady == MagickFalse)
    goto cleanup;

  blurColumnKernel=AcquireOpenCLKernel(device,"BlurColumn");
  if (blurColumnKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    goto cleanup;
  }

  i=0;
  status =SetOpenCLKernelArg(blurColumnKernel,i++,sizeof(cl_mem),(void *)&tempImageBuffer);
  status|=SetOpenCLKernelArg(blurColumnKernel,i++,sizeof(cl_uint),&number_channels);
  status|=SetOpenCLKernelArg(blurColumnKernel,i++,sizeof(cl_int),&channel_mask);
  status|=SetOpenCLKernelArg(blurColumnKernel,i++,sizeof(cl_mem),(void *)&imageKernelBuffer);
  status|=SetOpenCLKernelArg(blurColumnKernel,i++,sizeof(cl_uint),(void *)&kernelWidth);
  status|=SetOpenCLKernelArg(blurColumnKernel,i++,sizeof(cl_uint),(void *)&imageColumns);
  status|=SetOpenCLKernelArg(blurColumnKernel,i++,sizeof(cl_uint),(void *)&imageRows);
  status|=SetOpenCLKernelArg(blurColumnKernel,i++,sizeof(cl_float4)*(chunkSize+kernelWidth),(void *) NULL);
  status|=SetOpenCLKernelArg(blurColumnKernel,i++,sizeof(cl_mem),(void *)&filteredImageBuffer);
  if (status != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"SetOpenCLKernelArg failed.",".");
    goto cleanup;
  }

  gsize[0]=image->columns;
  gsize[1]=chunkSize*((image->rows+chunkSize-1)/chunkSize);
  lsize[0]=1;
  lsize[1]=chunkSize;

  outputReady=EnqueueOpenCLKernel(queue,blurColumnKernel,2,(size_t *) NULL,gsize,
    lsize,image,filteredImage,MagickFalse,exception);

cleanup:

  ReleaseOpenCLBuffer(imageBuffer);
  ReleaseOpenCLBuffer(filteredImageBuffer);
  ReleaseOpenCLBuffer(tempImageBuffer);
  ReleaseOpenCLBuffer(imageKernelBuffer);
  ReleaseKernel(blurRowKernel);
  ReleaseKernel(blurColumnKernel);
  ReleaseAccelerateQueue(device,queue);
  filteredImage=DestroyUnfinishedImage(outputReady,filteredImage);

  return(filteredImage);
}

MagickPrivate Image* AccelerateBlurImage(const Image *image,
  const double radius,const double sigma,ExceptionInfo *exception)
{
  return(ComputeBlurImage(image,getAccelerateEnvironment(image,
    BasicAccelerateCheck,__func__,exception),radius,sigma,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A c c e l e r a t e C o n t r a s t I m a g e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

typedef struct _AccelerateImageBufferTargets
{
  MagickCLDevice
    *device;

  cl_command_queue
    *queue;

  cl_mem
    *imageBuffer;
} AccelerateImageBufferTargets;

static MagickBooleanType AcquireAccelerateImageBuffer(MagickCLEnv clEnv,
  const Image *image,ExceptionInfo *exception,
  const AccelerateImageBufferTargets *targets)
{
  MagickCLDevice
    *const device = targets->device;

  cl_command_queue
    *const queue = targets->queue;

  cl_mem
    *const imageBuffer = targets->imageBuffer;

  *device=RequestOpenCLDevice(clEnv);
  if (*device == (MagickCLDevice) NULL)
    return(MagickFalse);
  *queue=AcquireOpenCLCommandQueue(*device);
  if (*queue == (cl_command_queue) NULL)
    return(MagickFalse);
  *imageBuffer=GetAuthenticOpenCLBuffer(image,*device,exception);
  if (*imageBuffer == (cl_mem) NULL)
    return(MagickFalse);
  return(MagickTrue);
}

static void TraceAccelerateImage(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
}

static inline int ContrastSign(const MagickBooleanType sharpen)
{
  return(sharpen != MagickFalse ? 1 : -1);
}

static MagickBooleanType ComputeContrastImage(Image *image,MagickCLEnv clEnv,
  const MagickBooleanType sharpen,ExceptionInfo *exception)
{
  cl_command_queue
    queue;

  cl_int
    status,
    sign;

  cl_kernel
    contrastKernel;

  cl_mem
    imageBuffer;

  cl_uint
    number_channels;

  MagickBooleanType
    outputReady;

  AccelerateImageBufferTargets
    targets;

  MagickCLDevice
    device;

  size_t
    gsize[2],
    i;

  if (clEnv == (MagickCLEnv) NULL)
    return(MagickFalse);

  TraceAccelerateImage(image);

  queue=NULL;
  contrastKernel=NULL;
  imageBuffer=NULL;
  outputReady=MagickFalse;

  targets.device=(&device);
  targets.queue=(&queue);
  targets.imageBuffer=(&imageBuffer);
  if (AcquireAccelerateImageBuffer(clEnv,image,exception,&targets) == MagickFalse)
    goto cleanup;

  contrastKernel=AcquireOpenCLKernel(device,"Contrast");
  if (contrastKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    goto cleanup;
  }

  number_channels=(cl_uint) image->number_channels;
  sign=ContrastSign(sharpen);

  i=0;
  status =SetOpenCLKernelArg(contrastKernel,i++,sizeof(cl_mem),(void *)&imageBuffer);
  status|=SetOpenCLKernelArg(contrastKernel,i++,sizeof(cl_uint),&number_channels);
  status|=SetOpenCLKernelArg(contrastKernel,i++,sizeof(cl_int),&sign);
  if (status != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"SetOpenCLKernelArg failed.",".");
    goto cleanup;
  }

  gsize[0]=image->columns;
  gsize[1]=image->rows;

  outputReady=EnqueueOpenCLKernel(queue,contrastKernel,2,(const size_t *) NULL,
    gsize,(const size_t *) NULL,image,(Image *) NULL,MagickFalse,exception);

cleanup:

  ReleaseOpenCLBuffer(imageBuffer);
  ReleaseKernel(contrastKernel);
  ReleaseAccelerateQueue(device,queue);

  return(outputReady);
}

MagickPrivate MagickBooleanType AccelerateContrastImage(Image *image,
  const MagickBooleanType sharpen,ExceptionInfo *exception)
{
  return(ComputeContrastImage(image,getAccelerateEnvironment(image,
    BasicAccelerateCheck,__func__,exception),sharpen,exception));
}


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
%     A c c e l e r a t e G r a y s c a l e I m a g e                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

static void ReleaseGrayscaleResources(cl_mem imageBuffer,
  cl_kernel grayscaleKernel,cl_command_queue queue,MagickCLDevice device)
{
  if (imageBuffer != (cl_mem) NULL)
    ReleaseOpenCLMemObject(imageBuffer);
  if (grayscaleKernel != (cl_kernel) NULL)
    ReleaseOpenCLKernel(grayscaleKernel);
  if (queue != (cl_command_queue) NULL)
    ReleaseOpenCLCommandQueue(device,queue);
  if (device != (MagickCLDevice) NULL)
    ReleaseOpenCLDevice(device);
}

typedef struct _GrayscaleResourceTargets
{
  cl_command_queue
    *queue;

  cl_mem
    *imageBuffer;

  cl_kernel
    *grayscaleKernel;
} GrayscaleResourceTargets;

static MagickBooleanType AcquireGrayscaleResources(MagickCLDevice device,
  Image *image,ExceptionInfo *exception,
  const GrayscaleResourceTargets *targets)
{
  cl_command_queue
    *const queue = targets->queue;

  cl_mem
    *const imageBuffer = targets->imageBuffer;

  cl_kernel
    *const grayscaleKernel = targets->grayscaleKernel;

  *queue=AcquireOpenCLCommandQueue(device);
  if (*queue == (cl_command_queue) NULL)
    return(MagickFalse);
  *imageBuffer=GetAuthenticOpenCLBuffer(image,device,exception);
  if (*imageBuffer == (cl_mem) NULL)
    return(MagickFalse);

  *grayscaleKernel=AcquireOpenCLKernel(device,"Grayscale");
  if (*grayscaleKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType ComputeGrayscaleImage(Image *image,MagickCLEnv clEnv,
  const PixelIntensityMethod method,ExceptionInfo *exception)
{
  cl_command_queue
    queue;

  cl_int
    status;

  cl_kernel
    grayscaleKernel;

  cl_mem
    imageBuffer;

  cl_uint
    number_channels,
    colorspace,
    intensityMethod;

  GrayscaleResourceTargets
    targets;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  size_t
    gsize[2],
    i;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);

  queue=NULL;
  imageBuffer=NULL;
  grayscaleKernel=NULL;
  outputReady=MagickFalse;

  device=RequestOpenCLDevice(clEnv);
  if (device == (MagickCLDevice) NULL)
    goto cleanup;
  targets.queue=(&queue);
  targets.imageBuffer=(&imageBuffer);
  targets.grayscaleKernel=(&grayscaleKernel);
  if (AcquireGrayscaleResources(device,image,exception,&targets) ==
      MagickFalse)
    goto cleanup;

  number_channels=(cl_uint) image->number_channels;
  intensityMethod=(cl_uint) method;
  colorspace=(cl_uint) image->colorspace;

  i=0;
  status =SetOpenCLKernelArg(grayscaleKernel,i++,sizeof(cl_mem),(void *)&imageBuffer);
  status|=SetOpenCLKernelArg(grayscaleKernel,i++,sizeof(cl_uint),&number_channels);
  status|=SetOpenCLKernelArg(grayscaleKernel,i++,sizeof(cl_uint),&colorspace);
  status|=SetOpenCLKernelArg(grayscaleKernel,i++,sizeof(cl_uint),&intensityMethod);
  if (status != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"SetOpenCLKernelArg failed.",".");
    goto cleanup;
  }

  gsize[0]=image->columns;
  gsize[1]=image->rows;
  outputReady=EnqueueOpenCLKernel(queue,grayscaleKernel,2,
    (const size_t *) NULL,gsize,(const size_t *) NULL,image,(Image *) NULL,
    MagickFalse,exception);

cleanup:

  ReleaseGrayscaleResources(imageBuffer,grayscaleKernel,queue,device);

  return(outputReady);
}

static MagickBooleanType HasGrayscaleInputChannels(const Image *image)
{
  if (image->number_channels < 3)
    return(MagickFalse);

  if (GetPixelRedTraits(image) == UndefinedPixelTrait)
    return(MagickFalse);
  if (GetPixelGreenTraits(image) == UndefinedPixelTrait)
    return(MagickFalse);
  if (GetPixelBlueTraits(image) == UndefinedPixelTrait)
    return(MagickFalse);
  return(MagickTrue);
}

MagickPrivate MagickBooleanType AccelerateGrayscaleImage(Image* image,
  const PixelIntensityMethod method,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  MagickCLEnv
    clEnv;

  assert(image != NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);

  if ((checkAccelerateCondition(image) == MagickFalse) ||
      (checkPixelIntensity(image,method) == MagickFalse))
    return(MagickFalse);

  if (HasGrayscaleInputChannels(image) == MagickFalse)
    return(MagickFalse);

  clEnv=getOpenCLEnvironment(exception);
  if (clEnv == (MagickCLEnv) NULL)
    return(MagickFalse);

  status=ComputeGrayscaleImage(image,clEnv,method,exception);
  return(status);
}

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

  image_view=DestroyCacheView(image_view);
  if (filteredImage_view != NULL)
    filteredImage_view=DestroyCacheView(filteredImage_view);

  {
    const LocalContrastBufferInputs inputs = { clEnv, imageBuffer,
      filteredImageBuffer, tempImageBuffer, imageKernelBuffer };

    ReleaseLocalContrastBuffers(&inputs);
  }
  ReleaseLocalContrastKernels(blurRowKernel,blurColumnKernel,queue,device);
  filteredImage=DiscardLocalContrastOutput(outputReady,filteredImage);

  return(filteredImage);
}

MagickPrivate Image *AccelerateLocalContrastImage(const Image *image,
  const double radius,const double strength,ExceptionInfo *exception)
{
  return(ComputeLocalContrastImage(image,getAccelerateEnvironment(image,
    RGBAAccelerateCheck,(const char *) NULL,exception),radius,strength,
    exception));
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

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);

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

static Image* ComputeMotionBlurImage(const Image *image,MagickCLEnv clEnv,
  const double *kernel,const size_t width,const OffsetInfo *offset,
  ExceptionInfo *exception)
{
  CacheView
    *filteredImage_view,
    *image_view;

  cl_command_queue
    queue;

  cl_int
    channel_mask=get32BitChannelValue(image->channel_mask),
    clStatus;

  cl_kernel
    motionBlurKernel;

  cl_mem
    filteredImageBuffer,
    imageBuffer,
    imageKernelBuffer,
    offsetBuffer;

  cl_mem_flags
    mem_flags;

  const void
    *inputPixels;

  Image
    *filteredImage;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  size_t
    length;

  void
    *filteredPixels,
    *hostPtr;

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

  device=RequestOpenCLDevice(clEnv);
  if (device == (MagickCLDevice) NULL)
    goto cleanup;

  /* Create and initialize OpenCL buffers. */

  image_view=AcquireAuthenticCacheView(image,exception);
  inputPixels=GetCacheViewAuthenticPixels(image_view,0,0,image->columns,
    image->rows,exception);
  if (inputPixels == (const void *) NULL)
  {
    (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
      "UnableToReadPixelCache.","`%s'",image->filename);
    goto cleanup;
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
  imageBuffer = clEnv->library->clCreateBuffer(device->context, mem_flags,
    length * sizeof(CLPixelPacket), (void*)inputPixels, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception, GetMagickModule(),
      ResourceLimitError, "clEnv->library->clCreateBuffer failed.",".");
    goto cleanup;
  }


  filteredImage = CloneImage(image,image->columns,image->rows,
    MagickTrue,exception);
  assert(filteredImage != NULL);
  if (SetImageStorageClass(filteredImage,DirectClass,exception) != MagickTrue)
  {
    (void) ThrowMagickException(exception, GetMagickModule(),
      ResourceLimitError, "CloneImage failed.", ".");
    goto cleanup;
  }
  filteredImage_view=AcquireAuthenticCacheView(filteredImage,exception);
  filteredPixels=GetCacheViewAuthenticPixels(filteredImage_view,0,0,filteredImage->columns,filteredImage->rows,exception);
  if (filteredPixels == (void *) NULL)
  {
    (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
      "UnableToReadPixelCache.","`%s'",filteredImage->filename);
    goto cleanup;
  }

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
  filteredImageBuffer = clEnv->library->clCreateBuffer(device->context, mem_flags,
    length * sizeof(CLPixelPacket), hostPtr, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception, GetMagickModule(),
      ResourceLimitError, "clEnv->library->clCreateBuffer failed.",".");
    goto cleanup;
  }


  {
    const MotionBlurFilterInputs inputs = { clEnv, device, kernel, width };
    const MotionBlurFilterTargets targets = { &imageKernelBuffer, &queue };

    if (CreateMotionBlurFilterBuffer(&inputs,exception,&targets) ==
        MagickFalse)
      goto cleanup;
  }

  offsetBuffer = clEnv->library->clCreateBuffer(device->context,
    CL_MEM_READ_ONLY|CL_MEM_ALLOC_HOST_PTR, width * sizeof(cl_int2), NULL,
    &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception, GetMagickModule(),
      ResourceLimitError, "clEnv->library->clCreateBuffer failed.",".");
    goto cleanup;
  }

  {
    const MotionBlurOffsetInputs inputs = { clEnv, queue, offsetBuffer,
      offset, width };

    if (FillMotionBlurOffsets(&inputs,exception) == MagickFalse)
      goto cleanup;
  }


  {
    const MotionBlurKernelInputs inputs = { clEnv, device, imageBuffer,
      filteredImageBuffer, imageKernelBuffer, offsetBuffer, width,
      channel_mask, image };

    if (SetMotionBlurKernelArgs(&inputs,&motionBlurKernel,exception) ==
        MagickFalse)
      goto cleanup;
  }

  {
    const MotionBlurLaunchInputs inputs = { clEnv, device, queue,
      motionBlurKernel, filteredImageBuffer, image, filteredPixels,
      filteredImage_view };

    if (LaunchMotionBlurKernel(&inputs,&outputReady,exception) == MagickFalse)
      goto cleanup;
  }

cleanup:

  image_view=DestroyCacheView(image_view);
  if (filteredImage_view != NULL)
    filteredImage_view=DestroyCacheView(filteredImage_view);

  {
    const MotionBlurReleaseInputs inputs = { clEnv, filteredImageBuffer,
      imageBuffer, imageKernelBuffer, motionBlurKernel, queue, device };

    ReleaseMotionBlurResources(&inputs);
  }
  if (outputReady == MagickFalse && filteredImage != NULL)
    filteredImage=DestroyImage(filteredImage);

  return(filteredImage);
}

MagickPrivate Image *AccelerateMotionBlurImage(const Image *image,
  const double* kernel,const size_t width,const OffsetInfo *offset,
  ExceptionInfo *exception)
{
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

  filteredImage=ComputeMotionBlurImage(image,clEnv,kernel,width,offset,
    exception);
  return(filteredImage);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A c c e l e r a t e R o t a t i o n a l B l u r I m a g e               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

static Image* ComputeRotationalBlurImage(const Image *image,MagickCLEnv clEnv,
  const double angle,ExceptionInfo *exception)
{
  cl_command_queue
    queue;

  cl_float2
    blurCenter;

  cl_int
    channel_mask=get32BitChannelValue(image->channel_mask),
    status;

  cl_mem
    cosThetaBuffer,
    filteredImageBuffer,
    imageBuffer,
    sinThetaBuffer;

  cl_kernel
    rotationalBlurKernel;

  cl_uint
    cossin_theta_size,
    number_channels;

  float
    blurRadius,
    *cosThetaPtr,
    offset,
    *sinThetaPtr,
    theta;

  Image
    *filteredImage;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  size_t
    gsize[2],
    i;

  const AccelerateBufferTargets
    targets = { &device, &queue, &filteredImage, &imageBuffer,
      &filteredImageBuffer };

  TraceAccelerateImage(image);

  queue=NULL;
  filteredImage=NULL;
  imageBuffer=NULL;
  filteredImageBuffer=NULL;
  sinThetaBuffer=NULL;
  cosThetaBuffer=NULL;
  rotationalBlurKernel=NULL;
  outputReady=MagickFalse;

  if (AcquireAccelerateBuffers(clEnv,image,exception,&targets) == MagickFalse)
    goto cleanup;

  blurCenter.x=(cl_float) ((image->columns-1)/2.0);
  blurCenter.y=(cl_float) ((image->rows-1)/2.0);
  blurRadius=(float) hypot(blurCenter.x,blurCenter.y);
  cossin_theta_size=(unsigned int) fabs(4.0*DegreesToRadians(angle)*sqrt(
    (double) blurRadius)+2UL);

  cosThetaPtr=(float *) AcquireQuantumMemory(cossin_theta_size,sizeof(float));
  if (cosThetaPtr == (float *) NULL)
    goto cleanup;
  sinThetaPtr=(float *) AcquireQuantumMemory(cossin_theta_size,sizeof(float));
  if (sinThetaPtr == (float *) NULL)
  {
    cosThetaPtr=(float *) RelinquishMagickMemory(cosThetaPtr);
    goto cleanup;
  }

  theta=(float) (DegreesToRadians(angle)/(double) (cossin_theta_size-1));
  offset=theta*(float) ((cossin_theta_size-1)/2.0);
  for (i=0; i < (size_t) cossin_theta_size; i++)
  {
    cosThetaPtr[i]=(float)cos((double) (theta*i-offset));
    sinThetaPtr[i]=(float)sin((double) (theta*i-offset));
  }

  sinThetaBuffer=CreateOpenCLBuffer(device,CL_MEM_READ_ONLY |
    CL_MEM_COPY_HOST_PTR,cossin_theta_size*sizeof(float),sinThetaPtr);
  sinThetaPtr=(float *) RelinquishMagickMemory(sinThetaPtr);
  cosThetaBuffer=CreateOpenCLBuffer(device,CL_MEM_READ_ONLY |
    CL_MEM_COPY_HOST_PTR,cossin_theta_size*sizeof(float),cosThetaPtr);
  cosThetaPtr=(float *) RelinquishMagickMemory(cosThetaPtr);
  if ((sinThetaBuffer == (cl_mem) NULL) || (cosThetaBuffer == (cl_mem) NULL))
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"CreateOpenCLBuffer failed.",".");
    goto cleanup;
  }

  rotationalBlurKernel=AcquireOpenCLKernel(device,"RotationalBlur");
  if (rotationalBlurKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    goto cleanup;
  }

  number_channels=(cl_uint) image->number_channels;

  i=0;
  status =SetOpenCLKernelArg(rotationalBlurKernel,i++,sizeof(cl_mem),(void *)&imageBuffer);
  status|=SetOpenCLKernelArg(rotationalBlurKernel,i++,sizeof(cl_uint),&number_channels);
  status|=SetOpenCLKernelArg(rotationalBlurKernel,i++,sizeof(cl_int),&channel_mask);
  status|=SetOpenCLKernelArg(rotationalBlurKernel,i++,sizeof(cl_float2), &blurCenter);
  status|=SetOpenCLKernelArg(rotationalBlurKernel,i++,sizeof(cl_mem),(void *)&cosThetaBuffer);
  status|=SetOpenCLKernelArg(rotationalBlurKernel,i++,sizeof(cl_mem),(void *)&sinThetaBuffer);
  status|=SetOpenCLKernelArg(rotationalBlurKernel,i++,sizeof(cl_uint), &cossin_theta_size);
  status|=SetOpenCLKernelArg(rotationalBlurKernel,i++,sizeof(cl_mem),(void *)&filteredImageBuffer);
  if (status != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"clEnv->library->clSetKernelArg failed.",".");
    goto cleanup;
  }

  gsize[0]=image->columns;
  gsize[1]=image->rows;
  outputReady=EnqueueOpenCLKernel(queue,rotationalBlurKernel,2,
    (const size_t *) NULL,gsize,(const size_t *) NULL,image,filteredImage,
    MagickFalse,exception);

cleanup:

  ReleaseOpenCLBuffer(imageBuffer);
  ReleaseOpenCLBuffer(filteredImageBuffer);
  ReleaseOpenCLBuffer(sinThetaBuffer);
  ReleaseOpenCLBuffer(cosThetaBuffer);
  ReleaseKernel(rotationalBlurKernel);
  ReleaseAccelerateQueue(device,queue);
  filteredImage=DestroyUnfinishedImage(outputReady,filteredImage);

  return(filteredImage);
}

MagickPrivate Image* AccelerateRotationalBlurImage(const Image *image,
  const double angle,ExceptionInfo *exception)
{
  Image
    *filteredImage;

  MagickCLEnv
    clEnv;

  assert(image != NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);

  if (checkAccelerateCondition(image) == MagickFalse)
    return((Image *) NULL);

  clEnv=getOpenCLEnvironment(exception);
  if (clEnv == (MagickCLEnv) NULL)
    return((Image *) NULL);

  filteredImage=ComputeRotationalBlurImage(image,clEnv,angle,exception);
  return filteredImage;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A c c e l e r a t e U n s h a r p M a s k I m a g e                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

static Image *ComputeUnsharpMaskImage(const Image *image,MagickCLEnv clEnv,
  const double radius,const double sigma,const double gain,
  const double threshold,ExceptionInfo *exception)
{
  cl_command_queue
    queue;

  cl_int
    channel_mask=get32BitChannelValue(image->channel_mask),
    status;

  cl_kernel
    blurRowKernel,
    unsharpMaskBlurColumnKernel;

  cl_mem
    filteredImageBuffer,
    imageBuffer,
    imageKernelBuffer,
    tempImageBuffer;

  cl_uint
    imageColumns,
    imageRows,
    kernelWidth,
    number_channels;

  float
    fGain,
    fThreshold;

  Image
    *filteredImage;

  int
    chunkSize;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  size_t
    gsize[2],
    i,
    length,
    lsize[2];

  const AccelerateBufferTargets
    targets = { &device, &queue, &filteredImage, &imageBuffer,
      &filteredImageBuffer };

  queue=NULL;
  filteredImage=NULL;
  imageBuffer=NULL;
  filteredImageBuffer=NULL;
  tempImageBuffer=NULL;
  imageKernelBuffer=NULL;
  blurRowKernel=NULL;
  unsharpMaskBlurColumnKernel=NULL;
  outputReady=MagickFalse;

  if (AcquireAccelerateBuffers(clEnv,image,exception,&targets) == MagickFalse)
    goto cleanup;

  imageKernelBuffer=createKernelInfo(device,radius,sigma,&kernelWidth,
    exception);

  length=image->columns*image->rows;
  tempImageBuffer=CreateOpenCLBuffer(device,CL_MEM_READ_WRITE,length*
    sizeof(cl_float4),NULL);
  if (tempImageBuffer == (cl_mem) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"CreateOpenCLBuffer failed.",".");
    goto cleanup;
  }

  blurRowKernel=AcquireOpenCLKernel(device,"BlurRow");
  if (blurRowKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    goto cleanup;
  }

  unsharpMaskBlurColumnKernel=AcquireOpenCLKernel(device,
    "UnsharpMaskBlurColumn");
  if (unsharpMaskBlurColumnKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    goto cleanup;
  }

  number_channels=(cl_uint) image->number_channels;
  imageColumns=(cl_uint) image->columns;
  imageRows=(cl_uint) image->rows;

  chunkSize = 256;

  i=0;
  status =SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_mem),(void *)&imageBuffer);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_uint),&number_channels);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_int),&channel_mask);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_mem),(void *)&imageKernelBuffer);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_uint),(void *)&kernelWidth);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_uint),(void *)&imageColumns);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_uint),(void *)&imageRows);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_float4)*(chunkSize+kernelWidth),(void *) NULL);
  status|=SetOpenCLKernelArg(blurRowKernel,i++,sizeof(cl_mem),(void *)&tempImageBuffer);
  if (status != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"clEnv->library->clSetKernelArg failed.",".");
    goto cleanup;
  }

  gsize[0]=chunkSize*((image->columns+chunkSize-1)/chunkSize);
  gsize[1]=image->rows;
  lsize[0]=chunkSize;
  lsize[1]=1;
  outputReady=EnqueueOpenCLKernel(queue,blurRowKernel,2,
    (const size_t *) NULL,gsize,lsize,image,filteredImage,MagickFalse,
    exception);

  chunkSize=256;
  fGain=(float) gain;
  fThreshold=(float) threshold;

  i=0;
  status =SetOpenCLKernelArg(unsharpMaskBlurColumnKernel,i++,sizeof(cl_mem),(void *)&imageBuffer);
  status|=SetOpenCLKernelArg(unsharpMaskBlurColumnKernel,i++,sizeof(cl_mem),(void *)&tempImageBuffer);
  status|=SetOpenCLKernelArg(unsharpMaskBlurColumnKernel,i++,sizeof(cl_uint),&number_channels);
  status|=SetOpenCLKernelArg(unsharpMaskBlurColumnKernel,i++,sizeof(cl_int),&channel_mask);
  status|=SetOpenCLKernelArg(unsharpMaskBlurColumnKernel,i++,sizeof(cl_uint),(void *)&imageColumns);
  status|=SetOpenCLKernelArg(unsharpMaskBlurColumnKernel,i++,sizeof(cl_uint),(void *)&imageRows);
  status|=SetOpenCLKernelArg(unsharpMaskBlurColumnKernel,i++,(chunkSize+kernelWidth-1)*sizeof(cl_float4),NULL);
  status|=SetOpenCLKernelArg(unsharpMaskBlurColumnKernel,i++,kernelWidth*sizeof(float),NULL);
  status|=SetOpenCLKernelArg(unsharpMaskBlurColumnKernel,i++,sizeof(cl_mem),(void *)&imageKernelBuffer);
  status|=SetOpenCLKernelArg(unsharpMaskBlurColumnKernel,i++,sizeof(cl_uint),(void *)&kernelWidth);
  status|=SetOpenCLKernelArg(unsharpMaskBlurColumnKernel,i++,sizeof(float),(void *)&fGain);
  status|=SetOpenCLKernelArg(unsharpMaskBlurColumnKernel,i++,sizeof(float),(void *)&fThreshold);
  status|=SetOpenCLKernelArg(unsharpMaskBlurColumnKernel,i++,sizeof(cl_mem),(void *)&filteredImageBuffer);
  if (status != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"clEnv->library->clSetKernelArg failed.",".");
    goto cleanup;
  }

  gsize[0]=image->columns;
  gsize[1]=chunkSize*((image->rows+chunkSize-1)/chunkSize);
  lsize[0]=1;
  lsize[1]=chunkSize;
  outputReady=EnqueueOpenCLKernel(queue,unsharpMaskBlurColumnKernel,2,
    (const size_t *) NULL,gsize,lsize,image,filteredImage,MagickFalse,
    exception);

cleanup:

  ReleaseOpenCLBuffer(imageBuffer);
  ReleaseOpenCLBuffer(filteredImageBuffer);
  ReleaseOpenCLBuffer(tempImageBuffer);
  ReleaseOpenCLBuffer(imageKernelBuffer);
  ReleaseKernel(blurRowKernel);
  ReleaseKernel(unsharpMaskBlurColumnKernel);
  ReleaseAccelerateQueue(device,queue);
  filteredImage=DestroyUnfinishedImage(outputReady,filteredImage);

  return(filteredImage);
}

static Image *ComputeUnsharpMaskImageSingle(const Image *image,
  MagickCLEnv clEnv,const double radius,const double sigma,const double gain,
  const double threshold,ExceptionInfo *exception)
{
  cl_command_queue
    queue;

  cl_int
    channel_mask=get32BitChannelValue(image->channel_mask),
    status;

  cl_kernel
    unsharpMaskKernel;

  cl_mem
    filteredImageBuffer,
    imageBuffer,
    imageKernelBuffer;

  cl_uint
    imageColumns,
    imageRows,
    kernelWidth,
    number_channels;

  float
    fGain,
    fThreshold;

  Image
    *filteredImage;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  size_t
    gsize[2],
    i,
    lsize[2];

  const AccelerateBufferTargets
    targets = { &device, &queue, &filteredImage, &imageBuffer,
      &filteredImageBuffer };

  queue=NULL;
  filteredImage=NULL;
  imageBuffer=NULL;
  filteredImageBuffer=NULL;
  imageKernelBuffer=NULL;
  unsharpMaskKernel=NULL;
  outputReady=MagickFalse;

  if (AcquireAccelerateBuffers(clEnv,image,exception,&targets) == MagickFalse)
    goto cleanup;

  imageKernelBuffer=createKernelInfo(device,radius,sigma,&kernelWidth,
    exception);

  unsharpMaskKernel=AcquireOpenCLKernel(device,"UnsharpMask");
  if (unsharpMaskKernel == NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    goto cleanup;
  }

  imageColumns=(cl_uint) image->columns;
  imageRows=(cl_uint) image->rows;
  number_channels=(cl_uint) image->number_channels;
  fGain=(float) gain;
  fThreshold=(float) threshold;

  i=0;
  status =SetOpenCLKernelArg(unsharpMaskKernel,i++,sizeof(cl_mem),(void *)&imageBuffer);
  status|=SetOpenCLKernelArg(unsharpMaskKernel,i++,sizeof(cl_uint),(void *)&number_channels);
  status|=SetOpenCLKernelArg(unsharpMaskKernel,i++,sizeof(cl_int),&channel_mask);
  status|=SetOpenCLKernelArg(unsharpMaskKernel,i++,sizeof(cl_mem),(void *)&imageKernelBuffer);
  status|=SetOpenCLKernelArg(unsharpMaskKernel,i++,sizeof(cl_uint),(void *)&kernelWidth);
  status|=SetOpenCLKernelArg(unsharpMaskKernel,i++,sizeof(cl_uint),(void *)&imageColumns);
  status|=SetOpenCLKernelArg(unsharpMaskKernel,i++,sizeof(cl_uint),(void *)&imageRows);
  status|=SetOpenCLKernelArg(unsharpMaskKernel,i++,sizeof(cl_float4)*(8 * (32 + kernelWidth)),(void *) NULL);
  status|=SetOpenCLKernelArg(unsharpMaskKernel,i++,sizeof(float),(void *)&fGain);
  status|=SetOpenCLKernelArg(unsharpMaskKernel,i++,sizeof(float),(void *)&fThreshold);
  status|=SetOpenCLKernelArg(unsharpMaskKernel,i++,sizeof(cl_mem),(void *)&filteredImageBuffer);
  if (status != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"SetOpenCLKernelArg failed.",".");
    goto cleanup;
  }

  gsize[0]=((image->columns + 7) / 8)*8;
  gsize[1]=((image->rows + 31) / 32)*32;
  lsize[0]=8;
  lsize[1]=32;
  outputReady=EnqueueOpenCLKernel(queue,unsharpMaskKernel,2,(const size_t *) NULL,
    gsize,lsize,image,filteredImage,MagickFalse,exception);

cleanup:

  ReleaseOpenCLBuffer(imageBuffer);
  ReleaseOpenCLBuffer(filteredImageBuffer);
  ReleaseOpenCLBuffer(imageKernelBuffer);
  ReleaseKernel(unsharpMaskKernel);
  ReleaseAccelerateQueue(device,queue);
  if ((outputReady == MagickFalse) && (filteredImage != (Image *) NULL))
    filteredImage=DestroyImage(filteredImage);

  return(filteredImage);
}

MagickPrivate Image *AccelerateUnsharpMaskImage(const Image *image,
  const double radius,const double sigma,const double gain,
  const double threshold,ExceptionInfo *exception)
{
  Image
    *filteredImage;

  MagickCLEnv
    clEnv;

  assert(image != NULL);
  assert(exception != (ExceptionInfo *) NULL);

  if (checkAccelerateCondition(image) == MagickFalse)
    return((Image *) NULL);

  clEnv=getOpenCLEnvironment(exception);
  if (clEnv == (MagickCLEnv) NULL)
    return((Image *) NULL);

  if (radius < 12.1)
    filteredImage=ComputeUnsharpMaskImageSingle(image,clEnv,radius,sigma,gain,
      threshold,exception);
  else
    filteredImage=ComputeUnsharpMaskImage(image,clEnv,radius,sigma,gain,
      threshold,exception);
  return(filteredImage);
}

#endif /* MAGICKCORE_OPENCL_SUPPORT */
