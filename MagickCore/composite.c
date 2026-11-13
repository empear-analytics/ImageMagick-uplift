/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        CCCC   OOO   M   M  PPPP    OOO   SSSSS  IIIII  TTTTT  EEEEE         %
%       C      O   O  MM MM  P   P  O   O  SS       I      T    E             %
%       C      O   O  M M M  PPPP   O   O   SSS     I      T    EEE           %
%       C      O   O  M   M  P      O   O     SS    I      T    E             %
%        CCCC   OOO   M   M  P       OOO   SSSSS  IIIII    T    EEEEE         %
%                                                                             %
%                                                                             %
%                     MagickCore Image Composite Methods                      %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
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
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/morphology.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/resample.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"

/*
  Typedef declarations.
*/
typedef struct _BlendSumWeights
{
  double
    attenuate,
    sign;
} BlendSumWeights;

typedef struct _BlendMagnitudePair
{
  const Image
    *alpha_image,
    *beta_image;
} BlendMagnitudePair;

typedef struct _BlendRequest
{
  ssize_t
    x_offset,
    y_offset;

  double
    iterations,
    residual_threshold;

  size_t
    tick;
} BlendRequest;

typedef struct _CompositePlacement
{
  MagickBooleanType
    clip_to_self;

  ssize_t
    x_offset,
    y_offset;
} CompositePlacement;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p o s i t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompositeImage() returns the second image composited onto the first
%  at the specified offset, using the specified composite method.
%
%  The format of the CompositeImage method is:
%
%      MagickBooleanType CompositeImage(Image *image,
%        const Image *source_image,const CompositeOperator compose,
%        const MagickBooleanType clip_to_self,const ssize_t x_offset,
%        const ssize_t y_offset,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the canvas image, modified by he composition
%
%    o source_image: the source image.
%
%    o compose: This operator affects how the composite is applied to
%      the image.  The operators and how they are utilized are listed here
%      http://www.w3.org/TR/SVG12/#compositing.
%
%    o clip_to_self: set to MagickTrue to limit composition to area composed.
%
%    o x_offset: the column offset of the composited image.
%
%    o y_offset: the row offset of the composited image.
%
%  Extra Controls from Image meta-data in 'image' (artifacts)
%
%    o "compose:args"
%        A string containing extra numerical arguments for specific compose
%        methods, generally expressed as a 'geometry' or a comma separated list
%        of numbers.
%
%        Compose methods needing such arguments include "BlendCompositeOp" and
%        "DisplaceCompositeOp".
%
%    o exception: return any errors or warnings in this structure.
%
*/

/*
   Composition based on the SVG specification:

   A Composition is defined by...
      Color Function :  f(Sc,Dc)  where Sc and Dc are the normalized colors
      Blending areas :  X = 1     for area of overlap, ie: f(Sc,Dc)
                        Y = 1     for source preserved
                        Z = 1     for canvas preserved

   Conversion to transparency (then optimized)
      Dca' = f(Sc, Dc)*Sa*Da + Y*Sca*(1-Da) + Z*Dca*(1-Sa)
      Da'  = X*Sa*Da + Y*Sa*(1-Da) + Z*Da*(1-Sa)

   Where...
      Sca = Sc*Sa     normalized Source color divided by Source alpha
      Dca = Dc*Da     normalized Dest color divided by Dest alpha
      Dc' = Dca'/Da'  the desired color value for this channel.

   Da' in in the follow formula as 'gamma'  The resulting alpha value.

   Most functions use a blending mode of over (X=1,Y=1,Z=1) this results in
   the following optimizations...
      gamma = Sa+Da-Sa*Da;
      gamma = 1 - QuantumScale*alpha * QuantumScale*beta;
      opacity = QuantumScale*alpha*beta;  // over blend, optimized 1-Gamma

   The above SVG definitions also define that Mathematical Composition
   methods should use a 'Over' blending mode for Alpha Channel.
   It however was not applied for composition modes of 'Plus', 'Minus',
   the modulus versions of 'Add' and 'Subtract'.

   Mathematical operator changes to be applied from IM v6.7...

    1) Modulus modes 'Add' and 'Subtract' are obsoleted and renamed
       'ModulusAdd' and 'ModulusSubtract' for clarity.

    2) All mathematical compositions work as per the SVG specification
       with regard to blending.  This now includes 'ModulusAdd' and
       'ModulusSubtract'.

    3) When the special channel flag 'sync' (synchronize channel updates)
       is turned off (enabled by default) then mathematical compositions are
       only performed on the channels specified, and are applied
       independently of each other.  In other words the mathematics is
       performed as 'pure' mathematical operations, rather than as image
       operations.
*/

static Image *BlendConvolveImage(const Image *image,const char *kernel,
  ExceptionInfo *exception)
{
  Image
    *clone_image,
    *convolve_image;

  KernelInfo
    *kernel_info;

  /*
    Convolve image with a kernel.
  */
  kernel_info=AcquireKernelInfo(kernel,exception);
  if (kernel_info == (KernelInfo *) NULL)
    return((Image *) NULL);
  clone_image=CloneImage(image,0,0,MagickTrue,exception);
  if (clone_image == (Image *) NULL)
    {
      kernel_info=DestroyKernelInfo(kernel_info);
      return((Image *) NULL);
    }
  (void) SetImageAlphaChannel(clone_image,OffAlphaChannel,exception);
  convolve_image=ConvolveImage(clone_image,kernel_info,exception);
  kernel_info=DestroyKernelInfo(kernel_info);
  clone_image=DestroyImage(clone_image);
  return(convolve_image);
}

static inline MagickBooleanType IsBlendRowMissing(const Quantum *p,
  const Quantum *q,const Quantum *r)
{
  return(((p == (const Quantum *) NULL) || (q == (const Quantum *) NULL) ||
    (r == (Quantum *) NULL)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsBlendChannelSkipped(const PixelTrait traits,
  const PixelTrait other_traits)
{
  return(((traits == UndefinedPixelTrait) ||
    (other_traits == UndefinedPixelTrait) ||
    ((other_traits & UpdatePixelTrait) == 0)) ? MagickTrue : MagickFalse);
}

typedef struct _BlendGradients
{
  const Image
    *dx_image,
    *dy_image;
} BlendGradients;

static void SetMagnitudePixel(const BlendGradients *gradients,
  const Quantum *magick_restrict p,const Quantum *magick_restrict q,
  Quantum *magick_restrict r)
{
  const Image
    *dx_image = gradients->dx_image,
    *dy_image = gradients->dy_image;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(dx_image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(dx_image,i);
    PixelTrait traits = GetPixelChannelTraits(dx_image,channel);
    PixelTrait dy_traits = GetPixelChannelTraits(dy_image,channel);
    if (IsBlendChannelSkipped(traits,dy_traits) != MagickFalse)
      continue;
    r[i]=ClampToQuantum(hypot((double) p[i],(double)
      GetPixelChannel(dy_image,channel,q)));
  }
}

static void SyncCompositeRow(CacheView *view,ExceptionInfo *exception,
  MagickBooleanType *status)
{
  if (SyncCacheViewAuthenticPixels(view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

typedef struct _MagnitudeLine
{
  CacheView
    *dx_view,
    *dy_view,
    *magnitude_view;

  const Image
    *dx_image,
    *dy_image;

  Image
    *magnitude_image;

  ExceptionInfo
    *exception;
} MagnitudeLine;

static void SetMagnitudeLine(const MagnitudeLine *line,const ssize_t y,
  MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p,
    *magick_restrict q;

  Quantum
    *magick_restrict r;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(line->dx_view,0,y,line->dx_image->columns,1,
    line->exception);
  q=GetCacheViewVirtualPixels(line->dy_view,0,y,line->dx_image->columns,1,
    line->exception);
  r=GetCacheViewAuthenticPixels(line->magnitude_view,0,y,
    line->dx_image->columns,1,line->exception);
  if (IsBlendRowMissing(p,q,r) != MagickFalse)
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) line->dx_image->columns; x++)
  {
    BlendGradients
      gradients;

    gradients.dx_image=line->dx_image;
    gradients.dy_image=line->dy_image;
    SetMagnitudePixel(&gradients,p,q,r);
    p+=(ptrdiff_t) GetPixelChannels(line->dx_image);
    q+=(ptrdiff_t) GetPixelChannels(line->dy_image);
    r+=(ptrdiff_t) GetPixelChannels(line->magnitude_image);
  }
  SyncCompositeRow(line->magnitude_view,line->exception,status);
}

static Image *BlendMagnitudeImage(const Image *dx_image,const Image *dy_image,
  ExceptionInfo *exception)
{
  CacheView
    *dx_view,
    *dy_view,
    *magnitude_view;

  Image
    *magnitude_image;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    y;

  /*
    Generate the magnitude between two images.
  */
  magnitude_image=CloneImage(dx_image,0,0,MagickTrue,exception);
  if (magnitude_image == (Image *) NULL)
    return(magnitude_image);
  dx_view=AcquireVirtualCacheView(dx_image,exception);
  dy_view=AcquireVirtualCacheView(dy_image,exception);
  magnitude_view=AcquireAuthenticCacheView(magnitude_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(dx_image,magnitude_image,dx_image->rows,1)
#endif
  for (y=0; y < (ssize_t) dx_image->rows; y++)
  {
    const MagnitudeLine
      line = { dx_view, dy_view, magnitude_view, dx_image, dy_image,
        magnitude_image, exception };

    SetMagnitudeLine(&line,y,&status);
  }
  magnitude_view=DestroyCacheView(magnitude_view);
  dy_view=DestroyCacheView(dy_view);
  dx_view=DestroyCacheView(dx_view);
  if (status == MagickFalse)
    magnitude_image=DestroyImage(magnitude_image);
  return(magnitude_image);
}

static inline MagickBooleanType IsBlendRowsMissing(const Quantum *p,
  const Quantum *q,const Quantum *r,const Quantum *s,const Quantum *t)
{
  return(((p == (const Quantum *) NULL) || (q == (const Quantum *) NULL) ||
    (r == (const Quantum *) NULL) || (s == (const Quantum *) NULL) ||
    (t == (Quantum *) NULL)) ? MagickTrue : MagickFalse);
}

typedef struct _MaxMagnitudeInputs
{
  const Image
    *alpha_image,
    *beta_image,
    *dx_image,
    *dy_image;

  const Quantum
    *p,
    *q;
} MaxMagnitudeInputs;

static void SelectMaxMagnitudePixel(const MaxMagnitudeInputs *selection,
  const Quantum *magick_restrict r,const Quantum *magick_restrict s,
  Quantum *magick_restrict t)
{
  const Image
    *alpha_image = selection->alpha_image,
    *beta_image = selection->beta_image,
    *dx_image = selection->dx_image,
    *dy_image = selection->dy_image;

  const Quantum
    *magick_restrict p = selection->p,
    *magick_restrict q = selection->q;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(alpha_image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(alpha_image,i);
    PixelTrait traits = GetPixelChannelTraits(alpha_image,channel);
    PixelTrait beta_traits = GetPixelChannelTraits(beta_image,channel);
    if (IsBlendChannelSkipped(traits,beta_traits) != MagickFalse)
      continue;
    if (p[i] > GetPixelChannel(beta_image,channel,q))
      t[i]=GetPixelChannel(dx_image,channel,r);
    else
      t[i]=GetPixelChannel(dy_image,channel,s);
  }
}

typedef struct _MaxMagnitudeLine
{
  CacheView
    *alpha_view,
    *beta_view,
    *dx_view,
    *dy_view,
    *magnitude_view;

  const Image
    *alpha_image,
    *beta_image,
    *dx_image,
    *dy_image;

  Image
    *magnitude_image;

  ExceptionInfo
    *exception;
} MaxMagnitudeLine;

static void SelectMaxMagnitudeLine(const MaxMagnitudeLine *line,
  const ssize_t y,MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p,
    *magick_restrict q,
    *magick_restrict r,
    *magick_restrict s;

  Quantum
    *magick_restrict t;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(line->alpha_view,0,y,
    line->alpha_image->columns,1,line->exception);
  q=GetCacheViewVirtualPixels(line->beta_view,0,y,line->alpha_image->columns,1,
    line->exception);
  r=GetCacheViewVirtualPixels(line->dx_view,0,y,line->alpha_image->columns,1,
    line->exception);
  s=GetCacheViewVirtualPixels(line->dy_view,0,y,line->alpha_image->columns,1,
    line->exception);
  t=GetCacheViewAuthenticPixels(line->magnitude_view,0,y,
    line->alpha_image->columns,1,line->exception);
  if (IsBlendRowsMissing(p,q,r,s,t) != MagickFalse)
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) line->alpha_image->columns; x++)
  {
    MaxMagnitudeInputs
      selection;

    selection.alpha_image=line->alpha_image;
    selection.beta_image=line->beta_image;
    selection.dx_image=line->dx_image;
    selection.dy_image=line->dy_image;
    selection.p=p;
    selection.q=q;
    SelectMaxMagnitudePixel(&selection,r,s,t);
    p+=(ptrdiff_t) GetPixelChannels(line->alpha_image);
    q+=(ptrdiff_t) GetPixelChannels(line->beta_image);
    r+=(ptrdiff_t) GetPixelChannels(line->dx_image);
    s+=(ptrdiff_t) GetPixelChannels(line->dy_image);
    t+=(ptrdiff_t) GetPixelChannels(line->magnitude_image);
  }
  SyncCompositeRow(line->magnitude_view,line->exception,status);
}

static Image *BlendMaxMagnitudeImage(const BlendMagnitudePair *magnitudes,
  const Image *dx_image,const Image *dy_image,ExceptionInfo *exception)
{
  const Image
    *alpha_image = magnitudes->alpha_image,
    *beta_image = magnitudes->beta_image;

  CacheView
    *alpha_view,
    *beta_view,
    *dx_view,
    *dy_view,
    *magnitude_view;

  Image
    *magnitude_image;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    y;

  /*
    Select the larger of two magnitudes.
  */
  magnitude_image=CloneImage(alpha_image,0,0,MagickTrue,exception);
  if (magnitude_image == (Image *) NULL)
    return(magnitude_image);
  alpha_view=AcquireVirtualCacheView(alpha_image,exception);
  beta_view=AcquireVirtualCacheView(beta_image,exception);
  dx_view=AcquireVirtualCacheView(dx_image,exception);
  dy_view=AcquireVirtualCacheView(dy_image,exception);
  magnitude_view=AcquireAuthenticCacheView(magnitude_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(alpha_image,magnitude_image,alpha_image->rows,1)
#endif
  for (y=0; y < (ssize_t) alpha_image->rows; y++)
  {
    const MaxMagnitudeLine
      line = { alpha_view, beta_view, dx_view, dy_view, magnitude_view,
        alpha_image, beta_image, dx_image, dy_image, magnitude_image,
        exception };

    SelectMaxMagnitudeLine(&line,y,&status);
  }
  magnitude_view=DestroyCacheView(magnitude_view);
  dy_view=DestroyCacheView(dy_view);
  dx_view=DestroyCacheView(dx_view);
  beta_view=DestroyCacheView(beta_view);
  alpha_view=DestroyCacheView(alpha_view);
  if (status == MagickFalse)
    magnitude_image=DestroyImage(magnitude_image);
  return(magnitude_image);
}

typedef struct _SumBlendInputs
{
  const Image
    *alpha_image,
    *beta_image;

  Image
    *sum_image;

  double
    attenuate,
    sign;
} SumBlendInputs;

static void SumBlendRow(const SumBlendInputs *row,
  const Quantum *magick_restrict p,const Quantum *magick_restrict q,
  Quantum *magick_restrict r)
{
  const Image
    *alpha_image = row->alpha_image,
    *beta_image = row->beta_image;

  Image
    *sum_image = row->sum_image;

  const double
    attenuate = row->attenuate,
    sign = row->sign;

  ssize_t
    x;

  for (x=0; x < (ssize_t) alpha_image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(alpha_image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(alpha_image,i);
      PixelTrait traits = GetPixelChannelTraits(alpha_image,channel);
      PixelTrait beta_traits = GetPixelChannelTraits(beta_image,channel);
      if (IsBlendChannelSkipped(traits,beta_traits) != MagickFalse)
        continue;
      r[i]=ClampToQuantum(attenuate*((double) p[i]+sign*
        (double) GetPixelChannel(beta_image,channel,q)));
    }
    p+=(ptrdiff_t) GetPixelChannels(alpha_image);
    q+=(ptrdiff_t) GetPixelChannels(beta_image);
    r+=(ptrdiff_t) GetPixelChannels(sum_image);
  }
}

static Image *FinishBlendImage(Image *sum_image,
  const MagickBooleanType status)
{
  if (status == MagickFalse)
    sum_image=DestroyImage(sum_image);
  return(sum_image);
}

static Image *BlendSumImage(const Image *alpha_image,const Image *beta_image,
  const BlendSumWeights *weights,ExceptionInfo *exception)
{
  const double
    attenuate = weights->attenuate,
    sign = weights->sign;

  CacheView
    *alpha_view,
    *beta_view,
    *sum_view;

  Image
    *sum_image;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    y;

  /*
    Add or subtract and optionally attenuate two images.
  */
  sum_image=CloneImage(alpha_image,0,0,MagickTrue,exception);
  if (sum_image == (Image *) NULL)
    return(sum_image);
  alpha_view=AcquireVirtualCacheView(alpha_image,exception);
  beta_view=AcquireVirtualCacheView(beta_image,exception);
  sum_view=AcquireAuthenticCacheView(sum_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(alpha_image,sum_image,alpha_image->rows,1)
#endif
  for (y=0; y < (ssize_t) alpha_image->rows; y++)
  {
    const Quantum
      *magick_restrict p,
      *magick_restrict q;

    Quantum
      *magick_restrict r;

    SumBlendInputs
      row;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(alpha_view,0,y,alpha_image->columns,1,
      exception);
    q=GetCacheViewVirtualPixels(beta_view,0,y,alpha_image->columns,1,exception);
    r=GetCacheViewAuthenticPixels(sum_view,0,y,alpha_image->columns,1,
      exception);
    if (IsBlendRowMissing(p,q,r) != MagickFalse)
      {
        status=MagickFalse;
        continue;
      }
    row.alpha_image=alpha_image;
    row.beta_image=beta_image;
    row.sum_image=sum_image;
    row.attenuate=attenuate;
    row.sign=sign;
    SumBlendRow(&row,p,q,r);
    SyncCompositeRow(sum_view,exception,&status);
  }
  sum_view=DestroyCacheView(sum_view);
  beta_view=DestroyCacheView(beta_view);
  alpha_view=DestroyCacheView(alpha_view);
  return(FinishBlendImage(sum_image,status));
}

static Image *BlendDivergentImage(const Image *alpha_image,
  const Image *beta_image,ExceptionInfo *exception)
{
#define FreeDivergentResources() \
{ \
  if (dy_image != (Image *) NULL) \
    dy_image=DestroyImage(dy_image); \
  if (dx_image != (Image *) NULL) \
    dx_image=DestroyImage(dx_image); \
  if (magnitude_beta != (Image *) NULL) \
    magnitude_beta=DestroyImage(magnitude_beta); \
  if (dy_beta != (Image *) NULL) \
    dy_beta=DestroyImage(dy_beta); \
  if (dx_beta != (Image *) NULL) \
    dx_beta=DestroyImage(dx_beta); \
  if (magnitude_alpha != (Image *) NULL) \
    magnitude_alpha=DestroyImage(magnitude_alpha); \
  if (dy_alpha != (Image *) NULL) \
    dy_alpha=DestroyImage(dy_alpha); \
  if (dx_alpha != (Image *) NULL) \
    dx_alpha=DestroyImage(dx_alpha); \
}

  Image
    *divergent_image = (Image *) NULL,
    *dx_alpha = (Image *) NULL,
    *dx_beta = (Image *) NULL,
    *dx_divergent = (Image *) NULL,
    *dx_image = (Image *) NULL,
    *dy_alpha = (Image *) NULL,
    *dy_beta = (Image *) NULL,
    *dy_divergent = (Image *) NULL,
    *dy_image = (Image *) NULL,
    *magnitude_alpha = (Image *) NULL,
    *magnitude_beta = (Image *) NULL;

  BlendMagnitudePair magnitudes;

  const BlendSumWeights quarter_sum = { 0.25, 1.0 };

  /*
    Create X and Y gradient images for alpha image and the magnitude.
  */
  dx_alpha=BlendConvolveImage(alpha_image,"3x1:-0.5,0.0,0.5",exception);
  if (dx_alpha == (Image *) NULL)
    {
      FreeDivergentResources();
      return((Image *) NULL);
    }
  dy_alpha=BlendConvolveImage(alpha_image,"1x3:-0.5,0.0,0.5",exception);
  if (dy_alpha == (Image *) NULL)
    {
      FreeDivergentResources();
      return((Image *) NULL);
    }
  magnitude_alpha=BlendMagnitudeImage(dx_alpha,dy_alpha,exception);
  if (magnitude_alpha == (Image *) NULL)
    {
      FreeDivergentResources();
      return((Image *) NULL);
    }
  /*
    Create X and Y gradient images for beta and the magnitude.
  */
  dx_beta=BlendConvolveImage(beta_image,"3x1:-0.5,0.0,0.5",exception);
  if (dx_beta == (Image *) NULL)
    {
      FreeDivergentResources();
      return((Image *) NULL);
    }
  dy_beta=BlendConvolveImage(beta_image,"1x3:-0.5,0.0,0.5",exception);
  if (dy_beta == (Image *) NULL)
    {
      FreeDivergentResources();
      return((Image *) NULL);
    }
  magnitude_beta=BlendMagnitudeImage(dx_beta,dy_beta,exception);
  if (magnitude_beta == (Image *) NULL)
    {
      FreeDivergentResources();
      return((Image *) NULL);
    }
  /*
    Select alpha or beta gradient for larger of two magnitudes.
  */
  magnitudes.alpha_image=magnitude_alpha;
  magnitudes.beta_image=magnitude_beta;
  dx_image=BlendMaxMagnitudeImage(&magnitudes,dx_alpha,dx_beta,exception);
  if (dx_image == (Image *) NULL)
    {
      FreeDivergentResources();
      return((Image *) NULL);
    }
  dy_image=BlendMaxMagnitudeImage(&magnitudes,dy_alpha,dy_beta,exception);
  if (dy_image == (Image *) NULL)
    {
      FreeDivergentResources();
      return((Image *) NULL);
    }
  dx_beta=DestroyImage(dx_beta);
  dx_alpha=DestroyImage(dx_alpha);
  magnitude_beta=DestroyImage(magnitude_beta);
  magnitude_alpha=DestroyImage(magnitude_alpha);
  /*
    Create divergence of gradients dx and dy and divide by 4 as guide image.
  */
  dx_divergent=BlendConvolveImage(dx_image,"3x1:-0.5,0.0,0.5",exception);
  if (dx_divergent == (Image *) NULL)
    {
      FreeDivergentResources();
      return((Image *) NULL);
    }
  dy_divergent=BlendConvolveImage(dy_image,"1x3:-0.5,0.0,0.5",exception);
  if (dy_divergent == (Image *) NULL)
    {
      FreeDivergentResources();
      return((Image *) NULL);
    }
  divergent_image=BlendSumImage(dx_divergent,dy_divergent,&quarter_sum,
    exception);
  dy_divergent=DestroyImage(dy_divergent);
  dx_divergent=DestroyImage(dx_divergent);
  if (divergent_image == (Image *) NULL)
    {
      FreeDivergentResources();
      return((Image *) NULL);
    }
  FreeDivergentResources();
  return(divergent_image);
}

static inline MagickBooleanType IsBlendRowPairMissing(const Quantum *p,
  const Quantum *q)
{
  return(((p == (const Quantum *) NULL) || (q == (Quantum *) NULL)) ?
    MagickTrue : MagickFalse);
}

static void ClearMaskedAlphaRow(const Image *image,const Image *mask_image,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    Quantum
      alpha = GetPixelAlpha(mask_image,p);

    ssize_t
      i = GetPixelChannelOffset(image,AlphaPixelChannel);

    if (fabs((double) alpha) >= MagickEpsilon)
      q[i]=(Quantum) 0;
    p+=(ptrdiff_t) GetPixelChannels(mask_image);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType BlendMaskAlphaChannel(Image *image,
  const Image *mask_image,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *mask_view;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    y;

  /*
    Threshold the alpha channel.
  */
  if (SetImageAlpha(image,OpaqueAlpha,exception) == MagickFalse)
    return(MagickFalse);
  image_view=AcquireAuthenticCacheView(image,exception);
  mask_view=AcquireVirtualCacheView(mask_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(mask_view,0,y,image->columns,1,exception);
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (IsBlendRowPairMissing(p,q) != MagickFalse)
      {
        status=MagickFalse;
        continue;
      }
    ClearMaskedAlphaRow(image,mask_image,p,q);
    SyncCompositeRow(image_view,exception,&status);
  }
  mask_view=DestroyCacheView(mask_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

typedef struct _MeanFillInputs
{
  const Image
    *mask_image;

  Image
    *mean_image;
} MeanFillInputs;

static void FillMeanRow(const MeanFillInputs *row,const double *mean,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *mask_image = row->mask_image;

  Image
    *mean_image = row->mean_image;

  ssize_t
    x;

  for (x=0; x < (ssize_t) mean_image->columns; x++)
  {
    Quantum
      alpha = GetPixelAlpha(mask_image,p),
      mask = GetPixelReadMask(mask_image,p);

    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(mean_image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(mean_image,i);
      PixelTrait traits = GetPixelChannelTraits(mean_image,channel);
      if (traits == UndefinedPixelTrait)
        continue;
      if (mask <= (QuantumRange/2))
        q[i]=(Quantum) 0;
      else
        if (fabs((double) alpha) >= MagickEpsilon)
          q[i]=ClampToQuantum(mean[i]);
    }
    p+=(ptrdiff_t) GetPixelChannels(mask_image);
    q+=(ptrdiff_t) GetPixelChannels(mean_image);
  }
}

static void AccumulateMeanRow(const Image *image,
  const Quantum *magick_restrict p,double *mean)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,i);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if (traits == UndefinedPixelTrait)
        continue;
      mean[i]+=QuantumScale*(double) p[i];
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType SumImageChannels(const Image *image,double *mean,
  ExceptionInfo *exception)
{
  CacheView
    *alpha_view;

  ssize_t
    y;

  alpha_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    p=GetCacheViewVirtualPixels(alpha_view,0,y,image->columns,1,
      exception);
    if (p == (const Quantum *) NULL)
      break;
    AccumulateMeanRow(image,p,mean);
  }
  alpha_view=DestroyCacheView(alpha_view);
  if (y < (ssize_t) image->rows)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _MeanFillLine
{
  CacheView
    *mask_view,
    *mean_view;

  const Image
    *mask_image;

  Image
    *mean_image;

  const double
    *mean;

  ExceptionInfo
    *exception;
} MeanFillLine;

static void FillMeanLine(const MeanFillLine *line,const ssize_t y,
  MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  MeanFillInputs
    row;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(line->mask_view,0,y,line->mean_image->columns,1,
    line->exception);
  q=GetCacheViewAuthenticPixels(line->mean_view,0,y,line->mean_image->columns,
    1,line->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return;
    }
  row.mask_image=line->mask_image;
  row.mean_image=line->mean_image;
  FillMeanRow(&row,line->mean,p,q);
  SyncCompositeRow(line->mean_view,line->exception,status);
}

static Image *BlendMeanImage(Image *image,const Image *mask_image,
  ExceptionInfo *exception)
{
  CacheView
    *mask_view,
    *mean_view;

  double
    mean[MaxPixelChannels];

  Image
    *mean_image;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    j,
    y;

  /*
    Compute the mean of the image.
  */
  (void) memset(mean,0,MaxPixelChannels*sizeof(*mean));
  if (SumImageChannels(image,mean,exception) == MagickFalse)
    return((Image *) NULL);
  for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
    mean[j]=(double) QuantumRange*mean[j]/image->columns/
      image->rows;
  /*
    Replace any unmasked pixels with the mean pixel.
  */
  mean_image=CloneImage(image,0,0,MagickTrue,exception);
  if (mean_image == (Image *) NULL)
    return(mean_image);
  mask_view=AcquireVirtualCacheView(mask_image,exception);
  mean_view=AcquireAuthenticCacheView(mean_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(mask_image,mean_image,mean_image->rows,4)
#endif
  for (y=0; y < (ssize_t) mean_image->rows; y++)
  {
    const MeanFillLine
      line = { mask_view, mean_view, mask_image, mean_image, mean, exception };

    FillMeanLine(&line,y,&status);
  }
  mask_view=DestroyCacheView(mask_view);
  mean_view=DestroyCacheView(mean_view);
  if (status == MagickFalse)
    mean_image=DestroyImage(mean_image);
  return(mean_image);
}

typedef struct _ResidualPixels
{
  const Image
    *beta_image;

  const Quantum
    *p,
    *q;

  double
    Da,
    Sa;
} ResidualPixels;

static double GetResidualDistance(const ResidualPixels *residual,
  const PixelChannel channel,const ssize_t i)
{
  double
    distance;

  if (channel == AlphaPixelChannel)
    distance=QuantumScale*((double) residual->p[i]-(double) GetPixelChannel(
      residual->beta_image,channel,residual->q));
  else
    distance=QuantumScale*(residual->Sa*(double) residual->p[i]-residual->Da*
      (double) GetPixelChannel(residual->beta_image,channel,residual->q));
  return(distance);
}

typedef struct _ChannelResidualInputs
{
  const Image
    *alpha_image,
    *beta_image;

  const Quantum
    *p,
    *q;

  double
    Da,
    Sa;
} ChannelResidualInputs;

static void AddChannelResiduals(const ChannelResidualInputs *inputs,
  double *channel_residual)
{
  const Image
    *const alpha_image = inputs->alpha_image,
    *const beta_image = inputs->beta_image;

  const Quantum
    *magick_restrict p = inputs->p,
    *magick_restrict q = inputs->q;

  const double
    Da = inputs->Da,
    Sa = inputs->Sa;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(alpha_image); i++)
  {
    double
      distance;

    ResidualPixels
      residual;

    PixelChannel channel = GetPixelChannelChannel(alpha_image,i);
    PixelTrait traits = GetPixelChannelTraits(alpha_image,channel);
    PixelTrait beta_traits = GetPixelChannelTraits(beta_image,channel);
    if (IsBlendChannelSkipped(traits,beta_traits) != MagickFalse)
      continue;
    residual.beta_image=beta_image;
    residual.p=p;
    residual.q=q;
    residual.Da=Da;
    residual.Sa=Sa;
    distance=GetResidualDistance(&residual,channel,i);
    (*channel_residual)+=distance*distance;
  }
}

typedef struct _RMSEResidualRow
{
  const Image
    *alpha_image,
    *beta_image;

  const Quantum
    *p,
    *q;

  size_t
    columns;
} RMSEResidualRow;

static void AddRMSERowResidual(const RMSEResidualRow *row,double *area,
  double *channel_residual)
{
  const Image
    *const alpha_image = row->alpha_image,
    *const beta_image = row->beta_image;

  const Quantum
    *magick_restrict p = row->p,
    *magick_restrict q = row->q;

  const size_t
    columns = row->columns;

  ssize_t
    x;

  for (x=0; x < (ssize_t) columns; x++)
  {
    ChannelResidualInputs
      inputs;

    double
      Da,
      Sa;

    if ((GetPixelReadMask(alpha_image,p) <= (QuantumRange/2)) ||
        (GetPixelReadMask(beta_image,q) <= (QuantumRange/2)))
      {
        p+=(ptrdiff_t) GetPixelChannels(alpha_image);
        q+=(ptrdiff_t) GetPixelChannels(beta_image);
        continue;
      }
    Sa=QuantumScale*(double) GetPixelAlpha(alpha_image,p);
    Da=QuantumScale*(double) GetPixelAlpha(beta_image,q);
    inputs.alpha_image=alpha_image;
    inputs.beta_image=beta_image;
    inputs.p=p;
    inputs.q=q;
    inputs.Da=Da;
    inputs.Sa=Sa;
    AddChannelResiduals(&inputs,channel_residual);
    (*area)++;
    p+=(ptrdiff_t) GetPixelChannels(alpha_image);
    q+=(ptrdiff_t) GetPixelChannels(beta_image);
  }
}

static MagickBooleanType BlendRMSEResidual(const Image *alpha_image,
  const Image *beta_image,double *residual,ExceptionInfo *exception)
{
  CacheView
    *alpha_view,
    *beta_view;

  double
    area = 0.0,
    channel_residual = 0.0;

  MagickBooleanType
    status = MagickTrue;

  size_t
    columns = MagickMax(alpha_image->columns,beta_image->columns),
    rows = MagickMax(alpha_image->rows,beta_image->rows);

  ssize_t
    y;

  alpha_view=AcquireVirtualCacheView(alpha_image,exception);
  beta_view=AcquireVirtualCacheView(beta_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    reduction(+:area) reduction(+:channel_residual) \
    magick_number_threads(alpha_image,alpha_image,rows,1)
#endif
  for (y=0; y < (ssize_t) rows; y++)
  {
    const Quantum
      *magick_restrict p,
      *magick_restrict q;

    RMSEResidualRow
      row;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(alpha_view,0,y,columns,1,exception);
    q=GetCacheViewVirtualPixels(beta_view,0,y,columns,1,exception);
    if ((p == (const Quantum *) NULL) || (q == (const Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    channel_residual=0.0;
    row.alpha_image=alpha_image;
    row.beta_image=beta_image;
    row.p=p;
    row.q=q;
    row.columns=columns;
    AddRMSERowResidual(&row,&area,&channel_residual);
  }
  beta_view=DestroyCacheView(beta_view);
  alpha_view=DestroyCacheView(alpha_view);
  area=MagickSafeReciprocal(area);
  *residual=sqrt(area*channel_residual/(double) GetImageChannels(alpha_image));
  return(status);
}

static inline MagickBooleanType IsOutsideSourceColumn(const Quantum *pixels,
  const ssize_t x,const ssize_t x_offset,const Image *source_image)
{
  return(((pixels == (Quantum *) NULL) || (x < x_offset) ||
    ((x-x_offset) >= (ssize_t) source_image->columns)) ? MagickTrue :
    MagickFalse);
}

typedef struct _OverChannelInputs
{
  const Image
    *source_image;

  const Quantum
    *p;

  PixelChannel
    channel;

  PixelTrait
    traits;

  MagickRealType
    alpha,
    Da,
    Sa;

  MagickBooleanType
    clamp;
} OverChannelInputs;

static void CompositeOverChannel(const OverChannelInputs *over,
  Quantum *magick_restrict q,const ssize_t i)
{
  double
    gamma;

  MagickRealType
    Dc,
    Dca,
    pixel,
    Sc,
    Sca;

  if (over->channel == AlphaPixelChannel)
    {
      /*
        Set alpha channel.
      */
      pixel=(double) QuantumRange*over->alpha;
      q[i]=over->clamp != MagickFalse ? ClampPixel(pixel) :
        ClampToQuantum(pixel);
      return;
    }
  /*
    Sc: source color.
    Dc: canvas color.
  */
  Sc=(MagickRealType) GetPixelChannel(over->source_image,over->channel,
    over->p);
  Dc=(MagickRealType) q[i];
  if ((over->traits & CopyPixelTrait) != 0)
    {
      /*
        Copy channel.
      */
      q[i]=ClampToQuantum(Sc);
      return;
    }
  /*
    Porter-Duff compositions:
      Sca: source normalized color multiplied by alpha.
      Dca: normalized canvas color multiplied by alpha.
  */
  Sca=QuantumScale*over->Sa*Sc;
  Dca=QuantumScale*over->Da*Dc;
  gamma=MagickSafeReciprocal(over->alpha);
  pixel=(double) QuantumRange*gamma*(Sca+Dca*(1.0-over->Sa));
  q[i]=over->clamp != MagickFalse ? ClampPixel(pixel) :
    ClampToQuantum(pixel);
}

typedef struct _OverPixelInputs
{
  Image
    *image;

  const Image
    *source_image;

  const Quantum
    *p;

  MagickRealType
    alpha,
    Da,
    Sa;

  MagickBooleanType
    clamp;
} OverPixelInputs;

static void CompositeOverPixelChannels(const OverPixelInputs *authentic,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(authentic->image); i++)
  {
    OverChannelInputs
      over;

    PixelChannel channel = GetPixelChannelChannel(authentic->image,i);
    PixelTrait traits = GetPixelChannelTraits(authentic->image,channel);
    PixelTrait source_traits=GetPixelChannelTraits(authentic->source_image,
      channel);
    if (traits == UndefinedPixelTrait)
      continue;
    if ((source_traits == UndefinedPixelTrait) &&
        (channel != AlphaPixelChannel))
        continue;
    over.source_image=authentic->source_image;
    over.p=authentic->p;
    over.channel=channel;
    over.traits=traits;
    over.alpha=authentic->alpha;
    over.Da=authentic->Da;
    over.Sa=authentic->Sa;
    over.clamp=authentic->clamp;
    CompositeOverChannel(&over,q,i);
  }
}

typedef struct _VirtualOverInputs
{
  Image
    *image;

  const Image
    *source_image;

  ssize_t
    x,
    x_offset,
    y,
    y_offset;

  MagickBooleanType
    clamp;

  ExceptionInfo
    *exception;
} VirtualOverInputs;

static void CompositeVirtualOverPixel(const VirtualOverInputs *virtual_pixel,
  Quantum *magick_restrict q)
{
  Quantum
    source[MaxPixelChannels];

  ssize_t
    i;

  (void) GetOneVirtualPixel(virtual_pixel->source_image,
    CastDoubleToSsizeT((double) virtual_pixel->x-virtual_pixel->x_offset),
    CastDoubleToSsizeT((double) virtual_pixel->y-virtual_pixel->y_offset),
    source,virtual_pixel->exception);
  for (i=0; i < (ssize_t) GetPixelChannels(virtual_pixel->image); i++)
  {
    MagickRealType
      pixel;

    PixelChannel channel = GetPixelChannelChannel(virtual_pixel->image,i);
    PixelTrait traits = GetPixelChannelTraits(virtual_pixel->image,channel);
    PixelTrait source_traits=GetPixelChannelTraits(virtual_pixel->source_image,
      channel);
    if ((traits == UndefinedPixelTrait) ||
        (source_traits == UndefinedPixelTrait))
      continue;
    if (channel == AlphaPixelChannel)
      pixel=(MagickRealType) TransparentAlpha;
    else
      pixel=(MagickRealType) q[i];
    q[i]=virtual_pixel->clamp != MagickFalse ? ClampPixel(pixel) :
      ClampToQuantum(pixel);
  }
}

static const Quantum *AdvanceOverSourcePixel(const Image *source_image,
  const Quantum *pixels,const Quantum *p)
{
  size_t
    channels;

  p+=(ptrdiff_t) GetPixelChannels(source_image);
  channels=GetPixelChannels(source_image);
  if (p >= (pixels+channels*source_image->columns))
    p=pixels;
  return(p);
}

typedef struct _OverRowInputs
{
  Image
    *image;

  const Image
    *source_image;

  const Quantum
    *pixels;

  MagickBooleanType
    clip_to_self;

  ssize_t
    x_offset,
    y_offset,
    y;

  MagickBooleanType
    clamp;

  ExceptionInfo
    *exception;
} OverRowInputs;

static void CompositeOverRow(const OverRowInputs *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) row->image->columns; x++)
  {
    MagickRealType
      alpha,
      Da,
      Sa;

    OverPixelInputs
      authentic;

    if (row->clip_to_self != MagickFalse)
      {
        if (x < row->x_offset)
          {
            q+=(ptrdiff_t) GetPixelChannels(row->image);
            continue;
          }
        if ((x-row->x_offset) >= (ssize_t) row->source_image->columns)
          break;
      }
    if (IsOutsideSourceColumn(row->pixels,x,row->x_offset,
          row->source_image) != MagickFalse)
      {
        VirtualOverInputs
          virtual_pixel;

        /*
          Virtual composite:
            Sc: source color.
            Dc: canvas color.
        */
        virtual_pixel.image=row->image;
        virtual_pixel.source_image=row->source_image;
        virtual_pixel.x=x;
        virtual_pixel.x_offset=row->x_offset;
        virtual_pixel.y=row->y;
        virtual_pixel.y_offset=row->y_offset;
        virtual_pixel.clamp=row->clamp;
        virtual_pixel.exception=row->exception;
        CompositeVirtualOverPixel(&virtual_pixel,q);
        q+=(ptrdiff_t) GetPixelChannels(row->image);
        continue;
      }
    /*
      Authentic composite:
        Sa:  normalized source alpha.
        Da:  normalized canvas alpha.
    */
    Sa=QuantumScale*(double) GetPixelAlpha(row->source_image,p);
    Da=QuantumScale*(double) GetPixelAlpha(row->image,q);
    alpha=Sa+Da-Sa*Da;
    authentic.image=row->image;
    authentic.source_image=row->source_image;
    authentic.p=p;
    authentic.alpha=alpha;
    authentic.Da=Da;
    authentic.Sa=Sa;
    authentic.clamp=row->clamp;
    CompositeOverPixelChannels(&authentic,q);
    p=AdvanceOverSourcePixel(row->source_image,row->pixels,p);
    q+=(ptrdiff_t) GetPixelChannels(row->image);
  }
}

static void SetCompositeProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

typedef struct _OverSourceRow
{
  CacheView
    *source_view;

  const Image
    *source_image;

  MagickBooleanType
    clip_to_self;

  ssize_t
    x_offset,
    y_offset,
    y;

  ExceptionInfo
    *exception;
} OverSourceRow;

static MagickBooleanType SkipOverRow(const OverSourceRow *source,
  const MagickBooleanType *status)
{
  if ((*status) == MagickFalse)
    return(MagickFalse);
  if (source->clip_to_self != MagickFalse)
    {
      if (source->y < source->y_offset)
        return(MagickFalse);
      if ((source->y-source->y_offset) >= (ssize_t) source->source_image->rows)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType FetchOverSourceRow(const OverSourceRow *source,
  const Quantum **pixels,const Quantum *magick_restrict *p,
  MagickBooleanType *status)
{
  /*
    If pixels is NULL, y is outside overlay region.
  */
  (*pixels)=(Quantum *) NULL;
  (*p)=(Quantum *) NULL;
  if ((source->y >= source->y_offset) &&
      ((source->y-source->y_offset) < (ssize_t) source->source_image->rows))
    {
      (*p)=GetCacheViewVirtualPixels(source->source_view,0,
        CastDoubleToSsizeT((double) source->y-source->y_offset),
        source->source_image->columns,1,source->exception);
      if ((*p) == (const Quantum *) NULL)
        {
          (*status)=MagickFalse;
          return(MagickFalse);
        }
      (*pixels)=(*p);
      if (source->x_offset < 0)
        (*p)-=(ptrdiff_t) CastDoubleToSsizeT((double) source->x_offset*
          GetPixelChannels(source->source_image));
    }
  return(MagickTrue);
}

static MagickBooleanType GetCompositeClamp(const Image *image)
{
  const char
    *value;

  MagickBooleanType
    clamp;

  clamp=MagickTrue;
  value=GetImageArtifact(image,"compose:clamp");
  if (value != (const char *) NULL)
    clamp=IsStringTrue(value);
  return(clamp);
}

typedef struct _OverRowHead
{
  OverSourceRow
    source;

  Image
    *image;

  CacheView
    *image_view;

  MagickBooleanType
    clamp;
} OverRowHead;

static MagickBooleanType CompositeOverRowHead(const OverRowHead *head,
  MagickBooleanType *status)
{
  const OverSourceRow
    source = head->source;

  Image
    *const image = head->image;

  CacheView
    *const image_view = head->image_view;

  const MagickBooleanType
    clamp = head->clamp;

  const Image
    *const source_image = source.source_image;

  const MagickBooleanType
    clip_to_self = source.clip_to_self;

  const ssize_t
    x_offset = source.x_offset,
    y_offset = source.y_offset,
    y = source.y;

  ExceptionInfo
    *const exception = source.exception;

  const Quantum
    *pixels;

  PixelInfo
    canvas_pixel,
    source_pixel;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  OverRowInputs
    row;

  if (SkipOverRow(&source,status) == MagickFalse)
    return(MagickFalse);
  if (FetchOverSourceRow(&source,&pixels,&p,status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  GetPixelInfo(image,&canvas_pixel);
  GetPixelInfo(source_image,&source_pixel);
  row.image=image;
  row.source_image=source_image;
  row.pixels=pixels;
  row.clip_to_self=clip_to_self;
  row.x_offset=x_offset;
  row.y_offset=y_offset;
  row.y=y;
  row.clamp=clamp;
  row.exception=exception;
  CompositeOverRow(&row,p,q);
  SyncCompositeRow(image_view,exception,status);
  return(MagickTrue);
}

static MagickBooleanType CompositeOverImage(Image *image,
  const Image *source_image,const CompositePlacement *placement,
  ExceptionInfo *exception)
{
#define CompositeImageTag  "Composite/Image"

  const MagickBooleanType
    clip_to_self = placement->clip_to_self;

  const ssize_t
    x_offset = placement->x_offset,
    y_offset = placement->y_offset;

  CacheView
    *image_view,
    *source_view;

  MagickBooleanType
    clamp,
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Composite image.
  */
  status=MagickTrue;
  progress=0;
  clamp=GetCompositeClamp(image);
  status=MagickTrue;
  progress=0;
  source_view=AcquireVirtualCacheView(source_image,exception);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(source_image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const OverSourceRow
      source = { source_view, source_image, clip_to_self, x_offset, y_offset,
        y, exception };

    const OverRowHead
      head = { source, image, image_view, clamp };

    if (CompositeOverRowHead(&head,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetCompositeProgress(image,CompositeImageTag,progress,&status);
      }
  }
  source_view=DestroyCacheView(source_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

static void ReportBlendResidual(const MagickBooleanType verbose,
  const ssize_t i,const double residual)
{
  if (verbose != MagickFalse)
    (void) FormatLocaleFile(stderr,"  %g: %g\n",(double) i,(double)
      residual);
}

typedef struct _BlendRelaxation
{
  Image
    *anchor_image,
    *divergent_image,
    **relax_image,
    **residual_image;

  Image
    **const *residual_failure,
    **const *kernel_failure;
} BlendRelaxation;

static void ReleaseBlendImages(Image **const *images)
{
  ssize_t
    k;

  for (k=0; images[k] != (Image **) NULL; k++)
    *images[k]=DestroyImage(*images[k]);
}

static void ReportBlendIteration(const MagickBooleanType verbose,
  const ssize_t i,const size_t tick,const double residual)
{
  if ((verbose != MagickFalse) && ((i % MagickMax(tick,1)) == 0))
    (void) FormatLocaleFile(stderr,"  %g: %g\n",(double) i,(double) residual);
}

static void ReportBlendTitle(const BlendRelaxation *relaxation,
  const MagickBooleanType verbose)
{
  if (verbose == MagickFalse)
    return;
  if (relaxation->divergent_image != (Image *) NULL)
    (void) FormatLocaleFile(stderr,"saliency blending:\n");
  else
    (void) FormatLocaleFile(stderr,"seamless blending:\n");
}

static Image *CloneBlendResidual(Image *residual_image,
  const Image *relax_image,ExceptionInfo *exception)
{
  if (residual_image != (Image *) NULL)
    residual_image=DestroyImage(residual_image);
  return(CloneImage(relax_image,0,0,MagickTrue,exception));
}

static MagickBooleanType RelaxBlendStep(const BlendRelaxation *relaxation,
  const KernelInfo *kernel_info,double *residual,ExceptionInfo *exception)
{
  Image
    *convolve_image,
    *sum_image;

  const BlendSumWeights difference = { 1.0, -1.0 };
  const CompositePlacement origin = { MagickTrue, 0, 0 };

  convolve_image=ConvolveImage(*relaxation->relax_image,kernel_info,exception);
  if (convolve_image == (Image *) NULL)
    return(MagickFalse);
  *relaxation->relax_image=DestroyImage(*relaxation->relax_image);
  *relaxation->relax_image=convolve_image;
  if (relaxation->divergent_image != (Image *) NULL)
    {
      sum_image=BlendSumImage(*relaxation->relax_image,
        relaxation->divergent_image,&difference,exception);
      if (sum_image == (Image *) NULL)
        return(MagickFalse);
      *relaxation->relax_image=DestroyImage(*relaxation->relax_image);
      *relaxation->relax_image=sum_image;
    }
  if (CompositeOverImage(*relaxation->relax_image,relaxation->anchor_image,
      &origin,exception) == MagickFalse)
    return(MagickFalse);
  return(BlendRMSEResidual(*relaxation->relax_image,
    *relaxation->residual_image,residual,exception));
}

static MagickBooleanType RelaxBlendImage(Image *image,
  const BlendRelaxation *relaxation,const BlendRequest *request,
  ExceptionInfo *exception)
{
  KernelInfo
    *kernel_info;

  MagickBooleanType
    verbose;

  ssize_t
    i;

  *relaxation->residual_image=CloneImage(*relaxation->relax_image,0,0,
    MagickTrue,exception);
  if (*relaxation->residual_image == (Image *) NULL)
    {
      ReleaseBlendImages(relaxation->residual_failure);
      return(MagickFalse);
    }
  /*
    Convolve relaxed image and blur area of interest.
  */
  kernel_info=AcquireKernelInfo("3x3:0,0.25,0,0.25,0,0.25,0,0.25,0",exception);
  if (kernel_info == (KernelInfo *) NULL)
    {
      ReleaseBlendImages(relaxation->kernel_failure);
      return(MagickFalse);
    }
  verbose=IsStringTrue(GetImageArtifact(image,"verbose"));
  ReportBlendTitle(relaxation,verbose);
  for (i=0; i < (ssize_t) request->iterations; i++)
  {
    double
      residual = 1.0;

    if (RelaxBlendStep(relaxation,kernel_info,&residual,
        exception) == MagickFalse)
      break;
    ReportBlendIteration(verbose,i,request->tick,residual);
    if (residual < request->residual_threshold)
      {
        ReportBlendResidual(verbose,i,residual);
        break;
      }
    *relaxation->residual_image=CloneBlendResidual(
      *relaxation->residual_image,*relaxation->relax_image,exception);
    if (*relaxation->residual_image == (Image *) NULL)
      break;
  }
  kernel_info=DestroyKernelInfo(kernel_info);
  return(MagickTrue);
}

static MagickBooleanType SaliencyBlendImage(Image *image,
  const Image *source_image,const BlendRequest *request,
  ExceptionInfo *exception)
{
  Image
    *crop_image,
    *divergent_image,
    *relax_image,
    *residual_image = (Image *) NULL;

  MagickBooleanType
    status = MagickTrue;

  RectangleInfo
    crop_info = {
      source_image->columns,
      source_image->rows,
      request->x_offset,
      request->y_offset
    };

  const CompositePlacement placement = { MagickTrue, request->x_offset, request->y_offset };

  Image
    **const residual_failure[] = { &crop_image, &relax_image, NULL },
    **const kernel_failure[] = { &crop_image, &residual_image, &relax_image,
      NULL };

  /*
    Saliency blend composite operator.
  */
  crop_image=CropImage(image,&crop_info,exception);
  if (crop_image == (Image *) NULL)
    return(MagickFalse);
  DisableCompositeClampUnlessSpecified(crop_image);
  divergent_image=BlendDivergentImage(crop_image,source_image,exception);
  if (divergent_image == (Image *) NULL)
    {
      crop_image=DestroyImage(crop_image);
      return(MagickFalse);
    }
  (void) ResetImagePage(crop_image,"0x0+0+0");
  relax_image=BlendMeanImage(crop_image,source_image,exception);
  if (relax_image == (Image *) NULL)
    {
      crop_image=DestroyImage(crop_image);
      divergent_image=DestroyImage(divergent_image);
      return(MagickFalse);
    }
  status=BlendMaskAlphaChannel(crop_image,source_image,exception);
  if (status == MagickFalse)
    {
      crop_image=DestroyImage(crop_image);
      divergent_image=DestroyImage(divergent_image);
      return(MagickFalse);
    }
  {
    const BlendRelaxation
      relaxation = { crop_image, divergent_image, &relax_image,
        &residual_image, residual_failure, kernel_failure };

    if (RelaxBlendImage(image,&relaxation,request,exception) == MagickFalse)
      return(MagickFalse);
  }
  crop_image=DestroyImage(crop_image);
  divergent_image=DestroyImage(divergent_image);
  residual_image=DestroyImage(residual_image);
  /*
    Composite relaxed over the background image.
  */
  status=CompositeOverImage(image,relax_image,&placement,exception);
  relax_image=DestroyImage(relax_image);
  return(status);
}

static Image *BlendSeamlessMeanImage(const Image *image,
  const Image *source_image,const BlendRequest *request,
  ExceptionInfo *exception)
{
  Image
    *crop_image,
    *mean_image,
    *sum_image;

  RectangleInfo
    crop_info = {
      source_image->columns,
      source_image->rows,
      request->x_offset,
      request->y_offset
    };

  const BlendSumWeights difference = { 1.0, -1.0 };

  crop_image=CropImage(image,&crop_info,exception);
  if (crop_image == (Image *) NULL)
    return((Image *) NULL);
  DisableCompositeClampUnlessSpecified(crop_image);
  (void) ResetImagePage(crop_image,"0x0+0+0");
  sum_image=BlendSumImage(crop_image,source_image,&difference,exception);
  crop_image=DestroyImage(crop_image);
  if (sum_image == (Image *) NULL)
    return((Image *) NULL);
  mean_image=BlendMeanImage(sum_image,source_image,exception);
  sum_image=DestroyImage(sum_image);
  return(mean_image);
}

static MagickBooleanType SeamlessBlendImage(Image *image,
  const Image *source_image,const BlendRequest *request,
  ExceptionInfo *exception)
{
  Image
    *foreground_image,
    *mean_image,
    *relax_image,
    *residual_image;

  MagickBooleanType
    status = MagickTrue;

  const BlendSumWeights sum = { 1.0, 1.0 };
  const CompositePlacement placement = { MagickTrue, request->x_offset, request->y_offset };

  Image
    **const residual_failure[] = { &relax_image, &mean_image, NULL },
    **const kernel_failure[] = { &residual_image, &relax_image, &mean_image,
      NULL };

  /*
    Seamless blend composite operator.
  */
  mean_image=BlendSeamlessMeanImage(image,source_image,request,exception);
  if (mean_image == (Image *) NULL)
    return(MagickFalse);
  relax_image=CloneImage(mean_image,0,0,MagickTrue,exception);
  if (relax_image == (Image *) NULL)
    {
      mean_image=DestroyImage(mean_image);
      return(MagickFalse);
    }
  status=BlendMaskAlphaChannel(mean_image,source_image,exception);
  if (status == MagickFalse)
    {
      relax_image=DestroyImage(relax_image);
      mean_image=DestroyImage(mean_image);
      return(MagickFalse);
    }
  {
    const BlendRelaxation
      relaxation = { mean_image, (Image *) NULL, &relax_image,
        &residual_image, residual_failure, kernel_failure };

    if (RelaxBlendImage(image,&relaxation,request,exception) == MagickFalse)
      return(MagickFalse);
  }
  mean_image=DestroyImage(mean_image);
  residual_image=DestroyImage(residual_image);
  /*
    Composite the foreground image over the background image.
  */
  foreground_image=BlendSumImage(source_image,relax_image,&sum,exception);
  relax_image=DestroyImage(relax_image);
  if (foreground_image == (Image *) NULL)
    return(MagickFalse);
  (void) SetImageMask(foreground_image,ReadPixelMask,(const Image *) NULL,
    exception);
  status=CompositeOverImage(image,foreground_image,&placement,exception);
  foreground_image=DestroyImage(foreground_image);
  return(status);
}

typedef struct _VirtualCompositeInputs
{
  const Image
    *image,
    *source_image;

  CompositeOperator
    compose;

  MagickRealType
    canvas_dissolve;

  MagickBooleanType
    clamp,
    canvas_pixel;
} VirtualCompositeInputs;

static void SetVirtualDissolvePixel(const VirtualCompositeInputs *composite,
  const PixelChannel channel,const Quantum *source,MagickRealType *pixel)
{
  switch (composite->compose)
  {
    case BlendCompositeOp:
    case DissolveCompositeOp:
    {
      if (channel == AlphaPixelChannel)
        (*pixel)=composite->canvas_dissolve*(double) GetPixelAlpha(
          composite->source_image,source);
      else
        (*pixel)=(MagickRealType) source[channel];
      break;
    }
    default:
    {
      (*pixel)=(MagickRealType) source[channel];
      break;
    }
  }
}

static void SetVirtualSourcePixel(const VirtualCompositeInputs *composite,
  const PixelChannel channel,const Quantum *source,MagickRealType *pixel)
{
  switch (composite->compose)
  {
    case ClearCompositeOp:
    case CopyCompositeOp:
    case ReplaceCompositeOp:
    {
      if (channel == AlphaPixelChannel)
        (*pixel)=(MagickRealType) TransparentAlpha;
      else
        (*pixel)=0.0;
      break;
    }
    default:
    {
      SetVirtualDissolvePixel(composite,channel,source,&(*pixel));
      break;
    }
  }
}

static MagickRealType GetVirtualCanvasPixel(const PixelChannel channel,
  const Quantum *magick_restrict q,const ssize_t i)
{
  MagickRealType
    pixel;

  if (channel == AlphaPixelChannel)
    pixel=(MagickRealType) TransparentAlpha;
  else
    pixel=(MagickRealType) q[i];
  return(pixel);
}

static const CompositeOperator
  CanvasPixelOperators[] =
  {
    AlphaCompositeOp,
    ChangeMaskCompositeOp,
    CopyAlphaCompositeOp,
    DstAtopCompositeOp,
    DstInCompositeOp,
    InCompositeOp,
    OutCompositeOp,
    SrcInCompositeOp,
    SrcOutCompositeOp
  };

static inline void SetVirtualCompositeChannels(
  const VirtualCompositeInputs *composite,const Quantum *source,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(composite->image); i++)
  {
    MagickRealType
      pixel = 0.0;

    PixelChannel channel = GetPixelChannelChannel(composite->image,i);
    PixelTrait traits = GetPixelChannelTraits(composite->image,channel);
    PixelTrait source_traits = GetPixelChannelTraits(composite->source_image,
      channel);
    if ((traits == UndefinedPixelTrait) ||
        (source_traits == UndefinedPixelTrait))
      continue;
    if (composite->canvas_pixel != MagickFalse)
      pixel=GetVirtualCanvasPixel(channel,q,i);
    else
      SetVirtualSourcePixel(composite,channel,source,&pixel);
    q[i]=composite->clamp != MagickFalse ? ClampPixel(pixel) :
      ClampToQuantum(pixel);
  }
}

static inline void CopySourceChannels(const Image *source_image,
  const Image *image,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(source_image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(source_image,i);
    PixelTrait source_traits = GetPixelChannelTraits(source_image,
      channel);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((source_traits == UndefinedPixelTrait) ||
        (traits == UndefinedPixelTrait))
      continue;
    SetPixelChannel(image,channel,p[i],q);
  }
}

static IlluminantType ParseComposeIlluminant(const char *artifact)
{
  IlluminantType
    illuminant;

  ssize_t
    illuminant_type;

  illuminant_type=ParseCommandOption(MagickIlluminantOptions,MagickFalse,
    artifact);
  if (illuminant_type < 0)
    illuminant=UndefinedIlluminant;
  else
    illuminant=(IlluminantType) illuminant_type;
  return(illuminant);
}

static MagickRealType GetColorDodgeBlend(const MagickRealType S,
  const MagickRealType D)
{
  MagickRealType
    blend;

  if (S >= 1.0)
    blend=1.0;
  else
    if (D <= 0.0)
      blend=0.0;
    else
      {
        if ((1.0-S) <= 0.0)
          blend=1.0;
        else
          blend=MagickMin(1.0,D/(1.0-S));
      }
  return(blend);
}

static MagickRealType GetSoftBurnBlend(const MagickRealType Sca,
  const MagickRealType Dca)
{
  MagickRealType
    blend;

  if (RoundToUnity(Dca) <= 0.0)
    blend = 0.0;
  else
    if (RoundToUnity(Sca) >= 1.0)
      blend = 1.0;
    else
      blend=1.0-MagickMin(1.0,(1.0-RoundToUnity(Dca))/
        RoundToUnity(Sca));
  return(blend);
}

static MagickRealType GetSoftDodgeBlend(const MagickRealType Sca,
  const MagickRealType Dca)
{
  MagickRealType
    blend;

  if (RoundToUnity(Sca) <= 0.0)
    blend=RoundToUnity(Dca);
  else
    if (RoundToUnity(Dca) >= 1.0)
      blend=1.0;
    else
      blend=MagickMin(1.0,RoundToUnity(Dca)/(1.0-RoundToUnity(Sca)));
  return(blend);
}

static MagickRealType GetVividLightBlend(const MagickRealType Sa,
  const MagickRealType Sca,const MagickRealType Da,const MagickRealType Dca)
{
  MagickRealType
    blend;

  if (RoundToUnity(Sca/Sa) <= 0.0)
    blend=0.0;
  else
    if (RoundToUnity(Sca/Sa) < 0.5)
      blend=1.0-(1.0-RoundToUnity(Dca/Da))/(2.0*RoundToUnity(Sca/Sa));
  else
    if (RoundToUnity(Sca/Sa) < 1.0)
      blend=RoundToUnity(Dca/Da)/(2.0*(1.0-RoundToUnity(Sca/Sa)));
    else
      blend=1.0;
  return(blend);
}

static MagickRealType GetPegtopLightBlend(const MagickRealType Sca,
  const MagickRealType Da,const MagickRealType Dca)
{
  MagickRealType
    blend;

  if (RoundToUnity(Sca) <= 0.5)
    blend=RoundToUnity(Dca/Da)-(1.0-2.0*RoundToUnity(Sca))*
      RoundToUnity(Dca/Da)*(1.0-RoundToUnity(Dca/Da));
  else
    {
      if (RoundToUnity(Dca/Da) <= 0.25)
        blend=((16.0*RoundToUnity(Dca/Da)-12.0)*RoundToUnity(Dca/Da)+
          4.0)*RoundToUnity(Dca/Da);
      else
        blend=sqrt(RoundToUnity(Dca/Da));
      blend=RoundToUnity(Dca/Da)+(2.0*RoundToUnity(Sca)-1.0)*
        (blend-RoundToUnity(Dca/Da));
    }
  return(blend);
}

static ColorspaceType ParseComposeColorspace(const char *artifact)
{
  ColorspaceType
    colorspace;

  ssize_t
    colorspace_type;

  colorspace_type=ParseCommandOption(MagickColorspaceOptions,MagickFalse,
    artifact);
  if (colorspace_type < 0)
    colorspace=UndefinedColorspace;
  else
    colorspace=(ColorspaceType) colorspace_type;
  return(colorspace);
}

static inline double GetCompositeGamma(const CompositeOperator compose,
  const MagickRealType alpha)
{
  double
    gamma;

  switch (compose)
  {
    case DarkenCompositeOp:
    case LightenCompositeOp:
    case ModulusSubtractCompositeOp:
    {
      gamma=MagickSafeReciprocal(1.0-alpha);
      break;
    }
    default:
    {
      gamma=MagickSafeReciprocal(alpha);
      break;
    }
  }
  return(gamma);
}

static MagickRealType GetReflectBlend(const MagickRealType Sc,
  const MagickRealType Dc)
{
  MagickRealType
    blend;

  if (Dc < 1.0)
    blend=(Sc*Sc)/(1.0-Dc);
  else
    blend=1.0;
  return(blend);
}

static MagickRealType GetHueAlpha(const MagickRealType Sa,
  const MagickRealType Da,MagickRealType pixel)
{
  if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
    {
      pixel=(double) QuantumRange*Da;
      return(pixel);
    }
  if (fabs((double) QuantumRange*Da-(double) TransparentAlpha) < MagickEpsilon)
    {
      pixel=(double) QuantumRange*Sa;
      return(pixel);
    }
  if (Sa < Da)
    {
      pixel=(double) QuantumRange*Da;
      return(pixel);
    }
  pixel=(double) QuantumRange*Sa;
  return(pixel);
}

static MagickRealType GetCopyAlpha(const Image *source_image,
  const MagickRealType Si,const MagickRealType Sa)
{
  MagickRealType
    pixel;

  if (source_image->alpha_trait == UndefinedPixelTrait)
    pixel=Si;
  else
    pixel=(double) QuantumRange*Sa;
  return(pixel);
}

static MagickRealType GetFreezeAlpha(const MagickRealType Sa,
  const MagickRealType Da)
{
  MagickRealType
    pixel;

  pixel=(double) QuantumRange*(1.0-(1.0-Sa)*(1.0-Sa)*
    MagickSafeReciprocal(Da));
  if (pixel < 0.0)
    pixel=0.0;
  return(pixel);
}

static MagickRealType GetReflectAlpha(const MagickRealType Sa,
  const MagickRealType Da)
{
  MagickRealType
    pixel;

  pixel=(double) QuantumRange*(Sa*Sa*
    MagickSafeReciprocal(1.0-Da));
  if (pixel > (double) QuantumRange)
    pixel=(double) QuantumRange;
  return(pixel);
}

static void SetDissolveFactors(const GeometryInfo geometry_info,
  const MagickStatusType flags,MagickRealType *source_dissolve,
  MagickRealType *canvas_dissolve)
{
  *source_dissolve=geometry_info.rho/100.0;
  *canvas_dissolve=1.0;
  if ((*source_dissolve-MagickEpsilon) < 0.0)
    *source_dissolve=0.0;
  if ((*source_dissolve+MagickEpsilon) > 1.0)
    {
      *canvas_dissolve=2.0-*source_dissolve;
      *source_dissolve=1.0;
    }
  if ((flags & SigmaValue) != 0)
    *canvas_dissolve=geometry_info.sigma/100.0;
  if ((*canvas_dissolve-MagickEpsilon) < 0.0)
    *canvas_dissolve=0.0;
  if ((*canvas_dissolve+MagickEpsilon) > 1.0)
    *canvas_dissolve=1.0;
}

static void SetBlendFactors(const GeometryInfo geometry_info,
  const MagickStatusType flags,MagickRealType *source_dissolve,
  MagickRealType *canvas_dissolve)
{
  *source_dissolve=geometry_info.rho/100.0;
  *canvas_dissolve=1.0-*source_dissolve;
  if ((flags & SigmaValue) != 0)
    *canvas_dissolve=geometry_info.sigma/100.0;
}

static MagickRealType GetChannelQuotient(const MagickRealType dividend,
  const MagickRealType divisor)
{
  MagickRealType
    pixel;

  if (fabs(divisor) < MagickEpsilon)
    pixel=(double) QuantumRange;
  else
    pixel=(double) QuantumRange*(dividend/divisor);
  return(pixel);
}

typedef struct _ChangeMaskImages
{
  Image
    *image,
    *source_image;
} ChangeMaskImages;

static MagickRealType GetChangeMaskAlpha(const ChangeMaskImages *images,
  const Quantum *magick_restrict p,const Quantum *magick_restrict q,
  const MagickRealType Da)
{
  MagickRealType
    pixel;

  if (IsFuzzyEquivalencePixel(images->source_image,p,images->image,q) !=
      MagickFalse)
    pixel=(MagickRealType) TransparentAlpha;
  else
    pixel=(double) QuantumRange*Da;
  return(pixel);
}

typedef struct _IntensityAlphaInputs
{
  MagickBooleanType
    compose_sync;

  MagickRealType
    Sa,
    Da,
    Si,
    Di;
} IntensityAlphaInputs;

static MagickRealType GetDarkenIntensityAlpha(
  const IntensityAlphaInputs *intensity,MagickRealType pixel)
{
  if (intensity->compose_sync == MagickFalse)
    {
      pixel=intensity->Si < intensity->Di ? intensity->Sa : intensity->Da;
      return(pixel);
    }
  pixel=intensity->Sa*intensity->Si < intensity->Da*intensity->Di ?
    intensity->Sa : intensity->Da;
  return(pixel);
}

static MagickRealType GetLightenIntensityAlpha(
  const IntensityAlphaInputs *intensity,MagickRealType pixel)
{
  const MagickBooleanType
    compose_sync = intensity->compose_sync;

  const MagickRealType
    Sa = intensity->Sa,
    Da = intensity->Da,
    Si = intensity->Si,
    Di = intensity->Di;

  if (compose_sync == MagickFalse)
    {
      pixel=Si > Di ? Sa : Da;
      return(pixel);
    }
  pixel=Sa*Si > Da*Di ? Sa : Da;
  return(pixel);
}

typedef struct _MultiplyAlphaInputs
{
  MagickBooleanType
    compose_sync;

  MagickRealType
    alpha,
    Sa,
    Da;
} MultiplyAlphaInputs;

static MagickRealType GetMultiplyAlpha(const MultiplyAlphaInputs *multiply,
  MagickRealType pixel)
{
  if (multiply->compose_sync == MagickFalse)
    {
      pixel=(double) QuantumRange*multiply->Sa*multiply->Da;
      return(pixel);
    }
  pixel=(double) QuantumRange*multiply->alpha;
  return(pixel);
}

static double GetHardMixBurn(const MagickRealType S,const MagickRealType D)
{
  double
    vivid;

  if (S <= MagickEpsilon)
    vivid=0.0;
  else
    vivid=1.0-(1.0-D)/(2.0*S);
  return(vivid);
}

static double GetHardMixVivid(const MagickRealType S,
  const MagickRealType D)
{
  double
    vivid;

  if (S <= 0.5)
    vivid=GetHardMixBurn(S,D);
  else
    {
      if ((1.0-(2.0*S-1.0)) <= MagickEpsilon)
        vivid=1.0;
      else
        vivid=D/(1.0-(2.0*S-1.0));
    }
  return(vivid);
}

static void CopyCompositeRow(const Image *source_image,const Image *image,
  const Quantum *p,Quantum *q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) source_image->columns; x++)
  {
    if (GetPixelReadMask(source_image,p) <= (QuantumRange/2))
      {
        p+=(ptrdiff_t) GetPixelChannels(source_image);
        q+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    CopySourceChannels(source_image,image,p,q);
    p+=(ptrdiff_t) GetPixelChannels(source_image);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

typedef struct _IntensityRowInputs
{
  Image
    *image,
    *source_image;

  MagickBooleanType
    clamp;
} IntensityRowInputs;

static void SetIntensityAlphaRow(const IntensityRowInputs *row,
  const Quantum *p,Quantum *q)
{
  Image
    *const image = row->image,
    *const source_image = row->source_image;

  const MagickBooleanType
    clamp = row->clamp;

  ssize_t
    x;

  for (x=0; x < (ssize_t) source_image->columns; x++)
  {
    if (GetPixelReadMask(source_image,p) <= (QuantumRange/2))
      {
        p+=(ptrdiff_t) GetPixelChannels(source_image);
        q+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    SetPixelAlpha(image,clamp != MagickFalse ?
      ClampPixel(GetPixelIntensity(source_image,p)) :
      ClampToQuantum(GetPixelIntensity(source_image,p)),q);
    p+=(ptrdiff_t) GetPixelChannels(source_image);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

typedef struct _CompositeSetup
{
  ssize_t
    x_offset,
    y_offset;
} CompositeSetup;

static MagickBooleanType CompositeCopyImage(Image *image,Image *source_image,
  const CompositeSetup *setup,ExceptionInfo *exception)
{
  const ssize_t
    x_offset = setup->x_offset,
    y_offset = setup->y_offset;

  CacheView
    *image_view,
    *source_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  status=MagickTrue;
  source_view=AcquireVirtualCacheView(source_image,exception);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(source_image,image,source_image->rows,4)
#endif
  for (y=0; y < (ssize_t) source_image->rows; y++)
  {
    const Quantum
      *p;

    Quantum
      *q;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(source_view,0,y,source_image->columns,1,
      exception);
    q=GetCacheViewAuthenticPixels(image_view,x_offset,y+y_offset,
      source_image->columns,1,exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    CopyCompositeRow(source_image,image,p,q);
    SyncCompositeRow(image_view,exception,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        SetCompositeProgress(image,CompositeImageTag,(MagickOffsetType) y,
          &status);
      }
  }
  source_view=DestroyCacheView(source_view);
  image_view=DestroyCacheView(image_view);
  source_image=DestroyImage(source_image);
  return(status);
}

typedef struct _DisplaceSetup
{
  CompositeOperator
    compose;

  MagickStatusType
    flags;

  GeometryInfo
    geometry_info;

  Image
    *image,
    *source_image;

  ssize_t
    x_offset,
    y_offset;
} DisplaceSetup;

static void SetDisplaceScaleArguments(const DisplaceSetup *displace,
  MagickRealType *horizontal_scale,MagickRealType *vertical_scale)
{
  (*horizontal_scale)=displace->geometry_info.rho;
  (*vertical_scale)=displace->geometry_info.sigma;
  if ((displace->flags & PercentValue) != 0)
    {
      if ((displace->flags & AspectValue) == 0)
        {
          (*horizontal_scale)*=(displace->source_image->columns-1)/200.0;
          (*vertical_scale)*=(displace->source_image->rows-1)/200.0;
        }
      else
        {
          (*horizontal_scale)*=(displace->image->columns-1)/200.0;
          (*vertical_scale)*=(displace->image->rows-1)/200.0;
        }
    }
  if ((displace->flags & HeightValue) == 0)
    (*vertical_scale)=(*horizontal_scale);
}

static void SetDisplaceScaleDefaults(const DisplaceSetup *displace,
  MagickRealType *horizontal_scale,MagickRealType *vertical_scale)
{
  if ((displace->flags & AspectValue) == 0)
    {
      (*horizontal_scale)=(MagickRealType) (displace->source_image->columns-1)/2.0;
      (*vertical_scale)=(MagickRealType) (displace->source_image->rows-1)/2.0;
    }
  else
    {
      (*horizontal_scale)=(MagickRealType) (displace->image->columns-1)/2.0;
      (*vertical_scale)=(MagickRealType) (displace->image->rows-1)/2.0;
    }
}

static void SetDistortCenterX(const DisplaceSetup *displace,PointInfo *center)
{
  if ((displace->flags & XValue) == 0)
    if ((displace->flags & AspectValue) != 0)
      (*center).x=(MagickRealType) ((displace->image->columns-1)/2.0);
    else
      (*center).x=(MagickRealType) (displace->x_offset+(displace->source_image->columns-1)/
        2.0);
  else
    if ((displace->flags & AspectValue) != 0)
      (*center).x=displace->geometry_info.xi;
    else
      (*center).x=(MagickRealType) (displace->x_offset+displace->geometry_info.xi);
}

static void SetDistortCenter(const DisplaceSetup *displace,PointInfo *center)
{
  if (displace->compose == DistortCompositeOp)
    {
      SetDistortCenterX(displace,center);
      if ((displace->flags & YValue) == 0)
        if ((displace->flags & AspectValue) != 0)
          (*center).y=(MagickRealType) ((displace->image->rows-1)/2.0);
        else
          (*center).y=(MagickRealType) (displace->y_offset+(displace->source_image->rows-1)/2.0);
      else
        if ((displace->flags & AspectValue) != 0)
          (*center).y=displace->geometry_info.psi;
        else
          (*center).y=(MagickRealType) (displace->y_offset+displace->geometry_info.psi);
    }
}

static void GetBlendIterations(const Image *image,double *iterations,
  double *residual_threshold,size_t *tick)
{
  const char
    *artifact;

  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  artifact=GetImageArtifact(image,"compose:args");
  if (artifact != (char *) NULL)
    {
      flags=ParseGeometry(artifact,&geometry_info);
      (*iterations)=geometry_info.rho;
      if ((flags & SigmaValue) != 0)
        (*residual_threshold)=geometry_info.sigma;
      if ((flags & XiValue) != 0)
        (*tick)=(size_t) geometry_info.xi;
    }
}

static void GetMathematicsArguments(const Image *image,
  GeometryInfo *geometry_info,ExceptionInfo *exception)
{
  const char
    *artifact;

  MagickStatusType
    flags;

  SetGeometryInfo(geometry_info);
  artifact=GetImageArtifact(image,"compose:args");
  if (artifact != (char *) NULL)
    {
      flags=ParseGeometry(artifact,geometry_info);
      if (flags == NoValue)
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "InvalidGeometry","`%s'",artifact);
    }
}

static void GetModulateArguments(const Image *image,
  GeometryInfo *geometry_info,MagickRealType *percent_luma,
  MagickRealType *percent_chroma)
{
  const char
    *artifact;

  MagickStatusType
    flags;

  artifact=GetImageArtifact(image,"compose:args");
  if (artifact != (char *) NULL)
    {
      flags=ParseGeometry(artifact,geometry_info);
      (*percent_luma)=(*geometry_info).rho;
      if ((flags & SigmaValue) != 0)
        (*percent_chroma)=(*geometry_info).sigma;
    }
}

static void GetThresholdArguments(const Image *image,
  GeometryInfo *geometry_info,MagickRealType *amount,MagickRealType *threshold)
{
  const char
    *artifact;

  MagickStatusType
    flags;

  artifact=GetImageArtifact(image,"compose:args");
  if (artifact != (char *) NULL)
    {
      flags=ParseGeometry(artifact,geometry_info);
      (*amount)=(*geometry_info).rho;
      (*threshold)=(*geometry_info).sigma;
      if ((flags & SigmaValue) == 0)
        (*threshold)=0.05f;
    }
  (*threshold)*=(double) QuantumRange;
}

typedef struct _BlurComposite
{
  double
    angle_range,
    angle_start,
    height,
    width;

  Image
    *canvas_image,
    *image,
    *source_image;

  CacheView
    *canvas_view,
    *source_view;

  ResampleFilter
    *resample_filter;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} BlurComposite;

static void RotateBlurEllipse(const BlurComposite *blur_composite,
  const Quantum *p,SegmentInfo *blur)
{
  const double
    angle_range = blur_composite->angle_range,
    angle_start = blur_composite->angle_start,
    height = blur_composite->height,
    width = blur_composite->width;

  Image
    *const source_image = blur_composite->source_image;

  if (fabs(angle_range) > MagickEpsilon)
    {
      MagickRealType
        angle;

      angle=angle_start+angle_range*QuantumScale*(double) 
        GetPixelBlue(source_image,p);
      (*blur).x1=width*cos(angle);
      (*blur).x2=width*sin(angle);
      (*blur).y1=(-height*sin(angle));
      (*blur).y2=height*cos(angle);
    }
}

typedef struct _BlurRow
{
  ssize_t
    y;

  const Quantum
    *p;

  Quantum
    *q;
} BlurRow;

static void BlurCompositeColumns(const BlurComposite *blur_composite,
  const BlurRow *row,SegmentInfo *blur,PixelInfo *pixel)
{
  Image
    *const canvas_image = blur_composite->canvas_image,
    *const image = blur_composite->image,
    *const source_image = blur_composite->source_image;

  ResampleFilter
    *const resample_filter = blur_composite->resample_filter;

  const ssize_t
    x_offset = blur_composite->x_offset,
    y_offset = blur_composite->y_offset,
    y = row->y;

  ExceptionInfo
    *const exception = blur_composite->exception;

  const Quantum
    *p = row->p;

  Quantum
    *q = row->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) source_image->columns; x++)
  {
    if (((x_offset+x) < 0) || ((x_offset+x) >= (ssize_t) image->columns))
      {
        p+=(ptrdiff_t) GetPixelChannels(source_image);
        continue;
      }
    RotateBlurEllipse(blur_composite,p,blur);
    ScaleResampleFilter(resample_filter,
      (*blur).x1*QuantumScale*(double) GetPixelRed(source_image,p),
      (*blur).y1*QuantumScale*(double) GetPixelGreen(source_image,p),
      (*blur).x2*QuantumScale*(double) GetPixelRed(source_image,p),
      (*blur).y2*QuantumScale*(double) GetPixelGreen(source_image,p) );
    (void) ResamplePixelColor(resample_filter,(double) x_offset+x,
      (double) y_offset+y,pixel,exception);
    SetPixelViaPixelInfo(canvas_image,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(source_image);
    q+=(ptrdiff_t) GetPixelChannels(canvas_image);
  }
}

static void BlurCompositeRows(const BlurComposite *blur_composite,
  SegmentInfo *blur,PixelInfo *pixel)
{
  Image
    *const canvas_image = blur_composite->canvas_image,
    *const image = blur_composite->image,
    *const source_image = blur_composite->source_image;

  CacheView
    *const canvas_view = blur_composite->canvas_view,
    *const source_view = blur_composite->source_view;

  const ssize_t
    y_offset = blur_composite->y_offset;

  ExceptionInfo
    *const exception = blur_composite->exception;

  ssize_t
    y;

  for (y=0; y < (ssize_t) source_image->rows; y++)
  {
    MagickBooleanType
      sync;

    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    BlurRow
      row;

    if (((y+y_offset) < 0) || ((y+y_offset) >= (ssize_t) image->rows))
      continue;
    p=GetCacheViewVirtualPixels(source_view,0,y,source_image->columns,1,
      exception);
    q=QueueCacheViewAuthenticPixels(canvas_view,0,y,canvas_image->columns,1,
      exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      break;
    row.y=y;
    row.p=p;
    row.q=q;
    BlurCompositeColumns(blur_composite,&row,blur,pixel);
    sync=SyncCacheViewAuthenticPixels(canvas_view,exception);
    if (sync == MagickFalse)
      break;
  }
}

typedef struct _DisplaceColumns
{
  Image
    *canvas_image,
    *image,
    *source_image;

  CacheView
    *image_view;

  PointInfo
    center;

  CompositeOperator
    compose;

  MagickRealType
    horizontal_scale,
    vertical_scale;

  ssize_t
    x_offset,
    y;

  ExceptionInfo
    *exception;
} DisplaceColumns;

static MagickBooleanType DisplaceCompositeColumns(
  const DisplaceColumns *columns,const Quantum *magick_restrict p,
  Quantum *magick_restrict q,PixelInfo *pixel)
{
  Image
    *const canvas_image = columns->canvas_image,
    *const image = columns->image,
    *const source_image = columns->source_image;

  CacheView
    *const image_view = columns->image_view;

  const PointInfo
    center = columns->center;

  const CompositeOperator
    compose = columns->compose;

  const MagickRealType
    horizontal_scale = columns->horizontal_scale,
    vertical_scale = columns->vertical_scale;

  const ssize_t
    x_offset = columns->x_offset,
    y = columns->y;

  ExceptionInfo
    *const exception = columns->exception;

  MagickBooleanType
    status;

  PointInfo
    offset;

  ssize_t
    x;

  for (x=0; x < (ssize_t) source_image->columns; x++)
  {
    if (((x_offset+x) < 0) || ((x_offset+x) >= (ssize_t) image->columns))
      {
        p+=(ptrdiff_t) GetPixelChannels(source_image);
        continue;
      }
    /*
      Displace the offset.
    */
    offset.x=(double) (horizontal_scale*((double) GetPixelRed(
      source_image,p)-(((MagickRealType) QuantumRange+1.0)/2.0)))/
      (((MagickRealType) QuantumRange+1.0)/2.0)+center.x+
      ((compose == DisplaceCompositeOp) ? x : 0);
    offset.y=(double) (vertical_scale*((double) GetPixelGreen(
      source_image,p)-(((MagickRealType) QuantumRange+1.0)/2.0)))/
      (((MagickRealType) QuantumRange+1.0)/2.0)+center.y+
      ((compose == DisplaceCompositeOp) ? y : 0);
    status=InterpolatePixelInfo(image,image_view,
      UndefinedInterpolatePixel,(double) offset.x,(double) offset.y,
      pixel,exception);
    if (status == MagickFalse)
      break;
    /*
      Mask with the 'invalid (*pixel) mask' in alpha channel.
    */
    (*pixel).alpha=(MagickRealType) QuantumRange*(QuantumScale*(*pixel).alpha)*
      (QuantumScale*(double) GetPixelAlpha(source_image,p));
    SetPixelViaPixelInfo(canvas_image,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(source_image);
    q+=(ptrdiff_t) GetPixelChannels(canvas_image);
  }
  return(MagickTrue);
}

typedef struct _CompositeRowInputs
{
  Image
    *image,
    *source_image;

  CompositeOperator
    compose;

  MagickBooleanType
    clip_to_self;

  ssize_t
    x_offset,
    y_offset,
    y;

  ExceptionInfo
    *exception;

  const Quantum
    *pixels;

  MagickRealType
    amount,
    canvas_dissolve,
    midpoint,
    percent_luma,
    percent_chroma,
    source_dissolve,
    threshold;

  MagickBooleanType
    clamp,
    compose_sync;

  ColorspaceType
    colorspace;

  double
    white_luminance;

  IlluminantType
    illuminant;

  GeometryInfo
    geometry_info;

  MagickBooleanType
    union_coverage,
    canvas_alpha,
    source_alpha,
    pixel_info_inputs,
    intensity_inputs;
} CompositeRowInputs;

static void SetCompositeCoverageModulus(const CompositeRowInputs *row,
  const MagickRealType Sa,const MagickRealType Da,MagickRealType *alpha)
{
  const CompositeOperator
    compose = row->compose;

  switch (compose)
  {
    case DstOutCompositeOp:
    {
      (*alpha)=Da*(1.0-Sa);
      break;
    }
    case XorCompositeOp:
    {
      (*alpha)=Sa+Da-2.0*Sa*Da;
      break;
    }
    case ModulusAddCompositeOp:
    {
      if ((Sa+Da) <= 1.0)
        {
          (*alpha)=(Sa+Da);
          break;
        }
      (*alpha)=((Sa+Da)-1.0);
      break;
    }
    case ModulusSubtractCompositeOp:
    {
      if ((Sa-Da) >= 0.0)
        {
          (*alpha)=(Sa-Da);
          break;
        }
      (*alpha)=((Sa-Da)+1.0);
      break;
    }
    default:
    {
      (*alpha)=1.0;
      break;
    }
  }
}

static void SetCompositeCoverageProduct(const CompositeRowInputs *row,
  const MagickRealType Sa,const MagickRealType Da,MagickRealType *alpha)
{
  const CompositeOperator
    compose = row->compose;

  switch (compose)
  {
    case DstAtopCompositeOp:
    case DstInCompositeOp:
    case InCompositeOp:
    case SrcInCompositeOp:
    {
      (*alpha)=Sa*Da;
      break;
    }
    case OutCompositeOp:
    case SrcOutCompositeOp:
    {
      (*alpha)=Sa*(1.0-Da);
      break;
    }
    default:
    {
      SetCompositeCoverageModulus(row,Sa,Da,alpha);
      break;
    }
  }
}

static void SetCompositeCoverageDissolve(const CompositeRowInputs *row,
  const MagickRealType Sa,const MagickRealType Da,MagickRealType *alpha)
{
  const CompositeOperator
    compose = row->compose;

  const MagickRealType
    canvas_dissolve = row->canvas_dissolve,
    source_dissolve = row->source_dissolve;

  switch (compose)
  {
    case DissolveCompositeOp:
    {
      (*alpha)=source_dissolve*Sa*(-canvas_dissolve*Da)+source_dissolve*Sa+
        canvas_dissolve*Da;
      break;
    }
    case DstOverCompositeOp:
    case OverCompositeOp:
    case SrcOverCompositeOp:
    {
      (*alpha)=Sa+Da-Sa*Da;
      break;
    }
    case BlendCompositeOp:
    case PlusCompositeOp:
    {
      (*alpha)=RoundToUnity(source_dissolve*Sa+canvas_dissolve*Da);
      break;
    }
    default:
    {
      SetCompositeCoverageProduct(row,Sa,Da,alpha);
      break;
    }
  }
}

static const CompositeOperator
  UnionCoverageOperators[] =
  {
    BumpmapCompositeOp,
    ColorBurnCompositeOp,
    ColorDodgeCompositeOp,
    DarkenCompositeOp,
    DifferenceCompositeOp,
    DivideCompositeOp,
    DivideDstCompositeOp,
    DivideSrcCompositeOp,
    ExclusionCompositeOp,
    FreezeCompositeOp,
    HardLightCompositeOp,
    HardMixCompositeOp,
    InterpolateCompositeOp,
    LightenCompositeOp,
    LinearBurnCompositeOp,
    LinearDodgeCompositeOp,
    LinearLightCompositeOp,
    MathematicsCompositeOp,
    MinusDstCompositeOp,
    MinusSrcCompositeOp,
    MultiplyCompositeOp,
    NegateCompositeOp,
    OverlayCompositeOp,
    PegtopLightCompositeOp,
    PinLightCompositeOp,
    ReflectCompositeOp,
    ScreenCompositeOp,
    SoftBurnCompositeOp,
    SoftDodgeCompositeOp,
    SoftLightCompositeOp,
    StampCompositeOp,
    VividLightCompositeOp
  };

static MagickBooleanType IsListedOperator(const CompositeOperator *list,
  const size_t count,const CompositeOperator compose)
{
  size_t
    i;

  for (i=0; i < count; i++)
    if (list[i] == compose)
      return(MagickTrue);
  return(MagickFalse);
}

static void SetCompositeCoverage(const CompositeRowInputs *row,
  const MagickRealType Sa,const MagickRealType Da,MagickRealType *alpha)
{
  if (row->union_coverage != MagickFalse)
    {
      (*alpha)=RoundToUnity(Sa+Da-Sa*Da);
      return;
    }
  SetCompositeCoverageDissolve(row,Sa,Da,alpha);
}

typedef struct _CompositePixelInputs
{
  const Quantum
    *p;

  MagickRealType
    alpha,
    Da,
    Di,
    Sa,
    Si;

  const PixelInfo
    *canvas_pixel,
    *source_pixel;
} CompositePixelInputs;

static const CompositeOperator
  CanvasAlphaOperators[] =
  {
    AtopCompositeOp,
    CopyBlackCompositeOp,
    CopyBlueCompositeOp,
    CopyCyanCompositeOp,
    CopyGreenCompositeOp,
    CopyMagentaCompositeOp,
    CopyRedCompositeOp,
    CopyYellowCompositeOp,
    SrcAtopCompositeOp,
    DstCompositeOp,
    NoCompositeOp
  };

static const CompositeOperator
  SourceAlphaOperators[] =
  {
    BlurCompositeOp,
    CopyCompositeOp,
    DisplaceCompositeOp,
    DistortCompositeOp,
    DstAtopCompositeOp,
    ReplaceCompositeOp,
    SrcCompositeOp
  };

static void CompositeAlphaMultiplyRun(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,MagickRealType *pixel)
{
  const CompositeOperator
    compose = row->compose;

  const MagickBooleanType
    compose_sync = row->compose_sync;

  const MagickRealType
    alpha = authentic->alpha,
    Da = authentic->Da,
    Sa = authentic->Sa;

  switch (compose)
  {
    case MultiplyCompositeOp:
    {
      MultiplyAlphaInputs
        multiply;

      multiply.compose_sync=compose_sync;
      multiply.alpha=alpha;
      multiply.Sa=Sa;
      multiply.Da=Da;
      (*pixel)=GetMultiplyAlpha(&multiply,(*pixel));
      break;
    }
    case NegateCompositeOp:
    {
      (*pixel)=(double) QuantumRange*((1.0-Sa-Da));
      break;
    }
    case ReflectCompositeOp:
    {
      (*pixel)=GetReflectAlpha(Sa,Da);
      break;
    }
    case StampCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(Sa+Da*Da-1.0);
      break;
    }
    case StereoCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(Sa+Da)/2;
      break;
    }
    default:
    {
      (*pixel)=(double) QuantumRange*alpha;
      break;
    }
  }
}

static void CompositeAlphaDarkenRun(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,MagickRealType *pixel)
{
  const CompositeOperator
    compose = row->compose;

  const MagickBooleanType
    compose_sync = row->compose_sync;

  const MagickRealType
    Da = authentic->Da,
    Di = authentic->Di,
    Sa = authentic->Sa,
    Si = authentic->Si;

  switch (compose)
  {
    case DarkenIntensityCompositeOp:
    {
      IntensityAlphaInputs
        intensity;

      intensity.compose_sync=compose_sync;
      intensity.Sa=Sa;
      intensity.Da=Da;
      intensity.Si=Si;
      intensity.Di=Di;
      (*pixel)=GetDarkenIntensityAlpha(&intensity,(*pixel));
      break;
    }
    case DifferenceCompositeOp:
    {
      (*pixel)=(double) QuantumRange*fabs((double) (Sa-Da));
      break;
    }
    case FreezeCompositeOp:
    {
      (*pixel)=GetFreezeAlpha(Sa,Da);
      break;
    }
    case InterpolateCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(0.5-0.25*cos(MagickPI*Sa)-0.25*
        cos(MagickPI*Da));
      break;
    }
    case LightenIntensityCompositeOp:
    {
      IntensityAlphaInputs
        intensity;

      intensity.compose_sync=compose_sync;
      intensity.Sa=Sa;
      intensity.Da=Da;
      intensity.Si=Si;
      intensity.Di=Di;
      (*pixel)=GetLightenIntensityAlpha(&intensity,(*pixel));
      break;
    }
    case ModulateCompositeOp:
    {
      (*pixel)=(double) QuantumRange*Da;
      break;
    }
    default:
    {
      CompositeAlphaMultiplyRun(row,authentic,pixel);
      break;
    }
  }
}

static void CompositeAlphaColorizeRun(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,MagickRealType *pixel)
{
  Image
    *const source_image = row->source_image;

  const CompositeOperator
    compose = row->compose;

  const MagickRealType
    Da = authentic->Da,
    Sa = authentic->Sa,
    Si = authentic->Si;

  switch (compose)
  {
    case ColorizeCompositeOp:
    case HueCompositeOp:
    case LuminizeCompositeOp:
    case RMSECompositeOp:
    case SaturateCompositeOp:
    {
      (*pixel)=GetHueAlpha(Sa,Da,(*pixel));
      break;
    }
    case CopyAlphaCompositeOp:
    {
      (*pixel)=GetCopyAlpha(source_image,Si,Sa);
      break;
    }
    default:
    {
      CompositeAlphaDarkenRun(row,authentic,pixel);
      break;
    }
  }
}

static void CompositeAlphaChannel(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,Quantum *magick_restrict q,
  MagickRealType *pixel)
{
  Image
    *const image = row->image,
    *const source_image = row->source_image;

  const CompositeOperator
    compose = row->compose;

  const Quantum
    *const p = authentic->p;

  const MagickRealType
    Da = authentic->Da,
    Sa = authentic->Sa,
    Si = authentic->Si;

  if (row->canvas_alpha != MagickFalse)
    {
      (*pixel)=(double) QuantumRange*Da;
      return;
    }
  if (row->source_alpha != MagickFalse)
    {
      (*pixel)=(double) QuantumRange*Sa;
      return;
    }
  switch (compose)
  {
    case AlphaCompositeOp:
    {
      (*pixel)=(double) QuantumRange*Sa;
      break;
    }
    case BumpmapCompositeOp:
    {
      (*pixel)=Si*Da;
      break;
    }
    case ChangeMaskCompositeOp:
    {
      ChangeMaskImages
        images;

      images.image=image;
      images.source_image=source_image;
      (*pixel)=GetChangeMaskAlpha(&images,p,q,Da);
      break;
    }
    case ClearCompositeOp:
    {
      (*pixel)=(MagickRealType) TransparentAlpha;
      break;
    }
    default:
    {
      CompositeAlphaColorizeRun(row,authentic,pixel);
      break;
    }
  }
}

typedef struct _CompositeChannelInputs
{
  PixelChannel
    channel;

  MagickRealType
    Dc,
    Dca,
    Sc,
    Sca;

  double
    gamma;
} CompositeChannelInputs;

static void CompositeChannelColor(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,const CompositeChannelInputs *tile,
  MagickRealType *pixel)
{
  Image
    *const source_image = row->source_image;

  const CompositeOperator
    compose = row->compose;

  const MagickRealType
    amount = row->amount,
    canvas_dissolve = row->canvas_dissolve,
    midpoint = row->midpoint,
    percent_luma = row->percent_luma,
    percent_chroma = row->percent_chroma,
    source_dissolve = row->source_dissolve,
    threshold = row->threshold;

  const MagickBooleanType
    compose_sync = row->compose_sync;

  const ColorspaceType
    colorspace = row->colorspace;

  const double
    white_luminance = row->white_luminance;

  const IlluminantType
    illuminant = row->illuminant;

  const GeometryInfo
    geometry_info = row->geometry_info;

  const Quantum
    *const p = authentic->p;

  const MagickRealType
    Da = authentic->Da,
    Di = authentic->Di,
    Sa = authentic->Sa,
    Si = authentic->Si;

  const PixelInfo
    *const canvas_pixel = authentic->canvas_pixel,
    *const source_pixel = authentic->source_pixel;

  const PixelChannel
    channel = tile->channel;

  const MagickRealType
    Dc = tile->Dc,
    Dca = tile->Dca,
    Sc = tile->Sc,
    Sca = tile->Sca;

  const double
    gamma = tile->gamma;

  MagickRealType
    blend = 0.0,
    D = 0.0,
    S = 0.0,
    blue = 0.0,
    chroma = 0.0,
    green = 0.0,
    hue = 0.0,
    luma = 0.0,
    red = 0.0,
    sans = 0.0;

  switch (compose)
  {
    case AlphaCompositeOp:
    {
      (*pixel)=(double) QuantumRange*Sa;
      break;
    }
    case AtopCompositeOp:
    case SrcAtopCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(Sca*Da+Dca*(1.0-Sa));
      break;
    }
    case BlendCompositeOp:
    {
      (*pixel)=gamma*(source_dissolve*Sa*Sc+canvas_dissolve*Da*Dc);
      break;
    }
    case CopyCompositeOp:
    case ReplaceCompositeOp:
    {
      (*pixel)=(double) QuantumRange*Sca;
      break;
    }
    case BlurCompositeOp:
    case DisplaceCompositeOp:
    case DistortCompositeOp:
    case SrcCompositeOp:
    {
      (*pixel)=Sc;
      break;
    }
    case BumpmapCompositeOp:
    {
      if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Dc;
          break;
        }
      (*pixel)=(QuantumScale*Si)*Dc;
      break;
    }
    case ChangeMaskCompositeOp:
    {
      (*pixel)=Dc;
      break;
    }
    case ClearCompositeOp:
    {
      (*pixel)=0.0;
      break;
    }
    case ColorBurnCompositeOp:
    {
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      if (fabs(S) < MagickEpsilon)
        blend=0.0;
      else
        blend=MagickMin(1.0,MagickMax(0.0,1.0-(1.0-D)/S));
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(Sa*Da*blend+Sa*
        (1.0-Da)*S+Da*(1.0-Sa)*D);
      break;
    }
    case ColorDodgeCompositeOp:
    {
      if (Sa > 0.0)
        S=RoundToUnity(Sca/Sa);
      else
        S=0.0;
      if (Da > 0.0)
        D=RoundToUnity(Dca/Da);
      else
        D=0.0;
      blend=GetColorDodgeBlend(S,D);
      (*pixel)=(double) QuantumRange*gamma*(Sa*Da*blend+Sca*(1.0-Da)+Dca*
        (1.0-Sa));
      break;
    }
    case ColorizeCompositeOp:
    {
      if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Dc;
          break;
        }
      if (fabs((double) QuantumRange*Da-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Sc;
          break;
        }
      ConvertRGBToGeneric(colorspace,(double) (*canvas_pixel).red,
        (double) (*canvas_pixel).green,(double) (*canvas_pixel).blue,
        white_luminance,illuminant,&sans,&sans,&luma);
      ConvertRGBToGeneric(colorspace,(double) (*source_pixel).red,
        (double) (*source_pixel).green,(double) (*source_pixel).blue,
        white_luminance,illuminant,&hue,&chroma,&sans);
      ConvertGenericToRGB(colorspace,hue,chroma,luma,
        white_luminance,illuminant,&red,&green,&blue);
      switch (channel)
      {
        case RedPixelChannel: (*pixel)=red; break;
        case GreenPixelChannel: (*pixel)=green; break;
        case BluePixelChannel: (*pixel)=blue; break;
        default: (*pixel)=Dc; break;
      }
      break;
    }
    case CopyAlphaCompositeOp:
    case DstCompositeOp:
    {
      (*pixel)=Dc;
      break;
    }
    case CopyBlackCompositeOp:
    {
      if (channel == BlackPixelChannel)
        (*pixel)=(MagickRealType) GetPixelBlack(source_image,p);
      break;
    }
    case CopyBlueCompositeOp:
    case CopyYellowCompositeOp:
    {
      if (channel == BluePixelChannel)
        (*pixel)=(MagickRealType) GetPixelBlue(source_image,p);
      break;
    }
    case CopyGreenCompositeOp:
    case CopyMagentaCompositeOp:
    {
      if (channel == GreenPixelChannel)
        (*pixel)=(MagickRealType) GetPixelGreen(source_image,p);
      break;
    }
    case CopyRedCompositeOp:
    case CopyCyanCompositeOp:
    {
      if (channel == RedPixelChannel)
        (*pixel)=(MagickRealType) GetPixelRed(source_image,p);
      break;
    }
    case DarkenCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=RoundToUnity(MagickMin(Sca,Dca)+Sca*(1.0-Da)+Dca*
            (1.0-Sa));
          break;
        }
      (*pixel)=(double) QuantumRange*RoundToUnity(MagickMin(Sca,Dca)+Sca*
        (1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case DarkenIntensityCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=RoundToUnity(MagickMin(Sca,Dca)+Sca*(1.0-Di)+Dca*
            (1.0-Si));
          break;
        }
      (*pixel)=(double) QuantumRange*RoundToUnity(MagickMin(Sca,Dca)+Sca*
        (1.0-Di)+Dca*(1.0-Si));
      break;
    }
    case DifferenceCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
           (*pixel)=(double) QuantumRange*RoundToUnity(fabs((double) Sc-
            (double) Dc));
           break;
        }
      S=(Sa > 0.0) ? (Sca/Sa) : 0.0;
      D=(Da > 0.0) ? (Dca/Da) : 0.0;
      (*pixel)=(double) QuantumRange*RoundToUnity(fabs(S-D));
      break;
    }
    case DissolveCompositeOp:
    {
      (*pixel)=gamma*(source_dissolve*Sa*Sc-source_dissolve*Sa*
        canvas_dissolve*Da*Dc+canvas_dissolve*Da*Dc);
      break;
    }
    case DivideCompositeOp:
    {
      D=(Da > 0.0) ? (Dca/Da) : 0.0;
      S=(Sa > 0.0) ? (Sca/Sa) : 0.0;
      if (fabs(S) < MagickEpsilon)
        blend=1.0;
      else
        blend=RoundToUnity(D/S);
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(Sca*(1.0-Da)+Dca*(1.0-Sa)+
        Sa*Da*blend);
      break;
    }
    case DivideDstCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=GetChannelQuotient(Sc,Dc);
          break;
        }
      if (fabs(Dca) < MagickEpsilon)
        (*pixel)=(double) QuantumRange*gamma*(Sca+(1.0-Sa));
      else
        (*pixel)=(double) QuantumRange*gamma*(Sca/Dca+Sca*(1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case DivideSrcCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=GetChannelQuotient(Dc,Sc);
          break;
        }
      if (fabs(Dca) < MagickEpsilon)
        (*pixel)=(double) QuantumRange*gamma*(Dca+(1.0-Da));
      else
        (*pixel)=(double) QuantumRange*gamma*(Dca/Sca+Dca*(1.0-Sa)+Sca*(1.0-Da));
      break;
    }
    case DstAtopCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(Dca*Sa+Sca*(1.0-Da));
      break;
    }
    case DstInCompositeOp:
    {
      (*pixel)=(double) QuantumRange*gamma*(Dca*Sa);
      break;
    }
    case DstOutCompositeOp:
    {
      (*pixel)=(double) QuantumRange*gamma*(Dca*(1.0-Sa));
      break;
    }
    case DstOverCompositeOp:
    {
      (*pixel)=(double) QuantumRange*gamma*(Dca+Sca*(1.0-Da));
      break;
    }
    case ExclusionCompositeOp:
    {
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      blend=RoundToUnity(S+D-2.0*S*D);
      (*pixel)=(double) QuantumRange*RoundToUnity((blend*Sa+D*(1.0-Sa))*
        (Sa+Da-Sa*Da));
      break;
    }
    case FreezeCompositeOp:
    {          
      if (Dca != 0.0)
        blend=1.0-(1.0-Sca)*(1.0-Sca)/Dca;
      else
        blend=0.0;
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(blend);
      break;
    }
    case HardLightCompositeOp:
    {
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      if (S <= 0.5)
        blend=2.0*S*D;
      else
        blend=1.0-2.0*(1.0-S)*(1.0-D);
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity((1.0-Da)*Sca+
        (1.0-Sa)*Dca+blend*Sa*Da);
      break;
    }
    case HardMixCompositeOp:
    {
      double
        hardmix,
        vivid;

      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      vivid=GetHardMixVivid(S,D);
      hardmix=(vivid < 0.5) ? 0.0 : 1.0;
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(Sa*Da*hardmix+Sca*
        (1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case HueCompositeOp:
    {
      if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Dc;
          break;
        }
      if (fabs((double) QuantumRange*Da-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Sc;
          break;
        }
      ConvertRGBToGeneric(colorspace,(double) (*canvas_pixel).red,
        (double) (*canvas_pixel).green,(double) (*canvas_pixel).blue,
        white_luminance,illuminant,&hue,&chroma,&luma);
      ConvertRGBToGeneric(colorspace,(double) (*source_pixel).red,
        (double) (*source_pixel).green,(double) (*source_pixel).blue,
        white_luminance,illuminant,&hue,&sans,&sans);
      ConvertGenericToRGB(colorspace,hue,chroma,luma,
        white_luminance,illuminant,&red,&green,&blue);
      switch (channel)
      {
        case RedPixelChannel: (*pixel)=red; break;
        case GreenPixelChannel: (*pixel)=green; break;
        case BluePixelChannel: (*pixel)=blue; break;
        default: (*pixel)=Dc; break;
      }
      break;
    }
    case InCompositeOp:
    case SrcInCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(Sca*Da);
      break;
    }
    case InterpolateCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(0.5-0.25*cos(MagickPI*Sca)-0.25*
        cos(MagickPI*Dca));
      break;
    }
    case LinearBurnCompositeOp:
    {
      /*
        LinearBurn: as defined by Abode Photoshop, according to
        http://www.simplefilter.de/en/basics/mixmods.html is:

          f(Sc,Dc) = Sc + Dc - 1
      */
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      blend=RoundToUnity(S+D-1.0);
      (*pixel)=(double) QuantumRange*RoundToUnity(Sa*Da*blend+Sca*(1.0-Da)+
        Dca*(1.0-Sa));
      break;
    }
    case LinearDodgeCompositeOp:
    {
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      blend=RoundToUnity(S+D);
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(Sa*Da*blend+
        Sca*(1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case LinearLightCompositeOp:
    {
      /*
        Linear Light (Adobe standard):
          f(Sc, Dc) = Dc + 2*Sc - 1
        Applied in linear (HDRI) space with clamping.
      */
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      blend=RoundToUnity(D+2.0*S-1.0);
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(Sa*Da*blend+Sca*
        (1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case LightenCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=MagickMax(Sc,Dc);
          break;
        }
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      (*pixel)=(double) QuantumRange*RoundToUnity(Sca*(1.0-Da)+Dca*(1.0-Sa)+
        MagickMax(S,D)*Sa*Da);
      break;
    }
    case LightenIntensityCompositeOp:
    {
      (*pixel)=Si > Di ? Sc : Dc;
      break;
    }
    case LuminizeCompositeOp:
    {
      if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Dc;
          break;
        }
      if (fabs((double) QuantumRange*Da-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Sc;
          break;
        }
      ConvertRGBToGeneric(colorspace,(double) (*canvas_pixel).red,
        (double) (*canvas_pixel).green,(double) (*canvas_pixel).blue,
        white_luminance,illuminant,&hue,&chroma,&luma);
      ConvertRGBToGeneric(colorspace,(double) (*source_pixel).red,
        (double) (*source_pixel).green,(double) (*source_pixel).blue,
        white_luminance,illuminant,&sans,&sans,&luma);
      ConvertGenericToRGB(colorspace,hue,chroma,luma,
        white_luminance,illuminant,&red,&green,&blue);
      switch (channel)
      {
        case RedPixelChannel: (*pixel)=red; break;
        case GreenPixelChannel: (*pixel)=green; break;
        case BluePixelChannel: (*pixel)=blue; break;
        default: (*pixel)=Dc; break;
      }
      break;
    }
    case MathematicsCompositeOp:
    {
      /*
        'Mathematics' a free form user control mathematical composition
        is defined as...

          f(Sc,Dc) = A*Sc*Dc + B*Sc + C*Dc + D

        Where the arguments A,B,C,D are (currently) passed to composite
        as a command separated 'geometry' string in "compose:args" image
        artifact.

           A = a->rho,   B = a->sigma,  C = a->xi,  D = a->psi

        Applying the SVG transparency formula (see above), we get...

         Dca' = Sa*Da*f(Sc,Dc) + Sca*(1.0-Da) + Dca*(1.0-Sa)

         Dca' = A*Sca*Dca + B*Sca*Da + C*Dca*Sa + D*Sa*Da + Sca*(1.0-Da) +
           Dca*(1.0-Sa)
      */
      if (compose_sync == MagickFalse)
        {
          (*pixel)=geometry_info.rho*Sc*Dc+geometry_info.sigma*Sc+
            geometry_info.xi*Dc+geometry_info.psi;
          break;
        }
      (*pixel)=(double) QuantumRange*gamma*(geometry_info.rho*Sca*Dca+
        geometry_info.sigma*Sca*Da+geometry_info.xi*Dca*Sa+
        geometry_info.psi*Sa*Da+Sca*(1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case MinusDstCompositeOp:
    {
     if (compose_sync == MagickFalse)
        {
          (*pixel)=Dc-Sc;
          break;
        }
      (*pixel)=gamma*(Sa*Sc+Da*Dc-2.0*Da*Dc*Sa);
      break;
    }
    case MinusSrcCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=Sc-Dc;
          break;
        }
      (*pixel)=gamma*(Da*Dc+Sa*Sc-2.0*Sa*Sc*Da);
      break;
    }
    case ModulateCompositeOp:
    {
      ssize_t
        offset;

      if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Dc;
          break;
        }
      offset=(ssize_t) (Si-midpoint);
      if (offset == 0)
        {
          (*pixel)=Dc;
          break;
        }
      ConvertRGBToGeneric(colorspace,(double) (*canvas_pixel).red,
        (double) (*canvas_pixel).green,(double) (*canvas_pixel).blue,
        white_luminance,illuminant,&hue,&chroma,&luma);
      luma+=(0.01*percent_luma*offset)/midpoint;
      chroma*=0.01*percent_chroma;
      ConvertGenericToRGB(colorspace,hue,chroma,luma,
        white_luminance,illuminant,&red,&green,&blue);
      switch (channel)
      {
        case RedPixelChannel: (*pixel)=red; break;
        case GreenPixelChannel: (*pixel)=green; break;
        case BluePixelChannel: (*pixel)=blue; break;
        default: (*pixel)=Dc; break;
      }
      break;
    }
    case ModulusAddCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=(Quantum) QuantumRange*((Sc+Dc)-floor(Sc+Dc));
          break;
        }
      (*pixel)=(Quantum) QuantumRange*((Sca+Dca)-floor(Sca+Dca));
      break;
    }
    case ModulusSubtractCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=(Quantum) QuantumRange*((Sc-Dc)-floor(Sc-Dc));
          break;
        }
      (*pixel)=(Quantum) QuantumRange*((Sca-Dca)-floor(Sca-Dca));
      break;
    }
    case MultiplyCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=(double) QuantumRange*Sc*Dc;
          break;
        }
      (*pixel)=(double) QuantumRange*gamma*(Sca*Dca+Sca*(1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case NegateCompositeOp:
    {
      D=(Da > 0.0) ? Dca/Da : 0.0;
      S=(Sa > 0.0) ? Sca/Sa : 0.0;
      (*pixel)=(double) QuantumRange*((1.0-fabs(1.0-S-D))*Da);
      break;
    }
    case NoCompositeOp:
    {
      (*pixel)=(double) QuantumRange*Dca;
      break;
    }
    case OutCompositeOp:
    case SrcOutCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(Sca*(1.0-Da));
      break;
    }
    case OverCompositeOp:
    case SrcOverCompositeOp:
    {
      if ((Sa+Da*(1.0-Sa)) <= MagickEpsilon)
        (*pixel)=0.0;
      else
        (*pixel)=(double) QuantumRange*gamma*((Sca+Dca*(1.0-Sa))/
          (Sa+Da*(1.0-Sa)));
      break;
    }
    case OverlayCompositeOp:
    {
      if ((2.0*Dca) < Da)
        {
          (*pixel)=(double) QuantumRange*gamma*(2.0*Dca*Sca+Dca*(1.0-Sa)+
            Sca*(1.0-Da));
          break;
        }
      (*pixel)=(double) QuantumRange*gamma*(Da*Sa-2.0*(Sa-Sca)*(Da-Dca)+Dca*
        (1.0-Sa)+Sca*(1.0-Da));
      break;
    }
    case PegtopLightCompositeOp:
    {
      if (fabs((double) Da) < MagickEpsilon)
        {
          (*pixel)=(double) QuantumRange*gamma*Sca;
          break;
        }
      blend=GetPegtopLightBlend(Sca,Da,Dca);
      (*pixel)=(double) QuantumRange*gamma*(RoundToUnity(blend)*Da*Sa+Dca*
        (1.0-Sa));
      break;
    }
    case PinLightCompositeOp:
    {
      /*
        Adobe Pin Light (colors in [0,1]):

          if (Cs <= 0.5)
            f = min(Cd, 2*Cs);
          else
            f = max(Cd, 2*Cs - 1);
      */
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      if (S <= 0.5)
        blend=MagickMin(D,2.0*S);
      else
        blend=MagickMax(D,2.0*S-1.0);
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(Sa*Da*blend+Sca*
        (1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case PlusCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=(double) QuantumRange*(Sc+Dc);
          break;
        }
      (*pixel)=(double) QuantumRange*(Sca+Dca);
      break;
    }
    case ReflectCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          blend=GetReflectBlend(Sc,Dc);
          (*pixel)=(double) QuantumRange*RoundToUnity(blend);
          break;
        }
      if (Sa > 0.0)
        S=Sca/Sa;
      else
        S=0.0;
      if (Da > 0.0)
        D=Dca/Da;
      else
        D=0.0;
      if (D < 1.0)
        blend=(S*S)/(1.0-D);
      else
        blend=1.0;
      (*pixel)=(double) QuantumRange*RoundToUnity((Sa+Da-Sa*Da)*blend);
      break;
    }
    case RMSECompositeOp:
    {
      double
        gray;

      if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Dc;
          break;
        }
      if (fabs((double) QuantumRange*Da-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Sc;
          break;
        }
      gray=sqrt(
        ((*canvas_pixel).red-(*source_pixel).red)*
        ((*canvas_pixel).red-(*source_pixel).red)+
        ((*canvas_pixel).green-(*source_pixel).green)*
        ((*canvas_pixel).green-(*source_pixel).green)+
        ((*canvas_pixel).blue-(*source_pixel).blue)*
        ((*canvas_pixel).blue-(*source_pixel).blue)/3.0);
      switch (channel)
      {
        case RedPixelChannel: (*pixel)=gray; break;
        case GreenPixelChannel: (*pixel)=gray; break;
        case BluePixelChannel: (*pixel)=gray; break;
        default: (*pixel)=Dc; break;
      }
      break;
    }
    case SaturateCompositeOp:
    {
      if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Dc;
          break;
        }
      if (fabs((double) QuantumRange*Da-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Sc;
          break;
        }
      ConvertRGBToGeneric(colorspace,(double) (*canvas_pixel).red,
        (double) (*canvas_pixel).green,(double) (*canvas_pixel).blue,
        white_luminance,illuminant,&hue,&chroma,&luma);
      ConvertRGBToGeneric(colorspace,(double) (*source_pixel).red,
        (double) (*source_pixel).green,(double) (*source_pixel).blue,
        white_luminance,illuminant,&sans,&chroma,&sans);
      ConvertGenericToRGB(colorspace,hue,chroma,luma,
        white_luminance,illuminant,&red,&green,&blue);
      switch (channel)
      {
        case RedPixelChannel: (*pixel)=red; break;
        case GreenPixelChannel: (*pixel)=green; break;
        case BluePixelChannel: (*pixel)=blue; break;
        default: (*pixel)=Dc; break;
      }
      break;
    }
    case ScreenCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=(double) QuantumRange*RoundToUnity(Sc+Dc-Sc*Dc);
          break;
        }
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      blend=RoundToUnity(S+D-S*D);
      (*pixel)=(double) QuantumRange*RoundToUnity(Sa*Da*blend+Sca*(1.0-Da)+
        Dca*(1.0-Sa));
      break;
    }
    case SoftBurnCompositeOp:
    {
      blend=GetSoftBurnBlend(Sca,Dca);
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(blend);
      break;
    }
    case SoftDodgeCompositeOp:
    {
      blend=GetSoftDodgeBlend(Sca,Dca);
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(blend);
      break;
    }
    case SoftLightCompositeOp:
    {
      if (RoundToUnity(Sca) <= 0.5)
        {
          (*pixel)=(double) QuantumRange*gamma*(RoundToUnity(Dca)*Sa+Da*
           (RoundToUnity(Dca)-(1.0-2.0*RoundToUnity(Sca))*
           RoundToUnity(Dca)*(1.0-RoundToUnity(Dca)))+RoundToUnity(Sca)*
           (1.0-Da)+RoundToUnity(Dca)*(1.0-Sa));
          break;
        }
      if (RoundToUnity(Dca) > 0.25)
        blend=sqrt(RoundToUnity(Dca));
      else
        blend=((16.0*RoundToUnity(Dca)-12.0)*RoundToUnity(Dca)+4.0)*
          RoundToUnity(Dca);
      (*pixel)=(double) QuantumRange*gamma*(RoundToUnity(Dca)*Sa+Da*
        (RoundToUnity(Dca)+(2.0*RoundToUnity(Sca)-1.0)*
        (RoundToUnity(blend)-RoundToUnity(Dca)))+RoundToUnity(Sca)*
        (1.0-Da)+RoundToUnity(Dca)*(1.0-Sa));
      break;
    }
    case StampCompositeOp:
    {
      (*pixel)=(double) QuantumRange*RoundToUnity(Sca+Dca-1.0);
      break;
    }
    case StereoCompositeOp:
    {
      if (channel == RedPixelChannel)
        (*pixel)=(MagickRealType) GetPixelRed(source_image,p);
      break;
    }
    case ThresholdCompositeOp:
    {
      S=(Sa > 0.0) ? (Sca/Sa) : 0.0;
      D=(Da > 0.0) ? (Dca/Da) : 0.0;
      if (fabs(2.0*(S-D)) < threshold)
        blend=D;
      else
        blend=D+(S-D)*amount;
      (*pixel)=(double) QuantumRange*(Sa*Da*RoundToUnity(blend)+Sca*
        (1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case VividLightCompositeOp:
    {
      if ((fabs((double) Sa) < MagickEpsilon) ||
          (fabs((double) Da) < MagickEpsilon))
        {
          (*pixel)=(double) QuantumRange*gamma*(Sa*Da+Sca*(1.0-Da)+Dca*
            (1.0-Sa));
          break;
        }
      blend=GetVividLightBlend(Sa,Sca,Da,Dca);
      (*pixel)=(double) QuantumRange*gamma*(Sa*Da*RoundToUnity(blend)+Sca*
        (1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case XorCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(Sca*(1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    default:
    {
      (*pixel)=Sc;
      break;
    }
  }
}

typedef struct _CompositeChannelTail
{
  const CompositeRowInputs
    *row;

  const CompositePixelInputs
    *authentic;

  Quantum
    *magick_restrict q;

  ssize_t
    i;

  PixelChannel
    channel;

  PixelTrait
    traits,
    source_traits;
} CompositeChannelTail;

static void CompositeSourceChannel(const CompositeChannelTail *tail)
{
  const CompositeRowInputs
    *const row = tail->row;

  const CompositePixelInputs
    *const authentic = tail->authentic;

  Quantum
    *magick_restrict q = tail->q;

  const ssize_t
    i = tail->i;

  const PixelChannel
    channel = tail->channel;

  const PixelTrait
    source_traits = tail->source_traits,
    traits = tail->traits;

  Image
    *const source_image = row->source_image;

  const CompositeOperator
    compose = row->compose;

  const MagickBooleanType
    clamp = row->clamp;

  const Quantum
    *const p = authentic->p;

  const MagickRealType
    alpha = authentic->alpha,
    Da = authentic->Da,
    Sa = authentic->Sa;

  double
    gamma = 0.0;

  MagickRealType
    Dc = 0.0,
    Dca = 0.0,
    Sc = 0.0,
    Sca = 0.0;

  CompositeChannelInputs
    tile;

  MagickRealType
    pixel = 0.0;

  if (source_traits == UndefinedPixelTrait)
    return;
  /*
    Sc: source color.
    Dc: canvas color.
  */
  Sc=(MagickRealType) GetPixelChannel(source_image,channel,p);
  Dc=(MagickRealType) q[i];
  if ((traits & CopyPixelTrait) != 0)
    {
      /*
        Copy channel.
      */
      q[i]=ClampToQuantum(Dc);
      return;
    }
  /*
    Porter-Duff compositions:
      Sca: source normalized color multiplied by alpha.
      Dca: normalized canvas color multiplied by alpha.
  */
  Sca=QuantumScale*Sa*Sc;
  Dca=QuantumScale*Da*Dc;
  gamma=GetCompositeGamma(compose,alpha);
  pixel=Dc;
  tile.channel=channel;
  tile.Dc=Dc;
  tile.Dca=Dca;
  tile.Sc=Sc;
  tile.Sca=Sca;
  tile.gamma=gamma;
  CompositeChannelColor(row,authentic,&tile,&pixel);
  q[i]=clamp != MagickFalse ? ClampPixel(pixel) : ClampToQuantum(pixel);
}

static void CompositeImageChannels(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,Quantum *magick_restrict q)
{
  Image
    *const image = row->image,
    *const source_image = row->source_image;

  const MagickBooleanType
    clamp = row->clamp;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    CompositeChannelTail
      tail;

    MagickRealType
      pixel = 0.0;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait source_traits = GetPixelChannelTraits(source_image,channel);
    if (traits == UndefinedPixelTrait)
      continue;
    if ((channel == AlphaPixelChannel) &&
        ((traits & UpdatePixelTrait) != 0))
      {
        /*
          Set alpha channel.
        */
        CompositeAlphaChannel(row,authentic,q,&pixel);
        q[i]=clamp != MagickFalse ? ClampPixel(pixel) :
          ClampToQuantum(pixel);
        continue;
      }
    tail.row=row;
    tail.authentic=authentic;
    tail.q=q;
    tail.i=i;
    tail.channel=channel;
    tail.traits=traits;
    tail.source_traits=source_traits;
    CompositeSourceChannel(&tail);
  }
}

static const CompositeOperator
  PixelInfoInputOperators[] =
  {
    ColorizeCompositeOp,
    HueCompositeOp,
    LuminizeCompositeOp,
    ModulateCompositeOp,
    RMSECompositeOp,
    SaturateCompositeOp
  };

static const CompositeOperator
  IntensityInputOperators[] =
  {
    BumpmapCompositeOp,
    CopyAlphaCompositeOp,
    DarkenIntensityCompositeOp,
    LightenIntensityCompositeOp
  };

static void CompositeImagePixel(const CompositeRowInputs *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  Image
    *const image = row->image,
    *const source_image = row->source_image;

  CompositePixelInputs
    authentic;

  MagickRealType
    alpha = 0.0,
    Da = 0.0,
    Di = 0.0,
    Sa = 0.0,
    Si = 0.0;

  PixelInfo
    canvas_pixel,
    source_pixel;

  /*
    Authentic composite:
      Sa:  normalized source alpha.
      Da:  normalized canvas alpha.
  */
  Sa=QuantumScale*(double) GetPixelAlpha(source_image,p);
  Da=QuantumScale*(double) GetPixelAlpha(image,q);
  SetCompositeCoverage(row,Sa,Da,&alpha);
  if (row->pixel_info_inputs != MagickFalse)
    {
        Si=GetPixelIntensity(source_image,p);
        GetPixelInfoPixel(source_image,p,&source_pixel);
        GetPixelInfoPixel(image,q,&canvas_pixel);
    }
  else if (row->intensity_inputs != MagickFalse)
    {
        Si=GetPixelIntensity(source_image,p);
        Di=GetPixelIntensity(image,q);
    }
  authentic.p=p;
  authentic.alpha=alpha;
  authentic.Da=Da;
  authentic.Di=Di;
  authentic.Sa=Sa;
  authentic.Si=Si;
  authentic.canvas_pixel=(&canvas_pixel);
  authentic.source_pixel=(&source_pixel);
  CompositeImageChannels(row,&authentic,q);
}

static void CompositeImageRow(const CompositeRowInputs *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  Image
    *const image = row->image,
    *const source_image = row->source_image;

  const CompositeOperator
    compose = row->compose;

  const MagickBooleanType
    clip_to_self = row->clip_to_self;

  const ssize_t
    x_offset = row->x_offset,
    y_offset = row->y_offset,
    y = row->y;

  ExceptionInfo
    *const exception = row->exception;

  const Quantum
    *const pixels = row->pixels;

  const MagickRealType
    canvas_dissolve = row->canvas_dissolve;

  const MagickBooleanType
    clamp = row->clamp;

  const VirtualCompositeInputs
    virtual_inputs = { image, source_image, compose, canvas_dissolve, clamp,
      IsListedOperator(CanvasPixelOperators,
        sizeof(CanvasPixelOperators)/sizeof(*CanvasPixelOperators),compose) };

  PixelInfo
    canvas_pixel,
    source_pixel;

  ssize_t
    x;

  GetPixelInfo(image,&canvas_pixel);
  GetPixelInfo(source_image,&source_pixel);
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (clip_to_self != MagickFalse)
      {
        if (x < x_offset)
          {
            q+=(ptrdiff_t) GetPixelChannels(image);
            continue;
          }
        if ((x-x_offset) >= (ssize_t) source_image->columns)
          break;
      }
    if (IsOutsideSourceColumn(pixels,x,x_offset,source_image) != MagickFalse)
      {
        Quantum
          source[MaxPixelChannels];

        /*
          Virtual composite:
            Sc: source color.
            Dc: canvas color.
        */
        (void) GetOneVirtualPixel(source_image,
          CastDoubleToSsizeT((double) x-x_offset),
          CastDoubleToSsizeT((double) y-y_offset),source,exception);
        SetVirtualCompositeChannels(&virtual_inputs,source,q);
        q+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    CompositeImagePixel(row,p,q);
    p=AdvanceOverSourcePixel(source_image,pixels,p);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

MagickExport MagickBooleanType CompositeImage(Image *image,
  const Image *composite,const CompositeOperator compose,
  const MagickBooleanType clip_to_self,const ssize_t x_offset,
  const ssize_t y_offset,ExceptionInfo *exception)
{
#define CompositeImageTag  "Composite/Image"

  CacheView
    *source_view,
    *image_view;

  ColorspaceType
    colorspace = HCLColorspace;

  CompositeSetup
    setup;

  const char
    *artifact;

  double
    white_luminance = 10000.0;

  GeometryInfo
    geometry_info;

  IlluminantType
    illuminant = D65Illuminant;

  Image
    *canvas_image,
    *source_image;

  MagickBooleanType
    clamp,
    compose_sync,
    status;

  MagickOffsetType
    progress;

  MagickRealType
    amount,
    canvas_dissolve,
    midpoint,
    percent_luma,
    percent_chroma,
    source_dissolve,
    threshold;

  MagickStatusType
    flags;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(composite != (Image *) NULL);
  assert(composite->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  source_image=CloneImage(composite,0,0,MagickTrue,exception);
  if (source_image == (const Image *) NULL)
    return(MagickFalse);
  (void) SetImageColorspace(source_image,image->colorspace,exception);
  if ((compose == OverCompositeOp) || (compose == SrcOverCompositeOp))
    {
      const CompositePlacement
        placement = { clip_to_self, x_offset, y_offset };

      status=CompositeOverImage(image,source_image,&placement,exception);
      source_image=DestroyImage(source_image);
      return(status);
    }
  amount=0.5;
  canvas_image=(Image *) NULL;
  canvas_dissolve=1.0;
  white_luminance=10000.0;
  artifact=GetImageArtifact(image,"compose:white-luminance");
  if (artifact != (const char *) NULL)
    white_luminance=StringToDouble(artifact,(char **) NULL);
  artifact=GetImageArtifact(image,"compose:illuminant");
  if (artifact != (const char *) NULL)
    illuminant=ParseComposeIlluminant(artifact);
  artifact=GetImageArtifact(image,"compose:colorspace");
  if (artifact != (const char *) NULL)
    colorspace=ParseComposeColorspace(artifact);
  clamp=MagickTrue;
  artifact=GetImageArtifact(image,"compose:clamp");
  if (artifact != (const char *) NULL)
    clamp=IsStringTrue(artifact);
  compose_sync=MagickTrue;
  artifact=GetImageArtifact(image,"compose:sync");
  if (artifact != (const char *) NULL)
    compose_sync=IsStringTrue(artifact);
  SetGeometryInfo(&geometry_info);
  percent_luma=100.0;
  percent_chroma=100.0;
  source_dissolve=1.0;
  threshold=0.05f;
  setup.x_offset=x_offset;
  setup.y_offset=y_offset;
  switch (compose)
  {
    case CopyCompositeOp:
    {
      if ((x_offset < 0) || (y_offset < 0))
        break;
      if ((x_offset+(ssize_t) source_image->columns) > (ssize_t) image->columns)
        break;
      if ((y_offset+(ssize_t) source_image->rows) > (ssize_t) image->rows)
        break;
      if ((source_image->alpha_trait == UndefinedPixelTrait) &&
          (image->alpha_trait != UndefinedPixelTrait))
        (void) SetImageAlphaChannel(source_image,OpaqueAlphaChannel,exception);
      return(CompositeCopyImage(image,source_image,&setup,exception));
    }
    case IntensityCompositeOp:
    {
      if ((x_offset < 0) || (y_offset < 0))
        break;
      if ((x_offset+(ssize_t) source_image->columns) > (ssize_t) image->columns)
        break;
      if ((y_offset+(ssize_t) source_image->rows) > (ssize_t) image->rows)
        break;
      status=MagickTrue;
      source_view=AcquireVirtualCacheView(source_image,exception);
      image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(source_image,image,source_image->rows,4)
#endif
      for (y=0; y < (ssize_t) source_image->rows; y++)
      {
        const Quantum
          *p;

        Quantum
          *q;

        IntensityRowInputs
          row;

        if (status == MagickFalse)
          continue;
        p=GetCacheViewVirtualPixels(source_view,0,y,source_image->columns,1,
          exception);
        q=GetCacheViewAuthenticPixels(image_view,x_offset,y+y_offset,
          source_image->columns,1,exception);
        if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
          {
            status=MagickFalse;
            continue;
          }
        row.image=image;
        row.source_image=source_image;
        row.clamp=clamp;
        SetIntensityAlphaRow(&row,p,q);
        SyncCompositeRow(image_view,exception,&status);
        if (image->progress_monitor != (MagickProgressMonitor) NULL)
          {
            SetCompositeProgress(image,CompositeImageTag,(MagickOffsetType) y,
              &status);
          }
      }
      source_view=DestroyCacheView(source_view);
      image_view=DestroyCacheView(image_view);
      source_image=DestroyImage(source_image);
      return(status);
    }
    case CopyAlphaCompositeOp:
    case ChangeMaskCompositeOp:
    {
      /*
        Modify canvas outside the overlaid region and require an alpha
        channel to exist, to add transparency.
      */
      if ((image->alpha_trait & BlendPixelTrait) == 0)
        (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
      break;
    }
    case BlurCompositeOp:
    {
      CacheView
        *canvas_view;

      double
        angle_range,
        angle_start,
        height,
        width;

      PixelInfo
        pixel;

      ResampleFilter
        *resample_filter;

      SegmentInfo
        blur;

      BlurComposite
        blur_composite;

      /*
        Blur Image by resampling dictated by an overlay gradient map:
          X = red_channel; Y = green_channel; compose:args =
          x_scale[,y_scale[,angle]].
      */
      canvas_image=CloneImage(image,0,0,MagickTrue,exception);
      if (canvas_image == (Image *) NULL)
        {
          source_image=DestroyImage(source_image);
          return(MagickFalse);
        }
      /*
        Gather the maximum blur sigma values from user.
      */
      flags=NoValue;
      artifact=GetImageArtifact(image,"compose:args");
      if (artifact != (const char *) NULL)
        flags=ParseGeometry(artifact,&geometry_info);
      if ((flags & WidthValue) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "InvalidSetting","'%s' '%s'","compose:args",artifact);
          source_image=DestroyImage(source_image);
          canvas_image=DestroyImage(canvas_image);
          return(MagickFalse);
        }
      /*
        Users input sigma now needs to be converted to the EWA ellipse size.
        The filter defaults to a sigma of 0.5 so to make this match the users
        input the ellipse size needs to be doubled.
      */
      width=2.0*geometry_info.rho;
      height=width;
      if ((flags & HeightValue) != 0)
        height=2.0*geometry_info.sigma;
      /*
        Default the unrotated ellipse width and height axis vectors.
      */
      blur.x1=width;
      blur.x2=0.0;
      blur.y1=0.0;
      blur.y2=height;
      if ((flags & XValue) != 0 )
        {
          MagickRealType
            angle;

          /*
            Rotate vectors if a rotation angle is given.
          */
          angle=DegreesToRadians(geometry_info.xi);
          blur.x1=width*cos(angle);
          blur.x2=width*sin(angle);
          blur.y1=(-height*sin(angle));
          blur.y2=height*cos(angle);
        }
      angle_start=0.0;
      angle_range=0.0;
      if ((flags & YValue) != 0 )
        {
          /*
            Lets set a angle range and calculate in the loop.
          */
          angle_start=DegreesToRadians(geometry_info.xi);
          angle_range=DegreesToRadians(geometry_info.psi)-angle_start;
        }
      /*
        Set up a gaussian cylindrical filter for EWA Blurring.

        As the minimum ellipse radius of support*1.0 the EWA algorithm
        can only produce a minimum blur of 0.5 for Gaussian (support=2.0)
        This means that even 'No Blur' will be still a little blurry! The
        solution (as well as the problem of preventing any user expert filter
        settings, is to set our own user settings, restore them afterwards.
      */
      resample_filter=AcquireResampleFilter(image,exception);
      SetResampleFilter(resample_filter,GaussianFilter);
      /*
        Perform the variable blurring of each pixel in image.
      */
      GetPixelInfo(image,&pixel);
      source_view=AcquireVirtualCacheView(source_image,exception);
      canvas_view=AcquireAuthenticCacheView(canvas_image,exception);
      blur_composite.angle_range=angle_range;
      blur_composite.angle_start=angle_start;
      blur_composite.height=height;
      blur_composite.width=width;
      blur_composite.canvas_image=canvas_image;
      blur_composite.image=image;
      blur_composite.source_image=source_image;
      blur_composite.canvas_view=canvas_view;
      blur_composite.source_view=source_view;
      blur_composite.resample_filter=resample_filter;
      blur_composite.x_offset=x_offset;
      blur_composite.y_offset=y_offset;
      blur_composite.exception=exception;
      BlurCompositeRows(&blur_composite,&blur,&pixel);
      resample_filter=DestroyResampleFilter(resample_filter);
      source_view=DestroyCacheView(source_view);
      canvas_view=DestroyCacheView(canvas_view);
      source_image=DestroyImage(source_image);
      source_image=canvas_image;
      break;
    }
    case DisplaceCompositeOp:
    case DistortCompositeOp:
    {
      CacheView
        *canvas_view;

      MagickRealType
        horizontal_scale,
        vertical_scale;

      PixelInfo
        pixel;

      PointInfo
        center;

      DisplaceSetup
        displace;

      /*
        Displace/Distort based on overlay gradient map:
          X = red_channel;  Y = green_channel;
          compose:args = x_scale[,y_scale[,center.x,center.y]]
      */
      canvas_image=CloneImage(image,0,0,MagickTrue,exception);
      if (canvas_image == (Image *) NULL)
        {
          source_image=DestroyImage(source_image);
          return(MagickFalse);
        }
      SetGeometryInfo(&geometry_info);
      flags=NoValue;
      artifact=GetImageArtifact(image,"compose:args");
      if (artifact != (char *) NULL)
        flags=ParseGeometry(artifact,&geometry_info);
      displace.compose=compose;
      displace.flags=flags;
      displace.geometry_info=geometry_info;
      displace.image=image;
      displace.source_image=source_image;
      displace.x_offset=x_offset;
      displace.y_offset=y_offset;
      if ((flags & (WidthValue | HeightValue)) == 0 )
        SetDisplaceScaleDefaults(&displace,&horizontal_scale,&vertical_scale);
      else
        SetDisplaceScaleArguments(&displace,&horizontal_scale,&vertical_scale);
      /*
        Determine fixed center point for absolute distortion map
         Absolute distort ==
           Displace offset relative to a fixed absolute point
           Select that point according to +X+Y user inputs.
           default = center of overlay image
           arg flag '!' = locations/percentage relative to background image
      */
      center.x=(MagickRealType) x_offset;
      center.y=(MagickRealType) y_offset;
      SetDistortCenter(&displace,&center);
      /*
        Shift the pixel offset point as defined by the provided,
        displacement/distortion map.  -- Like a lens...
      */
      GetPixelInfo(image,&pixel);
      image_view=AcquireVirtualCacheView(image,exception);
      source_view=AcquireVirtualCacheView(source_image,exception);
      canvas_view=AcquireAuthenticCacheView(canvas_image,exception);
      for (y=0; y < (ssize_t) source_image->rows; y++)
      {
        MagickBooleanType
          sync;

        const Quantum
          *magick_restrict p;

        Quantum
          *magick_restrict q;

        DisplaceColumns
          columns;

        if (((y+y_offset) < 0) || ((y+y_offset) >= (ssize_t) image->rows))
          continue;
        p=GetCacheViewVirtualPixels(source_view,0,y,source_image->columns,1,
          exception);
        q=QueueCacheViewAuthenticPixels(canvas_view,0,y,canvas_image->columns,1,
          exception);
        if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
          break;
        columns.canvas_image=canvas_image;
        columns.image=image;
        columns.source_image=source_image;
        columns.image_view=image_view;
        columns.center=center;
        columns.compose=compose;
        columns.horizontal_scale=horizontal_scale;
        columns.vertical_scale=vertical_scale;
        columns.x_offset=x_offset;
        columns.y=y;
        columns.exception=exception;
        if (DisplaceCompositeColumns(&columns,p,q,&pixel) == MagickFalse)
          break;
        sync=SyncCacheViewAuthenticPixels(canvas_view,exception);
        if (sync == MagickFalse)
          break;
      }
      canvas_view=DestroyCacheView(canvas_view);
      source_view=DestroyCacheView(source_view);
      image_view=DestroyCacheView(image_view);
      source_image=DestroyImage(source_image);
      source_image=canvas_image;
      break;
    }
    case DissolveCompositeOp:
    {
      /*
        Geometry arguments to dissolve factors.
      */
      artifact=GetImageArtifact(image,"compose:args");
      if (artifact != (char *) NULL)
        {
          flags=ParseGeometry(artifact,&geometry_info);
          SetDissolveFactors(geometry_info,flags,&source_dissolve,
            &canvas_dissolve);
        }
      break;
    }
    case BlendCompositeOp:
    {
      artifact=GetImageArtifact(image,"compose:args");
      if (artifact != (char *) NULL)
        {
          flags=ParseGeometry(artifact,&geometry_info);
          SetBlendFactors(geometry_info,flags,&source_dissolve,
            &canvas_dissolve);
        }
      break;
    }
    case SaliencyBlendCompositeOp:
    {
      double
        residual_threshold = 0.0002,
        iterations = 400.0;

      BlendRequest
        request;

      size_t
        tick = 100;

      GetBlendIterations(image,&iterations,&residual_threshold,&tick);
      request.x_offset=x_offset;
      request.y_offset=y_offset;
      request.iterations=iterations;
      request.residual_threshold=residual_threshold;
      request.tick=tick;
      status=SaliencyBlendImage(image,composite,&request,exception);
      source_image=DestroyImage(source_image);
      return(status);
    }
    case SeamlessBlendCompositeOp:
    {
      double
        residual_threshold = 0.0002,
        iterations = 400.0;

      BlendRequest
        request;

      size_t
        tick = 100;

      GetBlendIterations(image,&iterations,&residual_threshold,&tick);
      request.x_offset=x_offset;
      request.y_offset=y_offset;
      request.iterations=iterations;
      request.residual_threshold=residual_threshold;
      request.tick=tick;
      status=SeamlessBlendImage(image,composite,&request,exception);
      source_image=DestroyImage(source_image);
      return(status);
    }
    case MathematicsCompositeOp:
    {
      /*
        Just collect the values from "compose:args", setting.
        Unused values are set to zero automagically.

        Arguments are normally a comma separated list, so this probably should
        be changed to some 'general comma list' parser, (with a minimum
        number of values)
      */
      GetMathematicsArguments(image,&geometry_info,exception);
      break;
    }
    case ModulateCompositeOp:
    {
      /*
        Determine the luma and chroma scale.
      */
      GetModulateArguments(image,&geometry_info,&percent_luma,&percent_chroma);
      break;
    }
    case ThresholdCompositeOp:
    {
      /*
        Determine the amount and threshold.
      */
      GetThresholdArguments(image,&geometry_info,&amount,&threshold);
      break;
    }
    default:
      break;
  }
  /*
    Composite image.
  */
  status=MagickTrue;
  progress=0;
  midpoint=((MagickRealType) QuantumRange+1.0)/2;
  source_view=AcquireVirtualCacheView(source_image,exception);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(source_image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p,
      *pixels;

    Quantum
      *magick_restrict q;

    CompositeRowInputs
      row;

    const OverSourceRow
      source = { source_view, source_image, clip_to_self, x_offset, y_offset,
        y, exception };

    if (SkipOverRow(&source,&status) == MagickFalse)
      continue;
    if (FetchOverSourceRow(&source,&pixels,&p,&status) == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    row.image=image;
    row.source_image=source_image;
    row.compose=compose;
    row.union_coverage=IsListedOperator(UnionCoverageOperators,
      sizeof(UnionCoverageOperators)/sizeof(*UnionCoverageOperators),compose);
    row.canvas_alpha=IsListedOperator(CanvasAlphaOperators,
      sizeof(CanvasAlphaOperators)/sizeof(*CanvasAlphaOperators),compose);
    row.source_alpha=IsListedOperator(SourceAlphaOperators,
      sizeof(SourceAlphaOperators)/sizeof(*SourceAlphaOperators),compose);
    row.pixel_info_inputs=IsListedOperator(PixelInfoInputOperators,
      sizeof(PixelInfoInputOperators)/sizeof(*PixelInfoInputOperators),compose);
    row.intensity_inputs=IsListedOperator(IntensityInputOperators,
      sizeof(IntensityInputOperators)/sizeof(*IntensityInputOperators),compose);
    row.clip_to_self=clip_to_self;
    row.x_offset=x_offset;
    row.y_offset=y_offset;
    row.y=y;
    row.exception=exception;
    row.pixels=pixels;
    row.amount=amount;
    row.canvas_dissolve=canvas_dissolve;
    row.midpoint=midpoint;
    row.percent_luma=percent_luma;
    row.percent_chroma=percent_chroma;
    row.source_dissolve=source_dissolve;
    row.threshold=threshold;
    row.clamp=clamp;
    row.compose_sync=compose_sync;
    row.colorspace=colorspace;
    row.white_luminance=white_luminance;
    row.illuminant=illuminant;
    row.geometry_info=geometry_info;
    CompositeImageRow(&row,p,q);
    SyncCompositeRow(image_view,exception,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetCompositeProgress(image,CompositeImageTag,progress,&status);
      }
  }
  source_view=DestroyCacheView(source_view);
  image_view=DestroyCacheView(image_view);
  if (canvas_image != (Image * ) NULL)
    canvas_image=DestroyImage(canvas_image);
  else
    source_image=DestroyImage(source_image);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     T e x t u r e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TextureImage() repeatedly tiles the texture image across and down the image
%  canvas.
%
%  The format of the TextureImage method is:
%
%      MagickBooleanType TextureImage(Image *image,const Image *texture,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o texture_image: This image is the texture to layer on the background.
%
*/
static inline MagickBooleanType IsTextureComposited(const Image *image,
  const Image *texture_image)
{
  return(((image->compose != CopyCompositeOp) &&
    ((image->compose != OverCompositeOp) ||
    (image->alpha_trait != UndefinedPixelTrait) ||
    (texture_image->alpha_trait != UndefinedPixelTrait))) ? MagickTrue :
    MagickFalse);
}

typedef struct _TextureSpan
{
  Image
    *image,
    *texture_image;

  size_t
    width;
} TextureSpan;

static Quantum *CopyTextureSpan(const TextureSpan *span,const Quantum *p,
  Quantum *q)
{
  Image
    *image = span->image,
    *texture_image = span->texture_image;

  const size_t
    width = span->width;

  ssize_t
    j;

  for (j=0; j < (ssize_t) width; j++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(texture_image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(texture_image,i);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      PixelTrait texture_traits=GetPixelChannelTraits(texture_image,
        channel);
      if ((traits == UndefinedPixelTrait) ||
          (texture_traits == UndefinedPixelTrait))
        continue;
      SetPixelChannel(image,channel,p[i],q);
    }
    p+=(ptrdiff_t) GetPixelChannels(texture_image);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

#define TextureImageTag  "Texture/Image"

static MagickBooleanType ReportTextureProgress(Image *image,const ssize_t y)
{
  MagickBooleanType
    status;

  status=MagickTrue;
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(image,TextureImageTag,(MagickOffsetType) y,
        image->rows);
      if (proceed == MagickFalse)
        status=MagickFalse;
    }
  return(status);
}

typedef struct _TextureTiles
{
  Image
    *image,
    *texture_image;

  ssize_t
    y;
} TextureTiles;

static MagickBooleanType CompositeTextureRow(const TextureTiles *tiles,
  MagickBooleanType status,ExceptionInfo *exception)
{
  Image
    *const image = tiles->image,
    *const texture_image = tiles->texture_image;

  const ssize_t
    y = tiles->y;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x+=(ssize_t) texture_image->columns)
  {
    MagickBooleanType
      thread_status;

    thread_status=CompositeImage(image,texture_image,image->compose,
      MagickTrue,x+texture_image->tile_offset.x,y+
      texture_image->tile_offset.y,exception);
    if (thread_status == MagickFalse)
      {
        status=thread_status;
        break;
      }
  }
  return(status);
}

static MagickBooleanType TileTextureImage(Image *image,Image *texture_image,
  MagickBooleanType status,ExceptionInfo *exception)
{
  ssize_t
    y;

  /*
    Tile texture onto the image background.
  */
  for (y=0; y < (ssize_t) image->rows; y+=(ssize_t) texture_image->rows)
  {
    TextureTiles
      tiles;

    if (status == MagickFalse)
      continue;
    tiles.image=image;
    tiles.texture_image=texture_image;
    tiles.y=y;
    status=CompositeTextureRow(&tiles,status,exception);
    if (ReportTextureProgress(image,y) == MagickFalse)
      status=MagickFalse;
  }
  (void) SetImageProgress(image,TextureImageTag,(MagickOffsetType)
    image->rows,image->rows);
  texture_image=DestroyImage(texture_image);
  return(status);
}

static void CopyTextureRow(Image *image,Image *texture_image,
  const Quantum *pixels,Quantum *q)
{
  const Quantum
    *p;

  ssize_t
    x;

  size_t
    width;

  for (x=0; x < (ssize_t) image->columns; x+=(ssize_t) texture_image->columns)
  {
    TextureSpan
      span;

    p=pixels;
    width=texture_image->columns;
    if ((x+(ssize_t) width) > (ssize_t) image->columns)
      width=image->columns-(size_t) x;
    span.image=image;
    span.texture_image=texture_image;
    span.width=width;
    q=CopyTextureSpan(&span,p,q);
  }
}

static MagickBooleanType CopyTextureImage(Image *image,Image *texture_image,
  ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *texture_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  /*
    Tile texture onto the image background (optimized).
  */
  status=MagickTrue;
  texture_view=AcquireVirtualCacheView(texture_image,exception);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(texture_image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *pixels;

    Quantum
      *q;

    if (status == MagickFalse)
      continue;
    pixels=GetCacheViewVirtualPixels(texture_view,texture_image->tile_offset.x,
      (y+texture_image->tile_offset.y) % (ssize_t) texture_image->rows,
      texture_image->columns,1,exception);
    q=QueueCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if ((pixels == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    CopyTextureRow(image,texture_image,pixels,q);
    SyncCompositeRow(image_view,exception,&status);
    if (ReportTextureProgress(image,y) == MagickFalse)
      status=MagickFalse;
  }
  texture_view=DestroyCacheView(texture_view);
  image_view=DestroyCacheView(image_view);
  texture_image=DestroyImage(texture_image);
  return(status);
}

MagickExport MagickBooleanType TextureImage(Image *image,const Image *texture,
  ExceptionInfo *exception)
{
  Image
    *texture_image;

  MagickBooleanType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (texture == (const Image *) NULL)
    return(MagickFalse);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  texture_image=CloneImage(texture,0,0,MagickTrue,exception);
  if (texture_image == (const Image *) NULL)
    return(MagickFalse);
  (void) TransformImageColorspace(texture_image,image->colorspace,exception);
  (void) SetImageVirtualPixelMethod(texture_image,TileVirtualPixelMethod,
    exception);
  status=MagickTrue;
  if (IsTextureComposited(image,texture_image) != MagickFalse)
    return(TileTextureImage(image,texture_image,status,exception));
  return(CopyTextureImage(image,texture_image,exception));
}
