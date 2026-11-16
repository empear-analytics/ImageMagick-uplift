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

  MagickCore private methods to assign the colors of a quantized image and
  to dither it.
*/
#ifndef MAGICKCORE_QUANTIZE_ASSIGN_PRIVATE_H
#define MAGICKCORE_QUANTIZE_ASSIGN_PRIVATE_H

#include "MagickCore/artifact.h"
#include "MagickCore/quantize-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
  Method prototypes.
*/
static MagickBooleanType
  AssignImageColors(Image *,QCubeInfo *,ExceptionInfo *),
  DitherImage(Image *,QCubeInfo *,ExceptionInfo *);

static void
  ClosestColor(const Image *,QCubeInfo *,const QNodeInfo *);

typedef struct _RiemersmaCurve
{
  Image
    *image;

  CacheView
    *image_view;

  QCubeInfo
    *cube_info;
} RiemersmaCurve;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A s s i g n I m a g e C o l o r s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AssignImageColors() generates the output image from the pruned tree.  The
%  output image consists of two parts: (1)  A color map, which is an array
%  of color descriptions (RGB triples) for each color present in the
%  output image;  (2)  A pixel array, which represents each pixel as an
%  index into the color map array.
%
%  First, the assignment phase makes one pass over the pruned color
%  description tree to establish the image's color map.  For each node
%  with n2  > 0, it divides Sr, Sg, and Sb by n2 .  This produces the mean
%  color of all pixels that classify no lower than this node.  Each of
%  these colors becomes an entry in the color map.
%
%  Finally,  the assignment phase reclassifies each pixel in the pruned
%  tree to identify the deepest node containing the pixel's color.  The
%  pixel's value in the pixel array becomes the index of this node's mean
%  color in the color map.
%
%  The format of the AssignImageColors() method is:
%
%      MagickBooleanType AssignImageColors(Image *image,QCubeInfo *cube_info)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o cube_info: A pointer to the Cube structure.
%
*/

static inline void AssociateAlphaPixelInfo(const QCubeInfo *cube_info,
  const PixelInfo *pixel,DoublePixelPacket *alpha_pixel)
{
  double
    alpha;

  if ((cube_info->associate_alpha == MagickFalse) ||
      (pixel->alpha == (double) OpaqueAlpha))
    {
      alpha_pixel->red=(double) pixel->red;
      alpha_pixel->green=(double) pixel->green;
      alpha_pixel->blue=(double) pixel->blue;
      alpha_pixel->alpha=(double) pixel->alpha;
      return;
    }
  alpha=(double) (QuantumScale*pixel->alpha);
  alpha_pixel->red=alpha*pixel->red;
  alpha_pixel->green=alpha*pixel->green;
  alpha_pixel->blue=alpha*pixel->blue;
  alpha_pixel->alpha=(double) pixel->alpha;
}

static inline void SetColormapPixel(Image *image,const QCubeInfo *cube,
  const size_t index,Quantum *magick_restrict q)
{
  if (image->storage_class == PseudoClass)
    SetPixelIndex(image,(Quantum) index,q);
  if (cube->quantize_info->measure_error == MagickFalse)
    {
      SetPixelRed(image,ClampToQuantum(image->colormap[index].red),q);
      SetPixelGreen(image,ClampToQuantum(image->colormap[index].green),q);
      SetPixelBlue(image,ClampToQuantum(image->colormap[index].blue),q);
      if (cube->associate_alpha != MagickFalse)
        SetPixelAlpha(image,ClampToQuantum(image->colormap[index].alpha),q);
    }
}

static inline void SearchClosestCubeColor(const Image *image,QCubeInfo *cube,
  const DoublePixelPacket pixel)
{
  QNodeInfo
    *node_info;

  size_t
    index,
    node_id;

  /*
    Identify the deepest node containing the pixel's color.
  */
  node_info=cube->root;
  for (index=MaxTreeDepth-1; (ssize_t) index > 0; index--)
  {
    node_id=ColorToQNodeId(cube,&pixel,index);
    if (node_info->child[node_id] == (QNodeInfo *) NULL)
      break;
    node_info=node_info->child[node_id];
  }
  /*
    Find closest color among siblings and their children.
  */
  cube->target=pixel;
  cube->distance=(double) (4.0*((double) QuantumRange+1.0)*((double)
    QuantumRange+1.0)+1.0);
  ClosestColor(image,cube,node_info->parent);
}

static inline void SetMonochromeColormap(Image *image)
{
  double
    intensity;

  /*
    Monochrome image.
  */
  intensity=GetPixelInfoLuma(image->colormap+0) < (double)
    QuantumRange/2.0 ? 0.0 : (double) QuantumRange;
  if (image->colors > 1)
    {
      intensity=0.0;
      if (GetPixelInfoLuma(image->colormap+0) >
          GetPixelInfoLuma(image->colormap+1))
        intensity=(double) QuantumRange;
    }
  image->colormap[0].red=intensity;
  image->colormap[0].green=intensity;
  image->colormap[0].blue=intensity;
  if (image->colors > 1)
    {
      image->colormap[1].red=(double) QuantumRange-intensity;
      image->colormap[1].green=(double) QuantumRange-intensity;
      image->colormap[1].blue=(double) QuantumRange-intensity;
    }
}

static inline void AssignImageRowColors(Image *image,const QCubeInfo *cube_info,
  Quantum *magick_restrict q)
{
  QCubeInfo
    cube;

  ssize_t
    count,
    x;

  cube=(*cube_info);
  for (x=0; x < (ssize_t) image->columns; x+=count)
  {
    DoublePixelPacket
      pixel;

    ssize_t
      i;

    size_t
      index;

    /*
      Identify the deepest node containing the pixel's color.
    */
    for (count=1; (x+count) < (ssize_t) image->columns; count++)
    {
      PixelInfo
        packet;

      GetPixelInfoPixel(image,q+count*(ssize_t) GetPixelChannels(image),
        &packet);
      if (IsPixelEquivalent(image,q,&packet) == MagickFalse)
        break;
    }
    AssociateAlphaPixel(image,&cube,q,&pixel);
    SearchClosestCubeColor(image,&cube,pixel);
    index=cube.color_number;
    for (i=0; i < (ssize_t) count; i++)
    {
      SetColormapPixel(image,&cube,index,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
}

#define AssignImageTag  "Assign/Image"

static inline MagickBooleanType ReportImageRowProgress(Image *image,
  const char *tag,const ssize_t y)
{
  MagickBooleanType
    status;

  status=MagickTrue;
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(image,tag,(MagickOffsetType) y,
        image->rows);
      if (proceed == MagickFalse)
        status=MagickFalse;
    }
  return(status);
}

static inline MagickBooleanType IsMonochromeQuantize(
  const QCubeInfo *cube_info)
{
  return(((cube_info->quantize_info->number_colors == 2) &&
      (IsGrayColorspace(cube_info->quantize_info->colorspace))) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsColorspaceRestoreNeeded(
  const QCubeInfo *cube_info,const ColorspaceType colorspace)
{
  return(((cube_info->quantize_info->colorspace != UndefinedColorspace) &&
      (IssRGBCompatibleColorspace(colorspace) == MagickFalse)) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType FinishAssignImageColors(Image *image,
  QCubeInfo *cube_info,const ColorspaceType colorspace,
  ExceptionInfo *exception)
{
  if (cube_info->quantize_info->measure_error != MagickFalse)
    (void) GetImageQuantizeError(image,exception);
  if (IsMonochromeQuantize(cube_info) != MagickFalse)
    SetMonochromeColormap(image);
  (void) SyncImage(image,exception);
  if (IsColorspaceRestoreNeeded(cube_info,colorspace) != MagickFalse)
    (void) TransformImageColorspace(image,colorspace,exception);
  return(MagickTrue);
}

static inline MagickBooleanType AcquireAssignImageColormap(Image *image,
  QCubeInfo *cube_info,ExceptionInfo *exception)
{
  if (cube_info->quantize_info->colorspace != UndefinedColorspace)
    (void) TransformImageColorspace(image,cube_info->quantize_info->colorspace,
      exception);
  cube_info->transparent_pixels=0;
  cube_info->transparent_index=(-1);
  if (SetImageColormap(image,cube_info,exception) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static inline MagickBooleanType AssignImageColors(Image *image,QCubeInfo *cube_info,
  ExceptionInfo *exception)
{
  ColorspaceType
    colorspace;

  ssize_t
    y;

  /*
    Allocate image colormap.
  */
  colorspace=image->colorspace;
  if (AcquireAssignImageColormap(image,cube_info,exception) == MagickFalse)
    return(MagickFalse);
  /*
    Create a reduced color image.
  */
  if (cube_info->quantize_info->dither_method != NoDitherMethod)
    (void) DitherImage(image,cube_info,exception);
  else
    {
      CacheView
        *image_view;

      MagickBooleanType
        status;

      status=MagickTrue;
      image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(image,image,image->rows,1)
#endif
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        Quantum
          *magick_restrict q;

        const QuantizeRowInfo
          row = { image,image_view,exception };

        if (FetchQuantizeRow(&row,y,&q,&status) == MagickFalse)
          continue;
        AssignImageRowColors(image,cube_info,q);
        if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
          status=MagickFalse;
        if (ReportImageRowProgress(image,AssignImageTag,y) == MagickFalse)
          status=MagickFalse;
      }
      image_view=DestroyCacheView(image_view);
    }
  return(FinishAssignImageColors(image,cube_info,colorspace,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C l o s e s t C o l o r                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClosestColor() traverses the color cube tree at a particular node and
%  determines which colormap entry best represents the input color.
%
%  The format of the ClosestColor method is:
%
%      void ClosestColor(const Image *image,QCubeInfo *cube_info,
%        const QNodeInfo *node_info)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o cube_info: A pointer to the Cube structure.
%
%    o node_info: the address of a structure of type QNodeInfo which points to a
%      node in the color cube tree that is to be pruned.
%
*/
static inline void UpdateClosestColor(const Image *image,QCubeInfo *cube_info,
  const QNodeInfo *node_info)
{
  double
    alpha,
    beta,
    distance,
    pixel;

  DoublePixelPacket
    *magick_restrict q;

  PixelInfo
    *magick_restrict p;

  /*
    Determine if this color is "closest".
  */
  p=image->colormap+node_info->color_number;
  q=(&cube_info->target);
  alpha=1.0;
  beta=1.0;
  if (cube_info->associate_alpha != MagickFalse)
    {
      alpha=(MagickRealType) (QuantumScale*p->alpha);
      beta=(MagickRealType) (QuantumScale*q->alpha);
    }
  pixel=alpha*p->red-beta*q->red;
  distance=pixel*pixel;
  if (!(distance <= cube_info->distance))
    return;
  pixel=alpha*p->green-beta*q->green;
  distance+=pixel*pixel;
  if (!(distance <= cube_info->distance))
    return;
  pixel=alpha*p->blue-beta*q->blue;
  distance+=pixel*pixel;
  if (!(distance <= cube_info->distance))
    return;
  if (cube_info->associate_alpha != MagickFalse)
    {
      pixel=p->alpha-q->alpha;
      distance+=pixel*pixel;
    }
  if (distance <= cube_info->distance)
    {
      cube_info->distance=distance;
      cube_info->color_number=node_info->color_number;
    }
}

static inline void ClosestColor(const Image *image,QCubeInfo *cube_info,
  const QNodeInfo *node_info)
{
  size_t
    number_children;

  ssize_t
    i;

  /*
    Traverse any children.
  */
  number_children=cube_info->associate_alpha == MagickFalse ? 8UL : 16UL;
  for (i=0; i < (ssize_t) number_children; i++)
    if (node_info->child[i] != (QNodeInfo *) NULL)
      ClosestColor(image,cube_info,node_info->child[i]);
  if (node_info->number_unique == 0)
    return;
  UpdateClosestColor(image,cube_info,node_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D i t h e r I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DitherImage() distributes the difference between an original image and
%  the corresponding color reduced algorithm to neighboring pixels using
%  serpentine-scan Floyd-Steinberg error diffusion. DitherImage returns
%  MagickTrue if the image is dithered otherwise MagickFalse.
%
%  The format of the DitherImage method is:
%
%      MagickBooleanType DitherImage(Image *image,QCubeInfo *cube_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o cube_info: A pointer to the Cube structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline DoublePixelPacket **DestroyPixelTLS(DoublePixelPacket **pixels)
{
  ssize_t
    i;

  assert(pixels != (DoublePixelPacket **) NULL);
  for (i=0; i < (ssize_t) GetMagickResourceLimit(ThreadResource); i++)
    if (pixels[i] != (DoublePixelPacket *) NULL)
      pixels[i]=(DoublePixelPacket *) RelinquishMagickMemory(pixels[i]);
  pixels=(DoublePixelPacket **) RelinquishMagickMemory(pixels);
  return(pixels);
}

static inline DoublePixelPacket **AcquirePixelTLS(const size_t count)
{
  DoublePixelPacket
    **pixels;

  size_t
    number_threads;

  ssize_t
    i;

  number_threads=(size_t) GetMagickResourceLimit(ThreadResource);
  pixels=(DoublePixelPacket **) AcquireQuantumMemory(number_threads,
    sizeof(*pixels));
  if (pixels == (DoublePixelPacket **) NULL)
    return((DoublePixelPacket **) NULL);
  (void) memset(pixels,0,number_threads*sizeof(*pixels));
  for (i=0; i < (ssize_t) number_threads; i++)
  {
    pixels[i]=(DoublePixelPacket *) AcquireQuantumMemory(count,2*
      sizeof(**pixels));
    if (pixels[i] == (DoublePixelPacket *) NULL)
      return(DestroyPixelTLS(pixels));
  }
  return(pixels);
}

static inline ssize_t CacheOffset(QCubeInfo *cube_info,
  const DoublePixelPacket *pixel)
{
#define RedShift(pixel) (((pixel) >> CacheShift) << (0*(8-CacheShift)))
#define GreenShift(pixel) (((pixel) >> CacheShift) << (1*(8-CacheShift)))
#define BlueShift(pixel) (((pixel) >> CacheShift) << (2*(8-CacheShift)))
#define AlphaShift(pixel) (((pixel) >> CacheShift) << (3*(8-CacheShift)))

  ssize_t
    offset;

  offset=(ssize_t) (RedShift(ScaleQuantumToChar(ClampPixel(pixel->red))) |
    GreenShift(ScaleQuantumToChar(ClampPixel(pixel->green))) |
    BlueShift(ScaleQuantumToChar(ClampPixel(pixel->blue))));
  if (cube_info->associate_alpha != MagickFalse)
    offset|=AlphaShift(ScaleQuantumToChar(ClampPixel(pixel->alpha)));
  return(offset);
}

static inline ssize_t *GetCacheEntry(QCubeInfo *cube_info,
  const DoublePixelPacket *pixel)
{
  ssize_t
    **block,
    offset;

  /*
    Cache blocks are allocated on first use, -1 marks an entry not set yet.
  */
  offset=CacheOffset(cube_info,pixel);
  block=cube_info->cache+(offset >> CacheBlockShift);
  if (*block == (ssize_t *) NULL)
    {
      *block=(ssize_t *) AcquireQuantumMemory((size_t) 1UL << CacheBlockShift,
        sizeof(**block));
      if (*block == (ssize_t *) NULL)
        return((ssize_t *) NULL);
      (void) memset(*block,(-1),sizeof(**block) << CacheBlockShift);
    }
  return(*block+(offset & ((1L << CacheBlockShift)-1)));
}

static inline void FindClosestCubeColor(const Image *image,QCubeInfo *cube,
  const DoublePixelPacket pixel,ssize_t *entry)
{
  SearchClosestCubeColor(image,cube,pixel);
  if (entry != (ssize_t *) NULL)
    *entry=(ssize_t) cube->color_number;
}

static inline size_t AssignClosestColormapEntry(Image *image,const QCubeInfo *cube,
  const ssize_t *entry,Quantum *magick_restrict q)
{
  size_t
    index;

  index=(entry != (ssize_t *) NULL) ? (size_t) *entry : cube->color_number;
  SetColormapPixel(image,cube,index,q);
  return(index);
}

static inline DoublePixelPacket ClampDitherPixel(const QCubeInfo *cube,
  DoublePixelPacket pixel)
{
  pixel.red=(double) ClampPixel(pixel.red);
  pixel.green=(double) ClampPixel(pixel.green);
  pixel.blue=(double) ClampPixel(pixel.blue);
  if (cube->associate_alpha != MagickFalse)
    pixel.alpha=(double) ClampPixel(pixel.alpha);
  return(pixel);
}

typedef struct _FloydSteinbergBehind
{
  QCubeInfo
    *cube_info;

  DoublePixelPacket
    *previous;

  ssize_t
    x,
    u,
    v;
} FloydSteinbergBehind;

static inline DoublePixelPacket AddPreviousRowBehindError(
  const FloydSteinbergBehind *behind,const QCubeInfo *cube,
  DoublePixelPacket pixel)
{
  QCubeInfo
    *const cube_info = behind->cube_info;

  DoublePixelPacket
    *const previous = behind->previous;

  const ssize_t
    x = behind->x,
    u = behind->u,
    v = behind->v;

  if (x > 0)
    {
      pixel.red+=3.0*cube_info->diffusion*previous[u-v].red/16;
      pixel.green+=3.0*cube_info->diffusion*previous[u-v].green/16;
      pixel.blue+=3.0*cube_info->diffusion*previous[u-v].blue/16;
      if (cube->associate_alpha != MagickFalse)
        pixel.alpha+=3.0*cube_info->diffusion*previous[u-v].alpha/16;
    }
  return(pixel);
}

typedef struct _FloydSteinbergPreviousRow
{
  Image
    *image;

  QCubeInfo
    *cube_info;

  DoublePixelPacket
    *previous;

  ssize_t
    x,
    u,
    v;
} FloydSteinbergPreviousRow;

static inline DoublePixelPacket AddPreviousRowError(
  const FloydSteinbergPreviousRow *row,const QCubeInfo *cube,
  DoublePixelPacket pixel)
{
  Image
    *const image = row->image;

  QCubeInfo
    *const cube_info = row->cube_info;

  DoublePixelPacket
    *const previous = row->previous;

  const ssize_t
    x = row->x,
    u = row->u,
    v = row->v;

  FloydSteinbergBehind
    behind;

  if (x < (ssize_t) (image->columns-1))
    {
      pixel.red+=cube_info->diffusion*previous[u+v].red/16;
      pixel.green+=cube_info->diffusion*previous[u+v].green/16;
      pixel.blue+=cube_info->diffusion*previous[u+v].blue/16;
      if (cube->associate_alpha != MagickFalse)
        pixel.alpha+=cube_info->diffusion*previous[u+v].alpha/16;
    }
  pixel.red+=5.0*cube_info->diffusion*previous[u].red/16;
  pixel.green+=5.0*cube_info->diffusion*previous[u].green/16;
  pixel.blue+=5.0*cube_info->diffusion*previous[u].blue/16;
  if (cube->associate_alpha != MagickFalse)
    pixel.alpha+=5.0*cube_info->diffusion*previous[u].alpha/16;
  behind.cube_info=cube_info;
  behind.previous=previous;
  behind.x=x;
  behind.u=u;
  behind.v=v;
  pixel=AddPreviousRowBehindError(&behind,cube,pixel);
  return(pixel);
}

typedef struct _FloydSteinbergCurrentRow
{
  QCubeInfo
    *cube_info;

  DoublePixelPacket
    *current;

  ssize_t
    u,
    v;
} FloydSteinbergCurrentRow;

static inline DoublePixelPacket AddCurrentRowError(
  const FloydSteinbergCurrentRow *ahead,const QCubeInfo *cube,
  DoublePixelPacket pixel)
{
  pixel.red+=7.0*ahead->cube_info->diffusion*ahead->current[ahead->u-
    ahead->v].red/16;
  pixel.green+=7.0*ahead->cube_info->diffusion*ahead->current[ahead->u-
    ahead->v].green/16;
  pixel.blue+=7.0*ahead->cube_info->diffusion*ahead->current[ahead->u-
    ahead->v].blue/16;
  if (cube->associate_alpha != MagickFalse)
    pixel.alpha+=7.0*ahead->cube_info->diffusion*ahead->current[ahead->u-
      ahead->v].alpha/16;
  return(pixel);
}

typedef struct _FloydSteinbergPixel
{
  Image
    *image;

  QCubeInfo
    *cube_info;

  CacheView
    *image_view;

  DoublePixelPacket
    *current,
    *previous;

  Quantum
    *q;

  ssize_t
    v,
    x,
    y;

  ExceptionInfo
    *exception;
} FloydSteinbergPixel;

static inline void DitherFloydSteinbergPixel(const FloydSteinbergPixel *fs,
  QCubeInfo *cube,MagickBooleanType *status)
{
  DoublePixelPacket
    color,
    pixel;

  FloydSteinbergCurrentRow
    ahead;

  FloydSteinbergPreviousRow
    row;

  size_t
    index;

  ssize_t
    *entry;

  ssize_t
    u;

  u=(fs->y & 0x01) != 0 ? (ssize_t) fs->image->columns-1-fs->x : fs->x;
  AssociateAlphaPixel(fs->image,cube,fs->q+u*(ssize_t)
    GetPixelChannels(fs->image),&pixel);
  if (fs->x > 0)
    {
      ahead.cube_info=fs->cube_info;
      ahead.current=fs->current;
      ahead.u=u;
      ahead.v=fs->v;
      pixel=AddCurrentRowError(&ahead,cube,pixel);
    }
  if (fs->y > 0)
    {
      row.image=fs->image;
      row.cube_info=fs->cube_info;
      row.previous=fs->previous;
      row.x=fs->x;
      row.u=u;
      row.v=fs->v;
      pixel=AddPreviousRowError(&row,cube,pixel);
    }
  pixel=ClampDitherPixel(cube,pixel);
  entry=GetCacheEntry(cube,&pixel);
  if ((entry == (ssize_t *) NULL) || (*entry < 0))
    FindClosestCubeColor(fs->image,cube,pixel,entry);
  /*
    Assign pixel to closest colormap entry.
  */
  index=AssignClosestColormapEntry(fs->image,cube,entry,fs->q+u*(ssize_t)
    GetPixelChannels(fs->image));
  if (SyncCacheViewAuthenticPixels(fs->image_view,fs->exception) == MagickFalse)
    *status=MagickFalse;
  /*
    Store the error.
  */
  AssociateAlphaPixelInfo(cube,fs->image->colormap+index,&color);
  fs->current[u].red=pixel.red-color.red;
  fs->current[u].green=pixel.green-color.green;
  fs->current[u].blue=pixel.blue-color.blue;
  if ((*cube).associate_alpha != MagickFalse)
    fs->current[u].alpha=pixel.alpha-color.alpha;
}

#define DitherImageTag  "Dither/Image"

typedef struct _FloydSteinbergRow
{
  Image
    *image;

  QCubeInfo
    *cube_info;

  CacheView
    *image_view;

  DoublePixelPacket
    *current,
    *previous;

  Quantum
    *q;

  ssize_t
    v,
    y;

  ExceptionInfo
    *exception;
} FloydSteinbergRow;

static inline void DitherFloydSteinbergRow(const FloydSteinbergRow *row,
  QCubeInfo *cube,MagickBooleanType *status)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) row->image->columns; x++)
  {
    FloydSteinbergPixel
      fs;

    fs.image=row->image;
    fs.cube_info=row->cube_info;
    fs.image_view=row->image_view;
    fs.current=row->current;
    fs.previous=row->previous;
    fs.q=row->q;
    fs.v=row->v;
    fs.x=x;
    fs.y=row->y;
    fs.exception=row->exception;
    DitherFloydSteinbergPixel(&fs,cube,status);
    if (ReportImageRowProgress(row->image,DitherImageTag,row->y) == MagickFalse)
      *status=MagickFalse;
  }
}

static inline MagickBooleanType FloydSteinbergDither(Image *image,QCubeInfo *cube_info,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  DoublePixelPacket
    **pixels;

  MagickBooleanType
    status;

  ssize_t
    y;

  /*
    Distribute quantization error using Floyd-Steinberg.
  */
  pixels=AcquirePixelTLS(image->columns);
  if (pixels == (DoublePixelPacket **) NULL)
    return(MagickFalse);
  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const int
      id = GetOpenMPThreadId();

    DoublePixelPacket
      *current,
      *previous;

    FloydSteinbergRow
      row;

    QCubeInfo
      cube;

    Quantum
      *magick_restrict q;

    ssize_t
      v;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    cube=(*cube_info);
    current=pixels[id]+(y & 0x01)*image->columns;
    previous=pixels[id]+((y+1) & 0x01)*image->columns;
    v=(ssize_t) ((y & 0x01) != 0 ? -1 : 1);
    row.image=image;
    row.cube_info=cube_info;
    row.image_view=image_view;
    row.current=current;
    row.previous=previous;
    row.q=q;
    row.v=v;
    row.y=y;
    row.exception=exception;
    DitherFloydSteinbergRow(&row,&cube,&status);
  }
  image_view=DestroyCacheView(image_view);
  pixels=DestroyPixelTLS(pixels);
  return(MagickTrue);
}

static inline MagickBooleanType IsDitherPositionInImage(const Image *image,
  const QCubeInfo *p)
{
  return(((p->x >= 0) && (p->x < (ssize_t) image->columns) &&
    (p->y >= 0) && (p->y < (ssize_t) image->rows)) ? MagickTrue : MagickFalse);
}

static inline DoublePixelPacket AddRiemersmaQueueError(const QCubeInfo *cube_info,
  const QCubeInfo *p,DoublePixelPacket pixel)
{
  ssize_t
    i;

  for (i=0; i < ErrorQueueLength; i++)
  {
    pixel.red+=ErrorRelativeWeight*cube_info->diffusion*p->weights[i]*
      p->error[i].red;
    pixel.green+=ErrorRelativeWeight*cube_info->diffusion*p->weights[i]*
      p->error[i].green;
    pixel.blue+=ErrorRelativeWeight*cube_info->diffusion*p->weights[i]*
      p->error[i].blue;
    if (cube_info->associate_alpha != MagickFalse)
      pixel.alpha+=ErrorRelativeWeight*cube_info->diffusion*p->weights[i]*
        p->error[i].alpha;
  }
  return(pixel);
}

static inline void MoveRiemersmaPosition(QCubeInfo *p,const unsigned int direction)
{
  switch (direction)
  {
    case WestGravity: p->x--; break;
    case EastGravity: p->x++; break;
    case NorthGravity: p->y--; break;
    case SouthGravity: p->y++; break;
  }
}

static inline MagickBooleanType RiemersmaDither(const RiemersmaCurve *curve,
  const unsigned int direction,ExceptionInfo *exception)
{
#define DitherImageTag  "Dither/Image"

  Image
    *image = curve->image;

  CacheView
    *image_view = curve->image_view;

  QCubeInfo
    *cube_info = curve->cube_info;

  QCubeInfo
    *p;

  DoublePixelPacket
    color,
    pixel;

  MagickBooleanType
    proceed;

  size_t
    index;

  p=cube_info;
  if (IsDitherPositionInImage(image,p) != MagickFalse)
    {
      Quantum
        *magick_restrict q;

      ssize_t
        *entry;

      /*
        Distribute error.
      */
      q=GetCacheViewAuthenticPixels(image_view,p->x,p->y,1,1,exception);
      if (q == (Quantum *) NULL)
        return(MagickFalse);
      AssociateAlphaPixel(image,cube_info,q,&pixel);
      pixel=AddRiemersmaQueueError(cube_info,p,pixel);
      pixel=ClampDitherPixel(cube_info,pixel);
      entry=GetCacheEntry(p,&pixel);
      if ((entry == (ssize_t *) NULL) || (*entry < 0))
        FindClosestCubeColor(image,p,pixel,entry);
      /*
        Assign pixel to closest colormap entry.
      */
      index=AssignClosestColormapEntry(image,p,entry,q);
      if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
        return(MagickFalse);
      /*
        Propagate the error as the last entry of the error queue.
      */
      (void) memmove(p->error,p->error+1,(ErrorQueueLength-1)*
        sizeof(p->error[0]));
      AssociateAlphaPixelInfo(cube_info,image->colormap+index,&color);
      p->error[ErrorQueueLength-1].red=pixel.red-color.red;
      p->error[ErrorQueueLength-1].green=pixel.green-color.green;
      p->error[ErrorQueueLength-1].blue=pixel.blue-color.blue;
      if (cube_info->associate_alpha != MagickFalse)
        p->error[ErrorQueueLength-1].alpha=pixel.alpha-color.alpha;
      proceed=SetImageProgress(image,DitherImageTag,p->offset,p->span);
      if (proceed == MagickFalse)
        return(MagickFalse);
      p->offset++;
    }
  MoveRiemersmaPosition(p,direction);
  return(MagickTrue);
}

static inline void RiemersmaNorthUnitCurve(const RiemersmaCurve *curve,
  MagickBooleanType *status,ExceptionInfo *exception)
{
  *status=RiemersmaDither(curve,SouthGravity,exception);
  if (*status != MagickFalse)
    *status=RiemersmaDither(curve,EastGravity,exception);
  if (*status != MagickFalse)
    *status=RiemersmaDither(curve,NorthGravity,exception);
}

static inline void RiemersmaSouthUnitCurve(const RiemersmaCurve *curve,
  MagickBooleanType *status,ExceptionInfo *exception)
{
  *status=RiemersmaDither(curve,NorthGravity,exception);
  if (*status != MagickFalse)
    *status=RiemersmaDither(curve,WestGravity,exception);
  if (*status != MagickFalse)
    *status=RiemersmaDither(curve,SouthGravity,exception);
}

static inline void RiemersmaVerticalUnitCurve(const RiemersmaCurve *curve,
  const unsigned int direction,MagickBooleanType *status,
  ExceptionInfo *exception)
{
  switch (direction)
  {
    case NorthGravity:
    {
      RiemersmaNorthUnitCurve(curve,status,exception);
      break;
    }
    case SouthGravity:
    {
      RiemersmaSouthUnitCurve(curve,status,exception);
      break;
    }
    default:
      break;
  }
}

typedef struct _RiemersmaMoves
{
  unsigned int
    first,
    second,
    third;
} RiemersmaMoves;

static inline MagickBooleanType RiemersmaUnitCurve(const RiemersmaCurve *curve,
  const RiemersmaMoves *moves,ExceptionInfo *exception)
{
  const unsigned int
    first = moves->first,
    second = moves->second,
    third = moves->third;

  MagickBooleanType
    status;

  status=RiemersmaDither(curve,first,exception);
  if (status != MagickFalse)
    status=RiemersmaDither(curve,second,exception);
  if (status != MagickFalse)
    status=RiemersmaDither(curve,third,exception);
  return(status);
}

typedef struct _RiemersmaTurns
{
  unsigned int
    enter,
    step,
    along,
    leave;
} RiemersmaTurns;

static MagickBooleanType
  Riemersma(const RiemersmaCurve *,const size_t,const unsigned int,
    ExceptionInfo *);

static inline MagickBooleanType RiemersmaSubCurves(const RiemersmaCurve *curve,
  const size_t level,const RiemersmaTurns *turns,ExceptionInfo *exception)
{
  const unsigned int
    enter = turns->enter,
    step = turns->step,
    along = turns->along,
    leave = turns->leave;

  MagickBooleanType
    status;

  status=Riemersma(curve,level-1,enter,exception);
  if (status != MagickFalse)
    status=RiemersmaDither(curve,step,exception);
  if (status != MagickFalse)
    status=Riemersma(curve,level-1,along,exception);
  if (status != MagickFalse)
    status=RiemersmaDither(curve,leave,exception);
  if (status != MagickFalse)
    status=Riemersma(curve,level-1,along,exception);
  if (status != MagickFalse)
    status=RiemersmaDither(curve,along,exception);
  if (status != MagickFalse)
    status=Riemersma(curve,level-1,leave,exception);
  return(status);
}

static inline void RiemersmaUnitCurves(const RiemersmaCurve *curve,
  const unsigned int direction,MagickBooleanType *status,
  ExceptionInfo *exception)
{
  switch (direction)
  {
    case WestGravity:
    {
      const RiemersmaMoves
        moves = { EastGravity,SouthGravity,WestGravity };

      *status=RiemersmaUnitCurve(curve,&moves,exception);
      break;
    }
    case EastGravity:
    {
      const RiemersmaMoves
        moves = { WestGravity,NorthGravity,EastGravity };

      *status=RiemersmaUnitCurve(curve,&moves,exception);
      break;
    }
    default:
    {
      RiemersmaVerticalUnitCurve(curve,direction,status,exception);
      break;
    }
  }
}

static inline MagickBooleanType Riemersma(const RiemersmaCurve *curve,
  const size_t level,const unsigned int direction,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=MagickTrue;
  if (level == 1)
    RiemersmaUnitCurves(curve,direction,&status,exception);
  else
    switch (direction)
    {
      case WestGravity:
      {
        const RiemersmaTurns
          turns = { NorthGravity,EastGravity,WestGravity,SouthGravity };

        status=RiemersmaSubCurves(curve,level,&turns,exception);
        break;
      }
      case EastGravity:
      {
        const RiemersmaTurns
          turns = { SouthGravity,WestGravity,EastGravity,NorthGravity };

        status=RiemersmaSubCurves(curve,level,&turns,exception);
        break;
      }
      case NorthGravity:
      {
        const RiemersmaTurns
          turns = { WestGravity,SouthGravity,NorthGravity,EastGravity };

        status=RiemersmaSubCurves(curve,level,&turns,exception);
        break;
      }
      case SouthGravity:
      {
        const RiemersmaTurns
          turns = { EastGravity,NorthGravity,SouthGravity,WestGravity };

        status=RiemersmaSubCurves(curve,level,&turns,exception);
        break;
      }
      default:
        break;
    }
  return(status);
}

static inline void SetDitherDiffusion(const Image *image,QCubeInfo *cube_info)
{
  const char
    *artifact;

  artifact=GetImageArtifact(image,"dither:diffusion-amount");
  if (artifact != (const char *) NULL)
    cube_info->diffusion=StringToDoubleInterval(artifact,1.0);
}

static inline size_t GetRiemersmaLevel(const Image *image)
{
  size_t
    extent,
    level;

  extent=MagickMax(image->columns,image->rows);
  level=(size_t) log2((double) extent);
  if (((size_t) 1UL << level) < extent)
    level++;
  return(level);
}

static inline void ResetDitherError(QCubeInfo *cube_info)
{
  (void) memset(cube_info->error,0,ErrorQueueLength*sizeof(*cube_info->error));
  cube_info->x=0;
  cube_info->y=0;
}

static inline MagickBooleanType DitherImage(Image *image,QCubeInfo *cube_info,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  size_t
    level;

  RiemersmaCurve curve;

  SetDitherDiffusion(image,cube_info);
  if (cube_info->quantize_info->dither_method != RiemersmaDitherMethod)
    return(FloydSteinbergDither(image,cube_info,exception));
  /*
    Distribute quantization error along a Hilbert curve.
  */
  ResetDitherError(cube_info);
  level=GetRiemersmaLevel(image);
  cube_info->offset=0;
  cube_info->span=(MagickSizeType) image->columns*image->rows;
  image_view=AcquireAuthenticCacheView(image,exception);
  status=MagickTrue;
  curve.image=image;
  curve.image_view=image_view;
  curve.cube_info=cube_info;

  if (level > 0)
    status=Riemersma(&curve,level,NorthGravity,exception);
  if (status != MagickFalse)
    status=RiemersmaDither(&curve,ForgetGravity,exception);
  image_view=DestroyCacheView(image_view);
  return(status);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
