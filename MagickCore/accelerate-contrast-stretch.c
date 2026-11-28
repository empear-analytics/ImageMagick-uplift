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
%            CCCC   OOO   N   N  TTTTT  RRRR    AAA   SSSSS  TTTTT            %
%           C      O   O  NN  N    T    R   R  A   A  SS       T              %
%           C      O   O  N N N    T    RRRR   AAAAA   SSS     T              %
%           C      O   O  N  NN    T    R R    A   A     SS    T              %
%            CCCC   OOO   N   N    T    R  R   A   A  SSSSS    T              %
%                                                                             %
%               SSSSS  TTTTT  RRRR   EEEEE  TTTTT   CCCC  H   H               %
%               SS       T    R   R  E        T    C      H   H               %
%                SSS     T    RRRR   EEE      T    C      HHHHH               %
%                  SS    T    R R    E        T    C      H   H               %
%               SSSSS    T    R  R   EEEEE    T     CCCC  H   H               %
%                                                                             %
%                                                                             %
%            MagickCore OpenCL Accelerated Contrast Stretch Method            %
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
#include "MagickCore/accelerate-histogram-private.h"
#if defined(MAGICKCORE_OPENCL_SUPPORT)

/*
  Typedef declarations.
*/
typedef struct _ContrastStretchPoints
{
  double
    black_point,
    white_point;
} ContrastStretchPoints;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A c c e l e r a t e C o n t r a s t S t r e t c h I m a g e             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

static MagickBooleanType IsStretchDirectClass(const Image *image)
{
  return((((image->channel_mask & AlphaChannel) != 0) || (((image->channel_mask & IndexChannel) != 0) &&
      (image->colorspace == CMYKColorspace))) ? MagickTrue : MagickFalse);
}

static cl_uint FindStretchBlackBin(const cl_uint4 *histogram,
  const double black_point)
{
  cl_uint
    i;

  double
    intensity;

  intensity=0.0;
  for (i=0; i <= (ssize_t) MaxMap; i++)
  {
    intensity+=histogram[i].s[2];
    if (intensity > black_point)
      break;
  }
  return(i);
}

static cl_uint FindStretchWhiteBin(const Image *image,
  const cl_uint4 *histogram,const double white_point)
{
  cl_uint
    i;

  double
    intensity;

  intensity=0.0;
  for (i=(ssize_t) MaxMap; i != 0; i--)
  {
    intensity+=histogram[i].s[2];
    if (intensity > ((double) image->columns*image->rows-white_point))
      break;
  }
  return(i);
}

static MagickBooleanType StretchesRedChannel(const Image *image,
  const cl_float4 black,const cl_float4 white)
{
  return((((image->channel_mask & RedChannel) != 0) &&
    (black.x != white.x)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType StretchesGreenChannel(const Image *image,
  const cl_float4 black,const cl_float4 white)
{
  return((((image->channel_mask & GreenChannel) != 0) &&
    (black.y != white.y)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType StretchesBlueChannel(const Image *image,
  const cl_float4 black,const cl_float4 white)
{
  return((((image->channel_mask & BlueChannel) != 0) &&
    (black.z != white.z)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType StretchesAlphaChannel(const Image *image,
  const cl_float4 black,const cl_float4 white)
{
  return((((image->channel_mask & AlphaChannel) != 0) &&
    (black.w != white.w)) ? MagickTrue : MagickFalse);
}

static void StretchColormap(Image *image,PixelPacket *stretch_map,
  const cl_float4 black,const cl_float4 white)
{
  cl_uint
    i;

  for (i=0; i < (cl_uint) image->colors; i++)
  {
    if (StretchesRedChannel(image,black,white) != MagickFalse)
      image->colormap[i].red=stretch_map[
        ScaleQuantumToMap((const Quantum) image->colormap[i].red)].red;
    if (StretchesGreenChannel(image,black,white) != MagickFalse)
      image->colormap[i].green=stretch_map[
        ScaleQuantumToMap((const Quantum) image->colormap[i].green)].green;
    if (StretchesBlueChannel(image,black,white) != MagickFalse)
      image->colormap[i].blue=stretch_map[
        ScaleQuantumToMap((const Quantum) image->colormap[i].blue)].blue;
    if (StretchesAlphaChannel(image,black,white) != MagickFalse)
      image->colormap[i].alpha=stretch_map[
        ScaleQuantumToMap((const Quantum) image->colormap[i].alpha)].alpha;
  }
}

static void StretchImageClass(Image *image,PixelPacket *stretch_map,
  const cl_float4 black,const cl_float4 white)
{
  if (IsStretchDirectClass(image) != MagickFalse)
    image->storage_class=DirectClass;
  if (image->storage_class == PseudoClass)
  {
    /*
       Stretch colormap.
       */
    StretchColormap(image,stretch_map,black,white);
  }
}

#define ContrastStretchImageTag  "ContrastStretch/Image"
#define MaxRange(color)  ((cl_float) ScaleQuantumToMap((Quantum) (color)))

typedef struct _StretchLevelSearch
{
  const Image
    *image;

  const cl_uint4
    *histogram;

  const ContrastStretchPoints
    *points;
} StretchLevelSearch;

static void FindStretchLevels(const StretchLevelSearch *search,
  const ChannelType channel,cl_float *black,cl_float *white)
{
  cl_uint
    i;

  *black=0.0;
  *white=MaxRange(QuantumRange);
  if ((search->image->channel_mask & channel) != 0)
  {
    i=FindStretchBlackBin(search->histogram,search->points->black_point);
    *black=(cl_float) i;
    i=FindStretchWhiteBin(search->image,search->histogram,
      search->points->white_point);
    *white=(cl_float) i;
  }
}

typedef struct _StretchChannelLevels
{
  MagickBooleanType
    enabled;

  cl_float
    black,
    white;
} StretchChannelLevels;

static void SetStretchMapValue(const StretchChannelLevels *levels,
  const cl_uint i,unsigned int *value)
{
  const cl_float
    black = levels->black,
    white = levels->white;

  if (levels->enabled == MagickFalse)
    return;
  if (i < (cl_uint) black)
    *value=0;
  else
    if (i > (cl_uint) white)
      *value=(unsigned int) QuantumRange;
    else
      if (black != white)
        *value=(unsigned int) ScaleMapToQuantum(
          (MagickRealType) (MaxMap*(i-black)/(white-black)));
}

static StretchChannelLevels GetStretchChannelLevels(const Image *image,
  const ChannelType channel,const cl_float black,const cl_float white)
{
  StretchChannelLevels
    levels;

  levels.enabled=((image->channel_mask & channel) != 0) ? MagickTrue :
    MagickFalse;
  levels.black=black;
  levels.white=white;
  return(levels);
}

static void BuildStretchMap(const Image *image,PixelPacket *stretch_map,
  const cl_float4 black,const cl_float4 white)
{
  const StretchChannelLevels
    red = GetStretchChannelLevels(image,RedChannel,black.x,white.x),
    green = GetStretchChannelLevels(image,GreenChannel,black.y,white.y),
    blue = GetStretchChannelLevels(image,BlueChannel,black.z,white.z),
    alpha = GetStretchChannelLevels(image,AlphaChannel,black.w,white.w);

  cl_uint
    i;

  (void) memset(stretch_map,0,(MaxMap+1)*sizeof(*stretch_map));
  for (i=0; i <= (ssize_t) MaxMap; i++)
  {
    SetStretchMapValue(&red,i,&stretch_map[i].red);
    SetStretchMapValue(&green,i,&stretch_map[i].green);
    SetStretchMapValue(&blue,i,&stretch_map[i].blue);
    SetStretchMapValue(&alpha,i,&stretch_map[i].alpha);
  }
}

typedef struct _StretchResources
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  CacheView
    *image_view;

  void
    *inputPixels;

  cl_mem
    imageBuffer,
    histogramBuffer,
    stretchMapBuffer;

  cl_kernel
    histogramKernel,
    stretchKernel;

  cl_uint4
    *histogram;

  PixelPacket
    *stretch_map;

  cl_float4
    black,
    white;

  MagickBooleanType
    outputReady;
} StretchResources;

static void ReleaseStretchResources(StretchResources *r)
{
  r->image_view=DestroyCacheView(r->image_view);

  if (r->imageBuffer!=NULL)
    r->clEnv->library->clReleaseMemObject(r->imageBuffer);

  if (r->stretchMapBuffer!=NULL)
    r->clEnv->library->clReleaseMemObject(r->stretchMapBuffer);
  if (r->stretch_map!=NULL)
    r->stretch_map=(PixelPacket *) RelinquishMagickMemory(r->stretch_map);
  if (r->histogramBuffer!=NULL)
    r->clEnv->library->clReleaseMemObject(r->histogramBuffer);
  if (r->histogram!=NULL)
    r->histogram=(cl_uint4 *) RelinquishMagickMemory(r->histogram);
  ReleaseKernel(r->histogramKernel);
  ReleaseKernel(r->stretchKernel);
  ReleaseAccelerateQueue(r->device,r->queue);
}

static MagickBooleanType AcquireStretchQueue(StretchResources *r)
{
  r->device=RequestOpenCLDevice(r->clEnv);
  if (r->device == (MagickCLDevice) NULL)
    return(MagickFalse);
  r->queue=AcquireOpenCLCommandQueue(r->device);
  if (r->queue == (cl_command_queue) NULL)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType AcquireStretchHistogram(StretchResources *r)
{
  r->histogram=(cl_uint4 *) AcquireQuantumMemory(MaxMap+1UL,
    sizeof(*r->histogram));

  if (r->histogram == (cl_uint4 *) NULL)
    return(MagickFalse);

  /* reset histogram */
  (void) memset(r->histogram,0,(MaxMap+1)*sizeof(*r->histogram));
  return(MagickTrue);
}

static MagickBooleanType AcquireStretchInput(Image *image,StretchResources *r,
  ExceptionInfo *exception)
{
  cl_int
    clStatus;

  cl_mem_flags
    mem_flags;

  size_t
    length;

  r->image_view=AcquireAuthenticCacheView(image,exception);
  r->inputPixels=GetCacheViewAuthenticPixels(r->image_view,0,0,image->columns,image->rows,exception);

  if (r->inputPixels == (void *) NULL)
  {
    (void) OpenCLThrowMagickException(r->device,exception,GetMagickModule(),CacheWarning,"UnableToReadPixelCache.","`%s'",image->filename);
    return(MagickFalse);
  }
  /* If the host pointer is aligned to the size of CLPixelPacket,
     then use the host buffer directly from the GPU; otherwise,
     create a buffer on the GPU and copy the data over */
  mem_flags=GetInputBufferFlags(r->inputPixels);
  /* create a CL buffer from image pixel buffer */
  length = image->columns * image->rows;
  r->imageBuffer = r->clEnv->library->clCreateBuffer(r->device->context, mem_flags, length * sizeof(CLPixelPacket), (void*)r->inputPixels, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(r->device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType CreateStretchHistogramBuffer(StretchResources *r,
  ExceptionInfo *exception)
{
  cl_int
    clStatus;

  cl_mem_flags
    mem_flags;

  size_t
    length;

  void
    *hostPtr;

  /* If the host pointer is aligned to the size of cl_uint,
     then use the host buffer directly from the GPU; otherwise,
     create a buffer on the GPU and copy the data over */
  if (ALIGNED(r->histogram,cl_uint4))
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR;
    hostPtr = r->histogram;
  }
  else
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR;
    hostPtr = r->histogram;
  }
  /* create a CL buffer for histogram  */
  length = (MaxMap+1);
  r->histogramBuffer = r->clEnv->library->clCreateBuffer(r->device->context, mem_flags, length * sizeof(cl_uint4), hostPtr, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(r->device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType ReadStretchHistogram(StretchResources *r,
  ExceptionInfo *exception)
{
  cl_int
    clStatus;

  size_t
    length;

  /* read from the kernel output */
  if (ALIGNED(r->histogram,cl_uint4))
  {
    length = (MaxMap+1);
    r->clEnv->library->clEnqueueMapBuffer(r->queue, r->histogramBuffer, CL_TRUE, CL_MAP_READ|CL_MAP_WRITE, 0, length * sizeof(cl_uint4), 0, NULL, NULL, &clStatus);
  }
  else
  {
    length = (MaxMap+1);
    clStatus = r->clEnv->library->clEnqueueReadBuffer(r->queue, r->histogramBuffer, CL_TRUE, 0, length * sizeof(cl_uint4), r->histogram, 0, NULL, NULL);
  }
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(r->device,exception, GetMagickModule(), ResourceLimitWarning, "Reading output image from CL buffer failed.", ".");
    return(MagickFalse);
  }

  /* unmap, don't block gpu to use this buffer again.  */
  if (ALIGNED(r->histogram,cl_uint4))
    clStatus = r->clEnv->library->clEnqueueUnmapMemObject(r->queue, r->histogramBuffer, r->histogram, 0, NULL, NULL);
  if ((ALIGNED(r->histogram,cl_uint4)) && (clStatus != CL_SUCCESS))
  {
    (void) OpenCLThrowMagickException(r->device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clEnqueueUnmapMemObject failed.", ".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType RunStretchHistogramPass(Image *image,
  StretchResources *r,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=MagickTrue;


  /*
    Form histogram.
  */
  /* Create and initialize OpenCL buffers. */
  /* inputPixels = AcquirePixelCachePixels(image, &length, exception); */
  /* assume this  will get a writable image */
  if (AcquireStretchInput(image,r,exception) == MagickFalse)
    return(MagickFalse);

  if (CreateStretchHistogramBuffer(r,exception) == MagickFalse)
    return(MagickFalse);

  {
    const HistogramKernelLaunch
      launch = { r->clEnv, r->device, r->queue, r->imageBuffer,
        r->histogramBuffer };

    status = LaunchHistogramKernel(&launch, image, image->channel_mask,
      exception);
  }
  if (status == MagickFalse)
    return(MagickFalse);

  if (ReadStretchHistogram(r,exception) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType CreateStretchMapBuffer(StretchResources *r,
  ExceptionInfo *exception)
{
  cl_int
    clStatus;

  cl_mem_flags
    mem_flags;

  size_t
    length;

  void
    *hostPtr;

  /* Create and initialize OpenCL buffers. */
  if (ALIGNED(r->stretch_map, PixelPacket))
  {
    mem_flags = CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR;
    hostPtr = r->stretch_map;
  }
  else
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR;
    hostPtr = r->stretch_map;
  }
  /* create a CL buffer for stretch_map  */
  length = (MaxMap+1);
  r->stretchMapBuffer = r->clEnv->library->clCreateBuffer(r->device->context, mem_flags, length * sizeof(PixelPacket), hostPtr, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(r->device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType ComputeContrastStretchImage(Image *image,
  MagickCLEnv clEnv,const ContrastStretchPoints *points,
  ExceptionInfo *exception)
{
  StretchResources
    r;

  cl_int
    channel_mask=get32BitChannelValue(image->channel_mask),
    clStatus;

  cl_event
    event;

  cl_uint
    i;

  size_t
    global_work_size[2],
    length;

  if (clEnv == (MagickCLEnv) NULL)
    return(MagickFalse);

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  
  r.clEnv=clEnv;
  r.queue=NULL;
  r.image_view=NULL;
  r.histogram=NULL;
  r.stretch_map=NULL;
  r.inputPixels=NULL;
  r.imageBuffer=NULL;
  r.histogramBuffer=NULL;
  r.stretchMapBuffer=NULL;
  r.histogramKernel=NULL;
  r.stretchKernel=NULL;
  r.outputReady=MagickFalse;

  /*
    Initialize opencl environment.
  */
  if (AcquireStretchQueue(&r) == MagickFalse)
    goto cleanup;

  /*
    Allocate and initialize histogram arrays.
  */
  if (AcquireStretchHistogram(&r) == MagickFalse)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed", image->filename);

  /*
  if (IsGrayImage(image,exception) != MagickFalse)
    (void) SetImageColorspace(image,GRAYColorspace);
  */

  if (RunStretchHistogramPass(image,&r,exception) == MagickFalse)
    goto cleanup;

  /* recreate input buffer later, in case image updated */
#include "MagickCore/accelerate-recreate-release-private.h"

  /* CPU stuff */
  /*
     Find the histogram boundaries by locating the black/white levels.
  */
  {
    const StretchLevelSearch
      search = { image, r.histogram, points };

    FindStretchLevels(&search,RedChannel,&r.black.x,&r.white.x);
    FindStretchLevels(&search,GreenChannel,&r.black.y,&r.white.y);
    FindStretchLevels(&search,BlueChannel,&r.black.z,&r.white.z);
    FindStretchLevels(&search,AlphaChannel,&r.black.w,&r.white.w);
  }

  r.stretch_map=(PixelPacket *) AcquireQuantumMemory(MaxMap+1UL,
    sizeof(*r.stretch_map));

  if (r.stretch_map == (PixelPacket *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);

  /*
    Stretch the histogram to create the stretched image mapping.
  */
  BuildStretchMap(image,r.stretch_map,r.black,r.white);

  /*
    Stretch the image.
  */
  StretchImageClass(image,r.stretch_map,r.black,r.white);

  /*
    Stretch image.
  */


  /* GPU can work on this again, image and equalize map as input
    image:        uchar4 (CLPixelPacket)
    stretch_map:  uchar4 (PixelPacket)
    black, white: float4 (FloatPixelPacket) */

#include "MagickCore/accelerate-recreate-buffer-private.h"

  if (CreateStretchMapBuffer(&r,exception) == MagickFalse)
    goto cleanup;

  /* get the OpenCL kernel */
  r.stretchKernel = AcquireOpenCLKernel(r.device,"ContrastStretch");
  if (r.stretchKernel == NULL)
  {
    (void) OpenCLThrowMagickException(r.device,exception, GetMagickModule(), ResourceLimitWarning, "AcquireOpenCLKernel failed.", ".");
    goto cleanup;
  }

  /* set the kernel arguments */
  i=0;
  clStatus=clEnv->library->clSetKernelArg(r.stretchKernel,i++,sizeof(cl_mem),(void *)&r.imageBuffer);
  clStatus|=clEnv->library->clSetKernelArg(r.stretchKernel,i++,sizeof(cl_int),&channel_mask);
  clStatus|=clEnv->library->clSetKernelArg(r.stretchKernel,i++,sizeof(cl_mem),(void *)&r.stretchMapBuffer);
  clStatus|=clEnv->library->clSetKernelArg(r.stretchKernel,i++,sizeof(cl_float4),&r.white);
  clStatus|=clEnv->library->clSetKernelArg(r.stretchKernel,i++,sizeof(cl_float4),&r.black);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(r.device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
    goto cleanup;
  }

  /* launch the kernel */
  global_work_size[0] = image->columns;
  global_work_size[1] = image->rows;

  clStatus = clEnv->library->clEnqueueNDRangeKernel(r.queue, r.stretchKernel, 2, NULL, global_work_size, NULL, 0, NULL, &event);

  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(r.device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clEnqueueNDRangeKernel failed.", ".");
    goto cleanup;
  }
  RecordProfileData(r.device,r.stretchKernel,event);

  /* read the data back */
  if (ALIGNED(r.inputPixels,CLPixelPacket))
  {
    length = image->columns * image->rows;
    clEnv->library->clEnqueueMapBuffer(r.queue, r.imageBuffer, CL_TRUE, CL_MAP_READ|CL_MAP_WRITE, 0, length * sizeof(CLPixelPacket), 0, NULL, NULL, &clStatus);
  }
  else
  {
    length = image->columns * image->rows;
    clStatus = clEnv->library->clEnqueueReadBuffer(r.queue, r.imageBuffer, CL_TRUE, 0, length * sizeof(CLPixelPacket), r.inputPixels, 0, NULL, NULL);
  }
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(r.device,exception, GetMagickModule(), ResourceLimitWarning, "Reading output image from CL buffer failed.", ".");
    goto cleanup;
  }

  r.outputReady=SyncCacheViewAuthenticPixels(r.image_view,exception);

cleanup:

  ReleaseStretchResources(&r);

  return(r.outputReady);
}

MagickPrivate MagickBooleanType AccelerateContrastStretchImage(
  Image *image,const double black_point,const double white_point,
  ExceptionInfo *exception)
{
  const ContrastStretchPoints
    points = { black_point, white_point };

  return(ComputeContrastStretchImage(image,getAccelerateEnvironment(image,
    HistogramAccelerateCheck,__func__,exception),&points,exception));
}
#endif /* MAGICKCORE_OPENCL_SUPPORT */
