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

  MagickCore private methods of opencl-initialize.c: its typedefs, the
  accelerate timer, device comparison and acquisition, and the reader of the
  OpenCL device benchmark profile; included by that file only, inside its
  MAGICKCORE_OPENCL_SUPPORT conditional, after ltdl.h.
*/
#ifndef MAGICKCORE_OPENCL_INITIALIZE_BENCHMARK_PRIVATE_H
#define MAGICKCORE_OPENCL_INITIALIZE_BENCHMARK_PRIVATE_H

/*
  Typedef declarations.
*/
typedef struct
{
  long long freq;
  long long clocks;
  long long start;
} AccelerateTimer;

typedef struct
{
  char
    *name,
    *platform_name,
    *vendor_name,
    *version;

  cl_uint
    max_clock_frequency,
    max_compute_units;

  double
    score;
} MagickCLDeviceBenchmark;

/*
  Define declarations.
*/
#define IMAGEMAGICK_PROFILE_FILE "ImagemagickOpenCLDeviceProfile.xml"

/*
  Forward declarations.
*/
static MagickBooleanType
  HasOpenCLDevices(MagickCLEnv,ExceptionInfo *),
  LoadOpenCLLibrary(void);
static void
  BenchmarkOpenCLDevices(MagickCLEnv);

static inline MagickBooleanType IsSameOpenCLDevice(MagickCLDevice a,
  MagickCLDevice b)
{
  if (LocaleCompare(a->platform_name,b->platform_name) != 0)
    return(MagickFalse);
  if (LocaleCompare(a->vendor_name,b->vendor_name) != 0)
    return(MagickFalse);
  if (LocaleCompare(a->name,b->name) != 0)
    return(MagickFalse);
  if (LocaleCompare(a->version,b->version) != 0)
    return(MagickFalse);
  if (a->max_clock_frequency != b->max_clock_frequency)
    return(MagickFalse);
  if (a->max_compute_units != b->max_compute_units)
    return(MagickFalse);
  return(MagickTrue);
}

static inline MagickBooleanType IsBenchmarkedOpenCLDevice(MagickCLDevice a,
  MagickCLDeviceBenchmark *b)
{
  if (LocaleCompare(a->platform_name,b->platform_name) != 0)
    return(MagickFalse);
  if (LocaleCompare(a->vendor_name,b->vendor_name) != 0)
    return(MagickFalse);
  if (LocaleCompare(a->name,b->name) != 0)
    return(MagickFalse);
  if (LocaleCompare(a->version,b->version) != 0)
    return(MagickFalse);
  if (a->max_clock_frequency != b->max_clock_frequency)
    return(MagickFalse);
  if (a->max_compute_units != b->max_compute_units)
    return(MagickFalse);
  return(MagickTrue);
}

static inline void InitAccelerateTimer(AccelerateTimer *timer)
{
#ifdef _WIN32
  QueryPerformanceFrequency((LARGE_INTEGER*)&timer->freq);
#else
  timer->freq=(long long)1.0E3;
#endif
  timer->clocks=0;
  timer->start=0;
}

static inline double ReadAccelerateTimer(AccelerateTimer *timer)
{
  return (double)timer->clocks/(double)timer->freq;
}

static inline void StartAccelerateTimer(AccelerateTimer* timer)
{
#ifdef _WIN32
  QueryPerformanceCounter((LARGE_INTEGER*)&timer->start);
#else
  struct timeval
    s;
  gettimeofday(&s,0);
  timer->start=(long long)s.tv_sec*(long long)1.0E3+(long long)s.tv_usec/
    (long long)1.0E3;
#endif
}

static inline void StopAccelerateTimer(AccelerateTimer *timer)
{
  long long
    n;

  n=0;
#ifdef _WIN32
  QueryPerformanceCounter((LARGE_INTEGER*)&(n));
#else
  struct timeval
    s;
  gettimeofday(&s,0);
  n=(long long)s.tv_sec*(long long)1.0E3+(long long)s.tv_usec/
    (long long)1.0E3;
#endif
  n-=timer->start;
  timer->start=0;
  timer->clocks+=n;
}

static void EnableSameOpenCLDevices(MagickCLEnv clEnv,MagickCLDevice device,
  const size_t i)
{
  size_t
    j;

  for (j = i+1; j < clEnv->number_devices; j++)
  {
    MagickCLDevice
      other_device;

    other_device=clEnv->devices[j];
    if (IsSameOpenCLDevice(device,other_device))
      other_device->enabled=MagickTrue;
  }
}

static void SelectOpenCLDevice(MagickCLEnv clEnv,cl_device_type type)
{
  MagickCLDevice
    device;

  size_t
    i;

  (void) LogMagickEvent(AccelerateEvent,GetMagickModule(),
    "Selecting device for type: %d",(int) type);
  for (i = 0; i < clEnv->number_devices; i++)
    clEnv->devices[i]->enabled=MagickFalse;

  for (i = 0; i < clEnv->number_devices; i++)
  {
    device=clEnv->devices[i];
    if (device->type != type)
      continue;

    device->enabled=MagickTrue;
    (void) LogMagickEvent(AccelerateEvent,GetMagickModule(),
      "Selected device: %s",device->name);
    EnableSameOpenCLDevices(clEnv,device,i);
  }
}

static size_t StringSignature(const char* string)
{
  size_t
    n,
    i,
    j,
    signature,
    stringLength;

  union
  {
    const char* s;
    const size_t* u;
  } p;

  stringLength=(size_t) strlen(string);
  signature=stringLength;
  n=stringLength/sizeof(size_t);
  p.s=string;
  for (i = 0; i < n; i++)
    signature^=p.u[i];
  if (n * sizeof(size_t) != stringLength)
    {
      char
        padded[4];

      j=n*sizeof(size_t);
      for (i = 0; i < 4; i++, j++)
      {
        if (j < stringLength)
          padded[i]=p.s[j];
        else
          padded[i]=0;
      }
      p.s=padded;
      signature^=p.u[0];
    }
  return(signature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e M a g i c k C L D e v i c e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireMagickCLDevice() acquires an OpenCL device
%
%  The format of the AcquireMagickCLDevice method is:
%
%      MagickCLDevice AcquireMagickCLDevice()
%
*/

static MagickCLDevice AcquireMagickCLDevice()
{
  MagickCLDevice
    device;

  device=(MagickCLDevice) AcquireMagickMemory(sizeof(*device));
  if (device != NULL)
  {
    (void) memset(device,0,sizeof(*device));
    ActivateSemaphoreInfo(&device->lock);
    device->score=MAGICKCORE_OPENCL_UNDEFINED_SCORE;
    device->command_queues_index=-1;
    device->enabled=MagickTrue;
  }
  return(device);
}

#if !MAGICKCORE_ZERO_CONFIGURATION_SUPPORT
static MagickCLDeviceBenchmark* RelinquishDeviceBenchmark(
  MagickCLDeviceBenchmark *device_benchmark)
{
  if (device_benchmark == (MagickCLDeviceBenchmark*) NULL)
    return((MagickCLDeviceBenchmark *) NULL);

  device_benchmark->platform_name=(char *) RelinquishMagickMemory(
    device_benchmark->platform_name);
  device_benchmark->vendor_name=(char *) RelinquishMagickMemory(
    device_benchmark->vendor_name);
  device_benchmark->name=(char *) RelinquishMagickMemory(
    device_benchmark->name);
  device_benchmark->version=(char *) RelinquishMagickMemory(
    device_benchmark->version);
  return((MagickCLDeviceBenchmark *) RelinquishMagickMemory(
    device_benchmark));
}

static void SetOpenCLBenchmarkScore(MagickCLEnv clEnv,
  MagickCLDeviceBenchmark *device_benchmark)
{
  size_t
    i;

  if (LocaleCompare(device_benchmark->name,"CPU") == 0)
    clEnv->cpu_score=device_benchmark->score;
  else
    {
      MagickCLDevice
        device;

      /*
        Set the score for all devices that match this device.
      */
      for (i = 0; i < clEnv->number_devices; i++)
      {
        device=clEnv->devices[i];
        if (IsBenchmarkedOpenCLDevice(device,device_benchmark))
          device->score=device_benchmark->score;
      }
    }
}

static int CloseDoctypeBracket(int bracket_depth)
{
  if (bracket_depth > 0)
    bracket_depth--;
  return(bracket_depth);
}

typedef struct _OpenCLBenchmarkTokens
{
  MagickCLEnv
    clEnv;

  char
    *keyword,
    *token;

  size_t
    extent;
} OpenCLBenchmarkTokens;

static const char *SkipOpenCLBenchmarkComment(const char *q,
  const OpenCLBenchmarkTokens *tokens)
{
  const size_t
    extent = tokens->extent;

  char
    *const token = tokens->token;

  while ((LocaleNCompare(q,"->",2) != 0) && (*q != '\0'))
    (void) GetNextToken(q,&q,extent,token);
  return(q);
}

static MagickCLDeviceBenchmark *EndOpenCLDeviceBenchmark(MagickCLEnv clEnv,
  MagickCLDeviceBenchmark *device_benchmark)
{
  if (device_benchmark->score != MAGICKCORE_OPENCL_UNDEFINED_SCORE)
    SetOpenCLBenchmarkScore(clEnv,device_benchmark);
  device_benchmark=RelinquishDeviceBenchmark(device_benchmark);
  return(device_benchmark);
}

static void SetOpenCLBenchmarkVendorVersionAttribute(
  MagickCLDeviceBenchmark *device_benchmark,char *keyword,char *token)
{
  switch (*keyword)
  {
    case 'V':
    case 'v':
    {
      if (LocaleCompare((char *) keyword,"vendor") == 0)
        device_benchmark->vendor_name=ConstantString(token);
      if (LocaleCompare((char *) keyword,"version") == 0)
        device_benchmark->version=ConstantString(token);
      break;
    }
    default:
      break;
  }
}

static void SetOpenCLBenchmarkScoreAttribute(
  MagickCLDeviceBenchmark *device_benchmark,char *keyword,char *token)
{
  switch (*keyword)
  {
    case 'S':
    case 's':
    {
      if (LocaleCompare((char *) keyword,"score") == 0)
        device_benchmark->score=StringToDouble(token,(char **) NULL);
      break;
    }
    default:
    {
      SetOpenCLBenchmarkVendorVersionAttribute(device_benchmark,keyword,token);
      break;
    }
  }
}

static void SetOpenCLBenchmarkNamePlatformAttribute(
  MagickCLDeviceBenchmark *device_benchmark,
  const OpenCLBenchmarkTokens *tokens)
{
  char
    *const keyword = tokens->keyword,
    *const token = tokens->token;

  switch (*keyword)
  {
    case 'N':
    case 'n':
    {
      if (LocaleCompare((char *) keyword,"name") == 0)
        device_benchmark->name=ConstantString(token);
      break;
    }
    case 'P':
    case 'p':
    {
      if (LocaleCompare((char *) keyword,"platform") == 0)
        device_benchmark->platform_name=ConstantString(token);
      break;
    }
    default:
    {
      SetOpenCLBenchmarkScoreAttribute(device_benchmark,keyword,token);
      break;
    }
  }
}

static void SetOpenCLBenchmarkAttribute(
  MagickCLDeviceBenchmark *device_benchmark,
  const OpenCLBenchmarkTokens *tokens)
{
  char
    *const keyword = tokens->keyword,
    *const token = tokens->token;

  switch (*keyword)
  {
    case 'M':
    case 'm':
    {
      if (LocaleCompare((char *) keyword,"maxClockFrequency") == 0)
        {
          device_benchmark->max_clock_frequency=StringToInteger(token);
          break;
        }
      if (LocaleCompare((char *) keyword,"maxComputeUnits") == 0)
        {
          device_benchmark->max_compute_units=StringToInteger(token);
          break;
        }
      break;
    }
    default:
    {
      SetOpenCLBenchmarkNamePlatformAttribute(device_benchmark,tokens);
      break;
    }
  }
}

#include "MagickCore/opencl-initialize-doctype-private.h"

static void ReadOpenCLBenchmarkAttribute(const OpenCLBenchmarkTokens *tokens,
  MagickCLDeviceBenchmark **device_benchmark,const char **q)
{
  const MagickCLEnv
    clEnv = tokens->clEnv;

  char
    *const keyword = tokens->keyword,
    *const token = tokens->token;

  const size_t
    extent = tokens->extent;

  if ((*device_benchmark) == (MagickCLDeviceBenchmark *) NULL)
    return;
  if (LocaleCompare(keyword,"/>") == 0)
    {
      (*device_benchmark)=EndOpenCLDeviceBenchmark(clEnv,(*device_benchmark));
      return;
    }
  (void) GetNextToken((*q),(const char **) NULL,extent,token);
  if (*token != '=')
    return;
  (void) GetNextToken((*q),q,extent,token);
  (void) GetNextToken((*q),q,extent,token);
  SetOpenCLBenchmarkAttribute((*device_benchmark),tokens);
}

static MagickBooleanType SkipOpenCLBenchmarkMarkup(
  const OpenCLBenchmarkTokens *tokens,const char **q)
{
  const char
    *const keyword = tokens->keyword;

  if (LocaleNCompare(keyword,"<!DOCTYPE",9) == 0)
    {
      (*q)=SkipOpenCLBenchmarkDoctype((*q));
      return(MagickTrue);
    }
  if (LocaleNCompare(keyword,"<!--",4) == 0)
    {
      /*
        Comment element.
      */
      (*q)=SkipOpenCLBenchmarkComment((*q),tokens);
      return(MagickTrue);
    }
  return(MagickFalse);
}

static MagickBooleanType ReadOpenCLBenchmarkMarkup(
  const OpenCLBenchmarkTokens *tokens,const char **q,
  MagickCLDeviceBenchmark **device_benchmark,MagickBooleanType *broke)
{
  char
    *const keyword = tokens->keyword;

  (*broke)=MagickFalse;
  if (SkipOpenCLBenchmarkMarkup(tokens,q) != MagickFalse)
    return(MagickFalse);
  if (LocaleCompare(keyword,"<device") == 0)
    {
      /*
        Device element.
      */
      (*device_benchmark)=(MagickCLDeviceBenchmark *) AcquireQuantumMemory(1,
        sizeof(**device_benchmark));
      if ((*device_benchmark) == (MagickCLDeviceBenchmark *) NULL)
        {
          (*broke)=MagickTrue;
          return(MagickTrue);
        }
      (void) memset((*device_benchmark),0,sizeof(**device_benchmark));
      (*device_benchmark)->score=MAGICKCORE_OPENCL_UNDEFINED_SCORE;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void LoadOpenCLDeviceBenchmark(MagickCLEnv clEnv,const char *xml)
{
  char
    keyword[MagickPathExtent],
    *token;

  const char
    *q;

  MagickBooleanType
    broke = MagickFalse;

  MagickCLDeviceBenchmark
    *device_benchmark;

  size_t
    extent;

  if (xml == (char *) NULL)
    return;
  device_benchmark=(MagickCLDeviceBenchmark *) NULL;
  token=AcquireString(xml);
  extent=strlen(token)+MagickPathExtent;
  for (q=(char *) xml; *q != '\0'; )
  {
    OpenCLBenchmarkTokens
      tokens;

    /*
      Interpret XML.
    */
    (void) GetNextToken(q,&q,extent,token);
    if (*token == '\0')
      break;
    (void) CopyMagickString(keyword,token,MagickPathExtent);
    tokens.clEnv=clEnv;
    tokens.keyword=keyword;
    tokens.token=token;
    tokens.extent=extent;
    if (ReadOpenCLBenchmarkMarkup(&tokens,&q,&device_benchmark,&broke) ==
        MagickFalse)
      continue;
    if (broke != MagickFalse)
      break;
    ReadOpenCLBenchmarkAttribute(&tokens,&device_benchmark,&q);
  }
  token=(char *) RelinquishMagickMemory(token);
  device_benchmark=RelinquishDeviceBenchmark(device_benchmark);
}

static MagickBooleanType CanWriteProfileToFile(const char *filename)
{
  FILE
    *profileFile;

  profileFile=fopen_utf8(filename,"ab");

  if (profileFile == (FILE *) NULL)
    {
      (void) LogMagickEvent(AccelerateEvent,GetMagickModule(),
        "Unable to save profile to: \"%s\"",filename);
      return(MagickFalse);
    }

  fclose(profileFile);
  return(MagickTrue);
}
#endif

#endif
