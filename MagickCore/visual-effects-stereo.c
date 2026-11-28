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
%                  SSSSS  TTTTT  EEEEE  RRRR   EEEEE   OOO                    %
%                  SS       T    E      R   R  E      O   O                   %
%                   SSS     T    EEE    RRRR   EEE    O   O                   %
%                     SS    T    E      R R    E      O   O                   %
%                  SSSSS    T    EEEEE  R  R   EEEEE   OOO                    %
%                                                                             %
%                                                                             %
%                     MagickCore Stereo Anaglyph Methods                      %
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

static inline MagickBooleanType IsStereoSizeMismatch(const Image *left_image,
  const Image *right_image)
{
  return(((left_image->columns != right_image->columns) ||
    (left_image->rows != right_image->rows)) ? MagickTrue : MagickFalse);
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
  if (IsStereoSizeMismatch(left_image,right_image) != MagickFalse)
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
