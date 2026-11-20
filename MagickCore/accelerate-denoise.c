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
%               DDDD   EEEEE  N   N   OOO   IIIII  SSSSS  EEEEE               %
%               D   D  E      NN  N  O   O    I    SS     E                   %
%               D   D  EEE    N N N  O   O    I     SSS   EEE                 %
%               D   D  E      N  NN  O   O    I       SS  E                   %
%               DDDD   EEEEE  N   N   OOO   IIIII  SSSSS  EEEEE               %
%                                                                             %
%                                                                             %
%            MagickCore OpenCL Accelerated Noise Reduction Methods            %
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


typedef struct _WaveletDenoiseTargets
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
} WaveletDenoiseTargets;

static MagickBooleanType AcquireWaveletDenoiseBuffers(MagickCLEnv clEnv,
  const Image *image,ExceptionInfo *exception,
  const WaveletDenoiseTargets *targets)
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
  /* Work around an issue on low end Intel devices */
  if (strcmp("Intel(R) HD Graphics",(*device)->name) == 0)
    return(MagickFalse);
  *queue=AcquireOpenCLCommandQueue(*device);
  if (*queue == (cl_command_queue) NULL)
    return(MagickFalse);
  *filteredImage=CloneImage(image,0,0,MagickTrue,
    exception);
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

typedef struct _WaveletDenoisePassInputs
{
  cl_command_queue
    queue;

  cl_kernel
    denoiseKernel;

  const Image
    *image;

  Image
    *filteredImage;

  size_t
    passes;

  cl_uint
    width,
    height;

  int
    SIZE,
    TILESIZE;

  MagickBooleanType
    outputReady;
} WaveletDenoisePassInputs;

static MagickBooleanType EnqueueWaveletDenoisePasses(
  const WaveletDenoisePassInputs *pass,ExceptionInfo *exception)
{
  const size_t
    passes = pass->passes;

  const cl_uint
    width = pass->width,
    height = pass->height;

  const int
    SIZE = pass->SIZE,
    TILESIZE = pass->TILESIZE;

  MagickBooleanType
    outputReady = pass->outputReady;

  size_t
    goffset[2],
    gsize[2],
    lsize[2],
    x;

  for (x = 0; x < passes; ++x)
  {
    gsize[0]=((width+(SIZE-1))/SIZE)*TILESIZE;
    gsize[1]=((((height+(SIZE-1))/SIZE)+passes-1)/passes)*4;
    lsize[0]=TILESIZE;
    lsize[1]=4;
    goffset[0]=0;
    goffset[1]=x*gsize[1];

    {
      const OpenCLKernelLaunch
        enqueue = { pass->queue, pass->denoiseKernel, 2, goffset, gsize, lsize,
          pass->image, pass->filteredImage, MagickTrue };

      outputReady=EnqueueOpenCLKernel(&enqueue,exception);
    }
    if (outputReady == MagickFalse)
      break;
  }
  return(outputReady);
}

typedef struct _WaveletDenoiseLaunchInputs
{
  MagickCLDevice
    device;

  cl_command_queue
    queue;

  const Image
    *image;

  Image
    *filteredImage;

  cl_mem
    imageBuffer,
    filteredImageBuffer;

  double
    threshold;
} WaveletDenoiseLaunchInputs;

typedef struct _WaveletDenoiseLaunchTargets
{
  cl_kernel
    *denoiseKernel;

  MagickBooleanType
    *outputReady;
} WaveletDenoiseLaunchTargets;

static void LaunchWaveletDenoiseKernel(const WaveletDenoiseLaunchInputs *inputs,
  ExceptionInfo *exception,const WaveletDenoiseLaunchTargets *launch)
{
  const cl_int
    PASSES=5;

  const int
    TILESIZE=64,
    PAD=1<<(PASSES-1),
    SIZE=TILESIZE-2*PAD;

  cl_float
    thresh;

  cl_int
    status;

  cl_uint
    number_channels,
    width,
    height,
    max_channels;

  size_t
    i,
    passes;

  *launch->denoiseKernel=AcquireOpenCLKernel(inputs->device,"WaveletDenoise");
  if (*launch->denoiseKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(inputs->device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    return;
  }

  number_channels=(cl_uint)inputs->image->number_channels;
  width=(cl_uint)inputs->image->columns;
  height=(cl_uint)inputs->image->rows;
  max_channels=number_channels;
  if ((max_channels == 4) || (max_channels == 2))
    max_channels=max_channels-1;
  thresh=(cl_float) inputs->threshold;
  passes=(size_t) ((((1.0f*inputs->image->columns)*inputs->image->rows)+1999999.0f)/2000000.0f);
  passes=(passes < 1) ? 1 : passes;

  i=0;
  status =SetOpenCLKernelArg(*launch->denoiseKernel,i++,sizeof(cl_mem),(void *)&inputs->imageBuffer);
  status|=SetOpenCLKernelArg(*launch->denoiseKernel,i++,sizeof(cl_mem),(void *)&inputs->filteredImageBuffer);
  status|=SetOpenCLKernelArg(*launch->denoiseKernel,i++,sizeof(cl_uint),(void *)&number_channels);
  status|=SetOpenCLKernelArg(*launch->denoiseKernel,i++,sizeof(cl_uint),(void *)&max_channels);
  status|=SetOpenCLKernelArg(*launch->denoiseKernel,i++,sizeof(cl_float),(void *)&thresh);
  status|=SetOpenCLKernelArg(*launch->denoiseKernel,i++,sizeof(cl_int),(void *)&PASSES);
  status|=SetOpenCLKernelArg(*launch->denoiseKernel,i++,sizeof(cl_uint),(void *)&width);
  status|=SetOpenCLKernelArg(*launch->denoiseKernel,i++,sizeof(cl_uint),(void *)&height);
  if (status != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(inputs->device,exception,GetMagickModule(),
        ResourceLimitWarning,"SetOpenCLKernelArg failed.",".");
      return;
    }

  {
    const WaveletDenoisePassInputs pass = { inputs->queue,
      *launch->denoiseKernel, inputs->image, inputs->filteredImage, passes,
      width, height, SIZE, TILESIZE, *launch->outputReady };

    *launch->outputReady=EnqueueWaveletDenoisePasses(&pass,exception);
  }
}

static Image *ComputeWaveletDenoiseImage(const Image *image,MagickCLEnv clEnv,
  const double threshold,ExceptionInfo *exception)
{
  cl_command_queue
    queue;

  cl_kernel
    denoiseKernel;

  cl_mem
    filteredImageBuffer,
    imageBuffer;

  Image
    *filteredImage;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  filteredImage=NULL;
  imageBuffer=NULL;
  filteredImageBuffer=NULL;
  denoiseKernel=NULL;
  queue=NULL;
  outputReady=MagickFalse;

  {
    const WaveletDenoiseTargets targets = { &device, &queue, &filteredImage,
      &imageBuffer, &filteredImageBuffer };

    if (AcquireWaveletDenoiseBuffers(clEnv,image,exception,&targets) ==
        MagickFalse)
      goto cleanup;
  }

  {
    const WaveletDenoiseLaunchInputs inputs = { device, queue, image,
      filteredImage, imageBuffer, filteredImageBuffer, threshold };
    const WaveletDenoiseLaunchTargets launch = { &denoiseKernel,
      &outputReady };

    LaunchWaveletDenoiseKernel(&inputs,exception,&launch);
  }

cleanup:

  ReleaseOpenCLBuffer(imageBuffer);
  ReleaseOpenCLBuffer(filteredImageBuffer);
  ReleaseKernel(denoiseKernel);
  ReleaseAccelerateQueue(device,queue);
  filteredImage=DestroyUnfinishedImage(outputReady,filteredImage);

  return(filteredImage);
}

MagickPrivate Image *AccelerateWaveletDenoiseImage(const Image *image,
  const double threshold,ExceptionInfo *exception)
{
  Image
    *filteredImage;

  MagickCLEnv
    clEnv;

  assert(image != NULL);
  assert(exception != (ExceptionInfo *)NULL);

  if (checkAccelerateCondition(image) == MagickFalse)
    return((Image *) NULL);

  clEnv=getOpenCLEnvironment(exception);
  if (clEnv == (MagickCLEnv) NULL)
    return((Image *) NULL);

  filteredImage=ComputeWaveletDenoiseImage(image,clEnv,threshold,exception);

  return(filteredImage);
}
#endif /* MAGICKCORE_OPENCL_SUPPORT */
