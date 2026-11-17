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
