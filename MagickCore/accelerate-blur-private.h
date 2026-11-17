/*
  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.  You may
  obtain a copy of the License at

    https://imagemagick.org/license/

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore private methods shared by the OpenCL accelerated blur methods:
  the channel-mask test, the image clone, the blur kernel weights and the
  device, queue and buffer acquisition of Blur, RotationalBlur and UnsharpMask.
*/
#ifndef MAGICKCORE_ACCELERATE_BLUR_PRIVATE_H
#define MAGICKCORE_ACCELERATE_BLUR_PRIVATE_H

#include "MagickCore/accelerate-core-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(MAGICKCORE_OPENCL_SUPPORT)

static inline MagickBooleanType IsRGBAChannelMask(const Image *image)
{
  return((((image->channel_mask & RedChannel) != 0) &&
      ((image->channel_mask & GreenChannel) != 0) &&
      ((image->channel_mask & BlueChannel) != 0) &&
      ((image->channel_mask & AlphaChannel) != 0)) ? MagickTrue : MagickFalse);
}

static inline Image *cloneImage(const Image* image,ExceptionInfo *exception)
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

static inline cl_mem createKernelInfo(MagickCLDevice device,const double radius,
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

static inline MagickBooleanType AcquireAccelerateBuffers(MagickCLEnv clEnv,
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

#endif /* MAGICKCORE_OPENCL_SUPPORT */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
