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
%               DDDD   IIIII  SSSSS  PPPP    OOO   SSSSS  EEEEE               %
%               D   D    I    SS     P   P  O   O  SS     E                   %
%               D   D    I     SSS   PPPP   O   O   SSS   EEE                 %
%               D   D    I       SS  P      O   O     SS  E                   %
%               DDDD   IIIII  SSSSS  P       OOO   SSSSS  EEEEE               %
%                                                                             %
%                   MagickCore Image Layer Disposal Methods                   %
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
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o a l e s c e I m a g e s                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CoalesceImages() composites a set of images while respecting any page
%  offsets and disposal methods.  GIF, MIFF, and MNG animation sequences
%  typically start with an image background and each subsequent image
%  varies in size and offset.  A new image sequence is returned with all
%  images the same size as the first images virtual canvas and composited
%  with the next image in the sequence.
%
%  The format of the CoalesceImages method is:
%
%      Image *CoalesceImages(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image sequence.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static size_t GetCanvasExtent(size_t extent,const size_t size,
  const ssize_t offset)
{
  if (extent == 0)
    {
      extent=size;
      if (offset > 0)
        extent+=(size_t) offset;
    }
  return(extent);
}

static void CompositeCoalescedFrame(Image *coalesce_image,const Image *next,
  const char *attribute,ExceptionInfo *exception)
{
  if (attribute == (const char *) NULL)
    (void) CompositeImage(coalesce_image,next,
      next->alpha_trait != UndefinedPixelTrait ? OverCompositeOp :
      CopyCompositeOp,MagickTrue,next->page.x,next->page.y,exception);
  else
    (void) CompositeImage(coalesce_image,next,
      LocaleCompare(attribute,"AtopBackgroundAlphaBlend") == 0 ?
      OverCompositeOp : CopyCompositeOp,MagickTrue,next->page.x,next->page.y,
      exception);
}

static RectangleInfo ClipBoundsLeft(RectangleInfo bounds)
{
  if (bounds.x < 0)
    {
      bounds.width=(size_t) ((ssize_t) bounds.width+bounds.x);
      bounds.x=0;
    }
  return(bounds);
}

static RectangleInfo ClipBoundsTop(RectangleInfo bounds)
{
  if (bounds.y < 0)
    {
      bounds.height=(size_t) ((ssize_t) bounds.height+bounds.y);
      bounds.y=0;
    }
  return(bounds);
}

static RectangleInfo ClipBoundsRight(RectangleInfo bounds,
  const Image *image)
{
  if ((bounds.x+(ssize_t) bounds.width) > (ssize_t) image->columns)
    bounds.width=(size_t) ((ssize_t) image->columns-bounds.x);
  return(bounds);
}

static RectangleInfo ClipBoundsBottom(RectangleInfo bounds,
  const Image *image)
{
  if ((bounds.y+(ssize_t) bounds.height) > (ssize_t) image->rows)
    bounds.height=(size_t) ((ssize_t) image->rows-bounds.y);
  return(bounds);
}

static void SetCoalescedDispose(const Image *previous,Image *coalesce_image,
  RectangleInfo *bounds,ExceptionInfo *exception)
{
  if (IsBoundsCleared(previous,coalesce_image,bounds,exception) != MagickFalse)
    coalesce_image->dispose=BackgroundDispose;
  else
    coalesce_image->dispose=NoneDispose;
}

static MagickBooleanType ReplaceDisposeImage(const Image *next,
  const Image *coalesce_image,Image **dispose_image,ExceptionInfo *exception)
{
  if (GetPreviousImageInList(next)->dispose != PreviousDispose)
    {
      *dispose_image=DestroyImage(*dispose_image);
      *dispose_image=CloneImage(coalesce_image,0,0,MagickTrue,exception);
      if (*dispose_image == (Image *) NULL)
        return(MagickFalse);
      (*dispose_image)->background_color.alpha_trait=BlendPixelTrait;
    }
  return(MagickTrue);
}

static MagickBooleanType AcquireCoalesceCanvas(Image *next,
  const RectangleInfo bounds,Image **coalesce_image,ExceptionInfo *exception)
{
  *coalesce_image=CloneImage(next,bounds.width,bounds.height,MagickTrue,
    exception);
  if (*coalesce_image == (Image *) NULL)
    return(MagickFalse);
  (*coalesce_image)->background_color.alpha_trait=BlendPixelTrait;
  (*coalesce_image)->background_color.alpha=(MagickRealType) TransparentAlpha;
  (void) SetImageBackgroundColor(*coalesce_image,exception);
  (*coalesce_image)->alpha_trait=next->alpha_trait;
  (*coalesce_image)->page=bounds;
  (*coalesce_image)->dispose=NoneDispose;
  return(MagickTrue);
}

static RectangleInfo GetClippedFrameBounds(const Image *previous,
  const Image *coalesce_image)
{
  RectangleInfo
    bounds;

  bounds=previous->page;
  bounds.width=previous->columns;
  bounds.height=previous->rows;
  bounds=ClipBoundsLeft(bounds);
  bounds=ClipBoundsRight(bounds,coalesce_image);
  bounds=ClipBoundsTop(bounds);
  bounds=ClipBoundsBottom(bounds,coalesce_image);
  return(bounds);
}

static RectangleInfo GetCoalesceBounds(const Image *next)
{
  RectangleInfo
    bounds;

  bounds=next->page;
  bounds.width=GetCanvasExtent(bounds.width,next->columns,bounds.x);
  bounds.height=GetCanvasExtent(bounds.height,next->rows,bounds.y);
  bounds.x=0;
  bounds.y=0;
  return(bounds);
}

MagickExport Image *CoalesceImages(const Image *image,ExceptionInfo *exception)
{
  Image
    *coalesce_image,
    *dispose_image,
    *previous;

  Image
    *next;

  RectangleInfo
    bounds;

  /*
    Coalesce the image sequence.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  next=GetFirstImageInList(image);
  bounds=GetCoalesceBounds(next);
  if (AcquireCoalesceCanvas(next,bounds,&coalesce_image,exception) ==
      MagickFalse)
    return((Image *) NULL);
  /*
    Coalesce rest of the images.
  */
  dispose_image=CloneImage(coalesce_image,0,0,MagickTrue,exception);
  if (dispose_image == (Image *) NULL)
    {
      coalesce_image=DestroyImage(coalesce_image);
      return((Image *) NULL);
    }
  dispose_image->background_color.alpha_trait=BlendPixelTrait;
  (void) CompositeImage(coalesce_image,next,CopyCompositeOp,MagickTrue,
    next->page.x,next->page.y,exception);
  next=GetNextImageInList(next);
  for ( ; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    const char
      *attribute;

    /*
      Determine the bounds that was overlaid in the previous image.
    */
    previous=GetPreviousImageInList(next);
    bounds=GetClippedFrameBounds(previous,coalesce_image);
    /*
      Replace the dispose image with the new coalesced image.
    */
    if (ReplaceDisposeImage(next,coalesce_image,&dispose_image,exception) ==
        MagickFalse)
      {
        coalesce_image=DestroyImageList(coalesce_image);
        return((Image *) NULL);
      }
    /*
      Clear the overlaid area of the coalesced bounds for background disposal
    */
    if (next->previous->dispose == BackgroundDispose)
      ClearBounds(dispose_image,&bounds,exception);
    /*
      Next image is the dispose image, overlaid with next frame in sequence.
    */
    coalesce_image->next=CloneImage(dispose_image,0,0,MagickTrue,exception);
    if (coalesce_image->next != (Image *) NULL)
      coalesce_image->next->previous=coalesce_image;
    previous=coalesce_image;
    coalesce_image=GetNextImageInList(coalesce_image);
    coalesce_image->background_color.alpha_trait=BlendPixelTrait;
    attribute=GetImageProperty(next,"webp:mux-blend",exception);
    CompositeCoalescedFrame(coalesce_image,next,attribute,exception);
    (void) CloneImageProfiles(coalesce_image,next);
    (void) CloneImageProperties(coalesce_image,next);
    (void) CloneImageArtifacts(coalesce_image,next);
    coalesce_image->page=previous->page;
    /*
      If a pixel goes opaque to transparent, use background dispose.
    */
    SetCoalescedDispose(previous,coalesce_image,&bounds,exception);
    previous->dispose=coalesce_image->dispose;
  }
  dispose_image=DestroyImage(dispose_image);
  return(GetFirstImageInList(coalesce_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     D i s p o s e I m a g e s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DisposeImages() returns the coalesced frames of a GIF animation as it would
%  appear after the GIF dispose method of that frame has been applied.  That is
%  it returned the appearance of each frame before the next is overlaid.
%
%  The format of the DisposeImages method is:
%
%      Image *DisposeImages(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o images: the image sequence.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *SelectDisposeImage(const Image *next,Image *dispose_image,
  Image *current_image)
{
  if (next->dispose == PreviousDispose)
    current_image=DestroyImage(current_image);
  else
    {
      dispose_image=DestroyImage(dispose_image);
      dispose_image=current_image;
      current_image=(Image *) NULL;
    }
  return(dispose_image);
}

static MagickBooleanType CloneDisposeFrame(Image *dispose_image,
  const Image *next,Image **dispose,ExceptionInfo *exception)
{
  *dispose=CloneImage(dispose_image,0,0,MagickTrue,exception);
  if (*dispose == (Image *) NULL)
    return(MagickFalse);
  dispose_image->background_color.alpha_trait=BlendPixelTrait;
  (void) CloneImageProfiles(*dispose,next);
  (void) CloneImageProperties(*dispose,next);
  (void) CloneImageArtifacts(*dispose,next);
  (*dispose)->page.x=0;
  (*dispose)->page.y=0;
  (*dispose)->dispose=next->dispose;
  return(MagickTrue);
}

static MagickBooleanType AcquireDisposeCanvas(const Image *image,
  Image **dispose_image,ExceptionInfo *exception)
{
  *dispose_image=CloneImage(image,image->page.width,image->page.height,
    MagickTrue,exception);
  if (*dispose_image == (Image *) NULL)
    return(MagickFalse);
  (*dispose_image)->page=image->page;
  (*dispose_image)->page.x=0;
  (*dispose_image)->page.y=0;
  (*dispose_image)->dispose=NoneDispose;
  (*dispose_image)->background_color.alpha_trait=BlendPixelTrait;
  (*dispose_image)->background_color.alpha=(MagickRealType) TransparentAlpha;
  (void) SetImageBackgroundColor(*dispose_image,exception);
  return(MagickTrue);
}

MagickExport Image *DisposeImages(const Image *images,ExceptionInfo *exception)
{
  Image
    *dispose_image,
    *dispose_images;

  RectangleInfo
    bounds;

  Image
    *image,
    *next;

  /*
    Run the image through the animation sequence
  */
  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  image=GetFirstImageInList(images);
  if (AcquireDisposeCanvas(image,&dispose_image,exception) == MagickFalse)
    return((Image *) NULL);
  dispose_images=NewImageList();
  for (next=image; image != (Image *) NULL; image=GetNextImageInList(image))
  {
    Image
      *current_image;

    /*
      Overlay this frame's image over the previous disposal image.
    */
    current_image=CloneImage(dispose_image,0,0,MagickTrue,exception);
    if (current_image == (Image *) NULL)
      {
        dispose_images=DestroyImageList(dispose_images);
        dispose_image=DestroyImage(dispose_image);
        return((Image *) NULL);
      }
    current_image->background_color.alpha_trait=BlendPixelTrait;
    (void) CompositeImage(current_image,next,
      next->alpha_trait != UndefinedPixelTrait ? OverCompositeOp : CopyCompositeOp,
      MagickTrue,next->page.x,next->page.y,exception);
    /*
      Handle Background dispose: image is displayed for the delay period.
    */
    if (next->dispose == BackgroundDispose)
      {
        bounds=next->page;
        bounds.width=next->columns;
        bounds.height=next->rows;
        bounds=ClipBoundsLeft(bounds);
        bounds=ClipBoundsRight(bounds,current_image);
        bounds=ClipBoundsTop(bounds);
        bounds=ClipBoundsBottom(bounds,current_image);
        ClearBounds(current_image,&bounds,exception);
      }
    /*
      Select the appropriate previous/disposed image.
    */
    dispose_image=SelectDisposeImage(next,dispose_image,current_image);
    /*
      Save the dispose image just calculated for return.
    */
    {
      Image
        *dispose;

      if (CloneDisposeFrame(dispose_image,next,&dispose,exception) ==
          MagickFalse)
        {
          dispose_images=DestroyImageList(dispose_images);
          dispose_image=DestroyImage(dispose_image);
          return((Image *) NULL);
        }
      AppendImageToList(&dispose_images,dispose);
    }
  }
  dispose_image=DestroyImage(dispose_image);
  return(GetFirstImageInList(dispose_images));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     O p t i m i z e I m a g e T r a n s p a r e n c y                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OptimizeImageTransparency() takes a frame optimized GIF animation, and
%  compares the overlayed pixels against the disposal image resulting from all
%  the previous frames in the animation.  Any pixel that does not change the
%  disposal image (and thus does not effect the outcome of an overlay) is made
%  transparent.
%
%  WARNING: This modifies the current images directly, rather than generate
%  a new image sequence.
%
%  The format of the OptimizeImageTransparency method is:
%
%      void OptimizeImageTransparency(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image sequence
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *KeepDisposedImage(const Image *next,Image *dispose_image,
  Image *current_image)
{
  if (next->dispose != PreviousDispose)
    {
      dispose_image=DestroyImage(dispose_image);
      dispose_image=current_image;
    }
  else
    current_image=DestroyImage(current_image);
  return(dispose_image);
}

static void ClearTransparencyCanvas(const Image *next,Image *dispose_image,
  ExceptionInfo *exception)
{
  dispose_image->page=next->page;
  dispose_image->page.x=0;
  dispose_image->page.y=0;
  dispose_image->dispose=NoneDispose;
  dispose_image->background_color.alpha_trait=BlendPixelTrait;
  dispose_image->background_color.alpha=(MagickRealType) TransparentAlpha;
  (void) SetImageBackgroundColor(dispose_image,exception);
}

MagickExport void OptimizeImageTransparency(const Image *image,
     ExceptionInfo *exception)
{
  Image
    *dispose_image;

  Image
    *next;

  /*
    Run the image through the animation sequence
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  next=GetFirstImageInList(image);
  dispose_image=CloneImage(next,next->page.width,next->page.height,
    MagickTrue,exception);
  if (dispose_image == (Image *) NULL)
    return;
  ClearTransparencyCanvas(next,dispose_image,exception);

  while ( next != (Image *) NULL )
  {
    Image
      *current_image;

    /*
      Overlay this frame's image over the previous disposal image
    */
    current_image=CloneImage(dispose_image,0,0,MagickTrue,exception);
    if (current_image == (Image *) NULL)
      {
        dispose_image=DestroyImage(dispose_image);
        return;
      }
    current_image->background_color.alpha_trait=BlendPixelTrait;
    (void) CompositeImage(current_image,next,next->alpha_trait != UndefinedPixelTrait ?
      OverCompositeOp : CopyCompositeOp,MagickTrue,next->page.x,next->page.y,
      exception);
    /*
      At this point the image would be displayed, for the delay period
    **
      Work out the disposal of the previous image
    */
    if (next->dispose == BackgroundDispose)
      {
        RectangleInfo
          bounds=next->page;

        bounds.width=next->columns;
        bounds.height=next->rows;
        bounds=ClipBoundsLeft(bounds);
        bounds=ClipBoundsRight(bounds,current_image);
        bounds=ClipBoundsTop(bounds);
        bounds=ClipBoundsBottom(bounds,current_image);
        ClearBounds(current_image,&bounds,exception);
      }
    dispose_image=KeepDisposedImage(next,dispose_image,current_image);

    /*
      Optimize Transparency of the next frame (if present)
    */
    next=GetNextImageInList(next);
    if (next != (Image *) NULL)
      (void) CompositeImage(next,dispose_image,ChangeMaskCompositeOp,
        MagickTrue,-(next->page.x),-(next->page.y),exception);
  }
  dispose_image=DestroyImage(dispose_image);
  return;
}
