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

static void SelectResizeWorkgroup(const cl_uint resizedColumns,
  const unsigned int workgroupSize,unsigned int *chunkSize,
  unsigned int *pixelPerWorkgroup)
{
  if (resizedColumns < workgroupSize)
  {
    *chunkSize=32;
    *pixelPerWorkgroup=32;
  }
  else
  {
    *chunkSize=workgroupSize;
    *pixelPerWorkgroup=workgroupSize;
  }
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

typedef enum
{
  HorizontalResizeFilter,
  VerticalResizeFilter
} ResizeFilterSide;

typedef struct _ResizeFilterRun
{
  ResizeFilterSide
    side;

  const ResizeFilterPass
    *pass;

  float
    factor;
} ResizeFilterRun;

typedef struct _ResizeFilterLaunch
{
  cl_kernel
    filterKernel;

  unsigned int
    pixelPerWorkgroup,
    workgroupSize;
} ResizeFilterLaunch;

static MagickBooleanType EnqueueResizeFilterKernel(const ResizeFilterRun *run,
  const ResizeFilterLaunch *launch,ExceptionInfo *exception)
{
  const ResizeFilterSide
    side = run->side;

  const ResizeFilterPass
    *const pass = run->pass;

  const cl_kernel
    filterKernel = launch->filterKernel;

  const unsigned int
    pixelPerWorkgroup = launch->pixelPerWorkgroup,
    workgroupSize = launch->workgroupSize;

  size_t
    gsize[2],
    lsize[2];

  gsize[0]=((side == HorizontalResizeFilter) ?
    (pass->resizedColumns+pixelPerWorkgroup-1)/pixelPerWorkgroup*
    workgroupSize : pass->resizedColumns);
  gsize[1]=((side == HorizontalResizeFilter) ? pass->resizedRows :
    (pass->resizedRows+pixelPerWorkgroup-1)/pixelPerWorkgroup*
    workgroupSize);
  lsize[0]=((side == HorizontalResizeFilter) ? workgroupSize : 1);
  lsize[1]=((side == HorizontalResizeFilter) ? 1 : workgroupSize);
  return(EnqueueOpenCLKernel(pass->queue,filterKernel,2,
    (const size_t *) NULL,gsize,lsize,pass->image,pass->filteredImage,MagickFalse,
    exception));
}

static MagickBooleanType AcquireResizeFilterKernel(
  const ResizeFilterSide side,const ResizeFilterPass *pass,
  cl_kernel *filterKernel,ExceptionInfo *exception)
{
  *filterKernel=AcquireOpenCLKernel(pass->device,
    ((side == HorizontalResizeFilter) ? "ResizeHorizontalFilter" :
    "ResizeVerticalFilter"));
  if (*filterKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(pass->device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.", ".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _ResizeFilterArgsInput
{
  const ResizeFilterPass
    *pass;

  float
    factor;

  cl_kernel
    filterKernel;

  int
    resizeFilterType,
    resizeWindowType;

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
} ResizeFilterArgsInput;

static MagickBooleanType SetResizeFilterKernelArgs(
  const ResizeFilterArgsInput *input,ExceptionInfo *exception)
{
  const ResizeFilterPass
    *const pass = input->pass;

  const float
    factor = input->factor;

  const cl_kernel
    filterKernel = input->filterKernel;

  const int
    resizeFilterType = input->resizeFilterType,
    resizeWindowType = input->resizeWindowType;

  const size_t
    imageCacheLocalMemorySize = input->imageCacheLocalMemorySize;

  const int
    numCachedPixels = input->numCachedPixels;

  const unsigned int
    pixelPerWorkgroup = input->pixelPerWorkgroup,
    chunkSize = input->chunkSize;

  const size_t
    pixelAccumulatorLocalMemorySize = input->pixelAccumulatorLocalMemorySize,
    weightAccumulatorLocalMemorySize = input->weightAccumulatorLocalMemorySize,
    gammaAccumulatorLocalMemorySize = input->gammaAccumulatorLocalMemorySize;

  cl_int
    status;

  ResizeVerticalArgs
    args;

  args.resizeFilter=pass->resizeFilter;
  args.verticalKernel=filterKernel;
  args.imageBuffer=pass->imageBuffer;
  args.number_channels=pass->number_channels;
  args.columns=pass->columns;
  args.rows=pass->rows;
  args.resizedImageBuffer=pass->resizedImageBuffer;
  args.resizedColumns=pass->resizedColumns;
  args.resizedRows=pass->resizedRows;
  args.yFactor=factor;
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
    return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _ResizeFilterKernelSource
{
  const ResizeFilterPass
    *pass;

  ResizeFilterSide
    side;

  float
    factor;

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

  unsigned int
    workgroupSize;

  const ResizeFilterRun
    *run;
} ResizeFilterKernelSource;

typedef struct _ResizeFilterKernelTargets
{
  cl_kernel
    *filterKernel;

  MagickBooleanType
    *outputReady;
} ResizeFilterKernelTargets;

static MagickBooleanType RunResizeFilterKernel(
  const ResizeFilterKernelSource *source,ExceptionInfo *exception,
  const ResizeFilterKernelTargets *kernelTargets)
{
  const ResizeFilterPass
    *const pass = source->pass;

  const ResizeFilterSide
    side = source->side;

  const float
    factor = source->factor;

  const size_t
    imageCacheLocalMemorySize = source->imageCacheLocalMemorySize;

  const int
    numCachedPixels = source->numCachedPixels;

  const unsigned int
    pixelPerWorkgroup = source->pixelPerWorkgroup,
    chunkSize = source->chunkSize;

  const size_t
    pixelAccumulatorLocalMemorySize = source->pixelAccumulatorLocalMemorySize,
    weightAccumulatorLocalMemorySize = source->weightAccumulatorLocalMemorySize,
    gammaAccumulatorLocalMemorySize = source->gammaAccumulatorLocalMemorySize;

  const unsigned int
    workgroupSize = source->workgroupSize;

  const ResizeFilterRun
    *const run = source->run;

  cl_kernel
    *const filterKernel = kernelTargets->filterKernel;

  MagickBooleanType
    *const outputReady = kernelTargets->outputReady;

  int
    resizeFilterType,
    resizeWindowType;

  ResizeFilterLaunch
    launch;

  resizeFilterType=(int)GetResizeFilterWeightingType(pass->resizeFilter);
  resizeWindowType=(int)GetResizeFilterWindowWeightingType(pass->resizeFilter);

  if (AcquireResizeFilterKernel(side,pass,filterKernel,exception) ==
      MagickFalse)
    return(MagickFalse);

  {
    const ResizeFilterArgsInput
      input = { pass, factor, *filterKernel, resizeFilterType,
        resizeWindowType, imageCacheLocalMemorySize, numCachedPixels,
        pixelPerWorkgroup, chunkSize, pixelAccumulatorLocalMemorySize,
        weightAccumulatorLocalMemorySize, gammaAccumulatorLocalMemorySize };

    if (SetResizeFilterKernelArgs(&input,exception) == MagickFalse)
      return(MagickFalse);
  }

  launch.filterKernel=*filterKernel;
  launch.pixelPerWorkgroup=pixelPerWorkgroup;
  launch.workgroupSize=workgroupSize;
  *outputReady=EnqueueResizeFilterKernel(run,&launch,exception);
  return(MagickTrue);
}

static MagickBooleanType RunResizeFilterPass(const ResizeFilterRun *run,
  const unsigned int workgroupSize,ExceptionInfo *exception,
  const ResizeFilterKernelTargets *passTargets)
{
  const ResizeFilterSide
    side = run->side;

  const ResizeFilterPass
    *const pass = run->pass;

  const float
    factor = run->factor;

  cl_kernel
    *const filterKernel = passTargets->filterKernel;

  MagickBooleanType
    *const outputReady = passTargets->outputReady;

  float
    support;

  int
    numCachedPixels;

  size_t
    gammaAccumulatorLocalMemorySize,
    imageCacheLocalMemorySize,
    pixelAccumulatorLocalMemorySize,
    totalLocalMemorySize,
    weightAccumulatorLocalMemorySize;

  unsigned int
    chunkSize,
    pixelPerWorkgroup;

  /*
  Apply filter to resize vertically from image to resize image.
  */
  support=GetResizeScaledSupport(pass->resizeFilter,factor);

  SelectResizeWorkgroup(((side == HorizontalResizeFilter) ?
    pass->resizedColumns : pass->resizedRows),workgroupSize,&chunkSize,
    &pixelPerWorkgroup);

DisableMSCWarning(4127)
  while(1)
RestoreMSCWarning
  {
    /* calculate the local memory size needed per workgroup */
    numCachedPixels=(int) ceil((pixelPerWorkgroup-1)/factor+2*support);
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
    pixelPerWorkgroup=pixelPerWorkgroup/2;
    chunkSize=chunkSize/2;
    if ((pixelPerWorkgroup == 0) || (chunkSize == 0))
    {
      /* quit, fallback to CPU */
      return(MagickFalse);
    }
  }

  {
    const ResizeFilterKernelSource
      source = { pass, side, factor, imageCacheLocalMemorySize,
        numCachedPixels, pixelPerWorkgroup, chunkSize,
        pixelAccumulatorLocalMemorySize, weightAccumulatorLocalMemorySize,
        gammaAccumulatorLocalMemorySize, workgroupSize, run };

    const ResizeFilterKernelTargets
      kernelTargets = { filterKernel, outputReady };

    if (RunResizeFilterKernel(&source,exception,&kernelTargets) ==
        MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

static inline MagickBooleanType ResizeFilterImagePass(
  const ResizeFilterRun *run,ExceptionInfo *exception)
{
  cl_kernel
    filterKernel;

  const unsigned int
    workgroupSize = 256;

  MagickBooleanType
    outputReady;

  filterKernel=NULL;
  outputReady=MagickFalse;

  {
    const ResizeFilterKernelTargets
      passTargets = { &filterKernel, &outputReady };

    if (RunResizeFilterPass(run,workgroupSize,exception,&passTargets) ==
        MagickFalse)
      goto cleanup;
  }

cleanup:

  ReleaseKernel(filterKernel);

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

typedef struct _ResizeArmOrder
{
  ResizeFilterSide
    firstSide,
    secondSide;

  float
    firstFactor,
    secondFactor;

  size_t
    tempColumns,
    tempRows;
} ResizeArmOrder;

static MagickBooleanType ResizeThroughTempImage(const ResizeArmInfo *info,
  const ResizeArmOrder *order,ExceptionInfo *exception,
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
    resizedRows = info->resizedRows,
    tempColumns = order->tempColumns,
    tempRows = order->tempRows;

  const ResizeFilter
    *const resizeFilter = info->resizeFilter;

  cl_mem
    *const tempImageBuffer = targets->tempImageBuffer;

  MagickBooleanType
    *const outputReady = targets->outputReady;

  size_t
    length;

  length=tempColumns*tempRows*number_channels;
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
        number_channels, (cl_uint) image->columns,
        ((order->firstSide == HorizontalResizeFilter) ? (cl_uint) image->rows :
        (cl_int) image->rows),
        *tempImageBuffer, (cl_uint) tempColumns, (cl_uint) tempRows,
        resizeFilter, cubicCoefficientsBuffer };

    const ResizeFilterRun
      run = { order->firstSide, &pass, order->firstFactor };

    *outputReady=ResizeFilterImagePass(&run,exception);
  }
  if (*outputReady == MagickFalse)
    return(MagickFalse);

  {
    const ResizeFilterPass
      pass = { device, queue, image, filteredImage, *tempImageBuffer,
        number_channels, (cl_uint) tempColumns, (cl_uint) tempRows,
        filteredImageBuffer, (cl_uint) resizedColumns, (cl_uint) resizedRows,
        resizeFilter, cubicCoefficientsBuffer };

    const ResizeFilterRun
      run = { order->secondSide, &pass, order->secondFactor };

    *outputReady=ResizeFilterImagePass(&run,exception);
  }
  if (*outputReady == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType ResizeHorizontalThenVertical(
  const ResizeArmInfo *info,ExceptionInfo *exception,
  const ResizeArmTargets *targets)
{
  const ResizeArmOrder
    order = { HorizontalResizeFilter, VerticalResizeFilter, info->xFactor,
      info->yFactor, info->resizedColumns, info->image->rows };

  return(ResizeThroughTempImage(info,&order,exception,targets));
}

typedef struct _ResizeImageSource
{
  MagickCLEnv
    clEnv;

  const Image
    *image;

  size_t
    resizedColumns,
    resizedRows;
} ResizeImageSource;

typedef struct _ResizeImageTargets
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
} ResizeImageTargets;

static MagickBooleanType AcquireResizeBuffers(const ResizeImageSource *source,
  ExceptionInfo *exception,const ResizeImageTargets *targets)
{
  const MagickCLEnv
    clEnv = source->clEnv;

  const Image
    *const image = source->image;

  const size_t
    resizedColumns = source->resizedColumns,
    resizedRows = source->resizedRows;

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
  *filteredImage=CloneImage(image,resizedColumns,resizedRows,MagickTrue,
    exception);
  if (*filteredImage == (Image *) NULL)
    return(MagickFalse);
  if ((*filteredImage)->number_channels != image->number_channels)
    return(MagickFalse);
  *imageBuffer=GetAuthenticOpenCLBuffer(image,*device,exception);
  if (*imageBuffer == (cl_mem) NULL)
    return(MagickFalse);
  *filteredImageBuffer=GetAuthenticOpenCLBuffer(*filteredImage,*device,
    exception);
  if (*filteredImageBuffer == (cl_mem) NULL)
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

static MagickBooleanType ResizeVerticalThenHorizontal(
  const ResizeArmInfo *info,ExceptionInfo *exception,
  const ResizeArmTargets *targets)
{
  const ResizeArmOrder
    order = { VerticalResizeFilter, HorizontalResizeFilter, info->yFactor,
      info->xFactor, info->image->columns, info->resizedRows };

  return(ResizeThroughTempImage(info,&order,exception,targets));
}

static Image *ComputeResizeImage(const Image* image,MagickCLEnv clEnv,
  const AccelerateResizeRequest *resize,ExceptionInfo *exception)
{
  const size_t
    resizedColumns = resize->resizedColumns,
    resizedRows = resize->resizedRows;

  const ResizeFilter
    *const resizeFilter = resize->resizeFilter;

  cl_command_queue
    queue=NULL;

  cl_mem
    cubicCoefficientsBuffer=NULL,
    filteredImageBuffer=NULL,
    imageBuffer=NULL,
    tempImageBuffer=NULL;

  cl_uint
    number_channels;

  float
    xFactor,
    yFactor;

  MagickBooleanType
    outputReady=MagickFalse,
    status;

  MagickCLDevice
    device;

  Image
    *filteredImage=NULL;

  ResizeArmTargets
    targets;

  {
    const ResizeImageSource
      source = { clEnv, image, resizedColumns, resizedRows };

    const ResizeImageTargets
      imageTargets = { &device, &queue, &filteredImage, &imageBuffer,
        &filteredImageBuffer };

    if (AcquireResizeBuffers(&source,exception,&imageTargets) == MagickFalse)
      goto cleanup;
  }

  if (CreateCubicCoefficientsBuffer(device,resizeFilter,
        &cubicCoefficientsBuffer,exception) == MagickFalse)
    goto cleanup;

  number_channels=(cl_uint) image->number_channels;
  xFactor=(float) resizedColumns/(float) image->columns;
  yFactor=(float) resizedRows/(float) image->rows;
  {
    const ResizeArmInfo
      info = { device, queue, image, filteredImage, imageBuffer,
        filteredImageBuffer, cubicCoefficientsBuffer, number_channels,
        resizedColumns, resizedRows, resizeFilter, xFactor, yFactor };

    targets.tempImageBuffer=(&tempImageBuffer);
    targets.outputReady=(&outputReady);
    if (xFactor > yFactor)
      status=ResizeHorizontalThenVertical(&info,exception,&targets);
    else
      status=ResizeVerticalThenHorizontal(&info,exception,&targets);
  }
  if (status == MagickFalse)
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
  const AccelerateResizeRequest *resize,ExceptionInfo *exception)
{
  const ResizeFilter
    *const resizeFilter = resize->resizeFilter;

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

  filteredImage=ComputeResizeImage(image,clEnv,resize,exception);
  return(filteredImage);
}
#endif /* MAGICKCORE_OPENCL_SUPPORT */
