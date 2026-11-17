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
%        H   H  IIIII  SSSSS  TTTTT   OOO    GGGG  RRRR    AAA   M   M        %
%        H   H    I    SS       T    O   O  G      R   R  A   A  MM MM        %
%        HHHHH    I     SSS     T    O   O  G  GG  RRRR   AAAAA  M M M        %
%        H   H    I       SS    T    O   O  G   G  R R    A   A  M   M        %
%        H   H  IIIII  SSSSS    T     OOO    GGG   R  R   A   A  M   M        %
%                                                                             %
%                                                                             %
%               MagickCore OpenCL Accelerated Histogram Methods               %
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

static MagickBooleanType LaunchHistogramKernel(MagickCLEnv clEnv,
  MagickCLDevice device,cl_command_queue queue,cl_mem imageBuffer,
  cl_mem histogramBuffer,Image *image,const ChannelType channel,
  ExceptionInfo *exception)
{
  MagickBooleanType
    outputReady;

  cl_int
    channel_mask=get32BitChannelValue(channel),
    clStatus;

  cl_kernel
    histogramKernel;

  cl_event
    event;

  cl_uint
    colorspace,
    i,
    method;

  size_t
    global_work_size[2];

  histogramKernel=NULL;
  outputReady=MagickFalse;

  colorspace = image->colorspace;
  method = image->intensity;

  /* get the OpenCL kernel */
  histogramKernel = AcquireOpenCLKernel(device,"Histogram");
  if (histogramKernel == NULL)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "AcquireOpenCLKernel failed.", ".");
    goto cleanup;
  }

  /* set the kernel arguments */
  i=0;
  clStatus=clEnv->library->clSetKernelArg(histogramKernel,i++,sizeof(cl_mem),(void *)&imageBuffer);
  clStatus|=clEnv->library->clSetKernelArg(histogramKernel,i++,sizeof(cl_int),&channel_mask);
  clStatus|=clEnv->library->clSetKernelArg(histogramKernel,i++,sizeof(cl_uint),&colorspace);
  clStatus|=clEnv->library->clSetKernelArg(histogramKernel,i++,sizeof(cl_uint),&method);
  clStatus|=clEnv->library->clSetKernelArg(histogramKernel,i++,sizeof(cl_mem),(void *)&histogramBuffer);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
    goto cleanup;
  }

  /* launch the kernel */
  global_work_size[0] = image->columns;
  global_work_size[1] = image->rows;

  clStatus = clEnv->library->clEnqueueNDRangeKernel(queue, histogramKernel, 2, NULL, global_work_size, NULL, 0, NULL, &event);

  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clEnqueueNDRangeKernel failed.", ".");
    goto cleanup;
  }
  RecordProfileData(device,histogramKernel,event);

  outputReady = MagickTrue;

cleanup:

  if (histogramKernel!=NULL)
    ReleaseOpenCLKernel(histogramKernel);

  return(outputReady);
}

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

static MagickBooleanType ComputeContrastStretchImage(Image *image,
  MagickCLEnv clEnv,const double black_point,const double white_point,
  ExceptionInfo *exception)
{
#define ContrastStretchImageTag  "ContrastStretch/Image"
#define MaxRange(color)  ((cl_float) ScaleQuantumToMap((Quantum) (color)))

  CacheView
    *image_view;

  cl_command_queue
    queue;

  cl_int
    channel_mask=get32BitChannelValue(image->channel_mask),
    clStatus;

  cl_mem_flags
    mem_flags;

  cl_mem
    histogramBuffer,
    imageBuffer,
    stretchMapBuffer;

  cl_kernel
    histogramKernel,
    stretchKernel;

  cl_event
    event;

  cl_uint
    i;

  cl_uint4
    *histogram;

  cl_float4
    black,
    white;

  MagickBooleanType
    outputReady,
    status;

  MagickCLDevice
    device;

  PixelPacket
    *stretch_map;

  size_t
    global_work_size[2],
    length;

  void
    *hostPtr,
    *inputPixels;

  if (clEnv == (MagickCLEnv) NULL)
    return(MagickFalse);

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  
  queue=NULL;
  image_view=NULL;
  histogram=NULL;
  stretch_map=NULL;
  inputPixels=NULL;
  imageBuffer=NULL;
  histogramBuffer=NULL;
  stretchMapBuffer=NULL;
  histogramKernel=NULL;
  stretchKernel=NULL;
  outputReady=MagickFalse;

  /*
    Initialize opencl environment.
  */
  device=RequestOpenCLDevice(clEnv);
  if (device == (MagickCLDevice) NULL)
    goto cleanup;
  queue=AcquireOpenCLCommandQueue(device);
  if (queue == (cl_command_queue) NULL)
    goto cleanup;

  /*
    Allocate and initialize histogram arrays.
  */
  histogram=(cl_uint4 *) AcquireQuantumMemory(MaxMap+1UL, sizeof(*histogram));

  if (histogram == (cl_uint4 *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed", image->filename);

  /* reset histogram */
  (void) memset(histogram,0,(MaxMap+1)*sizeof(*histogram));

  /*
  if (IsGrayImage(image,exception) != MagickFalse)
    (void) SetImageColorspace(image,GRAYColorspace);
  */

  status=MagickTrue;


  /*
    Form histogram.
  */
  /* Create and initialize OpenCL buffers. */
  /* inputPixels = AcquirePixelCachePixels(image, &length, exception); */
  /* assume this  will get a writable image */
  image_view=AcquireAuthenticCacheView(image,exception);
  inputPixels=GetCacheViewAuthenticPixels(image_view,0,0,image->columns,image->rows,exception);

  if (inputPixels == (void *) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),CacheWarning,"UnableToReadPixelCache.","`%s'",image->filename);
    goto cleanup;
  }
  /* If the host pointer is aligned to the size of CLPixelPacket,
     then use the host buffer directly from the GPU; otherwise,
     create a buffer on the GPU and copy the data over */
  mem_flags=GetInputBufferFlags(inputPixels);
  /* create a CL buffer from image pixel buffer */
  length = image->columns * image->rows;
  imageBuffer = clEnv->library->clCreateBuffer(device->context, mem_flags, length * sizeof(CLPixelPacket), (void*)inputPixels, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    goto cleanup;
  }

  /* If the host pointer is aligned to the size of cl_uint,
     then use the host buffer directly from the GPU; otherwise,
     create a buffer on the GPU and copy the data over */
  if (ALIGNED(histogram,cl_uint4))
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR;
    hostPtr = histogram;
  }
  else
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR;
    hostPtr = histogram;
  }
  /* create a CL buffer for histogram  */
  length = (MaxMap+1);
  histogramBuffer = clEnv->library->clCreateBuffer(device->context, mem_flags, length * sizeof(cl_uint4), hostPtr, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    goto cleanup;
  }

  status = LaunchHistogramKernel(clEnv, device, queue, imageBuffer, histogramBuffer, image, image->channel_mask,exception);
  if (status == MagickFalse)
    goto cleanup;

  /* read from the kernel output */
  if (ALIGNED(histogram,cl_uint4))
  {
    length = (MaxMap+1);
    clEnv->library->clEnqueueMapBuffer(queue, histogramBuffer, CL_TRUE, CL_MAP_READ|CL_MAP_WRITE, 0, length * sizeof(cl_uint4), 0, NULL, NULL, &clStatus);
  }
  else
  {
    length = (MaxMap+1);
    clStatus = clEnv->library->clEnqueueReadBuffer(queue, histogramBuffer, CL_TRUE, 0, length * sizeof(cl_uint4), histogram, 0, NULL, NULL);
  }
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "Reading output image from CL buffer failed.", ".");
    goto cleanup;
  }

  /* unmap, don't block gpu to use this buffer again.  */
  if (ALIGNED(histogram,cl_uint4))
    clStatus = clEnv->library->clEnqueueUnmapMemObject(queue, histogramBuffer, histogram, 0, NULL, NULL);
  if ((ALIGNED(histogram,cl_uint4)) && (clStatus != CL_SUCCESS))
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clEnqueueUnmapMemObject failed.", ".");
    goto cleanup;
  }

  /* recreate input buffer later, in case image updated */
#ifdef RECREATEBUFFER
  if (imageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(imageBuffer);
#endif

  /* CPU stuff */
  /*
     Find the histogram boundaries by locating the black/white levels.
  */
  black.x=0.0;
  white.x=MaxRange(QuantumRange);
  if ((image->channel_mask & RedChannel) != 0)
  {
    i=FindStretchBlackBin(histogram,black_point);
    black.x=(cl_float) i;
    i=FindStretchWhiteBin(image,histogram,white_point);
    white.x=(cl_float) i;
  }
  black.y=0.0;
  white.y=MaxRange(QuantumRange);
  if ((image->channel_mask & GreenChannel) != 0)
  {
    i=FindStretchBlackBin(histogram,black_point);
    black.y=(cl_float) i;
    i=FindStretchWhiteBin(image,histogram,white_point);
    white.y=(cl_float) i;
  }
  black.z=0.0;
  white.z=MaxRange(QuantumRange);
  if ((image->channel_mask & BlueChannel) != 0)
  {
    i=FindStretchBlackBin(histogram,black_point);
    black.z=(cl_float) i;
    i=FindStretchWhiteBin(image,histogram,white_point);
    white.z=(cl_float) i;
  }
  black.w=0.0;
  white.w=MaxRange(QuantumRange);
  if ((image->channel_mask & AlphaChannel) != 0)
  {
    i=FindStretchBlackBin(histogram,black_point);
    black.w=(cl_float) i;
    i=FindStretchWhiteBin(image,histogram,white_point);
    white.w=(cl_float) i;
  }

  stretch_map=(PixelPacket *) AcquireQuantumMemory(MaxMap+1UL,
    sizeof(*stretch_map));

  if (stretch_map == (PixelPacket *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);

  /*
    Stretch the histogram to create the stretched image mapping.
  */
  (void) memset(stretch_map,0,(MaxMap+1)*sizeof(*stretch_map));
  for (i=0; i <= (ssize_t) MaxMap; i++)
  {
    if (((image->channel_mask & RedChannel) != 0) &&
        (i < (cl_uint) black.x))
      stretch_map[i].red=0;
    else
      if (((image->channel_mask & RedChannel) != 0) &&
          (i > (cl_uint) white.x))
        stretch_map[i].red=(unsigned int) QuantumRange;
      else
        if (((image->channel_mask & RedChannel) != 0) &&
            (black.x != white.x))
          stretch_map[i].red=(unsigned int) ScaleMapToQuantum(
            (MagickRealType) (MaxMap*(i-black.x)/(white.x-black.x)));
    if (((image->channel_mask & GreenChannel) != 0) &&
        (i < (cl_uint) black.y))
      stretch_map[i].green=0;
    else
      if (((image->channel_mask & GreenChannel) != 0) &&
          (i > (cl_uint) white.y))
        stretch_map[i].green=(unsigned int) QuantumRange;
      else
        if (((image->channel_mask & GreenChannel) != 0) &&
            (black.y != white.y))
          stretch_map[i].green=(unsigned int) ScaleMapToQuantum(
            (MagickRealType) (MaxMap*(i-black.y)/(white.y-black.y)));
    if (((image->channel_mask & BlueChannel) != 0) &&
        (i < (cl_uint) black.z))
      stretch_map[i].blue=0;
    else
      if (((image->channel_mask & BlueChannel) != 0) &&
          (i > (cl_uint) white.z))
        stretch_map[i].blue=(unsigned int) QuantumRange;
      else
        if (((image->channel_mask & BlueChannel) != 0) &&
            (black.z != white.z))
          stretch_map[i].blue=(unsigned int) ScaleMapToQuantum(
            (MagickRealType) (MaxMap*(i-black.z)/(white.z-black.z)));
    if (((image->channel_mask & AlphaChannel) != 0) &&
        (i < (cl_uint) black.w))
      stretch_map[i].alpha=0;
    else
      if (((image->channel_mask & AlphaChannel) != 0) &&
          (i > (cl_uint) white.w))
        stretch_map[i].alpha=(unsigned int) QuantumRange;
      else
        if (((image->channel_mask & AlphaChannel) != 0) &&
            (black.w != white.w))
          stretch_map[i].alpha=(unsigned int) ScaleMapToQuantum(
            (MagickRealType) (MaxMap*(i-black.w)/(white.w-black.w)));
  }

  /*
    Stretch the image.
  */
  StretchImageClass(image,stretch_map,black,white);

  /*
    Stretch image.
  */


  /* GPU can work on this again, image and equalize map as input
    image:        uchar4 (CLPixelPacket)
    stretch_map:  uchar4 (PixelPacket)
    black, white: float4 (FloatPixelPacket) */

#ifdef RECREATEBUFFER
  /* If the host pointer is aligned to the size of CLPixelPacket,
     then use the host buffer directly from the GPU; otherwise,
     create a buffer on the GPU and copy the data over */
  if (ALIGNED(inputPixels,CLPixelPacket))
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR;
  }
  else
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR;
  }
  /* create a CL buffer from image pixel buffer */
  length = image->columns * image->rows;
  imageBuffer = clEnv->library->clCreateBuffer(context, mem_flags, length * sizeof(CLPixelPacket), (void*)inputPixels, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    goto cleanup;
  }
#endif

  /* Create and initialize OpenCL buffers. */
  if (ALIGNED(stretch_map, PixelPacket))
  {
    mem_flags = CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR;
    hostPtr = stretch_map;
  }
  else
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR;
    hostPtr = stretch_map;
  }
  /* create a CL buffer for stretch_map  */
  length = (MaxMap+1);
  stretchMapBuffer = clEnv->library->clCreateBuffer(device->context, mem_flags, length * sizeof(PixelPacket), hostPtr, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    goto cleanup;
  }

  /* get the OpenCL kernel */
  stretchKernel = AcquireOpenCLKernel(device,"ContrastStretch");
  if (stretchKernel == NULL)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "AcquireOpenCLKernel failed.", ".");
    goto cleanup;
  }

  /* set the kernel arguments */
  i=0;
  clStatus=clEnv->library->clSetKernelArg(stretchKernel,i++,sizeof(cl_mem),(void *)&imageBuffer);
  clStatus|=clEnv->library->clSetKernelArg(stretchKernel,i++,sizeof(cl_int),&channel_mask);
  clStatus|=clEnv->library->clSetKernelArg(stretchKernel,i++,sizeof(cl_mem),(void *)&stretchMapBuffer);
  clStatus|=clEnv->library->clSetKernelArg(stretchKernel,i++,sizeof(cl_float4),&white);
  clStatus|=clEnv->library->clSetKernelArg(stretchKernel,i++,sizeof(cl_float4),&black);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
    goto cleanup;
  }

  /* launch the kernel */
  global_work_size[0] = image->columns;
  global_work_size[1] = image->rows;

  clStatus = clEnv->library->clEnqueueNDRangeKernel(queue, stretchKernel, 2, NULL, global_work_size, NULL, 0, NULL, &event);

  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clEnqueueNDRangeKernel failed.", ".");
    goto cleanup;
  }
  RecordProfileData(device,stretchKernel,event);

  /* read the data back */
  if (ALIGNED(inputPixels,CLPixelPacket))
  {
    length = image->columns * image->rows;
    clEnv->library->clEnqueueMapBuffer(queue, imageBuffer, CL_TRUE, CL_MAP_READ|CL_MAP_WRITE, 0, length * sizeof(CLPixelPacket), 0, NULL, NULL, &clStatus);
  }
  else
  {
    length = image->columns * image->rows;
    clStatus = clEnv->library->clEnqueueReadBuffer(queue, imageBuffer, CL_TRUE, 0, length * sizeof(CLPixelPacket), inputPixels, 0, NULL, NULL);
  }
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "Reading output image from CL buffer failed.", ".");
    goto cleanup;
  }

  outputReady=SyncCacheViewAuthenticPixels(image_view,exception);

cleanup:

  image_view=DestroyCacheView(image_view);

  if (imageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(imageBuffer);

  if (stretchMapBuffer!=NULL)
    clEnv->library->clReleaseMemObject(stretchMapBuffer);
  if (stretch_map!=NULL)
    stretch_map=(PixelPacket *) RelinquishMagickMemory(stretch_map);
  if (histogramBuffer!=NULL)
    clEnv->library->clReleaseMemObject(histogramBuffer);
  if (histogram!=NULL)
    histogram=(cl_uint4 *) RelinquishMagickMemory(histogram);
  if (histogramKernel!=NULL)
    ReleaseOpenCLKernel(histogramKernel);
  if (stretchKernel!=NULL)
    ReleaseOpenCLKernel(stretchKernel);
  if (queue != NULL)
    ReleaseOpenCLCommandQueue(device,queue);
  if (device != NULL)
    ReleaseOpenCLDevice(device);

  return(outputReady);
}

MagickPrivate MagickBooleanType AccelerateContrastStretchImage(
  Image *image,const double black_point,const double white_point,
  ExceptionInfo *exception)
{
  return(ComputeContrastStretchImage(image,getAccelerateEnvironment(image,
    HistogramAccelerateCheck,__func__,exception),black_point,white_point,
    exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A c c e l e r a t e E q u a l i z e I m a g e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

typedef struct _HistogramBufferInfo
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  cl_mem
    histogramBuffer;

  cl_uint4
    *histogram;
} HistogramBufferInfo;

static MagickBooleanType ReadHistogramBuffer(const HistogramBufferInfo *info,
  ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = info->clEnv;

  const MagickCLDevice
    device = info->device;

  const cl_command_queue
    queue = info->queue;

  const cl_mem
    histogramBuffer = info->histogramBuffer;

  cl_uint4
    *const histogram = info->histogram;

  cl_int
    clStatus;

  size_t
    length;

  /* read from the kernel output */
  if (ALIGNED(histogram,cl_uint4))
  {
    length = (MaxMap+1);
    clEnv->library->clEnqueueMapBuffer(queue, histogramBuffer, CL_TRUE, CL_MAP_READ|CL_MAP_WRITE, 0, length * sizeof(cl_uint4), 0, NULL, NULL, &clStatus);
  }
  else
  {
    length = (MaxMap+1);
    clStatus = clEnv->library->clEnqueueReadBuffer(queue, histogramBuffer, CL_TRUE, 0, length * sizeof(cl_uint4), histogram, 0, NULL, NULL);
  }
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "Reading output image from CL buffer failed.", ".");
    return(MagickFalse);
  }

  /* unmap, don't block gpu to use this buffer again.  */
  if (ALIGNED(histogram,cl_uint4))
  {
    clStatus = clEnv->library->clEnqueueUnmapMemObject(queue, histogramBuffer, histogram, 0, NULL, NULL);
    if (clStatus != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clEnqueueUnmapMemObject failed.", ".");
      return(MagickFalse);
    }
  }
  return(MagickTrue);
}

typedef struct _EqualizeLevels
{
  cl_float4
    white,
    black;
} EqualizeLevels;

static void EqualizeColormapSync(Image *image,PixelPacket *equalize_map,
  const cl_uint i,const EqualizeLevels *levels)
{
  const cl_float4
    white = levels->white,
    black = levels->black;

  if (white.x != black.x)
  {
    image->colormap[i].red=equalize_map[
      ScaleQuantumToMap((const Quantum) image->colormap[i].red)].red;
    image->colormap[i].green=equalize_map[
      ScaleQuantumToMap((const Quantum) image->colormap[i].green)].red;
    image->colormap[i].blue=equalize_map[
      ScaleQuantumToMap((const Quantum) image->colormap[i].blue)].red;
    image->colormap[i].alpha=equalize_map[
      ScaleQuantumToMap((const Quantum) image->colormap[i].alpha)].red;
  }
}

static void IntegrateEqualizeHistogram(const Image *image,
  const cl_uint4 *histogram,cl_float4 *map)
{
  cl_float4
    intensity;

  cl_uint
    i;

  (void) memset(&intensity,0,sizeof(intensity));
  for (i=0; i <= (ssize_t) MaxMap; i++)
  {
    if ((image->channel_mask & SyncChannels) != 0)
    {
      intensity.x+=histogram[i].s[2];
      map[i]=intensity;
      continue;
    }
    if ((image->channel_mask & RedChannel) != 0)
      intensity.x+=histogram[i].s[2];
    if ((image->channel_mask & GreenChannel) != 0)
      intensity.y+=histogram[i].s[1];
    if ((image->channel_mask & BlueChannel) != 0)
      intensity.z+=histogram[i].s[0];
    if ((image->channel_mask & AlphaChannel) != 0)
      intensity.w+=histogram[i].s[3];
    map[i]=intensity;
  }
}

static MagickBooleanType EqualizesRedChannel(const Image *image,
  const cl_float4 white,const cl_float4 black)
{
  return((((image->channel_mask & RedChannel) != 0) &&
    (white.x != black.x)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType EqualizesGreenChannel(const Image *image,
  const cl_float4 white,const cl_float4 black)
{
  return((((image->channel_mask & GreenChannel) != 0) &&
    (white.y != black.y)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType EqualizesBlueChannel(const Image *image,
  const cl_float4 white,const cl_float4 black)
{
  return((((image->channel_mask & BlueChannel) != 0) &&
    (white.z != black.z)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType EqualizesAlphaChannel(const Image *image,
  const cl_float4 white,const cl_float4 black)
{
  return((((image->channel_mask & AlphaChannel) != 0) &&
    (white.w != black.w)) ? MagickTrue : MagickFalse);
}

static void EqualizeColormap(Image *image,PixelPacket *equalize_map,
  const cl_float4 white,const cl_float4 black)
{
  cl_uint
    i;

  for (i=0; i < (cl_uint) image->colors; i++)
  {
    if ((image->channel_mask & SyncChannels) != 0)
    {
      EqualizeLevels
        levels;

      levels.white=white;
      levels.black=black;
      EqualizeColormapSync(image,equalize_map,i,&levels);
      continue;
    }
    if (EqualizesRedChannel(image,white,black) != MagickFalse)
      image->colormap[i].red=equalize_map[
        ScaleQuantumToMap((const Quantum) image->colormap[i].red)].red;
    if (EqualizesGreenChannel(image,white,black) != MagickFalse)
      image->colormap[i].green=equalize_map[
        ScaleQuantumToMap((const Quantum) image->colormap[i].green)].green;
    if (EqualizesBlueChannel(image,white,black) != MagickFalse)
      image->colormap[i].blue=equalize_map[
        ScaleQuantumToMap((const Quantum) image->colormap[i].blue)].blue;
    if (EqualizesAlphaChannel(image,white,black) != MagickFalse)
      image->colormap[i].alpha=equalize_map[
        ScaleQuantumToMap((const Quantum) image->colormap[i].alpha)].alpha;
  }
}

typedef struct _EqualizeMapInput
{
  Image
    *image;

  cl_float4
    *map,
    white,
    black;

  PixelPacket
    *equalize_map;
} EqualizeMapInput;

static void SetEqualizeMapEntry(const EqualizeMapInput *input,const cl_uint i)
{
  Image
    *const image = input->image;

  cl_float4
    *const map = input->map;

  const cl_float4
    white = input->white,
    black = input->black;

  PixelPacket
    *const equalize_map = input->equalize_map;

  if ((image->channel_mask & SyncChannels) != 0)
  {
    if (white.x != black.x)
      equalize_map[i].red=(unsigned int) ScaleMapToQuantum(
        (MagickRealType) ((MaxMap*(map[i].x-black.x))/(white.x-black.x)));
    return;
  }
  if (EqualizesRedChannel(image,white,black) != MagickFalse)
    equalize_map[i].red=(unsigned int) ScaleMapToQuantum(
      (MagickRealType) ((MaxMap*(map[i].x-black.x))/(white.x-black.x)));
  if (EqualizesGreenChannel(image,white,black) != MagickFalse)
    equalize_map[i].green=(unsigned int) ScaleMapToQuantum(
      (MagickRealType) ((MaxMap*(map[i].y-black.y))/(white.y-black.y)));
  if (EqualizesBlueChannel(image,white,black) != MagickFalse)
    equalize_map[i].blue=(unsigned int) ScaleMapToQuantum(
      (MagickRealType) ((MaxMap*(map[i].z-black.z))/(white.z-black.z)));
  if (EqualizesAlphaChannel(image,white,black) != MagickFalse)
    equalize_map[i].alpha=(unsigned int) ScaleMapToQuantum(
      (MagickRealType) ((MaxMap*(map[i].w-black.w))/(white.w-black.w)));
}

typedef struct _HistogramBufferSource
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_uint4
    *histogram;
} HistogramBufferSource;

static MagickBooleanType CreateHistogramBuffer(
  const HistogramBufferSource *source,cl_mem *histogramBuffer,
  ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = source->clEnv;

  const MagickCLDevice
    device = source->device;

  cl_uint4
    *const histogram = source->histogram;

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
  if (ALIGNED(histogram,cl_uint4))
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR;
    hostPtr = histogram;
  }
  else
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR;
    hostPtr = histogram;
  }
  /* create a CL buffer for histogram  */
  length = (MaxMap+1);
  *histogramBuffer = clEnv->library->clCreateBuffer(device->context, mem_flags, length * sizeof(cl_uint4), hostPtr, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _EqualizeMapSource
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  PixelPacket
    *equalize_map;
} EqualizeMapSource;

static MagickBooleanType CreateEqualizeMapBuffer(
  const EqualizeMapSource *source,cl_mem *equalizeMapBuffer,
  ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = source->clEnv;

  const MagickCLDevice
    device = source->device;

  PixelPacket
    *const equalize_map = source->equalize_map;

  cl_int
    clStatus;

  cl_mem_flags
    mem_flags;

  size_t
    length;

  void
    *hostPtr;

  /* Create and initialize OpenCL buffers. */
  if (ALIGNED(equalize_map, PixelPacket))
  {
    mem_flags = CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR;
    hostPtr = equalize_map;
  }
  else
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR;
    hostPtr = equalize_map;
  }
  /* create a CL buffer for equalize_map  */
  length = (MaxMap+1);
  *equalizeMapBuffer = clEnv->library->clCreateBuffer(device->context, mem_flags, length * sizeof(PixelPacket), hostPtr, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType ComputeEqualizeImage(Image *image,MagickCLEnv clEnv,
  ExceptionInfo *exception)
{
#define EqualizeImageTag  "Equalize/Image"

  CacheView
    *image_view;

  cl_command_queue
    queue;

  cl_int
    channel_mask=get32BitChannelValue(image->channel_mask),
    clStatus;

  cl_mem_flags
    mem_flags;

  cl_mem
    equalizeMapBuffer,
    histogramBuffer,
    imageBuffer;

  cl_kernel
    equalizeKernel,
    histogramKernel;

  cl_event
    event;

  cl_uint
    i;

  cl_uint4
    *histogram;

  cl_float4
    white,
    black,
    *map;

  HistogramBufferInfo
    histogram_info;

  EqualizeMapInput
    map_input;

  MagickBooleanType
    outputReady,
    status;

  MagickCLDevice
    device;

  PixelPacket
    *equalize_map;

  size_t
    global_work_size[2],
    length;

  void
    *inputPixels;

  if (clEnv == (MagickCLEnv) NULL)
    return(MagickFalse);

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);

  queue=NULL;
  image_view=NULL;
  map=NULL;
  histogram=NULL;
  equalize_map=NULL;
  inputPixels=NULL;
  imageBuffer=NULL;
  histogramBuffer=NULL;
  equalizeMapBuffer=NULL;
  histogramKernel=NULL;
  equalizeKernel=NULL;
  outputReady=MagickFalse;

  /*
   * initialize opencl env
   */
  device=RequestOpenCLDevice(clEnv);
  if (device == (MagickCLDevice) NULL)
    goto cleanup;
  queue=AcquireOpenCLCommandQueue(device);
  if (queue == (cl_command_queue) NULL)
    goto cleanup;

  /*
    Allocate and initialize histogram arrays.
  */
  histogram=(cl_uint4 *) AcquireQuantumMemory(MaxMap+1UL, sizeof(*histogram));
  if (histogram == (cl_uint4 *) NULL)
      ThrowBinaryException(ResourceLimitWarning,"MemoryAllocationFailed", image->filename);

  /* reset histogram */
  (void) memset(histogram,0,(MaxMap+1)*sizeof(*histogram));

  /* Create and initialize OpenCL buffers. */
  /* inputPixels = AcquirePixelCachePixels(image, &length, exception); */
  /* assume this  will get a writable image */
  image_view=AcquireAuthenticCacheView(image,exception);
  inputPixels=GetCacheViewAuthenticPixels(image_view,0,0,image->columns,image->rows,exception);

  if (inputPixels == (void *) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),CacheWarning,"UnableToReadPixelCache.","`%s'",image->filename);
    goto cleanup;
  }
  /* If the host pointer is aligned to the size of CLPixelPacket,
     then use the host buffer directly from the GPU; otherwise,
     create a buffer on the GPU and copy the data over */
  mem_flags=GetInputBufferFlags(inputPixels);
  /* create a CL buffer from image pixel buffer */
  length = image->columns * image->rows;
  imageBuffer = clEnv->library->clCreateBuffer(device->context, mem_flags, length * sizeof(CLPixelPacket), (void*)inputPixels, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    goto cleanup;
  }

  {
    const HistogramBufferSource
      source = { clEnv, device, histogram };

    if (CreateHistogramBuffer(&source,&histogramBuffer,exception) == MagickFalse)
      goto cleanup;
  }

  status = LaunchHistogramKernel(clEnv, device, queue, imageBuffer, histogramBuffer, image, image->channel_mask, exception);
  if (status == MagickFalse)
    goto cleanup;

  histogram_info.clEnv=clEnv;
  histogram_info.device=device;
  histogram_info.queue=queue;
  histogram_info.histogramBuffer=histogramBuffer;
  histogram_info.histogram=histogram;
  if (ReadHistogramBuffer(&histogram_info,exception) == MagickFalse)
    goto cleanup;

  /* recreate input buffer later, in case image updated */
#ifdef RECREATEBUFFER
  if (imageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(imageBuffer);
#endif

  /* CPU stuff */
  equalize_map=(PixelPacket *) AcquireQuantumMemory(MaxMap+1UL, sizeof(*equalize_map));
  if (equalize_map == (PixelPacket *) NULL)
    ThrowBinaryException(ResourceLimitWarning,"MemoryAllocationFailed", image->filename);

  map=(cl_float4 *) AcquireQuantumMemory(MaxMap+1UL,sizeof(*map));
  if (map == (cl_float4 *) NULL)
    ThrowBinaryException(ResourceLimitWarning,"MemoryAllocationFailed", image->filename);

  /*
    Integrate the histogram to get the equalization map.
  */
  IntegrateEqualizeHistogram(image,histogram,map);
  black=map[0];
  white=map[(int) MaxMap];
  (void) memset(equalize_map,0,(MaxMap+1)*sizeof(*equalize_map));
  map_input.image=image;
  map_input.map=map;
  map_input.white=white;
  map_input.black=black;
  map_input.equalize_map=equalize_map;
  for (i=0; i <= (ssize_t) MaxMap; i++)
    SetEqualizeMapEntry(&map_input,i);

  if (image->storage_class == PseudoClass)
  {
    /*
       Equalize colormap.
       */
    EqualizeColormap(image,equalize_map,white,black);
  }

  /*
    Equalize image.
  */

  /* GPU can work on this again, image and equalize map as input
    image:        uchar4 (CLPixelPacket)
    equalize_map: uchar4 (PixelPacket)
    black, white: float4 (FloatPixelPacket) */

#ifdef RECREATEBUFFER
  /* If the host pointer is aligned to the size of CLPixelPacket,
     then use the host buffer directly from the GPU; otherwise,
     create a buffer on the GPU and copy the data over */
  if (ALIGNED(inputPixels,CLPixelPacket))
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR;
  }
  else
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR;
  }
  /* create a CL buffer from image pixel buffer */
  length = image->columns * image->rows;
  imageBuffer = clEnv->library->clCreateBuffer(context, mem_flags, length * sizeof(CLPixelPacket), (void*)inputPixels, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    goto cleanup;
  }
#endif

  {
    const EqualizeMapSource
      source = { clEnv, device, equalize_map };

    if (CreateEqualizeMapBuffer(&source,&equalizeMapBuffer,exception) == MagickFalse)
      goto cleanup;
  }

  /* get the OpenCL kernel */
  equalizeKernel = AcquireOpenCLKernel(device,"Equalize");
  if (equalizeKernel == NULL)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "AcquireOpenCLKernel failed.", ".");
    goto cleanup;
  }

  /* set the kernel arguments */
  i=0;
  clStatus=clEnv->library->clSetKernelArg(equalizeKernel,i++,sizeof(cl_mem),(void *)&imageBuffer);
  clStatus|=clEnv->library->clSetKernelArg(equalizeKernel,i++,sizeof(cl_int),&channel_mask);
  clStatus|=clEnv->library->clSetKernelArg(equalizeKernel,i++,sizeof(cl_mem),(void *)&equalizeMapBuffer);
  clStatus|=clEnv->library->clSetKernelArg(equalizeKernel,i++,sizeof(cl_float4),&white);
  clStatus|=clEnv->library->clSetKernelArg(equalizeKernel,i++,sizeof(cl_float4),&black);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clSetKernelArg failed.", ".");
    goto cleanup;
  }

  /* launch the kernel */
  global_work_size[0] = image->columns;
  global_work_size[1] = image->rows;

  clStatus = clEnv->library->clEnqueueNDRangeKernel(queue, equalizeKernel, 2, NULL, global_work_size, NULL, 0, NULL, &event);

  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clEnqueueNDRangeKernel failed.", ".");
    goto cleanup;
  }
  RecordProfileData(device,equalizeKernel,event);

  /* read the data back */
  if (ALIGNED(inputPixels,CLPixelPacket))
  {
    length = image->columns * image->rows;
    clEnv->library->clEnqueueMapBuffer(queue, imageBuffer, CL_TRUE, CL_MAP_READ|CL_MAP_WRITE, 0, length * sizeof(CLPixelPacket), 0, NULL, NULL, &clStatus);
  }
  else
  {
    length = image->columns * image->rows;
    clStatus = clEnv->library->clEnqueueReadBuffer(queue, imageBuffer, CL_TRUE, 0, length * sizeof(CLPixelPacket), inputPixels, 0, NULL, NULL);
  }
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "Reading output image from CL buffer failed.", ".");
    goto cleanup;
  }

  outputReady=SyncCacheViewAuthenticPixels(image_view,exception);

cleanup:

  image_view=DestroyCacheView(image_view);

  if (imageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(imageBuffer);
  if (map!=NULL)
    map=(cl_float4 *) RelinquishMagickMemory(map);
  if (equalizeMapBuffer!=NULL)
    clEnv->library->clReleaseMemObject(equalizeMapBuffer);
  if (equalize_map!=NULL)
    equalize_map=(PixelPacket *) RelinquishMagickMemory(equalize_map);
  if (histogramBuffer!=NULL)
    clEnv->library->clReleaseMemObject(histogramBuffer);
  if (histogram!=NULL)
    histogram=(cl_uint4 *) RelinquishMagickMemory(histogram);
  if (histogramKernel!=NULL)
    ReleaseOpenCLKernel(histogramKernel);
  if (equalizeKernel!=NULL)
    ReleaseOpenCLKernel(equalizeKernel);
  if (queue != NULL)
    ReleaseOpenCLCommandQueue(device, queue);
  if (device != NULL)
    ReleaseOpenCLDevice(device);

  return(outputReady);
}

MagickPrivate MagickBooleanType AccelerateEqualizeImage(Image *image,
  ExceptionInfo *exception)
{
  return(ComputeEqualizeImage(image,getAccelerateEnvironment(image,
    HistogramAccelerateCheck,__func__,exception),exception));
}
#endif /* MAGICKCORE_OPENCL_SUPPORT */
