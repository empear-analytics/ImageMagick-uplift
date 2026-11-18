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

  MagickCore private methods of vision-components.c: the object metrics and
  helpers of ConnectedComponentsImage, from the CCThresholdRange typedef to
  AddLabeledObjectPixel; included by that file only.
*/
#ifndef MAGICKCORE_VISION_COMPONENTS_OBJECT_PRIVATE_H
#define MAGICKCORE_VISION_COMPONENTS_OBJECT_PRIVATE_H

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

#endif
