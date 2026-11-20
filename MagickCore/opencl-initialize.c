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
%    IIIII  N   N  IIIII  TTTTT  IIIII   AAA   L      IIIII  ZZZZZ  EEEEE     %
%      I    NN  N    I      T      I    A   A  L        I       ZZ  E         %
%      I    N N N    I      T      I    AAAAA  L        I     ZZZ   EEE       %
%      I    N  NN    I      T      I    A   A  L        I    ZZ     E         %
%    IIIII  N   N  IIIII    T    IIIII  A   A  LLLLL  IIIII  ZZZZZ  EEEEE     %
%                                                                             %
%                                                                             %
%                  MagickCore OpenCL Initialization Methods                   %
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
#include "MagickCore/accelerate-kernels-private.h"
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
#include "MagickCore/opencl-environment-private.h"

#if defined(MAGICKCORE_OPENCL_SUPPORT)
#if defined(MAGICKCORE_LTDL_DELEGATE)
#include "ltdl.h"
#endif
#include "MagickCore/opencl-initialize-benchmark-private.h"

static MagickBooleanType LoadOpenCLBenchmarks(MagickCLEnv clEnv)
{
#if !MAGICKCORE_ZERO_CONFIGURATION_SUPPORT
  char
    filename[MagickPathExtent];

  StringInfo
    *option;

  (void) FormatLocaleString(filename,MagickPathExtent,"%s%s%s",
    GetOpenCLCacheDirectory(),DirectorySeparator,IMAGEMAGICK_PROFILE_FILE);

  /*
    We don't run the benchmark when we can not write out a device profile. The
    first GPU device will be used.
  */
  if (CanWriteProfileToFile(filename) == MagickFalse)
#endif
    {
      size_t
        i;

      for (i = 0; i < clEnv->number_devices; i++)
        clEnv->devices[i]->score=1.0;

      SelectOpenCLDevice(clEnv,CL_DEVICE_TYPE_GPU);
      return(MagickFalse);
    }
#if !MAGICKCORE_ZERO_CONFIGURATION_SUPPORT
  option=ConfigureFileToStringInfo(filename);
  LoadOpenCLDeviceBenchmark(clEnv,(const char *) GetStringInfoDatum(option));
  option=DestroyStringInfo(option);
  return(MagickTrue);
#endif
}

static void SelectOpenCLDeviceOption(MagickCLEnv clEnv)
{
  char
    *option;

  option=GetEnvironmentValue("MAGICK_OCL_DEVICE");
  if (option != (const char *) NULL)
    {
      if (strcmp(option,"GPU") == 0)
        SelectOpenCLDevice(clEnv,CL_DEVICE_TYPE_GPU);
      else if (strcmp(option,"CPU") == 0)
        SelectOpenCLDevice(clEnv,CL_DEVICE_TYPE_CPU);
      option=DestroyString(option);
    }
}

static MagickBooleanType IsOpenCLBenchmarkNeeded(MagickCLEnv clEnv)
{
  MagickBooleanType
    benchmark;

  size_t
    i;

  benchmark=MagickFalse;
  if (clEnv->cpu_score == MAGICKCORE_OPENCL_UNDEFINED_SCORE)
    benchmark=MagickTrue;
  else
    {
      for (i = 0; i < clEnv->number_devices; i++)
      {
        if (clEnv->devices[i]->score == MAGICKCORE_OPENCL_UNDEFINED_SCORE)
        {
          benchmark=MagickTrue;
          break;
        }
      }
    }
  return(benchmark);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e M a g i c k C L E n v                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% AcquireMagickCLEnv() allocates the MagickCLEnv structure
%
*/

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A u t o S e l e c t O p e n C L D e v i c e s                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AutoSelectOpenCLDevices() determines the best device based on the
%  information from the micro-benchmark.
%
%  The format of the AutoSelectOpenCLDevices method is:
%
%      void AcquireOpenCLKernel(MagickCLEnv clEnv,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o clEnv: the OpenCL environment.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static void AutoSelectOpenCLDevices(MagickCLEnv clEnv)
{
  double
    best_score;

  MagickBooleanType
    benchmark;

  size_t
    i;

  SelectOpenCLDeviceOption(clEnv);

  if (LoadOpenCLBenchmarks(clEnv) == MagickFalse)
    return;

  benchmark=IsOpenCLBenchmarkNeeded(clEnv);

  if (benchmark != MagickFalse)
    BenchmarkOpenCLDevices(clEnv);

  best_score=clEnv->cpu_score;
  for (i = 0; i < clEnv->number_devices; i++)
    best_score=MagickMin(clEnv->devices[i]->score,best_score);

  for (i = 0; i < clEnv->number_devices; i++)
  {
    if (clEnv->devices[i]->score != best_score)
      clEnv->devices[i]->enabled=MagickFalse;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B e n c h m a r k O p e n C L D e v i c e s                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BenchmarkOpenCLDevices() benchmarks the OpenCL devices and the CPU to help
%  the automatic selection of the best device.
%
%  The format of the BenchmarkOpenCLDevices method is:
%
%    void BenchmarkOpenCLDevices(MagickCLEnv clEnv,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o clEnv: the OpenCL environment.
%
%    o exception: return any errors or warnings
*/

static void DestroyBenchmarkImages(Image *blurredImage,Image *unsharpedImage,
  Image *resizedImage)
{
  if (blurredImage != (Image *) NULL)
    DestroyImage(blurredImage);
  if (unsharpedImage != (Image *) NULL)
    DestroyImage(unsharpedImage);
  if (resizedImage != (Image *) NULL)
    DestroyImage(resizedImage);
}

static void WaitForOpenCLImageEvents(Image *resizedImage)
{
  CacheInfo
    *cache_info;

  cache_info=(CacheInfo *) resizedImage->cache;
  if (cache_info->opencl != (MagickCLCacheInfo) NULL)
    openCL_library->clWaitForEvents(cache_info->opencl->event_count,
      cache_info->opencl->events);
}

static double RunOpenCLBenchmark(MagickBooleanType is_cpu)
{
  AccelerateTimer
    timer;

  ExceptionInfo
    *exception;

  Image
    *inputImage;

  ImageInfo
    *imageInfo;

  size_t
    i;

  exception=AcquireExceptionInfo();
  imageInfo=AcquireImageInfo();
  CloneString(&imageInfo->size,"2048x1536");
  (void) CopyMagickString(imageInfo->filename,"xc:none",MagickPathExtent);
  inputImage=ReadImage(imageInfo,exception);
  if (inputImage == (Image *) NULL)
    return(0.0);

  InitAccelerateTimer(&timer);

  for (i=0; i<=2; i++)
  {
    Image
      *blurredImage,
      *resizedImage,
      *unsharpedImage;

    if (i > 0)
      StartAccelerateTimer(&timer);

    blurredImage=BlurImage(inputImage,10.0f,3.5f,exception);
    unsharpedImage=UnsharpMaskImage(blurredImage,2.0f,2.0f,50.0f,10.0f,
      exception);
    resizedImage=ResizeImage(unsharpedImage,640,480,LanczosFilter,
      exception);

    /*
      We need this to get a proper performance benchmark, the operations
      are executed asynchronous.
    */
    if (is_cpu == MagickFalse)
      WaitForOpenCLImageEvents(resizedImage);

    if (i > 0)
      StopAccelerateTimer(&timer);

    DestroyBenchmarkImages(blurredImage,unsharpedImage,resizedImage);
  }
  DestroyImage(inputImage);
  return(ReadAccelerateTimer(&timer));
}

static void RunDeviceBenchmark(MagickCLEnv clEnv,MagickCLEnv testEnv,
  MagickCLDevice device)
{
  testEnv->devices[0]=device;
  default_CLEnv=testEnv;
  device->score=RunOpenCLBenchmark(MagickFalse);
  default_CLEnv=clEnv;
  testEnv->devices[0]=(MagickCLDevice) NULL;
}

static void CacheOpenCLBenchmarks(MagickCLEnv clEnv)
{
  char
    filename[MagickPathExtent];

  FILE
    *cache_file;

  MagickCLDevice
    device;

  size_t
    i,
    j;

  (void) FormatLocaleString(filename,MagickPathExtent,"%s%s%s",
    GetOpenCLCacheDirectory(),DirectorySeparator,
    IMAGEMAGICK_PROFILE_FILE);

  cache_file=fopen_utf8(filename,"wb");
  if (cache_file == (FILE *) NULL)
    return;
  fwrite("<devices>\n",sizeof(char),10,cache_file);
  fprintf(cache_file,"  <device name=\"CPU\" score=\"%.4g\"/>\n",
    clEnv->cpu_score);
  for (i = 0; i < clEnv->number_devices; i++)
  {
    MagickBooleanType
      duplicate;

    device=clEnv->devices[i];
    duplicate=MagickFalse;
    for (j = 0; j < i; j++)
    {
      if (IsSameOpenCLDevice(clEnv->devices[j],device))
      {
        duplicate=MagickTrue;
        break;
      }
    }

    if (duplicate)
      continue;

    if (device->score != MAGICKCORE_OPENCL_UNDEFINED_SCORE)
      fprintf(cache_file,"  <device platform=\"%s\" vendor=\"%s\" name=\"%s\"\
 version=\"%s\" maxClockFrequency=\"%d\" maxComputeUnits=\"%d\"\
 score=\"%.4g\"/>\n",
        device->platform_name,device->vendor_name,device->name,device->version,
        (int)device->max_clock_frequency,(int)device->max_compute_units,
        device->score);
  }
  fwrite("</devices>",sizeof(char),10,cache_file);

  fclose(cache_file);
}

static void BenchmarkOpenCLDevice(MagickCLEnv clEnv,MagickCLEnv testEnv,
  const size_t i)
{
  MagickCLDevice
    device;

  size_t
    j;

  device=clEnv->devices[i];
  if (device->score == MAGICKCORE_OPENCL_UNDEFINED_SCORE)
    RunDeviceBenchmark(clEnv,testEnv,device);

  /* Set the score on all the other devices that are the same */
  for (j = i+1; j < clEnv->number_devices; j++)
  {
    MagickCLDevice
      other_device;

    other_device=clEnv->devices[j];
    if (IsSameOpenCLDevice(device,other_device))
      other_device->score=device->score;
  }
}

static void BenchmarkOpenCLDevices(MagickCLEnv clEnv)
{
  MagickCLEnv
    testEnv;

  size_t
    i;

  (void) LogMagickEvent(AccelerateEvent,GetMagickModule(),
    "Starting benchmark");
  testEnv=AcquireMagickCLEnv();
  testEnv->library=openCL_library;
  testEnv->devices=(MagickCLDevice *) AcquireCriticalMemory(
    sizeof(MagickCLDevice));
  testEnv->number_devices=1;
  testEnv->benchmark_thread_id=GetMagickThreadId();
  testEnv->initialized=MagickTrue;

  for (i = 0; i < clEnv->number_devices; i++)
    clEnv->devices[i]->score=MAGICKCORE_OPENCL_UNDEFINED_SCORE;

  for (i = 0; i < clEnv->number_devices; i++)
    BenchmarkOpenCLDevice(clEnv,testEnv,i);

  testEnv->enabled=MagickFalse;
  default_CLEnv=testEnv;
  clEnv->cpu_score=RunOpenCLBenchmark(MagickTrue);
  default_CLEnv=clEnv;

  testEnv=RelinquishMagickCLEnv(testEnv);
  CacheOpenCLBenchmarks(clEnv);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p i l e O p e n C L K e r n e l                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompileOpenCLKernel() compiles the kernel for the specified device. The
%  kernel will be cached on disk to reduce the compilation time.
%
%  The format of the CompileOpenCLKernel method is:
%
%      MagickBooleanType AcquireOpenCLKernel(MagickCLDevice clEnv,
%        unsigned int signature,const char *kernel,const char *options,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device.
%
%    o kernel: the source code of the kernel.
%
%    o options: options for the compiler.
%
%    o signature: a number to uniquely identify the kernel
%
%    o exception: return any errors or warnings in this structure.
%
*/

static void CacheOpenCLKernel(MagickCLDevice device,char *filename,
  ExceptionInfo *exception)
{
  cl_uint
    status;

  size_t
    binaryProgramSize;

  unsigned char
    *binaryProgram;

  status=openCL_library->clGetProgramInfo(device->program,
    CL_PROGRAM_BINARY_SIZES,sizeof(size_t),&binaryProgramSize,NULL);
  if (status != CL_SUCCESS)
    return;
  binaryProgram=(unsigned char*) AcquireQuantumMemory(1,binaryProgramSize);
  if (binaryProgram == (unsigned char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",filename);
      return;
    }
  status=openCL_library->clGetProgramInfo(device->program,
    CL_PROGRAM_BINARIES,sizeof(unsigned char*),&binaryProgram,NULL);
  if (status == CL_SUCCESS)
    {
      (void) LogMagickEvent(AccelerateEvent,GetMagickModule(),
        "Creating cache file: \"%s\"",filename);
      (void) BlobToFile(filename,binaryProgram,binaryProgramSize,exception);
    }
  binaryProgram=(unsigned char *) RelinquishMagickMemory(binaryProgram);
}

static MagickBooleanType LoadCachedOpenCLKernels(MagickCLDevice device,
  const char *filename)
{
  cl_int
    binaryStatus,
    status;

  ExceptionInfo
    *sans_exception;

  size_t
    length;

  unsigned char
    *binaryProgram;

  sans_exception=AcquireExceptionInfo();
  binaryProgram=(unsigned char *) FileToBlob(filename,SIZE_MAX,&length,
    sans_exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  if (binaryProgram == (unsigned char *) NULL)
    return(MagickFalse);
  (void) LogMagickEvent(AccelerateEvent,GetMagickModule(),
    "Loaded cached kernels: \"%s\"",filename);
  device->program=openCL_library->clCreateProgramWithBinary(device->context,1,
    &device->deviceID,&length,(const unsigned char**)&binaryProgram,
    &binaryStatus,&status);
  binaryProgram=(unsigned char *) RelinquishMagickMemory(binaryProgram);
  return((status != CL_SUCCESS) || (binaryStatus != CL_SUCCESS) ? MagickFalse :
    MagickTrue);
}

static void LogOpenCLBuildFailure(MagickCLDevice device,const char *kernel,
  ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent],
    *log;

  size_t
    log_size;

  (void) FormatLocaleString(filename,MagickPathExtent,"%s%s%s",
    GetOpenCLCacheDirectory(),DirectorySeparator,"magick_badcl.cl");

  (void) remove_utf8(filename);
  (void) BlobToFile(filename,kernel,strlen(kernel),exception);

  openCL_library->clGetProgramBuildInfo(device->program,device->deviceID,
    CL_PROGRAM_BUILD_LOG,0,NULL,&log_size);
  log=(char*)AcquireCriticalMemory(log_size);
  openCL_library->clGetProgramBuildInfo(device->program,device->deviceID,
    CL_PROGRAM_BUILD_LOG,log_size,log,&log_size);

  (void) FormatLocaleString(filename,MagickPathExtent,"%s%s%s",
    GetOpenCLCacheDirectory(),DirectorySeparator,"magick_badcl.log");

  (void) remove_utf8(filename);
  (void) BlobToFile(filename,log,log_size,exception);
  log=(char*)RelinquishMagickMemory(log);
}

static const char
  IllegalFileNameCharacters[] =
  {
    ' ',
    '\\',
    '/',
    ':',
    '*',
    '?',
    '"',
    '<',
    '>',
    '|'
  };

static inline MagickBooleanType IsIllegalFileNameCharacter(const char c)
{
  size_t
    i;

  for (i=0; i < (sizeof(IllegalFileNameCharacters)/
       sizeof(*IllegalFileNameCharacters)); i++)
    if (IllegalFileNameCharacters[i] == c)
      return(MagickTrue);
  return(MagickFalse);
}

static void ReplaceIllegalFileNameCharacters(char *ptr)
{
  while (*ptr != '\0')
  {
    if (IsIllegalFileNameCharacter(*ptr) != MagickFalse)
      *ptr = '_';
    ptr++;
  }
}

typedef struct _OpenCLKernelBuild
{
  const char
    *options;

  ExceptionInfo
    *exception;
} OpenCLKernelBuild;

static MagickBooleanType CompileOpenCLKernel(MagickCLDevice device,
  const char *kernel,const OpenCLKernelBuild *build,size_t signature)
{
  const char
    *const options = build->options;

  ExceptionInfo
    *const exception = build->exception;

  char
    deviceName[MagickPathExtent],
    filename[MagickPathExtent],
    *ptr;

  cl_int
    status;

  MagickBooleanType
    loaded;

  size_t
    length;

  (void) CopyMagickString(deviceName,device->name,MagickPathExtent);
  ptr=deviceName;
  /* Strip out illegal characters for file names */
  ReplaceIllegalFileNameCharacters(ptr);
  (void) FormatLocaleString(filename,MagickPathExtent,
    "%s%s%s_%s_%08x_%.17g.bin",GetOpenCLCacheDirectory(),
    DirectorySeparator,"magick_opencl",deviceName,(unsigned int) signature,
    (double) sizeof(char*)*8);
  loaded=LoadCachedOpenCLKernels(device,filename);
  if (loaded == MagickFalse)
    {
      /* Binary CL program unavailable, compile the program from source */
      length=strlen(kernel);
      device->program=openCL_library->clCreateProgramWithSource(
        device->context,1,&kernel,&length,&status);
      if (status != CL_SUCCESS)
        return(MagickFalse);
    }

  status=openCL_library->clBuildProgram(device->program,1,&device->deviceID,
    options,NULL,NULL);
  if (status != CL_SUCCESS)
  {
    (void) ThrowMagickException(exception,GetMagickModule(),DelegateWarning,
      "clBuildProgram failed.","(%d)",(int)status);
    LogOpenCLBuildFailure(device,kernel,exception);
    return(MagickFalse);
  }

  /* Save the binary to a file to avoid re-compilation of the kernels */
  if (loaded == MagickFalse)
    CacheOpenCLKernel(device,filename,exception);

  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   H a s O p e n C L D e v i c e s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  HasOpenCLDevices() checks if the OpenCL environment has devices that are
%  enabled and compiles the kernel for the device when necessary. False will be
%  returned if no enabled devices could be found
%
%  The format of the HasOpenCLDevices method is:
%
%    MagickBooleanType HasOpenCLDevices(MagickCLEnv clEnv,
%      ExceptionInfo exception)
%
%  A description of each parameter follows:
%
%    o clEnv: the OpenCL environment.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickBooleanType HasOpenCLKernelPrograms(MagickCLEnv clEnv)
{
  MagickBooleanType
    status;

  size_t
    i;

  status=MagickTrue;
  for (i = 0; i < clEnv->number_devices; i++)
  {
    if ((clEnv->devices[i]->enabled != MagickFalse) &&
        (clEnv->devices[i]->program == (cl_program) NULL))
    {
      status=MagickFalse;
      break;
    }
  }
  return(status);
}

static size_t FindEnabledOpenCLDevice(MagickCLEnv clEnv)
{
  size_t
    i;

  for (i = 0; i < clEnv->number_devices; i++)
  {
    if ((clEnv->devices[i]->enabled != MagickFalse))
      break;
  }
  return(i);
}

static inline MagickBooleanType IsOpenCLDeviceSkipped(
  const MagickCLDevice device)
{
  return(((device->enabled == MagickFalse) ||
    (device->program != (cl_program) NULL)) ? MagickTrue : MagickFalse);
}

typedef struct _OpenCLDeviceCompile
{
  MagickCLEnv
    clEnv;

  char
    *accelerateKernelsBuffer;

  const char
    *options;

  size_t
    signature;

  ExceptionInfo
    *exception;
} OpenCLDeviceCompile;

static MagickBooleanType CompileOpenCLDevices(
  const OpenCLDeviceCompile *compile)
{
  const MagickCLEnv
    clEnv = compile->clEnv;

  char
    *const accelerateKernelsBuffer = compile->accelerateKernelsBuffer;

  const char
    *const options = compile->options;

  const size_t
    signature = compile->signature;

  ExceptionInfo
    *const exception = compile->exception;

  const OpenCLKernelBuild
    build = { options, exception };

  MagickBooleanType
    status;

  size_t
    i;

  status=MagickTrue;
  for (i = 0; i < clEnv->number_devices; i++)
  {
    MagickCLDevice
      device;

    size_t
      device_signature;

    device=clEnv->devices[i];
    if (IsOpenCLDeviceSkipped(device) != MagickFalse)
      continue;

    LockSemaphoreInfo(device->lock);
    if (device->program != (cl_program) NULL)
    {
      UnlockSemaphoreInfo(device->lock);
      continue;
    }
    device_signature=signature;
    device_signature^=StringSignature(device->platform_name);
    status=CompileOpenCLKernel(device,accelerateKernelsBuffer,&build,
      device_signature);
    UnlockSemaphoreInfo(device->lock);
    if (status == MagickFalse)
      break;
  }
  return(status);
}

static MagickBooleanType HasOpenCLDevices(MagickCLEnv clEnv,
  ExceptionInfo *exception)
{
  char
    *accelerateKernelsBuffer,
    options[MagickPathExtent];

  MagickBooleanType
    status;

  OpenCLDeviceCompile
    compile;

  size_t
    i;

  size_t
    signature;

  /* Check if there are enabled devices */
  i=FindEnabledOpenCLDevice(clEnv);
  if (i == clEnv->number_devices)
    return(MagickFalse);

  /* Check if we need to compile a kernel for one of the devices */
  status=HasOpenCLKernelPrograms(clEnv);
  if (status != MagickFalse)
    return(MagickTrue);

  /* Get additional options */
  (void) FormatLocaleString(options,MagickPathExtent,CLOptions,
    (float)QuantumRange,(float)CLCharQuantumScale,(float)MagickEpsilon,
    (float)MagickPI,(unsigned int)MaxMap,(unsigned int)MAGICKCORE_QUANTUM_DEPTH);

  signature=StringSignature(options);
  accelerateKernelsBuffer=(char*) AcquireQuantumMemory(1,
    strlen(accelerateKernels)+strlen(accelerateKernels2)+1);
  if (accelerateKernelsBuffer == (char*) NULL)
    return(MagickFalse);
  (void) FormatLocaleString(accelerateKernelsBuffer,strlen(accelerateKernels)+
    strlen(accelerateKernels2)+1,"%s%s",accelerateKernels,accelerateKernels2);
  signature^=StringSignature(accelerateKernelsBuffer);

  compile.clEnv=clEnv;
  compile.accelerateKernelsBuffer=accelerateKernelsBuffer;
  compile.options=options;
  compile.signature=signature;
  compile.exception=exception;
  status=CompileOpenCLDevices(&compile);
  accelerateKernelsBuffer=(char *) RelinquishMagickMemory(
    accelerateKernelsBuffer);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e O p e n C L                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeOpenCL() is used to initialize the OpenCL environment. This method
%  makes sure the devices are properly initialized and benchmarked.
%
%  The format of the InitializeOpenCL method is:
%
%    MagickBooleanType InitializeOpenCL(ExceptionInfo exception)
%
%  A description of each parameter follows:
%
%    o exception: return any errors or warnings in this structure.
%
*/

static cl_uint GetOpenCLDeviceCount(MagickCLEnv clEnv,cl_platform_id platform)
{
  char
    version[MagickPathExtent];

  cl_uint
    num;

  if (clEnv->library->clGetPlatformInfo(platform,CL_PLATFORM_VERSION,
        MagickPathExtent,version,NULL) != CL_SUCCESS)
    return(0);
  if (strncmp(version,"OpenCL 1.0 ",11) == 0)
    return(0);
  if (clEnv->library->clGetDeviceIDs(platform,
        CL_DEVICE_TYPE_CPU|CL_DEVICE_TYPE_GPU,0,NULL,&num) != CL_SUCCESS)
    return(0);
  return(num);
}

static inline char *GetOpenCLPlatformString(cl_platform_id platform,
  cl_platform_info param_name)
{
  char
    *value;

  size_t
    length;

  openCL_library->clGetPlatformInfo(platform,param_name,0,NULL,&length);
  value=(char *) AcquireCriticalMemory(length*sizeof(*value));
  openCL_library->clGetPlatformInfo(platform,param_name,length,value,NULL);
  return(value);
}

static inline char *GetOpenCLDeviceString(cl_device_id device,
  cl_device_info param_name)
{
  char
    *value;

  size_t
    length;

  openCL_library->clGetDeviceInfo(device,param_name,0,NULL,&length);
  value=(char *) AcquireCriticalMemory(length*sizeof(*value));
  openCL_library->clGetDeviceInfo(device,param_name,length,value,NULL);
  return(value);
}

static void SetOpenCLDeviceInfo(MagickCLDevice device,cl_context context,
  cl_platform_id platform,cl_device_id device_id)
{
  device->context=context;
  device->deviceID=device_id;

  device->platform_name=GetOpenCLPlatformString(platform,
    CL_PLATFORM_NAME);

  device->vendor_name=GetOpenCLPlatformString(platform,
    CL_PLATFORM_VENDOR);

  device->name=GetOpenCLDeviceString(device_id,CL_DEVICE_NAME);

  device->version=GetOpenCLDeviceString(device_id,CL_DRIVER_VERSION);

  openCL_library->clGetDeviceInfo(device_id,CL_DEVICE_MAX_CLOCK_FREQUENCY,
    sizeof(cl_uint),&device->max_clock_frequency,NULL);

  openCL_library->clGetDeviceInfo(device_id,CL_DEVICE_MAX_COMPUTE_UNITS,
    sizeof(cl_uint),&device->max_compute_units,NULL);

  openCL_library->clGetDeviceInfo(device_id,CL_DEVICE_TYPE,
    sizeof(cl_device_type),&device->type,NULL);

  openCL_library->clGetDeviceInfo(device_id,CL_DEVICE_LOCAL_MEM_SIZE,
    sizeof(cl_ulong),&device->local_memory_size,NULL);
}

static void CountOpenCLDevices(MagickCLEnv clEnv,cl_platform_id *platforms,
  const cl_uint number_platforms)
{
  cl_uint
    i,
    number_devices;

  for (i = 0; i < number_platforms; i++)
  {
    number_devices=GetOpenCLDeviceCount(clEnv,platforms[i]);
    if (number_devices == 0)
      platforms[i]=(cl_platform_id) NULL;
    else
      clEnv->number_devices+=number_devices;
  }
}

typedef struct _OpenCLPlatformDevices
{
  MagickCLEnv
    clEnv;

  cl_platform_id
    *platforms;

  cl_device_id
    *devices;

  cl_uint
    i,
    number_devices;
} OpenCLPlatformDevices;

static cl_uint AddOpenCLPlatformDevices(
  const OpenCLPlatformDevices *platform_devices,cl_uint next)
{
  const MagickCLEnv
    clEnv = platform_devices->clEnv;

  cl_platform_id
    *const platforms = platform_devices->platforms;

  cl_device_id
    *const devices = platform_devices->devices;

  const cl_uint
    i = platform_devices->i,
    number_devices = platform_devices->number_devices;

  cl_uint
    j;

  for (j = 0; j < number_devices; j++,next++)
  {
    MagickCLDevice
      device;

    device=AcquireMagickCLDevice();
    if (device == (MagickCLDevice) NULL)
      break;

    SetOpenCLDeviceInfo(device,clEnv->contexts[i],platforms[i],devices[j]);

    clEnv->devices[next]=device;
    (void) LogMagickEvent(AccelerateEvent,GetMagickModule(),
      "Found device: %s (%s)",device->name,device->platform_name);
  }
  return(next);
}

typedef struct _OpenCLPlatformLoad
{
  MagickCLEnv
    clEnv;

  cl_platform_id
    *platforms;

  cl_device_id
    *devices;
} OpenCLPlatformLoad;

static void LoadOpenCLPlatformDevices(const OpenCLPlatformLoad *platform_load,
  cl_uint i,cl_uint *next)
{
  const MagickCLEnv
    clEnv = platform_load->clEnv;

  cl_platform_id
    *const platforms = platform_load->platforms;

  cl_device_id
    *const devices = platform_load->devices;

  OpenCLPlatformDevices
    platform_devices;

  cl_context_properties
    properties[3];

  cl_int
    status;

  cl_uint
    number_devices;

  if (platforms[i] == (cl_platform_id) NULL)
    return;

  status=clEnv->library->clGetDeviceIDs(platforms[i],CL_DEVICE_TYPE_CPU |
    CL_DEVICE_TYPE_GPU,(cl_uint) clEnv->number_devices,devices,&number_devices);
  if (status != CL_SUCCESS)
    return;

  properties[0]=CL_CONTEXT_PLATFORM;
  properties[1]=(cl_context_properties) platforms[i];
  properties[2]=0;
  clEnv->contexts[i]=openCL_library->clCreateContext(properties,number_devices,
    devices,NULL,NULL,&status);
  if (status != CL_SUCCESS)
    return;

  platform_devices.clEnv=clEnv;
  platform_devices.platforms=platforms;
  platform_devices.devices=devices;
  platform_devices.i=i;
  platform_devices.number_devices=number_devices;
  *next=AddOpenCLPlatformDevices(&platform_devices,*next);
}

static MagickBooleanType AcquireOpenCLContexts(MagickCLEnv clEnv,
  const cl_uint number_platforms,cl_device_id *devices,
  cl_platform_id *platforms)
{
  clEnv->number_contexts=(size_t) number_platforms;
  clEnv->contexts=(cl_context *) AcquireQuantumMemory(clEnv->number_contexts,
    sizeof(cl_context));
  if (clEnv->contexts == (cl_context *) NULL)
    {
      devices=(cl_device_id *) RelinquishMagickMemory(devices);
      platforms=(cl_platform_id *) RelinquishMagickMemory(platforms);
      RelinquishMagickCLDevices(clEnv);
      return(MagickFalse);
    }
  (void) memset(clEnv->contexts,0,clEnv->number_contexts*sizeof(cl_context));
  return(MagickTrue);
}

typedef struct _OpenCLPlatformTargets
{
  cl_uint
    *number_platforms;

  cl_platform_id
    **platforms;
} OpenCLPlatformTargets;

static MagickBooleanType AcquireOpenCLPlatforms(
  const OpenCLPlatformTargets *targets)
{
  cl_uint
    *const number_platforms = targets->number_platforms;

  cl_platform_id
    **const platforms = targets->platforms;

  *number_platforms=0;
  if (openCL_library->clGetPlatformIDs(0,NULL,number_platforms) != CL_SUCCESS)
    return(MagickFalse);
  if (*number_platforms == 0)
    return(MagickFalse);
  *platforms=(cl_platform_id *) AcquireQuantumMemory(1,*number_platforms*
    sizeof(cl_platform_id));
  if (*platforms == (cl_platform_id *) NULL)
    return(MagickFalse);
  if (openCL_library->clGetPlatformIDs(*number_platforms,*platforms,NULL) != CL_SUCCESS)
    {
       *platforms=(cl_platform_id *) RelinquishMagickMemory(*platforms);
       return(MagickFalse);
    }
  return(MagickTrue);
}

static void LoadOpenCLDevices(MagickCLEnv clEnv)
{
  OpenCLPlatformLoad
    platform_load;

  OpenCLPlatformTargets
    targets;

  cl_device_id
    *devices;

  cl_platform_id
    *platforms;

  cl_uint
    i,
    next,
    number_platforms;

  targets.number_platforms=(&number_platforms);
  targets.platforms=(&platforms);
  if (AcquireOpenCLPlatforms(&targets) == MagickFalse)
    return;
  CountOpenCLDevices(clEnv,platforms,number_platforms);
  if (clEnv->number_devices == 0)
    {
      platforms=(cl_platform_id *) RelinquishMagickMemory(platforms);
      return;
    }
  clEnv->devices=(MagickCLDevice *) AcquireQuantumMemory(clEnv->number_devices,
    sizeof(MagickCLDevice));
  if (clEnv->devices == (MagickCLDevice *) NULL)
    {
      RelinquishMagickCLDevices(clEnv);
      platforms=(cl_platform_id *) RelinquishMagickMemory(platforms);
      return;
    }
  (void) memset(clEnv->devices,0,clEnv->number_devices*sizeof(MagickCLDevice));
  devices=(cl_device_id *) AcquireQuantumMemory(clEnv->number_devices,
    sizeof(cl_device_id));
  if (devices == (cl_device_id *) NULL)
    {
      platforms=(cl_platform_id *) RelinquishMagickMemory(platforms);
      RelinquishMagickCLDevices(clEnv);
      return;
    }
  (void) memset(devices,0,clEnv->number_devices*sizeof(cl_device_id));
  if (AcquireOpenCLContexts(clEnv,number_platforms,devices,
      platforms) == MagickFalse)
    return;
  next=0;
  platform_load.clEnv=clEnv;
  platform_load.platforms=platforms;
  platform_load.devices=devices;
  for (i = 0; i < number_platforms; i++)
    LoadOpenCLPlatformDevices(&platform_load,i,&next);
  if (next != clEnv->number_devices)
    RelinquishMagickCLDevices(clEnv);
  platforms=(cl_platform_id *) RelinquishMagickMemory(platforms);
  devices=(cl_device_id *) RelinquishMagickMemory(devices);
}

MagickPrivate MagickBooleanType InitializeOpenCL(MagickCLEnv clEnv,
  ExceptionInfo *exception)
{
  LockSemaphoreInfo(clEnv->lock);
  if (clEnv->initialized != MagickFalse)
    {
      UnlockSemaphoreInfo(clEnv->lock);
      return(HasOpenCLDevices(clEnv,exception));
    }
  if (LoadOpenCLLibrary() != MagickFalse)
    {
      clEnv->library=openCL_library;
      LoadOpenCLDevices(clEnv);
      if (clEnv->number_devices > 0)
        AutoSelectOpenCLDevices(clEnv);
    }
  clEnv->initialized=MagickTrue;
  UnlockSemaphoreInfo(clEnv->lock);
  return(HasOpenCLDevices(clEnv,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L o a d O p e n C L L i b r a r y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadOpenCLLibrary() load and binds the OpenCL library.
%
%  The format of the LoadOpenCLLibrary method is:
%
%    MagickBooleanType LoadOpenCLLibrary(void)
%
*/

void *OsLibraryGetFunctionAddress(void *library,const char *functionName)
{
  if ((library == (void *) NULL) || (functionName == (const char *) NULL))
    return (void *) NULL;
  return lt_dlsym(library,functionName);
}

static MagickBooleanType BindOpenCLFunctions()
{
#ifdef MAGICKCORE_HAVE_OPENCL_CL_H
#define BIND(X) openCL_library->X= &X;
#else
  (void) memset(openCL_library,0,sizeof(MagickLibrary));
#ifdef MAGICKCORE_WINDOWS_SUPPORT
  openCL_library->library=(void *)lt_dlopen("OpenCL.dll");
#else
  openCL_library->library=(void *)lt_dlopen("libOpenCL.so");
#endif
#define BIND(X) \
  if ((openCL_library->X=(MAGICKpfn_##X)OsLibraryGetFunctionAddress(openCL_library->library,#X)) == NULL) \
    return(MagickFalse);
#endif

  if (openCL_library->library == (void*) NULL)
    return(MagickFalse);

  BIND(clGetPlatformIDs);
  BIND(clGetPlatformInfo);

  BIND(clGetDeviceIDs);
  BIND(clGetDeviceInfo);

  BIND(clCreateBuffer);
  BIND(clReleaseMemObject);
  BIND(clRetainMemObject);

  BIND(clCreateContext);
  BIND(clReleaseContext);

  BIND(clCreateCommandQueue);
  BIND(clReleaseCommandQueue);
  BIND(clFlush);
  BIND(clFinish);

  BIND(clCreateProgramWithSource);
  BIND(clCreateProgramWithBinary);
  BIND(clReleaseProgram);
  BIND(clBuildProgram);
  BIND(clGetProgramBuildInfo);
  BIND(clGetProgramInfo);

  BIND(clCreateKernel);
  BIND(clReleaseKernel);
  BIND(clSetKernelArg);
  BIND(clGetKernelInfo);

  BIND(clEnqueueReadBuffer);
  BIND(clEnqueueMapBuffer);
  BIND(clEnqueueUnmapMemObject);
  BIND(clEnqueueNDRangeKernel);

  BIND(clGetEventInfo);
  BIND(clWaitForEvents);
  BIND(clReleaseEvent);
  BIND(clRetainEvent);
  BIND(clSetEventCallback);

  BIND(clGetEventProfilingInfo);

  return(MagickTrue);
}

static MagickBooleanType LoadOpenCLLibrary(void)
{
  openCL_library=(MagickLibrary *) AcquireMagickMemory(sizeof(MagickLibrary));
  if (openCL_library == (MagickLibrary *) NULL)
    return(MagickFalse);

  if (BindOpenCLFunctions() == MagickFalse)
    {
      openCL_library=(MagickLibrary *)RelinquishMagickMemory(openCL_library);
      return(MagickFalse);
    }

  return(MagickTrue);
}
#endif
