/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  V   V  IIIII  SSSSS  U   U   AAA   L                       %
%                  V   V    I    SS     U   U  A   A  L                       %
%                  V   V    I     SSS   U   U  AAAAA  L                       %
%                   V V     I       SS  U   U  A   A  L                       %
%                    V    IIIII  SSSSS   UUU   A   A  LLLLL                   %
%                                                                             %
%               EEEEE  FFFFF  FFFFF  EEEEE   CCCC  TTTTT  SSSSS               %
%               E      F      F      E      C        T    SS                  %
%               EEE    FFF    FFF    EEE    C        T     SSS                %
%               E      F      F      E      C        T       SS               %
%               EEEEE  F      F      EEEEE   CCCC    T    SSSSS               %
%                                                                             %
%                      N   N   OOO   IIIII  SSSSS  EEEEE                      %
%                      NN  N  O   O    I    SS     E                          %
%                      N N N  O   O    I     SSS   EEE                        %
%                      N  NN  O   O    I       SS  E                          %
%                      N   N   OOO   IIIII  SSSSS  EEEEE                      %
%                                                                             %
%                                                                             %
%                         MagickCore Add Noise Method                         %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                 October 1996                                %
%                                                                             %
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
#include "MagickCore/accelerate-private.h"
#include "MagickCore/annotate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/decorate.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/layer.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/visual-effects.h"
#include "MagickCore/visual-effects-pixel-methods-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A d d N o i s e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AddNoiseImage() adds random noise to the image.
%
%  The format of the AddNoiseImage method is:
%
%      Image *AddNoiseImage(const Image *image,const NoiseType noise_type,
%        const double attenuate,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o channel: the channel type.
%
%    o noise_type:  The type of noise: Uniform, Gaussian, Multiplicative,
%      Impulse, Laplacian, or Poisson.
%
%    o attenuate:  attenuate the random distribution.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _AddNoisePixel
{
  const Image
    *image,
    *noise_image;

  NoiseType
    noise_type;

  double
    attenuate;
} AddNoisePixel;

static void AddNoisePixelChannels(const AddNoisePixel *noise_pixel,
  const Quantum *magick_restrict p,Quantum *magick_restrict q,
  RandomInfo *magick_restrict random_info)
{
  const Image
    *const image = noise_pixel->image,
    *const noise_image = noise_pixel->noise_image;

  const NoiseType
    noise_type = noise_pixel->noise_type;

  const double
    attenuate = noise_pixel->attenuate;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait noise_traits=GetPixelChannelTraits(noise_image,channel);
    if (IsPixelChannelPairUndefined(traits,noise_traits) != MagickFalse)
      continue;
    if ((noise_traits & CopyPixelTrait) != 0)
      {
        SetPixelChannel(noise_image,channel,p[i],q);
        continue;
      }
    SetPixelChannel(noise_image,channel,ClampToQuantum(
      GenerateDifferentialNoise(random_info,p[i],noise_type,attenuate)),
      q);
  }
}

typedef struct _AddNoiseRow
{
  const Image
    *image;

  Image
    *noise_image;

  CacheView
    *image_view,
    *noise_view;

  RandomInfo
    **random_info;

  NoiseType
    noise_type;

  double
    attenuate;

  ExceptionInfo
    *exception;
} AddNoiseRow;

static MagickBooleanType AddNoiseImageRow(const AddNoiseRow *noise_row,
  const ssize_t y,MagickBooleanType *status)
{
  const Image
    *const image = noise_row->image;

  Image
    *const noise_image = noise_row->noise_image;

  CacheView
    *const image_view = noise_row->image_view,
    *const noise_view = noise_row->noise_view;

  RandomInfo
    **const random_info = noise_row->random_info;

  const NoiseType
    noise_type = noise_row->noise_type;

  const double
    attenuate = noise_row->attenuate;

  ExceptionInfo
    *const exception = noise_row->exception;

  const AddNoisePixel
    noise_pixel = { image, noise_image, noise_type, attenuate };

  const int
    id = GetOpenMPThreadId();

  MagickBooleanType
    sync;

  const Quantum
    *magick_restrict p;

  ssize_t
    x;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
  q=QueueCacheViewAuthenticPixels(noise_view,0,y,noise_image->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    AddNoisePixelChannels(&noise_pixel,p,q,random_info[id]);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(noise_image);
  }
  sync=SyncCacheViewAuthenticPixels(noise_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

MagickExport Image *AddNoiseImage(const Image *image,const NoiseType noise_type,
  const double attenuate,ExceptionInfo *exception)
{
#define AddNoiseImageTag  "AddNoise/Image"

  CacheView
    *image_view,
    *noise_view;

  Image
    *noise_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  RandomInfo
    **magick_restrict random_info;

  ssize_t
    y;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
  unsigned long
    key;
#endif

  /*
    Initialize noise image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (CloneEffectImage(image,&noise_image,exception) == MagickFalse)
    return((Image *) NULL);
  /*
    Add noise in each row.
  */
  status=MagickTrue;
  progress=0;
  random_info=AcquireRandomInfoTLS();
  image_view=AcquireVirtualCacheView(image,exception);
  noise_view=AcquireAuthenticCacheView(noise_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  key=GetRandomSecretKey(random_info[0]);
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,noise_image,image->rows,key == ~0UL ? 0 : 2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const AddNoiseRow
      noise_row = { image, noise_image, image_view, noise_view, random_info,
        noise_type, attenuate, exception };

    if (AddNoiseImageRow(&noise_row,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(image,AddNoiseImageTag,progress,&status);
      }
  }
  noise_view=DestroyCacheView(noise_view);
  image_view=DestroyCacheView(image_view);
  random_info=DestroyRandomInfoTLS(random_info);
  return(ReleaseFailedImage(noise_image,status));
}
