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

static MagickBooleanType
  AssignImageColors(Image *,QCubeInfo *,ExceptionInfo *),
  DitherImage(Image *,QCubeInfo *,ExceptionInfo *);

static void
  ClosestColor(const Image *,QCubeInfo *,const QNodeInfo *);

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

static inline void RiemersmaVerticalUnitCurve(const RiemersmaCurve *curve,
  const unsigned int direction,MagickBooleanType *status,
  ExceptionInfo *exception)
{
  switch (direction)
  {
    case NorthGravity:
    {
      *status=RiemersmaDither(curve,SouthGravity,exception);
      if (*status != MagickFalse)
        *status=RiemersmaDither(curve,EastGravity,exception);
      if (*status != MagickFalse)
        *status=RiemersmaDither(curve,NorthGravity,exception);
      break;
    }
    case SouthGravity:
    {
      *status=RiemersmaDither(curve,NorthGravity,exception);
      if (*status != MagickFalse)
        *status=RiemersmaDither(curve,WestGravity,exception);
      if (*status != MagickFalse)
        *status=RiemersmaDither(curve,SouthGravity,exception);
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
