/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                CCCC  H   H   AAA   N   N  N   N  EEEEE  L                   %
%               C      H   H  A   A  NN  N  NN  N  E      L                   %
%               C      HHHHH  AAAAA  N N N  N N N  EEE    L                   %
%               C      H   H  A   A  N  NN  N  NN  E      L                   %
%                CCCC  H   H  A   A  N   N  N   N  EEEEE  LLLLL               %
%                                                                             %
%                       AAA   L      PPPP   H   H   AAA                       %
%                      A   A  L      P   P  H   H  A   A                      %
%                      AAAAA  L      PPPP   HHHHH  AAAAA                      %
%                      A   A  L      P      H   H  A   A                      %
%                      A   A  LLLLL  P      H   H  A   A                      %
%                                                                             %
%                                                                             %
%                    MagickCore Image Alpha Channel Method                    %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                               December 2003                                 %
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
#include "MagickCore/cache-private.h"
#include "MagickCore/channel.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/image.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/version.h"

/*
  Typedef declarations.
*/
typedef struct _FlattenSource
{
  const Image
    *image;

  const PixelInfo
    *p;

  double
    alpha;
} FlattenSource;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e A l p h a C h a n n e l                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageAlphaChannel() activates, deactivates, resets, or sets the alpha
%  channel.
%
%  The format of the SetImageAlphaChannel method is:
%
%      MagickBooleanType SetImageAlphaChannel(Image *image,
%        const AlphaChannelOption alpha_type,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o alpha_type:  The alpha channel type: ActivateAlphaChannel,
%      AssociateAlphaChannel, CopyAlphaChannel, DeactivateAlphaChannel,
%      DisassociateAlphaChannel,  ExtractAlphaChannel, OffAlphaChannel,
%      OnAlphaChannel, OpaqueAlphaChannel, SetAlphaChannel, ShapeAlphaChannel,
%      and TransparentAlphaChannel.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline void FlattenPixelInfo(const FlattenSource *source,
  const Quantum *q,const double beta,Quantum *composite)
{
  const Image
    *image = source->image;

  const PixelInfo
    *p = source->p;

  const double
    alpha = source->alpha;

  double
    Da,
    gamma,
    Sa;

  ssize_t
    i;

  /*
    Compose pixel p over pixel q with the given alpha.
  */
  Sa=QuantumScale*alpha;
  Da=QuantumScale*beta,
  gamma=Sa*(-Da)+Sa+Da;
  gamma=MagickSafeReciprocal(gamma);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if (traits == UndefinedPixelTrait)
      continue;
    switch (channel)
    {
      case RedPixelChannel:
      {
        composite[i]=ClampToQuantum(gamma*MagickOver_((double) q[i],beta,
          (double) p->red,alpha));
        break;
      }
      case GreenPixelChannel:
      {
        composite[i]=ClampToQuantum(gamma*MagickOver_((double) q[i],beta,
          (double) p->green,alpha));
        break;
      }
      case BluePixelChannel:
      {
        composite[i]=ClampToQuantum(gamma*MagickOver_((double) q[i],beta,
          (double) p->blue,alpha));
        break;
      }
      case BlackPixelChannel:
      {
        composite[i]=ClampToQuantum(gamma*MagickOver_((double) q[i],beta,
          (double) p->black,alpha));
        break;
      }
      case AlphaPixelChannel:
      {
        composite[i]=ClampToQuantum((double) QuantumRange*(Sa*(-Da)+Sa+Da));
        break;
      }
      default:
        break;
    }
  }
}

static void ScaleColorChannels(const Image *image,const double gamma,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if (channel == AlphaPixelChannel)
      continue;
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    q[i]=ClampToQuantum(gamma*(double) q[i]);
  }
}

static void SetTransparentPixelsToBackground(const Image *image,
  Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (GetPixelAlpha(image,q) == TransparentAlpha)
      {
        SetPixelViaPixelInfo(image,&image->background_color,q);
        SetPixelChannel(image,AlphaPixelChannel,TransparentAlpha,q);
      }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void FlattenRowOverBackground(const Image *image,
  Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    FlattenSource
      source;

    source.image=image;
    source.p=(&image->background_color);
    source.alpha=image->background_color.alpha;
    FlattenPixelInfo(&source,q,(double) GetPixelAlpha(image,q),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void SetShapeAlphaPixels(const Image *image,PixelInfo pixel,
  Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    pixel.alpha=GetPixelIntensity(image,q);
    SetPixelViaPixelInfo(image,&pixel,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void AssociateAlphaRow(const Image *image,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    double
      gamma;

    gamma=QuantumScale*(double) GetPixelAlpha(image,q);
    ScaleColorChannels(image,gamma,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType IsAlphaRowOpaque(const Image *image,
  const Quantum *magick_restrict p)
{
  MagickBooleanType
    status;

  ssize_t
    x;

  status=MagickTrue;
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (GetPixelAlpha(image,p) != OpaqueAlpha)
      {
        status=MagickFalse;
        break;
      }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(status);
}

static MagickBooleanType DisassociateAlphaRow(Image *image,
  CacheView *image_view,const ssize_t y,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  status=MagickTrue;
  q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
    exception);
  if (q == (Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    double
      gamma,
      Sa;

    Sa=QuantumScale*(double) GetPixelAlpha(image,q);
    gamma=MagickSafeReciprocal(Sa);
    ScaleColorChannels(image,gamma,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    status=MagickFalse;
  return(status);
}

MagickExport MagickBooleanType SetImageAlphaChannel(Image *image,
  const AlphaChannelOption alpha_type,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickCoreSignature);
  status=MagickTrue;
  switch (alpha_type)
  {
    case ActivateAlphaChannel:
    {
      if ((image->alpha_trait & BlendPixelTrait) != 0)
        return(status);
      image->alpha_trait=BlendPixelTrait;
      break;
    }
    case AssociateAlphaChannel:
    {
      /*
        Associate alpha.
      */
      status=SetImageStorageClass(image,DirectClass,exception);
      if (status == MagickFalse)
        break;
      image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(image,image,image->rows,2)
#endif
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        Quantum
          *magick_restrict q;

        if (status == MagickFalse)
          continue;
        q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
          exception);
        if (q == (Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        AssociateAlphaRow(image,q);
        if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
          status=MagickFalse;
      }
      image_view=DestroyCacheView(image_view);
      image->alpha_trait=CopyPixelTrait;
      return(status);
    }
    case BackgroundAlphaChannel:
    {
      /*
        Set transparent pixels to background color.
      */
      if ((image->alpha_trait & BlendPixelTrait) == 0)
        break;
      status=SetImageStorageClass(image,DirectClass,exception);
      if (status == MagickFalse)
        break;
      image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(image,image,image->rows,2)
#endif
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        Quantum
          *magick_restrict q;

        if (status == MagickFalse)
          continue;
        q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
          exception);
        if (q == (Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        SetTransparentPixelsToBackground(image,q);
        if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
          status=MagickFalse;
      }
      image_view=DestroyCacheView(image_view);
      return(status);
    }
    case CopyAlphaChannel:
    {
      image->alpha_trait=UpdatePixelTrait;
      status=CompositeImage(image,image,IntensityCompositeOp,MagickTrue,0,0,
        exception);
      break;
    }
    case DeactivateAlphaChannel:
    {
      if ((image->alpha_trait & BlendPixelTrait) == 0)
        status=SetImageAlpha(image,OpaqueAlpha,exception);
      image->alpha_trait=CopyPixelTrait;
      break;
    }
    case DisassociateAlphaChannel:
    {
      /*
        Disassociate alpha.
      */
      status=SetImageStorageClass(image,DirectClass,exception);
      if (status == MagickFalse)
        break;
      image->alpha_trait=BlendPixelTrait;
      image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(image,image,image->rows,2)
#endif
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        if (status == MagickFalse)
          continue;
        if (DisassociateAlphaRow(image,image_view,y,exception) == MagickFalse)
          status=MagickFalse;
      }
      image_view=DestroyCacheView(image_view);
      image->alpha_trait=UndefinedPixelTrait;
      return(status);
    }
    case DiscreteAlphaChannel:
    {
      if ((image->alpha_trait & BlendPixelTrait) == 0)
        status=SetImageAlpha(image,OpaqueAlpha,exception);
      image->alpha_trait=UpdatePixelTrait;
      break;
    }
    case ExtractAlphaChannel:
    {
      status=CompositeImage(image,image,AlphaCompositeOp,MagickTrue,0,0,
        exception);
      image->alpha_trait=UndefinedPixelTrait;
      break;
    }
    case OffAlphaChannel:
    {
      if ((image->alpha_trait & BlendPixelTrait) == 0)
        return(status);
      image->alpha_trait=UndefinedPixelTrait;
      break;
    }
    case OffIfOpaqueAlphaChannel:
    {
      MagickBooleanType
        opaque = MagickTrue;

      /*
        Remove opaque alpha channel.
      */
      if ((image->alpha_trait & BlendPixelTrait) == 0)
        break;
      image_view=AcquireVirtualCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(opaque,status) \
        magick_number_threads(image,image,image->rows,2)
#endif
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        const Quantum
          *magick_restrict p;

        if ((status == MagickFalse) || (opaque == MagickFalse))
          continue;
        p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
        if (p == (const Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        if (IsAlphaRowOpaque(image,p) == MagickFalse)
          opaque=MagickFalse;
      }
      image_view=DestroyCacheView(image_view);
      if (opaque != MagickFalse)
        image->alpha_trait=UndefinedPixelTrait;
      break;
    }
    case OnAlphaChannel:
    {
      if ((image->alpha_trait & BlendPixelTrait) == 0)
        status=SetImageAlpha(image,OpaqueAlpha,exception);
      image->alpha_trait=BlendPixelTrait;
      break;
    }
    case OpaqueAlphaChannel:
    {
      status=SetImageAlpha(image,OpaqueAlpha,exception);
      break;
    }
    case RemoveAlphaChannel:
    {
      /*
        Remove transparency.
      */
      if ((image->alpha_trait & BlendPixelTrait) == 0)
        break;
      status=SetImageStorageClass(image,DirectClass,exception);
      if (status == MagickFalse)
        break;
      image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(image,image,image->rows,2)
#endif
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        Quantum
          *magick_restrict q;

        if (status == MagickFalse)
          continue;
        q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
          exception);
        if (q == (Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        FlattenRowOverBackground(image,q);
        if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
          status=MagickFalse;
      }
      image_view=DestroyCacheView(image_view);
      image->alpha_trait=image->background_color.alpha_trait;
      break;
    }
    case SetAlphaChannel:
    {
      if ((image->alpha_trait & BlendPixelTrait) == 0)
        status=SetImageAlpha(image,OpaqueAlpha,exception);
      break;
    }
    case ShapeAlphaChannel:
    {
      PixelInfo
        background;

      /*
        Remove transparency.
      */
      ConformPixelInfo(image,&image->background_color,&background,exception);
      background.alpha_trait=BlendPixelTrait;
      image->alpha_trait=BlendPixelTrait;
      status=SetImageStorageClass(image,DirectClass,exception);
      if (status == MagickFalse)
        break;
      image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(image,image,image->rows,2)
#endif
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        PixelInfo
          pixel;

        Quantum
          *magick_restrict q;

        if (status == MagickFalse)
          continue;
        q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
          exception);
        if (q == (Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        pixel=background;
        SetShapeAlphaPixels(image,pixel,q);
        if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
          status=MagickFalse;
      }
      image_view=DestroyCacheView(image_view);
      break;
    }
    case TransparentAlphaChannel:
    {
      status=SetImageAlpha(image,TransparentAlpha,exception);
      break;
    }
    case UndefinedAlphaChannel:
      break;
  }
  if (status == MagickFalse)
    return(status);
  (void) SetPixelChannelMask(image,image->channel_mask);
  return(SyncImagePixelCache(image,exception));
}
