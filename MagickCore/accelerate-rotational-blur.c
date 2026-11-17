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
%    RRRR    OOO   TTTTT   AAA   TTTTT  IIIII   OOO   N   N   AAA   L         %
%    R   R  O   O    T    A   A    T      I    O   O  NN  N  A   A  L         %
%    RRRR   O   O    T    AAAAA    T      I    O   O  N N N  AAAAA  L         %
%    R R    O   O    T    A   A    T      I    O   O  N  NN  A   A  L         %
%    R  R    OOO     T    A   A    T    IIIII   OOO   N   N  A   A  LLLLL     %
%                                                                             %
%                         BBBB   L      U   U  RRRR                           %
%                         B   B  L      U   U  R   R                          %
%                         BBBB   L      U   U  RRRR                           %
%                         B   B  L      U   U  R R                            %
%                         BBBB   LLLLL   UUU   R  R                           %
%                                                                             %
%                                                                             %
%            MagickCore OpenCL Accelerated Rotational Blur Method             %
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
%     A c c e l e r a t e R o t a t i o n a l B l u r I m a g e               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

typedef struct _RotationalBlurTableInputs
{
  MagickCLDevice
    device;

  double
    angle;

  cl_uint
    cossin_theta_size;
} RotationalBlurTableInputs;

typedef struct _RotationalBlurTableTargets
{
  cl_mem
    *sinThetaBuffer,
    *cosThetaBuffer;
} RotationalBlurTableTargets;

static MagickBooleanType CreateRotationalBlurTables(
  const RotationalBlurTableInputs *inputs,ExceptionInfo *exception,
  const RotationalBlurTableTargets *tables)
{
  const MagickCLDevice
    device = inputs->device;

  const double
    angle = inputs->angle;

  const cl_uint
    cossin_theta_size = inputs->cossin_theta_size;

  cl_mem
    *const sinThetaBuffer = tables->sinThetaBuffer,
    *const cosThetaBuffer = tables->cosThetaBuffer;

  float
    *cosThetaPtr,
    offset,
    *sinThetaPtr,
    theta;

  size_t
    i;

  cosThetaPtr=(float *) AcquireQuantumMemory(cossin_theta_size,sizeof(float));
  if (cosThetaPtr == (float *) NULL)
    return(MagickFalse);
  sinThetaPtr=(float *) AcquireQuantumMemory(cossin_theta_size,sizeof(float));
  if (sinThetaPtr == (float *) NULL)
  {
    cosThetaPtr=(float *) RelinquishMagickMemory(cosThetaPtr);
    return(MagickFalse);
  }

  theta=(float) (DegreesToRadians(angle)/(double) (cossin_theta_size-1));
  offset=theta*(float) ((cossin_theta_size-1)/2.0);
  for (i=0; i < (size_t) cossin_theta_size; i++)
  {
    cosThetaPtr[i]=(float)cos((double) (theta*i-offset));
    sinThetaPtr[i]=(float)sin((double) (theta*i-offset));
  }

  *sinThetaBuffer=CreateOpenCLBuffer(device,CL_MEM_READ_ONLY |
    CL_MEM_COPY_HOST_PTR,cossin_theta_size*sizeof(float),sinThetaPtr);
  sinThetaPtr=(float *) RelinquishMagickMemory(sinThetaPtr);
  *cosThetaBuffer=CreateOpenCLBuffer(device,CL_MEM_READ_ONLY |
    CL_MEM_COPY_HOST_PTR,cossin_theta_size*sizeof(float),cosThetaPtr);
  cosThetaPtr=(float *) RelinquishMagickMemory(cosThetaPtr);
  if ((*sinThetaBuffer == (cl_mem) NULL) || (*cosThetaBuffer == (cl_mem) NULL))
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"CreateOpenCLBuffer failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

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
    blurRadius;

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

  {
    const RotationalBlurTableInputs inputs = { device, angle,
      cossin_theta_size };
    const RotationalBlurTableTargets tables = { &sinThetaBuffer,
      &cosThetaBuffer };

    if (CreateRotationalBlurTables(&inputs,exception,&tables) == MagickFalse)
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
#endif /* MAGICKCORE_OPENCL_SUPPORT */
