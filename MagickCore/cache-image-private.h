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

  MagickCore private methods of cache.c: GetImagePixelCache and its helpers,
  GetDynamicThrottlePolicy to GetImagePixelCache; included by that file only.
*/
#ifndef MAGICKCORE_CACHE_IMAGE_PRIVATE_H
#define MAGICKCORE_CACHE_IMAGE_PRIVATE_H

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t I m a g e P i x e l C a c h e                                       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImagePixelCache() ensures that there is only a single reference to the
%  pixel cache to be modified, updating the provided cache pointer to point to
%  a clone of the original pixel cache if necessary.
%
%  The format of the GetImagePixelCache method is:
%
%      Cache GetImagePixelCache(Image *image,const MagickBooleanType clone,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o clone: any value other than MagickFalse clones the cache pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickBooleanType GetDynamicThrottlePolicy(void)
{
  static MagickBooleanType
    check_policy = MagickTrue;

  static MagickBooleanType
    dynamic_throttle = MagickFalse;

  if (check_policy != MagickFalse)
    {
      char *value = GetPolicyValue("resource:dynamic-throttle");
      if (value != (char *) NULL)
        {
          dynamic_throttle=IsStringTrue(value);
          value=DestroyString(value);
        }
      check_policy=MagickFalse;
    }
  return(dynamic_throttle);
}

static inline MagickBooleanType IsCacheMorphologyChanged(const Image *image,
  const CacheInfo *cache_info,const PixelChannelMap *p,const PixelChannelMap *q)
{
  return(((image->storage_class != cache_info->storage_class) ||
    (image->colorspace != cache_info->colorspace) ||
    (image->alpha_trait != cache_info->alpha_trait) ||
    (image->channels != cache_info->channels) ||
    (image->columns != cache_info->columns) ||
    (image->rows != cache_info->rows) ||
    (image->number_channels != cache_info->number_channels) ||
    (memcmp(p,q,image->number_channels*sizeof(*p)) != 0) ||
    (image->metacontent_extent != cache_info->metacontent_extent) ||
    (cache_info->nexus_info == (NexusInfo **) NULL)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType ValidatePixelCacheMorphology(
  const Image *magick_restrict image)
{
  const CacheInfo
    *magick_restrict cache_info;

  const PixelChannelMap
    *magick_restrict p,
    *magick_restrict q;

  /*
    Does the image match the pixel cache morphology?
  */
  cache_info=(CacheInfo *) image->cache;
  p=image->channel_map;
  q=cache_info->channel_map;
  if (IsCacheMorphologyChanged(image,cache_info,p,q) != MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _ImagePixelCacheClone
{
  Image
    *image;

  MagickBooleanType
    clone;

  CacheInfo
    *cache_info;

  ExceptionInfo
    *exception;
} ImagePixelCacheClone;

static void CloneImagePixelCache(const ImagePixelCacheClone *pixel_cache_clone,
  MagickBooleanType *status,MagickBooleanType *destroy)
{
  Image
    *const image = pixel_cache_clone->image;

  const MagickBooleanType
    clone = pixel_cache_clone->clone;

  CacheInfo
    *const cache_info = pixel_cache_clone->cache_info;

  ExceptionInfo
    *const exception = pixel_cache_clone->exception;

  CacheInfo
    *clone_info;

  Image
    clone_image;

  /*
    Clone pixel cache.
  */
  clone_image=(*image);
  clone_image.semaphore=AcquireSemaphoreInfo();
  clone_image.reference_count=1;
  clone_image.cache=ClonePixelCache(cache_info);
  clone_info=(CacheInfo *) clone_image.cache;
  *status=OpenPixelCache(&clone_image,IOMode,exception);
  if (*status == MagickFalse)
    clone_info=(CacheInfo *) DestroyPixelCache(clone_info);
  else
    {
      if (clone != MagickFalse)
        *status=ClonePixelCacheRepository(clone_info,cache_info,
          exception);
      if (*status == MagickFalse)
        clone_info=(CacheInfo *) DestroyPixelCache(clone_info);
      else
        {
          *destroy=MagickTrue;
          image->cache=clone_info;
        }
    }
  RelinquishSemaphoreInfo(&clone_image.semaphore);
}

static Cache GetImagePixelCache(Image *image,const MagickBooleanType clone,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  MagickBooleanType
    destroy,
    status = MagickTrue;

  static MagickSizeType
    cpu_throttle = MagickResourceInfinity,
    cycles = 0;

  if (IsImageTTLExpired(image) != MagickFalse)
    {
#if defined(ESTALE)
      errno=ESTALE;
#endif
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"TimeLimitExceeded","`%s'",image->filename);
      return((Cache) NULL);
    }
  if (cpu_throttle == MagickResourceInfinity)
    cpu_throttle=GetMagickResourceLimit(ThrottleResource);
  if ((GetDynamicThrottlePolicy() != MagickFalse) && ((cycles % 65536) == 0))
    {
      const double
        max_delay = 50.0,
        sensitivity = 0.3;

      double
        load,
        load_average = 0.0;

      /*
        Dynamically throttle the CPU relative to the load average.
      */
#if defined(MAGICKCORE_HAVE_GETLOADAVG)
      if (getloadavg(&load_average,1) != 1)
        load_average=0.0;
#endif
      load=MagickMax(load_average-(double) GetOpenMPMaximumThreads(),0.0);
      cpu_throttle=(MagickSizeType) (max_delay*(1.0-exp(-sensitivity*load)));
    }
  if ((cpu_throttle != 0) && ((cycles % 4096) == 0))
    MagickDelay(cpu_throttle);
  cycles++;
  LockSemaphoreInfo(image->semaphore);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  CopyOpenCLBuffer(cache_info);
#endif
  destroy=MagickFalse;
  if ((cache_info->reference_count > 1) || (cache_info->mode == ReadMode))
    {
      LockSemaphoreInfo(cache_info->semaphore);
      if ((cache_info->reference_count > 1) || (cache_info->mode == ReadMode))
        {
          const ImagePixelCacheClone
            pixel_cache_clone = { image, clone, cache_info, exception };

          CloneImagePixelCache(&pixel_cache_clone,&status,&destroy);
        }
      UnlockSemaphoreInfo(cache_info->semaphore);
    }
  if (destroy != MagickFalse)
    cache_info=(CacheInfo *) DestroyPixelCache(cache_info);
  if (status != MagickFalse)
    {
      /*
        Ensure the image matches the pixel cache morphology.
      */
      if (ValidatePixelCacheMorphology(image) == MagickFalse)
        {
          image->type=UndefinedType;
          status=OpenPixelCache(image,IOMode,exception);
          cache_info=(CacheInfo *) image->cache;
          if (cache_info->file != -1)
            (void) ClosePixelCacheOnDisk(cache_info);
        }
    }
  UnlockSemaphoreInfo(image->semaphore);
  if (status == MagickFalse)
    return((Cache) NULL);
  return(image->cache);
}

#endif
