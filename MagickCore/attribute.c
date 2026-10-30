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
%                                                                             %
%                    MagickCore Get / Set Image Attributes                    %
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
%   G e t I m a g e C o n v e x H u l l                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageConvexHull() returns the convex hull points of an image canvas.
%
%  The format of the GetImageConvexHull method is:
%
%      PointInfo *GetImageConvexHull(const Image *image,
%        size_t number_vertices,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o number_vertices: the number of vertices in the convex hull.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static double LexicographicalOrder(PointInfo *a,PointInfo *b,PointInfo *c)
{
  /*
    Order by x-coordinate, and in case of a tie, by y-coordinate.
  */
  return((b->x-a->x)*(c->y-a->y)-(b->y-a->y)*(c->x-a->x));
}

typedef struct _ImageEdge
{
  const Image
    *image;

  const CacheView
    *image_view;

  ssize_t
    i;

  ExceptionInfo
    *exception;
} ImageEdge;

static void SelectImageEdge(const ImageEdge *edge,const Quantum **p,
  GravityType *gravity,RectangleInfo *edge_geometry)
{
  const Image
    *image = edge->image;

  const CacheView
    *image_view = edge->image_view;

  const ssize_t
    i = edge->i;

  ExceptionInfo
    *exception = edge->exception;

  switch (i)
  {
    case 0:
    default:
    {
      (*p)=GetCacheViewVirtualPixels(image_view,0,(ssize_t) image->rows-1,1,1,
        exception);
      (*gravity)=WestGravity;
      (*edge_geometry).width=1;
      (*edge_geometry).height=0;
      break;
    }
    case 1:
    {
      (*p)=GetCacheViewVirtualPixels(image_view,(ssize_t) image->columns-1,0,1,
        1,exception);
      (*gravity)=EastGravity;
      (*edge_geometry).width=1;
      (*edge_geometry).height=0;
      break;
    }
    case 2:
    {
      (*p)=GetCacheViewVirtualPixels(image_view,0,0,1,1,exception);
      (*gravity)=NorthGravity;
      (*edge_geometry).width=0;
      (*edge_geometry).height=1;
      break;
    }
    case 3:
    {
      (*p)=GetCacheViewVirtualPixels(image_view,(ssize_t) image->columns-1,
        (ssize_t) image->rows-1,1,1,exception);
      (*gravity)=SouthGravity;
      (*edge_geometry).width=0;
      (*edge_geometry).height=1;
      break;
    }
  }
}

static PixelInfo GetDominantEdgeBackground(const double *census,
  const PixelInfo *background,PixelInfo edge_background)
{
  double
    edge_census;

  ssize_t
    i;

  edge_census=(-1.0);
  for (i=0; i < 4; i++)
    if (census[i] > edge_census)
      {
        edge_background=background[i];
        edge_census=census[i];
      }
  return(edge_background);
}

static PixelInfo GetEdgeBackgroundColor(const Image *image,
  const CacheView *image_view,ExceptionInfo *exception)
{
  const char
    *artifact;

  double
    census[4];

  PixelInfo
    background[4],
    edge_background;

  ssize_t
    i;

  /*
    Most dominant color of edges/corners is the background color of the image.
  */
  memset(&edge_background,0,sizeof(edge_background));
  artifact=GetImageArtifact(image,"convex-hull:background-color");
  if (artifact == (const char *) NULL)
    artifact=GetImageArtifact(image,"background");
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static)
#endif
  for (i=0; i < 4; i++)
  {
    CacheView
      *edge_view;

    GravityType
      gravity;

    Image
      *edge_image;

    ImageEdge
      edge;

    PixelInfo
      pixel;

    RectangleInfo
      edge_geometry;

    const Quantum
      *p;

    ssize_t
      y;

    census[i]=0.0;
    (void) memset(&edge_geometry,0,sizeof(edge_geometry));
    edge.image=image;
    edge.image_view=image_view;
    edge.i=i;
    edge.exception=exception;
    SelectImageEdge(&edge,&p,&gravity,&edge_geometry);
    GetPixelInfoPixel(image,p,background+i);
    if (artifact != (const char *) NULL)
      (void) QueryColorCompliance(artifact,AllCompliance,background+i,
        exception);
    GravityAdjustGeometry(image->columns,image->rows,gravity,&edge_geometry);
    edge_image=CropImage(image,&edge_geometry,exception);
    if (edge_image == (Image *) NULL)
      continue;
    edge_view=AcquireVirtualCacheView(edge_image,exception);
    for (y=0; y < (ssize_t) edge_image->rows; y++)
    {
      ssize_t
        x;

      p=GetCacheViewVirtualPixels(edge_view,0,y,edge_image->columns,1,
        exception);
      if (p == (const Quantum *) NULL)
        break;
      for (x=0; x < (ssize_t) edge_image->columns; x++)
      {
        GetPixelInfoPixel(edge_image,p,&pixel);
        if (IsFuzzyEquivalencePixelInfo(&pixel,background+i) == MagickFalse)
          census[i]++;
        p+=(ptrdiff_t) GetPixelChannels(edge_image);
      }
    }
    edge_view=DestroyCacheView(edge_view);
    edge_image=DestroyImage(edge_image);
  }
  return(GetDominantEdgeBackground(census,background,edge_background));
}

void TraceConvexHull(PointInfo *vertices,size_t number_vertices,
  PointInfo ***monotone_chain,size_t *chain_length)
{
  PointInfo
    **chain;

  size_t
    demark,
    n;

  ssize_t
    i;

  /*
    Construct the upper and lower hulls: rightmost to leftmost counterclockwise.
  */
  chain=(*monotone_chain);
  n=0;
  for (i=0; i < (ssize_t) number_vertices; i++)
  {
    while ((n >= 2) &&
           (LexicographicalOrder(chain[n-2],chain[n-1],&vertices[i]) <= 0.0))
      n--;
    chain[n++]=(&vertices[i]);
  }
  demark=n+1;
  for (i=(ssize_t) number_vertices-2; i >= 0; i--)
  {
    while ((n >= demark) &&
           (LexicographicalOrder(chain[n-2],chain[n-1],&vertices[i]) <= 0.0))
      n--;
    chain[n++]=(&vertices[i]);
  }
  *chain_length=n;
}

MagickExport PointInfo *GetImageConvexHull(const Image *image,
  size_t *number_vertices,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  MemoryInfo
    *monotone_info,
    *vertices_info;

  PixelInfo
    background;

  PointInfo
    *convex_hull,
    **monotone_chain,
    *vertices;

  size_t
    n;

  ssize_t
    y;

  /*
    Identify convex hull vertices of image foreground object(s).
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  *number_vertices=0;
  vertices_info=AcquireVirtualMemory(image->columns,image->rows*
    sizeof(*vertices));
  monotone_info=AcquireVirtualMemory(2*image->columns,2*
    image->rows*sizeof(*monotone_chain));
  if ((vertices_info == (MemoryInfo *) NULL) ||
      (monotone_info == (MemoryInfo *) NULL))
    {
      if (monotone_info != (MemoryInfo *) NULL)
        monotone_info=(MemoryInfo *) RelinquishVirtualMemory(monotone_info);
      if (vertices_info != (MemoryInfo *) NULL)
        vertices_info=RelinquishVirtualMemory(vertices_info);
      return((PointInfo *) NULL);
    }
  vertices=(PointInfo *) GetVirtualMemoryBlob(vertices_info);
  monotone_chain=(PointInfo **) GetVirtualMemoryBlob(monotone_info);
  image_view=AcquireVirtualCacheView(image,exception);
  background=GetEdgeBackgroundColor(image,image_view,exception);
  status=MagickTrue;
  n=0;
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *p;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      PixelInfo
        pixel;

      GetPixelInfoPixel(image,p,&pixel);
      if (IsFuzzyEquivalencePixelInfo(&pixel,&background) == MagickFalse)
        {
          vertices[n].x=(double) x;
          vertices[n].y=(double) y;
          n++;
        }
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  image_view=DestroyCacheView(image_view);
  /*
    Return the convex hull of the image foreground object(s).
  */
  TraceConvexHull(vertices,n,&monotone_chain,number_vertices);
  convex_hull=(PointInfo *) AcquireQuantumMemory(*number_vertices,
    sizeof(*convex_hull));
  if (convex_hull != (PointInfo *) NULL)
    for (n=0; n < *number_vertices; n++)
      convex_hull[n]=(*monotone_chain[n]);
  monotone_info=RelinquishVirtualMemory(monotone_info);
  vertices_info=RelinquishVirtualMemory(vertices_info);
  return(convex_hull);
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e Q u a n t u m D e p t h                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageQuantumDepth() returns the depth of the image rounded to a legal
%  quantum depth: 8, 16, or 32.
%
%  The format of the GetImageQuantumDepth method is:
%
%      size_t GetImageQuantumDepth(const Image *image,
%        const MagickBooleanType constrain)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o constrain: A value other than MagickFalse, constrains the depth to
%      a maximum of MAGICKCORE_QUANTUM_DEPTH.
%
*/
MagickExport size_t GetImageQuantumDepth(const Image *image,
  const MagickBooleanType constrain)
{
  size_t
    depth;

  depth=image->depth;
  if (depth <= 8)
    depth=8;
  else
    if (depth <= 16)
      depth=16;
    else
      if (depth <= 32)
        depth=32;
      else
        if (depth <= 64)
          depth=64;
  if (constrain != MagickFalse)
    depth=(size_t) MagickMin((double) depth,(double) MAGICKCORE_QUANTUM_DEPTH);
  return(depth);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e T y p e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageType() returns the type of image:
%
%        Bilevel         Grayscale        GrayscaleMatte
%        Palette         PaletteMatte     TrueColor
%        TrueColorMatte  ColorSeparation  ColorSeparationMatte
%
%  The format of the GetImageType method is:
%
%      ImageType GetImageType(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
static ImageType GetColorSeparationType(const Image *image)
{
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    return(ColorSeparationType);
  return(ColorSeparationAlphaType);
}

static ImageType GetPaletteType(const Image *image)
{
  if (image->alpha_trait != UndefinedPixelTrait)
    return(PaletteAlphaType);
  return(PaletteType);
}

MagickExport ImageType GetImageType(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->colorspace == CMYKColorspace)
    return(GetColorSeparationType(image));
  if (IsImageMonochrome(image) != MagickFalse)
    return(BilevelType);
  if (IsImageGray(image) != MagickFalse)
    {
      if (image->alpha_trait != UndefinedPixelTrait)
        return(GrayscaleAlphaType);
      return(GrayscaleType);
    }
  if (IsPaletteImage(image) != MagickFalse)
    return(GetPaletteType(image));
  if (image->alpha_trait != UndefinedPixelTrait)
    return(TrueColorAlphaType);
  return(TrueColorType);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     I d e n t i f y I m a g e G r a y                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IdentifyImageGray() returns grayscale if all the pixels in the image have
%  the same red, green, and blue intensities, and bi-level if the intensity is
%  either 0 or QuantumRange. Otherwise undefined is returned.
%
%  The format of the IdentifyImageGray method is:
%
%      ImageType IdentifyImageGray(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport ImageType IdentifyImageGray(const Image *image,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  ImageType
    type = BilevelType;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (IsImageGray(image) != MagickFalse)
    return(image->type);
  if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
    return(UndefinedType);
  image_view=AcquireVirtualCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status,type) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *p;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      if (IsPixelGray(image,p) == MagickFalse)
        {
          status=MagickFalse;
          break;
        }
      if ((type == BilevelType) && (IsPixelMonochrome(image,p) == MagickFalse))
        type=GrayscaleType;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  image_view=DestroyCacheView(image_view);
  if ((type == GrayscaleType) && (image->alpha_trait != UndefinedPixelTrait))
    type=GrayscaleAlphaType;
  if (status == MagickFalse)
    return(UndefinedType);
  return(type);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I d e n t i f y I m a g e M o n o c h r o m e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IdentifyImageMonochrome() returns MagickTrue if all the pixels in the image
%  have the same red, green, and blue intensities and the intensity is either
%  0 or QuantumRange.
%
%  The format of the IdentifyImageMonochrome method is:
%
%      MagickBooleanType IdentifyImageMonochrome(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType IdentifyImageMonochrome(const Image *image,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  ImageType
    type = BilevelType;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->type == BilevelType)
    return(MagickTrue);
  if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
    return(MagickFalse);
  image_view=AcquireVirtualCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(type) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *p;

    ssize_t
      x;

    if (type == UndefinedType)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        type=UndefinedType;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      if (IsPixelMonochrome(image,p) == MagickFalse)
        {
          type=UndefinedType;
          break;
        }
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  image_view=DestroyCacheView(image_view);
  return(type == BilevelType ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I d e n t i f y I m a g e T y p e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IdentifyImageType() returns the potential type of image:
%
%        Bilevel         Grayscale        GrayscaleMatte
%        Palette         PaletteMatte     TrueColor
%        TrueColorMatte  ColorSeparation  ColorSeparationMatte
%
%  To ensure the image type matches its potential, use SetImageType():
%
%    (void) SetImageType(image,IdentifyImageType(image,exception),exception);
%
%  The format of the IdentifyImageType method is:
%
%      ImageType IdentifyImageType(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport ImageType IdentifyImageType(const Image *image,
  ExceptionInfo *exception)
{
  ImageType
    type;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->colorspace == CMYKColorspace)
    return(GetColorSeparationType(image));
  type=IdentifyImageGray(image,exception);
  if (IsGrayImageType(type))
    return(type);
  if (IdentifyPaletteImage(image,exception) != MagickFalse)
    {
      if (image->alpha_trait != UndefinedPixelTrait)
        return(PaletteAlphaType);
      return(PaletteType);
    }
  if (image->alpha_trait != UndefinedPixelTrait)
    return(TrueColorAlphaType);
  return(TrueColorType);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     I s I m a g e G r a y                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsImageGray() returns MagickTrue if the type of the image is grayscale or
%  bi-level.
%
%  The format of the IsImageGray method is:
%
%      MagickBooleanType IsImageGray(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport MagickBooleanType IsImageGray(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsGrayImageType(image->type) != MagickFalse)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s I m a g e M o n o c h r o m e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsImageMonochrome() returns MagickTrue if type of the image is bi-level.
%
%  The format of the IsImageMonochrome method is:
%
%      MagickBooleanType IsImageMonochrome(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport MagickBooleanType IsImageMonochrome(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->type == BilevelType)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     I s I m a g e O p a q u e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsImageOpaque() returns MagickTrue if none of the pixels in the image have
%  an alpha value other than OpaqueAlpha (QuantumRange).
%
%  Will return true immediately is alpha channel is not available.
%
%  The format of the IsImageOpaque method is:
%
%      MagickBooleanType IsImageOpaque(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType IsImageRowOpaque(const Image *image,const Quantum *p)
{
  MagickBooleanType
    status;

  ssize_t
    x;

  status=MagickTrue;
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (GetPixelAlpha(image,p) != OpaqueAlpha)
      {
        status=MagickFalse;
        break;
      }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(status);
}

MagickExport MagickBooleanType IsImageOpaque(const Image *image,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    opaque = MagickTrue;

  ssize_t
    y;

  /*
    Determine if image is opaque.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    return(MagickTrue);
  image_view=AcquireVirtualCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(opaque) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *p;

    if (opaque == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        opaque=MagickFalse;
        continue;
      }
    if (IsImageRowOpaque(image,p) == MagickFalse)
      opaque=MagickFalse;
  }
  image_view=DestroyCacheView(image_view);
  return(opaque);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e T y p e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageType() sets the type of image.  Choose from these types:
%
%        Bilevel        Grayscale       GrayscaleMatte
%        Palette        PaletteMatte    TrueColor
%        TrueColorMatte ColorSeparation ColorSeparationMatte
%        OptimizeType
%
%  The format of the SetImageType method is:
%
%      MagickBooleanType SetImageType(Image *image,const ImageType type,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o type: Image type.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SetOpaqueAlphaIfMissing(Image *image,ExceptionInfo *exception)
{
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
}

static MagickBooleanType TransformImageTosRGB(Image *image,
  MagickBooleanType status,ExceptionInfo *exception)
{
  if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
    status=TransformImageColorspace(image,sRGBColorspace,exception);
  return(status);
}

static MagickBooleanType SetImageDirectClass(Image *image,
  MagickBooleanType status,ExceptionInfo *exception)
{
  if (image->storage_class != DirectClass)
    status=SetImageStorageClass(image,DirectClass,exception);
  return(status);
}

static MagickBooleanType SetImageTypeBilevel(Image *image,
  const ImageInfo *image_info,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  QuantizeInfo
    *quantize_info;

  if (IsGrayImageType(image->type) == MagickFalse)
    status=TransformImageColorspace(image,GRAYColorspace,exception);
  (void) NormalizeImage(image,exception);
  (void) BilevelImage(image,(double) QuantumRange/2.0,exception);
  quantize_info=AcquireQuantizeInfo(image_info);
  quantize_info->number_colors=2;
  quantize_info->colorspace=GRAYColorspace;
  status=QuantizeImage(quantize_info,image,exception);
  quantize_info=DestroyQuantizeInfo(quantize_info);
  image->alpha_trait=UndefinedPixelTrait;
  return(status);
}

MagickExport MagickBooleanType SetImageType(Image *image,const ImageType type,
  ExceptionInfo *exception)
{
  const char
    *artifact;

  ImageInfo
    *image_info;

  MagickBooleanType
    status;

  QuantizeInfo
    *quantize_info;

  assert(image != (Image *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickCoreSignature);
  status=MagickTrue;
  image_info=AcquireImageInfo();
  image_info->dither=image->dither;
  artifact=GetImageArtifact(image,"dither");
  if (artifact != (const char *) NULL)
    (void) SetImageOption(image_info,"dither",artifact);
  switch (type)
  {
    case BilevelType:
    {
      status=SetImageTypeBilevel(image,image_info,exception);
      break;
    }
    case GrayscaleType:
    {
      if (IsGrayImageType(image->type) == MagickFalse)
        status=TransformImageColorspace(image,GRAYColorspace,exception);
      image->alpha_trait=UndefinedPixelTrait;
      break;
    }
    case GrayscaleAlphaType:
    {
      if (IsGrayImageType(image->type) == MagickFalse)
        status=TransformImageColorspace(image,GRAYColorspace,exception);
      SetOpaqueAlphaIfMissing(image,exception);
      break;
    }
    case PaletteType:
    {
      status=TransformImageTosRGB(image,status,exception);
      if ((image->storage_class == DirectClass) || (image->colors > 256))
        {
          quantize_info=AcquireQuantizeInfo(image_info);
          quantize_info->number_colors=256;
          status=QuantizeImage(quantize_info,image,exception);
          quantize_info=DestroyQuantizeInfo(quantize_info);
        }
      image->alpha_trait=UndefinedPixelTrait;
      break;
    }
    case PaletteBilevelAlphaType:
    {
      ChannelType
        channel_mask;

      status=TransformImageTosRGB(image,status,exception);
      SetOpaqueAlphaIfMissing(image,exception);
      channel_mask=SetImageChannelMask(image,AlphaChannel);
      (void) BilevelImage(image,(double) QuantumRange/2.0,exception);
      (void) SetImageChannelMask(image,channel_mask);
      quantize_info=AcquireQuantizeInfo(image_info);
      status=QuantizeImage(quantize_info,image,exception);
      quantize_info=DestroyQuantizeInfo(quantize_info);
      break;
    }
    case PaletteAlphaType:
    {
      status=TransformImageTosRGB(image,status,exception);
      SetOpaqueAlphaIfMissing(image,exception);
      quantize_info=AcquireQuantizeInfo(image_info);
      status=QuantizeImage(quantize_info,image,exception);
      quantize_info=DestroyQuantizeInfo(quantize_info);
      break;
    }
    case TrueColorType:
    {
      status=TransformImageTosRGB(image,status,exception);
      status=SetImageDirectClass(image,status,exception);
      image->alpha_trait=UndefinedPixelTrait;
      break;
    }
    case TrueColorAlphaType:
    {
      status=TransformImageTosRGB(image,status,exception);
      status=SetImageDirectClass(image,status,exception);
      SetOpaqueAlphaIfMissing(image,exception);
      break;
    }
    case ColorSeparationType:
    {
      if (image->colorspace != CMYKColorspace)
        status=TransformImageColorspace(image,CMYKColorspace,exception);
      status=SetImageDirectClass(image,status,exception);
      image->alpha_trait=UndefinedPixelTrait;
      break;
    }
    case ColorSeparationAlphaType:
    {
      if (image->colorspace != CMYKColorspace)
        status=TransformImageColorspace(image,CMYKColorspace,exception);
      status=SetImageDirectClass(image,status,exception);
      if ((image->alpha_trait & BlendPixelTrait) == 0)
        status=SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
      break;
    }
    case OptimizeType:
    case UndefinedType:
      break;
  }
  image_info=DestroyImageInfo(image_info);
  if (status == MagickFalse)
    return(status);
  image->type=type;
  return(MagickTrue);
}
