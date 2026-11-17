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
% EEEEE  N   N  V   V  IIIII  RRRR    OOO   N   N  M   M  EEEEE  N   N  TTTTT %
% E      NN  N  V   V    I    R   R  O   O  NN  N  MM MM  E      NN  N    T   %
% EEE    N N N  V   V    I    RRRR   O   O  N N N  M M M  EEE    N N N    T   %
% E      N  NN   V V     I    R R    O   O  N  NN  M   M  E      N  NN    T   %
% EEEEE  N   N    V    IIIII  R  R    OOO   N   N  M   M  EEEEE  N   N    T   %
%                                                                             %
%                                                                             %
%                    MagickCore OpenCL Environment Methods                    %
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
#include "MagickCore/opencl-environment-private.h"

#if defined(MAGICKCORE_OPENCL_SUPPORT)
#if defined(MAGICKCORE_LTDL_DELEGATE)
#include "ltdl.h"
#endif

/* Cached location of the OpenCL cache files */
char
  *cache_directory;
SemaphoreInfo
  *cache_directory_lock;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D u m p O p e n C L P r o f i l e D a t a                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DumpOpenCLProfileData() dumps the kernel profile data.
%
%  The format of the DumpProfileData method is:
%
%      void DumpProfileData()
%
*/

#define OpenCLLog(message) \
   fwrite(message,sizeof(char),strlen(message),log); \
   fwrite("\n",sizeof(char),1,log);

static void DumpOpenCLDeviceProfile(FILE *log,const size_t i)
{
  char
    buf[4096],
    indent[160];

  MagickCLDevice
    device;

  size_t
    j;

  device=default_CLEnv->devices[i];
  if ((device->profile_kernels == MagickFalse) ||
      (device->profile_records == (KernelProfileRecord *) NULL))
    return;

  OpenCLLog("====================================================");
  fprintf(log,"Device:  %s\n",device->name);
  fprintf(log,"Version: %s\n",device->version);
  OpenCLLog("====================================================");
  OpenCLLog("                     average   calls     min     max");
  OpenCLLog("                     -------   -----     ---     ---");
  j=0;
  while (device->profile_records[j] != (KernelProfileRecord) NULL)
  {
    KernelProfileRecord
      profile;

    profile=device->profile_records[j];
    (void) CopyMagickString(indent,"                              ",
      sizeof(indent));
    (void) CopyMagickString(indent,profile->kernel_name,MagickMin(strlen(
      profile->kernel_name),strlen(indent)));
    (void) FormatLocaleString(buf,sizeof(buf),"%s %7d %7d %7d %7d",indent,
      (int) (profile->total/profile->count),(int) profile->count,
      (int) profile->min,(int) profile->max);
    OpenCLLog(buf);
    j++;
  }
  OpenCLLog("====================================================");
  fwrite("\n\n",sizeof(char),2,log);
}

MagickPrivate void DumpOpenCLProfileData()
{
  char
    filename[MagickPathExtent];

  FILE
    *log;

  size_t
    i;

  if (default_CLEnv == (MagickCLEnv) NULL)
    return;

  for (i = 0; i < default_CLEnv->number_devices; i++)
    if (default_CLEnv->devices[i]->profile_kernels != MagickFalse)
      break;
  if (i == default_CLEnv->number_devices)
    return;

  (void) FormatLocaleString(filename,MagickPathExtent,"%s%s%s",
    GetOpenCLCacheDirectory(),DirectorySeparator,"ImageMagickOpenCL.log");

  log=fopen_utf8(filename,"wb");
  if (log == (FILE *) NULL)
    return;
  for (i = 0; i < default_CLEnv->number_devices; i++)
    DumpOpenCLDeviceProfile(log,i);
  fclose(log);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t C u r r e n t O p e n C L E n v                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCurrentOpenCLEnv() returns the current OpenCL env
%
%  The format of the GetCurrentOpenCLEnv method is:
%
%      MagickCLEnv GetCurrentOpenCLEnv()
%
*/

MagickPrivate MagickCLEnv GetCurrentOpenCLEnv(void)
{
  if (default_CLEnv != (MagickCLEnv) NULL)
  {
    if ((default_CLEnv->benchmark_thread_id != (MagickThreadType) 0) &&
        (default_CLEnv->benchmark_thread_id != GetMagickThreadId()))
      return((MagickCLEnv) NULL);
    else
      return(default_CLEnv);
  }

  if (GetOpenCLCacheDirectory() == (char *) NULL)
    return((MagickCLEnv) NULL);

  if (openCL_lock == (SemaphoreInfo *) NULL)
    ActivateSemaphoreInfo(&openCL_lock);

  LockSemaphoreInfo(openCL_lock);
  if (default_CLEnv == (MagickCLEnv) NULL)
    default_CLEnv=AcquireMagickCLEnv();
  UnlockSemaphoreInfo(openCL_lock);

  return(default_CLEnv);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   O p e n C L T e r m i n u s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OpenCLTerminus() destroys the OpenCL component.
%
%  The format of the OpenCLTerminus method is:
%
%      OpenCLTerminus(void)
%
*/

MagickPrivate void OpenCLTerminus()
{
  DumpOpenCLProfileData();
  if (cache_directory != (char *) NULL)
    cache_directory=DestroyString(cache_directory);
  if (cache_directory_lock != (SemaphoreInfo *) NULL)
    RelinquishSemaphoreInfo(&cache_directory_lock);
  if (default_CLEnv != (MagickCLEnv) NULL)
    default_CLEnv=RelinquishMagickCLEnv(default_CLEnv);
  if (openCL_lock != (SemaphoreInfo *) NULL)
    RelinquishSemaphoreInfo(&openCL_lock);
  if (openCL_library != (MagickLibrary *) NULL)
    {
      if (openCL_library->library != (void *) NULL)
        (void) lt_dlclose(openCL_library->library);
      openCL_library=(MagickLibrary *) RelinquishMagickMemory(openCL_library);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e l i n q u i s h M a g i c k C L D e v i c e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RelinquishMagickCLDevice() releases the OpenCL device
%
%  The format of the RelinquishMagickCLDevice method is:
%
%      MagickCLDevice RelinquishMagickCLDevice(MagickCLDevice device)
%
%  A description of each parameter follows:
%
%    o device: the OpenCL device to be released.
%
*/

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e l i n q u i s h M a g i c k C L E n v                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RelinquishMagickCLEnv() releases the OpenCL environment
%
%  The format of the RelinquishMagickCLEnv method is:
%
%      MagickCLEnv RelinquishMagickCLEnv(MagickCLEnv device)
%
%  A description of each parameter follows:
%
%    o clEnv: the OpenCL environment to be released.
%
*/

#endif
