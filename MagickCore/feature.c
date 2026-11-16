/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               FFFFF  EEEEE   AAA   TTTTT  U   U  RRRR   EEEEE               %
%               F      E      A   A    T    U   U  R   R  E                   %
%               FFF    EEE    AAAAA    T    U   U  RRRR   EEE                 %
%               F      E      A   A    T    U   U  R R    E                   %
%               F      EEEEE  A   A    T     UUU   R  R   EEEEE               %
%                                                                             %
%                                                                             %
%                      MagickCore Image Feature Methods                       %
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
#include "MagickCore/animate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
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
#include "MagickCore/compress.h"
#include "MagickCore/constitute.h"
#include "MagickCore/display.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/feature.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/list.h"
#include "MagickCore/image-private.h"
#include "MagickCore/magic.h"
#include "MagickCore/magick.h"
#include "MagickCore/matrix.h"
#include "MagickCore/memory_.h"
#include "MagickCore/module.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/morphology-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/statistic-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/timer.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/feature-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C a n n y E d g e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CannyEdgeImage() uses a multi-stage algorithm to detect a wide range of
%  edges in images.
%
%  The format of the CannyEdgeImage method is:
%
%      Image *CannyEdgeImage(const Image *image,const double radius,
%        const double sigma,const double lower_percent,
%        const double upper_percent,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the gaussian smoothing filter.
%
%    o sigma: the sigma of the gaussian smoothing filter.
%
%    o lower_percent: percentage of edge pixels in the lower threshold.
%
%    o upper_percent: percentage of edge pixels in the upper threshold.
%
%    o exception: return any errors or warnings in this structure.
%
*/

typedef struct _CannyInfo
{
  double
    magnitude,
    intensity;

  int
    orientation;

  ssize_t
    x,
    y;
} CannyInfo;

typedef struct _CannyTrace
{
  Image
    *edge_image;

  CacheView
    *edge_view;

  MatrixInfo
    *canny_cache;

  double
    lower_threshold;
} CannyTrace;

static inline MagickBooleanType IsAuthenticPixel(const Image *image,
  const ssize_t x,const ssize_t y)
{
  if ((x < 0) || (x >= (ssize_t) image->columns))
    return(MagickFalse);
  if ((y < 0) || (y >= (ssize_t) image->rows))
    return(MagickFalse);
  return(MagickTrue);
}

static inline MagickBooleanType IsUntracedEdgePixel(const Image *edge_image,
  const Quantum *q,const CannyInfo pixel,const double lower_threshold)
{
  return(((GetPixelIntensity(edge_image,q) == 0.0) &&
    (pixel.intensity >= lower_threshold)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType SeedCannyTrace(const CannyTrace *trace,
  const ssize_t x,const ssize_t y,ExceptionInfo *exception)
{
  CacheView
    *edge_view = trace->edge_view;

  MatrixInfo
    *canny_cache = trace->canny_cache;

  CannyInfo
    edge;

  MagickBooleanType
    status;

  Quantum
    *q;

  q=GetCacheViewAuthenticPixels(edge_view,x,y,1,1,exception);
  if (q == (Quantum *) NULL)
    return(MagickFalse);
  *q=QuantumRange;
  status=SyncCacheViewAuthenticPixels(edge_view,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  if (GetMatrixElement(canny_cache,0,0,&edge) == MagickFalse)
    return(MagickFalse);
  edge.x=x;
  edge.y=y;
  if (SetMatrixElement(canny_cache,0,0,&edge) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _CannyNeighbor
{
  const CannyTrace
    *trace;

  ExceptionInfo
    *exception;

  ssize_t
    u,
    v;
} CannyNeighbor;

typedef struct _CannyTraceTargets
{
  CannyInfo
    *edge;

  ssize_t
    *i;
} CannyTraceTargets;

static MagickBooleanType TraceCannyNeighbor(const CannyNeighbor *neighbor,
  const CannyTraceTargets *targets)
{
  const CannyTrace
    *const trace = neighbor->trace;

  Image
    *edge_image = trace->edge_image;

  CacheView
    *edge_view = trace->edge_view;

  MatrixInfo
    *canny_cache = trace->canny_cache;

  const double
    lower_threshold = trace->lower_threshold;

  ExceptionInfo
    *const exception = neighbor->exception;

  const ssize_t
    u = neighbor->u,
    v = neighbor->v;

  CannyInfo
    *const edge = targets->edge;

  ssize_t
    *const i = targets->i;

  CannyInfo
    pixel;

  MagickBooleanType
    status;

  Quantum
    *q;

  q=GetCacheViewAuthenticPixels(edge_view,(*edge).x+u,(*edge).y+v,1,1,
    exception);
  if (q == (Quantum *) NULL)
    return(MagickFalse);
  status=GetMatrixElement(canny_cache,(*edge).x+u,(*edge).y+v,&pixel);
  if (status == MagickFalse)
    return(MagickFalse);
  if (IsUntracedEdgePixel(edge_image,q,pixel,lower_threshold) !=
      MagickFalse)
    {
      *q=QuantumRange;
      status=SyncCacheViewAuthenticPixels(edge_view,exception);
      if (status == MagickFalse)
        return(MagickFalse);
      (*edge).x+=u;
      (*edge).y+=v;
      status=SetMatrixElement(canny_cache,(*i),0,edge);
      if (status == MagickFalse)
        return(MagickFalse);
      (*i)++;
    }
  return(MagickTrue);
}

static MagickBooleanType TraceCannyRow(const CannyTrace *trace,
  ExceptionInfo *exception,const ssize_t v,const CannyTraceTargets *targets)
{
  Image
    *edge_image = trace->edge_image;

  const CannyInfo
    *const edge = targets->edge;

  ssize_t
    u;

  for (u=(-1); u <= 1; u++)
  {
    CannyNeighbor
      neighbor;

    if ((u == 0) && (v == 0))
      continue;
    if (IsAuthenticPixel(edge_image,(*edge).x+u,(*edge).y+v) == MagickFalse)
      continue;
    /*
      Not an edge if gradient value is below the lower threshold.
    */
    neighbor.trace=trace;
    neighbor.exception=exception;
    neighbor.u=u;
    neighbor.v=v;
    if (TraceCannyNeighbor(&neighbor,targets) == MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType TraceCannyRows(const CannyTrace *trace,
  ExceptionInfo *exception,const CannyTraceTargets *targets)
{
  ssize_t
    v;

  for (v=(-1); v <= 1; v++)
  {
    if (TraceCannyRow(trace,exception,v,targets) == MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType TraceEdges(const CannyTrace *trace,const ssize_t x,
  const ssize_t y,ExceptionInfo *exception)
{
  MatrixInfo
    *canny_cache = trace->canny_cache;

  CannyInfo
    edge;

  MagickBooleanType
    status;

  ssize_t
    i;

  const CannyTraceTargets
    targets = { &edge, &i };

  if (SeedCannyTrace(trace,x,y,exception) == MagickFalse)
    return(MagickFalse);
  for (i=1; i != 0; )
  {
    i--;
    status=GetMatrixElement(canny_cache,i,0,&edge);
    if (status == MagickFalse)
      return(MagickFalse);
    if (TraceCannyRows(trace,exception,&targets) == MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

static int GetCannyNegativeSlopeOrientation(const double slope)
{
  int
    orientation;

  if (slope < -2.41421356237)
    orientation=0;
  else
    if (slope < -0.414213562373)
      orientation=1;
    else
      orientation=2;
  return(orientation);
}

static CannyInfo SetCannyOrientation(CannyInfo pixel,const double dx,
  const double dy)
{
  double
    slope;

  if (!(fabs(dx) > MagickEpsilon))
    return(pixel);
  slope=dy/dx;
  if (slope < 0.0)
    pixel.orientation=GetCannyNegativeSlopeOrientation(slope);
  else
    {
      if (slope > 2.41421356237)
        pixel.orientation=0;
      else
        if (slope > 0.414213562373)
          pixel.orientation=3;
        else
          pixel.orientation=2;
    }
  return(pixel);
}

typedef struct _CannyPosition
{
  ssize_t
    x,
    y;

  int
    orientation;
} CannyPosition;

static void GetCannyNeighbors(const MatrixInfo *canny_cache,
  const CannyPosition *position,CannyInfo *alpha_pixel,CannyInfo *beta_pixel)
{
  const ssize_t
    x = position->x,
    y = position->y;

  const int
    orientation = position->orientation;

  switch (orientation)
  {
    case 0:
    default:
    {
      /*
        0 degrees, north and south.
      */
      (void) GetMatrixElement(canny_cache,x,y-1,alpha_pixel);
      (void) GetMatrixElement(canny_cache,x,y+1,beta_pixel);
      break;
    }
    case 1:
    {
      /*
        45 degrees, northwest and southeast.
      */
      (void) GetMatrixElement(canny_cache,x-1,y-1,alpha_pixel);
      (void) GetMatrixElement(canny_cache,x+1,y+1,beta_pixel);
      break;
    }
    case 2:
    {
      /*
        90 degrees, east and west.
      */
      (void) GetMatrixElement(canny_cache,x-1,y,alpha_pixel);
      (void) GetMatrixElement(canny_cache,x+1,y,beta_pixel);
      break;
    }
    case 3:
    {
      /*
        135 degrees, northeast and southwest.
      */
      (void) GetMatrixElement(canny_cache,x+1,y-1,beta_pixel);
      (void) GetMatrixElement(canny_cache,x-1,y+1,alpha_pixel);
      break;
    }
  }
}

static void GetCannyGradient(const Image *edge_image,
  const Quantum *magick_restrict p,double *dx,double *dy)
{
  const Quantum
    *magick_restrict kernel_pixels;

  ssize_t
    v;

  static double
    Gx[2][2] =
    {
      { -1.0,  +1.0 },
      { -1.0,  +1.0 }
    },
    Gy[2][2] =
    {
      { +1.0, +1.0 },
      { -1.0, -1.0 }
    };

  kernel_pixels=p;
  for (v=0; v < 2; v++)
  {
    ssize_t
      u;

    for (u=0; u < 2; u++)
    {
      double
        intensity;

      intensity=GetPixelIntensity(edge_image,kernel_pixels+u);
      *dx+=0.5*Gx[v][u]*intensity;
      *dy+=0.5*Gy[v][u]*intensity;
    }
    kernel_pixels+=edge_image->columns+1;
  }
}

static void SetCannyGradientRow(const Image *edge_image,
  MatrixInfo *canny_cache,const Quantum *magick_restrict p,const ssize_t y)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) edge_image->columns; x++)
  {
    CannyInfo
      pixel;

    double
      dx,
      dy;

    (void) memset(&pixel,0,sizeof(pixel));
    dx=0.0;
    dy=0.0;
    GetCannyGradient(edge_image,p,&dx,&dy);
    pixel.magnitude=hypot(dx,dy);
    pixel.orientation=0;
    pixel=SetCannyOrientation(pixel,dx,dy);
    if (SetMatrixElement(canny_cache,x,y,&pixel) == MagickFalse)
      continue;
    p+=(ptrdiff_t) GetPixelChannels(edge_image);
  }
}

typedef struct _CannyHysteresis
{
  Image
    *edge_image;

  CacheView
    *edge_view;

  MatrixInfo
    *canny_cache;

  double
    upper_threshold;

  ssize_t
    y;
} CannyHysteresis;

static MagickBooleanType TraceCannyEdgeRow(const CannyHysteresis *hysteresis,
  CannyTrace trace,MagickBooleanType status,ExceptionInfo *exception)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) hysteresis->edge_image->columns; x++)
  {
    CannyInfo
      pixel;

    const Quantum
      *magick_restrict p;

    /*
      Edge if pixel gradient higher than upper threshold.
    */
    p=GetCacheViewVirtualPixels(hysteresis->edge_view,x,hysteresis->y,1,1,
      exception);
    if (p == (const Quantum *) NULL)
      continue;
    status=GetMatrixElement(hysteresis->canny_cache,x,hysteresis->y,&pixel);
    if (status == MagickFalse)
      continue;
    if ((GetPixelIntensity(hysteresis->edge_image,p) == 0.0) &&
        (pixel.intensity >= hysteresis->upper_threshold))
      status=TraceEdges(&trace,x,hysteresis->y,exception);
  }
  return(status);
}

static CannyInfo SuppressCannyNonMaxima(MatrixInfo *canny_cache,
  const ssize_t x,const ssize_t y)
{
  CannyInfo
    alpha_pixel,
    beta_pixel,
    pixel;

  CannyPosition
    position;

  (void) GetMatrixElement(canny_cache,x,y,&pixel);
  position.x=x;
  position.y=y;
  position.orientation=pixel.orientation;
  GetCannyNeighbors(canny_cache,&position,&alpha_pixel,&beta_pixel);
  pixel.intensity=pixel.magnitude;
  if ((pixel.magnitude < alpha_pixel.magnitude) ||
      (pixel.magnitude < beta_pixel.magnitude))
    pixel.intensity=0;
  (void) SetMatrixElement(canny_cache,x,y,&pixel);
  return(pixel);
}

static MagickBooleanType SetCannyEdgeGray(Image *edge_image,
  ExceptionInfo *exception)
{
  if (TransformImageColorspace(edge_image,GRAYColorspace,exception) == MagickFalse)
    {
      edge_image=DestroyImage(edge_image);
      return(MagickFalse);
    }
  (void) SetImageAlphaChannel(edge_image,OffAlphaChannel,exception);
  return(MagickTrue);
}

typedef struct _ComputeCannyGradientRowInfo
{
  CacheView
    *edge_view;

  Image
    *edge_image;

  ExceptionInfo
    *exception;

  MatrixInfo
    *canny_cache;
} ComputeCannyGradientRowInfo;

static void ComputeCannyGradientRow(MagickBooleanType *status,
  const ComputeCannyGradientRowInfo *info,const ssize_t y)
{
  CacheView
    *const edge_view = info->edge_view;

  Image
    *const edge_image = info->edge_image;

  ExceptionInfo
    *const exception = info->exception;

  MatrixInfo
    *const canny_cache = info->canny_cache;

  const Quantum
    *magick_restrict p;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(edge_view,0,y,edge_image->columns+1,2,
    exception);
  if (p == (const Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return;
    }
  SetCannyGradientRow(edge_image,canny_cache,p,y);
}

static void SyncFeatureView(CacheView *view,ExceptionInfo *exception,
  MagickBooleanType *status)
{
  if (SyncCacheViewAuthenticPixels(view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

static MagickBooleanType GetCannyRow(
  const ComputeCannyGradientRowInfo *info,const ssize_t y,
  Quantum *magick_restrict *q,MagickBooleanType *status)
{
  CacheView
    *const edge_view = info->edge_view;

  Image
    *const edge_image = info->edge_image;

  ExceptionInfo
    *const exception = info->exception;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  (*q)=GetCacheViewAuthenticPixels(edge_view,0,y,edge_image->columns,1,
    exception);
  if ((*q) == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType BlurCannyImage(const Image *image,
  KernelInfo *kernel_info,Image **edge_image,ExceptionInfo *exception)
{
  (*edge_image)=MorphologyImage(image,ConvolveMorphology,1,kernel_info,
    exception);
  kernel_info=DestroyKernelInfo(kernel_info);
  if ((*edge_image) == (Image *) NULL)
    return(MagickFalse);
  if (SetCannyEdgeGray((*edge_image),exception) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static void UpdateCannyRange(const double intensity,double *min,double *max)
{
  if (intensity < (*min))
    (*min)=intensity;
  if (intensity > (*max))
    (*max)=intensity;
}

static MagickBooleanType ComputeCannyGradients(Image *edge_image,
  MatrixInfo *canny_cache,ExceptionInfo *exception)
{
  CacheView
    *edge_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  status=MagickTrue;
  edge_view=AcquireVirtualCacheView(edge_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(edge_image,edge_image,edge_image->rows,1)
#endif
  for (y=0; y < (ssize_t) edge_image->rows; y++)
  {
    ComputeCannyGradientRowInfo
      info;

    info.edge_view=edge_view;
    info.edge_image=edge_image;
    info.exception=exception;
    info.canny_cache=canny_cache;
    ComputeCannyGradientRow(&status,&info,y);
  }
  edge_view=DestroyCacheView(edge_view);
  return(status);
}

#define CannyEdgeImageTag  "CannyEdge/Image"

typedef struct _CannyHysteresisInputs
{
  const Image
    *image;

  Image
    *edge_image;

  MatrixInfo
    *canny_cache;

  double
    lower_percent,
    upper_percent,
    max,
    min;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ExceptionInfo
    *exception;
} CannyHysteresisInputs;

static Image *TraceCannyHysteresis(
  const CannyHysteresisInputs *hysteresis_inputs)
{
  const Image
    *const image = hysteresis_inputs->image;

  Image
    *const edge_image = hysteresis_inputs->edge_image;

  MatrixInfo
    *canny_cache = hysteresis_inputs->canny_cache;

  const double
    lower_percent = hysteresis_inputs->lower_percent,
    upper_percent = hysteresis_inputs->upper_percent,
    max = hysteresis_inputs->max,
    min = hysteresis_inputs->min;

  MagickBooleanType
    status = hysteresis_inputs->status;

  MagickOffsetType
    progress = hysteresis_inputs->progress;

  ExceptionInfo
    *const exception = hysteresis_inputs->exception;

  CacheView
    *edge_view;

  CannyTrace trace;

  double
    lower_threshold,
    upper_threshold;

  ssize_t
    y;

  /*
    Estimate hysteresis threshold.
  */
  lower_threshold=lower_percent*(max-min)+min;
  upper_threshold=upper_percent*(max-min)+min;
  /*
    Hysteresis threshold.
  */
  edge_view=AcquireAuthenticCacheView(edge_image,exception);
  trace.edge_image=edge_image;
  trace.edge_view=edge_view;
  trace.canny_cache=canny_cache;
  trace.lower_threshold=lower_threshold;

  for (y=0; y < (ssize_t) edge_image->rows; y++)
  {
    CannyHysteresis
      hysteresis;

    if (status == MagickFalse)
      continue;
    hysteresis.edge_image=edge_image;
    hysteresis.edge_view=edge_view;
    hysteresis.canny_cache=canny_cache;
    hysteresis.upper_threshold=upper_threshold;
    hysteresis.y=y;
    status=TraceCannyEdgeRow(&hysteresis,trace,status,exception);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetFeatureProgress(image,CannyEdgeImageTag,progress,&status);
      }
  }
  edge_view=DestroyCacheView(edge_view);
  /*
    Free resources.
  */
  canny_cache=DestroyMatrixInfo(canny_cache);
  return(edge_image);
}

static void LogCannyEdgeEvent(const Image *image)
{
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
}

typedef struct _CannyEdgeInputs
{
  const Image
    *image;

  Image
    *edge_image;

  MatrixInfo
    *canny_cache;

  double
    lower_percent,
    upper_percent;

  MagickBooleanType
    status;

  ExceptionInfo
    *exception;
} CannyEdgeInputs;

static Image *ThinCannyEdges(const CannyEdgeInputs *canny_inputs)
{
  Image
    *const edge_image = canny_inputs->edge_image;

  MatrixInfo
    *const canny_cache = canny_inputs->canny_cache;

  CacheView
    *edge_view;

  CannyHysteresisInputs
    hysteresis_inputs;

  CannyInfo
    element;

  double
    max,
    min;

  MagickBooleanType
    status = canny_inputs->status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Non-maxima suppression, remove pixels that are not considered to be part
    of an edge.
  */
  progress=0;
  (void) GetMatrixElement(canny_cache,0,0,&element);
  max=element.intensity;
  min=element.intensity;
  edge_view=AcquireAuthenticCacheView(edge_image,canny_inputs->exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(edge_image,edge_image,edge_image->rows,1)
#endif
  for (y=0; y < (ssize_t) edge_image->rows; y++)
  {
    const ComputeCannyGradientRowInfo
      info = { edge_view, edge_image, canny_inputs->exception, canny_cache };

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (GetCannyRow(&info,y,&q,&status) == MagickFalse)
      continue;
    for (x=0; x < (ssize_t) edge_image->columns; x++)
    {
      CannyInfo
        pixel;

      pixel=SuppressCannyNonMaxima(canny_cache,x,y);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp critical (MagickCore_CannyEdgeImage)
#endif
      {
        UpdateCannyRange(pixel.intensity,&min,&max);
      }
      *q=(Quantum) 0;
      q+=(ptrdiff_t) GetPixelChannels(edge_image);
    }
    SyncFeatureView(edge_view,canny_inputs->exception,&status);
  }
  edge_view=DestroyCacheView(edge_view);
  hysteresis_inputs.image=canny_inputs->image;
  hysteresis_inputs.edge_image=edge_image;
  hysteresis_inputs.canny_cache=canny_cache;
  hysteresis_inputs.lower_percent=canny_inputs->lower_percent;
  hysteresis_inputs.upper_percent=canny_inputs->upper_percent;
  hysteresis_inputs.max=max;
  hysteresis_inputs.min=min;
  hysteresis_inputs.status=status;
  hysteresis_inputs.progress=progress;
  hysteresis_inputs.exception=canny_inputs->exception;
  return(TraceCannyHysteresis(&hysteresis_inputs));
}

MagickExport Image *CannyEdgeImage(const Image *image,const double radius,
  const double sigma,const double lower_percent,const double upper_percent,
  ExceptionInfo *exception)
{
  CannyEdgeInputs
    canny_inputs;

  char
    geometry[MagickPathExtent];

  Image
    *edge_image;

  KernelInfo
    *kernel_info;

  MagickBooleanType
    status;

  MatrixInfo
    *canny_cache;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  LogCannyEdgeEvent(image);
  /*
    Filter out noise.
  */
  (void) FormatLocaleString(geometry,MagickPathExtent,
    "blur:%.17gx%.17g;blur:%.17gx%.17g+90",radius,sigma,radius,sigma);
  kernel_info=AcquireKernelInfo(geometry,exception);
  if (kernel_info == (KernelInfo *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  if (BlurCannyImage(image,kernel_info,&edge_image,exception) == MagickFalse)
    return((Image *) NULL);
  /*
    Find the intensity gradient of the image.
  */
  canny_cache=AcquireMatrixInfo(edge_image->columns,edge_image->rows,
    sizeof(CannyInfo),exception);
  if (canny_cache == (MatrixInfo *) NULL)
    {
      edge_image=DestroyImage(edge_image);
      return((Image *) NULL);
    }
  status=ComputeCannyGradients(edge_image,canny_cache,exception);
  canny_inputs.image=image;
  canny_inputs.edge_image=edge_image;
  canny_inputs.canny_cache=canny_cache;
  canny_inputs.lower_percent=lower_percent;
  canny_inputs.upper_percent=upper_percent;
  canny_inputs.status=status;
  canny_inputs.exception=exception;
  return(ThinCannyEdges(&canny_inputs));
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M e a n S h i f t I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MeanShiftImage() delineate arbitrarily shaped clusters in the image. For
%  each pixel, it visits all the pixels in the neighborhood specified by
%  the window centered at the pixel and excludes those that are outside the
%  radius=(window-1)/2 surrounding the pixel. From those pixels, it finds those
%  that are within the specified color distance from the current mean, and
%  computes a new x,y centroid from those coordinates and a new mean. This new
%  x,y centroid is used as the center for a new window. This process iterates
%  until it converges and the final mean is replaces the (original window
%  center) pixel value. It repeats this process for the next pixel, etc.,
%  until it processes all pixels in the image. Results are typically better with
%  colorspaces other than sRGB. We recommend YIQ, YUV or YCbCr.
%
%  The format of the MeanShiftImage method is:
%
%      Image *MeanShiftImage(const Image *image,const size_t width,
%        const size_t height,const double color_distance,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o width, height: find pixels in this neighborhood.
%
%    o color_distance: the color distance.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static double GetMeanShiftColorDistance(const PixelInfo mean_pixel,
  const PixelInfo pixel)
{
  double
    distance;

  distance=(mean_pixel.red-pixel.red)*(mean_pixel.red-pixel.red)+
    (mean_pixel.green-pixel.green)*(mean_pixel.green-pixel.green)+
    (mean_pixel.blue-pixel.blue)*(mean_pixel.blue-pixel.blue);
  return(distance);
}

static double GetMeanShiftDistance(const PointInfo mean_location,
  const PointInfo previous_location,const PixelInfo mean_pixel,
  const PixelInfo previous_pixel)
{
  double
    distance;

  distance=(mean_location.x-previous_location.x)*
    (mean_location.x-previous_location.x)+
    (mean_location.y-previous_location.y)*
    (mean_location.y-previous_location.y)+
    255.0*QuantumScale*(mean_pixel.red-previous_pixel.red)*
    255.0*QuantumScale*(mean_pixel.red-previous_pixel.red)+
    255.0*QuantumScale*(mean_pixel.green-previous_pixel.green)*
    255.0*QuantumScale*(mean_pixel.green-previous_pixel.green)+
    255.0*QuantumScale*(mean_pixel.blue-previous_pixel.blue)*
    255.0*QuantumScale*(mean_pixel.blue-previous_pixel.blue);
  return(distance);
}

static void SetMeanShiftPixel(const Image *mean_image,
  const PixelInfo mean_pixel,Quantum *magick_restrict q)
{
  SetPixelRed(mean_image,ClampToQuantum(mean_pixel.red),q);
  SetPixelGreen(mean_image,ClampToQuantum(mean_pixel.green),q);
  SetPixelBlue(mean_image,ClampToQuantum(mean_pixel.blue),q);
  SetPixelAlpha(mean_image,ClampToQuantum(mean_pixel.alpha),q);
}

typedef struct _MeanShiftInfo
{
  const Image
    *image;

  Image
    *mean_image;

  CacheView
    *image_view,
    *mean_view,
    *pixel_view;

  ExceptionInfo
    *exception;

  size_t
    width,
    height;

  double
    color_distance;

  ssize_t
    y;
} MeanShiftInfo;

typedef struct _MeanShiftTargets
{
  PointInfo
    *mean_location;

  PixelInfo
    *mean_pixel;
} MeanShiftTargets;

typedef struct _MeanShiftSum
{
  PointInfo
    sum_location,
    previous_location;

  PixelInfo
    sum_pixel,
    previous_pixel;

  ssize_t
    count;
} MeanShiftSum;

static MagickBooleanType MeanShiftUpdate(const MeanShiftSum *sum,
  PointInfo *mean_location,PixelInfo *mean_pixel)
{
  const PointInfo
    sum_location = sum->sum_location,
    previous_location = sum->previous_location;

  const PixelInfo
    sum_pixel = sum->sum_pixel,
    previous_pixel = sum->previous_pixel;

  const ssize_t
    count = sum->count;

  double
    distance,
    gamma = 1.0;

  if (count != 0)
    gamma=MagickSafeReciprocal((double) count);
  (*mean_location).x=gamma*sum_location.x;
  (*mean_location).y=gamma*sum_location.y;
  (*mean_pixel).red=gamma*sum_pixel.red;
  (*mean_pixel).green=gamma*sum_pixel.green;
  (*mean_pixel).blue=gamma*sum_pixel.blue;
  (*mean_pixel).alpha=gamma*sum_pixel.alpha;
  distance=GetMeanShiftDistance((*mean_location),previous_location,
    (*mean_pixel),previous_pixel);
  if (distance <= 3.0)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType MeanShiftStep(const MeanShiftInfo *shift,
  MagickBooleanType *status,const MeanShiftTargets *targets)
{
  const Image
    *const image = shift->image;

  CacheView
    *const pixel_view = shift->pixel_view;

  ExceptionInfo
    *const exception = shift->exception;

  const size_t
    width = shift->width,
    height = shift->height;

  const double
    color_distance = shift->color_distance;

  PointInfo
    *const mean_location = targets->mean_location;

  PixelInfo
    *const mean_pixel = targets->mean_pixel;

  double
    distance;

  PixelInfo
    previous_pixel,
    sum_pixel;

  PointInfo
    previous_location,
    sum_location;

  MeanShiftSum
    sum;

  ssize_t
    count,
    v;

  sum_location.x=0.0;
  sum_location.y=0.0;
  GetPixelInfo(image,&sum_pixel);
  previous_location=(*mean_location);
  previous_pixel=(*mean_pixel);
  count=0;
  for (v=(-((ssize_t) height/2)); v <= (((ssize_t) height/2)); v++)
  {
    ssize_t
      u;

    for (u=(-((ssize_t) width/2)); u <= (((ssize_t) width/2)); u++)
    {
      PixelInfo
        pixel;

      if ((v*v+u*u) > (ssize_t) ((width/2)*(height/2)))
        continue;
      (*status)=GetOneCacheViewVirtualPixelInfo(pixel_view,(ssize_t)
        MagickRound((*mean_location).x+u),(ssize_t) MagickRound(
        (*mean_location).y+v),&pixel,exception);
      distance=GetMeanShiftColorDistance((*mean_pixel),pixel);
      if (!(distance <= (color_distance*color_distance)))
        continue;
      sum_location.x+=(*mean_location).x+u;
      sum_location.y+=(*mean_location).y+v;
      sum_pixel.red+=pixel.red;
      sum_pixel.green+=pixel.green;
      sum_pixel.blue+=pixel.blue;
      sum_pixel.alpha+=pixel.alpha;
      count++;
    }
  }
  sum.sum_location=sum_location;
  sum.sum_pixel=sum_pixel;
  sum.count=count;
  sum.previous_location=previous_location;
  sum.previous_pixel=previous_pixel;
  return(MeanShiftUpdate(&sum,mean_location,mean_pixel));
}

static void MeanShiftRow(MagickBooleanType *status,const MeanShiftInfo *shift,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
#define MaxMeanShiftIterations  100

  const Image
    *const image = shift->image;

  Image
    *const mean_image = shift->mean_image;

  const ssize_t
    y = shift->y;

  ssize_t
    x;

  for (x=0; x < (ssize_t) mean_image->columns; x++)
  {
    PixelInfo
      mean_pixel;

    PointInfo
      mean_location;

    const MeanShiftTargets
      targets = { &mean_location, &mean_pixel };

    ssize_t
      i;

    GetPixelInfo(image,&mean_pixel);
    GetPixelInfoPixel(image,p,&mean_pixel);
    mean_location.x=(double) x;
    mean_location.y=(double) y;
    for (i=0; i < MaxMeanShiftIterations; i++)
    {
      if (MeanShiftStep(shift,status,&targets) == MagickFalse)
        break;
    }
    SetMeanShiftPixel(mean_image,mean_pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(mean_image);
  }
}

static void MeanShiftLine(MagickBooleanType *status,
  const MeanShiftInfo *shift,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  CacheView
    *const mean_view = shift->mean_view;

  ExceptionInfo
    *const exception = shift->exception;

  MeanShiftRow(status,shift,p,q);
  if (SyncCacheViewAuthenticPixels(mean_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

static MagickBooleanType AcquireMeanShiftImage(const Image *image,
  Image **mean_image,ExceptionInfo *exception)
{
  (*mean_image)=CloneImage(image,0,0,MagickTrue,exception);
  if ((*mean_image) == (Image *) NULL)
    return(MagickFalse);
  if (SetImageStorageClass((*mean_image),DirectClass,exception) == MagickFalse)
    {
      (*mean_image)=DestroyImage((*mean_image));
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType MeanShiftHead(const MeanShiftInfo *shift,
  MagickBooleanType *status)
{
  const Image
    *const image = shift->image;

  Image
    *const mean_image = shift->mean_image;

  CacheView
    *const image_view = shift->image_view,
    *const mean_view = shift->mean_view;

  ExceptionInfo
    *const exception = shift->exception;

  const ssize_t
    y = shift->y;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
  q=GetCacheViewAuthenticPixels(mean_view,0,y,mean_image->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  MeanShiftLine(status,shift,p,q);
  return(MagickTrue);
}

MagickExport Image *MeanShiftImage(const Image *image,const size_t width,
  const size_t height,const double color_distance,ExceptionInfo *exception)
{
#define MeanShiftImageTag  "MeanShift/Image"

  CacheView
    *image_view,
    *mean_view,
    *pixel_view;

  Image
    *mean_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (AcquireMeanShiftImage(image,&mean_image,exception) == MagickFalse)
    return((Image *) NULL);
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  pixel_view=AcquireVirtualCacheView(image,exception);
  mean_view=AcquireAuthenticCacheView(mean_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status,progress) \
    magick_number_threads(mean_image,mean_image,mean_image->rows,1)
#endif
  for (y=0; y < (ssize_t) mean_image->rows; y++)
  {
    MeanShiftInfo
      shift;

    shift.image=image;
    shift.mean_image=mean_image;
    shift.image_view=image_view;
    shift.mean_view=mean_view;
    shift.pixel_view=pixel_view;
    shift.exception=exception;
    shift.width=width;
    shift.height=height;
    shift.color_distance=color_distance;
    shift.y=y;
    if (MeanShiftHead(&shift,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetFeatureProgress(image,MeanShiftImageTag,progress,&status);
      }
  }
  mean_view=DestroyCacheView(mean_view);
  pixel_view=DestroyCacheView(pixel_view);
  image_view=DestroyCacheView(image_view);
  return(mean_image);
}
