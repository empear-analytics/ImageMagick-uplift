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
%         CCCC   OOO   M   M  PPPP    OOO   SSSSS  IIIII  TTTTT  EEEEE        %
%        C      O   O  MM MM  P   P  O   O  SS       I      T    E            %
%        C      O   O  M M M  PPPP   O   O   SSS     I      T    EEE          %
%        C      O   O  M   M  P      O   O     SS    I      T    E            %
%         CCCC   OOO   M   M  P       OOO   SSSSS  IIIII    T    EEEEE        %
%                                                                             %
%                              Composite Layers                               %
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
%     C o m p o s i t e L a y e r s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompositeLayers() compose the source image sequence over the destination
%  image sequence, starting with the current image in both lists.
%
%  Each layer from the two image lists are composted together until the end of
%  one of the image lists is reached.  The offset of each composition is also
%  adjusted to match the virtual canvas offsets of each layer. As such the
%  given offset is relative to the virtual canvas, and not the actual image.
%
%  Composition uses given x and y offsets, as the 'origin' location of the
%  source images virtual canvas (not the real image) allowing you to compose a
%  list of 'layer images' into the destination images.  This makes it well
%  suitable for directly composing 'Clears Frame Animations' or 'Coalesced
%  Animations' onto a static or other 'Coalesced Animation' destination image
%  list.  GIF disposal handling is not looked at.
%
%  Special case:- If one of the image sequences is the last image (just a
%  single image remaining), that image is repeatedly composed with all the
%  images in the other image list.  Either the source or destination lists may
%  be the single image, for this situation.
%
%  In the case of a single destination image (or last image given), that image
%  will ve cloned to match the number of images remaining in the source image
%  list.
%
%  This is equivalent to the "-layer Composite" Shell API operator.
%
%
%  The format of the CompositeLayers method is:
%
%      void CompositeLayers(Image *destination, const CompositeOperator
%      compose, Image *source, const ssize_t x_offset, const ssize_t y_offset,
%      ExceptionInfo *exception);
%
%  A description of each parameter follows:
%
%    o destination: the destination images and results
%
%    o source: source image(s) for the layer composition
%
%    o compose, x_offset, y_offset:  arguments passed on to CompositeImages()
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline void CompositeCanvas(Image *destination,
  const CompositeOperator compose,Image *source,ssize_t x_offset,
  ssize_t y_offset,ExceptionInfo *exception)
{
  const char
    *value;

  x_offset+=source->page.x-destination->page.x;
  y_offset+=source->page.y-destination->page.y;
  value=GetImageArtifact(source,"compose:outside-overlay");
  (void) CompositeImage(destination,source,compose,
    (value != (const char *) NULL) && (IsStringTrue(value) != MagickFalse) ?
    MagickFalse : MagickTrue,x_offset,y_offset,exception);
}

typedef struct _LayerCompositeInfo
{
  CompositeOperator
    compose;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} LayerCompositeInfo;

static void CompositeSourceListOverCanvas(Image *destination,Image *source,
  const LayerCompositeInfo *composite)
{
  const CompositeOperator
    compose = composite->compose;

  const ssize_t
    x_offset = composite->x_offset,
    y_offset = composite->y_offset;

  ExceptionInfo
    *const exception = composite->exception;

  Image *dest = CloneImage(destination,0,0,MagickTrue,exception);

  if (dest != (Image *) NULL)
    {
      dest->background_color.alpha_trait=BlendPixelTrait;
      CompositeCanvas(destination, compose, source, x_offset, y_offset,
        exception);
      /* copy source image attributes ? */
      if ( source->next != (Image *) NULL )
        {
          destination->delay=source->delay;
          destination->iterations=source->iterations;
        }
      source=GetNextImageInList(source);
      while (source != (Image *) NULL)
      {
        AppendImageToList(&destination,
          CloneImage(dest,0,0,MagickTrue,exception));
        destination->background_color.alpha_trait=BlendPixelTrait;
        destination=GetLastImageInList(destination);
        CompositeCanvas(destination,compose,source,x_offset,y_offset,
          exception);
        destination->delay=source->delay;
        destination->iterations=source->iterations;
        source=GetNextImageInList(source);
      }
      dest=DestroyImage(dest);
    }
}

static void CompositeSourceOverList(Image *destination,Image *source,
  const LayerCompositeInfo *composite)
{
  const CompositeOperator
    compose = composite->compose;

  const ssize_t
    x_offset = composite->x_offset,
    y_offset = composite->y_offset;

  ExceptionInfo
    *const exception = composite->exception;

  while ( destination != (Image *) NULL )
  {
    CompositeCanvas(destination, compose, source, x_offset, y_offset,
      exception);
    destination=GetNextImageInList(destination);
  }
}

static void CompositeSourceListOverList(Image *destination,Image *source,
  const LayerCompositeInfo *composite)
{
  const CompositeOperator
    compose = composite->compose;

  const ssize_t
    x_offset = composite->x_offset,
    y_offset = composite->y_offset;

  ExceptionInfo
    *const exception = composite->exception;

  while ( source != (Image *) NULL && destination != (Image *) NULL )
  {
    CompositeCanvas(destination, compose, source, x_offset, y_offset,
      exception);
    source=GetNextImageInList(source);
    destination=GetNextImageInList(destination);
  }
}

MagickExport void CompositeLayers(Image *destination,
  const CompositeOperator compose, Image *source,const ssize_t x_offset,
  const ssize_t y_offset,ExceptionInfo *exception)
{
  assert(destination != (Image *) NULL);
  assert(destination->signature == MagickCoreSignature);
  assert(source != (Image *) NULL);
  assert(source->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s - %s",
      source->filename,destination->filename);
  /*
    Overlay single source image over destination image/list
  */
  if ( source->next == (Image *) NULL )
  {
    LayerCompositeInfo
      composite;

    composite.compose=compose;
    composite.x_offset=x_offset;
    composite.y_offset=y_offset;
    composite.exception=exception;
    CompositeSourceOverList(destination,source,&composite);
  }

  /*
    Overlay source image list over single destination.
    Multiple clones of destination image are created to match source list.
    Original Destination image becomes first image of generated list.
    As such the image list pointer does not require any change in caller.
    Some animation attributes however also needs coping in this case.
  */
  else if ( destination->next == (Image *) NULL )
  {
    LayerCompositeInfo
      composite;

    composite.compose=compose;
    composite.x_offset=x_offset;
    composite.y_offset=y_offset;
    composite.exception=exception;
    CompositeSourceListOverCanvas(destination,source,&composite);
  }

  /*
    Overlay a source image list over a destination image list
    until either list runs out of images. (Does not repeat)
  */
  else
  {
    LayerCompositeInfo
      composite;

    composite.compose=compose;
    composite.x_offset=x_offset;
    composite.y_offset=y_offset;
    composite.exception=exception;
    CompositeSourceListOverList(destination,source,&composite);
  }
}
