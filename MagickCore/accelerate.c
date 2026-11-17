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


#endif /* MAGICKCORE_OPENCL_SUPPORT */
