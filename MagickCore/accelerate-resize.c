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
%                  RRRR   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                   %
%                  R   R  E      SS       I       ZZ  E                       %
%                  RRRR   EEE     SSS     I     ZZZ   EEE                     %
%                  R R    E         SS    I    ZZ     E                       %
%                  R  R   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                   %
%                                                                             %
%                                                                             %
%                 MagickCore OpenCL Accelerated Resize Method                 %
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
  Define declarations.
*/
#define MAGICK_MAX(x,y) (((x) >= (y))?(x):(y))

/*
  Constant declarations.
*/
static const ResizeWeightingFunctionType supportedResizeWeighting[] =
{
  BoxWeightingFunction,
  TriangleWeightingFunction,
  HannWeightingFunction,
  HammingWeightingFunction,
  BlackmanWeightingFunction,
  CubicBCWeightingFunction,
  SincWeightingFunction,
  SincFastWeightingFunction,
  LastWeightingFunction
};

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A c c e l e r a t e R e s i z e I m a g e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

static size_t GetGammaAccumulatorSize(const cl_uint number_channels,
  const unsigned int chunkSize)
{
  size_t
    gammaAccumulatorLocalMemorySize;

  if ((number_channels == 4) || (number_channels == 2))
    gammaAccumulatorLocalMemorySize=chunkSize*sizeof(float);
  else
    gammaAccumulatorLocalMemorySize=sizeof(float);
  return(gammaAccumulatorLocalMemorySize);
}

static float GetResizeScaledSupport(const ResizeFilter *resizeFilter,
  const float xFactor)
{
  float
    scale,
    support;

  scale=(float) MAGICK_MAX(1.0/xFactor+MagickEpsilon,1.0);
  support=scale*(float) GetResizeFilterSupport(resizeFilter);
  if (support < 0.5)
  {
    /*
    Support too small even for nearest neighbour: Reduce to point
    sampling.
    */
    support=(float) 0.5;
    scale=1.0;
  }
  scale=(float) MagickSafeReciprocal(scale);
  return(support);
}

typedef struct _ResizeFilterPass
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
    imageBuffer;

  cl_uint
    number_channels,
    columns,
    rows;

  cl_mem
    resizedImageBuffer;

  cl_uint
    resizedColumns,
    resizedRows;

  const ResizeFilter
    *resizeFilter;

  cl_mem
    resizeFilterCubicCoefficients;
} ResizeFilterPass;

static MagickBooleanType resizeHorizontalFilter(const ResizeFilterPass *pass,
  const float xFactor,ExceptionInfo *exception)
{
  cl_kernel
    horizontalKernel;

  cl_int
    status;

  cl_uint
    i;

  const unsigned int
    workgroupSize = 256;

  float
    resizeFilterScale,
    resizeFilterSupport,
    resizeFilterWindowSupport,
    resizeFilterBlur,
    support;

  int
    numCachedPixels,
    resizeFilterType,
    resizeWindowType;

  MagickBooleanType
    outputReady;

  size_t
    gammaAccumulatorLocalMemorySize,
    gsize[2],
    imageCacheLocalMemorySize,
    pixelAccumulatorLocalMemorySize,
    lsize[2],
    totalLocalMemorySize,
    weightAccumulatorLocalMemorySize;

  unsigned int
    chunkSize,
    pixelPerWorkgroup;

  horizontalKernel=NULL;
  outputReady=MagickFalse;

  /*
  Apply filter to resize vertically from image to resize image.
  */
  support=GetResizeScaledSupport(pass->resizeFilter,xFactor);

  if (pass->resizedColumns < workgroupSize)
  {
    chunkSize=32;
    pixelPerWorkgroup=32;
  }
  else
  {
    chunkSize=workgroupSize;
    pixelPerWorkgroup=workgroupSize;
  }

DisableMSCWarning(4127)
  while(1)
RestoreMSCWarning
  {
    /* calculate the local memory size needed per workgroup */
    numCachedPixels=(int) ceil((pixelPerWorkgroup-1)/xFactor+2*support);
    imageCacheLocalMemorySize=numCachedPixels*sizeof(CLQuantum)*
      pass->number_channels;
    totalLocalMemorySize=imageCacheLocalMemorySize;

    /* local size for the pixel accumulator */
    pixelAccumulatorLocalMemorySize=chunkSize*sizeof(cl_float4);
    totalLocalMemorySize+=pixelAccumulatorLocalMemorySize;

    /* local memory size for the weight accumulator */
    weightAccumulatorLocalMemorySize=chunkSize*sizeof(float);
    totalLocalMemorySize+=weightAccumulatorLocalMemorySize;

    /* local memory size for the gamma accumulator */
    gammaAccumulatorLocalMemorySize=GetGammaAccumulatorSize(pass->number_channels,
      chunkSize);
    totalLocalMemorySize+=gammaAccumulatorLocalMemorySize;

    if (totalLocalMemorySize <= pass->device->local_memory_size)
      break;
    else
    {
      pixelPerWorkgroup=pixelPerWorkgroup/2;
      chunkSize=chunkSize/2;
      if ((pixelPerWorkgroup == 0) || (chunkSize == 0))
      {
        /* quit, fallback to CPU */
        goto cleanup;
      }
    }
  }

  resizeFilterType=(int)GetResizeFilterWeightingType(pass->resizeFilter);
  resizeWindowType=(int)GetResizeFilterWindowWeightingType(pass->resizeFilter);

  horizontalKernel=AcquireOpenCLKernel(pass->device,"ResizeHorizontalFilter");
  if (horizontalKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(pass->device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.", ".");
    goto cleanup;
  }

  resizeFilterScale=(float) GetResizeFilterScale(pass->resizeFilter);
  resizeFilterSupport=(float) GetResizeFilterSupport(pass->resizeFilter);
  resizeFilterWindowSupport=(float) GetResizeFilterWindowSupport(pass->resizeFilter);
  resizeFilterBlur=(float) GetResizeFilterBlur(pass->resizeFilter);

  i=0;
  status =SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_mem),(void*)&pass->imageBuffer);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_uint),(void*)&pass->number_channels);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_uint),(void*)&pass->columns);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_uint),(void*)&pass->rows);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_mem),(void*)&pass->resizedImageBuffer);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_uint),(void*)&pass->resizedColumns);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_uint),(void*)&pass->resizedRows);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(float),(void*)&xFactor);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(int),(void*)&resizeFilterType);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(int),(void*)&resizeWindowType);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_mem),(void*)&pass->resizeFilterCubicCoefficients);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(float),(void*)&resizeFilterScale);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(float),(void*)&resizeFilterSupport);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(float),(void*)&resizeFilterWindowSupport);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(float),(void*)&resizeFilterBlur);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,imageCacheLocalMemorySize,NULL);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(int),&numCachedPixels);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(unsigned int),&pixelPerWorkgroup);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(unsigned int),&chunkSize);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,pixelAccumulatorLocalMemorySize,NULL);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,weightAccumulatorLocalMemorySize,NULL);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,gammaAccumulatorLocalMemorySize,NULL);

  if (status != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(pass->device,exception,GetMagickModule(),
      ResourceLimitWarning,"SetOpenCLKernelArg failed.",".");
    goto cleanup;
  }

  gsize[0]=(pass->resizedColumns+pixelPerWorkgroup-1)/pixelPerWorkgroup*
    workgroupSize;
  gsize[1]=pass->resizedRows;
  lsize[0]=workgroupSize;
  lsize[1]=1;
  outputReady=EnqueueOpenCLKernel(pass->queue,horizontalKernel,2,
    (const size_t *) NULL,gsize,lsize,pass->image,pass->filteredImage,MagickFalse,
    exception);

cleanup:

  ReleaseKernel(horizontalKernel);

  return(outputReady);
}

typedef struct _ResizeVerticalArgs
{
  const ResizeFilter
    *resizeFilter;

  cl_kernel
    verticalKernel;

  cl_mem
    imageBuffer;

  cl_uint
    number_channels,
    columns,
    rows;

  cl_mem
    resizedImageBuffer;

  cl_uint
    resizedColumns,
    resizedRows;

  float
    yFactor;

  int
    resizeFilterType,
    resizeWindowType;

  cl_mem
    resizeFilterCubicCoefficients;

  size_t
    imageCacheLocalMemorySize;

  int
    numCachedPixels;

  unsigned int
    pixelPerWorkgroup,
    chunkSize;

  size_t
    pixelAccumulatorLocalMemorySize,
    weightAccumulatorLocalMemorySize,
    gammaAccumulatorLocalMemorySize;
} ResizeVerticalArgs;

static cl_int SetResizeVerticalFilterArgs(const ResizeVerticalArgs *args)
{
  cl_int
    status;

  float
    resizeFilterScale,
    resizeFilterSupport,
    resizeFilterWindowSupport,
    resizeFilterBlur;

  size_t
    i;

  resizeFilterScale=(float) GetResizeFilterScale(args->resizeFilter);
  resizeFilterSupport=(float) GetResizeFilterSupport(args->resizeFilter);
  resizeFilterBlur=(float) GetResizeFilterBlur(args->resizeFilter);
  resizeFilterWindowSupport=(float) GetResizeFilterWindowSupport(args->resizeFilter);
  i=0;
  status =SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(cl_mem),(void*)&args->imageBuffer);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(cl_uint),(void*)&args->number_channels);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(cl_uint),(void*)&args->columns);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(cl_uint),(void*)&args->rows);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(cl_mem),(void*)&args->resizedImageBuffer);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(cl_uint),(void*)&args->resizedColumns);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(cl_uint),(void*)&args->resizedRows);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(float),(void*)&args->yFactor);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(int),(void*)&args->resizeFilterType);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(int),(void*)&args->resizeWindowType);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(cl_mem),(void*)&args->resizeFilterCubicCoefficients);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(float),(void*)&resizeFilterScale);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(float),(void*)&resizeFilterSupport);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(float),(void*)&resizeFilterWindowSupport);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(float),(void*)&resizeFilterBlur);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,args->imageCacheLocalMemorySize, NULL);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(int), &args->numCachedPixels);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(unsigned int), &args->pixelPerWorkgroup);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,sizeof(unsigned int), &args->chunkSize);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,args->pixelAccumulatorLocalMemorySize, NULL);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,args->weightAccumulatorLocalMemorySize, NULL);
  status|=SetOpenCLKernelArg(args->verticalKernel,i++,args->gammaAccumulatorLocalMemorySize, NULL);
  return(status);
}

static MagickBooleanType resizeVerticalFilter(const ResizeFilterPass *pass,
  const float yFactor,ExceptionInfo *exception)
{
  cl_kernel
    verticalKernel;

  cl_int
    status;

  const unsigned int
    workgroupSize = 256;

  float
    support;

  int
    numCachedPixels,
    resizeFilterType,
    resizeWindowType;

  MagickBooleanType
    outputReady;

  ResizeVerticalArgs
    args;

  size_t
    gammaAccumulatorLocalMemorySize,
    gsize[2],
    imageCacheLocalMemorySize,
    pixelAccumulatorLocalMemorySize,
    lsize[2],
    totalLocalMemorySize,
    weightAccumulatorLocalMemorySize;

  unsigned int
    chunkSize,
    pixelPerWorkgroup;

  verticalKernel=NULL;
  outputReady=MagickFalse;

  /*
  Apply filter to resize vertically from image to resize image.
  */
  support=GetResizeScaledSupport(pass->resizeFilter,yFactor);

  if (pass->resizedRows < workgroupSize)
  {
    chunkSize=32;
    pixelPerWorkgroup=32;
  }
  else
  {
    chunkSize=workgroupSize;
    pixelPerWorkgroup=workgroupSize;
  }

DisableMSCWarning(4127)
  while(1)
RestoreMSCWarning
  {
    /* calculate the local memory size needed per workgroup */
    numCachedPixels=(int)ceil((pixelPerWorkgroup-1)/yFactor+2*support);
    imageCacheLocalMemorySize=numCachedPixels*sizeof(CLQuantum)*
      pass->number_channels;
    totalLocalMemorySize=imageCacheLocalMemorySize;

    /* local size for the pixel accumulator */
    pixelAccumulatorLocalMemorySize=chunkSize*sizeof(cl_float4);
    totalLocalMemorySize+=pixelAccumulatorLocalMemorySize;

    /* local memory size for the weight accumulator */
    weightAccumulatorLocalMemorySize=chunkSize*sizeof(float);
    totalLocalMemorySize+=weightAccumulatorLocalMemorySize;

    /* local memory size for the gamma accumulator */
    gammaAccumulatorLocalMemorySize=GetGammaAccumulatorSize(pass->number_channels,
      chunkSize);
    totalLocalMemorySize+=gammaAccumulatorLocalMemorySize;

    if (totalLocalMemorySize <= pass->device->local_memory_size)
      break;
    else
    {
      pixelPerWorkgroup=pixelPerWorkgroup/2;
      chunkSize=chunkSize/2;
      if ((pixelPerWorkgroup == 0) || (chunkSize == 0))
      {
        /* quit, fallback to CPU */
        goto cleanup;
      }
    }
  }

  resizeFilterType=(int)GetResizeFilterWeightingType(pass->resizeFilter);
  resizeWindowType=(int)GetResizeFilterWindowWeightingType(pass->resizeFilter);

  verticalKernel=AcquireOpenCLKernel(pass->device,"ResizeVerticalFilter");
  if (verticalKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(pass->device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    goto cleanup;
  }

  args.resizeFilter=pass->resizeFilter;
  args.verticalKernel=verticalKernel;
  args.imageBuffer=pass->imageBuffer;
  args.number_channels=pass->number_channels;
  args.columns=pass->columns;
  args.rows=pass->rows;
  args.resizedImageBuffer=pass->resizedImageBuffer;
  args.resizedColumns=pass->resizedColumns;
  args.resizedRows=pass->resizedRows;
  args.yFactor=yFactor;
  args.resizeFilterType=resizeFilterType;
  args.resizeWindowType=resizeWindowType;
  args.resizeFilterCubicCoefficients=pass->resizeFilterCubicCoefficients;
  args.imageCacheLocalMemorySize=imageCacheLocalMemorySize;
  args.numCachedPixels=numCachedPixels;
  args.pixelPerWorkgroup=pixelPerWorkgroup;
  args.chunkSize=chunkSize;
  args.pixelAccumulatorLocalMemorySize=pixelAccumulatorLocalMemorySize;
  args.weightAccumulatorLocalMemorySize=weightAccumulatorLocalMemorySize;
  args.gammaAccumulatorLocalMemorySize=gammaAccumulatorLocalMemorySize;
  status=SetResizeVerticalFilterArgs(&args);

  if (status != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(pass->device,exception,GetMagickModule(),
      ResourceLimitWarning,"SetOpenCLKernelArg failed.",".");
    goto cleanup;
  }

  gsize[0]=pass->resizedColumns;
  gsize[1]=(pass->resizedRows+pixelPerWorkgroup-1)/pixelPerWorkgroup*
    workgroupSize;
  lsize[0]=1;
  lsize[1]=workgroupSize;
  outputReady=EnqueueOpenCLKernel(pass->queue,verticalKernel,2,(const size_t *) NULL,
    gsize,lsize,pass->image,pass->filteredImage,MagickFalse,exception);

cleanup:

  ReleaseKernel(verticalKernel);

  return(outputReady);
}

typedef struct _ResizeArmInfo
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
    filteredImageBuffer,
    cubicCoefficientsBuffer;

  cl_uint
    number_channels;

  size_t
    resizedColumns,
    resizedRows;

  const ResizeFilter
    *resizeFilter;

  float
    xFactor,
    yFactor;
} ResizeArmInfo;

typedef struct _ResizeArmTargets
{
  cl_mem
    *tempImageBuffer;

  MagickBooleanType
    *outputReady;
} ResizeArmTargets;

static MagickBooleanType ResizeHorizontalThenVertical(
  const ResizeArmInfo *info,ExceptionInfo *exception,
  const ResizeArmTargets *targets)
{
  const Image
    *const image = info->image;

  const MagickCLDevice
    device = info->device;

  const cl_command_queue
    queue = info->queue;

  Image
    *const filteredImage = info->filteredImage;

  const cl_mem
    cubicCoefficientsBuffer = info->cubicCoefficientsBuffer,
    filteredImageBuffer = info->filteredImageBuffer,
    imageBuffer = info->imageBuffer;

  const cl_uint
    number_channels = info->number_channels;

  const size_t
    resizedColumns = info->resizedColumns,
    resizedRows = info->resizedRows;

  const ResizeFilter
    *const resizeFilter = info->resizeFilter;

  const float
    xFactor = info->xFactor,
    yFactor = info->yFactor;

  cl_mem
    *const tempImageBuffer = targets->tempImageBuffer;

  MagickBooleanType
    *const outputReady = targets->outputReady;

  size_t
    length;

  length=resizedColumns*image->rows*number_channels;
  *tempImageBuffer=CreateOpenCLBuffer(device,CL_MEM_READ_WRITE,length*
    sizeof(CLQuantum),(void *) NULL);
  if (*tempImageBuffer == (cl_mem) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"CreateOpenCLBuffer failed.",".");
    return(MagickFalse);
  }

  {
    const ResizeFilterPass
      pass = { device, queue, image, filteredImage, imageBuffer,
        number_channels, (cl_uint) image->columns, (cl_uint) image->rows,
        *tempImageBuffer, (cl_uint) resizedColumns, (cl_uint) image->rows,
        resizeFilter, cubicCoefficientsBuffer };

    *outputReady=resizeHorizontalFilter(&pass,xFactor,exception);
  }
  if (*outputReady == MagickFalse)
    return(MagickFalse);

  {
    const ResizeFilterPass
      pass = { device, queue, image, filteredImage, *tempImageBuffer,
        number_channels, (cl_uint) resizedColumns, (cl_uint) image->rows,
        filteredImageBuffer, (cl_uint) resizedColumns, (cl_uint) resizedRows,
        resizeFilter, cubicCoefficientsBuffer };

    *outputReady=resizeVerticalFilter(&pass,yFactor,exception);
  }
  if (*outputReady == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType CreateCubicCoefficientsBuffer(
  MagickCLDevice device,const ResizeFilter *resizeFilter,
  cl_mem *cubicCoefficientsBuffer,ExceptionInfo *exception)
{
  const double
    *resizeFilterCoefficient;

  float
    coefficientBuffer[7];

  size_t
    i;

  resizeFilterCoefficient=GetResizeFilterCoefficient(resizeFilter);
  for (i = 0; i < 7; i++)
    coefficientBuffer[i]=(float) resizeFilterCoefficient[i];
  *cubicCoefficientsBuffer=CreateOpenCLBuffer(device,CL_MEM_COPY_HOST_PTR |
    CL_MEM_READ_ONLY,sizeof(coefficientBuffer),&coefficientBuffer);
  if (*cubicCoefficientsBuffer == (cl_mem) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"CreateOpenCLBuffer failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

static Image *ComputeResizeImage(const Image* image,MagickCLEnv clEnv,
  const size_t resizedColumns,const size_t resizedRows,
  const ResizeFilter *resizeFilter,ExceptionInfo *exception)
{
  cl_command_queue
    queue;

  cl_mem
    cubicCoefficientsBuffer,
    filteredImageBuffer,
    imageBuffer,
    tempImageBuffer;

  cl_uint
    number_channels;

  float
    xFactor,
    yFactor;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  Image
    *filteredImage;

  ResizeArmInfo
    info;

  ResizeArmTargets
    targets;

  size_t
    length;

  queue=NULL;
  filteredImage=NULL;
  imageBuffer=NULL;
  filteredImageBuffer=NULL;
  tempImageBuffer=NULL;
  cubicCoefficientsBuffer=NULL;
  outputReady=MagickFalse;

  device=RequestOpenCLDevice(clEnv);
  if (device == (MagickCLDevice) NULL)
    goto cleanup;
  queue=AcquireOpenCLCommandQueue(device);
  if (queue == (cl_command_queue) NULL)
    goto cleanup;
  filteredImage=CloneImage(image,resizedColumns,resizedRows,MagickTrue,
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

  if (CreateCubicCoefficientsBuffer(device,resizeFilter,
        &cubicCoefficientsBuffer,exception) == MagickFalse)
    goto cleanup;

  number_channels=(cl_uint) image->number_channels;
  xFactor=(float) resizedColumns/(float) image->columns;
  yFactor=(float) resizedRows/(float) image->rows;
  if (xFactor > yFactor)
  {
    info.device=device;
    info.queue=queue;
    info.image=image;
    info.filteredImage=filteredImage;
    info.imageBuffer=imageBuffer;
    info.filteredImageBuffer=filteredImageBuffer;
    info.cubicCoefficientsBuffer=cubicCoefficientsBuffer;
    info.number_channels=number_channels;
    info.resizedColumns=resizedColumns;
    info.resizedRows=resizedRows;
    info.resizeFilter=resizeFilter;
    info.xFactor=xFactor;
    info.yFactor=yFactor;
    targets.tempImageBuffer=(&tempImageBuffer);
    targets.outputReady=(&outputReady);
  }
  else
  {
    length=image->columns*resizedRows*number_channels;
    tempImageBuffer=CreateOpenCLBuffer(device,CL_MEM_READ_WRITE,length*
      sizeof(CLQuantum),(void *) NULL);
    if (tempImageBuffer == (cl_mem) NULL)
    {
      (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
        ResourceLimitWarning,"CreateOpenCLBuffer failed.",".");
      goto cleanup;
    }

    {
      const ResizeFilterPass
        pass = { device, queue, image, filteredImage, imageBuffer,
          number_channels, (cl_uint) image->columns, (cl_int) image->rows,
          tempImageBuffer, (cl_uint) image->columns, (cl_uint) resizedRows,
          resizeFilter, cubicCoefficientsBuffer };

      outputReady=resizeVerticalFilter(&pass,yFactor,exception);
    }
    if (outputReady == MagickFalse)
      goto cleanup;

    {
      const ResizeFilterPass
        pass = { device, queue, image, filteredImage, tempImageBuffer,
          number_channels, (cl_uint) image->columns, (cl_uint) resizedRows,
          filteredImageBuffer, (cl_uint) resizedColumns,
          (cl_uint) resizedRows, resizeFilter, cubicCoefficientsBuffer };

      outputReady=resizeHorizontalFilter(&pass,xFactor,exception);
    }
    if (outputReady == MagickFalse)
      goto cleanup;
  }
  if ((xFactor > yFactor) &&
      (ResizeHorizontalThenVertical(&info,exception,&targets) == MagickFalse))
    goto cleanup;

cleanup:

  ReleaseOpenCLBuffer(imageBuffer);
  ReleaseOpenCLBuffer(filteredImageBuffer);
  ReleaseOpenCLBuffer(tempImageBuffer);
  ReleaseOpenCLBuffer(cubicCoefficientsBuffer);
  ReleaseAccelerateQueue(device,queue);
  filteredImage=DestroyUnfinishedImage(outputReady,filteredImage);

  return(filteredImage);
}

static MagickBooleanType gpuSupportedResizeWeighting(
  ResizeWeightingFunctionType f)
{
  unsigned int
    i;

  for (i = 0; ;i++)
  {
    if (supportedResizeWeighting[i] == LastWeightingFunction)
      break;
    if (supportedResizeWeighting[i] == f)
      return(MagickTrue);
  }
  return(MagickFalse);
}

MagickPrivate Image *AccelerateResizeImage(const Image *image,
  const size_t resizedColumns,const size_t resizedRows,
  const ResizeFilter *resizeFilter,ExceptionInfo *exception)
{
  Image
    *filteredImage;

  MagickCLEnv
    clEnv;

  assert(image != NULL);
  assert(exception != (ExceptionInfo *) NULL);

  if (checkAccelerateCondition(image) == MagickFalse)
    return((Image *) NULL);

  if ((gpuSupportedResizeWeighting(GetResizeFilterWeightingType(
         resizeFilter)) == MagickFalse) ||
      (gpuSupportedResizeWeighting(GetResizeFilterWindowWeightingType(
         resizeFilter)) == MagickFalse))
    return((Image *) NULL);

  clEnv=getOpenCLEnvironment(exception);
  if (clEnv == (MagickCLEnv) NULL)
    return((Image *) NULL);

  filteredImage=ComputeResizeImage(image,clEnv,resizedColumns,resizedRows,
    resizeFilter,exception);
  return(filteredImage);
}
#endif /* MAGICKCORE_OPENCL_SUPPORT */
