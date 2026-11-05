/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                   V   V  IIIII  SSSSS  U   U   AAA   L                      %
%                   V   V    I    SS     U   U  A   A  L                      %
%                   V   V    I     SSS   U   U  AAAAA  L                      %
%                    V V     I       SS  U   U  A   A  L                      %
%                     V    IIIII  SSSSS   UUU   A   A  LLLLL                  %
%                                                                             %
%                EEEEE  FFFFF  FFFFF  EEEEE  CCCC  TTTTT  SSSSS               %
%                E      F      F      E     C        T    SS                  %
%                EEE    FFF    FFF    EEE   C        T     SSS                %
%                E      F      F      E     C        T       SS               %
%                EEEEE  F      F      EEEEE  CCCC    T    SSSSS               %
%                                                                             %
%                                                                             %
%                   MagickCore Image Special Effects Methods                  %
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
static inline MagickBooleanType IsPixelChannelPairUndefined(
  const PixelTrait traits,const PixelTrait other_traits)
{
  return(((traits == UndefinedPixelTrait) ||
    (other_traits == UndefinedPixelTrait)) ? MagickTrue : MagickFalse);
}

static void AddNoisePixelChannels(const Image *image,
  const Image *noise_image,const Quantum *magick_restrict p,
  Quantum *magick_restrict q,RandomInfo *magick_restrict random_info,
  const NoiseType noise_type,const double attenuate)
{
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
    AddNoisePixelChannels(image,noise_image,p,q,random_info[id],noise_type,
      attenuate);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(noise_image);
  }
  sync=SyncCacheViewAuthenticPixels(noise_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

typedef struct _EffectRows
{
  const Image
    *image,
    *effect_image;

  CacheView
    *image_view,
    *effect_view;

  ExceptionInfo
    *exception;

  ssize_t
    y;
} EffectRows;

static MagickBooleanType GetEffectRows(const EffectRows *rows,
  const Quantum *magick_restrict *p,Quantum *magick_restrict *q,
  MagickBooleanType *status)
{
  if ((*status) == MagickFalse)
    return(MagickFalse);
  (*p)=GetCacheViewVirtualPixels(rows->image_view,0,rows->y,
    rows->image->columns,1,rows->exception);
  (*q)=QueueCacheViewAuthenticPixels(rows->effect_view,0,rows->y,
    rows->effect_image->columns,1,rows->exception);
  if (((*p) == (const Quantum *) NULL) || ((*q) == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void SyncEffectRow(CacheView *view,ExceptionInfo *exception,
  MagickBooleanType *status)
{
  if (SyncCacheViewAuthenticPixels(view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

static void SetEffectProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

static MagickBooleanType CloneEffectImage(const Image *image,
  Image **effect_image,ExceptionInfo *exception)
{
  *effect_image=CloneImage(image,0,0,MagickTrue,exception);
  if (*effect_image == (Image *) NULL)
    return(MagickFalse);
  if (SetImageStorageClass(*effect_image,DirectClass,exception) == MagickFalse)
    {
      *effect_image=DestroyImage(*effect_image);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static Image *ReleaseFailedImage(Image *image,const MagickBooleanType status)
{
  if (status == MagickFalse)
    image=DestroyImage(image);
  return(image);
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     B l u e S h i f t I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BlueShiftImage() mutes the colors of the image to simulate a scene at
%  nighttime in the moonlight.
%
%  The format of the BlueShiftImage method is:
%
%      Image *BlueShiftImage(const Image *image,const double factor,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o factor: the shift factor.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Quantum GetPixelMinimumRGB(const Image *image,
  const Quantum *magick_restrict p)
{
  Quantum
    quantum;

  quantum=GetPixelRed(image,p);
  if (GetPixelGreen(image,p) < quantum)
    quantum=GetPixelGreen(image,p);
  if (GetPixelBlue(image,p) < quantum)
    quantum=GetPixelBlue(image,p);
  return(quantum);
}

static Quantum GetPixelMaximumRGB(const Image *image,
  const Quantum *magick_restrict p)
{
  Quantum
    quantum;

  quantum=GetPixelRed(image,p);
  if (GetPixelGreen(image,p) > quantum)
    quantum=GetPixelGreen(image,p);
  if (GetPixelBlue(image,p) > quantum)
    quantum=GetPixelBlue(image,p);
  return(quantum);
}

typedef struct _BlueShiftRow
{
  const Image
    *image;

  Image
    *shift_image;

  CacheView
    *image_view,
    *shift_view;

  double
    factor;

  ExceptionInfo
    *exception;
} BlueShiftRow;

static MagickBooleanType BlueShiftImageRow(const BlueShiftRow *shift_row,
  const ssize_t y,MagickBooleanType *status)
{
  const Image
    *const image = shift_row->image;

  Image
    *const shift_image = shift_row->shift_image;

  CacheView
    *const image_view = shift_row->image_view,
    *const shift_view = shift_row->shift_view;

  const double
    factor = shift_row->factor;

  ExceptionInfo
    *const exception = shift_row->exception;

  MagickBooleanType
    sync;

  PixelInfo
    pixel;

  Quantum
    quantum;

  const Quantum
    *magick_restrict p;

  ssize_t
    x;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
  q=QueueCacheViewAuthenticPixels(shift_view,0,y,shift_image->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    quantum=GetPixelMinimumRGB(image,p);
    pixel.red=0.5*((double) GetPixelRed(image,p)+factor*(double) quantum);
    pixel.green=0.5*((double) GetPixelGreen(image,p)+factor*(double) quantum);
    pixel.blue=0.5*((double) GetPixelBlue(image,p)+factor*(double) quantum);
    quantum=GetPixelMaximumRGB(image,p);
    pixel.red=0.5*(pixel.red+factor*(double) quantum);
    pixel.green=0.5*(pixel.green+factor*(double) quantum);
    pixel.blue=0.5*(pixel.blue+factor*(double) quantum);
    SetPixelRed(shift_image,ClampToQuantum(pixel.red),q);
    SetPixelGreen(shift_image,ClampToQuantum(pixel.green),q);
    SetPixelBlue(shift_image,ClampToQuantum(pixel.blue),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(shift_image);
  }
  sync=SyncCacheViewAuthenticPixels(shift_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

MagickExport Image *BlueShiftImage(const Image *image,const double factor,
  ExceptionInfo *exception)
{
#define BlueShiftImageTag  "BlueShift/Image"

  CacheView
    *image_view,
    *shift_view;

  Image
    *shift_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Allocate blue shift image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (CloneEffectImage(image,&shift_image,exception) == MagickFalse)
    return((Image *) NULL);
  /*
    Blue-shift DirectClass image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  shift_view=AcquireAuthenticCacheView(shift_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,shift_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    BlueShiftRow
      shift_row;

    shift_row.image=image;
    shift_row.shift_image=shift_image;
    shift_row.image_view=image_view;
    shift_row.shift_view=shift_view;
    shift_row.factor=factor;
    shift_row.exception=exception;
    if (BlueShiftImageRow(&shift_row,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(image,BlueShiftImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  shift_view=DestroyCacheView(shift_view);
  if (status == MagickFalse)
    shift_image=DestroyImage(shift_image);
  return(shift_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C h a r c o a l I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CharcoalImage() creates a new image that is a copy of an existing one with
%  the edge highlighted.  It allocates the memory necessary for the new Image
%  structure and returns a pointer to the new image.
%
%  The format of the CharcoalImage method is:
%
%      Image *CharcoalImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the pixel neighborhood.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *CharcoalImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  Image
    *charcoal_image,
    *edge_image;

  MagickBooleanType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  edge_image=EdgeImage(image,radius,exception);
  if (edge_image == (Image *) NULL)
    return((Image *) NULL);
  edge_image->alpha_trait=UndefinedPixelTrait;
  charcoal_image=(Image *) NULL;
  status=ClampImage(edge_image,exception);
  if (status != MagickFalse)
    charcoal_image=BlurImage(edge_image,radius,sigma,exception);
  edge_image=DestroyImage(edge_image);
  if (charcoal_image == (Image *) NULL)
    return((Image *) NULL);
  status=NormalizeImage(charcoal_image,exception);
  if (status != MagickFalse)
    status=NegateImage(charcoal_image,MagickFalse,exception);
  if (status != MagickFalse)
    status=GrayscaleImage(charcoal_image,image->intensity,exception);
  if (status == MagickFalse)
    charcoal_image=DestroyImage(charcoal_image);
  return(charcoal_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o l o r i z e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ColorizeImage() blends the fill color with each pixel in the image.
%  A percentage blend is specified with opacity.  Control the application
%  of different color components by specifying a different percentage for
%  each component (e.g. 90/100/10 is 90% red, 100% green, and 10% blue).
%
%  The format of the ColorizeImage method is:
%
%      Image *ColorizeImage(const Image *image,const char *blend,
%        const PixelInfo *colorize,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o blend:  A character string indicating the level of blending as a
%      percentage.
%
%    o colorize: A color value.
%
%    o exception: return any errors or warnings in this structure.
%
*/
#define Colorize(pixel,blend_percentage,colorize)  \
  ((((double) pixel)*(100.0-(blend_percentage))+(colorize)*(blend_percentage))/100.0)

static void ColorizePixelChannels(const Image *colorize_image,
  const PixelInfo *blend_percentage,const PixelInfo *colorize,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(colorize_image); i++)
  {
    PixelTrait traits = GetPixelChannelTraits(colorize_image,
      (PixelChannel) i);
    if (traits == UndefinedPixelTrait)
      continue;
    if ((traits & CopyPixelTrait) != 0)
      continue;
    SetPixelChannel(colorize_image,(PixelChannel) i,ClampToQuantum(
      Colorize(q[i],GetPixelInfoChannel(blend_percentage,(PixelChannel) i),
      GetPixelInfoChannel(colorize,(PixelChannel) i))),q);
  }
}

static PixelInfo ParseColorizeBlendPercentage(const Image *colorize_image,
  const char *blend)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  PixelInfo
    blend_percentage;

  GetPixelInfo(colorize_image,&blend_percentage);
  flags=ParseGeometry(blend,&geometry_info);
  blend_percentage.red=geometry_info.rho;
  blend_percentage.green=geometry_info.rho;
  blend_percentage.blue=geometry_info.rho;
  blend_percentage.black=geometry_info.rho;
  blend_percentage.alpha=(MagickRealType) TransparentAlpha;
  if ((flags & SigmaValue) != 0)
    blend_percentage.green=geometry_info.sigma;
  if ((flags & XiValue) != 0)
    blend_percentage.blue=geometry_info.xi;
  if ((flags & PsiValue) != 0)
    blend_percentage.alpha=geometry_info.psi;
  if (blend_percentage.colorspace == CMYKColorspace)
    {
      if ((flags & PsiValue) != 0)
        blend_percentage.black=geometry_info.psi;
      if ((flags & ChiValue) != 0)
        blend_percentage.alpha=geometry_info.chi;
    }
  return(blend_percentage);
}

typedef struct _ColorizeRow
{
  CacheView
    *image_view;

  Image
    *colorize_image;

  PixelInfo
    blend_percentage;

  const PixelInfo
    *colorize;

  ExceptionInfo
    *exception;
} ColorizeRow;

static MagickBooleanType ColorizeImageRow(const ColorizeRow *colorize_row,
  const ssize_t y,MagickBooleanType *status)
{
  CacheView
    *const image_view = colorize_row->image_view;

  Image
    *const colorize_image = colorize_row->colorize_image;

  const PixelInfo
    blend_percentage = colorize_row->blend_percentage;

  const PixelInfo
    *const colorize = colorize_row->colorize;

  ExceptionInfo
    *const exception = colorize_row->exception;

  MagickBooleanType
    sync;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(image_view,0,y,colorize_image->columns,1,
    exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  for (x=0; x < (ssize_t) colorize_image->columns; x++)
  {
    ColorizePixelChannels(colorize_image,&blend_percentage,colorize,q);
    q+=(ptrdiff_t) GetPixelChannels(colorize_image);
  }
  sync=SyncCacheViewAuthenticPixels(image_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static void ConformColorizeImage(Image *colorize_image,
  const PixelInfo *colorize,ExceptionInfo *exception)
{
  if ((IsGrayColorspace(colorize_image->colorspace) != MagickFalse) ||
      (IsPixelInfoGray(colorize) != MagickFalse))
    (void) SetImageColorspace(colorize_image,sRGBColorspace,exception);
  if ((colorize_image->alpha_trait == UndefinedPixelTrait) &&
      (colorize->alpha_trait != UndefinedPixelTrait))
    (void) SetImageAlpha(colorize_image,OpaqueAlpha,exception);
}

MagickExport Image *ColorizeImage(const Image *image,const char *blend,
  const PixelInfo *colorize,ExceptionInfo *exception)
{
#define ColorizeImageTag  "Colorize/Image"

  CacheView
    *image_view;

  Image
    *colorize_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    blend_percentage;

  ssize_t
    y;

  /*
    Allocate colorized image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (CloneEffectImage(image,&colorize_image,exception) == MagickFalse)
    return((Image *) NULL);
  ConformColorizeImage(colorize_image,colorize,exception);
  if (blend == (const char *) NULL)
    return(colorize_image);
  blend_percentage=ParseColorizeBlendPercentage(colorize_image,blend);
  /*
    Colorize DirectClass image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(colorize_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(colorize_image,colorize_image,colorize_image->rows,2)
#endif
  for (y=0; y < (ssize_t) colorize_image->rows; y++)
  {
    ColorizeRow
      colorize_row;

    colorize_row.image_view=image_view;
    colorize_row.colorize_image=colorize_image;
    colorize_row.blend_percentage=blend_percentage;
    colorize_row.colorize=colorize;
    colorize_row.exception=exception;
    if (ColorizeImageRow(&colorize_row,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(image,ColorizeImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(ReleaseFailedImage(colorize_image,status));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o l o r M a t r i x I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ColorMatrixImage() applies color transformation to an image. This method
%  permits saturation changes, hue rotation, luminance to alpha, and various
%  other effects.  Although variable-sized transformation matrices can be used,
%  typically one uses a 5x5 matrix for an RGBA image and a 6x6 for CMYKA
%  (or RGBA with offsets).  The matrix is similar to those used by Adobe Flash
%  except offsets are in column 6 rather than 5 (in support of CMYKA images)
%  and offsets are normalized (divide Flash offset by 255).
%
%  The format of the ColorMatrixImage method is:
%
%      Image *ColorMatrixImage(const Image *image,
%        const KernelInfo *color_matrix,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o color_matrix:  the color matrix.  It is the callers responsibility to
%      ensure the matrix has at least width * height values.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void LoadColorMatrix(const KernelInfo *color_matrix,
  double ColorMatrix[][6])
{
  ssize_t
    i,
    u,
    v;

  i=0;
  for (v=0; v < (ssize_t) color_matrix->height; v++)
    for (u=0; u < (ssize_t) color_matrix->width; u++)
    {
      if ((v < 6) && (u < 6))
        ColorMatrix[v][u]=color_matrix->values[i];
      i++;
    }
}

static double GetColorMatrixSum(const Image *image,
  const Quantum *magick_restrict p,const double *row)
{
  double
    sum;

  sum=row[0]*(double) GetPixelRed(image,p)+row[1]*
    (double) GetPixelGreen(image,p)+row[2]*(double)
    GetPixelBlue(image,p);
  if (image->colorspace == CMYKColorspace)
    sum+=row[3]*(double) GetPixelBlack(image,p);
  if (image->alpha_trait != UndefinedPixelTrait)
    sum+=row[4]*(double) GetPixelAlpha(image,p);
  sum+=(double) QuantumRange*row[5];
  return(sum);
}

static PixelInfo SetColorMatrixChannel(PixelInfo pixel,const ssize_t h,
  const double sum)
{
  switch (h)
  {
    case 0: pixel.red=sum; break;
    case 1: pixel.green=sum; break;
    case 2: pixel.blue=sum; break;
    case 3: pixel.black=sum; break;
    case 4: pixel.alpha=sum; break;
    default: break;
  }
  return(pixel);
}

static void LogColorMatrix(double ColorMatrix[][6])
{
  char
    format[MagickPathExtent],
    *message;

  ssize_t
    u,
    v;

  (void) LogMagickEvent(TransformEvent,GetMagickModule(),
    "  ColorMatrix image with color matrix:");
  message=AcquireString("");
  for (v=0; v < 6; v++)
  {
    *message='\0';
    (void) FormatLocaleString(format,MagickPathExtent,"%.17g: ",(double) v);
    (void) ConcatenateString(&message,format);
    for (u=0; u < 6; u++)
    {
      (void) FormatLocaleString(format,MagickPathExtent,"%+f ",
        ColorMatrix[v][u]);
      (void) ConcatenateString(&message,format);
    }
    (void) LogMagickEvent(TransformEvent,GetMagickModule(),"%s",message);
  }
  message=DestroyString(message);
}

typedef struct _ColorMatrixRow
{
  const Image
    *image;

  Image
    *color_image;

  const KernelInfo
    *color_matrix;

  CacheView
    *image_view,
    *color_view;

  ExceptionInfo
    *exception;
} ColorMatrixRow;

static void ApplyColorMatrixRow(const ColorMatrixRow *color_row,
  double ColorMatrix[][6],const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  const Image
    *const image = color_row->image;

  Image
    *const color_image = color_row->color_image;

  const KernelInfo
    *const color_matrix = color_row->color_matrix;

  PixelInfo
    pixel;

  ssize_t
    x;

  GetPixelInfo(image,&pixel);
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      h;

    size_t
      height;

    GetPixelInfoPixel(image,p,&pixel);
    height=color_matrix->height > 6 ? 6UL : color_matrix->height;
    for (h=0; h < (ssize_t) height; h++)
    {
      double
        sum;

      sum=GetColorMatrixSum(image,p,ColorMatrix[h]);
      pixel=SetColorMatrixChannel(pixel,h,sum);
    }
    SetPixelViaPixelInfo(color_image,&pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(color_image);
  }
}

static MagickBooleanType ColorMatrixImageRow(
  const ColorMatrixRow *color_row,double ColorMatrix[][6],const ssize_t y,
  MagickBooleanType *status)
{
  const Image
    *const image = color_row->image;

  Image
    *const color_image = color_row->color_image;

  CacheView
    *const image_view = color_row->image_view,
    *const color_view = color_row->color_view;

  ExceptionInfo
    *const exception = color_row->exception;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
  q=GetCacheViewAuthenticPixels(color_view,0,y,color_image->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  ApplyColorMatrixRow(color_row,ColorMatrix,p,q);
  if (SyncCacheViewAuthenticPixels(color_view,exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

MagickExport Image *ColorMatrixImage(const Image *image,
  const KernelInfo *color_matrix,ExceptionInfo *exception)
{
#define ColorMatrixImageTag  "ColorMatrix/Image"

  CacheView
    *color_view,
    *image_view;

  double
    ColorMatrix[6][6] =
    {
      { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
      { 0.0, 1.0, 0.0, 0.0, 0.0, 0.0 },
      { 0.0, 0.0, 1.0, 0.0, 0.0, 0.0 },
      { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0 },
      { 0.0, 0.0, 0.0, 0.0, 1.0, 0.0 },
      { 0.0, 0.0, 0.0, 0.0, 0.0, 1.0 }
    };

  Image
    *color_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Map given color_matrix, into a 6x6 matrix   RGBKA and a constant
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  LoadColorMatrix(color_matrix,ColorMatrix);
  /*
    Initialize color image.
  */
  if (CloneEffectImage(image,&color_image,exception) == MagickFalse)
    return((Image *) NULL);
  if (image->debug != MagickFalse)
    LogColorMatrix(ColorMatrix);
  /*
    Apply the ColorMatrix to image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  color_view=AcquireAuthenticCacheView(color_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,color_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const ColorMatrixRow
      color_row = { image, color_image, color_matrix, image_view, color_view,
        exception };

    if (ColorMatrixImageRow(&color_row,ColorMatrix,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(image,ColorMatrixImageTag,progress,&status);
      }
  }
  color_view=DestroyCacheView(color_view);
  image_view=DestroyCacheView(image_view);
  return(ReleaseFailedImage(color_image,status));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     I m p l o d e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImplodeImage() creates a new image that is a copy of an existing
%  one with the image pixels "implode" by the specified percentage.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ImplodeImage method is:
%
%      Image *ImplodeImage(const Image *image,const double amount,
%        const PixelInterpolateMethod method,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o implode_image: Method ImplodeImage returns a pointer to the image
%      after it is implode.  A null image is returned if there is a memory
%      shortage.
%
%    o image: the image.
%
%    o amount:  Define the extent of the implosion.
%
%    o method: the pixel interpolation method.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _WarpInfo
{
  const Image
    *image,
    *canvas_image,
    *warp_image;

  CacheView
    *interpolate_view;

  ExceptionInfo
    *exception;

  PixelInterpolateMethod
    method;

  PointInfo
    center,
    scale;

  double
    amount,
    radius;

  ssize_t
    y;
} WarpInfo;

typedef struct _WarpSite
{
  PointInfo
    delta;

  double
    distance;
} WarpSite;

static void CopyPixelChannels(const Image *source,
  const Image *destination,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(source); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(source,i);
    PixelTrait traits = GetPixelChannelTraits(source,channel);
    PixelTrait destination_traits = GetPixelChannelTraits(destination,
      channel);
    if (IsPixelChannelPairUndefined(traits,destination_traits) != MagickFalse)
      continue;
    SetPixelChannel(destination,channel,p[i],q);
  }
}

static MagickBooleanType ImplodePixel(const WarpInfo *warp,
  const WarpSite *site,Quantum *magick_restrict q,MagickBooleanType *status)
{
  double
    factor;

  PointInfo
    offset;

  /*
    Implode the pixel.
  */
  factor=1.0;
  if (site->distance > 0.0)
    factor=pow(sin(MagickPI*sqrt(site->distance)*
      MagickSafeReciprocal(warp->radius)/2.0),-warp->amount);
  offset.x=factor*site->delta.x*MagickSafeReciprocal(warp->scale.x)+
    warp->center.x;
  offset.y=factor*site->delta.y*MagickSafeReciprocal(warp->scale.y)+
    warp->center.y;
  if ((IsValidPixelOffset((ssize_t) offset.x,warp->image->columns) !=
       MagickFalse) &&
      (IsValidPixelOffset((ssize_t) offset.y,warp->image->rows) !=
       MagickFalse))
    (*status)=InterpolatePixelChannels(warp->canvas_image,
      warp->interpolate_view,warp->warp_image,warp->method,offset.x,offset.y,q,
      warp->exception);
  if ((*status) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static void ImplodeRow(const WarpInfo *warp,const Quantum *magick_restrict p,
  Quantum *magick_restrict q,MagickBooleanType *status)
{
  ssize_t
    x;

  WarpSite
    site;

  site.delta.y=warp->scale.y*((double) warp->y-warp->center.y);
  for (x=0; x < (ssize_t) warp->canvas_image->columns; x++)
  {
    /*
      Determine if the pixel is within an ellipse.
    */
    site.delta.x=warp->scale.x*((double) x-warp->center.x);
    site.distance=site.delta.x*site.delta.x+site.delta.y*site.delta.y;
    if (site.distance >= (warp->radius*warp->radius))
      CopyPixelChannels(warp->canvas_image,warp->warp_image,p,q);
    else
      if (ImplodePixel(warp,&site,q,status) == MagickFalse)
        break;
    p+=(ptrdiff_t) GetPixelChannels(warp->canvas_image);
    q+=(ptrdiff_t) GetPixelChannels(warp->warp_image);
  }
}

static MagickBooleanType CloneWarpImage(Image *canvas_image,
  Image **warp_image,ExceptionInfo *exception)
{
  *warp_image=CloneImage(canvas_image,0,0,MagickTrue,exception);
  if (*warp_image == (Image *) NULL)
    {
      canvas_image=DestroyImage(canvas_image);
      return(MagickFalse);
    }
  if (SetImageStorageClass(*warp_image,DirectClass,exception) == MagickFalse)
    {
      canvas_image=DestroyImage(canvas_image);
      *warp_image=DestroyImage(*warp_image);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void DestroyWarpResources(CacheView *warp_view,
  CacheView *interpolate_view,CacheView *canvas_view,Image *canvas_image)
{
  warp_view=DestroyCacheView(warp_view);
  interpolate_view=DestroyCacheView(interpolate_view);
  canvas_view=DestroyCacheView(canvas_view);
  canvas_image=DestroyImage(canvas_image);
}

static void SetImplodeCanvasAlpha(Image *canvas_image,
  ExceptionInfo *exception)
{
  if ((canvas_image->alpha_trait == UndefinedPixelTrait) &&
      (canvas_image->background_color.alpha != (double) OpaqueAlpha))
    (void) SetImageAlphaChannel(canvas_image,OpaqueAlphaChannel,exception);
}

static MagickBooleanType CloneImplodeCanvas(const Image *image,
  Image **canvas_image,ExceptionInfo *exception)
{
  *canvas_image=CloneImage(image,0,0,MagickTrue,exception);
  if (*canvas_image == (Image *) NULL)
    return(MagickFalse);
  SetImplodeCanvasAlpha(*canvas_image,exception);
  return(MagickTrue);
}

static void GetImplodeGeometry(const Image *canvas_image,PointInfo *scale,
  PointInfo *center,double *radius)
{
  (*scale).x=1.0;
  (*scale).y=1.0;
  (*center).x=0.5*canvas_image->columns;
  (*center).y=0.5*canvas_image->rows;
  *radius=(*center).x;
  if (canvas_image->columns > canvas_image->rows)
    (*scale).y=(double) canvas_image->columns*MagickSafeReciprocal((double)
      canvas_image->rows);
  else
    if (canvas_image->columns < canvas_image->rows)
      {
        (*scale).x=(double) canvas_image->rows*MagickSafeReciprocal((double)
          canvas_image->columns);
        *radius=(*center).y;
      }
}

MagickExport Image *ImplodeImage(const Image *image,const double amount,
  const PixelInterpolateMethod method,ExceptionInfo *exception)
{
#define ImplodeImageTag  "Implode/Image"

  CacheView
    *canvas_view,
    *implode_view,
    *interpolate_view;

  double
    radius;

  Image
    *canvas_image,
    *implode_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PointInfo
    center,
    scale;

  ssize_t
    y;

  /*
    Initialize implode image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (CloneImplodeCanvas(image,&canvas_image,exception) == MagickFalse)
    return((Image *) NULL);
  if (CloneWarpImage(canvas_image,&implode_image,exception) == MagickFalse)
    return((Image *) NULL);
  /*
    Compute scaling factor.
  */
  GetImplodeGeometry(canvas_image,&scale,&center,&radius);
  /*
    Implode image.
  */
  status=MagickTrue;
  progress=0;
  canvas_view=AcquireVirtualCacheView(canvas_image,exception);
  interpolate_view=AcquireVirtualCacheView(canvas_image,exception);
  implode_view=AcquireAuthenticCacheView(implode_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(canvas_image,implode_image,canvas_image->rows,1)
#endif
  for (y=0; y < (ssize_t) canvas_image->rows; y++)
  {
    const EffectRows
      rows = { canvas_image,implode_image,canvas_view,implode_view,exception,y };

    const WarpInfo
      warp = { image,canvas_image,implode_image,interpolate_view,exception,
        method,center,scale,amount,radius,y };

    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    if (GetEffectRows(&rows,&p,&q,&status) == MagickFalse)
      continue;
    ImplodeRow(&warp,p,q,&status);
    SyncEffectRow(implode_view,exception,&status);
    if (canvas_image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(canvas_image,ImplodeImageTag,progress,&status);
      }
  }
  DestroyWarpResources(implode_view,interpolate_view,canvas_view,canvas_image);
  return(ReleaseFailedImage(implode_image,status));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M o r p h I m a g e s                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  The MorphImages() method requires a minimum of two images.  The first
%  image is transformed into the second by a number of intervening images
%  as specified by frames.
%
%  The format of the MorphImage method is:
%
%      Image *MorphImages(const Image *image,const size_t number_frames,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o number_frames:  Define the number of in-between image to generate.
%      The more in-between frames, the smoother the morph.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void MorphPixelChannels(const Image *morph_image,
  const Image *morph_images,const Quantum *magick_restrict p,
  Quantum *magick_restrict q,const double alpha,const double beta)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(morph_image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(morph_image,i);
    PixelTrait traits = GetPixelChannelTraits(morph_image,channel);
    PixelTrait morph_traits=GetPixelChannelTraits(morph_images,channel);
    if (IsPixelChannelPairUndefined(traits,morph_traits) != MagickFalse)
      continue;
    if ((morph_traits & CopyPixelTrait) != 0)
      {
        SetPixelChannel(morph_image,channel,p[i],q);
        continue;
      }
    SetPixelChannel(morph_image,channel,ClampToQuantum(alpha*(double)
      GetPixelChannel(morph_images,channel,q)+beta*(double) p[i]),q);
  }
}

#define MorphImageTag  "Morph/Image"

static MagickBooleanType ReportMorphProgress(const Image *image,
  const MagickOffsetType scene,MagickBooleanType status)
{
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(image,MorphImageTag,scene,
        GetImageListLength(image));
      if (proceed == MagickFalse)
        status=MagickFalse;
    }
  return(status);
}

static void ReportMorphFrameProgress(const Image *image,const ssize_t n,
  const size_t number_frames,MagickBooleanType *status)
{
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(image,MorphImageTag,(MagickOffsetType) n,
        number_frames);
      if (proceed == MagickFalse)
        (*status)=MagickFalse;
    }
}

typedef struct _MorphFrameRowInfo
{
  CacheView
    *image_view;

  Image
    *morph_image;

  ExceptionInfo
    *exception;

  CacheView
    *morph_view;

  Image
    *morph_images;

  double
    alpha;

  double
    beta;
} MorphFrameRowInfo;

static void MorphFrameRow(MagickBooleanType *status,
  const MorphFrameRowInfo *info,const ssize_t y)
{
  CacheView
    *const image_view = info->image_view;

  Image
    *const morph_image = info->morph_image;

  ExceptionInfo
    *const exception = info->exception;

  CacheView
    *const morph_view = info->morph_view;

  Image
    *const morph_images = info->morph_images;

  const double
    alpha = info->alpha;

  const double
    beta = info->beta;

  MagickBooleanType
    sync;

  const Quantum
    *magick_restrict p;

  ssize_t
    x;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(image_view,0,y,morph_image->columns,1,
    exception);
  q=GetCacheViewAuthenticPixels(morph_view,0,y,morph_images->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) morph_images->columns; x++)
  {
    MorphPixelChannels(morph_image,morph_images,p,q,alpha,beta);
    p+=(ptrdiff_t) GetPixelChannels(morph_image);
    q+=(ptrdiff_t) GetPixelChannels(morph_images);
  }
  sync=SyncCacheViewAuthenticPixels(morph_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
}

typedef struct _MorphSingleInfo
{
  const Image
    *image;

  size_t
    number_frames;

  ExceptionInfo
    *exception;
} MorphSingleInfo;

static Image *MorphSingleImage(const MorphSingleInfo *single,
  Image *morph_images,MagickBooleanType *status)
{
  const Image
    *const image = single->image;

  const size_t
    number_frames = single->number_frames;

  ExceptionInfo
    *const exception = single->exception;

  Image
    *morph_image;

  ssize_t
    n;

  /*
    Morph single image.
  */
  for (n=1; n < (ssize_t) number_frames; n++)
  {
    morph_image=CloneImage(image,0,0,MagickTrue,exception);
    if (morph_image == (Image *) NULL)
      {
        morph_images=DestroyImageList(morph_images);
        return((Image *) NULL);
      }
    AppendImageToList(&morph_images,morph_image);
    ReportMorphFrameProgress(image,n,number_frames,status);
  }
  return(GetFirstImageInList(morph_images));
}

MagickExport Image *MorphImages(const Image *image,const size_t number_frames,
  ExceptionInfo *exception)
{
  double
    alpha,
    beta;

  Image
    *morph_image,
    *morph_images;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  const Image
    *next;

  ssize_t
    n;

  ssize_t
    y;

  /*
    Clone first frame in sequence.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  morph_images=CloneImage(image,0,0,MagickTrue,exception);
  if (morph_images == (Image *) NULL)
    return((Image *) NULL);
  if (GetNextImageInList(image) == (Image *) NULL)
    {
      const MorphSingleInfo
        single = { image, number_frames, exception };

      return(MorphSingleImage(&single,morph_images,&status));
    }
  /*
    Morph image sequence.
  */
  status=MagickTrue;
  scene=0;
  next=image;
  for ( ; GetNextImageInList(next) != (Image *) NULL; next=GetNextImageInList(next))
  {
    for (n=0; n < (ssize_t) number_frames; n++)
    {
      CacheView
        *image_view,
        *morph_view;

      beta=(double) (n+1.0)/(double) (number_frames+1.0);
      alpha=1.0-beta;
      morph_image=ResizeImage(next,(size_t) (alpha*next->columns+beta*
        GetNextImageInList(next)->columns+0.5),(size_t) (alpha*next->rows+beta*
        GetNextImageInList(next)->rows+0.5),next->filter,exception);
      if (morph_image == (Image *) NULL)
        {
          morph_images=DestroyImageList(morph_images);
          return((Image *) NULL);
        }
      status=SetImageStorageClass(morph_image,DirectClass,exception);
      if (status == MagickFalse)
        {
          morph_image=DestroyImage(morph_image);
          return((Image *) NULL);
        }
      AppendImageToList(&morph_images,morph_image);
      morph_images=GetLastImageInList(morph_images);
      morph_image=ResizeImage(GetNextImageInList(next),morph_images->columns,
        morph_images->rows,GetNextImageInList(next)->filter,exception);
      if (morph_image == (Image *) NULL)
        {
          morph_images=DestroyImageList(morph_images);
          return((Image *) NULL);
        }
      image_view=AcquireVirtualCacheView(morph_image,exception);
      morph_view=AcquireAuthenticCacheView(morph_images,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(morph_image,morph_image,morph_image->rows,2)
#endif
      for (y=0; y < (ssize_t) morph_images->rows; y++)
      {
        const MorphFrameRowInfo
          info = { image_view, morph_image, exception, morph_view,
            morph_images, alpha, beta };

        MorphFrameRow(&status,&info,y);
      }
      morph_view=DestroyCacheView(morph_view);
      image_view=DestroyCacheView(image_view);
      morph_image=DestroyImage(morph_image);
    }
    if (n < (ssize_t) number_frames)
      break;
    /*
      Clone last frame in sequence.
    */
    morph_image=CloneImage(GetNextImageInList(next),0,0,MagickTrue,exception);
    if (morph_image == (Image *) NULL)
      {
        morph_images=DestroyImageList(morph_images);
        return((Image *) NULL);
      }
    AppendImageToList(&morph_images,morph_image);
    morph_images=GetLastImageInList(morph_images);
    status=ReportMorphProgress(image,scene,status);
    scene++;
  }
  if (GetNextImageInList(next) != (Image *) NULL)
    {
      morph_images=DestroyImageList(morph_images);
      return((Image *) NULL);
    }
  return(GetFirstImageInList(morph_images));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P o l a r o i d I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PolaroidImage() simulates a Polaroid picture.
%
%  The format of the PolaroidImage method is:
%
%      Image *PolaroidImage(const Image *image,const DrawInfo *draw_info,
%        const char *caption,const double angle,
%        const PixelInterpolateMethod method,ExceptionInfo exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o caption: the Polaroid caption.
%
%    o angle: Apply the effect along this angle.
%
%    o method: the pixel interpolation method.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void AnnotatePolaroidCaption(const Image *image,
  const DrawInfo *draw_info,Image **caption_image,size_t *height,char **text,
  ExceptionInfo *exception)
{
  char
    geometry[MagickPathExtent];

  DrawInfo
    *annotate_info;

  MagickBooleanType
    status;

  ssize_t
    count;

  TypeMetric
    metrics;

  annotate_info=CloneDrawInfo((const ImageInfo *) NULL,draw_info);
  (void) CloneString(&annotate_info->text,*text);
  count=FormatMagickCaption(*caption_image,annotate_info,MagickTrue,
    &metrics,text,exception);
  status=SetImageExtent(*caption_image,image->columns,(size_t)
    ((count+1)*(metrics.ascent-metrics.descent)+0.5),exception);
  if (status == MagickFalse)
    *caption_image=DestroyImage(*caption_image);
  else
    {
      (*caption_image)->background_color=image->border_color;
      (void) SetImageBackgroundColor(*caption_image,exception);
      (void) CloneString(&annotate_info->text,*text);
      (void) FormatLocaleString(geometry,MagickPathExtent,"+0+%.17g",
        metrics.ascent);
      if (annotate_info->gravity == UndefinedGravity)
        (void) CloneString(&annotate_info->geometry,AcquireString(
          geometry));
      (void) AnnotateImage(*caption_image,annotate_info,exception);
      *height+=(*caption_image)->rows;
    }
  annotate_info=DestroyDrawInfo(annotate_info);
  *text=DestroyString(*text);
}

typedef struct _PolaroidFinish
{
  const Image
    *image;

  PixelInterpolateMethod
    method;

  ssize_t
    quantum;

  double
    angle;
} PolaroidFinish;

static Image *FinishPolaroidImage(const PolaroidFinish *polaroid_finish,
  Image *picture_image,ExceptionInfo *exception)
{
  const Image
    *const image = polaroid_finish->image;

  const PixelInterpolateMethod
    method = polaroid_finish->method;

  const ssize_t
    quantum = polaroid_finish->quantum;

  const double
    angle = polaroid_finish->angle;

  Image
    *bend_image,
    *flop_image,
    *polaroid_image,
    *rotate_image,
    *trim_image;

  rotate_image=RotateImage(picture_image,90.0,exception);
  picture_image=DestroyImage(picture_image);
  if (rotate_image == (Image *) NULL)
    return((Image *) NULL);
  picture_image=rotate_image;
  bend_image=WaveImage(picture_image,0.01*picture_image->rows,2.0*
    picture_image->columns,method,exception);
  picture_image=DestroyImage(picture_image);
  if (bend_image == (Image *) NULL)
    return((Image *) NULL);
  picture_image=bend_image;
  rotate_image=RotateImage(picture_image,-90.0,exception);
  picture_image=DestroyImage(picture_image);
  if (rotate_image == (Image *) NULL)
    return((Image *) NULL);
  picture_image=rotate_image;
  picture_image->background_color=image->background_color;
  polaroid_image=ShadowImage(picture_image,80.0,2.0,quantum/3,quantum/3,
    exception);
  if (polaroid_image == (Image *) NULL)
    {
      picture_image=DestroyImage(picture_image);
      return(picture_image);
    }
  flop_image=FlopImage(polaroid_image,exception);
  polaroid_image=DestroyImage(polaroid_image);
  if (flop_image == (Image *) NULL)
    {
      picture_image=DestroyImage(picture_image);
      return(picture_image);
    }
  polaroid_image=flop_image;
  (void) CompositeImage(polaroid_image,picture_image,OverCompositeOp,
    MagickTrue,(ssize_t) (-0.01*picture_image->columns/2.0),0L,exception);
  picture_image=DestroyImage(picture_image);
  (void) QueryColorCompliance("none",AllCompliance,
    &polaroid_image->background_color,exception);
  rotate_image=RotateImage(polaroid_image,angle,exception);
  polaroid_image=DestroyImage(polaroid_image);
  if (rotate_image == (Image *) NULL)
    return((Image *) NULL);
  polaroid_image=rotate_image;
  trim_image=TrimImage(polaroid_image,exception);
  polaroid_image=DestroyImage(polaroid_image);
  if (trim_image == (Image *) NULL)
    return((Image *) NULL);
  polaroid_image=trim_image;
  return(polaroid_image);
}

static void DestroyPolaroidCaption(Image *caption_image)
{
  if (caption_image != (Image *) NULL)
    caption_image=DestroyImage(caption_image);
}

MagickExport Image *PolaroidImage(const Image *image,const DrawInfo *draw_info,
  const char *caption,const double angle,const PixelInterpolateMethod method,
  ExceptionInfo *exception)
{
  Image
    *caption_image,
    *picture_image;

  PolaroidFinish
    polaroid_finish;

  size_t
    height;

  ssize_t
    quantum;

  /*
    Simulate a Polaroid picture.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  quantum=(ssize_t) MagickMax(MagickMax((double) image->columns,(double)
    image->rows)/25.0,10.0);
  height=(size_t) ((ssize_t) image->rows+2*quantum);
  caption_image=(Image *) NULL;
  if (caption != (const char *) NULL)
    {
      char
        *text;

      /*
        Generate caption image.
      */
      caption_image=CloneImage(image,image->columns,1,MagickTrue,exception);
      if (caption_image == (Image *) NULL)
        return((Image *) NULL);
      text=InterpretImageProperties((ImageInfo *) NULL,(Image *) image,caption,
        exception);
      if (text != (char *) NULL)
        AnnotatePolaroidCaption(image,draw_info,&caption_image,&height,&text,
          exception);
    }
  picture_image=CloneImage(image,(size_t) ((ssize_t) image->columns+2*quantum),
    height,MagickTrue,exception);
  if (picture_image == (Image *) NULL)
    {
      DestroyPolaroidCaption(caption_image);
      return((Image *) NULL);
    }
  picture_image->background_color=image->border_color;
  (void) SetImageBackgroundColor(picture_image,exception);
  (void) CompositeImage(picture_image,image,OverCompositeOp,MagickTrue,quantum,
    quantum,exception);
  if (caption_image != (Image *) NULL)
    {
      (void) CompositeImage(picture_image,caption_image,OverCompositeOp,
        MagickTrue,quantum,((ssize_t) image->rows+3*quantum/2),exception);
      caption_image=DestroyImage(caption_image);
    }
  (void) QueryColorCompliance("none",AllCompliance,
    &picture_image->background_color,exception);
  (void) SetImageAlphaChannel(picture_image,OpaqueAlphaChannel,exception);
  polaroid_finish.image=image;
  polaroid_finish.method=method;
  polaroid_finish.quantum=quantum;
  polaroid_finish.angle=angle;
  return(FinishPolaroidImage(&polaroid_finish,picture_image,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S e p i a T o n e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SepiaToneImage() applies a special effect to the image, similar to the
%  effect achieved in a photo darkroom by sepia toning.  Threshold ranges from
%  0 to QuantumRange and is a measure of the extent of the sepia toning.  A
%  threshold of 80% is a good starting point for a reasonable tone.
%
%  The format of the SepiaToneImage method is:
%
%      Image *SepiaToneImage(const Image *image,const double threshold,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o threshold: the tone threshold.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SepiaTonePixel(const Image *image,const Image *sepia_image,
  const double threshold,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  double
    intensity,
    tone;

  intensity=GetPixelIntensity(image,p);
  tone=intensity > threshold ? (double) QuantumRange : intensity+
    (double) QuantumRange-threshold;
  SetPixelRed(sepia_image,ClampToQuantum(tone),q);
  tone=intensity > (7.0*threshold/6.0) ? (double) QuantumRange :
    intensity+(double) QuantumRange-7.0*threshold/6.0;
  SetPixelGreen(sepia_image,ClampToQuantum(tone),q);
  tone=intensity < (threshold/6.0) ? 0 : intensity-threshold/6.0;
  SetPixelBlue(sepia_image,ClampToQuantum(tone),q);
  tone=threshold/7.0;
  if ((double) GetPixelGreen(image,q) < tone)
    SetPixelGreen(sepia_image,ClampToQuantum(tone),q);
  if ((double) GetPixelBlue(image,q) < tone)
    SetPixelBlue(sepia_image,ClampToQuantum(tone),q);
  SetPixelAlpha(sepia_image,GetPixelAlpha(image,p),q);
}

typedef struct _SepiaToneRow
{
  const Image
    *image;

  Image
    *sepia_image;

  double
    threshold;
} SepiaToneRow;

static void SepiaToneImageRow(const SepiaToneRow *sepia_row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = sepia_row->image;

  Image
    *const sepia_image = sepia_row->sepia_image;

  const double
    threshold = sepia_row->threshold;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    SepiaTonePixel(image,sepia_image,threshold,p,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(sepia_image);
  }
}

static MagickBooleanType GetSepiaRows(const EffectRows *rows,
  const Quantum *magick_restrict *p,Quantum *magick_restrict *q,
  MagickBooleanType *status)
{
  if ((*status) == MagickFalse)
    return(MagickFalse);
  (*p)=GetCacheViewVirtualPixels(rows->image_view,0,rows->y,
    rows->image->columns,1,rows->exception);
  (*q)=GetCacheViewAuthenticPixels(rows->effect_view,0,rows->y,
    rows->effect_image->columns,1,rows->exception);
  if (((*p) == (const Quantum *) NULL) || ((*q) == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  return(MagickTrue);
}

MagickExport Image *SepiaToneImage(const Image *image,const double threshold,
  ExceptionInfo *exception)
{
#define SepiaToneImageTag  "SepiaTone/Image"

  CacheView
    *image_view,
    *sepia_view;

  Image
    *sepia_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Initialize sepia-toned image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (CloneEffectImage(image,&sepia_image,exception) == MagickFalse)
    return((Image *) NULL);
  /*
    Tone each row of the image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  sepia_view=AcquireAuthenticCacheView(sepia_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,sepia_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const EffectRows
      rows = { image,sepia_image,image_view,sepia_view,exception,y };

    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    SepiaToneRow
      sepia_row;

    if (GetSepiaRows(&rows,&p,&q,&status) == MagickFalse)
      continue;
    sepia_row.image=image;
    sepia_row.sepia_image=sepia_image;
    sepia_row.threshold=threshold;
    SepiaToneImageRow(&sepia_row,p,q);
    SyncEffectRow(sepia_view,exception,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(image,SepiaToneImageTag,progress,&status);
      }
  }
  sepia_view=DestroyCacheView(sepia_view);
  image_view=DestroyCacheView(image_view);
  (void) NormalizeImage(sepia_image,exception);
  (void) ContrastImage(sepia_image,MagickTrue,exception);
  return(ReleaseFailedImage(sepia_image,status));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S h a d o w I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShadowImage() simulates a shadow from the specified image and returns it.
%
%  The format of the ShadowImage method is:
%
%      Image *ShadowImage(const Image *image,const double alpha,
%        const double sigma,const ssize_t x_offset,const ssize_t y_offset,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o alpha: percentage transparency.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o x_offset: the shadow x-offset.
%
%    o y_offset: the shadow y-offset.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static PixelInfo ShadowImageRow(const Image *border_image,
  PixelInfo background_color,const double alpha,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) border_image->columns; x++)
  {
    if (border_image->alpha_trait != UndefinedPixelTrait)
      background_color.alpha=(double) GetPixelAlpha(border_image,q)*alpha/
        100.0;
    SetPixelViaPixelInfo(border_image,&background_color,q);
    q+=(ptrdiff_t) GetPixelChannels(border_image);
  }
  return(background_color);
}

static void OffsetShadowPage(Image *shadow_image,
  const RectangleInfo border_info,const ssize_t x_offset,const ssize_t y_offset)
{
  if (shadow_image->page.width == 0)
    shadow_image->page.width=shadow_image->columns;
  if (shadow_image->page.height == 0)
    shadow_image->page.height=shadow_image->rows;
  shadow_image->page.width=(size_t) ((ssize_t) shadow_image->page.width+
    x_offset-(ssize_t) border_info.width);
  shadow_image->page.height=(size_t) ((ssize_t) shadow_image->page.height+
    y_offset-(ssize_t) border_info.height);
  shadow_image->page.x+=x_offset-(ssize_t) border_info.width;
  shadow_image->page.y+=y_offset-(ssize_t) border_info.height;
}

static MagickBooleanType ShadowBorderImage(const Image *border_image,
  const double alpha,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  PixelInfo
    background_color;

  ssize_t
    y;

  status=MagickTrue;
  background_color=border_image->background_color;
  background_color.alpha_trait=BlendPixelTrait;
  image_view=AcquireAuthenticCacheView(border_image,exception);
  for (y=0; y < (ssize_t) border_image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=QueueCacheViewAuthenticPixels(image_view,0,y,border_image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    background_color=ShadowImageRow(border_image,background_color,alpha,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

MagickExport Image *ShadowImage(const Image *image,const double alpha,
  const double sigma,const ssize_t x_offset,const ssize_t y_offset,
  ExceptionInfo *exception)
{
#define ShadowImageTag  "Shadow/Image"

  ChannelType
    channel_mask;

  Image
    *border_image,
    *clone_image,
    *shadow_image;

  MagickBooleanType
    status;

  RectangleInfo
    border_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  clone_image=CloneImage(image,0,0,MagickTrue,exception);
  if (clone_image == (Image *) NULL)
    return((Image *) NULL);
  if (IsGrayColorspace(image->colorspace) != MagickFalse)
    (void) SetImageColorspace(clone_image,sRGBColorspace,exception);
  (void) SetImageVirtualPixelMethod(clone_image,EdgeVirtualPixelMethod,
    exception);
  border_info.width=CastDoubleToSizeT(2.0*sigma+0.5);
  border_info.height=CastDoubleToSizeT(2.0*sigma+0.5);
  border_info.x=0;
  border_info.y=0;
  (void) QueryColorCompliance("none",AllCompliance,&clone_image->border_color,
    exception);
  clone_image->alpha_trait=BlendPixelTrait;
  border_image=BorderImage(clone_image,&border_info,OverCompositeOp,exception);
  clone_image=DestroyImage(clone_image);
  if (border_image == (Image *) NULL)
    return((Image *) NULL);
  if (border_image->alpha_trait == UndefinedPixelTrait)
    (void) SetImageAlphaChannel(border_image,OpaqueAlphaChannel,exception);
  /*
    Shadow image.
  */
  status=ShadowBorderImage(border_image,alpha,exception);
  if (status == MagickFalse)
    {
      border_image=DestroyImage(border_image);
      return((Image *) NULL);
    }
  channel_mask=SetImageChannelMask(border_image,AlphaChannel);
  shadow_image=BlurImage(border_image,0.0,sigma,exception);
  border_image=DestroyImage(border_image);
  if (shadow_image == (Image *) NULL)
    return((Image *) NULL);
  (void) SetPixelChannelMask(shadow_image,channel_mask);
  OffsetShadowPage(shadow_image,border_info,x_offset,y_offset);
  return(shadow_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S k e t c h I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SketchImage() simulates a pencil sketch.  We convolve the image with a
%  Gaussian operator of the given radius and standard deviation (sigma).  For
%  reasonable results, radius should be larger than sigma.  Use a radius of 0
%  and SketchImage() selects a suitable radius for you.  Angle gives the angle
%  of the sketch.
%
%  The format of the SketchImage method is:
%
%    Image *SketchImage(const Image *image,const double radius,
%      const double sigma,const double angle,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the Gaussian, in pixels, not counting the
%      center pixel.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o angle: apply the effect along this angle.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SetRandomPixelChannels(const Image *image,
  const Image *random_image,const double value,Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(random_image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if (traits == UndefinedPixelTrait)
      continue;
    q[i]=ClampToQuantum((double) QuantumRange*value);
  }
}

static Image *BlendSketchImage(const Image *image,Image *blur_image,
  const double radius,ExceptionInfo *exception)
{
  Image
    *blend_image,
    *dodge_image,
    *sketch_image;

  MagickBooleanType
    status;

  dodge_image=EdgeImage(blur_image,radius,exception);
  blur_image=DestroyImage(blur_image);
  if (dodge_image == (Image *) NULL)
    return((Image *) NULL);
  status=ClampImage(dodge_image,exception);
  if (status != MagickFalse)
    status=NormalizeImage(dodge_image,exception);
  if (status != MagickFalse)
    status=NegateImage(dodge_image,MagickFalse,exception);
  if (status != MagickFalse)
    status=TransformImage(&dodge_image,(char *) NULL,"50%",exception);
  sketch_image=CloneImage(image,0,0,MagickTrue,exception);
  if (sketch_image == (Image *) NULL)
    {
      dodge_image=DestroyImage(dodge_image);
      return((Image *) NULL);
    }
  (void) CompositeImage(sketch_image,dodge_image,ColorDodgeCompositeOp,
    MagickTrue,0,0,exception);
  dodge_image=DestroyImage(dodge_image);
  blend_image=CloneImage(image,0,0,MagickTrue,exception);
  if (blend_image == (Image *) NULL)
    {
      sketch_image=DestroyImage(sketch_image);
      return((Image *) NULL);
    }
  if (blend_image->alpha_trait != BlendPixelTrait)
    (void) SetImageAlpha(blend_image,TransparentAlpha,exception);
  (void) SetImageArtifact(blend_image,"compose:args","20x80");
  (void) CompositeImage(sketch_image,blend_image,BlendCompositeOp,MagickTrue,
    0,0,exception);
  blend_image=DestroyImage(blend_image);
  return(sketch_image);
}

typedef struct _SketchBlur
{
  const Image
    *image;

  double
    radius,
    sigma,
    angle;
} SketchBlur;

static Image *BlurSketchImage(const SketchBlur *sketch_blur,
  Image *random_image,const MagickBooleanType status,ExceptionInfo *exception)
{
  const Image
    *const image = sketch_blur->image;

  const double
    radius = sketch_blur->radius,
    sigma = sketch_blur->sigma,
    angle = sketch_blur->angle;

  Image
    *blur_image;

  if (status == MagickFalse)
    {
      random_image=DestroyImage(random_image);
      return(random_image);
    }
  blur_image=MotionBlurImage(random_image,radius,sigma,angle,exception);
  random_image=DestroyImage(random_image);
  if (blur_image == (Image *) NULL)
    return((Image *) NULL);
  return(BlendSketchImage(image,blur_image,radius,exception));
}

typedef struct _SketchRandomRow
{
  const Image
    *image;

  Image
    *random_image;

  CacheView
    *random_view;

  RandomInfo
    **random_info;

  int
    id;

  ssize_t
    y;

  ExceptionInfo
    *exception;
} SketchRandomRow;

static MagickBooleanType SetRandomSketchRow(const SketchRandomRow *sketch_row)
{
  const Image
    *const image = sketch_row->image;

  Image
    *const random_image = sketch_row->random_image;

  CacheView
    *const random_view = sketch_row->random_view;

  RandomInfo
    **const random_info = sketch_row->random_info;

  const int
    id = sketch_row->id;

  const ssize_t
    y = sketch_row->y;

  ExceptionInfo
    *const exception = sketch_row->exception;

  MagickBooleanType
    status;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  status=MagickTrue;
  q=QueueCacheViewAuthenticPixels(random_view,0,y,random_image->columns,1,
    exception);
  if (q == (Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  for (x=0; x < (ssize_t) random_image->columns; x++)
  {
    double
      value;

    value=GetPseudoRandomValue(random_info[id]);
    SetRandomPixelChannels(image,random_image,value,q);
    q+=(ptrdiff_t) GetPixelChannels(random_image);
  }
  if (SyncCacheViewAuthenticPixels(random_view,exception) == MagickFalse)
    status=MagickFalse;
  return(status);
}

MagickExport Image *SketchImage(const Image *image,const double radius,
  const double sigma,const double angle,ExceptionInfo *exception)
{
  CacheView
    *random_view;

  Image
    *random_image;

  MagickBooleanType
    status;

  RandomInfo
    **magick_restrict random_info;

  SketchBlur
    sketch_blur;

  ssize_t
    y;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
  unsigned long
    key;
#endif

  /*
    Sketch image.
  */
  random_image=CloneImage(image,image->columns << 1,image->rows << 1,
    MagickTrue,exception);
  if (random_image == (Image *) NULL)
    return((Image *) NULL);
  status=MagickTrue;
  random_info=AcquireRandomInfoTLS();
  random_view=AcquireAuthenticCacheView(random_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  key=GetRandomSecretKey(random_info[0]);
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(random_image,random_image,random_image->rows,key == ~0UL ? 0 : 2)
#endif
  for (y=0; y < (ssize_t) random_image->rows; y++)
  {
    const int
      id = GetOpenMPThreadId();

    SketchRandomRow
      sketch_row;

    if (status == MagickFalse)
      continue;
    sketch_row.image=image;
    sketch_row.random_image=random_image;
    sketch_row.random_view=random_view;
    sketch_row.random_info=random_info;
    sketch_row.id=id;
    sketch_row.y=y;
    sketch_row.exception=exception;
    if (SetRandomSketchRow(&sketch_row) == MagickFalse)
      status=MagickFalse;
  }
  random_view=DestroyCacheView(random_view);
  random_info=DestroyRandomInfoTLS(random_info);
  sketch_blur.image=image;
  sketch_blur.radius=radius;
  sketch_blur.sigma=sigma;
  sketch_blur.angle=angle;
  return(BlurSketchImage(&sketch_blur,random_image,status,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S o l a r i z e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SolarizeImage() applies a special effect to the image, similar to the effect
%  achieved in a photo darkroom by selectively exposing areas of photo
%  sensitive paper to light.  Threshold ranges from 0 to QuantumRange and is a
%  measure of the extent of the solarization.
%
%  The format of the SolarizeImage method is:
%
%      MagickBooleanType SolarizeImage(Image *image,const double threshold,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o threshold:  Define the extent of the solarization.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SolarizePixelChannels(const Image *image,const double threshold,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    if ((double) q[i] > threshold)
      q[i]=QuantumRange-q[i];
  }
}

static void SolarizeColormap(Image *image,const double threshold)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) image->colors; i++)
  {
    if ((double) image->colormap[i].red > threshold)
      image->colormap[i].red=(double) QuantumRange-image->colormap[i].red;
    if ((double) image->colormap[i].green > threshold)
      image->colormap[i].green=(double) QuantumRange-
        image->colormap[i].green;
    if ((double) image->colormap[i].blue > threshold)
      image->colormap[i].blue=(double) QuantumRange-image->colormap[i].blue;
  }
}

typedef struct _SolarizeRow
{
  Image
    *image;

  CacheView
    *image_view;

  double
    threshold;

  ExceptionInfo
    *exception;
} SolarizeRow;

static MagickBooleanType SolarizeImageRow(const SolarizeRow *solarize_row,
  const ssize_t y,MagickBooleanType *status)
{
  Image
    *const image = solarize_row->image;

  CacheView
    *const image_view = solarize_row->image_view;

  const double
    threshold = solarize_row->threshold;

  ExceptionInfo
    *const exception = solarize_row->exception;

  ssize_t
    x;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    SolarizePixelChannels(image,threshold,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

MagickExport MagickBooleanType SolarizeImage(Image *image,
  const double threshold,ExceptionInfo *exception)
{
#define SolarizeImageTag  "Solarize/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (IsGrayColorspace(image->colorspace) != MagickFalse)
    (void) SetImageColorspace(image,sRGBColorspace,exception);
  if (image->storage_class == PseudoClass)
    {
      /*
        Solarize colormap.
      */
      SolarizeColormap(image,threshold);
      return(SyncImage(image,exception));
    }
  /*
    Solarize image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    SolarizeRow
      solarize_row;

    solarize_row.image=image;
    solarize_row.image_view=image_view;
    solarize_row.threshold=threshold;
    solarize_row.exception=exception;
    if (SolarizeImageRow(&solarize_row,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(image,SolarizeImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S w i r l I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SwirlImage() swirls the pixels about the center of the image, where
%  degrees indicates the sweep of the arc through which each pixel is moved.
%  You get a more dramatic effect as the degrees move from 1 to 360.
%
%  The format of the SwirlImage method is:
%
%      Image *SwirlImage(const Image *image,double degrees,
%        const PixelInterpolateMethod method,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o degrees: Define the tightness of the swirling effect.
%
%    o method: the pixel interpolation method.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType SwirlPixel(const WarpInfo *warp,
  const WarpSite *site,Quantum *magick_restrict q,MagickBooleanType *status)
{
  double
    cosine,
    factor,
    sine;

  /*
    Swirl the pixel.
  */
  factor=1.0-sqrt((double) site->distance)/warp->radius;
  sine=sin((double) (warp->amount*factor*factor));
  cosine=cos((double) (warp->amount*factor*factor));
  (*status)=InterpolatePixelChannels(warp->canvas_image,warp->interpolate_view,
    warp->warp_image,warp->method,((cosine*site->delta.x-sine*site->delta.y)/
    warp->scale.x+warp->center.x),(double) ((sine*site->delta.x+cosine*
    site->delta.y)/warp->scale.y+warp->center.y),q,warp->exception);
  if ((*status) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static PointInfo GetSwirlScale(const Image *canvas_image)
{
  PointInfo
    scale;

  scale.x=1.0;
  scale.y=1.0;
  if (canvas_image->columns > canvas_image->rows)
    scale.y=(double) canvas_image->columns/(double) canvas_image->rows;
  else
    if (canvas_image->columns < canvas_image->rows)
      scale.x=(double) canvas_image->rows/(double) canvas_image->columns;
  return(scale);
}

MagickExport Image *SwirlImage(const Image *image,double degrees,
  const PixelInterpolateMethod method,ExceptionInfo *exception)
{
#define SwirlImageTag  "Swirl/Image"

  CacheView
    *canvas_view,
    *interpolate_view,
    *swirl_view;

  double
    radius;

  Image
    *canvas_image,
    *swirl_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PointInfo
    center,
    scale;

  ssize_t
    y;

  /*
    Initialize swirl image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  canvas_image=CloneImage(image,0,0,MagickTrue,exception);
  if (canvas_image == (Image *) NULL)
    return((Image *) NULL);
  if (CloneWarpImage(canvas_image,&swirl_image,exception) == MagickFalse)
    return((Image *) NULL);
  if (swirl_image->background_color.alpha_trait != UndefinedPixelTrait)
    (void) SetImageAlphaChannel(swirl_image,OnAlphaChannel,exception);
  /*
    Compute scaling factor.
  */
  center.x=(double) canvas_image->columns/2.0;
  center.y=(double) canvas_image->rows/2.0;
  radius=MagickMax(center.x,center.y);
  scale=GetSwirlScale(canvas_image);
  degrees=(double) DegreesToRadians(degrees);
  /*
    Swirl image.
  */
  status=MagickTrue;
  progress=0;
  canvas_view=AcquireVirtualCacheView(canvas_image,exception);
  interpolate_view=AcquireVirtualCacheView(image,exception);
  swirl_view=AcquireAuthenticCacheView(swirl_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(canvas_image,swirl_image,canvas_image->rows,1)
#endif
  for (y=0; y < (ssize_t) canvas_image->rows; y++)
  {
    const EffectRows
      rows = { canvas_image,swirl_image,canvas_view,swirl_view,exception,y };

    const WarpInfo
      warp = { image,canvas_image,swirl_image,interpolate_view,exception,
        method,center,scale,degrees,radius,y };

    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    Quantum
      *magick_restrict q;

    WarpSite
      site;

    if (GetEffectRows(&rows,&p,&q,&status) == MagickFalse)
      continue;
    site.delta.y=scale.y*(double) (y-center.y);
    for (x=0; x < (ssize_t) canvas_image->columns; x++)
    {
      /*
        Determine if the pixel is within an ellipse.
      */
      site.delta.x=scale.x*(double) (x-center.x);
      site.distance=site.delta.x*site.delta.x+site.delta.y*site.delta.y;
      if (site.distance >= (radius*radius))
        CopyPixelChannels(canvas_image,swirl_image,p,q);
      else
        if (SwirlPixel(&warp,&site,q,&status) == MagickFalse)
          break;
      p+=(ptrdiff_t) GetPixelChannels(canvas_image);
      q+=(ptrdiff_t) GetPixelChannels(swirl_image);
    }
    SyncEffectRow(swirl_view,exception,&status);
    if (canvas_image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(canvas_image,SwirlImageTag,progress,&status);
      }
  }
  DestroyWarpResources(swirl_view,interpolate_view,canvas_view,canvas_image);
  return(ReleaseFailedImage(swirl_image,status));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     T i n t I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TintImage() applies a color vector to each pixel in the image.  The length
%  of the vector is 0 for black and white and at its maximum for the midtones.
%  The vector weighting function is f(x)=(1-(4.0*((x-0.5)*(x-0.5))))
%
%  The format of the TintImage method is:
%
%      Image *TintImage(const Image *image,const char *blend,
%        const PixelInfo *tint,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o blend: A color value used for tinting.
%
%    o tint: A color value used for tinting.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static PixelInfo ParseTintColorVector(const Image *image,const char *blend)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  PixelInfo
    color_vector;

  GetPixelInfo(image,&color_vector);
  flags=ParseGeometry(blend,&geometry_info);
  color_vector.red=geometry_info.rho;
  color_vector.green=geometry_info.rho;
  color_vector.blue=geometry_info.rho;
  color_vector.alpha=(MagickRealType) OpaqueAlpha;
  if ((flags & SigmaValue) != 0)
    color_vector.green=geometry_info.sigma;
  if ((flags & XiValue) != 0)
    color_vector.blue=geometry_info.xi;
  if ((flags & PsiValue) != 0)
    color_vector.alpha=geometry_info.psi;
  if (image->colorspace == CMYKColorspace)
    {
      color_vector.black=geometry_info.rho;
      if ((flags & PsiValue) != 0)
        color_vector.black=geometry_info.psi;
      if ((flags & ChiValue) != 0)
        color_vector.alpha=geometry_info.chi;
    }
  return(color_vector);
}

static void TintPixel(const Image *image,const Image *tint_image,
  const PixelInfo *color_vector,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  PixelInfo
    pixel;

  double
    weight;

  GetPixelInfo(image,&pixel);
  weight=QuantumScale*(double) GetPixelRed(image,p)-0.5;
  pixel.red=(MagickRealType) GetPixelRed(image,p)+color_vector->red*
    (1.0-(4.0*(weight*weight)));
  weight=QuantumScale*(double) GetPixelGreen(image,p)-0.5;
  pixel.green=(MagickRealType) GetPixelGreen(image,p)+color_vector->green*
    (1.0-(4.0*(weight*weight)));
  weight=QuantumScale*(double) GetPixelBlue(image,p)-0.5;
  pixel.blue=(MagickRealType) GetPixelBlue(image,p)+color_vector->blue*
    (1.0-(4.0*(weight*weight)));
  weight=QuantumScale*(double) GetPixelBlack(image,p)-0.5;
  pixel.black=(MagickRealType) GetPixelBlack(image,p)+color_vector->black*
    (1.0-(4.0*(weight*weight)));
  pixel.alpha=(MagickRealType) GetPixelAlpha(image,p);
  SetPixelViaPixelInfo(tint_image,&pixel,q);
}

static void SetTintColorspace(const Image *image,const PixelInfo *tint,
  Image *tint_image,ExceptionInfo *exception)
{
  if ((IsGrayColorspace(image->colorspace) != MagickFalse) &&
      (IsPixelInfoGray(tint) == MagickFalse))
    (void) SetImageColorspace(tint_image,sRGBColorspace,exception);
}

static PixelInfo ScaleTintColorVector(PixelInfo color_vector,
  const PixelInfo *tint)
{
  double
    intensity;

  intensity=(double) GetPixelInfoIntensity((const Image *) NULL,tint);
  color_vector.red=(double) (color_vector.red*tint->red/100.0-intensity);
  color_vector.green=(double) (color_vector.green*tint->green/100.0-intensity);
  color_vector.blue=(double) (color_vector.blue*tint->blue/100.0-intensity);
  color_vector.black=(double) (color_vector.black*tint->black/100.0-intensity);
  color_vector.alpha=(double) (color_vector.alpha*tint->alpha/100.0-intensity);
  return(color_vector);
}

typedef struct _TintRow
{
  const Image
    *image;

  Image
    *tint_image;

  PixelInfo
    color_vector;
} TintRow;

static void TintImageRow(const TintRow *tint_row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = tint_row->image;

  Image
    *const tint_image = tint_row->tint_image;

  const PixelInfo
    color_vector = tint_row->color_vector;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    TintPixel(image,tint_image,&color_vector,p,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(tint_image);
  }
}

MagickExport Image *TintImage(const Image *image,const char *blend,
  const PixelInfo *tint,ExceptionInfo *exception)
{
#define TintImageTag  "Tint/Image"

  CacheView
    *image_view,
    *tint_view;

  Image
    *tint_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    color_vector;

  ssize_t
    y;

  /*
    Allocate tint image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (CloneEffectImage(image,&tint_image,exception) == MagickFalse)
    return((Image *) NULL);
  SetTintColorspace(image,tint,tint_image,exception);
  if (blend == (const char *) NULL)
    return(tint_image);
  /*
    Determine RGB values of the color.
  */
  color_vector=ParseTintColorVector(image,blend);
  color_vector=ScaleTintColorVector(color_vector,tint);
  /*
    Tint image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  tint_view=AcquireAuthenticCacheView(tint_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,tint_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const EffectRows
      rows = { image,tint_image,image_view,tint_view,exception,y };

    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    TintRow
      tint_row;

    if (GetEffectRows(&rows,&p,&q,&status) == MagickFalse)
      continue;
    tint_row.image=image;
    tint_row.tint_image=tint_image;
    tint_row.color_vector=color_vector;
    TintImageRow(&tint_row,p,q);
    SyncEffectRow(tint_view,exception,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(image,TintImageTag,progress,&status);
      }
  }
  tint_view=DestroyCacheView(tint_view);
  image_view=DestroyCacheView(image_view);
  return(ReleaseFailedImage(tint_image,status));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     V i g n e t t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  VignetteImage() softens the edges of the image in vignette style.
%
%  The format of the VignetteImage method is:
%
%      Image *VignetteImage(const Image *image,const double radius,
%        const double sigma,const ssize_t x,const ssize_t y,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the pixel neighborhood.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o x, y:  Define the x and y ellipse offset.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *VignetteImage(const Image *image,const double radius,
  const double sigma,const ssize_t x,const ssize_t y,ExceptionInfo *exception)
{
  char
    ellipse[MagickPathExtent];

  DrawInfo
    *draw_info;

  Image
    *canvas,
    *blur_image,
    *oval_image,
    *vignette_image;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  canvas=CloneImage(image,0,0,MagickTrue,exception);
  if (canvas == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(canvas,DirectClass,exception) == MagickFalse)
    {
      canvas=DestroyImage(canvas);
      return((Image *) NULL);
    }
  canvas->alpha_trait=BlendPixelTrait;
  oval_image=CloneImage(canvas,canvas->columns,canvas->rows,MagickTrue,
    exception);
  if (oval_image == (Image *) NULL)
    {
      canvas=DestroyImage(canvas);
      return((Image *) NULL);
    }
  (void) QueryColorCompliance("#000000",AllCompliance,
    &oval_image->background_color,exception);
  (void) SetImageBackgroundColor(oval_image,exception);
  draw_info=CloneDrawInfo((const ImageInfo *) NULL,(const DrawInfo *) NULL);
  (void) QueryColorCompliance("#ffffff",AllCompliance,&draw_info->fill,
    exception);
  (void) QueryColorCompliance("#ffffff",AllCompliance,&draw_info->stroke,
    exception);
  (void) FormatLocaleString(ellipse,MagickPathExtent,"ellipse %g,%g,%g,%g,"
    "0.0,360.0",image->columns/2.0,image->rows/2.0,image->columns/2.0-x,
    image->rows/2.0-y);
  draw_info->primitive=AcquireString(ellipse);
  (void) DrawImage(oval_image,draw_info,exception);
  draw_info=DestroyDrawInfo(draw_info);
  blur_image=BlurImage(oval_image,radius,sigma,exception);
  oval_image=DestroyImage(oval_image);
  if (blur_image == (Image *) NULL)
    {
      canvas=DestroyImage(canvas);
      return((Image *) NULL);
    }
  blur_image->alpha_trait=UndefinedPixelTrait;
  (void) CompositeImage(canvas,blur_image,IntensityCompositeOp,MagickTrue,
    0,0,exception);
  blur_image=DestroyImage(blur_image);
  vignette_image=MergeImageLayers(canvas,FlattenLayer,exception);
  canvas=DestroyImage(canvas);
  if (vignette_image != (Image *) NULL)
    (void) TransformImageColorspace(vignette_image,image->colorspace,exception);
  return(vignette_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     W a v e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WaveImage() creates a "ripple" effect in the image by shifting the pixels
%  vertically along a sine wave whose amplitude and wavelength is specified
%  by the given parameters.
%
%  The format of the WaveImage method is:
%
%      Image *WaveImage(const Image *image,const double amplitude,
%        const double wave_length,const PixelInterpolateMethod method,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o amplitude, wave_length:  Define the amplitude and wave length of the
%      sine wave.
%
%    o interpolate: the pixel interpolation method.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _WaveRow
{
  Image
    *canvas_image,
    *wave_image;

  CacheView
    *canvas_image_view,
    *wave_view;

  float
    *sine_map;

  PixelInterpolateMethod
    method;

  ExceptionInfo
    *exception;
} WaveRow;

static MagickBooleanType WaveImageRow(const WaveRow *wave_row,const ssize_t y,
  MagickBooleanType *status)
{
  Image
    *const canvas_image = wave_row->canvas_image,
    *const wave_image = wave_row->wave_image;

  CacheView
    *const canvas_image_view = wave_row->canvas_image_view,
    *const wave_view = wave_row->wave_view;

  float
    *const sine_map = wave_row->sine_map;

  const PixelInterpolateMethod
    method = wave_row->method;

  ExceptionInfo
    *const exception = wave_row->exception;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(canvas_image_view,0,y,canvas_image->columns,1,
    exception);
  q=QueueCacheViewAuthenticPixels(wave_view,0,y,wave_image->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  for (x=0; x < (ssize_t) wave_image->columns; x++)
  {
    (*status)=InterpolatePixelChannels(canvas_image,canvas_image_view,
      wave_image,method,(double) x,(double) (y-sine_map[x]),q,exception);
    if ((*status) == MagickFalse)
      break;
    p+=(ptrdiff_t) GetPixelChannels(canvas_image);
    q+=(ptrdiff_t) GetPixelChannels(wave_image);
  }
  if (SyncCacheViewAuthenticPixels(wave_view,exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static void FillSineMap(float *sine_map,const size_t columns,
  const double amplitude,const double wave_length)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) columns; i++)
    sine_map[i]=(float) (fabs(amplitude)+amplitude*sin((double)
      ((2.0*MagickPI*i)*(double) MagickSafeReciprocal(wave_length))));
}

static void SetWaveCanvasAlpha(Image *canvas_image,ExceptionInfo *exception)
{
  if ((canvas_image->alpha_trait == UndefinedPixelTrait) &&
      (canvas_image->background_color.alpha != (double) OpaqueAlpha))
    (void) SetImageAlpha(canvas_image,OpaqueAlpha,exception);
}

MagickExport Image *WaveImage(const Image *image,const double amplitude,
  const double wave_length,const PixelInterpolateMethod method,
  ExceptionInfo *exception)
{
#define WaveImageTag  "Wave/Image"

  CacheView
    *canvas_image_view,
    *wave_view;

  float
    *sine_map;

  Image
    *canvas_image,
    *wave_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Initialize wave image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  canvas_image=CloneImage(image,0,0,MagickTrue,exception);
  if (canvas_image == (Image *) NULL)
    return((Image *) NULL);
  SetWaveCanvasAlpha(canvas_image,exception);
  wave_image=CloneImage(canvas_image,canvas_image->columns,(size_t)
    (canvas_image->rows+2.0*fabs(amplitude)),MagickTrue,exception);
  if (wave_image == (Image *) NULL)
    {
      canvas_image=DestroyImage(canvas_image);
      return((Image *) NULL);
    }
  if (SetImageStorageClass(wave_image,DirectClass,exception) == MagickFalse)
    {
      canvas_image=DestroyImage(canvas_image);
      wave_image=DestroyImage(wave_image);
      return((Image *) NULL);
    }
  /*
    Allocate sine map.
  */
  sine_map=(float *) AcquireQuantumMemory((size_t) wave_image->columns,
    sizeof(*sine_map));
  if (sine_map == (float *) NULL)
    {
      canvas_image=DestroyImage(canvas_image);
      wave_image=DestroyImage(wave_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  FillSineMap(sine_map,wave_image->columns,amplitude,wave_length);
  /*
    Wave image.
  */
  status=MagickTrue;
  progress=0;
  canvas_image_view=AcquireVirtualCacheView(canvas_image,exception);
  wave_view=AcquireAuthenticCacheView(wave_image,exception);
  (void) SetCacheViewVirtualPixelMethod(canvas_image_view,
    BackgroundVirtualPixelMethod);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(canvas_image,wave_image,wave_image->rows,1)
#endif
  for (y=0; y < (ssize_t) wave_image->rows; y++)
  {
    const WaveRow
      wave_row = { canvas_image, wave_image, canvas_image_view, wave_view,
        sine_map, method, exception };

    if (WaveImageRow(&wave_row,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(canvas_image,WaveImageTag,progress,&status);
      }
  }
  wave_view=DestroyCacheView(wave_view);
  canvas_image_view=DestroyCacheView(canvas_image_view);
  canvas_image=DestroyImage(canvas_image);
  sine_map=(float *) RelinquishMagickMemory(sine_map);
  return(ReleaseFailedImage(wave_image,status));
}

