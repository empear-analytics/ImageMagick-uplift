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

  MagickCore private methods shared by the OpenCL environment methods: the
  OpenCL cache directory, environment and device acquire and release helpers
  that InitializeOpenCL and the environment accessors share.
*/
#ifndef MAGICKCORE_OPENCL_ENVIRONMENT_PRIVATE_H
#define MAGICKCORE_OPENCL_ENVIRONMENT_PRIVATE_H

#include "MagickCore/opencl-private.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/string_.h"
#include "MagickCore/utility-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(MAGICKCORE_OPENCL_SUPPORT)

extern char
  *cache_directory;

extern SemaphoreInfo
  *cache_directory_lock;

/*
  Method prototypes.
*/
static MagickCLDevice
  RelinquishMagickCLDevice(MagickCLDevice);
static MagickCLEnv
  RelinquishMagickCLEnv(MagickCLEnv);

static inline void RelinquishMagickCLDevices(MagickCLEnv clEnv)
{
  size_t
    i;

  if (clEnv->devices != (MagickCLDevice *) NULL)
    {
      for (i = 0; i < clEnv->number_devices; i++)
        clEnv->devices[i]=RelinquishMagickCLDevice(clEnv->devices[i]);
      clEnv->devices=(MagickCLDevice *) RelinquishMagickMemory(clEnv->devices);
    }
  clEnv->number_devices=0;
}

static inline MagickBooleanType MagickCreateDirectory(const char *path)
{
  int
    status;

#ifdef MAGICKCORE_WINDOWS_SUPPORT
  status=_mkdir(path);
#else
  status=mkdir(path,0777);
#endif
  return(status == 0 ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType CreateOpenCLCachePath(const char *path)
{
  MagickBooleanType
    status;

  struct stat
    attributes;

  status=GetPathAttributes(path,&attributes);
  if (status == MagickFalse)
    status=MagickCreateDirectory(path);
  return(status);
}

typedef struct _OpenCLCacheSearch
{
  char
    *home,
    *path,
    *temp;
} OpenCLCacheSearch;

static inline char *CreateOpenCLCacheDirectory(
  const OpenCLCacheSearch *search)
{
  char
    *home = search->home,
    *const path = search->path,
    *temp = search->temp;

  MagickBooleanType
    status;

  /* first check if $HOME exists */
  (void) FormatLocaleString(path,MagickPathExtent,"%s",home);
  status=CreateOpenCLCachePath(path);

  /* first check if $HOME/ImageMagick exists */
  if (status != MagickFalse)
    {
      (void) FormatLocaleString(path,MagickPathExtent,
        "%s%sImageMagick",home,DirectorySeparator);

      status=CreateOpenCLCachePath(path);
    }

  if (status != MagickFalse)
    {
      temp=(char*) AcquireCriticalMemory(strlen(path)+1);
      (void) CopyMagickString(temp,path,strlen(path)+1);
    }
  home=DestroyString(home);
  return(temp);
}

static inline char *AcquireOpenCLCacheDirectory(
  const OpenCLCacheSearch *search)
{
  char
    *home = search->home,
    *const path = search->path,
    *temp = search->temp;

  MagickBooleanType
    status;

  if (home != (char *) NULL)
    {
      const OpenCLCacheSearch
        cache_search = { home, path, temp };

      temp=CreateOpenCLCacheDirectory(&cache_search);
    }
  else
    {
      home=GetEnvironmentValue("HOME");
      if (home != (char *) NULL)
        {
          /* first check if $HOME/.cache exists */
          (void) FormatLocaleString(path,MagickPathExtent,"%s%s.cache",
            home,DirectorySeparator);
          status=CreateOpenCLCachePath(path);

          /* first check if $HOME/.cache/ImageMagick exists */
          if (status != MagickFalse)
            {
              (void) FormatLocaleString(path,MagickPathExtent,
                "%s%s.cache%sImageMagick",home,DirectorySeparator,
                DirectorySeparator);
              status=CreateOpenCLCachePath(path);
            }

          if (status != MagickFalse)
            {
              temp=(char*) AcquireCriticalMemory(strlen(path)+1);
              (void) CopyMagickString(temp,path,strlen(path)+1);
            }
          home=DestroyString(home);
        }
    }
  return(temp);
}

static inline void SetOpenCLCacheDirectory(char *home,char *path,char *temp)
{
  {
    const OpenCLCacheSearch
      cache_search = { home, path, temp };

    temp=AcquireOpenCLCacheDirectory(&cache_search);
  }
  if (temp == (char *) NULL)
    {
      temp=AcquireString("?");
      (void) LogMagickEvent(AccelerateEvent,GetMagickModule(),
        "Cannot use cache directory: \"%s\"",path);
    }
  else
    (void) LogMagickEvent(AccelerateEvent,GetMagickModule(),
      "Using cache directory: \"%s\"",temp);
  cache_directory=temp;
}

static inline const char *GetOpenCLCacheDirectoryValue(void)
{
  if (*cache_directory == '?')
    return((const char *) NULL);
  return(cache_directory);
}

static inline void LockOpenCLCacheDirectory(void)
{
  if (cache_directory_lock == (SemaphoreInfo *) NULL)
    ActivateSemaphoreInfo(&cache_directory_lock);
  LockSemaphoreInfo(cache_directory_lock);
}

static inline const char *GetOpenCLCacheDirectory()
{
  if (cache_directory != (char *) NULL)
    return(GetOpenCLCacheDirectoryValue());
  {
      LockOpenCLCacheDirectory();
      if (cache_directory == (char *) NULL)
        {
          char
            *home,
            path[MagickPathExtent],
            *temp;

          temp=(char *) NULL;
          home=GetEnvironmentValue("MAGICK_OPENCL_CACHE_DIR");
          if (home == (char *) NULL)
            {
              home=GetEnvironmentValue("XDG_CACHE_HOME");
#if defined(MAGICKCORE_WINDOWS_SUPPORT) || defined(__MINGW32__)
              if (home == (char *) NULL)
                home=GetEnvironmentValue("LOCALAPPDATA");
              if (home == (char *) NULL)
                home=GetEnvironmentValue("APPDATA");
              if (home == (char *) NULL)
                home=GetEnvironmentValue("USERPROFILE");
#endif
            }

          SetOpenCLCacheDirectory(home,path,temp);
        }
      UnlockSemaphoreInfo(cache_directory_lock);
  }
  return(GetOpenCLCacheDirectoryValue());
}

static inline MagickBooleanType IsOpenCLDeviceOptionEnabled(
  const char *option)
{
  return(((IsStringTrue(option) != MagickFalse) ||
    (strcmp(option,"GPU") == 0) ||
    (strcmp(option,"CPU") == 0)) ? MagickTrue : MagickFalse);
}

static inline MagickCLEnv AcquireMagickCLEnv(void)
{
  char
    *option;

  MagickCLEnv
    clEnv;

  clEnv=(MagickCLEnv) AcquireMagickMemory(sizeof(*clEnv));
  if (clEnv != (MagickCLEnv) NULL)
  {
    (void) memset(clEnv,0,sizeof(*clEnv));
    ActivateSemaphoreInfo(&clEnv->lock);
    clEnv->cpu_score=MAGICKCORE_OPENCL_UNDEFINED_SCORE;
    clEnv->enabled=MagickFalse;
    option=GetEnvironmentValue("MAGICK_OCL_DEVICE");
    if (option != (const char *) NULL)
      {
        if (IsOpenCLDeviceOptionEnabled(option) != MagickFalse)
          clEnv->enabled=MagickTrue;
        option=DestroyString(option);
      }
  }
  return clEnv;
}

static inline MagickCLDevice RelinquishMagickCLDevice(MagickCLDevice device)
{
  if (device == (MagickCLDevice) NULL)
    return((MagickCLDevice) NULL);

  device->platform_name=(char *) RelinquishMagickMemory(device->platform_name);
  device->vendor_name=(char *) RelinquishMagickMemory(device->vendor_name);
  device->name=(char *) RelinquishMagickMemory(device->name);
  device->version=(char *) RelinquishMagickMemory(device->version);
  if (device->program != (cl_program) NULL)
    (void) openCL_library->clReleaseProgram(device->program);
  while (device->command_queues_index >= 0)
    (void) openCL_library->clReleaseCommandQueue(
      device->command_queues[device->command_queues_index--]);
  RelinquishSemaphoreInfo(&device->lock);
  return((MagickCLDevice) RelinquishMagickMemory(device));
}

static inline MagickCLEnv RelinquishMagickCLEnv(MagickCLEnv clEnv)
{
  if (clEnv == (MagickCLEnv) NULL)
    return((MagickCLEnv) NULL);

  RelinquishSemaphoreInfo(&clEnv->lock);
  RelinquishMagickCLDevices(clEnv);
  if (clEnv->contexts != (cl_context *) NULL)
    {
      ssize_t
        i;

      for (i=0; i < (ssize_t) clEnv->number_contexts; i++)
        if (clEnv->contexts[i] != (cl_context) NULL)
          (void) openCL_library->clReleaseContext(clEnv->contexts[i]);
      clEnv->contexts=(cl_context *) RelinquishMagickMemory(clEnv->contexts);
    }
  return((MagickCLEnv) RelinquishMagickMemory(clEnv));
}

#endif /* MAGICKCORE_OPENCL_SUPPORT */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
