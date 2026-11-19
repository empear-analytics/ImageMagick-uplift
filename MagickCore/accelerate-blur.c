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
%                         BBBB   L      U   U  RRRR                           %
%                         B   B  L      U   U  R   R                          %
%                         BBBB   L      U   U  RRRR                           %
%                         B   B  L      U   U  R R                            %
%                         BBBB   LLLLL   UUU   R  R                           %
%                                                                             %
%                                                                             %
%                 MagickCore OpenCL Accelerated Blur Methods                  %
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
#include "MagickCore/accelerate-blur-private.h"
#if defined(MAGICKCORE_OPENCL_SUPPORT)


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

typedef struct _BlurKernelArgs
{
  MagickCLDevice
    device;

  cl_kernel
    blurRowKernel;

  cl_mem
    imageBuffer;

  cl_uint
    number_channels;

  cl_int
    channel_mask;

  cl_mem
    imageKernelBuffer;

  cl_uint
    kernelWidth,
    imageColumns,
    imageRows;

  size_t
    chunkSize;

  cl_mem
    tempImageBuffer;
} BlurKernelArgs;

static MagickBooleanType SetBlurKernelArgs(const BlurKernelArgs *args,
  ExceptionInfo *exception)
{
  const MagickCLDevice
    device = args->device;

  const cl_kernel
    blurRowKernel = args->blurRowKernel;

  const cl_mem
    imageBuffer = args->imageBuffer;

  const cl_uint
    number_channels = args->number_channels;

  const cl_int
    channel_mask = args->channel_mask;

  const cl_mem
    imageKernelBuffer = args->imageKernelBuffer;

  const cl_uint
    kernelWidth = args->kernelWidth,
    imageColumns = args->imageColumns,
    imageRows = args->imageRows;

  const size_t
    chunkSize = args->chunkSize;

  const cl_mem
    tempImageBuffer = args->tempImageBuffer;

  cl_int
    status;

  size_t
    i;

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
    return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _BlurImageRequest
{
  const Image
    *image;

  double
    radius,
    sigma;
} BlurImageRequest;

typedef struct _BlurPassesInput
{
  MagickCLDevice
    device;

  const Image
    *image;

  cl_mem
    imageBuffer;

  cl_int
    channel_mask;

  cl_mem
    imageKernelBuffer;

  cl_uint
    kernelWidth;

  size_t
    chunkSize;

  cl_mem
    tempImageBuffer;

  cl_command_queue
    queue;

  Image
    *filteredImage;

  cl_mem
    filteredImageBuffer;
} BlurPassesInput;

typedef struct _BlurPassesTargets
{
  cl_kernel
    *blurRowKernel,
    *blurColumnKernel;

  MagickBooleanType
    *outputReady;
} BlurPassesTargets;

static MagickBooleanType EnqueueBlurPasses(const BlurPassesInput *input,
  ExceptionInfo *exception,const BlurPassesTargets *targets)
{
  cl_kernel
    *const blurRowKernel = targets->blurRowKernel,
    *const blurColumnKernel = targets->blurColumnKernel;

  MagickBooleanType
    *const outputReady = targets->outputReady;

  cl_uint
    imageColumns,
    imageRows,
    number_channels;

  size_t
    gsize[2],
    lsize[2];

  *blurRowKernel=AcquireOpenCLKernel(input->device,"BlurRow");
  if (*blurRowKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(input->device,exception,
      GetMagickModule(),ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    return(MagickFalse);
  }

  number_channels=(cl_uint) input->image->number_channels;
  imageColumns=(cl_uint) input->image->columns;
  imageRows=(cl_uint) input->image->rows;

  {
    const BlurKernelArgs
      args = { input->device, *blurRowKernel, input->imageBuffer,
        number_channels, input->channel_mask, input->imageKernelBuffer,
        input->kernelWidth, imageColumns, imageRows, input->chunkSize,
        input->tempImageBuffer };

    if (SetBlurKernelArgs(&args,exception) == MagickFalse)
      return(MagickFalse);
  }

  gsize[0]=input->chunkSize*((input->image->columns+input->chunkSize-1)/
    input->chunkSize);
  gsize[1]=input->image->rows;
  lsize[0]=input->chunkSize;
  lsize[1]=1;

  *outputReady=EnqueueOpenCLKernel(input->queue,*blurRowKernel,2,
    (size_t *) NULL,gsize,lsize,input->image,input->filteredImage,MagickFalse,
    exception);
  if (*outputReady == MagickFalse)
    return(MagickFalse);

  *blurColumnKernel=AcquireOpenCLKernel(input->device,"BlurColumn");
  if (*blurColumnKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(input->device,exception,
      GetMagickModule(),ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    return(MagickFalse);
  }

  {
    const BlurKernelArgs
      args = { input->device, *blurColumnKernel, input->tempImageBuffer,
        number_channels, input->channel_mask, input->imageKernelBuffer,
        input->kernelWidth, imageColumns, imageRows, input->chunkSize,
        input->filteredImageBuffer };

    if (SetBlurKernelArgs(&args,exception) == MagickFalse)
      return(MagickFalse);
  }

  gsize[0]=input->image->columns;
  gsize[1]=input->chunkSize*((input->image->rows+input->chunkSize-1)/
    input->chunkSize);
  lsize[0]=1;
  lsize[1]=input->chunkSize;

  *outputReady=EnqueueOpenCLKernel(input->queue,*blurColumnKernel,2,
    (size_t *) NULL,gsize,lsize,input->image,input->filteredImage,MagickFalse,
    exception);
  return(MagickTrue);
}

typedef struct _BlurKernelsSource
{
  MagickCLDevice
    device;

  double
    radius,
    sigma;

  const Image
    *image;

  cl_mem
    imageBuffer;

  cl_int
    channel_mask;

  size_t
    chunkSize;

  cl_command_queue
    queue;

  Image
    *filteredImage;

  cl_mem
    filteredImageBuffer;
} BlurKernelsSource;

typedef struct _BlurKernelsTargets
{
  cl_mem
    *imageKernelBuffer,
    *tempImageBuffer;

  cl_kernel
    *blurRowKernel,
    *blurColumnKernel;

  MagickBooleanType
    *outputReady;
} BlurKernelsTargets;

static MagickBooleanType RunBlurKernels(const BlurKernelsSource *source,
  ExceptionInfo *exception,const BlurKernelsTargets *targets)
{
  const MagickCLDevice
    device = source->device;

  const double
    radius = source->radius,
    sigma = source->sigma;

  const Image
    *const image = source->image;

  const cl_mem
    imageBuffer = source->imageBuffer;

  const cl_int
    channel_mask = source->channel_mask;

  const size_t
    chunkSize = source->chunkSize;

  const cl_command_queue
    queue = source->queue;

  Image
    *const filteredImage = source->filteredImage;

  const cl_mem
    filteredImageBuffer = source->filteredImageBuffer;

  cl_mem
    *const imageKernelBuffer = targets->imageKernelBuffer,
    *const tempImageBuffer = targets->tempImageBuffer;

  cl_kernel
    *const blurRowKernel = targets->blurRowKernel,
    *const blurColumnKernel = targets->blurColumnKernel;

  MagickBooleanType
    *const outputReady = targets->outputReady;

  cl_uint
    kernelWidth;

  size_t
    length;

  *imageKernelBuffer=createKernelInfo(device,radius,sigma,&kernelWidth,
    exception);
  if (*imageKernelBuffer == (cl_mem) NULL)
    return(MagickFalse);

  length=image->columns*image->rows;
  *tempImageBuffer=CreateOpenCLBuffer(device,CL_MEM_READ_WRITE,length*
    sizeof(cl_float4),(void *) NULL);
  if (*tempImageBuffer == (cl_mem) NULL)
    return(MagickFalse);

  {
    const BlurPassesInput
      input = { device, image, imageBuffer, channel_mask, *imageKernelBuffer,
        kernelWidth, chunkSize, *tempImageBuffer, queue, filteredImage,
        filteredImageBuffer };

    const BlurPassesTargets
      passTargets = { blurRowKernel, blurColumnKernel, outputReady };

    if (EnqueueBlurPasses(&input,exception,&passTargets) == MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

static Image *ComputeBlurImage(const BlurImageRequest *request,
  MagickCLEnv clEnv,ExceptionInfo *exception)
{
  const Image
    *const image = request->image;

  const double
    radius = request->radius,
    sigma = request->sigma;

  cl_command_queue
    queue;

  cl_int
    channel_mask=get32BitChannelValue(image->channel_mask);

  cl_kernel
    blurColumnKernel,
    blurRowKernel;

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

  size_t
    chunkSize=256;

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

  {
    const BlurKernelsSource
      source = { device, radius, sigma, image, imageBuffer, channel_mask,
        chunkSize, queue, filteredImage, filteredImageBuffer };

    const BlurKernelsTargets
      kernelTargets = { &imageKernelBuffer, &tempImageBuffer, &blurRowKernel,
        &blurColumnKernel, &outputReady };

    if (RunBlurKernels(&source,exception,&kernelTargets) == MagickFalse)
      goto cleanup;
  }

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
  const BlurImageRequest
    request = { image, radius, sigma };

  return(ComputeBlurImage(&request,getAccelerateEnvironment(image,
    BasicAccelerateCheck,__func__,exception),exception));
}


#endif /* MAGICKCORE_OPENCL_SUPPORT */
