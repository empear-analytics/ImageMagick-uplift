/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  V   V  IIIII  SSSSS  IIIII   OOO   N   N                   %
%                  V   V    I    SS       I    O   O  NN  N                   %
%                  V   V    I     SSS     I    O   O  N N N                   %
%                   V V     I       SS    I    O   O  N  NN                   %
%                    V    IIIII  SSSSS  IIIII   OOO   N   N                   %
%                                                                             %
%     CCCC   OOO   M   M  PPPP    OOO   N   N  EEEEE  N   N  TTTTT  SSSSS     %
%    C      O   O  MM MM  P   P  O   O  NN  N  E      NN  N    T    SS        %
%    C      O   O  M M M  PPPP   O   O  N N N  EEE    N N N    T     SSS      %
%    C      O   O  M   M  P      O   O  N  NN  E      N  NN    T       SS     %
%     CCCC   OOO   M   M  P       OOO   N   N  EEEEE  N   N    T    SSSSS     %
%                                                                             %
%                                                                             %
%                MagickCore Image Connected Components Method                 %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                               September 2014                                %
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
*/

#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/effect.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/matrix.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/morphology.h"
#include "MagickCore/morphology-private.h"
#include "MagickCore/opencl-private.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/resource_.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/vision.h"

/*
  Typedef declarations.
*/
typedef struct _CCThresholdRange
{
  double
    min_threshold,
    max_threshold;

  ssize_t
    background_id;
} CCThresholdRange;

typedef struct _CCNeighborDirection
{
  ssize_t
    dx,
    dy;
} CCNeighborDirection;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o n n e c t e d C o m p o n e n t s I m a g e                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConnectedComponentsImage() returns the connected-components of the image
%  uniquely labeled.  The returned connected components image colors member
%  defines the number of unique objects.  Choose from 4 or 8-way connectivity.
%
%  You are responsible for freeing the connected components objects resources
%  with this statement;
%
%    objects = (CCObjectInfo *) RelinquishMagickMemory(objects);
%
%  The format of the ConnectedComponentsImage method is:
%
%      Image *ConnectedComponentsImage(const Image *image,
%        const size_t connectivity,CCObjectInfo **objects,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o connectivity: how many neighbors to visit, choose from 4 or 8.
%
%    o objects: return the attributes of each unique object.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static int CCObjectInfoCompareIncreasing(CCObjectInfo *p,CCObjectInfo *q)
{
  if (p->key == 1)
    return((int) (p->area-(ssize_t) q->area));
  if (p->key == 2)
    return((int) (p->bounding_box.width-q->bounding_box.width));
  if (p->key == 3)
    return((int) (p->bounding_box.height-q->bounding_box.height));
  if (p->key == 4)
    return((int) (p->bounding_box.x-(ssize_t) q->bounding_box.x));
  if (p->key == 5)
    return((int) (p->bounding_box.y-(ssize_t) q->bounding_box.y));
  return((int) (q->area-(ssize_t) p->area));
}


static int CCObjectInfoCompare(const void *x,const void *y)
{
  CCObjectInfo
    *p,
    *q;

  p=(CCObjectInfo *) x;
  q=(CCObjectInfo *) y;
  if (p->key == -5)
    return((int) (q->bounding_box.y-(ssize_t) p->bounding_box.y));
  if (p->key == -4)
    return((int) (q->bounding_box.x-(ssize_t) p->bounding_box.x));
  if (p->key == -3)
    return((int) (q->bounding_box.height-p->bounding_box.height));
  if (p->key == -2)
    return((int) (q->bounding_box.width-p->bounding_box.width));
  if (p->key == -1)
    return((int) (q->area-(ssize_t) p->area));
  return(CCObjectInfoCompareIncreasing(p,q));
}

static inline MagickBooleanType IsDiagonalPair(const Quantum *pixels,
  const ssize_t i)
{
  return(((((ssize_t) pixels[0] == i) && ((ssize_t) pixels[3] == i)) ||
    (((ssize_t) pixels[1] == i) && ((ssize_t) pixels[2] == i))) ? MagickTrue :
    MagickFalse);
}

static void AddPerimeterPattern(const Quantum *pixels,const size_t foreground,
  const ssize_t i,size_t *pattern)
{
  if (foreground == 1)
    pattern[1]++;
  else
    if (foreground == 2)
      {
        if (IsDiagonalPair(pixels,i) != MagickFalse)
          pattern[0]++;  /* diagonal */
        else
          pattern[2]++;
      }
    else
      if (foreground == 3)
        pattern[3]++;
}


typedef struct _CCPerimeterPass
{
  const Image
    *component_image;

  size_t
    width;

  ssize_t
    i;
} CCPerimeterPass;

static void CountPerimeterPattern(const CCPerimeterPass *perimeter_pass,
  const Quantum *magick_restrict p,size_t *pattern)
{
  const Image
    *const component_image = perimeter_pass->component_image;

  const size_t
    width = perimeter_pass->width;

  const ssize_t
    i = perimeter_pass->i;

  Quantum
    pixels[4];

  size_t
    foreground;

  ssize_t
    v;

  /*
    An Algorithm for Calculating Objects’ Shape Features in Binary
    Images, Lifeng He, Yuyan Chao.
  */
  foreground=0;
  for (v=0; v < 2; v++)
  {
    ssize_t
      u;

    for (u=0; u < 2; u++)
    {
      ssize_t
        offset;

      offset=v*((ssize_t) width+2)*
        (ssize_t) GetPixelChannels(component_image)+u*
        (ssize_t) GetPixelChannels(component_image);
      pixels[2*v+u]=GetPixelIndex(component_image,p+offset);
      if ((ssize_t) pixels[2*v+u] == i)
        foreground++;
    }
  }
  AddPerimeterPattern(pixels,foreground,i,pattern);
}

typedef struct _CCObjectPerimeter
{
  const Image
    *component_image;

  CacheView
    *component_view;

  RectangleInfo
    bounding_box;

  ssize_t
    i;

  ExceptionInfo
    *exception;
} CCObjectPerimeter;

static MagickBooleanType CountObjectPerimeterPatterns(
  const CCObjectPerimeter *object_perimeter,size_t *pattern)
{
  const Image
    *const component_image = object_perimeter->component_image;

  CacheView
    *const component_view = object_perimeter->component_view;

  const RectangleInfo
    bounding_box = object_perimeter->bounding_box;

  const ssize_t
    i = object_perimeter->i;

  ExceptionInfo
    *const exception = object_perimeter->exception;

  MagickBooleanType
    status;

  ssize_t
    y;

  status=MagickTrue;
  for (y=(-1); y < (ssize_t) bounding_box.height; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    p=GetCacheViewVirtualPixels(component_view,bounding_box.x-1,
      bounding_box.y+y,bounding_box.width+2,2,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        break;
      }
    for (x=(-1); x < (ssize_t) bounding_box.width; x++)
    {
      CCPerimeterPass
        perimeter_pass;

      perimeter_pass.component_image=component_image;
      perimeter_pass.width=bounding_box.width;
      perimeter_pass.i=i;
      CountPerimeterPattern(&perimeter_pass,p,pattern);
      p+=(ptrdiff_t) GetPixelChannels(component_image);
    }
  }
  return(status);
}

static void PerimeterThreshold(const Image *component_image,
  CCObjectInfo *object,const ssize_t metric_index,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  ssize_t
    i;

  status=MagickTrue;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic) shared(status) \
    magick_number_threads(component_image,component_image,component_image->colors,1)
#endif
  for (i=0; i < (ssize_t) component_image->colors; i++)
  {
    CacheView
      *component_view;

    CCObjectPerimeter
      object_perimeter;

    RectangleInfo
      bounding_box;

    size_t
      pattern[4] = { 1, 0, 0, 0 };

    /*
      Compute perimeter of each object.
    */
    if (status == MagickFalse)
      continue;
    component_view=AcquireAuthenticCacheView(component_image,exception);
    bounding_box=object[i].bounding_box;
    object_perimeter.component_image=component_image;
    object_perimeter.component_view=component_view;
    object_perimeter.bounding_box=bounding_box;
    object_perimeter.i=i;
    object_perimeter.exception=exception;
    if (CountObjectPerimeterPatterns(&object_perimeter,pattern) == MagickFalse)
      status=MagickFalse;
    component_view=DestroyCacheView(component_view);
    object[i].metric[metric_index]=ceil(MagickSQ1_2*pattern[1]+1.0*pattern[2]+
      MagickSQ1_2*pattern[3]+MagickSQ2*pattern[0]-0.5);
  }
}

typedef struct _CCCircularityPass
{
  const Image
    *component_image;

  CCObjectInfo
    *object;

  ssize_t
    metric_index,
    i;

  ExceptionInfo
    *exception;
} CCCircularityPass;

static void MeasureObjectCircularity(const CCCircularityPass *circularity_pass,
  MagickBooleanType *status)
{
  const Image
    *const component_image = circularity_pass->component_image;

  CCObjectInfo
    *const object = circularity_pass->object;

  const ssize_t
    metric_index = circularity_pass->metric_index,
    i = circularity_pass->i;

  ExceptionInfo
    *const exception = circularity_pass->exception;

  CacheView
    *component_view;

  RectangleInfo
    bounding_box;

  size_t
    pattern[4] = { 1, 0, 0, 0 };

  ssize_t
    y;

  component_view=AcquireAuthenticCacheView(component_image,exception);
  bounding_box=object[i].bounding_box;
  for (y=(-1); y < (ssize_t) bounding_box.height; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    p=GetCacheViewVirtualPixels(component_view,bounding_box.x-1,
      bounding_box.y+y,bounding_box.width+2,2,exception);
    if (p == (const Quantum *) NULL)
      {
        (*status)=MagickFalse;
        break;
      }
    for (x=(-1); x < (ssize_t) bounding_box.width; x++)
    {
      CCPerimeterPass
        perimeter_pass;

      perimeter_pass.component_image=component_image;
      perimeter_pass.width=bounding_box.width;
      perimeter_pass.i=i;
      CountPerimeterPattern(&perimeter_pass,p,pattern);
      p+=(ptrdiff_t) GetPixelChannels(component_image);
    }
  }
  component_view=DestroyCacheView(component_view);
  object[i].metric[metric_index]=ceil(MagickSQ1_2*pattern[1]+1.0*pattern[2]+
    MagickSQ1_2*pattern[3]+MagickSQ2*pattern[0]-0.5);
  object[i].metric[metric_index]=4.0*MagickPI*object[i].area/
    (object[i].metric[metric_index]*object[i].metric[metric_index]);
}

static void CircularityThreshold(const Image *component_image,
  CCObjectInfo *object,const ssize_t metric_index,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  ssize_t
    i;

  status=MagickTrue;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic) shared(status) \
    magick_number_threads(component_image,component_image,component_image->colors,1)
#endif
  for (i=0; i < (ssize_t) component_image->colors; i++)
  {
    const CCCircularityPass
      circularity_pass = { component_image, object, metric_index, i,
        exception };

    /*
      Compute perimeter of each object.
    */
    if (status == MagickFalse)
      continue;
    MeasureObjectCircularity(&circularity_pass,&status);
  }
}

typedef struct _CCCentroidPass
{
  const Image
    *component_image;

  CacheView
    *component_view;

  RectangleInfo
    bounding_box;

  ssize_t
    i;

  ExceptionInfo
    *exception;
} CCCentroidPass;

typedef struct _CCCentroidTargets
{
  MagickBooleanType
    *status;

  double
    *M00,
    *M10,
    *M01;
} CCCentroidTargets;

static MagickBooleanType AccumulateCentroidRow(
  const CCCentroidPass *centroid_pass,const CCCentroidTargets *centroid_targets,
  const ssize_t y)
{
  const Image
    *const component_image = centroid_pass->component_image;

  CacheView
    *const component_view = centroid_pass->component_view;

  const RectangleInfo
    bounding_box = centroid_pass->bounding_box;

  const ssize_t
    i = centroid_pass->i;

  ExceptionInfo
    *const exception = centroid_pass->exception;

  MagickBooleanType
    *const status = centroid_targets->status;

  double
    *const M00 = centroid_targets->M00,
    *const M10 = centroid_targets->M10,
    *const M01 = centroid_targets->M01;

  const Quantum
    *magick_restrict p;

  ssize_t
    x;

  p=GetCacheViewVirtualPixels(component_view,bounding_box.x,
    bounding_box.y+y,bounding_box.width,1,exception);
  if (p == (const Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  for (x=0; x < (ssize_t) bounding_box.width; x++)
  {
    if ((ssize_t) GetPixelIndex(component_image,p) == i)
      {
        (*M00)++;
        *M10+=x;
        *M01+=y;
      }
    p+=(ptrdiff_t) GetPixelChannels(component_image);
  }
  return(MagickTrue);
}

static MagickBooleanType GetObjectCentroidSums(
  const CCCentroidPass *centroid_pass,double *M00,double *M10,double *M01)
{
  const RectangleInfo
    bounding_box = centroid_pass->bounding_box;

  MagickBooleanType
    status;

  const CCCentroidTargets
    centroid_targets = { &status, M00, M10, M01 };

  ssize_t
    y;

  status=MagickTrue;
  for (y=0; y < (ssize_t) bounding_box.height; y++)
    if (AccumulateCentroidRow(centroid_pass,&centroid_targets,y) == MagickFalse)
      break;
  return(status);
}

typedef struct _CCMomentRow
{
  const Image
    *component_image;

  RectangleInfo
    bounding_box;

  ssize_t
    i;

  PointInfo
    centroid;

  ssize_t
    y;
} CCMomentRow;

typedef struct _CCMomentSums
{
  double
    *M11,
    *M20,
    *M02;

  const Quantum
    *magick_restrict *p;
} CCMomentSums;

static void AccumulateSecondMoments(const CCMomentRow *moment_row,
  const CCMomentSums *moment_sums)
{
  double
    *const M11 = moment_sums->M11,
    *const M20 = moment_sums->M20,
    *const M02 = moment_sums->M02;

  const Quantum
    *magick_restrict *const p = moment_sums->p;

  ssize_t
    x;

  for (x=0; x < (ssize_t) moment_row->bounding_box.width; x++)
  {
    if ((ssize_t) GetPixelIndex(moment_row->component_image,*p) ==
        moment_row->i)
      {
        *M11+=(x-moment_row->centroid.x)*(moment_row->y-moment_row->centroid.y);
        *M20+=(x-moment_row->centroid.x)*(x-moment_row->centroid.x);
        *M02+=(moment_row->y-moment_row->centroid.y)*
          (moment_row->y-moment_row->centroid.y);
      }
    *p+=(ptrdiff_t) GetPixelChannels(moment_row->component_image);
  }
}

typedef struct _CCMomentPass
{
  const Image
    *component_image;

  ExceptionInfo
    *exception;

  CCObjectInfo
    *object;

  ssize_t
    i;
} CCMomentPass;

typedef struct _CCMomentTargets
{
  double
    *M00,
    *M11,
    *M20,
    *M02;
} CCMomentTargets;

static void ComputeObjectMoments(const CCMomentPass *moment_pass,
  const CCMomentTargets *targets,MagickBooleanType *status)
{
  const Image
    *const component_image = moment_pass->component_image;

  ExceptionInfo
    *const exception = moment_pass->exception;

  CCObjectInfo
    *const object = moment_pass->object;

  const ssize_t
    i = moment_pass->i;

  double
    *const M00 = targets->M00,
    *const M11 = targets->M11,
    *const M20 = targets->M20,
    *const M02 = targets->M02;

  CacheView
    *component_view;

  CCCentroidPass
    centroid_pass;

  double
    M01 = 0.0,
    M10 = 0.0;

  PointInfo
    centroid = { 0.0, 0.0 };

  RectangleInfo
    bounding_box;

  const Quantum
    *magick_restrict p;

  ssize_t
    y;

  component_view=AcquireAuthenticCacheView(component_image,exception);
  bounding_box=object[i].bounding_box;
  centroid_pass.component_image=component_image;
  centroid_pass.component_view=component_view;
  centroid_pass.bounding_box=bounding_box;
  centroid_pass.i=i;
  centroid_pass.exception=exception;
  if (GetObjectCentroidSums(&centroid_pass,M00,&M10,&M01) == MagickFalse)
    (*status)=MagickFalse;
  centroid.x=M10*MagickSafeReciprocal((*M00));
  centroid.y=M01*MagickSafeReciprocal((*M00));
  for (y=0; y < (ssize_t) bounding_box.height; y++)
  {
    CCMomentRow
      moment_row;

    const CCMomentSums
      moment_sums = { M11, M20, M02, &p };

    if ((*status) == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(component_view,bounding_box.x,
      bounding_box.y+y,bounding_box.width,1,exception);
    if (p == (const Quantum *) NULL)
      {
        (*status)=MagickFalse;
        break;
      }
    moment_row.component_image=component_image;
    moment_row.bounding_box=bounding_box;
    moment_row.i=i;
    moment_row.centroid=centroid;
    moment_row.y=y;
    AccumulateSecondMoments(&moment_row,&moment_sums);
  }
  component_view=DestroyCacheView(component_view);
}

static void MajorAxisThreshold(const Image *component_image,
  CCObjectInfo *object,const ssize_t metric_index,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  ssize_t
    i;

  status=MagickTrue;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic) shared(status) \
    magick_number_threads(component_image,component_image,component_image->colors,1)
#endif
  for (i=0; i < (ssize_t) component_image->colors; i++)
  {
    CCMomentPass
      moment_pass;

    double
      M00 = 0.0,
      M02 = 0.0,
      M11 = 0.0,
      M20 = 0.0;

    const CCMomentTargets
      targets = { &M00, &M11, &M20, &M02 };

    /*
      Compute ellipse major axis of each object.
    */
    if (status == MagickFalse)
      continue;
    moment_pass.component_image=component_image;
    moment_pass.exception=exception;
    moment_pass.object=object;
    moment_pass.i=i;
    ComputeObjectMoments(&moment_pass,&targets,&status);
    object[i].metric[metric_index]=sqrt((2.0*MagickSafeReciprocal(M00))*
      ((M20+M02)+sqrt(4.0*M11*M11+(M20-M02)*(M20-M02))));
  }
}

static void MinorAxisThreshold(const Image *component_image,
  CCObjectInfo *object,const ssize_t metric_index,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  ssize_t
    i;

  status=MagickTrue;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic) shared(status) \
    magick_number_threads(component_image,component_image,component_image->colors,1)
#endif
  for (i=0; i < (ssize_t) component_image->colors; i++)
  {
    CCMomentPass
      moment_pass;

    double
      M00 = 0.0,
      M02 = 0.0,
      M11 = 0.0,
      M20 = 0.0;

    const CCMomentTargets
      targets = { &M00, &M11, &M20, &M02 };

    /*
      Compute ellipse major axis of each object.
    */
    if (status == MagickFalse)
      continue;
    moment_pass.component_image=component_image;
    moment_pass.exception=exception;
    moment_pass.object=object;
    moment_pass.i=i;
    ComputeObjectMoments(&moment_pass,&targets,&status);
    object[i].metric[metric_index]=sqrt((2.0*MagickSafeReciprocal(M00))*
      ((M20+M02)-sqrt(4.0*M11*M11+(M20-M02)*(M20-M02))));
  }
}

static PointInfo GetEllipseAxes(const double M00,const double M11,
  const double M20,const double M02)
{
  PointInfo
    ellipse_axis = { 0.0, 0.0 };

  ellipse_axis.x=sqrt((2.0*MagickSafeReciprocal(M00))*((M20+M02)+
    sqrt(4.0*M11*M11+(M20-M02)*(M20-M02))));
  ellipse_axis.y=sqrt((2.0*MagickSafeReciprocal(M00))*((M20+M02)-
    sqrt(4.0*M11*M11+(M20-M02)*(M20-M02))));
  return(ellipse_axis);
}

static void EccentricityThreshold(const Image *component_image,
  CCObjectInfo *object,const ssize_t metric_index,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  ssize_t
    i;

  status=MagickTrue;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic) shared(status) \
    magick_number_threads(component_image,component_image,component_image->colors,1)
#endif
  for (i=0; i < (ssize_t) component_image->colors; i++)
  {
    CCMomentPass
      moment_pass;

    double
      M00 = 0.0,
      M02 = 0.0,
      M11 = 0.0,
      M20 = 0.0;

    const CCMomentTargets
      targets = { &M00, &M11, &M20, &M02 };

    PointInfo
      ellipse_axis = { 0.0, 0.0 };

    /*
      Compute eccentricity of each object.
    */
    if (status == MagickFalse)
      continue;
    moment_pass.component_image=component_image;
    moment_pass.exception=exception;
    moment_pass.object=object;
    moment_pass.i=i;
    ComputeObjectMoments(&moment_pass,&targets,&status);
    ellipse_axis=GetEllipseAxes(M00,M11,M20,M02);
    object[i].metric[metric_index]=sqrt(1.0-(ellipse_axis.y*ellipse_axis.y*
      MagickSafeReciprocal(ellipse_axis.x*ellipse_axis.x)));
  }
}

static inline MagickBooleanType IsNegativeAxisDifference(const double M20,
  const double M02)
{
  return(((fabs(M20-M02) >= 0.0) && ((M20-M02) < 0.0)) ? MagickTrue :
    MagickFalse);
}

typedef struct _CCEllipseAngle
{
  double
    M11,
    M20,
    M02;

  ssize_t
    i,
    metric_index;
} CCEllipseAngle;

static void AdjustEllipseAngle(CCObjectInfo *object,
  const CCEllipseAngle *ellipse_angle)
{
  const double
    M11 = ellipse_angle->M11,
    M20 = ellipse_angle->M20,
    M02 = ellipse_angle->M02;

  const ssize_t
    i = ellipse_angle->i,
    metric_index = ellipse_angle->metric_index;

  if (M11 < 0.0)
    {
      if (fabs(M20-M02) >= 0.0)
        {
          if ((M20-M02) < 0.0)
            object[i].metric[metric_index]+=90.0;
          else
            object[i].metric[metric_index]+=180.0;
        }
    }
  else
    if (IsNegativeAxisDifference(M20,M02) != MagickFalse)
      object[i].metric[metric_index]+=90.0;
}


static void SetEllipseAngleMetric(CCObjectInfo *object,
  const CCEllipseAngle *angle_moments)
{
  CCEllipseAngle
    ellipse_angle;

  object[angle_moments->i].metric[angle_moments->metric_index]=
    RadiansToDegrees(1.0/2.0*atan(2.0*angle_moments->M11*
    MagickSafeReciprocal(angle_moments->M20-angle_moments->M02)));
  if (fabs(angle_moments->M11) < 0.0)
      {
        if (IsNegativeAxisDifference(angle_moments->M20,angle_moments->M02) !=
            MagickFalse)
          object[angle_moments->i].metric[angle_moments->metric_index]+=90.0;
      }
    else
      {
        ellipse_angle.M11=angle_moments->M11;
        ellipse_angle.M20=angle_moments->M20;
        ellipse_angle.M02=angle_moments->M02;
        ellipse_angle.i=angle_moments->i;
        ellipse_angle.metric_index=angle_moments->metric_index;
        AdjustEllipseAngle(object,&ellipse_angle);
      }
}

typedef struct _ComputeObjectAngleInfo
{
  const Image
    *component_image;

  ExceptionInfo
    *exception;

  CCObjectInfo
    *object;

  ssize_t
    metric_index;
} ComputeObjectAngleInfo;

static void ComputeObjectAngle(MagickBooleanType *status,
  const ComputeObjectAngleInfo *info,const ssize_t i)
{
  const Image
    *const component_image = info->component_image;

  ExceptionInfo
    *const exception = info->exception;

  CCObjectInfo
    *const object = info->object;

  const ssize_t
    metric_index = info->metric_index;

  CCEllipseAngle
    ellipse_angle;

  CCMomentPass
    moment_pass;

  double
    M00 = 0.0,
    M02 = 0.0,
    M11 = 0.0,
    M20 = 0.0;

  const CCMomentTargets
    targets = { &M00, &M11, &M20, &M02 };

  /*
    Compute ellipse angle of each object.
  */
  if ((*status) == MagickFalse)
    return;
  moment_pass.component_image=component_image;
  moment_pass.exception=exception;
  moment_pass.object=object;
  moment_pass.i=i;
  ComputeObjectMoments(&moment_pass,&targets,status);
  ellipse_angle.M11=M11;
  ellipse_angle.M20=M20;
  ellipse_angle.M02=M02;
  ellipse_angle.i=i;
  ellipse_angle.metric_index=metric_index;
  SetEllipseAngleMetric(object,&ellipse_angle);
}

static void AngleThreshold(const Image *component_image,
  CCObjectInfo *object,const ssize_t metric_index,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  ssize_t
    i;

  status=MagickTrue;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic) shared(status) \
    magick_number_threads(component_image,component_image,component_image->colors,1)
#endif
  for (i=0; i < (ssize_t) component_image->colors; i++)
  {
    ComputeObjectAngleInfo
      info;

    info.component_image=component_image;
    info.exception=exception;
    info.object=object;
    info.metric_index=metric_index;
    ComputeObjectAngle(&status,&info,i);
  }
}

static inline MagickBooleanType IsMetricOutsideRange(const CCObjectInfo *object,
  const ssize_t i,const ssize_t n,const CCThresholdRange *range)
{
  return((((object[i].metric[n] < range->min_threshold) ||
    (object[i].metric[n] >= range->max_threshold)) &&
    (i != range->background_id)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsNeighborOutsideImage(const Image *image,
  const ssize_t x,const ssize_t y,const CCNeighborDirection *direction)
{
  const ssize_t
    dx = direction->dx,
    dy = direction->dy;

  return((((x+dx) < 0) || ((x+dx) >= (ssize_t) image->columns) ||
    ((y+dy) < 0) || ((y+dy) >= (ssize_t) image->rows)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsAreaOutsideRange(const CCObjectInfo *object,
  const ssize_t i,const CCThresholdRange *range)
{
  return((((object[i].area < range->min_threshold) ||
    (object[i].area >= range->max_threshold)) &&
    (i != range->background_id)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsGreaterAdjacentObject(
  const CCObjectInfo *object,const size_t *neighbors,const ssize_t j,
  const size_t id)
{
  return(((object[neighbors[j]].census > object[id].census) ||
    ((object[neighbors[j]].census == object[id].census) &&
    (neighbors[j] < id))) ? MagickTrue : MagickFalse);
}

static void MergeObjectsOutsideRange(CCObjectInfo *object,const size_t colors,
  const ssize_t n,const CCThresholdRange *range)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) colors; i++)
    if (IsMetricOutsideRange(object,i,n,range) != MagickFalse)
      object[i].merge=MagickTrue;
}

static void SetObjectMergeByColor(CCObjectInfo *object,const size_t colors,
  const PixelInfo *pixel,const MagickBooleanType merge)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) colors; i++)
    if (IsFuzzyEquivalencePixelInfo(&object[i].color,pixel) != MagickFalse)
      object[i].merge=merge;
}

static inline ssize_t GetObjectIndex(const ssize_t id,const size_t colors)
{
  if (id < 0)
    return(id+(ssize_t) colors);
  return(id);
}

static MagickBooleanType GetNextObjectIdRange(char **cursor,
  const size_t colors,ssize_t *first,ssize_t *last)
{
  char
    *c,
    *d;

  c=(*cursor);
  while ((isspace((int) ((unsigned char) *c)) != 0) || (*c == ','))
    c++;
  d=c;
  *first=(ssize_t) strtol(c,&d,10);
  if (d == c)
    return(MagickFalse);
  c=d;
  *first=GetObjectIndex(*first,colors);
  *last=(*first);
  while (isspace((int) ((unsigned char) *c)) != 0)
    c++;
  if (*c == '-')
    *last=GetObjectIndex((ssize_t) strtol(c+1,&c,10),colors);
  *cursor=c;
  return(MagickTrue);
}

static void SetObjectMergeByIds(CCObjectInfo *object,const size_t colors,
  const char *artifact,const MagickBooleanType merge)
{
  char
    *c;

  ssize_t
    first,
    last,
    step;

  c=(char *) artifact;
  while ((*c != '\0') &&
         (GetNextObjectIdRange(&c,colors,&first,&last) != MagickFalse))
  {
    step=(ssize_t) (first > last ? -1 : 1);
    for ( ; first != (last+step); first+=step)
      if ((first >= 0) &&
          (first < (ssize_t) colors))
        object[first].merge=merge;
  }
}

static MagickBooleanType SetEquivalencePathRoot(MatrixInfo *equivalences,
  const ssize_t ox,const ssize_t root)
{
  MagickBooleanType
    status;

  ssize_t
    obj;

  status=GetMatrixElement(equivalences,ox,0,&obj);
  while (obj != root)
  {
    status=GetMatrixElement(equivalences,ox,0,&obj);
    status=SetMatrixElement(equivalences,ox,0,&root);
  }
  return(status);
}

static ssize_t GetObjectSortKey(const Image *image)
{
  const char
    *artifact;

  ssize_t
    key;

  key=0;
  artifact=GetImageArtifact(image,"connected-components:sort");
  if (artifact != (const char *) NULL)
    {
      if (LocaleCompare(artifact,"area") == 0)
        key=1;
      if (LocaleCompare(artifact,"width") == 0)
        key=2;
      if (LocaleCompare(artifact,"height") == 0)
        key=3;
      if (LocaleCompare(artifact,"x") == 0)
        key=4;
      if (LocaleCompare(artifact,"y") == 0)
        key=5;
    }
  return(key);
}

static ssize_t GetObjectSortOrder(const Image *image)
{
  const char
    *artifact;

  ssize_t
    order;

  order=1;
  artifact=GetImageArtifact(image,"connected-components:sort-order");
  if (artifact != (const char *) NULL)
    if (LocaleCompare(artifact,"decreasing") == 0)
      order=(-1);
  return(order);
}

static size_t GetGreatestAdjacentObject(const CCObjectInfo *object,
  const size_t *neighbors,const size_t count)
{
  size_t
    id;

  ssize_t
    j;

  id=0;
  for (j=0; j < (ssize_t) count; j++)
    if (IsGreaterAdjacentObject(object,neighbors,j,id) != MagickFalse)
      id=neighbors[j];
  return(id);
}

static void ResetObjectStatistics(const Image *component_image,
  CCObjectInfo *object)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) component_image->colors; i++)
  {
    object[i].bounding_box.width=0;
    object[i].bounding_box.height=0;
    object[i].bounding_box.x=(ssize_t) component_image->columns;
    object[i].bounding_box.y=(ssize_t) component_image->rows;
    object[i].centroid.x=0;
    object[i].centroid.y=0;
    object[i].census=object[i].area == 0.0 ? 0.0 : 1.0;
    object[i].area=0;
  }
}

static void NormalizeObjectStatistics(const Image *component_image,
  CCObjectInfo *object)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) component_image->colors; i++)
  {
    object[i].bounding_box.width=(size_t) ((ssize_t)
      object[i].bounding_box.width-(object[i].bounding_box.x-1));
    object[i].bounding_box.height=(size_t) ((ssize_t)
      object[i].bounding_box.height-(object[i].bounding_box.y-1));
    object[i].centroid.x=object[i].centroid.x/object[i].area;
    object[i].centroid.y=object[i].centroid.y/object[i].area;
  }
}

static void PrintObjectsHeader(const Image *image,const char **metrics,
  const ssize_t n)
{
  const char
    *artifact;

  ssize_t
    j;

  artifact=GetImageArtifact(image,
    "connected-components:exclude-header");
  if (IsStringTrue(artifact) == MagickFalse)
    {
      (void) fprintf(stdout,"Objects (");
      artifact=GetImageArtifact(image,
        "connected-components:exclude-ids");
      if (IsStringTrue(artifact) == MagickFalse)
        (void) fprintf(stdout,"id: ");
      (void) fprintf(stdout,"bounding-box centroid area mean-color");
      for (j=0; j <= n; j++)
        (void) fprintf(stdout," %s",metrics[j]);
      (void) fprintf(stdout,"):\n");
    }
}

static void PrintObject(const Image *image,const CCObjectInfo *object,
  const ssize_t i,const ssize_t n)
{
  char
    mean_color[MagickPathExtent];

  const char
    *artifact;

  ssize_t
    j;

  GetColorTuple(&object[i].color,MagickFalse,mean_color);
  (void) fprintf(stdout,"  ");
  artifact=GetImageArtifact(image,
    "connected-components:exclude-ids");
  if (IsStringTrue(artifact) == MagickFalse)
    (void) fprintf(stdout,"%.17g: ",(double) object[i].id);
  (void) fprintf(stdout,
    "%.17gx%.17g%+.20g%+.20g %.1f,%.1f %.*g %s",(double)
    object[i].bounding_box.width,(double)
    object[i].bounding_box.height,(double)
    object[i].bounding_box.x,(double) object[i].bounding_box.y,
    object[i].centroid.x,object[i].centroid.y,
    GetMagickPrecision(),(double) object[i].area,mean_color);
  for (j=0; j <= n; j++)
    (void) fprintf(stdout," %.*g",GetMagickPrecision(),
      object[i].metric[j]);
  (void) fprintf(stdout,"\n");
}

static size_t AddAdjacentObject(CCObjectInfo *object,size_t *neighbors,
  const ssize_t j,size_t count)
{
  if (object[j].census == 0.0)
    neighbors[count++]=(size_t) j;
  object[j].census++;
  return(count);
}

static void AddObjectPixel(CCObjectInfo *object,const size_t id,
  const ssize_t x,const ssize_t y)
{
  if (x < object[id].bounding_box.x)
    object[id].bounding_box.x=x;
  if (x > (ssize_t) object[id].bounding_box.width)
    object[id].bounding_box.width=(size_t) x;
  if (y < object[id].bounding_box.y)
    object[id].bounding_box.y=y;
  if (y > (ssize_t) object[id].bounding_box.height)
    object[id].bounding_box.height=(size_t) y;
  object[id].centroid.x+=x;
  object[id].centroid.y+=y;
  object[id].area++;
}

typedef struct _CCObjectPixel
{
  const Image
    *image;

  const Quantum
    *p;

  ssize_t
    x,
    y,
    id;
} CCObjectPixel;

static void AddLabeledObjectPixel(CCObjectInfo *object,
  const CCObjectPixel *object_pixel)
{
  const Image
    *const image = object_pixel->image;

  const Quantum
    *const p = object_pixel->p;

  const ssize_t
    x = object_pixel->x,
    y = object_pixel->y,
    id = object_pixel->id;

  if (x < object[id].bounding_box.x)
    object[id].bounding_box.x=x;
  if (x >= (ssize_t) object[id].bounding_box.width)
    object[id].bounding_box.width=(size_t) x;
  if (y < object[id].bounding_box.y)
    object[id].bounding_box.y=y;
  if (y >= (ssize_t) object[id].bounding_box.height)
    object[id].bounding_box.height=(size_t) y;
  object[id].color.red+=QuantumScale*(double) GetPixelRed(image,p);
  object[id].color.green+=QuantumScale*(double) GetPixelGreen(image,p);
  object[id].color.blue+=QuantumScale*(double) GetPixelBlue(image,p);
  if (image->alpha_trait != UndefinedPixelTrait)
    object[id].color.alpha+=QuantumScale*(double) GetPixelAlpha(image,p);
  if (image->colorspace == CMYKColorspace)
    object[id].color.black+=QuantumScale*(double) GetPixelBlack(image,p);
  object[id].centroid.x+=x;
  object[id].centroid.y+=y;
  object[id].area++;
}

typedef struct _CCEquivalencePixel
{
  const Image
    *image;

  ssize_t
    x,
    y,
    dx,
    dy;
} CCEquivalencePixel;

static MagickBooleanType ResolveEquivalence(MatrixInfo *equivalences,
  const CCEquivalencePixel *equivalence_pixel)
{
  const Image
    *const image = equivalence_pixel->image;

  const ssize_t
    x = equivalence_pixel->x,
    y = equivalence_pixel->y,
    dx = equivalence_pixel->dx,
    dy = equivalence_pixel->dy;

  MagickBooleanType
    status;

  ssize_t
    neighbor_offset,
    obj,
    offset,
    ox,
    oy,
    root;

  offset=y*(ssize_t) image->columns+x;
  neighbor_offset=dy*(ssize_t) image->columns+dx;
  ox=offset;
  status=GetMatrixElement(equivalences,ox,0,&obj);
  while (obj != ox)
  {
    ox=obj;
    status=GetMatrixElement(equivalences,ox,0,&obj);
  }
  oy=offset+neighbor_offset;
  status=GetMatrixElement(equivalences,oy,0,&obj);
  while (obj != oy)
  {
    oy=obj;
    status=GetMatrixElement(equivalences,oy,0,&obj);
  }
  if (ox < oy)
    {
      status=SetMatrixElement(equivalences,oy,0,&ox);
      root=ox;
    }
  else
    {
      status=SetMatrixElement(equivalences,ox,0,&oy);
      root=oy;
    }
  ox=offset;
  status=SetEquivalencePathRoot(equivalences,ox,root);
  oy=offset+neighbor_offset;
  status=SetEquivalencePathRoot(equivalences,oy,root);
  status=SetMatrixElement(equivalences,y*(ssize_t) image->columns+x,0,
    &root);
  return(status);
}

typedef struct _CCObjectRelabel
{
  Image
    *component_image;

  ssize_t
    i;

  size_t
    id;
} CCObjectRelabel;

static void RelabelObjectRow(const CCObjectRelabel *object_relabel,
  const RectangleInfo bounding_box,Quantum *q)
{
  Image
    *const component_image = object_relabel->component_image;

  const ssize_t
    i = object_relabel->i;

  const size_t
    id = object_relabel->id;

  ssize_t
    x;

  for (x=0; x < (ssize_t) bounding_box.width; x++)
  {
    if ((ssize_t) GetPixelIndex(component_image,q) == i)
      SetPixelIndex(component_image,(Quantum) id,q);
    q+=(ptrdiff_t) GetPixelChannels(component_image);
  }
}

static void MergeObjectsBelowTop(const Image *component_image,
  CCObjectInfo *object,CCObjectInfo *top_objects,const ssize_t top_ids)
{
  ssize_t
    i;

  (void) memcpy(top_objects,object,component_image->colors*sizeof(*object));
  qsort((void *) top_objects,component_image->colors,sizeof(*top_objects),
    CCObjectInfoCompare);
  for (i=top_ids+1; i < (ssize_t) component_image->colors; i++)
  {
    ssize_t id = (ssize_t) top_objects[i].id;
    if ((id >= 0) && (id < (ssize_t) component_image->colors))
      object[id].merge=MagickTrue;
  }
}

static ssize_t GetTopObjectIds(const Image *component_image,
  const char *artifact)
{
  ssize_t
    top_ids;

  top_ids=(ssize_t) StringToLong(artifact);
  if (top_ids < 0)
    top_ids=0;
  if (top_ids >= (ssize_t) component_image->colors)
    top_ids=(ssize_t) component_image->colors-1;
  return(top_ids);
}

static void DiameterThreshold(const Image *component_image,
  CCObjectInfo *object,const ssize_t n,const CCThresholdRange range)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) component_image->colors; i++)
  {
    object[i].metric[n]=ceil(sqrt(4.0*object[i].area/MagickPI)-0.5);
    if (IsMetricOutsideRange(object,i,n,&range) != MagickFalse)
      object[i].merge=MagickTrue;
  }
}

static void NormalizeLabeledObject(const Image *image,CCObjectInfo *object,
  const ssize_t i)
{
  object[i].bounding_box.width=(size_t) ((ssize_t)
    object[i].bounding_box.width-(object[i].bounding_box.x-1));
  object[i].bounding_box.height=(size_t) ((ssize_t)
    object[i].bounding_box.height-(object[i].bounding_box.y-1));
  object[i].color.red/=(QuantumScale*object[i].area);
  object[i].color.green/=(QuantumScale*object[i].area);
  object[i].color.blue/=(QuantumScale*object[i].area);
  if (image->alpha_trait != UndefinedPixelTrait)
    object[i].color.alpha/=(QuantumScale*object[i].area);
  if (image->colorspace == CMYKColorspace)
    object[i].color.black/=(QuantumScale*object[i].area);
  object[i].centroid.x/=object[i].area;
  object[i].centroid.y/=object[i].area;
}

typedef struct _CCObjectStatistics
{
  Image
    *component_image;

  CacheView
    *component_view;

  ExceptionInfo
    *exception;
} CCObjectStatistics;

static MagickBooleanType AccumulateObjectStatistics(CCObjectInfo *object,
  const CCObjectStatistics *object_statistics,MagickBooleanType status)
{
  Image
    *const component_image = object_statistics->component_image;

  CacheView
    *const component_view = object_statistics->component_view;

  ExceptionInfo
    *const exception = object_statistics->exception;

  ssize_t
    y;

  for (y=0; y < (ssize_t) component_image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(component_view,0,y,component_image->columns,
      1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) component_image->columns; x++)
    {
      size_t
        id;

      id=(size_t) GetPixelIndex(component_image,p);
      AddObjectPixel(object,id,x,y);
      p+=(ptrdiff_t) GetPixelChannels(component_image);
    }
  }
  return(status);
}

static void InitializeObjects(const Image *image,CCObjectInfo *object)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) MaxColormapSize; i++)
  {
    object[i].id=i;
    object[i].bounding_box.x=(ssize_t) image->columns;
    object[i].bounding_box.y=(ssize_t) image->rows;
    GetPixelInfo(image,&object[i].color);
  }
}

typedef struct _CCEquivalenceRow
{
  const Image
    *image;

  MatrixInfo
    *equivalences;

  ssize_t
    y,
    dx,
    dy;
} CCEquivalenceRow;

static MagickBooleanType ResolveRowEquivalences(
  const CCEquivalenceRow *equivalence_row,const CCNeighborDirection direction,
  const Quantum *p,MagickBooleanType status)
{
  const Image
    *const image = equivalence_row->image;

  MatrixInfo
    *const equivalences = equivalence_row->equivalences;

  const ssize_t
    y = equivalence_row->y,
    dx = equivalence_row->dx,
    dy = equivalence_row->dy;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    CCEquivalencePixel
      equivalence_pixel;

    PixelInfo
      pixel,
      target;

    ssize_t
      neighbor_offset;

    /*
      Is neighbor an authentic pixel and a different color than the pixel?
    */
    GetPixelInfoPixel(image,p,&pixel);
    if (IsNeighborOutsideImage(image,x,y,&direction) != MagickFalse)
      {
        p+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    neighbor_offset=dy*((ssize_t) GetPixelChannels(image)*(ssize_t)
      image->columns)+dx*(ssize_t) GetPixelChannels(image);
    GetPixelInfoPixel(image,p+neighbor_offset,&target);
    if (IsFuzzyEquivalencePixelInfo(&pixel,&target) == MagickFalse)
      {
        p+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    /*
      Resolve this equivalence.
    */
    equivalence_pixel.image=image;
    equivalence_pixel.x=x;
    equivalence_pixel.y=y;
    equivalence_pixel.dx=dx;
    equivalence_pixel.dy=dy;
    status=ResolveEquivalence(equivalences,&equivalence_pixel);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(status);
}

typedef struct _CCColorMerge
{
  const Image
    *component_image;

  CCObjectInfo
    *object;

  const char
    *artifact;

  MagickBooleanType
    merge;

  ExceptionInfo
    *exception;
} CCColorMerge;

static void MergeObjectsByColor(const CCColorMerge *color_merge)
{
  const Image
    *const component_image = color_merge->component_image;

  CCObjectInfo
    *const object = color_merge->object;

  const char
    *const artifact = color_merge->artifact;

  const MagickBooleanType
    merge = color_merge->merge;

  ExceptionInfo
    *const exception = color_merge->exception;

  const char
    *p;

  for (p=artifact;  ; )
  {
    char
      color[MagickPathExtent];

    PixelInfo
      pixel;

    const char
      *q;

    for (q=p; *q != '\0'; q++)
      if (*q == ';')
        break;
    (void) CopyMagickString(color,p,(size_t) MagickMin(q-p+1,
      MagickPathExtent));
    (void) QueryColorCompliance(color,AllCompliance,&pixel,exception);
    SetObjectMergeByColor(object,component_image->colors,&pixel,merge);
    if (*q == '\0')
      break;
    p=q+1;
  }
}

static void RemoveObjectsByColor(const Image *component_image,
  CCObjectInfo *object,const char *artifact,ExceptionInfo *exception)
{
  CCColorMerge
    color_merge;

  color_merge.component_image=component_image;
  color_merge.object=object;
  color_merge.artifact=artifact;
  color_merge.merge=MagickTrue;
  color_merge.exception=exception;
  MergeObjectsByColor(&color_merge);
}

static void AreaThreshold(const Image *component_image,CCObjectInfo *object,
  const CCThresholdRange range)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) component_image->colors; i++)
    if (IsAreaOutsideRange(object,i,&range) != MagickFalse)
      object[i].merge=MagickTrue;
}

static void PrintObjects(const Image *image,const Image *component_image,
  const CCObjectInfo *object,const ssize_t n)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) component_image->colors; i++)
    if (object[i].census > 0.0)
      PrintObject(image,object,i,n);
}

static void ResetAdjacentCensus(CCObjectInfo *object,const size_t *neighbors,
  const size_t count)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) count; j++)
    object[neighbors[j]].census=0.0;
}

static void MergeAllObjects(const Image *component_image,CCObjectInfo *object)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) component_image->colors; i++)
    object[i].merge=MagickTrue;
}

static void SetObjectMeanColors(Image *component_image,
  const CCObjectInfo *object)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) component_image->colors; i++)
    component_image->colormap[i]=object[i].color;
}

typedef struct _CCAdjacentCensus
{
  Image
    *component_image;

  CacheView
    *object_view;

  CCObjectInfo
    *object;

  size_t
    *neighbors;

  RectangleInfo
    bounding_box;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    x,
    y,
    i;

  ExceptionInfo
    *exception;
} CCAdjacentCensus;

static const Quantum *GetAdjacentObjectPixel(const CCAdjacentCensus *census,
  const ssize_t k)
{
  ssize_t
    dx,
    dy;

  dx=census->connectivity > 4 ? census->connect8[k][1] :
    census->connect4[k][1];
  dy=census->connectivity > 4 ? census->connect8[k][0] :
    census->connect4[k][0];
  return(GetCacheViewVirtualPixels(census->object_view,census->bounding_box.x+
    census->x+dx,census->bounding_box.y+census->y+dy,1,1,census->exception));
}

static void CountAdjacentObjects(const CCAdjacentCensus *census,
  MagickBooleanType *status,size_t *count)
{
  ssize_t
    j,
    k;

  for (k=0; k < (ssize_t) (census->connectivity > 4 ? 4 : 2); k++)
  {
    const Quantum
      *q;

    /*
      Compute area of adjacent objects.
    */
    if (*status == MagickFalse)
      continue;
    q=GetAdjacentObjectPixel(census,k);
    if (q == (const Quantum *) NULL)
      {
        *status=MagickFalse;
        break;
      }
    j=(ssize_t) GetPixelIndex(census->component_image,q);
    if (j != census->i)
      *count=AddAdjacentObject(census->object,census->neighbors,j,*count);
  }
}

typedef struct _CCLabelPixel
{
  const Image
    *image;

  MatrixInfo
    *equivalences;

  CCObjectInfo
    *object;

  Image
    *component_image;

  ssize_t
    x,
    y;
} CCLabelPixel;

typedef struct _CCLabelTargets
{
  MagickBooleanType
    *status;

  ssize_t
    *n;

  const Quantum
    *magick_restrict *p;

  Quantum
    *magick_restrict *q;
} CCLabelTargets;

static MagickBooleanType LabelObjectPixel(const CCLabelPixel *label_pixel,
  const CCLabelTargets *label_targets)
{
  MagickBooleanType
    *const status = label_targets->status;

  ssize_t
    *const n = label_targets->n;

  const Quantum
    *magick_restrict *const p = label_targets->p;

  Quantum
    *magick_restrict *const q = label_targets->q;

  CCObjectPixel
    object_pixel;

  ssize_t
    id,
    offset;

  offset=label_pixel->y*(ssize_t) label_pixel->image->columns+label_pixel->x;
  *status=GetMatrixElement(label_pixel->equivalences,offset,0,&id);
  if (id != offset)
    *status=GetMatrixElement(label_pixel->equivalences,id,0,&id);
  else
    {
      id=(*n)++;
      if (id >= (ssize_t) MaxColormapSize)
        return(MagickFalse);
    }
  *status=SetMatrixElement(label_pixel->equivalences,offset,0,&id);
  object_pixel.image=label_pixel->image;
  object_pixel.p=(*p);
  object_pixel.x=label_pixel->x;
  object_pixel.y=label_pixel->y;
  object_pixel.id=id;
  AddLabeledObjectPixel(label_pixel->object,&object_pixel);
  SetPixelIndex(label_pixel->component_image,(Quantum) id,*q);
  *p+=(ptrdiff_t) GetPixelChannels(label_pixel->image);
  *q+=(ptrdiff_t) GetPixelChannels(label_pixel->component_image);
  return(MagickTrue);
}

typedef struct _CCAdjacentRow
{
  Image
    *component_image;

  CacheView
    *object_view;

  CCObjectInfo
    *object;

  size_t
    *neighbors;

  RectangleInfo
    bounding_box;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    y,
    i;

  ExceptionInfo
    *exception;
} CCAdjacentRow;

typedef struct _CCAdjacentTargets
{
  MagickBooleanType
    *status;

  size_t
    *count;

  const Quantum
    *magick_restrict *p;
} CCAdjacentTargets;

static void CountAdjacentObjectsInRow(const CCAdjacentRow *adjacent_row,
  const CCAdjacentTargets *adjacent_targets)
{
  MagickBooleanType
    *const status = adjacent_targets->status;

  size_t
    *const count = adjacent_targets->count;

  const Quantum
    *magick_restrict *const p = adjacent_targets->p;

  ssize_t
    j,
    x;

  for (x=0; x < (ssize_t) adjacent_row->bounding_box.width; x++)
  {
    CCAdjacentCensus
      census;

    if (*status == MagickFalse)
      continue;
    j=(ssize_t) GetPixelIndex(adjacent_row->component_image,*p);
    if (j == adjacent_row->i)
      {
        census.component_image=adjacent_row->component_image;
        census.object_view=adjacent_row->object_view;
        census.object=adjacent_row->object;
        census.neighbors=adjacent_row->neighbors;
        census.bounding_box=adjacent_row->bounding_box;
        census.connectivity=adjacent_row->connectivity;
        census.connect4=adjacent_row->connect4;
        census.connect8=adjacent_row->connect8;
        census.x=x;
        census.y=adjacent_row->y;
        census.i=adjacent_row->i;
        census.exception=adjacent_row->exception;
        CountAdjacentObjects(&census,status,count);
      }
    *p+=(ptrdiff_t) GetPixelChannels(adjacent_row->component_image);
  }
}

static void SortObjects(const Image *image,const Image *component_image,
  CCObjectInfo *object)
{
  ssize_t
    i,
    key,
    order;

  order=GetObjectSortOrder(image);
  key=GetObjectSortKey(image);
  for (i=0; i < (ssize_t) component_image->colors; i++)
     object[i].key=order*key;
  qsort((void *) object,component_image->colors,sizeof(*object),
    CCObjectInfoCompare);
}

static MagickBooleanType ComputeObjectStatistics(Image *component_image,
  CCObjectInfo *object,MagickBooleanType status,ExceptionInfo *exception)
{
  CacheView
    *component_view;

  CCObjectStatistics
    object_statistics;

  ResetObjectStatistics(component_image,object);
  component_view=AcquireVirtualCacheView(component_image,exception);
  object_statistics.component_image=component_image;
  object_statistics.component_view=component_view;
  object_statistics.exception=exception;
  status=AccumulateObjectStatistics(object,&object_statistics,status);
  NormalizeObjectStatistics(component_image,object);
  component_view=DestroyCacheView(component_view);
  return(status);
}

static CCThresholdRange GetThresholdRange(const char *artifact,
  double *min_threshold,double *max_threshold,const ssize_t background_id)
{
  CCThresholdRange
    range;

  (void) MagickSscanf(artifact,"%lf%*[ -]%lf",min_threshold,max_threshold);
  range.min_threshold=(*min_threshold);
  range.max_threshold=(*max_threshold);
  range.background_id=background_id;
  return(range);
}

typedef struct _CCEquivalencePass
{
  const Image
    *image;

  CacheView
    *image_view;

  MatrixInfo
    *equivalences;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    n;

  ExceptionInfo
    *exception;
} CCEquivalencePass;

static MagickBooleanType ResolveDirectionEquivalences(
  const CCEquivalencePass *equivalence_pass,MagickBooleanType status)
{
  const Image
    *const image = equivalence_pass->image;

  CacheView
    *const image_view = equivalence_pass->image_view;

  MatrixInfo
    *const equivalences = equivalence_pass->equivalences;

  const size_t
    connectivity = equivalence_pass->connectivity;

  ssize_t
    (*const connect4)[2] = equivalence_pass->connect4,
    (*const connect8)[2] = equivalence_pass->connect8;

  const ssize_t
    n = equivalence_pass->n;

  ExceptionInfo
    *const exception = equivalence_pass->exception;

  CCNeighborDirection direction;

  ssize_t
    dx,
    dy,
    y;

  dx=connectivity > 4 ? connect8[n][1] : connect4[n][1];
  dy=connectivity > 4 ? connect8[n][0] : connect4[n][0];
  direction.dx=dx;
  direction.dy=dy;

  for (y=0; y < (ssize_t) image->rows; y++)
  {
    CCEquivalenceRow
      equivalence_row;

    const Quantum
      *magick_restrict p;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y-1,image->columns,3,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    p+=(ptrdiff_t) GetPixelChannels(image)*image->columns;
    equivalence_row.image=image;
    equivalence_row.equivalences=equivalences;
    equivalence_row.y=y;
    equivalence_row.dx=dx;
    equivalence_row.dy=dy;
    status=ResolveRowEquivalences(&equivalence_row,direction,p,status);
  }
  return(status);
}

typedef struct _CCEquivalenceInputs
{
  const Image
    *image;

  CacheView
    *image_view;

  MatrixInfo
    *equivalences;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2];

  ExceptionInfo
    *exception;
} CCEquivalenceInputs;

static MagickBooleanType ResolveEquivalences(
  const CCEquivalenceInputs *equivalence_inputs,MagickBooleanType status)
{
  const Image
    *const image = equivalence_inputs->image;

  CacheView
    *const image_view = equivalence_inputs->image_view;

  MatrixInfo
    *const equivalences = equivalence_inputs->equivalences;

  const size_t
    connectivity = equivalence_inputs->connectivity;

  ssize_t
    (*const connect4)[2] = equivalence_inputs->connect4,
    (*const connect8)[2] = equivalence_inputs->connect8;

  ExceptionInfo
    *const exception = equivalence_inputs->exception;

  ssize_t
    n;

  for (n=0; n < (ssize_t) (connectivity > 4 ? 4 : 2); n++)
  {
    CCEquivalencePass
      equivalence_pass;

    if (status == MagickFalse)
      continue;
    equivalence_pass.image=image;
    equivalence_pass.image_view=image_view;
    equivalence_pass.equivalences=equivalences;
    equivalence_pass.connectivity=connectivity;
    equivalence_pass.connect4=connect4;
    equivalence_pass.connect8=connect8;
    equivalence_pass.n=n;
    equivalence_pass.exception=exception;
    status=ResolveDirectionEquivalences(&equivalence_pass,status);
  }
  return(status);
}

typedef struct _CCObjectCensus
{
  CacheView
    *component_view;

  Image
    *component_image;

  CacheView
    *object_view;

  CCObjectInfo
    *object;

  size_t
    *neighbors;

  RectangleInfo
    bounding_box;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    i;

  ExceptionInfo
    *exception;
} CCObjectCensus;

static void CountMergedObjectNeighbors(const CCObjectCensus *object_census,
  MagickBooleanType *status,size_t *count)
{
  CacheView
    *const component_view = object_census->component_view;

  Image
    *const component_image = object_census->component_image;

  CacheView
    *const object_view = object_census->object_view;

  CCObjectInfo
    *const object = object_census->object;

  size_t
    *const neighbors = object_census->neighbors;

  const RectangleInfo
    bounding_box = object_census->bounding_box;

  const size_t
    connectivity = object_census->connectivity;

  ssize_t
    (*const connect4)[2] = object_census->connect4,
    (*const connect8)[2] = object_census->connect8;

  const ssize_t
    i = object_census->i;

  ExceptionInfo
    *const exception = object_census->exception;

  ssize_t
    y;

  for (y=0; y < (ssize_t) bounding_box.height; y++)
  {
    CCAdjacentRow
      adjacent_row;

    const Quantum
      *magick_restrict p;

    const CCAdjacentTargets
      adjacent_targets = { status, count, &p };

    if (*status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(component_view,bounding_box.x,
      bounding_box.y+y,bounding_box.width,1,exception);
    if (p == (const Quantum *) NULL)
      {
        *status=MagickFalse;
        continue;
      }
    adjacent_row.component_image=component_image;
    adjacent_row.object_view=object_view;
    adjacent_row.object=object;
    adjacent_row.neighbors=neighbors;
    adjacent_row.bounding_box=bounding_box;
    adjacent_row.connectivity=connectivity;
    adjacent_row.connect4=connect4;
    adjacent_row.connect8=connect8;
    adjacent_row.y=y;
    adjacent_row.i=i;
    adjacent_row.exception=exception;
    CountAdjacentObjectsInRow(&adjacent_row,&adjacent_targets);
  }
}

typedef struct _CCObjectMerge
{
  Image
    *component_image;

  CacheView
    *component_view;

  RectangleInfo
    bounding_box;

  ssize_t
    i;

  size_t
    id;

  ExceptionInfo
    *exception;
} CCObjectMerge;

static MagickBooleanType RelabelMergedObject(const CCObjectMerge *object_merge,
  MagickBooleanType status)
{
  Image
    *const component_image = object_merge->component_image;

  CacheView
    *const component_view = object_merge->component_view;

  const RectangleInfo
    bounding_box = object_merge->bounding_box;

  const ssize_t
    i = object_merge->i;

  const size_t
    id = object_merge->id;

  ExceptionInfo
    *const exception = object_merge->exception;

  ssize_t
    y;

  for (y=0; y < (ssize_t) bounding_box.height; y++)
  {
    CCObjectRelabel
      object_relabel;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(component_view,bounding_box.x,
      bounding_box.y+y,bounding_box.width,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    object_relabel.component_image=component_image;
    object_relabel.i=i;
    object_relabel.id=id;
    RelabelObjectRow(&object_relabel,bounding_box,q);
    if (SyncCacheViewAuthenticPixels(component_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  return(status);
}

typedef struct _CCLabelRow
{
  const Image
    *image;

  MatrixInfo
    *equivalences;

  CCObjectInfo
    *object;

  Image
    *component_image;

  ssize_t
    y;
} CCLabelRow;

static void LabelObjectRow(const CCLabelRow *label_row,
  const CCLabelTargets label_targets)
{
  const Image
    *const image = label_row->image;

  MatrixInfo
    *const equivalences = label_row->equivalences;

  CCObjectInfo
    *const object = label_row->object;

  Image
    *const component_image = label_row->component_image;

  const ssize_t
    y = label_row->y;

  ssize_t
    x;

  for (x=0; x < (ssize_t) component_image->columns; x++)
  {
    CCLabelPixel
      label_pixel;

    label_pixel.image=image;
    label_pixel.equivalences=equivalences;
    label_pixel.object=object;
    label_pixel.component_image=component_image;
    label_pixel.x=x;
    label_pixel.y=y;
    if (LabelObjectPixel(&label_pixel,&label_targets) == MagickFalse)
      break;
  }
}

typedef struct _CCLabeledObjects
{
  const Image
    *image;

  CCObjectInfo
    *object;

  Image
    *component_image;

  ssize_t
    n;
} CCLabeledObjects;

static void NormalizeLabeledObjects(const CCLabeledObjects *labeled_objects,
  ssize_t *background_id,double *min_threshold,double *max_threshold)
{
  const Image
    *const image = labeled_objects->image;

  CCObjectInfo
    *const object = labeled_objects->object;

  Image
    *const component_image = labeled_objects->component_image;

  const ssize_t
    n = labeled_objects->n;

  ssize_t
    i;

  *background_id=0;
  *min_threshold=0.0;
  *max_threshold=0.0;
  component_image->colors=(size_t) n;
  for (i=0; i < (ssize_t) component_image->colors; i++)
  {
    NormalizeLabeledObject(image,object,i);
    *max_threshold+=object[i].area;
    if (object[i].area > object[*background_id].area)
      *background_id=i;
  }
  *max_threshold+=MagickEpsilon;
}

static MagickBooleanType KeepTopObjects(const Image *component_image,
  CCObjectInfo *object,const char *artifact)
{
  CCObjectInfo
    *top_objects;

  ssize_t
    top_ids;

  top_ids=GetTopObjectIds(component_image,artifact);
  top_objects=(CCObjectInfo *) AcquireQuantumMemory(component_image->colors,
    sizeof(*top_objects));
  if (top_objects == (CCObjectInfo *) NULL)
    return(MagickFalse);
  MergeObjectsBelowTop(component_image,object,top_objects,top_ids);
  top_objects=(CCObjectInfo *) RelinquishMagickMemory(top_objects);
  return(MagickTrue);
}

typedef struct _CCMergeObject
{
  CacheView
    *component_view;

  Image
    *component_image;

  CacheView
    *object_view;

  CCObjectInfo
    *object;

  size_t
    *neighbors;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    i;

  ExceptionInfo
    *exception;
} CCMergeObject;

static MagickBooleanType MergeObject(const CCMergeObject *merge_object,
  MagickBooleanType status)
{
  CacheView
    *const component_view = merge_object->component_view;

  Image
    *const component_image = merge_object->component_image;

  CacheView
    *const object_view = merge_object->object_view;

  CCObjectInfo
    *const object = merge_object->object;

  size_t
    *const neighbors = merge_object->neighbors;

  const size_t
    connectivity = merge_object->connectivity;

  ssize_t
    (*const connect4)[2] = merge_object->connect4,
    (*const connect8)[2] = merge_object->connect8;

  const ssize_t
    i = merge_object->i;

  ExceptionInfo
    *const exception = merge_object->exception;

  CCObjectCensus
    object_census;

  CCObjectMerge
    object_merge;

  RectangleInfo
    bounding_box;

  size_t
    count,
    id;

  count=0;
  bounding_box=object[i].bounding_box;
  object_census.component_view=component_view;
  object_census.component_image=component_image;
  object_census.object_view=object_view;
  object_census.object=object;
  object_census.neighbors=neighbors;
  object_census.bounding_box=bounding_box;
  object_census.connectivity=connectivity;
  object_census.connect4=connect4;
  object_census.connect8=connect8;
  object_census.i=i;
  object_census.exception=exception;
  CountMergedObjectNeighbors(&object_census,&status,&count);
  /*
    Merge with object of greatest adjacent area.
  */
  id=GetGreatestAdjacentObject(object,neighbors,count);
  ResetAdjacentCensus(object,neighbors,count);
  object[i].area=0.0;
  object_merge.component_image=component_image;
  object_merge.component_view=component_view;
  object_merge.bounding_box=bounding_box;
  object_merge.i=i;
  object_merge.id=id;
  object_merge.exception=exception;
  status=RelabelMergedObject(&object_merge,status);
  return(status);
}

typedef struct _CCMergeInputs
{
  Image
    *component_image;

  CCObjectInfo
    *object;

  size_t
    *neighbors;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    background_id;

  ExceptionInfo
    *exception;
} CCMergeInputs;

static MagickBooleanType MergeSelectedObjects(
  const CCMergeInputs *merge_inputs,MagickBooleanType status)
{
  Image
    *const component_image = merge_inputs->component_image;

  CCObjectInfo
    *const object = merge_inputs->object;

  size_t
    *const neighbors = merge_inputs->neighbors;

  const size_t
    connectivity = merge_inputs->connectivity;

  ssize_t
    (*const connect4)[2] = merge_inputs->connect4,
    (*const connect8)[2] = merge_inputs->connect8;

  const ssize_t
    background_id = merge_inputs->background_id;

  ExceptionInfo
    *const exception = merge_inputs->exception;

  CacheView
    *component_view,
    *object_view;

  ssize_t
    i;

  component_view=AcquireAuthenticCacheView(component_image,exception);
  object_view=AcquireVirtualCacheView(component_image,exception);
  (void) SetCacheViewVirtualPixelMethod(object_view,TileVirtualPixelMethod);
  for (i=0; i < (ssize_t) component_image->colors; i++)
  {
    CCMergeObject
      merge_object;

    if (status == MagickFalse)
      continue;
    if ((object[i].merge == MagickFalse) || (i == background_id))
      continue;  /* keep object */
    /*
      Merge this object.
    */
    merge_object.component_view=component_view;
    merge_object.component_image=component_image;
    merge_object.object_view=object_view;
    merge_object.object=object;
    merge_object.neighbors=neighbors;
    merge_object.connectivity=connectivity;
    merge_object.connect4=connect4;
    merge_object.connect8=connect8;
    merge_object.i=i;
    merge_object.exception=exception;
    status=MergeObject(&merge_object,status);
  }
  object_view=DestroyCacheView(object_view);
  component_view=DestroyCacheView(component_view);
  return(status);
}

#define ConnectedComponentsImageTag  "ConnectedComponents/Image"

static void UpdateComponentsProgress(const Image *image,
  MagickOffsetType *progress,MagickBooleanType *status)
{
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      (*progress)++;
      proceed=SetImageProgress(image,ConnectedComponentsImageTag,*progress,
        image->rows);
      if (proceed == MagickFalse)
        *status=MagickFalse;
    }
}

typedef struct _CCMetricInputs
{
  const Image
    *image;

  Image
    *component_image;

  CCObjectInfo
    *object;

  ExceptionInfo
    *exception;

  ssize_t
    background_id;
} CCMetricInputs;

typedef struct _CCMetricTargets
{
  double
    *min_threshold,
    *max_threshold;

  ssize_t
    *n;

  const char
    *(*metrics)[CCMaxMetrics];
} CCMetricTargets;

static void ApplyShapeThresholds(const CCMetricInputs *inputs,
  const CCMetricTargets *targets)
{
  const Image
    *const image = inputs->image;

  Image
    *const component_image = inputs->component_image;

  CCObjectInfo
    *const object = inputs->object;

  ExceptionInfo
    *const exception = inputs->exception;

  const ssize_t
    background_id = inputs->background_id;

  double
    *const min_threshold = targets->min_threshold,
    *const max_threshold = targets->max_threshold;

  ssize_t
    *const n = targets->n;

  const char
    *(*const metrics)[CCMaxMetrics] = targets->metrics;

  const char
    *artifact;

  artifact=GetImageArtifact(image,"connected-components:perimeter-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max perimeter threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        background_id);
      (*metrics)[++(*n)]="perimeter";
      PerimeterThreshold(component_image,object,(*n),exception);
      MergeObjectsOutsideRange(object,component_image->colors,(*n),&range);
    }
  artifact=GetImageArtifact(image,"connected-components:circularity-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max circularity threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        background_id);
      (*metrics)[++(*n)]="circularity";
      CircularityThreshold(component_image,object,(*n),exception);
      MergeObjectsOutsideRange(object,component_image->colors,(*n),&range);
    }
  artifact=GetImageArtifact(image,"connected-components:diameter-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max diameter threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        background_id);
      (*metrics)[++(*n)]="diameter";
      DiameterThreshold(component_image,object,(*n),range);
    }
}

static void ApplyAxisThresholds(const CCMetricInputs *inputs,
  const CCMetricTargets *targets)
{
  const Image
    *const image = inputs->image;

  Image
    *const component_image = inputs->component_image;

  CCObjectInfo
    *const object = inputs->object;

  ExceptionInfo
    *const exception = inputs->exception;

  const ssize_t
    background_id = inputs->background_id;

  double
    *const min_threshold = targets->min_threshold,
    *const max_threshold = targets->max_threshold;

  ssize_t
    *const n = targets->n;

  const char
    *(*const metrics)[CCMaxMetrics] = targets->metrics;

  const char
    *artifact;

  artifact=GetImageArtifact(image,"connected-components:major-axis-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max ellipse major threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        background_id);
      (*metrics)[++(*n)]="major-axis";
      MajorAxisThreshold(component_image,object,(*n),exception);
      MergeObjectsOutsideRange(object,component_image->colors,(*n),&range);
    }
  artifact=GetImageArtifact(image,"connected-components:minor-axis-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max ellipse minor threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        background_id);
      (*metrics)[++(*n)]="minor-axis";
      MinorAxisThreshold(component_image,object,(*n),exception);
      MergeObjectsOutsideRange(object,component_image->colors,(*n),&range);
    }
  artifact=GetImageArtifact(image,"connected-components:eccentricity-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max eccentricity threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        background_id);
      (*metrics)[++(*n)]="eccentricity";
      EccentricityThreshold(component_image,object,(*n),exception);
      MergeObjectsOutsideRange(object,component_image->colors,(*n),&range);
    }
  artifact=GetImageArtifact(image,"connected-components:angle-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max ellipse angle threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        background_id);
      (*metrics)[++(*n)]="angle";
      AngleThreshold(component_image,object,(*n),exception);
      MergeObjectsOutsideRange(object,component_image->colors,(*n),&range);
    }
}

static void KeepSelectedObjects(const Image *image,
  const Image *component_image,CCObjectInfo *object,ExceptionInfo *exception)
{
  const char
    *artifact;

  artifact=GetImageArtifact(image,"connected-components:keep-colors");
  if (artifact != (const char *) NULL)
    {
      CCColorMerge
        color_merge;

      /*
        Keep selected objects based on color, merge others.
      */
      MergeAllObjects(component_image,object);
      color_merge.component_image=component_image;
      color_merge.object=object;
      color_merge.artifact=artifact;
      color_merge.merge=MagickFalse;
      color_merge.exception=exception;
      MergeObjectsByColor(&color_merge);
    }
  artifact=GetImageArtifact(image,"connected-components:keep-ids");
  if (artifact == (const char *) NULL)
    artifact=GetImageArtifact(image,"connected-components:keep");
  if (artifact != (const char *) NULL)
    {
      /*
        Keep selected objects based on id, merge others.
      */
      MergeAllObjects(component_image,object);
      SetObjectMergeByIds(object,component_image->colors,artifact,MagickFalse);
    }
}

static void RemoveSelectedObjects(const Image *image,
  const Image *component_image,CCObjectInfo *object,ExceptionInfo *exception)
{
  const char
    *artifact;

  artifact=GetImageArtifact(image,"connected-components:remove-colors");
  if (artifact != (const char *) NULL)
    {
      /*
        Remove selected objects based on color, keep others.
      */
      RemoveObjectsByColor(component_image,object,artifact,exception);
    }
  artifact=GetImageArtifact(image,"connected-components:remove-ids");
  if (artifact == (const char *) NULL)
    artifact=GetImageArtifact(image,"connected-components:remove");
  if (artifact != (const char *) NULL)
    {
      /*
        Remove selected objects based on id, keep others.
      */
      SetObjectMergeByIds(object,component_image->colors,artifact,MagickTrue);
    }
}

typedef struct _CCReportInputs
{
  const Image
    *image;

  Image
    *component_image;

  CCObjectInfo
    *object,
    **objects;

  ssize_t
    n;

  ExceptionInfo
    *exception;
} CCReportInputs;

static Image *ReportConnectedComponents(const CCReportInputs *report_inputs,
  const char **metrics,MagickBooleanType status)
{
  const Image
    *const image = report_inputs->image;

  Image
    *const component_image = report_inputs->component_image;

  CCObjectInfo
    *object = report_inputs->object,
    **const objects = report_inputs->objects;

  const ssize_t
    n = report_inputs->n;

  ExceptionInfo
    *const exception = report_inputs->exception;

  const char
    *artifact;

  artifact=GetImageArtifact(image,"connected-components:mean-color");
  if (IsStringTrue(artifact) != MagickFalse)
    {
      /*
        Replace object with mean color.
      */
      SetObjectMeanColors(component_image,object);
    }
  (void) SyncImage(component_image,exception);
  artifact=GetImageArtifact(image,"connected-components:verbose");
  if ((IsStringTrue(artifact) != MagickFalse) ||
      (objects != (CCObjectInfo **) NULL))
    {
      /*
        Report statistics on each unique object.
      */
      status=ComputeObjectStatistics(component_image,object,status,exception);
      SortObjects(image,component_image,object);
      if (objects == (CCObjectInfo **) NULL)
        {
          PrintObjectsHeader(image,metrics,n);
          PrintObjects(image,component_image,object,n);
        }
    }
  if (objects == (CCObjectInfo **) NULL)
    object=(CCObjectInfo *) RelinquishMagickMemory(object);
  else
    *objects=object;
  return(component_image);
}

static MagickBooleanType SyncComponentView(CacheView *component_view,
  MagickBooleanType status,ExceptionInfo *exception)
{
  if (SyncCacheViewAuthenticPixels(component_view,exception) == MagickFalse)
    status=MagickFalse;
  return(status);
}

typedef struct _CCLabelInputs
{
  const Image
    *image;

  CacheView
    *image_view;

  MatrixInfo
    *equivalences;

  CCObjectInfo
    *object;

  Image
    *component_image;

  ExceptionInfo
    *exception;
} CCLabelInputs;

typedef struct _CCLabelCounts
{
  ssize_t
    *n;

  MagickBooleanType
    *status;

  MagickOffsetType
    *progress;
} CCLabelCounts;

static void LabelConnectedComponents(const CCLabelInputs *label_inputs,
  const CCLabelCounts *label_counts)
{
  const Image
    *const image = label_inputs->image;

  CacheView
    *const image_view = label_inputs->image_view;

  MatrixInfo
    *const equivalences = label_inputs->equivalences;

  CCObjectInfo
    *const object = label_inputs->object;

  Image
    *const component_image = label_inputs->component_image;

  ExceptionInfo
    *const exception = label_inputs->exception;

  ssize_t
    *const n = label_counts->n;

  MagickBooleanType
    *const status = label_counts->status;

  MagickOffsetType
    *const progress = label_counts->progress;

  CacheView
    *component_view;

  ssize_t
    y;

  (*n)=0;
  component_view=AcquireAuthenticCacheView(component_image,exception);
  for (y=0; y < (ssize_t) component_image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    const CCLabelTargets
      label_targets = { status, n, &p, &q };

    CCLabelRow
      label_row;

    if ((*status) == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    q=QueueCacheViewAuthenticPixels(component_view,0,y,component_image->columns,
      1,exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        (*status)=MagickFalse;
        continue;
      }
    label_row.image=image;
    label_row.equivalences=equivalences;
    label_row.object=object;
    label_row.component_image=component_image;
    label_row.y=y;
    LabelObjectRow(&label_row,label_targets);
    if ((*n) > (ssize_t) MaxColormapSize)
      break;
    (*status)=SyncComponentView(component_view,(*status),exception);
    UpdateComponentsProgress(image,progress,status);
  }
  component_view=DestroyCacheView(component_view);
}

static void InitializeEquivalences(const Image *image,
  MatrixInfo *equivalences)
{
  ssize_t
    n;

  for (n=0; n < (ssize_t) (image->columns*image->rows); n++)
    (void) SetMatrixElement(equivalences,n,0,&n);
}

typedef struct _CCFinishInputs
{
  const Image
    *image;

  Image
    *component_image;

  CCObjectInfo
    *object,
    **objects;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    background_id,
    n;

  MagickBooleanType
    status;

  ExceptionInfo
    *exception;
} CCFinishInputs;

static Image *MergeAndReportComponents(const CCFinishInputs *finish_inputs,
  const char **metrics)
{
  const Image
    *const image = finish_inputs->image;

  Image
    *component_image = finish_inputs->component_image;

  CCObjectInfo
    *object = finish_inputs->object,
    **const objects = finish_inputs->objects;

  const size_t
    connectivity = finish_inputs->connectivity;

  ssize_t
    (*const connect4)[2] = finish_inputs->connect4,
    (*const connect8)[2] = finish_inputs->connect8;

  const ssize_t
    background_id = finish_inputs->background_id,
    n = finish_inputs->n;

  MagickBooleanType
    status = finish_inputs->status;

  ExceptionInfo
    *const exception = finish_inputs->exception;

  CCMergeInputs
    merge_inputs;

  CCReportInputs
    report_inputs;

  size_t
    *neighbors;

  /*
    Merge any object not within the min and max area threshold.
  */
  neighbors=(size_t *) AcquireQuantumMemory(component_image->colors+1,
    sizeof(*neighbors));
  if (neighbors == (size_t *) NULL)
    {
      object=(CCObjectInfo *) RelinquishMagickMemory(object);
      component_image=DestroyImage(component_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  merge_inputs.component_image=component_image;
  merge_inputs.object=object;
  merge_inputs.neighbors=neighbors;
  merge_inputs.connectivity=connectivity;
  merge_inputs.connect4=connect4;
  merge_inputs.connect8=connect8;
  merge_inputs.background_id=background_id;
  merge_inputs.exception=exception;
  status=MergeSelectedObjects(&merge_inputs,status);
  neighbors=(size_t *) RelinquishMagickMemory(neighbors);
  report_inputs.image=image;
  report_inputs.component_image=component_image;
  report_inputs.object=object;
  report_inputs.objects=objects;
  report_inputs.n=n;
  report_inputs.exception=exception;
  return(ReportConnectedComponents(&report_inputs,metrics,status));
}

typedef struct _CCSelectionInputs
{
  const Image
    *image;

  CCObjectInfo
    *object;

  Image
    *component_image;
} CCSelectionInputs;

typedef struct _CCSelectionTargets
{
  ssize_t
    *background_id;

  double
    *min_threshold,
    *max_threshold;

  ssize_t
    *n;
} CCSelectionTargets;

static void PrepareObjectSelection(const CCSelectionInputs *selection_inputs,
  const CCSelectionTargets *selection_targets)
{
  const Image
    *const image = selection_inputs->image;

  CCObjectInfo
    *const object = selection_inputs->object;

  Image
    *const component_image = selection_inputs->component_image;

  ssize_t
    *const background_id = selection_targets->background_id;

  double
    *const min_threshold = selection_targets->min_threshold,
    *const max_threshold = selection_targets->max_threshold;

  ssize_t
    *const n = selection_targets->n;

  CCLabeledObjects
    labeled_objects;

  const char
    *artifact;

  labeled_objects.image=image;
  labeled_objects.object=object;
  labeled_objects.component_image=component_image;
  labeled_objects.n=(*n);
  NormalizeLabeledObjects(&labeled_objects,background_id,min_threshold,
    max_threshold);
  (*n)=(-1);
  artifact=GetImageArtifact(image,"connected-components:background-id");
  if (artifact != (const char *) NULL)
    (*background_id)=(ssize_t) StringToLong(artifact);
  artifact=GetImageArtifact(image,"connected-components:area-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max area threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        (*background_id));
      AreaThreshold(component_image,object,range);
    }
}

typedef struct _CCThresholdInputs
{
  const Image
    *image;

  Image
    *component_image;

  CCObjectInfo
    *object,
    **objects;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    background_id;

  double
    min_threshold,
    max_threshold;

  ssize_t
    n;

  MagickBooleanType
    status;

  ExceptionInfo
    *exception;
} CCThresholdInputs;

static Image *ApplyObjectThresholds(const CCThresholdInputs *threshold_inputs)
{
  CCFinishInputs
    finish_inputs;

  CCMetricInputs
    inputs;

  CCMetricTargets
    targets;

  const char
    *metrics[CCMaxMetrics];

  double
    max_threshold = threshold_inputs->max_threshold,
    min_threshold = threshold_inputs->min_threshold;

  ssize_t
    n = threshold_inputs->n;

  inputs.image=threshold_inputs->image;
  inputs.component_image=threshold_inputs->component_image;
  inputs.object=threshold_inputs->object;
  inputs.exception=threshold_inputs->exception;
  inputs.background_id=threshold_inputs->background_id;
  targets.min_threshold=(&min_threshold);
  targets.max_threshold=(&max_threshold);
  targets.n=(&n);
  targets.metrics=(&metrics);
  ApplyShapeThresholds(&inputs,&targets);
  inputs.image=threshold_inputs->image;
  inputs.component_image=threshold_inputs->component_image;
  inputs.object=threshold_inputs->object;
  inputs.exception=threshold_inputs->exception;
  inputs.background_id=threshold_inputs->background_id;
  targets.min_threshold=(&min_threshold);
  targets.max_threshold=(&max_threshold);
  targets.n=(&n);
  targets.metrics=(&metrics);
  ApplyAxisThresholds(&inputs,&targets);
  finish_inputs.image=threshold_inputs->image;
  finish_inputs.component_image=threshold_inputs->component_image;
  finish_inputs.object=threshold_inputs->object;
  finish_inputs.objects=threshold_inputs->objects;
  finish_inputs.connectivity=threshold_inputs->connectivity;
  finish_inputs.connect4=threshold_inputs->connect4;
  finish_inputs.connect8=threshold_inputs->connect8;
  finish_inputs.background_id=threshold_inputs->background_id;
  finish_inputs.n=n;
  finish_inputs.status=threshold_inputs->status;
  finish_inputs.exception=threshold_inputs->exception;
  return(MergeAndReportComponents(&finish_inputs,metrics));
}

static Image *SelectConnectedComponents(const CCThresholdInputs *selection)
{
  const Image
    *const image = selection->image;

  Image
    *component_image = selection->component_image;

  CCObjectInfo
    *object = selection->object,
    **const objects = selection->objects;

  const size_t
    connectivity = selection->connectivity;

  ssize_t
    (*const connect4)[2] = selection->connect4,
    (*const connect8)[2] = selection->connect8;

  const ssize_t
    background_id = selection->background_id,
    n = selection->n;

  const double
    min_threshold = selection->min_threshold,
    max_threshold = selection->max_threshold;

  const MagickBooleanType
    status = selection->status;

  ExceptionInfo
    *const exception = selection->exception;

  CCThresholdInputs
    threshold_inputs;

  const char
    *artifact;

  KeepSelectedObjects(image,component_image,object,exception);
  artifact=GetImageArtifact(image,"connected-components:keep-top");
  /*
    Keep top objects.
  */
  if ((artifact != (const char *) NULL) &&
      (KeepTopObjects(component_image,object,artifact) == MagickFalse))
    {
      object=(CCObjectInfo *) RelinquishMagickMemory(object);
      component_image=DestroyImage(component_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  RemoveSelectedObjects(image,component_image,object,exception);
  threshold_inputs.image=image;
  threshold_inputs.component_image=component_image;
  threshold_inputs.object=object;
  threshold_inputs.objects=objects;
  threshold_inputs.connectivity=connectivity;
  threshold_inputs.connect4=connect4;
  threshold_inputs.connect8=connect8;
  threshold_inputs.background_id=background_id;
  threshold_inputs.min_threshold=min_threshold;
  threshold_inputs.max_threshold=max_threshold;
  threshold_inputs.n=n;
  threshold_inputs.status=status;
  threshold_inputs.exception=exception;
  return(ApplyObjectThresholds(&threshold_inputs));
}

static void ResetObjectsResult(CCObjectInfo **objects)
{
  if (objects != (CCObjectInfo **) NULL)
    *objects=(CCObjectInfo *) NULL;
}

typedef struct _CCLabeledInputs
{
  const Image
    *image;

  Image
    *component_image;

  CCObjectInfo
    *object,
    **objects;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    n;

  MagickBooleanType
    status;

  ExceptionInfo
    *exception;
} CCLabeledInputs;

static Image *SelectLabeledComponents(const CCLabeledInputs *labeled)
{
  const Image
    *const image = labeled->image;

  Image
    *component_image = labeled->component_image;

  CCObjectInfo
    *object = labeled->object,
    **const objects = labeled->objects;

  const size_t
    connectivity = labeled->connectivity;

  ssize_t
    (*const connect4)[2] = labeled->connect4,
    (*const connect8)[2] = labeled->connect8,
    background_id,
    n = labeled->n;

  const MagickBooleanType
    status = labeled->status;

  ExceptionInfo
    *const exception = labeled->exception;

  CCSelectionInputs
    selection_inputs;

  CCSelectionTargets
    selection_targets;

  CCThresholdInputs
    selection;

  double
    max_threshold,
    min_threshold;

  if (n > (ssize_t) MaxColormapSize)
    {
      object=(CCObjectInfo *) RelinquishMagickMemory(object);
      component_image=DestroyImage(component_image);
      ThrowImageException(ResourceLimitError,"TooManyObjects");
    }
  selection_inputs.image=image;
  selection_inputs.object=object;
  selection_inputs.component_image=component_image;
  selection_targets.background_id=(&background_id);
  selection_targets.min_threshold=(&min_threshold);
  selection_targets.max_threshold=(&max_threshold);
  selection_targets.n=(&n);
  PrepareObjectSelection(&selection_inputs,&selection_targets);
  selection.image=image;
  selection.component_image=component_image;
  selection.object=object;
  selection.objects=objects;
  selection.connectivity=connectivity;
  selection.connect4=connect4;
  selection.connect8=connect8;
  selection.background_id=background_id;
  selection.min_threshold=min_threshold;
  selection.max_threshold=max_threshold;
  selection.n=n;
  selection.status=status;
  selection.exception=exception;
  return(SelectConnectedComponents(&selection));
}

typedef struct _CCComponentInputs
{
  const Image
    *image;

  Image
    *component_image;

  MatrixInfo
    *equivalences;

  CCObjectInfo
    *object,
    **objects;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2];

  ExceptionInfo
    *exception;
} CCComponentInputs;

static Image *FindConnectedComponents(const CCComponentInputs *components)
{
  CacheView
    *image_view;

  CCEquivalenceInputs
    equivalence_inputs;

  CCLabelCounts
    label_counts;

  CCLabelInputs
    label_inputs;

  CCLabeledInputs
    labeled;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  MatrixInfo
    *equivalences = components->equivalences;

  ssize_t
    n;

  /*
    Find connected components.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(components->image,components->exception);
  equivalence_inputs.image=components->image;
  equivalence_inputs.image_view=image_view;
  equivalence_inputs.equivalences=equivalences;
  equivalence_inputs.connectivity=components->connectivity;
  equivalence_inputs.connect4=components->connect4;
  equivalence_inputs.connect8=components->connect8;
  equivalence_inputs.exception=components->exception;
  status=ResolveEquivalences(&equivalence_inputs,status);
  /*
    Label connected components.
  */
  label_inputs.image=components->image;
  label_inputs.image_view=image_view;
  label_inputs.equivalences=equivalences;
  label_inputs.object=components->object;
  label_inputs.component_image=components->component_image;
  label_inputs.exception=components->exception;
  label_counts.n=(&n);
  label_counts.status=(&status);
  label_counts.progress=(&progress);
  LabelConnectedComponents(&label_inputs,&label_counts);
  image_view=DestroyCacheView(image_view);
  equivalences=DestroyMatrixInfo(equivalences);
  labeled.image=components->image;
  labeled.component_image=components->component_image;
  labeled.object=components->object;
  labeled.objects=components->objects;
  labeled.connectivity=components->connectivity;
  labeled.connect4=components->connect4;
  labeled.connect8=components->connect8;
  labeled.n=n;
  labeled.status=status;
  labeled.exception=components->exception;
  return(SelectLabeledComponents(&labeled));
}

MagickExport Image *ConnectedComponentsImage(const Image *image,
  const size_t connectivity,CCObjectInfo **objects,ExceptionInfo *exception)
{
  CCComponentInputs
    components;

  CCObjectInfo
    *object;

  Image
    *component_image;

  MatrixInfo
    *equivalences;

  size_t
    size;

  ssize_t
    connect4[2][2] = { { -1,  0 }, {  0, -1 } },
    connect8[4][2] = { { -1, -1 }, { -1,  0 }, { -1,  1 }, {  0, -1 } };

  /*
    Initialize connected components image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  ResetObjectsResult(objects);
  component_image=CloneImage(image,0,0,MagickTrue,exception);
  if (component_image == (Image *) NULL)
    return((Image *) NULL);
  component_image->depth=MAGICKCORE_QUANTUM_DEPTH;
  if (AcquireImageColormap(component_image,MaxColormapSize,exception) == MagickFalse)
    {
      component_image=DestroyImage(component_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Initialize connected components equivalences.
  */
  size=image->columns*image->rows;
  if (image->columns != (size/image->rows))
    {
      component_image=DestroyImage(component_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  equivalences=AcquireMatrixInfo(size,1,sizeof(ssize_t),exception);
  if (equivalences == (MatrixInfo *) NULL)
    {
      component_image=DestroyImage(component_image);
      return((Image *) NULL);
    }
  InitializeEquivalences(image,equivalences);
  object=(CCObjectInfo *) AcquireQuantumMemory(MaxColormapSize,sizeof(*object));
  if (object == (CCObjectInfo *) NULL)
    {
      equivalences=DestroyMatrixInfo(equivalences);
      component_image=DestroyImage(component_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  (void) memset(object,0,MaxColormapSize*sizeof(*object));
  InitializeObjects(image,object);
  /*
    Find connected components.
  */
  components.image=image;
  components.component_image=component_image;
  components.equivalences=equivalences;
  components.object=object;
  components.objects=objects;
  components.connectivity=connectivity;
  components.connect4=connect4;
  components.connect8=connect8;
  components.exception=exception;
  return(FindConnectedComponents(&components));
}
