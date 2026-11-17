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

static void DestroyAccelerateViews(CacheView *image_view,
  CacheView *filteredImage_view)
{
  image_view=DestroyCacheView(image_view);
  if (filteredImage_view != NULL)
    filteredImage_view=DestroyCacheView(filteredImage_view);
}

static Image *ComputeDespeckleImage(const Image *image,MagickCLEnv clEnv,
  ExceptionInfo*exception)
{
  static const int
    X[4] = {0, 1, 1,-1},
    Y[4] = {1, 0, 1, 1};

  CacheView
    *filteredImage_view,
    *image_view;

  cl_command_queue
    queue;

  cl_int
    clStatus;

  cl_kernel
    hullPass1,
    hullPass2;

  cl_mem_flags
    mem_flags;

  cl_mem
    filteredImageBuffer,
    imageBuffer,
    tempImageBuffer[2];

  const void
    *inputPixels;

  Image
    *filteredImage;

  int
    k,
    matte;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  size_t
    global_work_size[2],
    length;

  unsigned int
    imageHeight,
    imageWidth;

  void
    *filteredPixels,
    *hostPtr;

  if (clEnv == (MagickCLEnv) NULL)
    return((Image *) NULL);

  queue=NULL;
  image_view=NULL;
  inputPixels=NULL;
  filteredImage=NULL;
  filteredImage_view=NULL;
  filteredPixels=NULL;
  imageBuffer=NULL;
  filteredImageBuffer=NULL;
  hullPass1=NULL;
  hullPass2=NULL;
  tempImageBuffer[0]=NULL;
  tempImageBuffer[1]=NULL;
  outputReady=MagickFalse;

  device=RequestOpenCLDevice(clEnv);
  if (device == (MagickCLDevice) NULL)
    goto cleanup;
  queue=AcquireOpenCLCommandQueue(device);
  if (queue == (cl_command_queue) NULL)
    goto cleanup;

  image_view=AcquireAuthenticCacheView(image,exception);
  inputPixels=GetCacheViewAuthenticPixels(image_view,0,0,image->columns,image->rows,exception);
  if (inputPixels == (void *) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),CacheWarning,"UnableToReadPixelCache.","`%s'",image->filename);
    goto cleanup;
  }

  mem_flags=GetInputBufferFlags(inputPixels);
  /* create a CL buffer from image pixel buffer */
  length = image->columns * image->rows;
  imageBuffer = clEnv->library->clCreateBuffer(device->context, mem_flags, length * sizeof(CLPixelPacket), (void*)inputPixels, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    goto cleanup;
  }

  mem_flags = CL_MEM_READ_WRITE;
  length = image->columns * image->rows;
  {
    const TempBufferInfo
      info = { clEnv, device, mem_flags, length };

    if (CreateTempImageBuffers(&info,tempImageBuffer,exception) == MagickFalse)
      goto cleanup;
  }

  filteredImage = CloneImage(image,0,0,MagickTrue,exception);
  assert(filteredImage != NULL);
  if (SetImageStorageClass(filteredImage,DirectClass,exception) != MagickTrue)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "CloneImage failed.", ".");
    goto cleanup;
  }
  filteredImage_view=AcquireAuthenticCacheView(filteredImage,exception);
  filteredPixels=GetCacheViewAuthenticPixels(filteredImage_view,0,0,filteredImage->columns,filteredImage->rows,exception);
  if (filteredPixels == (void *) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),CacheWarning, "UnableToReadPixelCache.","`%s'",filteredImage->filename);
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
  /* create a CL buffer from image pixel buffer */
  length = image->columns * image->rows;
  filteredImageBuffer = clEnv->library->clCreateBuffer(device->context, mem_flags, length * sizeof(CLPixelPacket), hostPtr, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    goto cleanup;
  }

  hullPass1 = AcquireOpenCLKernel(device,"HullPass1");
  hullPass2 = AcquireOpenCLKernel(device,"HullPass2");

  clStatus =clEnv->library->clSetKernelArg(hullPass1,0,sizeof(cl_mem),(void *)&imageBuffer);
  clStatus |=clEnv->library->clSetKernelArg(hullPass1,1,sizeof(cl_mem),(void *)(tempImageBuffer+1));
  imageWidth = (unsigned int) image->columns;
  clStatus |=clEnv->library->clSetKernelArg(hullPass1,2,sizeof(unsigned int),(void *)&imageWidth);
  imageHeight = (unsigned int) image->rows;
  clStatus |=clEnv->library->clSetKernelArg(hullPass1,3,sizeof(unsigned int),(void *)&imageHeight);
  matte = (image->alpha_trait > CopyPixelTrait)?1:0;
  clStatus |=clEnv->library->clSetKernelArg(hullPass1,6,sizeof(int),(void *)&matte);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
    goto cleanup;
  }

  clStatus = clEnv->library->clSetKernelArg(hullPass2,0,sizeof(cl_mem),(void *)(tempImageBuffer+1));
  clStatus |=clEnv->library->clSetKernelArg(hullPass2,1,sizeof(cl_mem),(void *)tempImageBuffer);
  imageWidth = (unsigned int) image->columns;
  clStatus |=clEnv->library->clSetKernelArg(hullPass2,2,sizeof(unsigned int),(void *)&imageWidth);
  imageHeight = (unsigned int) image->rows;
  clStatus |=clEnv->library->clSetKernelArg(hullPass2,3,sizeof(unsigned int),(void *)&imageHeight);
  matte = (image->alpha_trait > CopyPixelTrait)?1:0;
  clStatus |=clEnv->library->clSetKernelArg(hullPass2,6,sizeof(int),(void *)&matte);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
    goto cleanup;
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
    clStatus = clEnv->library->clSetKernelArg(hullPass1,4,sizeof(cl_int2),(void *)&offset);
    clStatus|= clEnv->library->clSetKernelArg(hullPass1,5,sizeof(int),(void *)&polarity);
    clStatus|=clEnv->library->clSetKernelArg(hullPass2,4,sizeof(cl_int2),(void *)&offset);
    clStatus|=clEnv->library->clSetKernelArg(hullPass2,5,sizeof(int),(void *)&polarity);
    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
      goto cleanup;
    }
    {
      const HullPassLaunch
        launch = { clEnv, device, queue, hullPass1, hullPass2,
          global_work_size };

      if (EnqueueHullPasses(&launch,exception) == MagickFalse)
        goto cleanup;
    }

    if (k == 0)
      clStatus =clEnv->library->clSetKernelArg(hullPass1,0,sizeof(cl_mem),(void *)(tempImageBuffer));
    offset.s[0] = -X[k];
    offset.s[1] = -Y[k];
    polarity = 1;
    clStatus = clEnv->library->clSetKernelArg(hullPass1,4,sizeof(cl_int2),(void *)&offset);
    clStatus|= clEnv->library->clSetKernelArg(hullPass1,5,sizeof(int),(void *)&polarity);
    clStatus|=clEnv->library->clSetKernelArg(hullPass2,4,sizeof(cl_int2),(void *)&offset);
    clStatus|=clEnv->library->clSetKernelArg(hullPass2,5,sizeof(int),(void *)&polarity);
    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
      goto cleanup;
    }
    {
      const HullPassLaunch
        launch = { clEnv, device, queue, hullPass1, hullPass2,
          global_work_size };

      if (EnqueueHullPasses(&launch,exception) == MagickFalse)
        goto cleanup;
    }

    offset.s[0] = -X[k];
    offset.s[1] = -Y[k];
    polarity = -1;
    clStatus = clEnv->library->clSetKernelArg(hullPass1,4,sizeof(cl_int2),(void *)&offset);
    clStatus|= clEnv->library->clSetKernelArg(hullPass1,5,sizeof(int),(void *)&polarity);
    clStatus|=clEnv->library->clSetKernelArg(hullPass2,4,sizeof(cl_int2),(void *)&offset);
    clStatus|=clEnv->library->clSetKernelArg(hullPass2,5,sizeof(int),(void *)&polarity);
    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
      goto cleanup;
    }
    {
      const HullPassLaunch
        launch = { clEnv, device, queue, hullPass1, hullPass2,
          global_work_size };

      if (EnqueueHullPasses(&launch,exception) == MagickFalse)
        goto cleanup;
    }

    offset.s[0] = X[k];
    offset.s[1] = Y[k];
    polarity = -1;
    clStatus = clEnv->library->clSetKernelArg(hullPass1,4,sizeof(cl_int2),(void *)&offset);
    clStatus|= clEnv->library->clSetKernelArg(hullPass1,5,sizeof(int),(void *)&polarity);
    clStatus|=clEnv->library->clSetKernelArg(hullPass2,4,sizeof(cl_int2),(void *)&offset);
    clStatus|=clEnv->library->clSetKernelArg(hullPass2,5,sizeof(int),(void *)&polarity);

    if (k == 3)
      clStatus |=clEnv->library->clSetKernelArg(hullPass2,1,sizeof(cl_mem),(void *)&filteredImageBuffer);

    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
      goto cleanup;
    }
    {
      const HullPassLaunch
        launch = { clEnv, device, queue, hullPass1, hullPass2,
          global_work_size };

      if (EnqueueHullPasses(&launch,exception) == MagickFalse)
        goto cleanup;
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

static Image *ComputeWaveletDenoiseImage(const Image *image,MagickCLEnv clEnv,
  const double threshold,ExceptionInfo *exception)
{
  cl_command_queue
    queue;

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

  cl_kernel
    denoiseKernel;

  cl_mem
    filteredImageBuffer,
    imageBuffer;

  cl_uint
    number_channels,
    width,
    height,
    max_channels;

  Image
    *filteredImage;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  size_t
    goffset[2],
    gsize[2],
    i,
    lsize[2],
    passes,
    x;

  filteredImage=NULL;
  imageBuffer=NULL;
  filteredImageBuffer=NULL;
  denoiseKernel=NULL;
  queue=NULL;
  outputReady=MagickFalse;

  device=RequestOpenCLDevice(clEnv);
  if (device == (MagickCLDevice) NULL)
    goto cleanup;
  /* Work around an issue on low end Intel devices */
  if (strcmp("Intel(R) HD Graphics",device->name) == 0)
    goto cleanup;
  queue=AcquireOpenCLCommandQueue(device);
  if (queue == (cl_command_queue) NULL)
    goto cleanup;
  filteredImage=CloneImage(image,0,0,MagickTrue,
    exception);
  if (filteredImage == (Image *) NULL)
    goto cleanup;
  if (filteredImage->number_channels != image->number_channels)
    goto cleanup;
  imageBuffer=GetAuthenticOpenCLBuffer(image,device,exception);
  if (imageBuffer == (cl_mem) NULL)
    goto cleanup;
  filteredImageBuffer=GetAuthenticOpenCLBuffer(filteredImage,device,exception);
  if (filteredImageBuffer == (cl_mem) NULL)
    goto cleanup;

  denoiseKernel=AcquireOpenCLKernel(device,"WaveletDenoise");
  if (denoiseKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    goto cleanup;
  }

  number_channels=(cl_uint)image->number_channels;
  width=(cl_uint)image->columns;
  height=(cl_uint)image->rows;
  max_channels=number_channels;
  if ((max_channels == 4) || (max_channels == 2))
    max_channels=max_channels-1;
  thresh=(cl_float) threshold;
  passes=(size_t) ((((1.0f*image->columns)*image->rows)+1999999.0f)/2000000.0f);
  passes=(passes < 1) ? 1 : passes;

  i=0;
  status =SetOpenCLKernelArg(denoiseKernel,i++,sizeof(cl_mem),(void *)&imageBuffer);
  status|=SetOpenCLKernelArg(denoiseKernel,i++,sizeof(cl_mem),(void *)&filteredImageBuffer);
  status|=SetOpenCLKernelArg(denoiseKernel,i++,sizeof(cl_uint),(void *)&number_channels);
  status|=SetOpenCLKernelArg(denoiseKernel,i++,sizeof(cl_uint),(void *)&max_channels);
  status|=SetOpenCLKernelArg(denoiseKernel,i++,sizeof(cl_float),(void *)&thresh);
  status|=SetOpenCLKernelArg(denoiseKernel,i++,sizeof(cl_int),(void *)&PASSES);
  status|=SetOpenCLKernelArg(denoiseKernel,i++,sizeof(cl_uint),(void *)&width);
  status|=SetOpenCLKernelArg(denoiseKernel,i++,sizeof(cl_uint),(void *)&height);
  if (status != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
        ResourceLimitWarning,"SetOpenCLKernelArg failed.",".");
      goto cleanup;
    }

  for (x = 0; x < passes; ++x)
  {
    gsize[0]=((width+(SIZE-1))/SIZE)*TILESIZE;
    gsize[1]=((((height+(SIZE-1))/SIZE)+passes-1)/passes)*4;
    lsize[0]=TILESIZE;
    lsize[1]=4;
    goffset[0]=0;
    goffset[1]=x*gsize[1];

    outputReady=EnqueueOpenCLKernel(queue,denoiseKernel,2,goffset,gsize,lsize,
      image,filteredImage,MagickTrue,exception);
    if (outputReady == MagickFalse)
      break;
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
