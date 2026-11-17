/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                   OOO   PPPP   EEEEE  N   N   CCCC  L                       %
%                  O   O  P   P  E      NN  N  C      L                       %
%                  O   O  PPPP   EEE    N N N  C      L                       %
%                  O   O  P      E      N  NN  C      L                       %
%                   OOO   P      EEEEE  N   N   CCCC  LLLLL                   %
%                                                                             %
%               EEEEE  N   N   QQQ   U   U  EEEEE  U   U  EEEEE               %
%               E      NN  N  Q   Q  U   U  E      U   U  E                   %
%               EEE    N N N  Q   Q  U   U  EEE    U   U  EEE                 %
%               E      N  NN  Q  QQ  U   U  E      U   U  E                   %
%               EEEEE  N   N   QQQQ   UUU   EEEEE   UUU   EEEEE               %
%                                                                             %
%                                                                             %
%                  MagickCore OpenCL Kernel Enqueue Methods                   %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 March 2000                                  %
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
%
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/color.h"
#include "MagickCore/compare.h"
#include "MagickCore/constitute.h"
#include "MagickCore/configure.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/layer.h"
#include "MagickCore/locale_.h"
#include "MagickCore/mime-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/montage.h"
#include "MagickCore/morphology.h"
#include "MagickCore/nt-base.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/opencl.h"
#include "MagickCore/opencl-private.h"
#include "MagickCore/option.h"
#include "MagickCore/policy.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resource_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"

#if defined(MAGICKCORE_OPENCL_SUPPORT)
#if defined(MAGICKCORE_LTDL_DELEGATE)
#include "ltdl.h"
#endif

static void CopyOpenCLEventLists(cl_event *events,MagickCLCacheInfo first,
  MagickCLCacheInfo second)
{
  size_t
    i;

  size_t
    j;

  j=0;
  for (i=0; i < first->event_count; i++, j++)
    events[j]=first->events[i];
  if (second != (MagickCLCacheInfo) NULL)
    {
      for (i=0; i < second->event_count; i++, j++)
        events[j]=second->events[i];
    }
}

static cl_event* CopyOpenCLEvents(MagickCLCacheInfo first,
  MagickCLCacheInfo second,cl_uint *event_count)
{
  cl_event
    *events;

  assert(first != (MagickCLCacheInfo) NULL);
  assert(event_count != (cl_uint *) NULL);
  events=(cl_event *) NULL;
  LockSemaphoreInfo(first->events_semaphore);
  if (second != (MagickCLCacheInfo) NULL)
    LockSemaphoreInfo(second->events_semaphore);
  *event_count=first->event_count;
  if (second != (MagickCLCacheInfo) NULL)
    *event_count+=second->event_count;
  if (*event_count > 0)
    {
      events=(cl_event *) AcquireQuantumMemory(*event_count,sizeof(*events));
      if (events == (cl_event *) NULL)
        *event_count=0;
      else
        CopyOpenCLEventLists(events,first,second);
    }
  UnlockSemaphoreInfo(first->events_semaphore);
  if (second != (MagickCLCacheInfo) NULL)
    UnlockSemaphoreInfo(second->events_semaphore);
  return(events);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C o p y M a g i c k C L C a c h e I n f o                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CopyMagickCLCacheInfo() copies the memory from the device into host memory.
%
%  The format of the CopyMagickCLCacheInfo method is:
%
%      void CopyMagickCLCacheInfo(MagickCLCacheInfo info)
%
%  A description of each parameter follows:
%
%    o info: the OpenCL cache info.
%
*/
MagickPrivate MagickCLCacheInfo CopyMagickCLCacheInfo(MagickCLCacheInfo info)
{
  cl_command_queue
    queue;

  cl_event
    *events;

  cl_uint
    event_count;

  Quantum
    *pixels;

  if (info == (MagickCLCacheInfo) NULL)
    return((MagickCLCacheInfo) NULL);
  events=CopyOpenCLEvents(info,(MagickCLCacheInfo) NULL,&event_count);
  if (events != (cl_event *) NULL)
    {
      queue=AcquireOpenCLCommandQueue(info->device);
      pixels=(Quantum *) openCL_library->clEnqueueMapBuffer(queue,info->buffer,
        CL_TRUE,CL_MAP_READ | CL_MAP_WRITE,0,(size_t) info->length,event_count,
        events,
        (cl_event *) NULL,(cl_int *) NULL);
      assert(pixels == info->pixels);
      ReleaseOpenCLCommandQueue(info->device,queue);
      events=(cl_event *) RelinquishMagickMemory(events);
    }
  return(RelinquishMagickCLCacheInfo(info,MagickFalse));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   E n q u e u e O p e n C L K e r n e l                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EnqueueOpenCLKernel() enques the specified kernel and registers the OpenCL
%  events with the images.
%
%  The format of the EnqueueOpenCLKernel method is:
%
%      MagickBooleanType EnqueueOpenCLKernel(cl_kernel kernel,cl_uint work_dim,
%        const size_t *global_work_offset,const size_t *global_work_size,
%        const size_t *local_work_size,const Image *input_image,
%        const Image *output_image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o kernel: the OpenCL kernel.
%
%    o work_dim: the number of dimensions used to specify the global work-items
%                and work-items in the work-group.
%
%    o offset: can be used to specify an array of work_dim unsigned values
%              that describe the offset used to calculate the global ID of a
%              work-item.
%
%    o gsize: points to an array of work_dim unsigned values that describe the
%             number of global work-items in work_dim dimensions that will
%             execute the kernel function.
%
%    o lsize: points to an array of work_dim unsigned values that describe the
%             number of work-items that make up a work-group that will execute
%             the kernel specified by kernel.
%
%    o input_image: the input image of the operation.
%
%    o output_image: the output or secondary image of the operation.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickBooleanType RegisterCacheEvent(MagickCLCacheInfo info,
  cl_event event)
{
  assert(info != (MagickCLCacheInfo) NULL);
  assert(event != (cl_event) NULL);
  if (openCL_library->clRetainEvent(event) != CL_SUCCESS)
    {
      openCL_library->clWaitForEvents(1,&event);
      return(MagickFalse);
    }
  LockSemaphoreInfo(info->events_semaphore);
  if (info->events == (cl_event *) NULL)
    {
      info->events=(cl_event *) AcquireMagickMemory(sizeof(*info->events));
      info->event_count=1;
    }
  else
    info->events=(cl_event *) ResizeQuantumMemory(info->events,
      ++info->event_count,sizeof(*info->events));
  if (info->events == (cl_event *) NULL)
    {
      UnlockSemaphoreInfo(info->events_semaphore);
      return(MagickFalse);
    }
  info->events[info->event_count-1]=event;
  UnlockSemaphoreInfo(info->events_semaphore);
  return(MagickTrue);
}

static void RegisterOpenCLKernelEvent(CacheInfo *input_info,
  CacheInfo *output_info,cl_event event)
{
  if (RegisterCacheEvent(input_info->opencl,event) != MagickFalse)
    {
      if (output_info != (CacheInfo *) NULL)
        (void) RegisterCacheEvent(output_info->opencl,event);
    }
}

MagickPrivate MagickBooleanType EnqueueOpenCLKernel(cl_command_queue queue,
  cl_kernel kernel,cl_uint work_dim,const size_t *offset,const size_t *gsize,
  const size_t *lsize,const Image *input_image,const Image *output_image,
  MagickBooleanType flush,ExceptionInfo *exception)
{
  CacheInfo
    *output_info,
    *input_info;

  cl_event
    event,
    *events;

  cl_int
    status;

  cl_uint
    event_count;

  assert(input_image != (const Image *) NULL);
  input_info=(CacheInfo *) input_image->cache;
  assert(input_info != (CacheInfo *) NULL);
  assert(input_info->opencl != (MagickCLCacheInfo) NULL);
  output_info=(CacheInfo *) NULL;
  if (output_image == (const Image *) NULL)
    events=CopyOpenCLEvents(input_info->opencl,(MagickCLCacheInfo) NULL,
      &event_count);
  else
    {
      output_info=(CacheInfo *) output_image->cache;
      assert(output_info != (CacheInfo *) NULL);
      assert(output_info->opencl != (MagickCLCacheInfo) NULL);
      events=CopyOpenCLEvents(input_info->opencl,output_info->opencl,
        &event_count);
    }
  status=openCL_library->clEnqueueNDRangeKernel(queue,kernel,work_dim,offset,
    gsize,lsize,event_count,events,&event);
  /* This can fail due to memory issues and calling clFinish might help. */
  if ((status != CL_SUCCESS) && (event_count > 0))
    {
      openCL_library->clFinish(queue);
      status=openCL_library->clEnqueueNDRangeKernel(queue,kernel,work_dim,
        offset,gsize,lsize,event_count,events,&event);
    }
  events=(cl_event *) RelinquishMagickMemory(events);
  if (status != CL_SUCCESS)
    {
      (void) OpenCLThrowMagickException(input_info->opencl->device,exception,
        GetMagickModule(),ResourceLimitWarning,
        "clEnqueueNDRangeKernel failed.","'%s'",".");
      return(MagickFalse);
    }
  if (flush != MagickFalse)
    openCL_library->clFlush(queue);
  if (RecordProfileData(input_info->opencl->device,kernel,event) == MagickFalse)
    RegisterOpenCLKernelEvent(input_info,output_info,event);
  openCL_library->clReleaseEvent(event);
  return(MagickTrue);
}
#endif
