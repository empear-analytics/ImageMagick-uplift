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
%                      M   M  EEEEE  RRRR    GGGG  EEEEE                      %
%                      MM MM  E      R   R  G      E                          %
%                      M M M  EEE    RRRR   G  GG  EEE                        %
%                      M   M  E      R R    G   G  E                          %
%                      M   M  EEEEE  R  R    GGG   EEEEE                      %
%                                                                             %
%                     MagickCore Image Layer Merge Method                     %
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M e r g e I m a g e L a y e r s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MergeImageLayers() composes all the image layers from the current given
%  image onward to produce a single image of the merged layers.
%
%  The inital canvas's size depends on the given LayerMethod, and is
%  initialized using the first images background color.  The images
%  are then composited onto that image in sequence using the given
%  composition that has been assigned to each individual image.
%
%  The format of the MergeImageLayers is:
%
%      Image *MergeImageLayers(Image *image,const LayerMethod method,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image list to be composited together
%
%    o method: the method of selecting the size of the initial canvas.
%
%        MergeLayer: Merge all layers onto a canvas just large enough
%           to hold all the actual images. The virtual canvas of the
%           first image is preserved but otherwise ignored.
%
%        FlattenLayer: Use the virtual canvas size of first image.
%           Images which fall outside this canvas is clipped.
%           This can be used to 'fill out' a given virtual canvas.
%
%        MosaicLayer: Start with the virtual canvas of the first image,
%           enlarging left and right edges to contain all images.
%           Images with negative offsets will be clipped.
%
%        TrimBoundsLayer: Determine the overall bounds of all the image
%           layers just as in "MergeLayer", then adjust the canvas
%           and offsets to be relative to those bounds, without overlaying
%           the images.
%
%           WARNING: a new image is not returned, the original image
%           sequence page data is modified instead.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void ExtendToLayerBounds(const Image *image,RectangleInfo *page,
  size_t *width,size_t *height)
{
  const Image
    *next;

  next=GetNextImageInList(image);
  for ( ; next != (Image *) NULL;  next=GetNextImageInList(next))
  {
    if ((*page).x > next->page.x)
      {
        *width=(size_t) ((ssize_t) *width+(*page).x-next->page.x);
        (*page).x=next->page.x;
      }
    if ((*page).y > next->page.y)
      {
        *height=(size_t) ((ssize_t) *height+(*page).y-next->page.y);
        (*page).y=next->page.y;
      }
    if ((ssize_t) *width < (next->page.x+(ssize_t) next->columns-(*page).x))
      *width=(size_t) (next->page.x+(ssize_t) next->columns-(*page).x);
    if ((ssize_t) *height < (next->page.y+(ssize_t) next->rows-(*page).y))
      *height=(size_t) (next->page.y+(ssize_t) next->rows-(*page).y);
  }
}

static void ExtendToMosaicBounds(const Image *image,size_t *width,
  size_t *height)
{
  const Image
    *next;

  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    if ((ssize_t) *width < (next->page.x+(ssize_t) next->columns))
      *width=(size_t) next->page.x+next->columns;
    if ((ssize_t) *height < (next->page.y+(ssize_t) next->rows))
      *height=(size_t) next->page.y+next->rows;
  }
}

#define MergeLayersTag  "Merge/Layers"

static Image *TrimLayerBounds(Image *image,const RectangleInfo page,
  const size_t width,const size_t height)
{
  MagickBooleanType
    proceed;

  size_t
    number_images;

  ssize_t
    scene;

  number_images=GetImageListLength(image);
  for (scene=0; scene < (ssize_t) number_images; scene++)
  {
    image->page.x-=page.x;
    image->page.y-=page.y;
    image->page.width=width;
    image->page.height=height;
    proceed=SetImageProgress(image,MergeLayersTag,(MagickOffsetType) scene,
      number_images);
    if (proceed == MagickFalse)
      break;
    image=GetNextImageInList(image);
    if (image == (Image *) NULL)
      break;
  }
  return((Image *) NULL);
}

MagickExport Image *MergeImageLayers(Image *image,const LayerMethod method,
  ExceptionInfo *exception)
{
  Image
    *canvas;

  MagickBooleanType
    proceed;

  RectangleInfo
    page;

  size_t
    number_images,
    height,
    width;

  ssize_t
    scene;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  /*
    Determine canvas image size, and its virtual canvas size and offset
  */
  page=image->page;
  width=image->columns;
  height=image->rows;
  switch (method)
  {
    case TrimBoundsLayer:
    case MergeLayer:
    default:
    {
      ExtendToLayerBounds(image,&page,&width,&height);
      break;
    }
    case FlattenLayer:
    {
      if (page.width > 0)
        width=page.width;
      if (page.height > 0)
        height=page.height;
      page.x=0;
      page.y=0;
      break;
    }
    case MosaicLayer:
    {
      if (page.width > 0)
        width=page.width;
      if (page.height > 0)
        height=page.height;
      ExtendToMosaicBounds(image,&width,&height);
      page.width=width;
      page.height=height;
      page.x=0;
      page.y=0;
    }
    break;
  }
  /*
    Set virtual canvas size if not defined.
  */
  if (page.width == 0)
    page.width=page.x < 0 ? width : (size_t) ((ssize_t) width+page.x);
  if (page.height == 0)
    page.height=page.y < 0 ? height : (size_t) ((ssize_t) height+page.y);
  /*
    Handle "TrimBoundsLayer" method separately to normal 'layer merge'.
  */
  if (method == TrimBoundsLayer)
    return(TrimLayerBounds(image,page,width,height));
  /*
    Create canvas size of width and height, and background color.
  */
  canvas=CloneImage(image,width,height,MagickTrue,exception);
  if (canvas == (Image *) NULL)
    return((Image *) NULL);
  canvas->background_color.alpha_trait=BlendPixelTrait;
  (void) SetImageBackgroundColor(canvas,exception);
  canvas->page=page;
  canvas->dispose=UndefinedDispose;
  /*
    Compose images onto canvas, with progress monitor
  */
  number_images=GetImageListLength(image);
  for (scene=0; scene < (ssize_t) number_images; scene++)
  {
    (void) CompositeImage(canvas,image,image->compose,MagickTrue,image->page.x-
      canvas->page.x,image->page.y-canvas->page.y,exception);
    proceed=SetImageProgress(image,MergeLayersTag,(MagickOffsetType) scene,
      number_images);
    if (proceed == MagickFalse)
      break;
    image=GetNextImageInList(image);
    if (image == (Image *) NULL)
      break;
  }
  return(canvas);
}
