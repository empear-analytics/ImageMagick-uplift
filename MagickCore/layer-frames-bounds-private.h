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

  MagickCore private image layer methods: the bounds clearing and pixel scan
  helpers, from ClearBounds to IsOverlayPixelChanged; included by
  layer-frames-private.h only.
*/
#ifndef MAGICKCORE_LAYER_FRAMES_BOUNDS_PRIVATE_H
#define MAGICKCORE_LAYER_FRAMES_BOUNDS_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     C l e a r B o u n d s                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClearBounds() Clear the area specified by the bounds in an image to
%  transparency.  This typically used to handle Background Disposal for the
%  previous frame in an animation sequence.
%
%  Warning: no bounds checks are performed, except for the null or missed
%  image, for images that don't change. in all other cases bound must fall
%  within the image.
%
%  The format is:
%
%      void ClearBounds(Image *image,RectangleInfo *bounds,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image to had the area cleared in
%
%    o bounds: the area to be clear within the imag image
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline void ClearBounds(Image *image,RectangleInfo *bounds,
  ExceptionInfo *exception)
{
  ssize_t
    y;

  if (bounds->x < 0)
    return;
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
  for (y=0; y < (ssize_t) bounds->height; y++)
  {
    ssize_t
      x;

    Quantum
      *magick_restrict q;

    q=GetAuthenticPixels(image,bounds->x,bounds->y+y,bounds->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) bounds->width; x++)
    {
      SetPixelAlpha(image,TransparentAlpha,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     I s B o u n d s C l e a r e d                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsBoundsCleared() tests whether any pixel in the bounds given, gets cleared
%  when going from the first image to the second image.  This typically used
%  to check if a proposed disposal method will work successfully to generate
%  the second frame image from the first disposed form of the previous frame.
%
%  Warning: no bounds checks are performed, except for the null or missed
%  image, for images that don't change. in all other cases bound must fall
%  within the image.
%
%  The format is:
%
%      MagickBooleanType IsBoundsCleared(const Image *image1,
%        const Image *image2,RectangleInfo bounds,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image1, image 2: the images to check for cleared pixels
%
%    o bounds: the area to be clear within the imag image
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsPixelCleared(const Image *image1,
  const Quantum *p,const Image *image2,const Quantum *q)
{
  return(((GetPixelAlpha(image1,p) >= (Quantum) (QuantumRange/2)) &&
    (GetPixelAlpha(image2,q) < (Quantum) (QuantumRange/2))) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsEitherRowMissing(const Quantum *p,
  const Quantum *q)
{
  return(((p == (const Quantum *) NULL) || (q == (Quantum *) NULL)) ?
    MagickTrue : MagickFalse);
}

typedef struct _ClearedPixelScan
{
  const Image
    *image1,
    *image2;

  RectangleInfo
    *bounds;
} ClearedPixelScan;

static inline ssize_t FindClearedPixel(const ClearedPixelScan *scan,const Quantum *p,
  const Quantum *q)
{
  const Image
    *const image1 = scan->image1,
    *const image2 = scan->image2;

  RectangleInfo
    *const bounds = scan->bounds;

  ssize_t
    x;

  for (x=0; x < (ssize_t) bounds->width; x++)
  {
    if (IsPixelCleared(image1,p,image2,q) != MagickFalse)
      break;
    p+=(ptrdiff_t) GetPixelChannels(image1);
    q+=(ptrdiff_t) GetPixelChannels(image2);
  }
  return(x);
}

static inline MagickBooleanType IsBoundsCleared(const Image *image1,
  const Image *image2,RectangleInfo *bounds,ExceptionInfo *exception)
{
  const Quantum
    *p,
    *q;

  ssize_t
    x;

  ssize_t
    y;

  if (bounds->x < 0)
    return(MagickFalse);
  for (y=0; y < (ssize_t) bounds->height; y++)
  {
    ClearedPixelScan
      scan;

    p=GetVirtualPixels(image1,bounds->x,bounds->y+y,bounds->width,1,exception);
    q=GetVirtualPixels(image2,bounds->x,bounds->y+y,bounds->width,1,exception);
    if (IsEitherRowMissing(p,q) != MagickFalse)
      break;
    scan.image1=image1;
    scan.image2=image2;
    scan.bounds=bounds;
    x=FindClearedPixel(&scan,p,q);
    if (x < (ssize_t) bounds->width)
      break;
  }
  return(y < (ssize_t) bounds->height ? MagickTrue : MagickFalse);
}

typedef struct _DifferingPixelScan
{
  LayerMethod
    method;

  const Image
    *alpha_image,
    *beta_image;

  PixelInfo
    *alpha_pixel,
    *beta_pixel;
} DifferingPixelScan;

static inline MagickBooleanType IsOverlayPixelChanged(const double o2,
  const PixelInfo *p,const PixelInfo *q)
{
  if (o2 < ((double) QuantumRange/2.0))
    return MagickFalse;
  return(IsFuzzyEquivalencePixelInfo(p,q) == MagickFalse ? MagickTrue :
    MagickFalse);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
