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

  MagickCore private methods to reduce the number of unique colors in an image.
*/
#ifndef MAGICKCORE_QUANTIZE_PRIVATE_H
#define MAGICKCORE_QUANTIZE_PRIVATE_H

#include "MagickCore/cache-view.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
  Define declarations.
*/
#if !defined(__APPLE__) && !defined(TARGET_OS_IPHONE)
#define CacheShift  2
#else
#define CacheShift  3
#endif
#define CacheBlockShift  9
#define CacheBlocks  ((size_t) 1UL << (4*(8-CacheShift)-CacheBlockShift))
#define ErrorQueueLength  16
#define ErrorRelativeWeight  MagickSafeReciprocal(16)
#define MaxQNodes  266817
#define MaxTreeDepth  8
#define QNodesInAList  1920

/*
  Typedef declarations.
*/
typedef struct _DoublePixelPacket
{
  double
    red,
    green,
    blue,
    alpha;
} DoublePixelPacket;

typedef struct _QNodeInfo
{
  struct _QNodeInfo
    *parent,
    *child[16];

  MagickSizeType
    number_unique;

  DoublePixelPacket
    total_color;

  double
    quantize_error;

  size_t
    color_number,
    id,
    level;
} QNodeInfo;

typedef struct _QNodes
{
  QNodeInfo
    *nodes;

  struct _QNodes
    *next;
} QNodes;

typedef struct _QCubeInfo
{
  QNodeInfo
    *root;

  size_t
    colors,
    maximum_colors;

  ssize_t
    transparent_index;

  MagickSizeType
    transparent_pixels;

  DoublePixelPacket
    target;

  double
    distance,
    pruning_threshold,
    next_threshold;

  size_t
    nodes,
    free_nodes,
    color_number;

  QNodeInfo
    *next_node;

  QNodes
    *node_queue;

  ssize_t
    **cache;

  DoublePixelPacket
    error[ErrorQueueLength];

  double
    diffusion,
    weights[ErrorQueueLength];

  QuantizeInfo
    *quantize_info;

  MagickBooleanType
    associate_alpha;

  ssize_t
    x,
    y;

  size_t
    depth;

  MagickOffsetType
    offset;

  MagickSizeType
    span;
} QCubeInfo;

/*
  Method prototypes.
*/
static QCubeInfo
  *GetQCubeInfo(const QuantizeInfo *,const size_t,const size_t);

static QNodeInfo
  *GetQNodeInfo(QCubeInfo *,const size_t,const size_t,QNodeInfo *);

static MagickBooleanType
  ClassifyImageColors(QCubeInfo *,const Image *,ExceptionInfo *),
  SetImageColormap(Image *,QCubeInfo *,ExceptionInfo *);

static void
  DefineImageColormap(Image *,QCubeInfo *,QNodeInfo *),
  DestroyQCubeInfo(QCubeInfo *),
  PruneLevel(QCubeInfo *,const QNodeInfo *),
  PruneToCubeDepth(QCubeInfo *,const QNodeInfo *),
  ReduceImageColors(const Image *,QCubeInfo *);

static inline void AssociateAlphaPixel(const Image *image,
  const QCubeInfo *cube_info,const Quantum *pixel,
  DoublePixelPacket *alpha_pixel)
{
  double
    alpha;

  if ((cube_info->associate_alpha == MagickFalse) ||
      (GetPixelAlpha(image,pixel) == OpaqueAlpha))
    {
      alpha_pixel->red=(double) GetPixelRed(image,pixel);
      alpha_pixel->green=(double) GetPixelGreen(image,pixel);
      alpha_pixel->blue=(double) GetPixelBlue(image,pixel);
      alpha_pixel->alpha=(double) GetPixelAlpha(image,pixel);
      return;
    }
  alpha=QuantumScale*(double) GetPixelAlpha(image,pixel);
  alpha_pixel->red=alpha*(double) GetPixelRed(image,pixel);
  alpha_pixel->green=alpha*(double) GetPixelGreen(image,pixel);
  alpha_pixel->blue=alpha*(double) GetPixelBlue(image,pixel);
  alpha_pixel->alpha=(double) GetPixelAlpha(image,pixel);
}

static inline size_t ColorToQNodeId(const QCubeInfo *cube_info,
  const DoublePixelPacket *pixel,size_t index)
{
  size_t
    id;

  id=(size_t) (((ScaleQuantumToChar(ClampPixel(pixel->red)) >> index) & 0x01) |
    ((ScaleQuantumToChar(ClampPixel(pixel->green)) >> index) & 0x01) << 1 |
    ((ScaleQuantumToChar(ClampPixel(pixel->blue)) >> index) & 0x01) << 2);
  if (cube_info->associate_alpha != MagickFalse)
    id|=((((size_t) ScaleQuantumToChar(ClampPixel(pixel->alpha)) >> index) &
      0x1) << 3);
  return(id);
}

typedef struct _QuantizeRowInfo
{
  Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;
} QuantizeRowInfo;

static inline MagickBooleanType FetchQuantizeRow(const QuantizeRowInfo *row,
  const ssize_t y,Quantum *magick_restrict *q,MagickBooleanType *status)
{
  if ((*status) == MagickFalse)
    return(MagickFalse);
  (*q)=GetCacheViewAuthenticPixels(row->image_view,0,y,row->image->columns,1,
    row->exception);
  if ((*q) == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C l a s s i f y I m a g e C o l o r s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClassifyImageColors() begins by initializing a color description tree
%  of sufficient depth to represent each possible input color in a leaf.
%  However, it is impractical to generate a fully-formed color
%  description tree in the storage_class phase for realistic values of
%  Cmax.  If colors components in the input image are quantized to k-bit
%  precision, so that Cmax= 2k-1, the tree would need k levels below the
%  root node to allow representing each possible input color in a leaf.
%  This becomes prohibitive because the tree's total number of nodes is
%  1 + sum(i=1,k,8k).
%
%  A complete tree would require 19,173,961 nodes for k = 8, Cmax = 255.
%  Therefore, to avoid building a fully populated tree, QUANTIZE: (1)
%  Initializes data structures for nodes only as they are needed;  (2)
%  Chooses a maximum depth for the tree as a function of the desired
%  number of colors in the output image (currently log2(colormap size)).
%
%  For each pixel in the input image, storage_class scans downward from
%  the root of the color description tree.  At each level of the tree it
%  identifies the single node which represents a cube in RGB space
%  containing It updates the following data for each such node:
%
%    n1 : Number of pixels whose color is contained in the RGB cube
%    which this node represents;
%
%    n2 : Number of pixels whose color is not represented in a node at
%    lower depth in the tree;  initially,  n2 = 0 for all nodes except
%    leaves of the tree.
%
%    Sr, Sg, Sb : Sums of the red, green, and blue component values for
%    all pixels not classified at a lower depth. The combination of
%    these sums and n2 will ultimately characterize the mean color of a
%    set of pixels represented by this node.
%
%    E: the distance squared in RGB space between each pixel contained
%    within a node and the nodes' center.  This represents the quantization
%    error for a node.
%
%  The format of the ClassifyImageColors() method is:
%
%      MagickBooleanType ClassifyImageColors(QCubeInfo *cube_info,
%        const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o cube_info: A pointer to the Cube structure.
%
%    o image: the image.
%
*/

static inline MagickBooleanType IsTwoColorGrayQuantize(
  const QCubeInfo *cube_info)
{
  return(((cube_info->quantize_info->number_colors == 2) &&
    ((cube_info->quantize_info->colorspace == LinearGRAYColorspace) ||
     (cube_info->quantize_info->colorspace == GRAYColorspace))) ?
    MagickTrue : MagickFalse);
}

static inline void SetAssociatedAlpha(const Image *image,QCubeInfo *cube_info)
{
  MagickBooleanType
    associate_alpha;

  associate_alpha=image->alpha_trait != UndefinedPixelTrait ? MagickTrue :
    MagickFalse;
  if (IsTwoColorGrayQuantize(cube_info) != MagickFalse)
    associate_alpha=MagickFalse;
  cube_info->associate_alpha=associate_alpha;
}

static inline ssize_t GetPixelRunLength(const Image *image,const Quantum *p,
  const ssize_t x)
{
  ssize_t
    count;

  for (count=1; (x+(ssize_t) count) < (ssize_t) image->columns; count++)
  {
    PixelInfo
      packet;

    GetPixelInfoPixel(image,p+count*(ssize_t) GetPixelChannels(image),
      &packet);
    if (IsPixelEquivalent(image,p,&packet) == MagickFalse)
      break;
  }
  return(count);
}

static inline void TransformClassifyColorspace(const QCubeInfo *cube_info,
  const Image *image,ExceptionInfo *exception)
{
  if ((cube_info->quantize_info->colorspace != UndefinedColorspace) &&
      (cube_info->quantize_info->colorspace != CMYKColorspace))
    (void) TransformImageColorspace((Image *) image,
      cube_info->quantize_info->colorspace,exception);
  else
    if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
      (void) TransformImageColorspace((Image *) image,sRGBColorspace,
        exception);
}

static inline void PruneOversizedColorTree(QCubeInfo *cube_info)
{
  if (cube_info->nodes > MaxQNodes)
    {
      /*
        Prune one level if the color tree is too large.
      */
      PruneLevel(cube_info,cube_info->root);
      cube_info->depth--;
    }
}

typedef struct _QNodeError
{
  QCubeInfo
    *cube_info;

  QNodeInfo
    *node_info;

  ssize_t
    count;
} QNodeError;

static inline DoublePixelPacket AddQNodeQuantizeError(const QNodeError *node_error,
  DoublePixelPacket pixel,DoublePixelPacket mid,DoublePixelPacket error)
{
  QCubeInfo
    *const cube_info = node_error->cube_info;

  QNodeInfo
    *const node_info = node_error->node_info;

  const ssize_t
    count = node_error->count;

  double
    distance;

  error.red=QuantumScale*(pixel.red-mid.red);
  error.green=QuantumScale*(pixel.green-mid.green);
  error.blue=QuantumScale*(pixel.blue-mid.blue);
  if (cube_info->associate_alpha != MagickFalse)
    error.alpha=QuantumScale*(pixel.alpha-mid.alpha);
  distance=(double) (error.red*error.red+error.green*error.green+
    error.blue*error.blue+error.alpha*error.alpha);
  if (IsNaN(distance) != 0)
    distance=0.0;
  node_info->quantize_error+=count*sqrt(distance);
  cube_info->root->quantize_error+=node_info->quantize_error;
  return(error);
}

static inline void SumQNodeLeafColor(const QCubeInfo *cube_info,QNodeInfo *node_info,
  const ssize_t count,const DoublePixelPacket pixel)
{
  node_info->number_unique=(size_t) ((ssize_t) node_info->number_unique+
    count);
  node_info->total_color.red+=count*QuantumScale*(double)
    ClampPixel(pixel.red);
  node_info->total_color.green+=count*QuantumScale*(double)
    ClampPixel(pixel.green);
  node_info->total_color.blue+=count*QuantumScale*(double)
    ClampPixel(pixel.blue);
  if (cube_info->associate_alpha != MagickFalse)
    node_info->total_color.alpha+=count*QuantumScale*(double)
      ClampPixel(pixel.alpha);
  else
    node_info->total_color.alpha+=count*QuantumScale*(double)
      ClampPixel((double) OpaqueAlpha);
}

static inline DoublePixelPacket MoveQNodeMidpoint(DoublePixelPacket mid,
  const size_t id,const double bisect)
{
  mid.red+=(id & 1) != 0 ? bisect : -bisect;
  mid.green+=(id & 2) != 0 ? bisect : -bisect;
  mid.blue+=(id & 4) != 0 ? bisect : -bisect;
  mid.alpha+=(id & 8) != 0 ? bisect : -bisect;
  return(mid);
}

typedef struct _QNodeDescent
{
  QCubeInfo
    *cube_info;

  const Image
    *image;

  DoublePixelPacket
    pixel,
    midpoint;

  ssize_t
    count;

  ExceptionInfo
    *exception;
} QNodeDescent;

static inline void DescendClassifyTree(const QNodeDescent *descent,
  QNodeInfo **node_info,DoublePixelPacket *error)
{
  QCubeInfo
    *const cube_info = descent->cube_info;

  const Image
    *const image = descent->image;

  const DoublePixelPacket
    pixel = descent->pixel,
    midpoint = descent->midpoint;

  const ssize_t
    count = descent->count;

  ExceptionInfo
    *const exception = descent->exception;

  double
    bisect;

  DoublePixelPacket
    mid;

  size_t
    id,
    index,
    level;

  index=MaxTreeDepth-1;
  bisect=((double) QuantumRange+1.0)/2.0;
  mid=midpoint;
  *node_info=cube_info->root;
  for (level=1; level <= MaxTreeDepth; level++)
  {
    QNodeError
      node_error;

    bisect*=0.5;
    id=ColorToQNodeId(cube_info,&pixel,index);
    mid=MoveQNodeMidpoint(mid,id,bisect);
    if ((*node_info)->child[id] == (QNodeInfo *) NULL)
      {
        /*
          Set colors of new node to contain pixel.
        */
        (*node_info)->child[id]=GetQNodeInfo(cube_info,id,level,*node_info);
        if ((*node_info)->child[id] == (QNodeInfo *) NULL)
          {
            (void) ThrowMagickException(exception,GetMagickModule(),
              ResourceLimitError,"MemoryAllocationFailed","`%s'",
              image->filename);
            continue;
          }
        if (level == MaxTreeDepth)
          cube_info->colors++;
      }
    /*
      Approximate the quantization error represented by this node.
    */
    *node_info=(*node_info)->child[id];
    node_error.cube_info=cube_info;
    node_error.node_info=*node_info;
    node_error.count=count;
    *error=AddQNodeQuantizeError(&node_error,pixel,mid,*error);
    index--;
  }
}

static inline void DescendPrunedClassifyTree(const QNodeDescent *descent,
  QNodeInfo **node_info,DoublePixelPacket *error)
{
  double
    bisect;

  DoublePixelPacket
    mid;

  size_t
    id,
    index,
    level;

  index=MaxTreeDepth-1;
  bisect=((double) QuantumRange+1.0)/2.0;
  mid=descent->midpoint;
  *node_info=descent->cube_info->root;
  for (level=1; level <= descent->cube_info->depth; level++)
  {
    QNodeError
      node_error;

    bisect*=0.5;
    id=ColorToQNodeId(descent->cube_info,&descent->pixel,index);
    mid=MoveQNodeMidpoint(mid,id,bisect);
    if ((*node_info)->child[id] == (QNodeInfo *) NULL)
      {
        /*
          Set colors of new node to contain pixel.
        */
        (*node_info)->child[id]=GetQNodeInfo(descent->cube_info,id,level,
          *node_info);
        if ((*node_info)->child[id] == (QNodeInfo *) NULL)
          {
            (void) ThrowMagickException(descent->exception,GetMagickModule(),
              ResourceLimitError,"MemoryAllocationFailed","%s",
              descent->image->filename);
            continue;
          }
        if (level == descent->cube_info->depth)
          descent->cube_info->colors++;
      }
    /*
      Approximate the quantization error represented by this node.
    */
    *node_info=(*node_info)->child[id];
    node_error.cube_info=descent->cube_info;
    node_error.node_info=*node_info;
    node_error.count=descent->count;
    *error=AddQNodeQuantizeError(&node_error,descent->pixel,mid,*error);
    index--;
  }
}

typedef struct _QCubeRow
{
  QCubeInfo
    *cube_info;

  const Image
    *image;

  DoublePixelPacket
    midpoint;

  ExceptionInfo
    *exception;
} QCubeRow;

static inline void ClassifyPrunedImageRow(const QCubeRow *row,
  const Quantum *magick_restrict *p,DoublePixelPacket *error)
{
  QCubeInfo
    *const cube_info = row->cube_info;

  const Image
    *const image = row->image;

  const DoublePixelPacket
    midpoint = row->midpoint;

  ExceptionInfo
    *const exception = row->exception;

  DoublePixelPacket
    pixel;

  QNodeDescent
    descent;

  QNodeInfo
    *node_info;

  ssize_t
    count,
    x;

  for (x=0; x < (ssize_t) image->columns; x+=(ssize_t) count)
  {
    /*
      Start at the root and descend the color cube tree.
    */
    count=GetPixelRunLength(image,*p,x);
    AssociateAlphaPixel(image,cube_info,*p,&pixel);
    descent.cube_info=cube_info;
    descent.image=image;
    descent.pixel=pixel;
    descent.midpoint=midpoint;
    descent.count=count;
    descent.exception=exception;
    DescendPrunedClassifyTree(&descent,&node_info,error);
    /*
      Sum RGB for this leaf for later derivation of the mean cube color.
    */
    node_info->number_unique=(size_t) ((ssize_t) node_info->number_unique+
      count);
    node_info->total_color.red+=count*QuantumScale*(double)
      ClampPixel(pixel.red);
    node_info->total_color.green+=count*QuantumScale*(double)
      ClampPixel(pixel.green);
    node_info->total_color.blue+=count*QuantumScale*(double)
      ClampPixel(pixel.blue);
    if (cube_info->associate_alpha != MagickFalse)
      node_info->total_color.alpha+=count*QuantumScale*(double)
        ClampPixel(pixel.alpha);
    else
      node_info->total_color.alpha+=count*QuantumScale*(double)
        ClampPixel((MagickRealType) OpaqueAlpha);
    *p+=(ptrdiff_t) count*(ssize_t) GetPixelChannels(image);
  }
}

static inline void ClassifyImageRow(const QCubeRow *row,
  const Quantum *magick_restrict *p,DoublePixelPacket *error)
{
  QCubeInfo
    *const cube_info = row->cube_info;

  const Image
    *const image = row->image;

  const DoublePixelPacket
    midpoint = row->midpoint;

  ExceptionInfo
    *const exception = row->exception;

  DoublePixelPacket
    pixel;

  QNodeDescent
    descent;

  QNodeInfo
    *node_info;

  ssize_t
    count,
    x;

  for (x=0; x < (ssize_t) image->columns; x+=(ssize_t) count)
  {
    /*
      Start at the root and descend the color cube tree.
    */
    count=GetPixelRunLength(image,*p,x);
    AssociateAlphaPixel(image,cube_info,*p,&pixel);
    descent.cube_info=cube_info;
    descent.image=image;
    descent.pixel=pixel;
    descent.midpoint=midpoint;
    descent.count=count;
    descent.exception=exception;
    DescendClassifyTree(&descent,&node_info,error);
    /*
      Sum RGB for this leaf for later derivation of the mean cube color.
    */
    SumQNodeLeafColor(cube_info,node_info,count,pixel);
    *p+=(ptrdiff_t) count*(ssize_t) GetPixelChannels(image);
  }
}

static inline void RestoreClassifyColorspace(const QCubeInfo *cube_info,
  const Image *image,ExceptionInfo *exception)
{
  if (cube_info->quantize_info->colorspace != image->colorspace)
    if ((cube_info->quantize_info->colorspace != UndefinedColorspace) &&
        (cube_info->quantize_info->colorspace != CMYKColorspace))
      (void) TransformImageColorspace((Image *) image,sRGBColorspace,exception);
}

static inline DoublePixelPacket GetQCubeMidpoint(void)
{
  DoublePixelPacket
    midpoint;

  midpoint.red=(double) QuantumRange/2.0;
  midpoint.green=(double) QuantumRange/2.0;
  midpoint.blue=(double) QuantumRange/2.0;
  midpoint.alpha=(double) QuantumRange/2.0;
  return(midpoint);
}

#define ClassifyImageTag  "Classify/Image"

typedef struct _QCubeRows
{
  QCubeInfo
    *cube_info;

  const Image
    *image;

  CacheView
    *image_view;

  DoublePixelPacket
    midpoint;

  ExceptionInfo
    *exception;
} QCubeRows;

static inline void ClassifyPrunedImageRows(const QCubeRows *rows,ssize_t *y,
  DoublePixelPacket *error)
{
  QCubeInfo
    *const cube_info = rows->cube_info;

  const Image
    *const image = rows->image;

  CacheView
    *const image_view = rows->image_view;

  const DoublePixelPacket
    midpoint = rows->midpoint;

  ExceptionInfo
    *const exception = rows->exception;

  MagickBooleanType
    proceed;

  for ((*y)++; *y < (ssize_t) image->rows; (*y)++)
  {
    const Quantum
      *magick_restrict p;

    QCubeRow
      row;

    p=GetCacheViewVirtualPixels(image_view,0,*y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    PruneOversizedColorTree(cube_info);
    row.cube_info=cube_info;
    row.image=image;
    row.midpoint=midpoint;
    row.exception=exception;
    ClassifyPrunedImageRow(&row,&p,error);
    proceed=SetImageProgress(image,ClassifyImageTag,(MagickOffsetType) *y,
      image->rows);
    if (proceed == MagickFalse)
      break;
  }
}

static inline MagickBooleanType ClassifyImageColors(QCubeInfo *cube_info,
  const Image *image,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  DoublePixelPacket
    error,
    midpoint;

  MagickBooleanType
    proceed;

  QCubeRows
    rows;

  ssize_t
    y;

  /*
    Classify the first cube_info->maximum_colors colors to a tree depth of 8.
  */
  SetAssociatedAlpha(image,cube_info);
  if (cube_info->quantize_info->colorspace != image->colorspace)
    TransformClassifyColorspace(cube_info,image,exception);
  midpoint=GetQCubeMidpoint();
  error.alpha=0.0;
  image_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    QCubeRow
      row;

    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    PruneOversizedColorTree(cube_info);
    row.cube_info=cube_info;
    row.image=image;
    row.midpoint=midpoint;
    row.exception=exception;
    ClassifyImageRow(&row,&p,&error);
    if (cube_info->colors > cube_info->maximum_colors)
      {
        PruneToCubeDepth(cube_info,cube_info->root);
        break;
      }
    proceed=SetImageProgress(image,ClassifyImageTag,(MagickOffsetType) y,
      image->rows);
    if (proceed == MagickFalse)
      break;
  }
  rows.cube_info=cube_info;
  rows.image=image;
  rows.image_view=image_view;
  rows.midpoint=midpoint;
  rows.exception=exception;
  ClassifyPrunedImageRows(&rows,&y,&error);
  image_view=DestroyCacheView(image_view);
  RestoreClassifyColorspace(cube_info,image,exception);
  return(y < (ssize_t) image->rows ? MagickFalse : MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e f i n e I m a g e C o l o r m a p                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DefineImageColormap() traverses the color cube tree and notes each colormap
%  entry.  A colormap entry is any node in the color cube tree where the
%  of unique colors is not zero.
%
%  The format of the DefineImageColormap method is:
%
%      void DefineImageColormap(Image *image,QCubeInfo *cube_info,
%        QNodeInfo *node_info)
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
typedef struct _ColormapEntryInputs
{
  Image
    *image;

  QCubeInfo
    *cube_info;

  QNodeInfo
    *node_info;

  double
    alpha;
} ColormapEntryInputs;

static inline void SetAlphaColormapEntry(const ColormapEntryInputs *entry,
  PixelInfo *magick_restrict q)
{
  Image
    *const image = entry->image;

  QCubeInfo
    *const cube_info = entry->cube_info;

  QNodeInfo
    *const node_info = entry->node_info;

  const double
    alpha = entry->alpha;

  double
    opacity;

  opacity=(double) (alpha*(double) QuantumRange*
    node_info->total_color.alpha);
  q->alpha=(double) ClampToQuantum(opacity);
  if (q->alpha == (double) OpaqueAlpha)
    {
      q->red=(double) ClampToQuantum(alpha*(double) QuantumRange*
        node_info->total_color.red);
      q->green=(double) ClampToQuantum(alpha*(double) QuantumRange*
        node_info->total_color.green);
      q->blue=(double) ClampToQuantum(alpha*(double) QuantumRange*
        node_info->total_color.blue);
    }
  else
    {
      double
        gamma;

      gamma=(double) (QuantumScale*q->alpha);
      gamma=MagickSafeReciprocal(gamma);
      q->red=(double) ClampToQuantum(alpha*gamma*(double) QuantumRange*
        node_info->total_color.red);
      q->green=(double) ClampToQuantum(alpha*gamma*(double)
        QuantumRange*node_info->total_color.green);
      q->blue=(double) ClampToQuantum(alpha*gamma*(double) QuantumRange*
        node_info->total_color.blue);
      if (node_info->number_unique > cube_info->transparent_pixels)
        {
          cube_info->transparent_pixels=node_info->number_unique;
          cube_info->transparent_index=(ssize_t) image->colors;
        }
    }
}

static inline void DefineImageColormap(Image *image,QCubeInfo *cube_info,
  QNodeInfo *node_info)
{
  double
    alpha;

  PixelInfo
    *magick_restrict q;

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
      DefineImageColormap(image,cube_info,node_info->child[i]);
  if (node_info->number_unique == 0)
    return;
  /*
    Colormap entry is defined by the mean color in this cube.
  */
  q=image->colormap+image->colors;
  alpha=(double) ((MagickOffsetType) node_info->number_unique);
  alpha=MagickSafeReciprocal(alpha);
  if (cube_info->associate_alpha == MagickFalse)
    {
      q->red=(double) ClampToQuantum(alpha*(double) QuantumRange*
        node_info->total_color.red);
      q->green=(double) ClampToQuantum(alpha*(double) QuantumRange*
        node_info->total_color.green);
      q->blue=(double) ClampToQuantum(alpha*(double) QuantumRange*
        node_info->total_color.blue);
      q->alpha=(double) OpaqueAlpha;
    }
  else
    {
      ColormapEntryInputs
        entry;

      entry.image=image;
      entry.cube_info=cube_info;
      entry.node_info=node_info;
      entry.alpha=alpha;
      SetAlphaColormapEntry(&entry,q);
    }
  node_info->color_number=image->colors++;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y Q C u b e I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyQCubeInfo() deallocates memory associated with an image.
%
%  The format of the DestroyQCubeInfo method is:
%
%      DestroyQCubeInfo(QCubeInfo *cube_info)
%
%  A description of each parameter follows:
%
%    o cube_info: the address of a structure of type QCubeInfo.
%
*/
static inline void DestroyQCubeInfo(QCubeInfo *cube_info)
{
  QNodes
    *nodes;

  ssize_t
    i;

  /*
    Release color cube tree storage.
  */
  do
  {
    nodes=cube_info->node_queue->next;
    cube_info->node_queue->nodes=(QNodeInfo *) RelinquishMagickMemory(
      cube_info->node_queue->nodes);
    cube_info->node_queue=(QNodes *) RelinquishMagickMemory(
      cube_info->node_queue);
    cube_info->node_queue=nodes;
  } while (cube_info->node_queue != (QNodes *) NULL);
  if (cube_info->cache != (ssize_t **) NULL)
    {
      for (i=0; i < (ssize_t) CacheBlocks; i++)
        if (cube_info->cache[i] != (ssize_t *) NULL)
          cube_info->cache[i]=(ssize_t *) RelinquishMagickMemory(
            cube_info->cache[i]);
      cube_info->cache=(ssize_t **) RelinquishMagickMemory(cube_info->cache);
    }
  cube_info->quantize_info=DestroyQuantizeInfo(cube_info->quantize_info);
  cube_info=(QCubeInfo *) RelinquishMagickMemory(cube_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t Q C u b e I n f o                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetQCubeInfo() initialize the Cube data structure.
%
%  The format of the GetQCubeInfo method is:
%
%      QCubeInfo GetQCubeInfo(const QuantizeInfo *quantize_info,
%        const size_t depth,const size_t maximum_colors)
%
%  A description of each parameter follows.
%
%    o quantize_info: Specifies a pointer to an QuantizeInfo structure.
%
%    o depth: Normally, this integer value is zero or one.  A zero or
%      one tells Quantize to choose a optimal tree depth of Log4(number_colors).
%      A tree of this depth generally allows the best representation of the
%      reference image with the least amount of memory and the fastest
%      computational speed.  In some cases, such as an image with low color
%      dispersion (a few number of colors), a value other than
%      Log4(number_colors) is required.  To expand the color tree completely,
%      use a value of 8.
%
%    o maximum_colors: maximum colors.
%
*/
static inline void SetQCubeDepth(QCubeInfo *cube_info,const size_t depth)
{
  cube_info->depth=depth;
  if (cube_info->depth > MaxTreeDepth)
    cube_info->depth=MaxTreeDepth;
  if (cube_info->depth < 2)
    cube_info->depth=2;
}

static inline void DistributeQCubeWeights(QCubeInfo *cube_info)
{
  double
    weight;

  ssize_t
    i;

  weight=1.0;
  for (i=0; i < ErrorQueueLength; i++)
  {
    cube_info->weights[i]=MagickSafeReciprocal(weight);
    weight*=exp(log(1.0/ErrorRelativeWeight)/(ErrorQueueLength-1.0));
  }
}

static inline QCubeInfo *GetQCubeInfo(const QuantizeInfo *quantize_info,
  const size_t depth,const size_t maximum_colors)
{
  QCubeInfo
    *cube_info;

  /*
    Initialize tree to describe color cube_info.
  */
  cube_info=(QCubeInfo *) AcquireMagickMemory(sizeof(*cube_info));
  if (cube_info == (QCubeInfo *) NULL)
    return((QCubeInfo *) NULL);
  (void) memset(cube_info,0,sizeof(*cube_info));
  SetQCubeDepth(cube_info,depth);
  cube_info->maximum_colors=maximum_colors;
  /*
    Initialize root node.
  */
  cube_info->root=GetQNodeInfo(cube_info,0,0,(QNodeInfo *) NULL);
  if (cube_info->root == (QNodeInfo *) NULL)
    return((QCubeInfo *) NULL);
  cube_info->root->parent=cube_info->root;
  cube_info->quantize_info=CloneQuantizeInfo(quantize_info);
  if (cube_info->quantize_info->dither_method == NoDitherMethod)
    return(cube_info);
  /*
    Initialize dither resources.
  */
  cube_info->cache=(ssize_t **) AcquireQuantumMemory(CacheBlocks,
    sizeof(*cube_info->cache));
  if (cube_info->cache == (ssize_t **) NULL)
    return((QCubeInfo *) NULL);
  (void) memset(cube_info->cache,0,CacheBlocks*sizeof(*cube_info->cache));
  /*
    Distribute weights along a curve of exponential decay.
  */
  DistributeQCubeWeights(cube_info);
  cube_info->diffusion=1.0;
  return(cube_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t N o d e I n f o                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetQNodeInfo() allocates memory for a new node in the color cube tree and
%  presets all fields to zero.
%
%  The format of the GetQNodeInfo method is:
%
%      QNodeInfo *GetQNodeInfo(QCubeInfo *cube_info,const size_t id,
%        const size_t level,QNodeInfo *parent)
%
%  A description of each parameter follows.
%
%    o node: The GetQNodeInfo method returns a pointer to a queue of nodes.
%
%    o id: Specifies the child number of the node.
%
%    o level: Specifies the level in the storage_class the node resides.
%
*/
static inline QNodeInfo *GetQNodeInfo(QCubeInfo *cube_info,const size_t id,
  const size_t level,QNodeInfo *parent)
{
  QNodeInfo
    *node_info;

  if (cube_info->free_nodes == 0)
    {
      QNodes
        *nodes;

      /*
        Allocate a new queue of nodes.
      */
      nodes=(QNodes *) AcquireMagickMemory(sizeof(*nodes));
      if (nodes == (QNodes *) NULL)
        return((QNodeInfo *) NULL);
      nodes->nodes=(QNodeInfo *) AcquireQuantumMemory(QNodesInAList,
        sizeof(*nodes->nodes));
      if (nodes->nodes == (QNodeInfo *) NULL)
        return((QNodeInfo *) NULL);
      nodes->next=cube_info->node_queue;
      cube_info->node_queue=nodes;
      cube_info->next_node=nodes->nodes;
      cube_info->free_nodes=QNodesInAList;
    }
  cube_info->nodes++;
  cube_info->free_nodes--;
  node_info=cube_info->next_node++;
  (void) memset(node_info,0,sizeof(*node_info));
  node_info->parent=parent;
  node_info->id=id;
  node_info->level=level;
  return(node_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   P r u n e C h i l d                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PruneChild() deletes the given node and merges its statistics into its
%  parent.
%
%  The format of the PruneSubtree method is:
%
%      PruneChild(QCubeInfo *cube_info,const QNodeInfo *node_info)
%
%  A description of each parameter follows.
%
%    o cube_info: A pointer to the Cube structure.
%
%    o node_info: pointer to node in color cube tree that is to be pruned.
%
*/
static inline void PruneChild(QCubeInfo *cube_info,const QNodeInfo *node_info)
{
  QNodeInfo
    *parent;

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
      PruneChild(cube_info,node_info->child[i]);
  if (cube_info->nodes > cube_info->maximum_colors)
    {
      /*
        Merge color statistics into parent.
      */
      parent=node_info->parent;
      parent->number_unique+=node_info->number_unique;
      parent->total_color.red+=node_info->total_color.red;
      parent->total_color.green+=node_info->total_color.green;
      parent->total_color.blue+=node_info->total_color.blue;
      parent->total_color.alpha+=node_info->total_color.alpha;
      parent->child[node_info->id]=(QNodeInfo *) NULL;
      cube_info->nodes--;
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  P r u n e L e v e l                                                        %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PruneLevel() deletes all nodes at the bottom level of the color tree merging
%  their color statistics into their parent node.
%
%  The format of the PruneLevel method is:
%
%      PruneLevel(QCubeInfo *cube_info,const QNodeInfo *node_info)
%
%  A description of each parameter follows.
%
%    o cube_info: A pointer to the Cube structure.
%
%    o node_info: pointer to node in color cube tree that is to be pruned.
%
*/
static inline void PruneLevel(QCubeInfo *cube_info,const QNodeInfo *node_info)
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
      PruneLevel(cube_info,node_info->child[i]);
  if (node_info->level == cube_info->depth)
    PruneChild(cube_info,node_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  P r u n e T o C u b e D e p t h                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PruneToCubeDepth() deletes any nodes at a depth greater than
%  cube_info->depth while merging their color statistics into their parent
%  node.
%
%  The format of the PruneToCubeDepth method is:
%
%      PruneToCubeDepth(QCubeInfo *cube_info,const QNodeInfo *node_info)
%
%  A description of each parameter follows.
%
%    o cube_info: A pointer to the Cube structure.
%
%    o node_info: pointer to node in color cube tree that is to be pruned.
%
*/
static inline void PruneToCubeDepth(QCubeInfo *cube_info,const QNodeInfo *node_info)
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
      PruneToCubeDepth(cube_info,node_info->child[i]);
  if (node_info->level > cube_info->depth)
    PruneChild(cube_info,node_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   Q u a n t i z e E r r o r F l a t t e n                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  QuantizeErrorFlatten() traverses the color cube and flattens the quantization
%  error into a sorted 1D array.  This accelerates the color reduction process.
%
%  Contributed by Yoya.
%
%  The format of the QuantizeErrorFlatten method is:
%
%      size_t QuantizeErrorFlatten(const QCubeInfo *cube_info,
%        const QNodeInfo *node_info,const ssize_t offset,
%        double *quantize_error)
%
%  A description of each parameter follows.
%
%    o cube_info: A pointer to the Cube structure.
%
%    o node_info: pointer to node in color cube tree that is current pointer.
%
%    o offset: quantize error offset.
%
%    o quantize_error: the quantization error vector.
%
*/
static inline size_t QuantizeErrorFlatten(const QCubeInfo *cube_info,
  const QNodeInfo *node_info,const ssize_t offset,double *quantize_error)
{
  size_t
    n,
    number_children;

  ssize_t
    i;

  if (offset >= (ssize_t) cube_info->nodes)
    return(0);
  quantize_error[offset]=node_info->quantize_error;
  n=1;
  number_children=cube_info->associate_alpha == MagickFalse ? 8UL : 16UL;
  for (i=0; i < (ssize_t) number_children ; i++)
    if (node_info->child[i] != (QNodeInfo *) NULL)
      n+=QuantizeErrorFlatten(cube_info,node_info->child[i],offset+(ssize_t) n,
        quantize_error);
  return(n);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e d u c e                                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Reduce() traverses the color cube tree and prunes any node whose
%  quantization error falls below a particular threshold.
%
%  The format of the Reduce method is:
%
%      Reduce(QCubeInfo *cube_info,const QNodeInfo *node_info)
%
%  A description of each parameter follows.
%
%    o cube_info: A pointer to the Cube structure.
%
%    o node_info: pointer to node in color cube tree that is to be pruned.
%
*/
static inline void UpdateNextPruningThreshold(QCubeInfo *cube_info,
  const QNodeInfo *node_info)
{
  /*
    Find minimum pruning threshold.
  */
  if (node_info->number_unique > 0)
    cube_info->colors++;
  if (node_info->quantize_error < cube_info->next_threshold)
    cube_info->next_threshold=node_info->quantize_error;
}

static inline void Reduce(QCubeInfo *cube_info,const QNodeInfo *node_info)
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
      Reduce(cube_info,node_info->child[i]);
  if (node_info->quantize_error <= cube_info->pruning_threshold)
    PruneChild(cube_info,node_info);
  else
    UpdateNextPruningThreshold(cube_info,node_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e d u c e I m a g e C o l o r s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReduceImageColors() repeatedly prunes the tree until the number of nodes
%  with n2 > 0 is less than or equal to the maximum number of colors allowed
%  in the output image.  On any given iteration over the tree, it selects
%  those nodes whose E value is minimal for pruning and merges their
%  color statistics upward. It uses a pruning threshold, Ep, to govern
%  node selection as follows:
%
%    Ep = 0
%    while number of nodes with (n2 > 0) > required maximum number of colors
%      prune all nodes such that E <= Ep
%      Set Ep to minimum E in remaining nodes
%
%  This has the effect of minimizing any quantization error when merging
%  two nodes together.
%
%  When a node to be pruned has offspring, the pruning procedure invokes
%  itself recursively in order to prune the tree from the leaves upward.
%  n2,  Sr, Sg,  and  Sb in a node being pruned are always added to the
%  corresponding data in that node's parent.  This retains the pruned
%  node's color characteristics for later averaging.
%
%  For each node, n2 pixels exist for which that node represents the
%  smallest volume in RGB space containing those pixel's colors.  When n2
%  > 0 the node will uniquely define a color in the output image. At the
%  beginning of reduction,  n2 = 0  for all nodes except a the leaves of
%  the tree which represent colors present in the input image.
%
%  The other pixel count, n1, indicates the total number of colors
%  within the cubic volume which the node represents.  This includes n1 -
%  n2  pixels whose colors should be defined by nodes at a lower level in
%  the tree.
%
%  The format of the ReduceImageColors method is:
%
%      ReduceImageColors(const Image *image,QCubeInfo *cube_info)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o cube_info: A pointer to the Cube structure.
%
*/

static inline int QuantizeErrorCompare(const void *error_p,const void *error_q)
{
  double
    *p,
    *q;

  p=(double *) error_p;
  q=(double *) error_q;
  if (*p > *q)
    return(1);
  if (fabs(*q-*p) <= MagickEpsilon)
    return(0);
  return(-1);
}

static inline void SetRapidReductionThreshold(QCubeInfo *cube_info)
{
  double
    *quantize_error;

  /*
    Enable rapid reduction of the number of unique colors.
  */
  quantize_error=(double *) AcquireQuantumMemory(cube_info->nodes,
    sizeof(*quantize_error));
  if (quantize_error != (double *) NULL)
    {
      (void) QuantizeErrorFlatten(cube_info,cube_info->root,0,
        quantize_error);
      qsort(quantize_error,cube_info->nodes,sizeof(double),
        QuantizeErrorCompare);
      if (cube_info->nodes > (110*(cube_info->maximum_colors+1)/100))
        cube_info->next_threshold=quantize_error[cube_info->nodes-110*
          (cube_info->maximum_colors+1)/100];
      quantize_error=(double *) RelinquishMagickMemory(quantize_error);
    }
}

static inline void ReduceImageColors(const Image *image,QCubeInfo *cube_info)
{
#define ReduceImageTag  "Reduce/Image"

  MagickBooleanType
    proceed;

  MagickOffsetType
    offset;

  size_t
    span;

  cube_info->next_threshold=0.0;
  if (cube_info->colors > cube_info->maximum_colors)
    SetRapidReductionThreshold(cube_info);
  for (span=cube_info->colors; cube_info->colors > cube_info->maximum_colors; )
  {
    cube_info->pruning_threshold=cube_info->next_threshold;
    cube_info->next_threshold=cube_info->root->quantize_error-1;
    cube_info->colors=0;
    Reduce(cube_info,cube_info->root);
    offset=(MagickOffsetType) span-(MagickOffsetType) cube_info->colors;
    proceed=SetImageProgress(image,ReduceImageTag,offset,span-
      cube_info->maximum_colors+1);
    if (proceed == MagickFalse)
      break;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t I m a g e C o l o r m a p                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageColormap() traverses the color cube tree and sets the colormap of
%  the image.  A colormap entry is any node in the color cube tree where the
%  of unique colors is not zero.
%
%  The format of the SetImageColormap method is:
%
%      MagickBooleanType SetImageColormap(Image *image,QCubeInfo *cube_info,
%        ExceptionInfo *node_info)
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
MagickBooleanType SetImageColormap(Image *image,QCubeInfo *cube_info,
  ExceptionInfo *exception)
{
  size_t
    number_colors;

  number_colors=MagickMax(cube_info->maximum_colors,cube_info->colors);
  if (AcquireImageColormap(image,number_colors,exception) == MagickFalse)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  image->colors=0;
  DefineImageColormap(image,cube_info,cube_info->root);
  if (image->colors != number_colors)
    {
      image->colormap=(PixelInfo *) ResizeQuantumMemory(image->colormap,
        image->colors+1,sizeof(*image->colormap));
      if (image->colormap == (PixelInfo *) NULL)
        ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
          image->filename);
    }
  return(MagickTrue);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
