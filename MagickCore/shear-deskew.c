/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      SSSSS  H   H  EEEEE   AAA   RRRR                       %
%                      SS     H   H  E      A   A  R   R                      %
%                       SSS   HHHHH  EEE    AAAAA  RRRR                       %
%                         SS  H   H  E      A   A  R R                        %
%                      SSSSS  H   H  EEEEE  A   A  R  R                       %
%                                                                             %
%                  DDDD   EEEEE  SSSSS  K   K  EEEEE  W   W                   %
%                  D   D  E      SS     K  K   E      W   W                   %
%                  D   D  EEE     SSS   KKK    EEE    W W W                   %
%                  D   D  E         SS  K  K   E      WW WW                   %
%                  DDDD   EEEEE  SSSSS  K   K  EEEEE  W   W                   %
%                                                                             %
%                                                                             %
%                       MagickCore Image Deskew Method                        %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                  July 1992                                  %
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
%  The XShearImage() and YShearImage() methods are based on the paper "A Fast
%  Algorithm for General Raster Rotation" by Alan W. Paeth, Graphics
%  Interface '86 (Vancouver).  ShearRotateImage() is adapted from a similar
%  method based on the Paeth paper written by Michael Halle of the Spatial
%  Imaging Group, MIT Media Lab.
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/channel.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/decorate.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/matrix.h"
#include "MagickCore/memory_.h"
#include "MagickCore/list.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile-private.h"
#include "MagickCore/quantum.h"
#include "MagickCore/resource_.h"
#include "MagickCore/shear.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/transform.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     D e s k e w I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DeskewImage() removes skew from the image.  Skew is an artifact that
%  occurs in scanned images because of the camera being misaligned,
%  imperfections in the scanning or surface, or simply because the paper was
%  not placed completely flat when scanned.
%
%  The result will be auto-cropped if the artifact "deskew:auto-crop" is
%  defined, while the amount the image is to be deskewed, in degrees is also
%  saved as the artifact "deskew:angle".
%
%  The format of the DeskewImage method is:
%
%      Image *DeskewImage(const Image *image,const double threshold,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o threshold: separate background from foreground.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static size_t SumRadonColumnDeltas(MatrixInfo *p,const ssize_t x)
{
  size_t
    sum;

  ssize_t
    y;

  sum=0;
  for (y=0; y < (ssize_t) (GetMatrixRows(p)-1); y++)
  {
    ssize_t
      delta;

    unsigned short
      element,
      neighbor;

    if (GetMatrixElement(p,x,y,&element) == MagickFalse)
      continue;
    if (GetMatrixElement(p,x,y+1,&neighbor) == MagickFalse)
      continue;
    delta=(ssize_t) element-(ssize_t) neighbor;
    sum+=(size_t) (delta*delta);
  }
  return(sum);
}

typedef struct _RadonCell
{
  MatrixInfo
    *p;

  ssize_t
    x,
    i;

  size_t
    step;

  ssize_t
    y;
} RadonCell;

typedef struct _RadonPairTargets
{
  unsigned short
    *element,
    *neighbor;
} RadonPairTargets;

static MagickBooleanType MergeRadonPairHead(const RadonCell *cell,
  MatrixInfo *q,const RadonPairTargets *targets)
{
  unsigned short
    *const element = targets->element,
    *const neighbor = targets->neighbor;

  if (GetMatrixElement(cell->p,cell->x+cell->i,cell->y,element) == MagickFalse)
    return(MagickFalse);
  if (GetMatrixElement(cell->p,cell->x+cell->i+(ssize_t) cell->step,
      cell->y+cell->i,neighbor) == MagickFalse)
    return(MagickFalse);
  *neighbor+=*element;
  if (SetMatrixElement(q,cell->x+2*cell->i,cell->y,neighbor) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static void MergeRadonPair(MatrixInfo *q,const RadonCell *cell)
{
  RadonPairTargets
    targets;

  unsigned short
    element,
    neighbor;

  targets.element=(&element);
  targets.neighbor=(&neighbor);
  if (MergeRadonPairHead(cell,q,&targets) == MagickFalse)
    return;
  if (GetMatrixElement(cell->p,cell->x+cell->i+(ssize_t) cell->step,
      cell->y+cell->i+1,&neighbor) == MagickFalse)
    return;
  neighbor+=element;
  if (SetMatrixElement(q,cell->x+2*cell->i+1,cell->y,&neighbor) == MagickFalse)
    return;
}

typedef struct _RadonEdgeCell
{
  MatrixInfo
    *p;

  ssize_t
    x,
    i;
} RadonEdgeCell;

static void CopyRadonElement(MatrixInfo *q,const ssize_t y,
  const RadonEdgeCell *edge)
{
  unsigned short
    element;

  if (GetMatrixElement(edge->p,edge->x+edge->i,y,&element) == MagickFalse)
    return;
  if (SetMatrixElement(q,edge->x+2*edge->i,y,&element) == MagickFalse)
    return;
  if (SetMatrixElement(q,edge->x+2*edge->i+1,y,&element) == MagickFalse)
    return;
}

static void RadonProjection(MatrixInfo *source_matrices,
  MatrixInfo *destination_matrices,const ssize_t sign,size_t *projection)
{
  MatrixInfo
    *p,
    *q,
    *swap;

  size_t
    step;

  ssize_t
    x;

  p=source_matrices;
  q=destination_matrices;
  for (step=1; step < GetMatrixColumns(p); step*=2)
  {
    for (x=0; x < (ssize_t) GetMatrixColumns(p); x+=2*(ssize_t) step)
    {
      ssize_t
        i,
        y;

      unsigned short
        element,
        neighbor;

      for (i=0; i < (ssize_t) step; i++)
      {
        for (y=0; y < ((ssize_t) GetMatrixRows(p)-i-1); y++)
        {
          RadonCell
            cell;

          cell.p=p;
          cell.x=x;
          cell.i=i;
          cell.step=step;
          cell.y=y;
          MergeRadonPair(q,&cell);
        }
        for ( ; y < ((ssize_t) GetMatrixRows(p)-i); y++)
        {
          if (GetMatrixElement(p,x+i,y,&element) == MagickFalse)
            continue;
          if (GetMatrixElement(p,x+i+(ssize_t) step,y+i,&neighbor) == MagickFalse)
            continue;
          neighbor+=element;
          if (SetMatrixElement(q,x+2*i,y,&neighbor) == MagickFalse)
            continue;
          if (SetMatrixElement(q,x+2*i+1,y,&element) == MagickFalse)
            continue;
        }
        for ( ; y < (ssize_t) GetMatrixRows(p); y++)
        {
          RadonEdgeCell
            edge;

          edge.p=p;
          edge.x=x;
          edge.i=i;
          CopyRadonElement(q,y,&edge);
        }
      }
    }
    swap=p;
    p=q;
    q=swap;
  }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) \
    num_threads((int) GetMagickResourceLimit(ThreadResource))
#endif
  for (x=0; x < (ssize_t) GetMatrixColumns(p); x++)
  {
    size_t
      sum;

    sum=SumRadonColumnDeltas(p,x);
    projection[(ssize_t) GetMatrixColumns(p)+sign*x-1]=sum;
  }
}

static inline MagickBooleanType IsPixelBelowThreshold(const Image *image,
  const Quantum *magick_restrict p,const double threshold)
{
  return((((MagickRealType) GetPixelRed(image,p) < threshold) ||
    ((MagickRealType) GetPixelGreen(image,p) < threshold) ||
    ((MagickRealType) GetPixelBlue(image,p) < threshold)) ?
    MagickTrue : MagickFalse);
}

static void FillBitCountTable(unsigned short *bits)
{
  size_t
    count;

  ssize_t
    j;

  unsigned char
    c;

  for (j=0; j < 256; j++)
  {
    c=(unsigned char) j;
    for (count=0; c != 0; c>>=1)
      count+=c & 0x01;
    bits[j]=(unsigned short) count;
  }
}

typedef struct _RadonRow
{
  const Image
    *image;

  double
    threshold;

  ssize_t
    y;
} RadonRow;

static void PackRadonRowReversed(const Quantum *magick_restrict p,
  MatrixInfo *source_matrices,unsigned short *bits,const RadonRow *row)
{
  const Image
    *const image = row->image;

  const double
    threshold = row->threshold;

  const ssize_t
    y = row->y;

  size_t
    bit,
    byte;

  ssize_t
    i,
    x;

  unsigned short
    value;

  bit=0;
  byte=0;
  i=(ssize_t) (image->columns+7)/8;
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    byte<<=1;
    if (IsPixelBelowThreshold(image,p,threshold) != MagickFalse)
      byte|=0x01;
    bit++;
    if (bit == 8)
      {
        value=bits[byte];
        (void) SetMatrixElement(source_matrices,--i,y,&value);
        bit=0;
        byte=0;
      }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (bit != 0)
    {
      byte<<=(8-bit);
      value=bits[byte];
      (void) SetMatrixElement(source_matrices,--i,y,&value);
    }
}

static void DestroyRadonMatrices(MatrixInfo *source_matrices,
  MatrixInfo *destination_matrices)
{
  if (destination_matrices != (MatrixInfo *) NULL)
    destination_matrices=DestroyMatrixInfo(destination_matrices);
  if (source_matrices != (MatrixInfo *) NULL)
    source_matrices=DestroyMatrixInfo(source_matrices);
}

static MagickBooleanType RadonTransform(const Image *image,
  const double threshold,size_t *projection,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MatrixInfo
    *destination_matrices,
    *source_matrices;

  MagickBooleanType
    status;

  size_t
    width;

  ssize_t
    y;

  unsigned short
    bits[256];

  for (width=1; width < ((image->columns+7)/8); width<<=1) ;
  source_matrices=AcquireMatrixInfo(width,image->rows,sizeof(unsigned short),
    exception);
  destination_matrices=AcquireMatrixInfo(width,image->rows,
    sizeof(unsigned short),exception);
  if ((source_matrices == (MatrixInfo *) NULL) ||
      (destination_matrices == (MatrixInfo *) NULL))
    {
      DestroyRadonMatrices(source_matrices,destination_matrices);
      return(MagickFalse);
    }
  if (NullMatrix(source_matrices) == MagickFalse)
    {
      destination_matrices=DestroyMatrixInfo(destination_matrices);
      source_matrices=DestroyMatrixInfo(source_matrices);
      return(MagickFalse);
    }
  FillBitCountTable(bits);
  status=MagickTrue;
  image_view=AcquireVirtualCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    RadonRow
      row;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    row.image=image;
    row.threshold=threshold;
    row.y=y;
    PackRadonRowReversed(p,source_matrices,bits,&row);
  }
  RadonProjection(source_matrices,destination_matrices,-1,projection);
  (void) NullMatrix(source_matrices);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    size_t
      bit,
      byte;

    ssize_t
      i,
      x;

    unsigned short
     value;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    bit=0;
    byte=0;
    i=0;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      byte<<=1;
      if (IsPixelBelowThreshold(image,p,threshold) != MagickFalse)
        byte|=0x01;
      bit++;
      if (bit == 8)
        {
          value=bits[byte];
          (void) SetMatrixElement(source_matrices,i++,y,&value);
          bit=0;
          byte=0;
        }
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (bit != 0)
      {
        byte<<=(8-bit);
        value=bits[byte];
        (void) SetMatrixElement(source_matrices,i++,y,&value);
      }
  }
  RadonProjection(source_matrices,destination_matrices,1,projection);
  image_view=DestroyCacheView(image_view);
  destination_matrices=DestroyMatrixInfo(destination_matrices);
  source_matrices=DestroyMatrixInfo(source_matrices);
  return(MagickTrue);
}

static PixelInfo AccumulateBackgroundPixel(const Image *image,
  const Quantum *magick_restrict p,PixelInfo background)
{
  background.red+=QuantumScale*(double) GetPixelRed(image,p);
  background.green+=QuantumScale*(double) GetPixelGreen(image,p);
  background.blue+=QuantumScale*(double) GetPixelBlue(image,p);
  if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
    background.alpha+=QuantumScale*(double) GetPixelAlpha(image,p);
  return(background);
}

static void SetAverageBackgroundColor(Image *image,const PixelInfo background,
  const double count)
{
  image->background_color.red=(double) ClampToQuantum((double) QuantumRange*
    (double) background.red/count);
  image->background_color.green=(double) ClampToQuantum((double) QuantumRange*
    (double) background.green/count);
  image->background_color.blue=(double) ClampToQuantum((double) QuantumRange*
    (double) background.blue/count);
  if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
    image->background_color.alpha=(double) ClampToQuantum((double) QuantumRange*
      (double) background.alpha/count);
}

typedef struct _BackgroundRow
{
  Image
    *image;

  ssize_t
    offset;
} BackgroundRow;

static void AccumulateBackgroundRow(const BackgroundRow *row,
  PixelInfo *background,double *count,const Quantum *magick_restrict *p)
{
  Image
    *const image = row->image;

  const ssize_t
    offset = row->offset;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if ((x >= offset) && (x < ((ssize_t) image->columns-offset)))
      continue;
    *background=AccumulateBackgroundPixel(image,*p,*background);
    (*count)++;
    *p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void GetImageBackgroundColor(Image *image,const ssize_t offset,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  double
    count;

  PixelInfo
    background;

  ssize_t
    y;

  /*
    Compute average background color.
  */
  if (offset <= 0)
    return;
  GetPixelInfo(image,&background);
  count=0.0;
  image_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    BackgroundRow
      row;

    const Quantum
      *magick_restrict p;

    if ((y >= offset) && (y < ((ssize_t) image->rows-offset)))
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      continue;
    row.image=image;
    row.offset=offset;
    AccumulateBackgroundRow(&row,&background,&count,&p);
  }
  image_view=DestroyCacheView(image_view);
  SetAverageBackgroundColor(image,background,count);
}

static ssize_t FindDeskewSkew(const size_t *projection,const size_t width)
{
  size_t
    max_projection;

  ssize_t
    i,
    skew;

  max_projection=0;
  skew=0;
  for (i=0; i < (ssize_t) (2*width-1); i++)
  {
    if (projection[i] > max_projection)
      {
        skew=i-(ssize_t) width+1;
        max_projection=projection[i];
      }
  }
  return(skew);
}

static Image *AutoCropDeskewImage(const Image *image,Image *clone_image,
  AffineMatrix affine_matrix,ExceptionInfo *exception)
{
  Image
    *crop_image,
    *deskew_image,
    *median_image;

  RectangleInfo
    geometry;

  deskew_image=AffineTransformImage(clone_image,&affine_matrix,exception);
  clone_image=DestroyImage(clone_image);
  if (deskew_image == (Image *) NULL)
    return((Image *) NULL);
  median_image=StatisticImage(deskew_image,MedianStatistic,3,3,exception);
  if (median_image == (Image *) NULL)
    {
      deskew_image=DestroyImage(deskew_image);
      return((Image *) NULL);
    }
  geometry=GetImageBoundingBox(median_image,exception);
  median_image=DestroyImage(median_image);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TransformEvent,GetMagickModule(),"  Deskew geometry: "
      "%.17gx%.17g%+.20g%+.20g",(double) geometry.width,(double)
      geometry.height,(double) geometry.x,(double) geometry.y);
  crop_image=CropImage(deskew_image,&geometry,exception);
  deskew_image=DestroyImage(deskew_image);
  return(crop_image);
}

MagickExport Image *DeskewImage(const Image *image,const double threshold,
  ExceptionInfo *exception)
{
  AffineMatrix
    affine_matrix;

  const char
    *artifact;

  double
    degrees;

  Image
    *clone_image,
    *deskew_image;

  MagickBooleanType
    status;

  size_t
    *projection,
    width;

  ssize_t
    skew;

  /*
    Compute deskew angle.
  */
  for (width=1; width < ((image->columns+7)/8); width<<=1) ;
  projection=(size_t *) AcquireQuantumMemory((size_t) (2*width-1),
    sizeof(*projection));
  if (projection == (size_t *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  status=RadonTransform(image,threshold,projection,exception);
  if (status == MagickFalse)
    {
      projection=(size_t *) RelinquishMagickMemory(projection);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  skew=FindDeskewSkew(projection,width);
  projection=(size_t *) RelinquishMagickMemory(projection);
  degrees=RadiansToDegrees(-atan((double) skew/width/8));
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TransformEvent,GetMagickModule(),
      "  Deskew angle: %g",degrees);
  /*
    Deskew image.
  */
  clone_image=CloneImage(image,0,0,MagickTrue,exception);
  if (clone_image == (Image *) NULL)
    return((Image *) NULL);
  {
    char
      angle[MagickPathExtent];

    (void) FormatLocaleString(angle,MagickPathExtent,"%.17g",degrees);
    (void) SetImageArtifact(clone_image,"deskew:angle",angle);
  }
  (void) SetImageVirtualPixelMethod(clone_image,BackgroundVirtualPixelMethod,
    exception);
  affine_matrix.sx=cos(DegreesToRadians(fmod((double) degrees,360.0)));
  affine_matrix.rx=sin(DegreesToRadians(fmod((double) degrees,360.0)));
  affine_matrix.ry=(-sin(DegreesToRadians(fmod((double) degrees,360.0))));
  affine_matrix.sy=cos(DegreesToRadians(fmod((double) degrees,360.0)));
  affine_matrix.tx=0.0;
  affine_matrix.ty=0.0;
  artifact=GetImageArtifact(image,"deskew:auto-crop");
  if (IsStringTrue(artifact) == MagickFalse)
    {
      deskew_image=AffineTransformImage(clone_image,&affine_matrix,exception);
      clone_image=DestroyImage(clone_image);
      return(deskew_image);
    }
  /*
    Auto-crop image.
  */
  GetImageBackgroundColor(clone_image,(ssize_t) StringToLong(artifact),
    exception);
  return(AutoCropDeskewImage(image,clone_image,affine_matrix,exception));
}
