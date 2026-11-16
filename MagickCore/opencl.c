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
%                                                                             %
%                         MagickCore OpenCL Methods                           %
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

/* OpenCL library */
MagickLibrary
  *openCL_library;

/* Default OpenCL environment */
MagickCLEnv
  default_CLEnv;
MagickThreadType
  test_thread_id=0;
SemaphoreInfo
  *openCL_lock;


static void DestroyMagickCLCacheInfo(MagickCLCacheInfo info)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) info->event_count; i++)
    openCL_library->clReleaseEvent(info->events[i]);
  info->events=(cl_event *) RelinquishMagickMemory(info->events);
  if (info->buffer != (cl_mem) NULL)
    openCL_library->clReleaseMemObject(info->buffer);
  RelinquishSemaphoreInfo(&info->events_semaphore);
  ReleaseOpenCLDevice(info->device);
  RelinquishMagickMemory(info);
}

/*
  Provide call to OpenCL library methods
*/

MagickPrivate cl_mem CreateOpenCLBuffer(MagickCLDevice device,
  cl_mem_flags flags,size_t size,void *host_ptr)
{
  return(openCL_library->clCreateBuffer(device->context,flags,size,host_ptr,
    (cl_int *) NULL));
}

MagickPrivate void ReleaseOpenCLKernel(cl_kernel kernel)
{
  (void) openCL_library->clReleaseKernel(kernel);
}

MagickPrivate void ReleaseOpenCLMemObject(cl_mem memobj)
{
  (void) openCL_library->clReleaseMemObject(memobj);
}

MagickPrivate void RetainOpenCLMemObject(cl_mem memobj)
{
  (void) openCL_library->clRetainMemObject(memobj);
}

MagickPrivate cl_int SetOpenCLKernelArg(cl_kernel kernel,size_t arg_index,
  size_t arg_size,const void *arg_value)
{
  return(openCL_library->clSetKernelArg(kernel,(cl_uint) arg_index,arg_size,
    arg_value));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e M a g i c k C L C a c h e I n f o                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireMagickCLCacheInfo() acquires an OpenCL cache info structure.
%
%  The format of the AcquireMagickCLCacheInfo method is:
%
%      MagickCLCacheInfo AcquireMagickCLCacheInfo(MagickCLDevice device,
%        Quantum *pixels,const MagickSizeType length)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device.
%
%    o pixels: the pixel buffer of the image.
%
%    o length: the length of the pixel buffer.
%
*/

MagickPrivate MagickCLCacheInfo AcquireMagickCLCacheInfo(MagickCLDevice device,
  Quantum *pixels,const MagickSizeType length)
{
  cl_int
    status;

  MagickCLCacheInfo
    info;

  info=(MagickCLCacheInfo) AcquireCriticalMemory(sizeof(*info));
  (void) memset(info,0,sizeof(*info));
  LockSemaphoreInfo(openCL_lock);
  device->requested++;
  UnlockSemaphoreInfo(openCL_lock);
  info->device=device;
  info->length=length;
  info->pixels=pixels;
  info->events_semaphore=AcquireSemaphoreInfo();
  info->buffer=openCL_library->clCreateBuffer(device->context,
    CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,(size_t) length,(void *) pixels,
    &status);
  if (status == CL_SUCCESS)
    return(info);
  DestroyMagickCLCacheInfo(info);
  return((MagickCLCacheInfo) NULL);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e O p e n C L C o m m a n d Q u e u e                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireOpenCLCommandQueue() acquires an OpenCL command queue
%
%  The format of the AcquireOpenCLCommandQueue method is:
%
%      cl_command_queue AcquireOpenCLCommandQueue(MagickCLDevice device)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device.
%
*/

MagickPrivate cl_command_queue AcquireOpenCLCommandQueue(MagickCLDevice device)
{
  cl_command_queue
    queue;

  cl_command_queue_properties
    properties;

  assert(device != (MagickCLDevice) NULL);
  LockSemaphoreInfo(device->lock);
  if ((device->profile_kernels == MagickFalse) &&
      (device->command_queues_index >= 0))
  {
    queue=device->command_queues[device->command_queues_index--];
    UnlockSemaphoreInfo(device->lock);
  }
  else
  {
    UnlockSemaphoreInfo(device->lock);
    properties=0;
    if (device->profile_kernels != MagickFalse)
      properties=CL_QUEUE_PROFILING_ENABLE;
    queue=openCL_library->clCreateCommandQueue(device->context,
      device->deviceID,properties,(cl_int *) NULL);
  }
  return(queue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e O p e n C L K e r n e l                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireOpenCLKernel() acquires an OpenCL kernel
%
%  The format of the AcquireOpenCLKernel method is:
%
%      cl_kernel AcquireOpenCLKernel(MagickCLEnv clEnv,
%        MagickOpenCLProgram program, const char* kernelName)
%
%  A description of each parameter follows:
%
%    o clEnv: the OpenCL environment.
%
%    o program: the OpenCL program module that the kernel belongs to.
%
%    o kernelName:  the name of the kernel
%
*/

MagickPrivate cl_kernel AcquireOpenCLKernel(MagickCLDevice device,
  const char *kernel_name)
{
  cl_kernel
    kernel;

  assert(device != (MagickCLDevice) NULL);
  (void) LogMagickEvent(AccelerateEvent,GetMagickModule(),"Using kernel: %s",
    kernel_name);
  kernel=openCL_library->clCreateKernel(device->program,kernel_name,
    (cl_int *) NULL);
  return(kernel);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   O p e n C L T h r o w M a g i c k E x c e p t i o n                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OpenCLThrowMagickException logs an OpenCL exception as determined by the log
%  configuration file.  If an error occurs, MagickFalse is returned
%  otherwise MagickTrue.
%
%  The format of the OpenCLThrowMagickException method is:
%
%      MagickBooleanType OpenCLThrowMagickException(ExceptionInfo *exception,
%        const char *module,const char *function,const size_t line,
%        const ExceptionType severity,const char *tag,const char *format,...)
%
%  A description of each parameter follows:
%
%    o exception: the exception info.
%
%    o filename: the source module filename.
%
%    o function: the function name.
%
%    o line: the line number of the source module.
%
%    o severity: Specifies the numeric error category.
%
%    o tag: the locale tag.
%
%    o format: the output format.
%
*/

MagickPrivate MagickBooleanType OpenCLThrowMagickException(
  MagickCLDevice device,ExceptionInfo *exception,const char *module,
  const char *function,const size_t line,const ExceptionType severity,
  const char *tag,const char *format,...)
{
  MagickBooleanType
    status;

  assert(device != (MagickCLDevice) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  (void) exception;
  status=MagickTrue;
  if (severity != 0)
  {
    if (device->type == CL_DEVICE_TYPE_CPU)
    {
      /* Workaround for Intel OpenCL CPU runtime bug */
      /* Turn off OpenCL when a problem is detected! */
      if (strncmp(device->platform_name,"Intel",5) == 0)
        default_CLEnv->enabled=MagickFalse;
    }
  }

#ifdef OPENCLLOG_ENABLED
  {
    va_list
      operands;
    va_start(operands,format);
    status=ThrowMagickExceptionList(exception,module,function,line,severity,tag,
      format,operands);
    va_end(operands);
  }
#else
  magick_unreferenced(module);
  magick_unreferenced(function);
  magick_unreferenced(line);
  magick_unreferenced(tag);
  magick_unreferenced(format);
#endif

  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e c o r d P r o f i l e D a t a                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RecordProfileData() records profile data.
%
%  The format of the RecordProfileData method is:
%
%      void RecordProfileData(MagickCLDevice device,ProfiledKernels kernel,
%        cl_event event)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device that did the operation.
%
%    o event: the event that contains the profiling data.
%
*/

static void FindKernelProfileRecord(MagickCLDevice device,const char *name,
  size_t *i,KernelProfileRecord *profile_record)
{
  if (device->profile_records != (KernelProfileRecord *) NULL)
    {
      while (device->profile_records[*i] != (KernelProfileRecord) NULL)
      {
        if (LocaleCompare(device->profile_records[*i]->kernel_name,name) == 0)
          {
            *profile_record=device->profile_records[*i];
            break;
          }
        (*i)++;
      }
    }
}

static void UpdateKernelProfileRecord(KernelProfileRecord profile_record,
  const cl_ulong elapsed)
{
  if ((elapsed < profile_record->min) || (profile_record->count == 0))
    profile_record->min=(unsigned long) elapsed;
  if (elapsed > profile_record->max)
    profile_record->max=(unsigned long) elapsed;
  profile_record->total+=(unsigned long) elapsed;
  profile_record->count+=1;
}

static KernelProfileRecord AcquireKernelProfileRecord(char *name)
{
  KernelProfileRecord
    profile_record;

  profile_record=(KernelProfileRecord) AcquireCriticalMemory(
    sizeof(*profile_record));
  (void) memset(profile_record,0,sizeof(*profile_record));
  profile_record->kernel_name=name;
  return(profile_record);
}

MagickPrivate MagickBooleanType RecordProfileData(MagickCLDevice device,
  cl_kernel kernel,cl_event event)
{
  char
    *name;

  cl_int
    status;

  cl_ulong
    elapsed,
    end,
    start;

  KernelProfileRecord
    profile_record;

  size_t
    i,
    length;

  if (device->profile_kernels == MagickFalse)
    return(MagickFalse);
  status=openCL_library->clWaitForEvents(1,&event);
  if (status != CL_SUCCESS)
    return(MagickFalse);
  status=openCL_library->clGetKernelInfo(kernel,CL_KERNEL_FUNCTION_NAME,0,NULL,
    &length);
  if (status != CL_SUCCESS)
    return(MagickTrue);
  name=(char *) AcquireQuantumMemory(length,sizeof(*name));
  if (name == (char *) NULL)
    return(MagickTrue);
  start=end=elapsed=0;
  status=openCL_library->clGetKernelInfo(kernel,CL_KERNEL_FUNCTION_NAME,length,
    name,(size_t *) NULL);
  status|=openCL_library->clGetEventProfilingInfo(event,
    CL_PROFILING_COMMAND_START,sizeof(cl_ulong),&start,NULL);
  status|=openCL_library->clGetEventProfilingInfo(event,
    CL_PROFILING_COMMAND_END,sizeof(cl_ulong),&end,NULL);
  if (status != CL_SUCCESS)
    {
      name=DestroyString(name);
      return(MagickTrue);
    }
  start/=1000; /* usecs */
  end/=1000;   
  elapsed=end-start;
  LockSemaphoreInfo(device->lock);
  i=0;
  profile_record=(KernelProfileRecord) NULL;
  FindKernelProfileRecord(device,name,&i,&profile_record);
  if (profile_record != (KernelProfileRecord) NULL)
    name=DestroyString(name);
  else
    {
      profile_record=AcquireKernelProfileRecord(name);
      device->profile_records=(KernelProfileRecord *) ResizeQuantumMemory(
        device->profile_records,(i+2),sizeof(*device->profile_records));
      if (device->profile_records == (KernelProfileRecord *) NULL)
        {
          UnlockSemaphoreInfo(device->lock);
          profile_record=(KernelProfileRecord) RelinquishMagickMemory(
            profile_record);
          name=DestroyString(name);
          return(MagickFalse);
        }
      device->profile_records[i]=profile_record;
      device->profile_records[i+1]=(KernelProfileRecord) NULL;
    }
  UpdateKernelProfileRecord(profile_record,elapsed);
  UnlockSemaphoreInfo(device->lock);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e l e a s e O p e n C L C o m m a n d Q u e u e                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReleaseOpenCLCommandQueue() releases the OpenCL command queue
%
%  The format of the ReleaseOpenCLCommandQueue method is:
%
%      void ReleaseOpenCLCommandQueue(MagickCLDevice device,
%        cl_command_queue queue)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device.
%
%    o queue: the OpenCL queue to be released.
*/

MagickPrivate void ReleaseOpenCLCommandQueue(MagickCLDevice device,
  cl_command_queue queue)
{
  if (queue == (cl_command_queue) NULL)
    return;

  assert(device != (MagickCLDevice) NULL);
  LockSemaphoreInfo(device->lock);
  if ((device->profile_kernels != MagickFalse) ||
      (device->command_queues_index >= MAGICKCORE_OPENCL_COMMAND_QUEUES-1))
    {
      UnlockSemaphoreInfo(device->lock);
      openCL_library->clFinish(queue);
      (void) openCL_library->clReleaseCommandQueue(queue);
    }
  else
    {
      openCL_library->clFlush(queue);
      device->command_queues[++device->command_queues_index]=queue;
      UnlockSemaphoreInfo(device->lock);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e l e a s e  M a g i c k C L D e v i c e                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReleaseOpenCLDevice() returns the OpenCL device to the environment
%
%  The format of the ReleaseOpenCLDevice method is:
%
%      void ReleaseOpenCLDevice(MagickCLDevice device)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device to be released.
%
*/

MagickPrivate void ReleaseOpenCLDevice(MagickCLDevice device)
{
  assert(device != (MagickCLDevice) NULL);
  LockSemaphoreInfo(openCL_lock);
  device->requested--;
  UnlockSemaphoreInfo(openCL_lock);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e l i n q u i s h M a g i c k C L C a c h e I n f o                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RelinquishMagickCLCacheInfo() frees memory acquired with
%  AcquireMagickCLCacheInfo()
%
%  The format of the RelinquishMagickCLCacheInfo method is:
%
%      MagickCLCacheInfo RelinquishMagickCLCacheInfo(MagickCLCacheInfo info,
%        const MagickBooleanType relinquish_pixels)
%
%  A description of each parameter follows:
%
%    o info: the OpenCL cache info.
%
%    o relinquish_pixels: the pixels will be relinquish when set to true.
%
*/

static void CL_API_CALL DestroyMagickCLCacheInfoAndPixels(
  cl_event magick_unused(event),
  cl_int magick_unused(event_command_exec_status),void *user_data)
{
  MagickCLCacheInfo
    info;

  Quantum
    *pixels;

  ssize_t
    i;

  magick_unreferenced(event);
  magick_unreferenced(event_command_exec_status);
  info=(MagickCLCacheInfo) user_data;
  for (i=(ssize_t)info->event_count-1; i >= 0; i--)
  {
    cl_int
      event_status;

    cl_uint
      status;

    status=openCL_library->clGetEventInfo(info->events[i],
      CL_EVENT_COMMAND_EXECUTION_STATUS,sizeof(event_status),&event_status,
      NULL);
    if ((status == CL_SUCCESS) && (event_status > CL_COMPLETE))
      {
        openCL_library->clSetEventCallback(info->events[i],CL_COMPLETE,
          &DestroyMagickCLCacheInfoAndPixels,info);
        return;
      }
  }
  pixels=info->pixels;
  RelinquishMagickResource(MemoryResource,info->length);
  DestroyMagickCLCacheInfo(info);
  (void) RelinquishAlignedMemory(pixels);
}

MagickPrivate MagickCLCacheInfo RelinquishMagickCLCacheInfo(
  MagickCLCacheInfo info,const MagickBooleanType relinquish_pixels)
{
  if (info == (MagickCLCacheInfo) NULL)
    return((MagickCLCacheInfo) NULL);
  if (relinquish_pixels != MagickFalse)
    DestroyMagickCLCacheInfoAndPixels((cl_event) NULL,0,info);
  else
    DestroyMagickCLCacheInfo(info);
  return((MagickCLCacheInfo) NULL);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e q u e s t O p e n C L D e v i c e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RequestOpenCLDevice() returns one of the enabled OpenCL devices.
%
%  The format of the RequestOpenCLDevice method is:
%
%      MagickCLDevice RequestOpenCLDevice(MagickCLEnv clEnv)
%
%  A description of each parameter follows:
%
%    o clEnv: the OpenCL environment.
*/

static MagickCLDevice SelectBestOpenCLDevice(MagickCLEnv clEnv,
  MagickCLDevice device,double best_score)
{
  double
    score;

  size_t
    i;

  for (i = 0; i < clEnv->number_devices; i++)
  {
    if (clEnv->devices[i]->enabled == MagickFalse)
      continue;

    score=clEnv->devices[i]->score+(clEnv->devices[i]->score*
      clEnv->devices[i]->requested);
    if ((device == (MagickCLDevice) NULL) || (score < best_score))
    {
      device=clEnv->devices[i];
      best_score=score;
    }
  }
  return(device);
}

MagickPrivate MagickCLDevice RequestOpenCLDevice(MagickCLEnv clEnv)
{
  MagickCLDevice
    device;

  double
    best_score;

  if (clEnv == (MagickCLEnv) NULL)
    return((MagickCLDevice) NULL);

  if (clEnv->number_devices == 1)
  {
    if (clEnv->devices[0]->enabled)
      return(clEnv->devices[0]);
    else
      return((MagickCLDevice) NULL);
  }

  device=(MagickCLDevice) NULL;
  best_score=0.0;
  LockSemaphoreInfo(openCL_lock);
  device=SelectBestOpenCLDevice(clEnv,device,best_score);
  if (device != (MagickCLDevice)NULL)
    device->requested++;
  UnlockSemaphoreInfo(openCL_lock);

  return(device);
}

#else
#endif
