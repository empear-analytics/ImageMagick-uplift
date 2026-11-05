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
%               SSSSS  TTTTT  EEEEE   GGGG   AAA   N   N   OOO                %
%               SS       T    E      G      A   A  NN  N  O   O               %
%                SSS     T    EEE    G  GG  AAAAA  N N N  O   O               %
%                  SS    T    E      G   G  A   A  N  NN  O   O               %
%               SSSSS    T    EEEEE   GGG   A   A  N   N   OOO                %
%                                                                             %
%                                                                             %
%                 MagickCore Steganography and Stereo Methods                 %
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
%   S t e g a n o I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SteganoImage() hides a digital watermark within the image.  Recover
%  the hidden watermark later to prove that the authenticity of an image.
%  Offset defines the start position within the image to hide the watermark.
%
%  The format of the SteganoImage method is:
%
%      Image *SteganoImage(const Image *image,Image *watermark,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o watermark: the watermark image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
#define GetBit(alpha,i) ((((size_t) (alpha) >> (size_t) (i)) & 0x01) != 0)
#define SetBit(alpha,i,set) (Quantum) ((set) != 0 ? (size_t) (alpha) \
  | (one << (size_t) (i)) : (size_t) (alpha) & ~(one << (size_t) (i)))
#define SteganoImageTag  "Stegano/Image"

static void AdvanceSteganoCursor(const Image *stegano_image,int *c,
  ssize_t *k,ssize_t *j)
{
  (*c)++;
  if (*c == 3)
    *c=0;
  (*k)++;
  if (*k == (ssize_t) (stegano_image->columns*stegano_image->columns))
    *k=0;
  if (*k == stegano_image->offset)
    (*j)++;
}

typedef struct _SteganoBit
{
  Image
    *stegano_image;

  int
    c;

  size_t
    one;

  ssize_t
    i,
    j;
} SteganoBit;

static void SetSteganoBit(const SteganoBit *stegano_bit,const PixelInfo pixel,
  Quantum *q)
{
  Image
    *const stegano_image = stegano_bit->stegano_image;

  const int
    c = stegano_bit->c;

  const size_t
    one = stegano_bit->one;

  const ssize_t
    i = stegano_bit->i,
    j = stegano_bit->j;

  switch (c)
  {
    case 0:
    {
      SetPixelRed(stegano_image,SetBit(GetPixelRed(stegano_image,q),j,
        GetBit(GetPixelInfoIntensity(stegano_image,&pixel),i)),q);
      break;
    }
    case 1:
    {
      SetPixelGreen(stegano_image,SetBit(GetPixelGreen(stegano_image,q),j,
        GetBit(GetPixelInfoIntensity(stegano_image,&pixel),i)),q);
      break;
    }
    case 2:
    {
      SetPixelBlue(stegano_image,SetBit(GetPixelBlue(stegano_image,q),j,
        GetBit(GetPixelInfoIntensity(stegano_image,&pixel),i)),q);
      break;
    }
  }
}

typedef struct _SteganoRow
{
  const Image
    *watermark;

  CacheView
    *watermark_view;

  Image
    *stegano_image;

  CacheView
    *stegano_view;

  size_t
    depth,
    one;

  ssize_t
    i,
    y;

  ExceptionInfo
    *exception;
} SteganoRow;

static void HideSteganoRow(const SteganoRow *stegano_row,int *c,ssize_t *k,
  ssize_t *j)
{
  const Image
    *const watermark = stegano_row->watermark;

  CacheView
    *const watermark_view = stegano_row->watermark_view;

  Image
    *const stegano_image = stegano_row->stegano_image;

  CacheView
    *const stegano_view = stegano_row->stegano_view;

  const size_t
    depth = stegano_row->depth,
    one = stegano_row->one;

  const ssize_t
    i = stegano_row->i,
    y = stegano_row->y;

  ExceptionInfo
    *const exception = stegano_row->exception;

  PixelInfo
    pixel;

  Quantum
    *q;

  ssize_t
    x;

  for (x=0; (x < (ssize_t) watermark->columns) && ((*j) < (ssize_t) depth); x++)
  {
    ssize_t
      offset;

    SteganoBit
      stegano_bit;

    (void) GetOneCacheViewVirtualPixelInfo(watermark_view,x,y,&pixel,
      exception);
    offset=(*k)/(ssize_t) stegano_image->columns;
    if (offset >= (ssize_t) stegano_image->rows)
      break;
    q=GetCacheViewAuthenticPixels(stegano_view,(*k) % (ssize_t)
      stegano_image->columns,(*k)/(ssize_t) stegano_image->columns,1,1,
      exception);
    if (q == (Quantum *) NULL)
      break;
    stegano_bit.stegano_image=stegano_image;
    stegano_bit.c=(*c);
    stegano_bit.one=one;
    stegano_bit.i=i;
    stegano_bit.j=(*j);
    SetSteganoBit(&stegano_bit,pixel,q);
    if (SyncCacheViewAuthenticPixels(stegano_view,exception) == MagickFalse)
      break;
    AdvanceSteganoCursor(stegano_image,c,k,j);
  }
}

static MagickBooleanType CloneSteganoImage(const Image *image,
  Image **stegano_image,ExceptionInfo *exception)
{
  *stegano_image=CloneImage(image,0,0,MagickTrue,exception);
  if (*stegano_image == (Image *) NULL)
    return(MagickFalse);
  (*stegano_image)->depth=MAGICKCORE_QUANTUM_DEPTH;
  if (SetImageStorageClass(*stegano_image,DirectClass,exception) == MagickFalse)
    {
      *stegano_image=DestroyImage(*stegano_image);
      return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _SteganoPlane
{
  const Image
    *watermark;

  CacheView
    *watermark_view;

  Image
    *stegano_image;

  CacheView
    *stegano_view;

  size_t
    depth,
    one;

  ssize_t
    i;

  ExceptionInfo
    *exception;
} SteganoPlane;

static void HideSteganoPlane(const SteganoPlane *stegano_plane,int *c,
  ssize_t *k,ssize_t *j)
{
  const Image
    *const watermark = stegano_plane->watermark;

  CacheView
    *const watermark_view = stegano_plane->watermark_view;

  Image
    *const stegano_image = stegano_plane->stegano_image;

  CacheView
    *const stegano_view = stegano_plane->stegano_view;

  const size_t
    depth = stegano_plane->depth,
    one = stegano_plane->one;

  const ssize_t
    i = stegano_plane->i;

  ExceptionInfo
    *const exception = stegano_plane->exception;

  ssize_t
    y;

  for (y=0; (y < (ssize_t) watermark->rows) && ((*j) < (ssize_t) depth); y++)
  {
    SteganoRow
      stegano_row;

    stegano_row.watermark=watermark;
    stegano_row.watermark_view=watermark_view;
    stegano_row.stegano_image=stegano_image;
    stegano_row.stegano_view=stegano_view;
    stegano_row.depth=depth;
    stegano_row.one=one;
    stegano_row.i=i;
    stegano_row.y=y;
    stegano_row.exception=exception;
    HideSteganoRow(&stegano_row,c,k,j);
  }
}

static MagickBooleanType ReportSteganoProgress(const Image *image,
  const size_t depth,const ssize_t i,MagickBooleanType status)
{
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(image,SteganoImageTag,(MagickOffsetType)
        depth-i,depth);
      if (proceed == MagickFalse)
        status=MagickFalse;
    }
  return(status);
}

MagickExport Image *SteganoImage(const Image *image,const Image *watermark,
  ExceptionInfo *exception)
{
  CacheView
    *stegano_view,
    *watermark_view;

  Image
    *stegano_image;

  int
    c;

  MagickBooleanType
    status;

  size_t
    depth,
    one;

  ssize_t
    i,
    j,
    k;

  /*
    Initialize steganographic image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(watermark != (const Image *) NULL);
  assert(watermark->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  one=1UL;
  if (CloneSteganoImage(image,&stegano_image,exception) == MagickFalse)
    return((Image *) NULL);
  /*
    Hide watermark in low-order bits of image.
  */
  c=0;
  i=0;
  j=0;
  depth=stegano_image->depth;
  k=stegano_image->offset;
  status=MagickTrue;
  watermark_view=AcquireVirtualCacheView(watermark,exception);
  stegano_view=AcquireAuthenticCacheView(stegano_image,exception);
  for (i=(ssize_t) depth-1; (i >= 0) && (j < (ssize_t) depth); i--)
  {
    SteganoPlane
      stegano_plane;

    stegano_plane.watermark=watermark;
    stegano_plane.watermark_view=watermark_view;
    stegano_plane.stegano_image=stegano_image;
    stegano_plane.stegano_view=stegano_view;
    stegano_plane.depth=depth;
    stegano_plane.one=one;
    stegano_plane.i=i;
    stegano_plane.exception=exception;
    HideSteganoPlane(&stegano_plane,&c,&k,&j);
    status=ReportSteganoProgress(image,depth,i,status);
  }
  stegano_view=DestroyCacheView(stegano_view);
  watermark_view=DestroyCacheView(watermark_view);
  if (status == MagickFalse)
    stegano_image=DestroyImage(stegano_image);
  return(stegano_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t e r e o A n a g l y p h I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StereoAnaglyphImage() combines two images and produces a single image that
%  is the composite of a left and right image of a stereo pair.  Special
%  red-green stereo glasses are required to view this effect.
%
%  The format of the StereoAnaglyphImage method is:
%
%      Image *StereoImage(const Image *left_image,const Image *right_image,
%        ExceptionInfo *exception)
%      Image *StereoAnaglyphImage(const Image *left_image,
%        const Image *right_image,const ssize_t x_offset,const ssize_t y_offset,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o left_image: the left image.
%
%    o right_image: the right image.
%
%    o exception: return any errors or warnings in this structure.
%
%    o x_offset: amount, in pixels, by which the left image is offset to the
%      right of the right image.
%
%    o y_offset: amount, in pixels, by which the left image is offset to the
%      bottom of the right image.
%
%
*/
MagickExport Image *StereoImage(const Image *left_image,
  const Image *right_image,ExceptionInfo *exception)
{
  return(StereoAnaglyphImage(left_image,right_image,0,0,exception));
}

static inline MagickBooleanType IsStereoRowMissing(const Quantum *p,
  const Quantum *q,const Quantum *r)
{
  return(((p == (const Quantum *) NULL) || (q == (Quantum *) NULL) ||
    (r == (Quantum *) NULL)) ? MagickTrue : MagickFalse);
}

typedef struct _StereoPixels
{
  const Image
    *left_image,
    *right_image;

  Image
    *stereo_image;
} StereoPixels;

static void SetStereoRowPixels(const StereoPixels *stereo_pixels,
  const Quantum *magick_restrict p,const Quantum *magick_restrict q,
  Quantum *magick_restrict r)
{
  const Image
    *const left_image = stereo_pixels->left_image,
    *const right_image = stereo_pixels->right_image;

  Image
    *const stereo_image = stereo_pixels->stereo_image;

  ssize_t
    x;

  for (x=0; x < (ssize_t) stereo_image->columns; x++)
  {
    SetPixelRed(stereo_image,GetPixelRed(left_image,p),r);
    SetPixelGreen(stereo_image,GetPixelGreen(right_image,q),r);
    SetPixelBlue(stereo_image,GetPixelBlue(right_image,q),r);
    if ((GetPixelAlphaTraits(stereo_image) & CopyPixelTrait) != 0)
      SetPixelAlpha(stereo_image,(GetPixelAlpha(left_image,p)+
        GetPixelAlpha(right_image,q))/2,r);
    p+=(ptrdiff_t) GetPixelChannels(left_image);
    q+=(ptrdiff_t) GetPixelChannels(right_image);
    r+=(ptrdiff_t) GetPixelChannels(stereo_image);
  }
}

#define StereoImageTag  "Stereo/Image"

typedef struct _StereoRow
{
  const Image
    *left_image,
    *right_image,
    *image;

  Image
    *stereo_image;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} StereoRow;

static MagickBooleanType SetStereoRow(const StereoRow *stereo_row,
  const ssize_t y,MagickBooleanType *status)
{
  const Image
    *const left_image = stereo_row->left_image,
    *const right_image = stereo_row->right_image,
    *const image = stereo_row->image;

  Image
    *const stereo_image = stereo_row->stereo_image;

  const ssize_t
    x_offset = stereo_row->x_offset,
    y_offset = stereo_row->y_offset;

  ExceptionInfo
    *const exception = stereo_row->exception;

  const Quantum
    *magick_restrict p,
    *magick_restrict q;

  Quantum
    *magick_restrict r;

  StereoPixels
    stereo_pixels;

  p=GetVirtualPixels(left_image,-x_offset,y-y_offset,image->columns,1,
    exception);
  q=GetVirtualPixels(right_image,0,y,right_image->columns,1,exception);
  r=QueueAuthenticPixels(stereo_image,0,y,stereo_image->columns,1,exception);
  if (IsStereoRowMissing(p,q,r) != MagickFalse)
    return(MagickFalse);
  stereo_pixels.left_image=left_image;
  stereo_pixels.right_image=right_image;
  stereo_pixels.stereo_image=stereo_image;
  SetStereoRowPixels(&stereo_pixels,p,q,r);
  if (SyncAuthenticPixels(stereo_image,exception) == MagickFalse)
    return(MagickFalse);
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(image,StereoImageTag,(MagickOffsetType) y,
        stereo_image->rows);
      if (proceed == MagickFalse)
        *status=MagickFalse;
    }
  return(MagickTrue);
}

static MagickBooleanType CloneStereoImage(const Image *left_image,
  Image **stereo_image,ExceptionInfo *exception)
{
  *stereo_image=CloneImage(left_image,left_image->columns,left_image->rows,
    MagickTrue,exception);
  if (*stereo_image == (Image *) NULL)
    return(MagickFalse);
  if (SetImageStorageClass(*stereo_image,DirectClass,exception) == MagickFalse)
    {
      *stereo_image=DestroyImage(*stereo_image);
      return(MagickFalse);
    }
  return(MagickTrue);
}

MagickExport Image *StereoAnaglyphImage(const Image *left_image,
  const Image *right_image,const ssize_t x_offset,const ssize_t y_offset,
  ExceptionInfo *exception)
{
  const Image
    *image;

  Image
    *stereo_image;

  MagickBooleanType
    status;

  ssize_t
    y;

  assert(left_image != (const Image *) NULL);
  assert(left_image->signature == MagickCoreSignature);
  assert(right_image != (const Image *) NULL);
  assert(right_image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      left_image->filename);
  image=left_image;
  if ((left_image->columns != right_image->columns) ||
      (left_image->rows != right_image->rows))
    ThrowImageException(ImageError,"LeftAndRightImageSizesDiffer");
  /*
    Initialize stereo image attributes.
  */
  if (CloneStereoImage(left_image,&stereo_image,exception) == MagickFalse)
    return((Image *) NULL);
  (void) SetImageColorspace(stereo_image,sRGBColorspace,exception);
  /*
    Copy left image to red channel and right image to blue channel.
  */
  status=MagickTrue;
  for (y=0; y < (ssize_t) stereo_image->rows; y++)
  {
    StereoRow
      stereo_row;

    stereo_row.left_image=left_image;
    stereo_row.right_image=right_image;
    stereo_row.image=image;
    stereo_row.stereo_image=stereo_image;
    stereo_row.x_offset=x_offset;
    stereo_row.y_offset=y_offset;
    stereo_row.exception=exception;
    if (SetStereoRow(&stereo_row,y,&status) == MagickFalse)
      break;
  }
  if (status == MagickFalse)
    stereo_image=DestroyImage(stereo_image);
  return(stereo_image);
}
