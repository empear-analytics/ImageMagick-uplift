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

  MagickCore private methods shared by the OpenCL accelerated histogram
  methods: the histogram kernel launch of ContrastStretch and Equalize.
*/
#ifndef MAGICKCORE_ACCELERATE_HISTOGRAM_PRIVATE_H
#define MAGICKCORE_ACCELERATE_HISTOGRAM_PRIVATE_H

#include "MagickCore/accelerate-core-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(MAGICKCORE_OPENCL_SUPPORT)

typedef struct _HistogramKernelLaunch
{
  MagickCLEnv
    clEnv;

  MagickCLDevice
    device;

  cl_command_queue
    queue;

  cl_mem
    imageBuffer,
    histogramBuffer;
} HistogramKernelLaunch;

static inline MagickBooleanType LaunchHistogramKernel(
  const HistogramKernelLaunch *launch,Image *image,const ChannelType channel,
  ExceptionInfo *exception)
{
  const MagickCLEnv
    clEnv = launch->clEnv;

  const MagickCLDevice
    device = launch->device;

  const cl_command_queue
    queue = launch->queue;

  cl_mem
    imageBuffer = launch->imageBuffer,
    histogramBuffer = launch->histogramBuffer;

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

#endif /* MAGICKCORE_OPENCL_SUPPORT */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
