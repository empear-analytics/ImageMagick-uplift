/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%         AAA   TTTTT  TTTTT  RRRR   IIIII  BBBB   U   U  TTTTT  EEEEE        %
%        A   A    T      T    R   R    I    B   B  U   U    T    E            %
%        AAAAA    T      T    RRRR     I    BBBB   U   U    T    EEE          %
%        A   A    T      T    R R      I    B   B  U   U    T    E            %
%        A   A    T      T    R  R   IIIII  BBBB    UUU     T    EEEEE        %
%                                                                             %
%                  BBBB    OOO   U   U  N   N  DDDD   SSSSS                   %
%                  B   B  O   O  U   U  NN  N  D   D  SS                      %
%                  BBBB   O   O  U   U  N N N  D   D   SSS                    %
%                  B   B  O   O  U   U  N  NN  D   D     SS                   %
%                  BBBB    OOO    UUU   N   N  DDDD   SSSSS                   %
%                                                                             %
%                                                                             %
%                    MagickCore Image Bounding Box Method                     %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                October 2002                                 %
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
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colormap-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/draw-private.h"
#include "MagickCore/effect.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/histogram.h"
#include "MagickCore/identify.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/magick.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/segment.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t I m a g e B o u n d i n g B o x                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageBoundingBox() returns the bounding box of an image canvas.
%
%  The format of the GetImageBoundingBox method is:
%
%      RectangleInfo GetImageBoundingBox(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o bounds: Method GetImageBoundingBox returns the bounding box of an
%      image canvas.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/

typedef struct _CensusInfo
{
  double
    left,
    right,
    top,
    bottom;
} CensusInfo;

static double GetEdgeBackgroundCensus(const Image *image,
  const CacheView *image_view,const GravityType gravity,const size_t width,
  const size_t height,const ssize_t x_offset,const ssize_t y_offset,
  ExceptionInfo *exception)
{
  CacheView
    *edge_view;

  const char
    *artifact;

  const Quantum
    *p;

  double
    census;

  Image
    *edge_image;

  PixelInfo
    background,
    pixel;

  RectangleInfo
    edge_geometry;

  ssize_t
    y;

  /*
    Determine the percent of image background for this edge.
  */
  switch (gravity)
  {
    case NorthWestGravity:
    case NorthGravity:
    default:
    {
      p=GetCacheViewVirtualPixels(image_view,0,0,1,1,exception);
      break;
    }
    case NorthEastGravity:
    case EastGravity:
    {
      p=GetCacheViewVirtualPixels(image_view,(ssize_t) image->columns-1,0,1,1,
        exception);
      break;
    }
    case SouthEastGravity:
    case SouthGravity:
    {
      p=GetCacheViewVirtualPixels(image_view,(ssize_t) image->columns-1,
        (ssize_t) image->rows-1,1,1,exception);
      break;
    }
    case SouthWestGravity:
    case WestGravity:
    {
      p=GetCacheViewVirtualPixels(image_view,0,(ssize_t) image->rows-1,1,1,
        exception);
      break;
    }
  }
  if (p == (const Quantum *) NULL)
    return(0.0);
  GetPixelInfoPixel(image,p,&background);
  artifact=GetImageArtifact(image,"background");
  if (artifact != (const char *) NULL)
    (void) QueryColorCompliance(artifact,AllCompliance,&background,exception);
  artifact=GetImageArtifact(image,"trim:background-color");
  if (artifact != (const char *) NULL)
    (void) QueryColorCompliance(artifact,AllCompliance,&background,exception);
  edge_geometry.width=width;
  edge_geometry.height=height;
  edge_geometry.x=x_offset;
  edge_geometry.y=y_offset;
  GravityAdjustGeometry(image->columns,image->rows,gravity,&edge_geometry);
  edge_image=CropImage(image,&edge_geometry,exception);
  if (edge_image == (Image *) NULL)
    return(0.0);
  census=0.0;
  edge_view=AcquireVirtualCacheView(edge_image,exception);
  for (y=0; y < (ssize_t) edge_image->rows; y++)
  {
    ssize_t
      x;

    p=GetCacheViewVirtualPixels(edge_view,0,y,edge_image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) edge_image->columns; x++)
    {
      GetPixelInfoPixel(edge_image,p,&pixel);
      if (IsFuzzyEquivalencePixelInfo(&pixel,&background) == MagickFalse)
        census++;
      p+=(ptrdiff_t) GetPixelChannels(edge_image);
    }
  }
  census/=((double) edge_image->columns*edge_image->rows);
  edge_view=DestroyCacheView(edge_view);
  edge_image=DestroyImage(edge_image);
  return(census);
}

static inline double GetMinEdgeBackgroundCensus(const CensusInfo *edge)
{
  double
    census;

  census=MagickMin(MagickMin(MagickMin(edge->left,edge->right),edge->top),
    edge->bottom);
  return(census);
}

static RectangleInfo GetEdgeBoundingBox(const Image *image,
  ExceptionInfo *exception)
{
  CacheView
    *edge_view;

  CensusInfo
    edge,
    vertex;

  const char
    *artifact;

  double
    background_census,
    percent_background;

  Image
    *edge_image;

  RectangleInfo
    bounds;

  /*
    Get the image bounding box.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  SetGeometry(image,&bounds);
  edge_image=CloneImage(image,0,0,MagickTrue,exception);
  if (edge_image == (Image *) NULL)
    return(bounds);
  (void) ParseAbsoluteGeometry("0x0+0+0",&edge_image->page);
  (void) memset(&vertex,0,sizeof(vertex));
  edge_view=AcquireVirtualCacheView(edge_image,exception);
  edge.left=GetEdgeBackgroundCensus(edge_image,edge_view,WestGravity,
    1,0,0,0,exception);
  edge.right=GetEdgeBackgroundCensus(edge_image,edge_view,EastGravity,
    1,0,0,0,exception);
  edge.top=GetEdgeBackgroundCensus(edge_image,edge_view,NorthGravity,
    0,1,0,0,exception);
  edge.bottom=GetEdgeBackgroundCensus(edge_image,edge_view,SouthGravity,
    0,1,0,0,exception);
  percent_background=1.0;
  artifact=GetImageArtifact(edge_image,"trim:percent-background");
  if (artifact != (const char *) NULL)
    percent_background=StringToDouble(artifact,(char **) NULL)/100.0;
  percent_background=MagickMin(MagickMax(1.0-percent_background,MagickEpsilon),
    1.0);
  background_census=GetMinEdgeBackgroundCensus(&edge);
  for ( ; background_census < percent_background;
          background_census=GetMinEdgeBackgroundCensus(&edge))
  {
    if ((bounds.width == 0) || (bounds.height == 0))
      break;
    if (fabs(edge.left-background_census) < MagickEpsilon)
      {
        /*
          Trim left edge.
        */
        vertex.left++;
        bounds.width--;
        edge.left=GetEdgeBackgroundCensus(edge_image,edge_view,
          NorthWestGravity,1,bounds.height,(ssize_t) vertex.left,(ssize_t)
          vertex.top,exception);
        edge.top=GetEdgeBackgroundCensus(edge_image,edge_view,
          NorthWestGravity,bounds.width,1,(ssize_t) vertex.left,(ssize_t)
          vertex.top,exception);
        edge.bottom=GetEdgeBackgroundCensus(edge_image,edge_view,
          SouthWestGravity,bounds.width,1,(ssize_t) vertex.left,(ssize_t)
          vertex.bottom,exception);
        continue;
      }
    if (fabs(edge.right-background_census) < MagickEpsilon)
      {
        /*
          Trim right edge.
        */
        vertex.right++;
        bounds.width--;
        edge.right=GetEdgeBackgroundCensus(edge_image,edge_view,
          NorthEastGravity,1,bounds.height,(ssize_t) vertex.right,(ssize_t)
          vertex.top,exception);
        edge.top=GetEdgeBackgroundCensus(edge_image,edge_view,
          NorthWestGravity,bounds.width,1,(ssize_t) vertex.left,(ssize_t)
          vertex.top,exception);
        edge.bottom=GetEdgeBackgroundCensus(edge_image,edge_view,
          SouthWestGravity,bounds.width,1,(ssize_t) vertex.left,(ssize_t)
          vertex.bottom,exception);
        continue;
      }
    if (fabs(edge.top-background_census) < MagickEpsilon)
      {
        /*
          Trim top edge.
        */
        vertex.top++;
        bounds.height--;
        edge.left=GetEdgeBackgroundCensus(edge_image,edge_view,
          NorthWestGravity,1,bounds.height,(ssize_t) vertex.left,(ssize_t)
          vertex.top,exception);
        edge.right=GetEdgeBackgroundCensus(edge_image,edge_view,
          NorthEastGravity,1,bounds.height,(ssize_t) vertex.right,(ssize_t)
          vertex.top,exception);
        edge.top=GetEdgeBackgroundCensus(edge_image,edge_view,
          NorthWestGravity,bounds.width,1,(ssize_t) vertex.left,(ssize_t)
          vertex.top,exception);
        continue;
      }
    if (fabs(edge.bottom-background_census) < MagickEpsilon)
      {
        /*
          Trim bottom edge.
        */
        vertex.bottom++;
        bounds.height--;
        edge.left=GetEdgeBackgroundCensus(edge_image,edge_view,
          NorthWestGravity,1,bounds.height,(ssize_t) vertex.left,(ssize_t)
          vertex.top,exception);
        edge.right=GetEdgeBackgroundCensus(edge_image,edge_view,
          NorthEastGravity,1,bounds.height,(ssize_t) vertex.right,(ssize_t)
          vertex.top,exception);
        edge.bottom=GetEdgeBackgroundCensus(edge_image,edge_view,
          SouthWestGravity,bounds.width,1,(ssize_t) vertex.left,(ssize_t)
          vertex.bottom,exception);
        continue;
      }
  }
  edge_view=DestroyCacheView(edge_view);
  edge_image=DestroyImage(edge_image);
  bounds.x=(ssize_t) vertex.left;
  bounds.y=(ssize_t) vertex.top;
  if ((bounds.width == 0) || (bounds.height == 0))
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
      "GeometryDoesNotContainImage","`%s'",image->filename);
  return(bounds);
}

static inline MagickBooleanType IsEmptyBoundingBox(const RectangleInfo bounds)
{
  return(((bounds.width == 0) || (bounds.height == 0) ||
    (bounds.x > (ssize_t) bounds.width) ||
    (bounds.y > (ssize_t) bounds.height)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsBelowLeftOfBounds(const ssize_t x,
  const ssize_t y,const RectangleInfo bounding_box)
{
  return(((x < (ssize_t) bounding_box.width) &&
    (y > (ssize_t) bounding_box.height)) ? MagickTrue : MagickFalse);
}

MagickExport RectangleInfo GetImageBoundingBox(const Image *image,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  const char
    *artifact;

  const Quantum
    *p;

  MagickBooleanType
    status;

  PixelInfo
    target[4],
    zero;

  RectangleInfo
    bounds;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  artifact=GetImageArtifact(image,"trim:percent-background");
  if (artifact != (const char *) NULL)
    return(GetEdgeBoundingBox(image,exception));
  artifact=GetImageArtifact(image,"trim:edges");
  if (artifact == (const char *) NULL)
    {
      bounds.width=(size_t) (image->columns == 1 ? 1 : 0);
      bounds.height=(size_t) (image->rows == 1 ? 1 : 0);
      bounds.x=(ssize_t) image->columns;
      bounds.y=(ssize_t) image->rows;
    }
  else
    {
      char
        *edges,
        *q,
        *r;

      bounds.width=(size_t) image->columns;
      bounds.height=(size_t) image->rows;
      bounds.x=0;
      bounds.y=0;
      edges=AcquireString(artifact);
      r=edges;
      while ((q=StringToken(",",&r)) != (char *) NULL)
      {
        if (LocaleCompare(q,"north") == 0)
          bounds.y=(ssize_t) image->rows;
        if (LocaleCompare(q,"east") == 0)
          bounds.width=0;
        if (LocaleCompare(q,"south") == 0)
          bounds.height=0;
        if (LocaleCompare(q,"west") == 0)
          bounds.x=(ssize_t) image->columns;
      }
      edges=DestroyString(edges);
    }
  GetPixelInfo(image,&target[0]);
  image_view=AcquireVirtualCacheView(image,exception);
  p=GetCacheViewVirtualPixels(image_view,0,0,1,1,exception);
  if (p == (const Quantum *) NULL)
    {
      image_view=DestroyCacheView(image_view);
      return(bounds);
    }
  GetPixelInfoPixel(image,p,&target[0]);
  GetPixelInfo(image,&target[1]);
  p=GetCacheViewVirtualPixels(image_view,(ssize_t) image->columns-1,0,1,1,
    exception);
  if (p != (const Quantum *) NULL)
    GetPixelInfoPixel(image,p,&target[1]);
  GetPixelInfo(image,&target[2]);
  p=GetCacheViewVirtualPixels(image_view,0,(ssize_t) image->rows-1,1,1,
    exception);
  if (p != (const Quantum *) NULL)
    GetPixelInfoPixel(image,p,&target[2]);
  GetPixelInfo(image,&target[3]);
  p=GetCacheViewVirtualPixels(image_view,(ssize_t) image->columns-1,(ssize_t)
    image->rows-1,1,1,exception);
  if (p != (const Quantum *) NULL)
    GetPixelInfoPixel(image,p,&target[3]);
  status=MagickTrue;
  GetPixelInfo(image,&zero);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict q;

    PixelInfo
      pixel;

    RectangleInfo
      bounding_box;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
#  pragma omp critical (MagickCore_GetImageBoundingBox)
#endif
    bounding_box=bounds;
    q=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (q == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    pixel=zero;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      GetPixelInfoPixel(image,q,&pixel);
      if ((x < bounding_box.x) &&
          (IsFuzzyEquivalencePixelInfo(&pixel,&target[0]) == MagickFalse))
        bounding_box.x=x;
      if ((x > (ssize_t) bounding_box.width) &&
          (IsFuzzyEquivalencePixelInfo(&pixel,&target[1]) == MagickFalse))
        bounding_box.width=(size_t) x;
      if ((y < bounding_box.y) &&
          (IsFuzzyEquivalencePixelInfo(&pixel,&target[0]) == MagickFalse))
        bounding_box.y=y;
      if ((y > (ssize_t) bounding_box.height) &&
          (IsFuzzyEquivalencePixelInfo(&pixel,&target[2]) == MagickFalse))
        bounding_box.height=(size_t) y;
      if ((IsBelowLeftOfBounds(x,y,bounding_box) != MagickFalse) &&
          (IsFuzzyEquivalencePixelInfo(&pixel,&target[3]) == MagickFalse))
        {
          bounding_box.width=(size_t) x;
          bounding_box.height=(size_t) y;
        }
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
#  pragma omp critical (MagickCore_GetImageBoundingBox)
#endif
    {
      if (bounding_box.x < bounds.x)
        bounds.x=bounding_box.x;
      if (bounding_box.y < bounds.y)
        bounds.y=bounding_box.y;
      if (bounding_box.width > bounds.width)
        bounds.width=bounding_box.width;
      if (bounding_box.height > bounds.height)
        bounds.height=bounding_box.height;
    }
  }
  image_view=DestroyCacheView(image_view);
  if (IsEmptyBoundingBox(bounds) != MagickFalse)
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
      "GeometryDoesNotContainImage","`%s'",image->filename);
  else
    {
      bounds.width-=(size_t) (bounds.x-1);
      bounds.height-=(size_t) (bounds.y-1);
    }
  return(bounds);
}
