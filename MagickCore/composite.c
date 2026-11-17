/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        CCCC   OOO   M   M  PPPP    OOO   SSSSS  IIIII  TTTTT  EEEEE         %
%       C      O   O  MM MM  P   P  O   O  SS       I      T    E             %
%       C      O   O  M M M  PPPP   O   O   SSS     I      T    EEE           %
%       C      O   O  M   M  P      O   O     SS    I      T    E             %
%        CCCC   OOO   M   M  P       OOO   SSSSS  IIIII    T    EEEEE         %
%                                                                             %
%                                                                             %
%                     MagickCore Image Composite Methods                      %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
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
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/morphology.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/resample.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/composite-row-private.h"


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     T e x t u r e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TextureImage() repeatedly tiles the texture image across and down the image
%  canvas.
%
%  The format of the TextureImage method is:
%
%      MagickBooleanType TextureImage(Image *image,const Image *texture,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o texture_image: This image is the texture to layer on the background.
%
*/
static inline MagickBooleanType IsTextureComposited(const Image *image,
  const Image *texture_image)
{
  return(((image->compose != CopyCompositeOp) &&
    ((image->compose != OverCompositeOp) ||
    (image->alpha_trait != UndefinedPixelTrait) ||
    (texture_image->alpha_trait != UndefinedPixelTrait))) ? MagickTrue :
    MagickFalse);
}

typedef struct _TextureSpan
{
  Image
    *image,
    *texture_image;

  size_t
    width;
} TextureSpan;

static Quantum *CopyTextureSpan(const TextureSpan *span,const Quantum *p,
  Quantum *q)
{
  Image
    *image = span->image,
    *texture_image = span->texture_image;

  const size_t
    width = span->width;

  ssize_t
    j;

  for (j=0; j < (ssize_t) width; j++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(texture_image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(texture_image,i);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      PixelTrait texture_traits=GetPixelChannelTraits(texture_image,
        channel);
      if ((traits == UndefinedPixelTrait) ||
          (texture_traits == UndefinedPixelTrait))
        continue;
      SetPixelChannel(image,channel,p[i],q);
    }
    p+=(ptrdiff_t) GetPixelChannels(texture_image);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

#define TextureImageTag  "Texture/Image"

static MagickBooleanType ReportTextureProgress(Image *image,const ssize_t y)
{
  MagickBooleanType
    status;

  status=MagickTrue;
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(image,TextureImageTag,(MagickOffsetType) y,
        image->rows);
      if (proceed == MagickFalse)
        status=MagickFalse;
    }
  return(status);
}

typedef struct _TextureTiles
{
  Image
    *image,
    *texture_image;

  ssize_t
    y;
} TextureTiles;

static MagickBooleanType CompositeTextureRow(const TextureTiles *tiles,
  MagickBooleanType status,ExceptionInfo *exception)
{
  Image
    *const image = tiles->image,
    *const texture_image = tiles->texture_image;

  const ssize_t
    y = tiles->y;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x+=(ssize_t) texture_image->columns)
  {
    MagickBooleanType
      thread_status;

    thread_status=CompositeImage(image,texture_image,image->compose,
      MagickTrue,x+texture_image->tile_offset.x,y+
      texture_image->tile_offset.y,exception);
    if (thread_status == MagickFalse)
      {
        status=thread_status;
        break;
      }
  }
  return(status);
}

static MagickBooleanType TileTextureImage(Image *image,Image *texture_image,
  MagickBooleanType status,ExceptionInfo *exception)
{
  ssize_t
    y;

  /*
    Tile texture onto the image background.
  */
  for (y=0; y < (ssize_t) image->rows; y+=(ssize_t) texture_image->rows)
  {
    TextureTiles
      tiles;

    if (status == MagickFalse)
      continue;
    tiles.image=image;
    tiles.texture_image=texture_image;
    tiles.y=y;
    status=CompositeTextureRow(&tiles,status,exception);
    if (ReportTextureProgress(image,y) == MagickFalse)
      status=MagickFalse;
  }
  (void) SetImageProgress(image,TextureImageTag,(MagickOffsetType)
    image->rows,image->rows);
  texture_image=DestroyImage(texture_image);
  return(status);
}

static void CopyTextureRow(Image *image,Image *texture_image,
  const Quantum *pixels,Quantum *q)
{
  const Quantum
    *p;

  ssize_t
    x;

  size_t
    width;

  for (x=0; x < (ssize_t) image->columns; x+=(ssize_t) texture_image->columns)
  {
    TextureSpan
      span;

    p=pixels;
    width=texture_image->columns;
    if ((x+(ssize_t) width) > (ssize_t) image->columns)
      width=image->columns-(size_t) x;
    span.image=image;
    span.texture_image=texture_image;
    span.width=width;
    q=CopyTextureSpan(&span,p,q);
  }
}

typedef struct _TextureCopyRow
{
  Image
    *image,
    *texture_image;

  CacheView
    *texture_view,
    *image_view;

  ExceptionInfo
    *exception;
} TextureCopyRow;

static void CopyTextureImageRow(const TextureCopyRow *row,const ssize_t y,
  MagickBooleanType *status)
{
  Image
    *const image = row->image,
    *const texture_image = row->texture_image;

  CacheView
    *const texture_view = row->texture_view,
    *const image_view = row->image_view;

  ExceptionInfo
    *const exception = row->exception;

  const Quantum
    *pixels;

  Quantum
    *q;

  if ((*status) == MagickFalse)
    return;
  pixels=GetCacheViewVirtualPixels(texture_view,texture_image->tile_offset.x,
    (y+texture_image->tile_offset.y) % (ssize_t) texture_image->rows,
    texture_image->columns,1,exception);
  q=QueueCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if ((pixels == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return;
    }
  CopyTextureRow(image,texture_image,pixels,q);
  SyncCompositeRow(image_view,exception,status);
  if (ReportTextureProgress(image,y) == MagickFalse)
    (*status)=MagickFalse;
}

static MagickBooleanType CopyTextureImage(Image *image,Image *texture_image,
  ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *texture_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  /*
    Tile texture onto the image background (optimized).
  */
  status=MagickTrue;
  texture_view=AcquireVirtualCacheView(texture_image,exception);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(texture_image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const TextureCopyRow
      row = { image, texture_image, texture_view, image_view, exception };

    CopyTextureImageRow(&row,y,&status);
  }
  texture_view=DestroyCacheView(texture_view);
  image_view=DestroyCacheView(image_view);
  texture_image=DestroyImage(texture_image);
  return(status);
}

static void PrepareTextureImage(const Image *image,Image *texture_image,
  ExceptionInfo *exception)
{
  (void) TransformImageColorspace(texture_image,image->colorspace,exception);
  (void) SetImageVirtualPixelMethod(texture_image,TileVirtualPixelMethod,
    exception);
}

MagickExport MagickBooleanType TextureImage(Image *image,const Image *texture,
  ExceptionInfo *exception)
{
  Image
    *texture_image;

  MagickBooleanType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (texture == (const Image *) NULL)
    return(MagickFalse);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  texture_image=CloneImage(texture,0,0,MagickTrue,exception);
  if (texture_image == (const Image *) NULL)
    return(MagickFalse);
  PrepareTextureImage(image,texture_image,exception);
  status=MagickTrue;
  if (IsTextureComposited(image,texture_image) != MagickFalse)
    return(TileTextureImage(image,texture_image,status,exception));
  return(CopyTextureImage(image,texture_image,exception));
}
