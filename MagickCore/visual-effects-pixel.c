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
%                      PPPP   IIIII  X   X  EEEEE  L                          %
%                      P   P    I     X X   E      L                          %
%                      PPPP     I      X    EEE    L                          %
%                      P        I     X X   E      L                          %
%                      P      IIIII  X   X  EEEEE  LLLLL                      %
%                                                                             %
%                                                                             %
%                   MagickCore Pixel Visual Effects Methods                   %
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

static Image *ToneSketchDodge(Image *dodge_image,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=ClampImage(dodge_image,exception);
  if (status != MagickFalse)
    status=NormalizeImage(dodge_image,exception);
  if (status != MagickFalse)
    status=NegateImage(dodge_image,MagickFalse,exception);
  if (status != MagickFalse)
    status=TransformImage(&dodge_image,(char *) NULL,"50%",exception);
  return(dodge_image);
}

static void BlendSketchOverlay(Image *sketch_image,Image *blend_image,
  ExceptionInfo *exception)
{
  if (blend_image->alpha_trait != BlendPixelTrait)
    (void) SetImageAlpha(blend_image,TransparentAlpha,exception);
  (void) SetImageArtifact(blend_image,"compose:args","20x80");
  (void) CompositeImage(sketch_image,blend_image,BlendCompositeOp,MagickTrue,
    0,0,exception);
  blend_image=DestroyImage(blend_image);
}

static Image *BlendSketchImage(const Image *image,Image *blur_image,
  const double radius,ExceptionInfo *exception)
{
  Image
    *blend_image,
    *dodge_image,
    *sketch_image;

  dodge_image=EdgeImage(blur_image,radius,exception);
  blur_image=DestroyImage(blur_image);
  if (dodge_image == (Image *) NULL)
    return((Image *) NULL);
  dodge_image=ToneSketchDodge(dodge_image,exception);
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
  BlendSketchOverlay(sketch_image,blend_image,exception);
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

