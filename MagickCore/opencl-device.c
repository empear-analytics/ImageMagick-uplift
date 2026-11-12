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
%                  DDDD   EEEEE  V   V  IIIII   CCCC  EEEEE                   %
%                  D   D  E      V   V    I    C      E                       %
%                  D   D  EEE    V   V    I    C      EEE                     %
%                  D   D  E       V V     I    C      E                       %
%                  DDDD   EEEEE    V    IIIII   CCCC  EEEEE                   %
%                                                                             %
%                                                                             %
%                  MagickCore OpenCL Device Property Methods                  %
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t O p e n C L D e v i c e B e n c h m a r k D u r a t i o n           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOpenCLDeviceBenchmarkScore() returns the score of the benchmark for the
%  device. The score is determined by the duration of the micro benchmark so
%  that means a lower score is better than a higher score.
%
%  The format of the GetOpenCLDeviceBenchmarkScore method is:
%
%      double GetOpenCLDeviceBenchmarkScore(const MagickCLDevice device)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device.
*/

MagickExport double GetOpenCLDeviceBenchmarkScore(
  const MagickCLDevice device)
{
  if (device == (MagickCLDevice) NULL)
    return(MAGICKCORE_OPENCL_UNDEFINED_SCORE);
  return(device->score);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t O p e n C L D e v i c e E n a b l e d                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOpenCLDeviceEnabled() returns true if the device is enabled.
%
%  The format of the GetOpenCLDeviceEnabled method is:
%
%      MagickBooleanType GetOpenCLDeviceEnabled(const MagickCLDevice device)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device.
*/

MagickExport MagickBooleanType GetOpenCLDeviceEnabled(
  const MagickCLDevice device)
{
  if (device == (MagickCLDevice) NULL)
    return(MagickFalse);
  return(device->enabled);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t O p e n C L D e v i c e N a m e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOpenCLDeviceName() returns the name of the device.
%
%  The format of the GetOpenCLDeviceName method is:
%
%      const char *GetOpenCLDeviceName(const MagickCLDevice device)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device.
*/

MagickExport const char *GetOpenCLDeviceName(const MagickCLDevice device)
{
  if (device == (MagickCLDevice) NULL)
    return((const char *) NULL);
  return(device->name);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t O p e n C L D e v i c e V e n d o r N a m e                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOpenCLDeviceVendorName() returns the vendor name of the device.
%
%  The format of the GetOpenCLDeviceVendorName method is:
%
%      const char *GetOpenCLDeviceVendorName(const MagickCLDevice device)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device.
*/

MagickExport const char *GetOpenCLDeviceVendorName(const MagickCLDevice device)
{
  if (device == (MagickCLDevice) NULL)
    return((const char *) NULL);
  return(device->vendor_name);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t O p e n C L D e v i c e s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOpenCLDevices() returns the devices of the OpenCL environment at sets the
%  value of length to the number of devices that are available.
%
%  The format of the GetOpenCLDevices method is:
%
%      const MagickCLDevice *GetOpenCLDevices(size_t *length,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o length: the number of device.
%
%    o exception: return any errors or warnings in this structure.
%
*/

MagickExport MagickCLDevice *GetOpenCLDevices(size_t *length,
  ExceptionInfo *exception)
{
  MagickCLEnv
    clEnv;

  clEnv=GetCurrentOpenCLEnv();
  if (clEnv == (MagickCLEnv) NULL)
    {
      if (length != (size_t *) NULL)
        *length=0;
      return((MagickCLDevice *) NULL);
    }
  InitializeOpenCL(clEnv,exception);
  if (length != (size_t *) NULL)
    *length=clEnv->number_devices;
  return(clEnv->devices);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t O p e n C L D e v i c e T y p e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOpenCLDeviceType() returns the type of the device.
%
%  The format of the GetOpenCLDeviceType method is:
%
%      MagickCLDeviceType GetOpenCLDeviceType(const MagickCLDevice device)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device.
*/

MagickExport MagickCLDeviceType GetOpenCLDeviceType(
  const MagickCLDevice device)
{
  if (device == (MagickCLDevice) NULL)
    return(UndefinedCLDeviceType);
  if (device->type == CL_DEVICE_TYPE_GPU)
    return(GpuCLDeviceType);
  if (device->type == CL_DEVICE_TYPE_CPU)
    return(CpuCLDeviceType);
  return(UndefinedCLDeviceType);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t O p e n C L D e v i c e V e r s i o n                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOpenCLDeviceVersion() returns the version of the device.
%
%  The format of the GetOpenCLDeviceName method is:
%
%      const char *GetOpenCLDeviceVersion(MagickCLDevice device)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device.
*/

MagickExport const char *GetOpenCLDeviceVersion(const MagickCLDevice device)
{
  if (device == (MagickCLDevice) NULL)
    return((const char *) NULL);
  return(device->version);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t O p e n C L E n a b l e d                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOpenCLEnabled() returns true if OpenCL acceleration is enabled.
%
%  The format of the GetOpenCLEnabled method is:
%
%      MagickBooleanType GetOpenCLEnabled()
%
*/

static MagickBooleanType HasCurrentOpenCLEnv(MagickCLEnv *clEnv)
{
  *clEnv=GetCurrentOpenCLEnv();
  if (*clEnv == (MagickCLEnv) NULL)
    return(MagickFalse);
  return(MagickTrue);
}

MagickExport MagickBooleanType GetOpenCLEnabled(void)
{
  MagickCLEnv
    clEnv;

  if (HasCurrentOpenCLEnv(&clEnv) == MagickFalse)
    return(MagickFalse);
  return(clEnv->enabled);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t O p e n C L K e r n e l P r o f i l e R e c o r d s                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOpenCLKernelProfileRecords() returns the profile records for the
%  specified device and sets length to the number of profile records.
%
%  The format of the GetOpenCLKernelProfileRecords method is:
%
%      const KernelProfileRecord *GetOpenCLKernelProfileRecords(size *length)
%
%  A description of each parameter follows:
%
%    o length: the number of profiles records.
*/

static void CountKernelProfileRecords(const MagickCLDevice device,
  size_t *length)
{
  *length=0;
  LockSemaphoreInfo(device->lock);
  while (device->profile_records[*length] != (KernelProfileRecord) NULL)
    *length=*length+1;
  UnlockSemaphoreInfo(device->lock);
}

MagickExport const KernelProfileRecord *GetOpenCLKernelProfileRecords(
  const MagickCLDevice device,size_t *length)
{
  if ((device == (const MagickCLDevice) NULL) || (device->profile_records ==
      (KernelProfileRecord *) NULL))
  {
    if (length != (size_t *) NULL)
      *length=0;
    return((const KernelProfileRecord *) NULL);
  }
  if (length != (size_t *) NULL)
    CountKernelProfileRecords(device,length);
  return(device->profile_records);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t O p e n C L D e v i c e E n a b l e d                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetOpenCLDeviceEnabled() can be used to enable or disabled the device.
%
%  The format of the SetOpenCLDeviceEnabled method is:
%
%      void SetOpenCLDeviceEnabled(MagickCLDevice device,
%        MagickBooleanType value)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device.
%
%    o value: determines if the device should be enabled or disabled.
*/

MagickExport void SetOpenCLDeviceEnabled(MagickCLDevice device,
  const MagickBooleanType value)
{
  if (device == (MagickCLDevice) NULL)
    return;
  device->enabled=value;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t O p e n C L K e r n e l P r o f i l e E n a b l e d                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetOpenCLKernelProfileEnabled() can be used to enable or disabled the
%  kernel profiling of a device.
%
%  The format of the SetOpenCLKernelProfileEnabled method is:
%
%      void SetOpenCLKernelProfileEnabled(MagickCLDevice device,
%        MagickBooleanType value)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device.
%
%    o value: determines if kernel profiling for the device should be enabled
%             or disabled.
*/

MagickExport void SetOpenCLKernelProfileEnabled(MagickCLDevice device,
  const MagickBooleanType value)
{
  if (device == (MagickCLDevice) NULL)
    return;
  device->profile_kernels=value;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t O p e n C L E n a b l e d                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetOpenCLEnabled() can be used to enable or disable OpenCL acceleration.
%
%  The format of the SetOpenCLEnabled method is:
%
%      void SetOpenCLEnabled(MagickBooleanType)
%
%  A description of each parameter follows:
%
%    o value: specify true to enable OpenCL acceleration
*/

MagickExport MagickBooleanType SetOpenCLEnabled(const MagickBooleanType value)
{
  MagickCLEnv
    clEnv;

  clEnv=GetCurrentOpenCLEnv();
  if (clEnv == (MagickCLEnv) NULL)
    return(MagickFalse);
  clEnv->enabled=value;
  return(clEnv->enabled);
}

#else

MagickExport double GetOpenCLDeviceBenchmarkScore(
  const MagickCLDevice magick_unused(device))
{
  magick_unreferenced(device);
  return(0.0);
}

MagickExport MagickBooleanType GetOpenCLDeviceEnabled(
  const MagickCLDevice magick_unused(device))
{
  magick_unreferenced(device);
  return(MagickFalse);
}

MagickExport const char *GetOpenCLDeviceName(
  const MagickCLDevice magick_unused(device))
{
  magick_unreferenced(device);
  return((const char *) NULL);
}

MagickExport MagickCLDevice *GetOpenCLDevices(size_t *length,
  ExceptionInfo *magick_unused(exception))
{
  magick_unreferenced(exception);
  if (length != (size_t *) NULL)
    *length=0;
  return((MagickCLDevice *) NULL);
}

MagickExport MagickCLDeviceType GetOpenCLDeviceType(
  const MagickCLDevice magick_unused(device))
{
  magick_unreferenced(device);
  return(UndefinedCLDeviceType);
}

MagickExport const KernelProfileRecord *GetOpenCLKernelProfileRecords(
  const MagickCLDevice magick_unused(device),size_t *length)
{
  magick_unreferenced(device);
  if (length != (size_t *) NULL)
    *length=0;
  return((const KernelProfileRecord *) NULL);
}

MagickExport const char *GetOpenCLDeviceVersion(
  const MagickCLDevice magick_unused(device))
{
  magick_unreferenced(device);
  return((const char *) NULL);
}

MagickExport MagickBooleanType GetOpenCLEnabled(void)
{
  return(MagickFalse);
}

MagickExport void SetOpenCLDeviceEnabled(
  MagickCLDevice magick_unused(device),
  const MagickBooleanType magick_unused(value))
{
  magick_unreferenced(device);
  magick_unreferenced(value);
}

MagickExport MagickBooleanType SetOpenCLEnabled(
  const MagickBooleanType magick_unused(value))
{
  magick_unreferenced(value);
  return(MagickFalse);
}

MagickExport void SetOpenCLKernelProfileEnabled(
  MagickCLDevice magick_unused(device),
  const MagickBooleanType magick_unused(value))
{
  magick_unreferenced(device);
  magick_unreferenced(value);
}
#endif
