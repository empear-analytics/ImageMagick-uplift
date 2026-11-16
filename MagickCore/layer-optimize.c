/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                      L       AAA   Y   Y  EEEEE  RRRR                       %
%                      L      A   A   Y Y   E      R   R                      %
%                      L      AAAAA    Y    EEE    RRRR                       %
%                      L      A   A    Y    E      R R                        %
%                      LLLLL  A   A    Y    EEEEE  R  R                       %
%                                                                             %
%            OOO   PPPP   TTTTT  IIIII  M   M  IIIII  ZZZZZ  EEEEE            %
%           O   O  P   P    T      I    MM MM    I       ZZ  E                %
%           O   O  PPPP     T      I    M M M    I     ZZZ   EEE              %
%           O   O  P        T      I    M   M    I    ZZ     E                %
%            OOO   P        T    IIIII  M   M  IIIII  ZZZZZ  EEEEE            %
%                                                                             %
%                 MagickCore Image Layer Optimization Methods                 %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                              Anthony Thyssen                                %
%                               January 2006                                  %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/effect.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/layer.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/profile.h"
#include "MagickCore/resource_.h"
#include "MagickCore/resize.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/layer-frames-private.h"

/*
  Typedef declarations.
*/
typedef struct _FrameBoundsEntry
{
  const RectangleInfo
    *bounds;

  ssize_t
    i;
} FrameBoundsEntry;






/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o m p a r e I m a g e L a y e r s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompareImagesLayers() compares each image with the next in a sequence and
%  returns the minimum bounding region of all the pixel differences (of the
%  LayerMethod specified) it discovers.
%
%  Images do NOT have to be the same size, though it is best that all the
%  images are 'coalesced' (images are all the same size, on a flattened
%  canvas, so as to represent exactly how an specific frame should look).
%
%  No GIF dispose methods are applied, so GIF animations must be coalesced
%  before applying this image operator to find differences to them.
%
%  The format of the CompareImagesLayers method is:
%
%      Image *CompareImagesLayers(const Image *images,
%        const LayerMethod method,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o method: the layers type to compare images with. Must be one of...
%              CompareAnyLayer, CompareClearLayer, CompareOverlayLayer.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType IsUnchangedFrameBounds(
  const RectangleInfo *bounds,const ssize_t i)
{
  return(((bounds[i].x == -1) && (bounds[i].y == -1) &&
    (bounds[i].width == 1) && (bounds[i].height == 1)) ? MagickTrue :
    MagickFalse);
}

typedef struct _LayerFrameComparison
{
  Image
    *image_a;

  RectangleInfo
    *bounds;

  LayerMethod
    method;

  ExceptionInfo
    *exception;
} LayerFrameComparison;

static MagickBooleanType CompareLayerFrames(
  const LayerFrameComparison *frames,const Image *next)
{
  Image
    *const image_a = frames->image_a;

  RectangleInfo
    *const bounds = frames->bounds;

  const LayerMethod
    method = frames->method;

  ExceptionInfo
    *const exception = frames->exception;

  Image
    *image_b;

  ssize_t
    i;

  i=0;
  next=GetNextImageInList(next);
  for ( ; next != (const Image *) NULL; next=GetNextImageInList(next))
  {
    image_b=CloneImage(image_a,0,0,MagickTrue,exception);
    if (image_b == (Image *) NULL)
      return(MagickFalse);
    image_b->background_color.alpha_trait=BlendPixelTrait;
    (void) CompositeImage(image_a,next,CopyCompositeOp,MagickTrue,next->page.x,
      next->page.y,exception);
    bounds[i]=CompareImagesBounds(image_b,image_a,method,exception);
    image_b=DestroyImage(image_b);
    i++;
  }
  return(MagickTrue);
}

static MagickBooleanType AcquireCompareCanvas(const Image *next,
  Image **image_a,ExceptionInfo *exception)
{
  *image_a=CloneImage(next,next->page.width,next->page.height,
    MagickTrue,exception);
  if (*image_a == (Image *) NULL)
    return(MagickFalse);
  (*image_a)->background_color.alpha_trait=BlendPixelTrait;
  (*image_a)->background_color.alpha=(MagickRealType) TransparentAlpha;
  (void) SetImageBackgroundColor(*image_a,exception);
  (*image_a)->page=next->page;
  (*image_a)->page.x=0;
  (*image_a)->page.y=0;
  (void) CompositeImage(*image_a,next,CopyCompositeOp,MagickTrue,next->page.x,
    next->page.y,exception);
  return(MagickTrue);
}

static void DeconstructLayerFrames(RectangleInfo *bounds,
  ExceptionInfo *exception,const Image **next,Image **layers)
{
  Image
    *image_a,
    *image_b;

  ssize_t
    i;

  i=0;
  *next=GetNextImageInList(*next);
  for ( ; *next != (const Image *) NULL; *next=GetNextImageInList(*next))
  {
    if (IsUnchangedFrameBounds(bounds,i) != MagickFalse)
      {
        /*
          An empty frame is returned from CompareImageBounds(), which means the
          current frame is identical to the previous frame.
        */
        i++;
        continue;
      }
    image_a=CloneImage(*next,0,0,MagickTrue,exception);
    if (image_a == (Image *) NULL)
      break;
    image_a->background_color.alpha_trait=BlendPixelTrait;
    image_b=CropImage(image_a,&bounds[i],exception);
    image_a=DestroyImage(image_a);
    if (image_b == (Image *) NULL)
      break;
    AppendImageToList(&(*layers),image_b);
    i++;
  }
}

static Image *FinishDeconstructedLayers(RectangleInfo *bounds,
  const Image *next,Image *layers)
{
  bounds=(RectangleInfo *) RelinquishMagickMemory(bounds);
  if (next != (Image *) NULL)
    {
      layers=DestroyImageList(layers);
      return((Image *) NULL);
    }
  return(GetFirstImageInList(layers));
}

MagickExport Image *CompareImagesLayers(const Image *image,
  const LayerMethod method,ExceptionInfo *exception)
{
  Image
    *image_a,
    *layers;

  LayerFrameComparison
    frames;

  RectangleInfo
    *bounds;

  const Image
    *next;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  assert((method == CompareAnyLayer) ||
         (method == CompareClearLayer) ||
         (method == CompareOverlayLayer));
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  /*
    Allocate bounds memory.
  */
  next=GetFirstImageInList(image);
  bounds=(RectangleInfo *) AcquireQuantumMemory((size_t)
    GetImageListLength(next),sizeof(*bounds));
  if (bounds == (RectangleInfo *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  /*
    Set up first comparison images.
  */
  if (AcquireCompareCanvas(next,&image_a,exception) == MagickFalse)
    {
      bounds=(RectangleInfo *) RelinquishMagickMemory(bounds);
      return((Image *) NULL);
    }
  /*
    Compute the bounding box of changes for the later images
  */
  frames.image_a=image_a;
  frames.bounds=bounds;
  frames.method=method;
  frames.exception=exception;
  if (CompareLayerFrames(&frames,next) == MagickFalse)
    {
      image_a=DestroyImage(image_a);
      bounds=(RectangleInfo *) RelinquishMagickMemory(bounds);
      return((Image *) NULL);
    }
  image_a=DestroyImage(image_a);
  /*
    Clone first image in sequence.
  */
  next=GetFirstImageInList(image);
  layers=CloneImage(next,0,0,MagickTrue,exception);
  if (layers == (Image *) NULL)
    {
      bounds=(RectangleInfo *) RelinquishMagickMemory(bounds);
      return((Image *) NULL);
    }
  layers->background_color.alpha_trait=BlendPixelTrait;
  /*
    Deconstruct the image sequence.
  */
  DeconstructLayerFrames(bounds,exception,&next,&layers);
  return(FinishDeconstructedLayers(bounds,next,layers));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     O p t i m i z e L a y e r F r a m e s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OptimizeLayerFrames() takes a coalesced GIF animation, and compares each
%  frame against the three different 'disposal' forms of the previous frame.
%  From this it then attempts to select the smallest cropped image and
%  disposal method needed to reproduce the resulting image.
%
%  Note that this not easy, and may require the expansion of the bounds
%  of previous frame, simply clear pixels for the next animation frame to
%  transparency according to the selected dispose method.
%
%  The format of the OptimizeLayerFrames method is:
%
%      Image *OptimizeLayerFrames(const Image *image,
%        const LayerMethod method,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o method: the layers technique to optimize with. Must be one of...
%      OptimizeImageLayer, or  OptimizePlusLayer.  The Plus form allows
%      the addition of extra 'zero delay' frames to clear pixels from
%      the previous frame, and the removal of frames that done change,
%      merging the delay times together.
%
%    o exception: return any errors or warnings in this structure.
%
*/
/*
  Define a 'fake' dispose method where the frame is duplicated, (for
  OptimizePlusLayer) with a extra zero time delay frame which does a
  BackgroundDisposal to clear the pixels that need to be cleared.
*/
#define DupDispose  ((DisposeType)9)
/*
  Another 'fake' dispose method used to removed frames that don't change.
*/
#define DelDispose  ((DisposeType)8)

#define DEBUG_OPT_FRAME 0

static inline MagickBooleanType IsPartialPageFrame(const Image *curr,
  const Image *image)
{
  return(((curr->page.x != 0) || (curr->page.y != 0) ||
    (curr->page.width != image->page.width) ||
    (curr->page.height != image->page.height)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsBetterFrameBounds(
  const MagickBooleanType try_cleared,const MagickBooleanType cleared,
  const RectangleInfo try_bounds,const FrameBoundsEntry *entry)
{
  const RectangleInfo
    *bounds = entry->bounds;

  const ssize_t
    i = entry->i;

  return(( (!try_cleared && cleared ) ||
    try_bounds.width * try_bounds.height
    <  bounds[i].width * bounds[i].height ) ? MagickTrue : MagickFalse);
}

static size_t GrowExtent(size_t extent,const size_t try_extent)
{
  if (extent < try_extent)
    extent=try_extent;
  return(extent);
}

static RectangleInfo ExpandBoundsWidth(RectangleInfo bgnd_bounds,
  RectangleInfo try_bounds)
{
  if ( try_bounds.x < bgnd_bounds.x )
    {
       bgnd_bounds.width=(size_t) ((ssize_t) bgnd_bounds.width+
         bgnd_bounds.x-try_bounds.x);
       bgnd_bounds.width=GrowExtent(bgnd_bounds.width,try_bounds.width);
       bgnd_bounds.x = try_bounds.x;
    }
  else
    {
       try_bounds.width=(size_t) ((ssize_t) try_bounds.width+
         try_bounds.x-bgnd_bounds.x);
       bgnd_bounds.width=GrowExtent(bgnd_bounds.width,try_bounds.width);
    }
  return(bgnd_bounds);
}

static RectangleInfo ExpandBoundsHeight(RectangleInfo bgnd_bounds,
  RectangleInfo try_bounds)
{
  if ( try_bounds.y < bgnd_bounds.y )
    {
      bgnd_bounds.height=(size_t) ((ssize_t) bgnd_bounds.height+
        bgnd_bounds.y-try_bounds.y);
      bgnd_bounds.height=GrowExtent(bgnd_bounds.height,try_bounds.height);
      bgnd_bounds.y=try_bounds.y;
    }
  else
    {
      try_bounds.height=(size_t) ((ssize_t) try_bounds.height+
        try_bounds.y-bgnd_bounds.y);
      bgnd_bounds.height=GrowExtent(bgnd_bounds.height,try_bounds.height);
    }
  return(bgnd_bounds);
}

static RectangleInfo ExpandDisposalBounds(RectangleInfo bgnd_bounds,
  RectangleInfo try_bounds)
{
  if ( bgnd_bounds.x < 0 )
    bgnd_bounds = try_bounds;
  else
    {
#if DEBUG_OPT_FRAME
      (void) FormatLocaleFile(stderr, "expand_bgnd: %.17gx%.17g%+.20g%+.20g\n",
          (double) bgnd_bounds.width,(double) bgnd_bounds.height,
          (double) bgnd_bounds.x,(double) bgnd_bounds.y );
#endif
      bgnd_bounds=ExpandBoundsWidth(bgnd_bounds,try_bounds);
      bgnd_bounds=ExpandBoundsHeight(bgnd_bounds,try_bounds);
#if DEBUG_OPT_FRAME
      (void) FormatLocaleFile(stderr, "        to : %.17gx%.17g%+.20g%+.20g\n",
          (double) bgnd_bounds.width,(double) bgnd_bounds.height,
          (double) bgnd_bounds.x,(double) bgnd_bounds.y );
#endif
    }
  return(bgnd_bounds);
}

static void MergeDeletedFrameDelays(const DisposeType *disposals,
  Image *prev_image,const Image **curr,ssize_t *i)
{
  if ( disposals[*i] == DelDispose ) {
    size_t time = 0;
    while ( disposals[*i] == DelDispose ) {
      time +=(size_t) ((*curr)->delay*1000*
        MagickSafeReciprocal((double) (*curr)->ticks_per_second));
      *curr=GetNextImageInList(*curr);
      (*i)++;
    }
    time += (size_t)((*curr)->delay*1000*
      MagickSafeReciprocal((double) (*curr)->ticks_per_second));
    prev_image->ticks_per_second = 100L;
    prev_image->delay = time*(size_t) prev_image->ticks_per_second/1000;
  }
}

static Image *OptimizeImageFrames(const Image *image,RectangleInfo *bounds,
  DisposeType *disposals,ExceptionInfo *exception)
{
  const Image
    *curr;

  ExceptionInfo
    *sans_exception;

  Image
    *prev_image,
    *bgnd_image,
    *optimized_image;

  ssize_t
    i;

  /*
    Optimize all images in sequence.
  */
  sans_exception=AcquireExceptionInfo();
  i=0;
  curr=GetFirstImageInList(image);
  optimized_image=NewImageList();
  while ( curr != (const Image *) NULL )
  {
    prev_image=CloneImage(curr,0,0,MagickTrue,exception);
    if (prev_image == (Image *) NULL)
      break;
    prev_image->background_color.alpha_trait=BlendPixelTrait;
    MergeDeletedFrameDelays(disposals,prev_image,&curr,&i);
    bgnd_image=CropImage(prev_image,&bounds[i],sans_exception);
    prev_image=DestroyImage(prev_image);
    if (bgnd_image == (Image *) NULL)
      break;
    bgnd_image->dispose=disposals[i];
    if ( disposals[i] == DupDispose ) {
      bgnd_image->delay=0;
      bgnd_image->dispose=NoneDispose;
    }
    else
      curr=GetNextImageInList(curr);
    AppendImageToList(&optimized_image,bgnd_image);
    i++;
  }
  sans_exception=DestroyExceptionInfo(sans_exception);
  bounds=(RectangleInfo *) RelinquishMagickMemory(bounds);
  disposals=(DisposeType *) RelinquishMagickMemory(disposals);
  if (curr != (Image *) NULL)
    {
      optimized_image=DestroyImageList(optimized_image);
      return((Image *) NULL);
    }
  return(GetFirstImageInList(optimized_image));
}

static Image *DestroyImageIfSet(Image *image)
{
  if (image != (Image *) NULL)
    image=DestroyImage(image);
  return(image);
}

static MagickBooleanType ReferenceNoneDisposeImage(
  const DisposeType *disposals,const ssize_t i,const Image *curr,
  Image **prev_image)
{
  if ( disposals[i-1] == NoneDispose )
    {
      *prev_image=ReferenceImage(curr->previous);
      if (*prev_image == (Image *) NULL)
        return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _DuplicateFrameTrial
{
  MagickBooleanType
    add_frames;

  const Image
    *curr;

  RectangleInfo
    *bounds;

  DisposeType
    *disposals;

  ssize_t
    i;

  ExceptionInfo
    *exception;

  Image
    *prev_image;
} DuplicateFrameTrial;

typedef struct _DuplicateFrameTargets
{
  Image
    **dup_image;

  RectangleInfo
    *dup_bounds;

  MagickBooleanType
    *cleared;

  Image
    **bgnd_image;

  ssize_t
    *i;
} DuplicateFrameTargets;

static MagickBooleanType TryDuplicateFrame(const DuplicateFrameTrial *trial,
  const DuplicateFrameTargets *targets)
{
  const MagickBooleanType
    add_frames = trial->add_frames;

  const Image
    *const curr = trial->curr;

  RectangleInfo
    *const bounds = trial->bounds;

  DisposeType
    *const disposals = trial->disposals;

  const ssize_t
    i = trial->i;

  ExceptionInfo
    *const exception = trial->exception;

  Image
    **const dup_image = targets->dup_image;

  RectangleInfo
    *const dup_bounds = targets->dup_bounds;

  MagickBooleanType
    *const cleared = targets->cleared;

  RectangleInfo
    try_bounds;

if ( add_frames )
  {
    *dup_image=CloneImage(curr->previous,0,0,MagickTrue,exception);
    if (*dup_image == (Image *) NULL)
      return(MagickFalse);
    (*dup_image)->background_color.alpha_trait=BlendPixelTrait;
    *dup_bounds=CompareImagesBounds(*dup_image,curr,CompareClearLayer,exception);
    ClearBounds(*dup_image,dup_bounds,exception);
    try_bounds=CompareImagesBounds(*dup_image,curr,CompareAnyLayer,exception);
    if ( *cleared ||
           (*dup_bounds).width*(*dup_bounds).height
              +try_bounds.width*try_bounds.height
           < bounds[i].width * bounds[i].height )
      {
        *cleared=MagickFalse;
        bounds[i]=try_bounds;
        disposals[i-1]=DupDispose;
        /* to be finalised later, if found to be optimal */
      }
    else
      (*dup_bounds).width=(*dup_bounds).height=0;
  }
  return(MagickTrue);
}

static Image *DestroyDuplicateImage(const DisposeType *disposals,
  const ssize_t i,Image *dup_image)
{
  if ( disposals[i-1] == DupDispose )
    dup_image=DestroyImage(dup_image);
  return(dup_image);
}

typedef struct _DisposeChoice
{
  const Image
    *curr;

  RectangleInfo
    *bounds;

  DisposeType
    *disposals;

  RectangleInfo
    dup_bounds;
} DisposeChoice;

typedef struct _DisposeChoiceTargets
{
  Image
    **bgnd_image,
    **prev_image,
    **dup_image;

  ssize_t
    *i;
} DisposeChoiceTargets;

static MagickBooleanType FinaliseDisposeChoice(const DisposeChoice *choice,
  const DisposeChoiceTargets *targets)
{
  const Image
    *const curr = choice->curr;

  RectangleInfo
    *const bounds = choice->bounds;

  DisposeType
    *const disposals = choice->disposals;

  const RectangleInfo
    dup_bounds = choice->dup_bounds;

  Image
    **const bgnd_image = targets->bgnd_image,
    **const prev_image = targets->prev_image,
    **const dup_image = targets->dup_image;

  ssize_t
    *const i = targets->i;

  if ( disposals[*i-1] == DupDispose )
    {
       *bgnd_image=DestroyImageIfSet(*bgnd_image);
       *prev_image=DestroyImage(*prev_image);
       *prev_image=*dup_image, *dup_image=(Image *) NULL;
       bounds[*i+1]=bounds[*i];
       bounds[*i]=dup_bounds;
       disposals[*i-1]=DupDispose;
       disposals[*i]=BackgroundDispose;
       (*i)++;
    }
  else
    {
      *dup_image=DestroyImageIfSet(*dup_image);
      if ( disposals[*i-1] != PreviousDispose )
        *prev_image=DestroyImage(*prev_image);
      if ( disposals[*i-1] == BackgroundDispose )
        *prev_image=*bgnd_image, *bgnd_image=(Image *) NULL;
      *bgnd_image=DestroyImageIfSet(*bgnd_image);
      if (ReferenceNoneDisposeImage(disposals,*i,curr,prev_image) ==
          MagickFalse)
        return(MagickFalse);

    }
  return(MagickTrue);
}

static MagickBooleanType AcquireTransparentFrame(const Image *curr,
  Image **prev_image,ExceptionInfo *exception)
{
  *prev_image=CloneImage(curr,curr->columns,curr->rows,MagickTrue,exception);
  if (*prev_image == (Image *) NULL)
    return(MagickFalse);
  (*prev_image)->page=curr->page;  /* ERROR: <-- should not be need, but is! */
  (*prev_image)->page.x=0;
  (*prev_image)->page.y=0;
  (*prev_image)->dispose=NoneDispose;
  (*prev_image)->background_color.alpha_trait=BlendPixelTrait;
  (*prev_image)->background_color.alpha=(MagickRealType) TransparentAlpha;
  (void) SetImageBackgroundColor(*prev_image,exception);
  return(MagickTrue);
}

static MagickBooleanType MarkDeletedFrame(const MagickBooleanType add_frames,
  const ssize_t i,DisposeType *disposals,RectangleInfo *bounds)
{
  if ( add_frames && i>=2 ) {
    disposals[i-1]=DelDispose;
    disposals[i]=NoneDispose;
    bounds[i]=bounds[i-1];
    return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType AcquireBackgroundFrame(const Image *curr,
  Image **bgnd_image,ExceptionInfo *exception)
{
  *bgnd_image=CloneImage(curr->previous,0,0,MagickTrue,exception);
  if (*bgnd_image == (Image *) NULL)
    return(MagickFalse);
  (*bgnd_image)->background_color.alpha_trait=BlendPixelTrait;
  return(MagickTrue);
}

static Image *ReleaseFrameLists(RectangleInfo *bounds,
  DisposeType *disposals)
{
  bounds=(RectangleInfo *) RelinquishMagickMemory(bounds);
  disposals=(DisposeType *) RelinquishMagickMemory(disposals);
  return((Image *) NULL);
}

static Image *ReleaseFrameListsAndPrevious(RectangleInfo *bounds,
  DisposeType *disposals,Image *prev_image)
{
  bounds=(RectangleInfo *) RelinquishMagickMemory(bounds);
  disposals=(DisposeType *) RelinquishMagickMemory(disposals);
  prev_image=DestroyImage(prev_image);
  return((Image *) NULL);
}

static Image *ReleaseFrameListsAndImages(RectangleInfo *bounds,
  DisposeType *disposals,Image *prev_image,Image *dup_image)
{
  bounds=(RectangleInfo *) RelinquishMagickMemory(bounds);
  disposals=(DisposeType *) RelinquishMagickMemory(disposals);
  prev_image=DestroyImage(prev_image);
  dup_image=DestroyImageIfSet(dup_image);
  return((Image *) NULL);
}

static RectangleInfo *AcquireFrameBounds(const Image *curr,
  const MagickBooleanType add_frames)
{
  RectangleInfo
    *bounds;

  bounds=(RectangleInfo *) AcquireQuantumMemory((size_t)
    GetImageListLength(curr),(add_frames != MagickFalse ? 2UL : 1UL)*
    sizeof(*bounds));
  return(bounds);
}

static DisposeType *AcquireFrameDisposals(const Image *image,
  const MagickBooleanType add_frames)
{
  DisposeType
    *disposals;

  disposals=(DisposeType *) AcquireQuantumMemory((size_t)
    GetImageListLength(image),(add_frames != MagickFalse ? 2UL : 1UL)*
    sizeof(*disposals));
  return(disposals);
}

static MagickBooleanType AreFramesCoalesced(const Image *image,
  ExceptionInfo *exception)
{
  const Image
    *curr;

  /*
    Ensure  all the images are the same size.
  */
  curr=GetFirstImageInList(image);
  for (; curr != (Image *) NULL; curr=GetNextImageInList(curr))
  {
    if ((curr->columns != image->columns) || (curr->rows != image->rows))
      {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "ImagesAreNotTheSameSize","`%s'",image->filename);
        return(MagickFalse);
      }

    if (IsPartialPageFrame(curr,image) != MagickFalse)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "ImagePagesAreNotCoalesced","`%s'",image->filename);
        return(MagickFalse);
      }
  }
  return(MagickTrue);
}

static MagickBooleanType TryPreviousDisposal(
  const DuplicateFrameTrial *trial,MagickBooleanType cleared)
{
  const Image
    *const curr = trial->curr;

  RectangleInfo
    *const bounds = trial->bounds;

  DisposeType
    *const disposals = trial->disposals;

  const ssize_t
    i = trial->i;

  ExceptionInfo
    *const exception = trial->exception;

  Image
    *const prev_image = trial->prev_image;

  const FrameBoundsEntry
    entry = { bounds, i };

  RectangleInfo
    try_bounds;

  MagickBooleanType
    try_cleared;

  /*
    Compare a none disposal against a previous disposal
  */
  try_bounds=CompareImagesBounds(prev_image,curr,CompareAnyLayer,exception);
  try_cleared=IsBoundsCleared(prev_image,curr,&try_bounds,exception);
#if DEBUG_OPT_FRAME
    (void) FormatLocaleFile(stderr, "test_prev: %.17gx%.17g%+.20g%+.20g%s\n",
         (double) try_bounds.width,(double) try_bounds.height,
         (double) try_bounds.x,(double) try_bounds.y,
         try_cleared?"  (pixels were cleared)":"");
#endif
  if (IsBetterFrameBounds(try_cleared,cleared,try_bounds,&entry) !=
      MagickFalse)
    {
      cleared=try_cleared;
      bounds[i]=try_bounds;
      disposals[i-1]=PreviousDispose;
#if DEBUG_OPT_FRAME
            (void) FormatLocaleFile(stderr,"previous: accepted\n");
          } else {
            (void) FormatLocaleFile(stderr,"previous: rejected\n");
#endif
    }
  return(cleared);
}

typedef struct _BackgroundBoundsTargets
{
  RectangleInfo
    *bgnd_bounds,
    *try_bounds;
} BackgroundBoundsTargets;

static void ExpandBackgroundBounds(const Image *curr,Image *bgnd_image,
  ExceptionInfo *exception,const BackgroundBoundsTargets *targets)
{
  RectangleInfo
    *const bgnd_bounds = targets->bgnd_bounds,
    *const try_bounds = targets->try_bounds;

  /*
    Straight background disposal failed to clear pixels needed!
    Lets try expanding the disposal area of the previous frame, to
    include the pixels that are cleared.  This guaranteed
    to work, though may not be the most optimized solution.
  */
  (*try_bounds)=CompareImagesBounds(curr->previous,curr,CompareClearLayer,exception);
#if DEBUG_OPT_FRAME
            (void) FormatLocaleFile(stderr, "expand_clear: %.17gx%.17g%+.20g%+.20g%s\n",
                (double) try_bounds.width,(double) try_bounds.height,
                (double) try_bounds.x,(double) try_bounds.y,
                try_bounds.x<0?"  (no expand necessary)":"");
#endif
  (*bgnd_bounds)=ExpandDisposalBounds((*bgnd_bounds),(*try_bounds));
  ClearBounds(bgnd_image,bgnd_bounds,exception);
#if DEBUG_OPT_FRAME
/* Something strange is happening with a specific animation
 * CompareAnyLayers (normal method) and CompareClearLayers returns the whole
 * image, which is not possibly correct!  As verified by previous tests.
 * Something changed beyond the bgnd_bounds clearing.  But without being able
 * to see, or writet he image at this point it is hard to tell what is wrong!
 * Only CompareOverlay seemed to return something sensible.
 */
            try_bounds=CompareImagesBounds(bgnd_image,curr,CompareClearLayer,exception);
            (void) FormatLocaleFile(stderr, "expand_ctst: %.17gx%.17g%+.20g%+.20g\n",
                (double) try_bounds.width,(double) try_bounds.height,
                (double) try_bounds.x,(double) try_bounds.y );
            try_bounds=CompareImagesBounds(bgnd_image,curr,CompareAnyLayer,exception);
            try_cleared=IsBoundsCleared(bgnd_image,curr,&try_bounds,exception);
            (void) FormatLocaleFile(stderr, "expand_any : %.17gx%.17g%+.20g%+.20g%s\n",
                (double) try_bounds.width,(double) try_bounds.height,
                (double) try_bounds.x,(double) try_bounds.y,
                try_cleared?"   (pixels cleared)":"");
#endif
  (*try_bounds)=CompareImagesBounds(bgnd_image,curr,CompareOverlayLayer,exception);
#if DEBUG_OPT_FRAME
            try_cleared=IsBoundsCleared(bgnd_image,curr,&try_bounds,exception);
            (void) FormatLocaleFile(stderr, "expand_test: %.17gx%.17g%+.20g%+.20g%s\n",
                (double) try_bounds.width,(double) try_bounds.height,
                (double) try_bounds.x,(double) try_bounds.y,
                try_cleared?"   (pixels cleared)":"");
#endif
}

static MagickBooleanType TryBackgroundDisposal(
  const DuplicateFrameTrial *trial,const RectangleInfo dup_bounds,
  const DuplicateFrameTargets *targets)
{
  const Image
    *const curr = trial->curr;

  RectangleInfo
    *const bounds = trial->bounds;

  DisposeType
    *const disposals = trial->disposals;

  const ssize_t
    i = trial->i;

  ExceptionInfo
    *const exception = trial->exception;

  Image
    **const bgnd_image = targets->bgnd_image,
    **const dup_image = targets->dup_image;

  MagickBooleanType
    *const cleared = targets->cleared;

  RectangleInfo
    bgnd_bounds,
    try_bounds;

  MagickBooleanType
    try_cleared;

  const BackgroundBoundsTargets
    bgnd_targets = { &bgnd_bounds, &try_bounds };

  /*
    Now compare against a simple background disposal
  */
  if (AcquireBackgroundFrame(curr,bgnd_image,exception) == MagickFalse)
    return(MagickFalse);
  bgnd_bounds=bounds[i-1]; /* interim bounds of the previous image */
  ClearBounds((*bgnd_image),&bgnd_bounds,exception);
  try_bounds=CompareImagesBounds((*bgnd_image),curr,CompareAnyLayer,exception);
  try_cleared=IsBoundsCleared((*bgnd_image),curr,&try_bounds,exception);
#if DEBUG_OPT_FRAME
    (void) FormatLocaleFile(stderr, "background: %s\n",
         try_cleared?"(pixels cleared)":"");
#endif
  if ( try_cleared )
    {
      ExpandBackgroundBounds(curr,(*bgnd_image),exception,&bgnd_targets);
    }
  /*
    Test if this background dispose is smaller than any of the
    other methods we tried before this (including duplicated frame)
  */
  if ( (*cleared) ||
        bgnd_bounds.width*bgnd_bounds.height
          +try_bounds.width*try_bounds.height
        < bounds[i-1].width*bounds[i-1].height
            +dup_bounds.width*dup_bounds.height
            +bounds[i].width*bounds[i].height )
    {
      (*cleared)=MagickFalse;
      bounds[i-1]=bgnd_bounds;
      bounds[i]=try_bounds;
      (*dup_image)=DestroyDuplicateImage(disposals,i,(*dup_image));
      disposals[i-1]=BackgroundDispose;
#if DEBUG_OPT_FRAME
    (void) FormatLocaleFile(stderr,"expand_bgnd: accepted\n");
          } else {
    (void) FormatLocaleFile(stderr,"expand_bgnd: reject\n");
#endif
    }
  return(MagickTrue);
}

static void RecordInterimDisposal(const Image *prev_image,
  DisposeType *disposals,const ssize_t i)
{
  assert(prev_image != (Image *) NULL);
  magick_unreferenced(prev_image);
  disposals[i]=disposals[i-1];
#if DEBUG_OPT_FRAME
    (void) FormatLocaleFile(stderr, "final   %.17g : %s  %.17gx%.17g%+.20g%+.20g\n",
         (double) i-1,
         CommandOptionToMnemonic(MagickDisposeOptions,disposals[i-1]),
         (double) bounds[i-1].width,(double) bounds[i-1].height,
         (double) bounds[i-1].x,(double) bounds[i-1].y );
#endif
#if DEBUG_OPT_FRAME
    (void) FormatLocaleFile(stderr, "interim %.17g : %s  %.17gx%.17g%+.20g%+.20g\n",
         (double) i,
         CommandOptionToMnemonic(MagickDisposeOptions,disposals[i]),
         (double) bounds[i].width,(double) bounds[i].height,
         (double) bounds[i].x,(double) bounds[i].y );
    (void) FormatLocaleFile(stderr,"\n");
#endif
}

typedef struct _FrameListTargets
{
  const Image
    **curr;

  RectangleInfo
    **bounds;

  DisposeType
    **disposals;
} FrameListTargets;

static MagickBooleanType AcquireFrameLists(const Image *image,
  const MagickBooleanType add_frames,ExceptionInfo *exception,
  const FrameListTargets *targets)
{
  const Image
    **const curr = targets->curr;

  RectangleInfo
    **const bounds = targets->bounds;

  DisposeType
    **const disposals = targets->disposals;

  /*
    Allocate memory (times 2 if we allow the use of frame duplications)
  */
  (*curr)=GetFirstImageInList(image);
  (*bounds)=AcquireFrameBounds((*curr),add_frames);
  if ((*bounds) == (RectangleInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(MagickFalse);
    }
  (*disposals)=AcquireFrameDisposals(image,add_frames);
  if ((*disposals) == (DisposeType *) NULL)
    {
      (*bounds)=(RectangleInfo *) RelinquishMagickMemory((*bounds));
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType AssumeNoneDisposal(
  const DuplicateFrameTrial *trial)
{
  const Image
    *const curr = trial->curr;

  RectangleInfo
    *const bounds = trial->bounds;

  DisposeType
    *const disposals = trial->disposals;

  const ssize_t
    i = trial->i;

  ExceptionInfo
    *const exception = trial->exception;

  MagickBooleanType
    cleared;

#if DEBUG_OPT_FRAME
    (void) FormatLocaleFile(stderr,"frame %.17g :-\n",(double) i);
#endif
  /*
    Assume none disposal is the best
  */
  bounds[i]=CompareImagesBounds(curr->previous,curr,CompareAnyLayer,exception);
  cleared=IsBoundsCleared(curr->previous,curr,&bounds[i],exception);
  disposals[i-1]=NoneDispose;
#if DEBUG_OPT_FRAME
    (void) FormatLocaleFile(stderr, "overlay: %.17gx%.17g%+.20g%+.20g%s%s\n",
         (double) bounds[i].width,(double) bounds[i].height,
         (double) bounds[i].x,(double) bounds[i].y,
         bounds[i].x < 0?"  (unchanged)":"",
         cleared?"  (pixels cleared)":"");
#endif
  return(cleared);
}

typedef enum
{
  FrameDisposalChosen,
  FrameDisposalSkipped,
  FrameDisposalPreviousFailed,
  FrameDisposalImagesFailed
} FrameDisposalExit;

static FrameDisposalExit TryFrameTrials(const DuplicateFrameTrial *trial,
  const DuplicateFrameTargets *targets)
{
  MagickBooleanType
    *const cleared = targets->cleared;

  RectangleInfo
    *const dup_bounds = targets->dup_bounds;

  (*cleared)=TryPreviousDisposal(trial,(*cleared));

  /*
    If we are allowed lets try a complex frame duplication.
    It is useless if the previous image already clears pixels correctly.
    This method will always clear all the pixels that need to be cleared.
  */
  (*dup_bounds).width=(*dup_bounds).height=0; /* no dup, no pixel added */
  if (TryDuplicateFrame(trial,targets) == MagickFalse)
    return(FrameDisposalPreviousFailed);
  if (TryBackgroundDisposal(trial,(*dup_bounds),targets) == MagickFalse)
    return(FrameDisposalImagesFailed);
  return(FrameDisposalChosen);
}

static FrameDisposalExit SelectFrameDisposal(const DuplicateFrameTrial *trial,
  const DuplicateFrameTargets *targets)
{
  const MagickBooleanType
    add_frames = trial->add_frames;

  RectangleInfo
    *const bounds = trial->bounds;

  DisposeType
    *const disposals = trial->disposals;

  ssize_t
    *const i = targets->i;

  if ( bounds[(*i)].x < 0 ) {
    /*
      Image frame is exactly the same as the previous frame!
      If not adding frames leave it to be cropped down to a null image.
      Otherwise mark previous image for deleted, transfering its crop bounds
      to the current image.
    */
    if (MarkDeletedFrame(add_frames,(*i),disposals,bounds) == MagickFalse)
      {
        (*i)++;
        return(FrameDisposalSkipped);
      }
  }
  else
    return(TryFrameTrials(trial,targets));
  return(FrameDisposalChosen);
}

typedef struct _LayerFramePairs
{
  const Image
    *image,
    *curr;

  MagickBooleanType
    add_frames;

  RectangleInfo
    *bounds;

  DisposeType
    *disposals;

  ExceptionInfo
    *exception;

  Image
    *prev_image;

  ssize_t
    i;
} LayerFramePairs;

static Image *CompareLayerFramePairs(const LayerFramePairs *pairs)
{
  const Image
    *const image = pairs->image,
    *curr = pairs->curr;

  const MagickBooleanType
    add_frames = pairs->add_frames;

  RectangleInfo
    *const bounds = pairs->bounds;

  DisposeType
    *const disposals = pairs->disposals;

  ExceptionInfo
    *const exception = pairs->exception;

  Image
    *prev_image = pairs->prev_image,
    *dup_image,
    *bgnd_image;

  RectangleInfo
    dup_bounds;

  MagickBooleanType
    cleared;

  ssize_t
    i = pairs->i;

  const DisposeChoiceTargets
    choice_targets = { &bgnd_image, &prev_image, &dup_image, &i };

  const DuplicateFrameTargets
    targets = { &dup_image, &dup_bounds, &cleared, &bgnd_image, &i };

  bgnd_image=(Image *) NULL;
  dup_image=(Image *) NULL;
  dup_bounds.width=0;
  dup_bounds.height=0;
  dup_bounds.x=0;
  dup_bounds.y=0;
  curr=GetNextImageInList(curr);
  for ( ; curr != (const Image *) NULL; curr=GetNextImageInList(curr))
  {
    const DuplicateFrameTrial
      trial = { add_frames, curr, bounds, disposals, i, exception,
        prev_image };

    cleared=AssumeNoneDisposal(&trial);
    switch (SelectFrameDisposal(&trial,&targets))
    {
      case FrameDisposalSkipped:
        continue;
      case FrameDisposalPreviousFailed:
        return(ReleaseFrameListsAndPrevious(bounds,disposals,prev_image));
      case FrameDisposalImagesFailed:
        return(ReleaseFrameListsAndImages(bounds,disposals,prev_image,
          dup_image));
      default:
        break;
    }
    /*
       Finalise choice of dispose, set new prev_image,
       and junk any extra images as appropriate,
    */
    {
      const DisposeChoice
        choice = { curr, bounds, disposals, dup_bounds };

      if (FinaliseDisposeChoice(&choice,&choice_targets) == MagickFalse)
        return(ReleaseFrameLists(bounds,disposals));
    }
    RecordInterimDisposal(prev_image,disposals,i);
    i++;
  }
  prev_image=DestroyImage(prev_image);
  return(OptimizeImageFrames(image,bounds,disposals,exception));
}

static Image *OptimizeLayerFrames(const Image *image,const LayerMethod method,
  ExceptionInfo *exception)
{
  const Image
    *curr;

  DisposeType
    *disposals;

  Image
    *prev_image;

  LayerFramePairs
    pairs;

  RectangleInfo
    *bounds;

  MagickBooleanType
    add_frames;

  ssize_t
    i;

  const FrameListTargets
    lists = { &curr, &bounds, &disposals };

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  assert(method == OptimizeLayer ||
         method == OptimizeImageLayer ||
         method == OptimizePlusLayer);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  /*
    Are we allowed to add/remove frames from animation?
  */
  add_frames=method == OptimizePlusLayer ? MagickTrue : MagickFalse;
  if (AreFramesCoalesced(image,exception) == MagickFalse)
    return((Image *) NULL);
  if (AcquireFrameLists(image,add_frames,exception,&lists) == MagickFalse)
    return((Image *) NULL);
  /*
    Initialise Previous Image as fully transparent
  */
  if (AcquireTransparentFrame(curr,&prev_image,exception) == MagickFalse)
    return(ReleaseFrameLists(bounds,disposals));
  /*
    Figure out the area of overlay of the first frame
    No pixel could be cleared as all pixels are already cleared.
  */
#if DEBUG_OPT_FRAME
  i=0;
  (void) FormatLocaleFile(stderr,"frame %.17g :-\n",(double) i);
#endif
  disposals[0]=NoneDispose;
  bounds[0]=CompareImagesBounds(prev_image,curr,CompareAnyLayer,exception);
#if DEBUG_OPT_FRAME
  (void) FormatLocaleFile(stderr, "overlay: %.17gx%.17g%+.20g%+.20g\n\n",
    (double) bounds[i].width,(double) bounds[i].height,
    (double) bounds[i].x,(double) bounds[i].y );
#endif
  /*
    Compute the bounding box of changes for each pair of images.
  */
  i=1;
  pairs.image=image;
  pairs.curr=curr;
  pairs.add_frames=add_frames;
  pairs.bounds=bounds;
  pairs.disposals=disposals;
  pairs.exception=exception;
  pairs.prev_image=prev_image;
  pairs.i=i;
  return(CompareLayerFramePairs(&pairs));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     O p t i m i z e I m a g e L a y e r s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OptimizeImageLayers() compares each image the GIF disposed forms of the
%  previous image in the sequence.  From this it attempts to select the
%  smallest cropped image to replace each frame, while preserving the results
%  of the GIF animation.
%
%  The format of the OptimizeImageLayers method is:
%
%      Image *OptimizeImageLayers(const Image *image,
%               ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *OptimizeImageLayers(const Image *image,
  ExceptionInfo *exception)
{
  return(OptimizeLayerFrames(image,OptimizeImageLayer,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     O p t i m i z e P l u s I m a g e L a y e r s                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OptimizeImagePlusLayers() is exactly as OptimizeImageLayers(), but may
%  also add or even remove extra frames in the animation, if it improves
%  the total number of pixels in the resulting GIF animation.
%
%  The format of the OptimizePlusImageLayers method is:
%
%      Image *OptimizePlusImageLayers(const Image *image,
%               ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *OptimizePlusImageLayers(const Image *image,
  ExceptionInfo *exception)
{
  return OptimizeLayerFrames(image,OptimizePlusLayer,exception);
}
