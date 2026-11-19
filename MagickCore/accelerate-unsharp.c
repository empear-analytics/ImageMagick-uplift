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
%               U   U  N   N  SSSSS  H   H   AAA   RRRR   PPPP                %
%               U   U  NN  N  SS     H   H  A   A  R   R  P   P               %
%               U   U  N N N   SSS   HHHHH  AAAAA  RRRR   PPPP                %
%               U   U  N  NN     SS  H   H  A   A  R R    P                   %
%                UUU   N   N  SSSSS  H   H  A   A  R  R   P                   %
%                                                                             %
%                                                                             %
%              MagickCore OpenCL Accelerated Unsharp Mask Method              %
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
%     A c c e l e r a t e U n s h a r p M a s k I m a g e                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

typedef struct _UnsharpColumnArgs
{
  cl_kernel
    unsharpMaskBlurColumnKernel;

  cl_mem
    imageBuffer;

  cl_mem
    tempImageBuffer;

  cl_uint
    number_channels;

  cl_int
    channel_mask;

  cl_uint
    imageColumns;

  cl_uint
    imageRows;

  int
    chunkSize;

  cl_uint
    kernelWidth;

  cl_mem
    imageKernelBuffer;

  float
    fGain;

  float
    fThreshold;

  cl_mem
    filteredImageBuffer;
} UnsharpColumnArgs;

static cl_int SetUnsharpColumnArgs(const UnsharpColumnArgs *args)
{
  const cl_kernel
    unsharpMaskBlurColumnKernel = args->unsharpMaskBlurColumnKernel;

  const cl_mem
    imageBuffer = args->imageBuffer;

  const cl_mem
    tempImageBuffer = args->tempImageBuffer;

  const cl_uint
    number_channels = args->number_channels;

  const cl_int
    channel_mask = args->channel_mask;

  const cl_uint
    imageColumns = args->imageColumns;

  const cl_uint
    imageRows = args->imageRows;

  const int
    chunkSize = args->chunkSize;

  const cl_uint
    kernelWidth = args->kernelWidth;

  const cl_mem
    imageKernelBuffer = args->imageKernelBuffer;

  const float
    fGain = args->fGain;

  const float
    fThreshold = args->fThreshold;

  const cl_mem
    filteredImageBuffer = args->filteredImageBuffer;

  cl_int
    status;

  size_t
    i;

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
  return(status);
}

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

  const BlurKernelGeometry geometry = { radius, sigma };

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

  imageKernelBuffer=createKernelInfo(device,&geometry,&kernelWidth,exception);

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

  {
    const UnsharpColumnArgs args = { unsharpMaskBlurColumnKernel, imageBuffer,
      tempImageBuffer, number_channels, channel_mask, imageColumns, imageRows,
      chunkSize, kernelWidth, imageKernelBuffer, fGain, fThreshold,
      filteredImageBuffer };

    status=SetUnsharpColumnArgs(&args);
  }
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

  const BlurKernelGeometry geometry = { radius, sigma };

  queue=NULL;
  filteredImage=NULL;
  imageBuffer=NULL;
  filteredImageBuffer=NULL;
  imageKernelBuffer=NULL;
  unsharpMaskKernel=NULL;
  outputReady=MagickFalse;

  if (AcquireAccelerateBuffers(clEnv,image,exception,&targets) == MagickFalse)
    goto cleanup;

  imageKernelBuffer=createKernelInfo(device,&geometry,&kernelWidth,exception);

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
