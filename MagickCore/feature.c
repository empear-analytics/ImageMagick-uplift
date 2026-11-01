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

static MagickBooleanType TraceEdges(const CannyTrace *trace,const ssize_t x,
  const ssize_t y,ExceptionInfo *exception)
{
  Image
    *edge_image = trace->edge_image;

  CacheView
    *edge_view = trace->edge_view;

  MatrixInfo
    *canny_cache = trace->canny_cache;

  const double
    lower_threshold = trace->lower_threshold;

  CannyInfo
    edge,
    pixel;

  MagickBooleanType
    status;

  Quantum
    *q;

  ssize_t
    i;

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
  for (i=1; i != 0; )
  {
    ssize_t
      v;

    i--;
    status=GetMatrixElement(canny_cache,i,0,&edge);
    if (status == MagickFalse)
      return(MagickFalse);
    for (v=(-1); v <= 1; v++)
    {
      ssize_t
        u;

      for (u=(-1); u <= 1; u++)
      {
        if ((u == 0) && (v == 0))
          continue;
        if (IsAuthenticPixel(edge_image,edge.x+u,edge.y+v) == MagickFalse)
          continue;
        /*
          Not an edge if gradient value is below the lower threshold.
        */
        q=GetCacheViewAuthenticPixels(edge_view,edge.x+u,edge.y+v,1,1,
          exception);
        if (q == (Quantum *) NULL)
          return(MagickFalse);
        status=GetMatrixElement(canny_cache,edge.x+u,edge.y+v,&pixel);
        if (status == MagickFalse)
          return(MagickFalse);
        if (IsUntracedEdgePixel(edge_image,q,pixel,lower_threshold) !=
            MagickFalse)
          {
            *q=QuantumRange;
            status=SyncCacheViewAuthenticPixels(edge_view,exception);
            if (status == MagickFalse)
              return(MagickFalse);
            edge.x+=u;
            edge.y+=v;
            status=SetMatrixElement(canny_cache,i,0,&edge);
            if (status == MagickFalse)
              return(MagickFalse);
            i++;
          }
      }
    }
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

static void GetCannyNeighbors(const MatrixInfo *canny_cache,const ssize_t x,
  const ssize_t y,const int orientation,CannyInfo *alpha_pixel,
  CannyInfo *beta_pixel)
{
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

  (void) GetMatrixElement(canny_cache,x,y,&pixel);
  GetCannyNeighbors(canny_cache,x,y,pixel.orientation,&alpha_pixel,
    &beta_pixel);
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

MagickExport Image *CannyEdgeImage(const Image *image,const double radius,
  const double sigma,const double lower_percent,const double upper_percent,
  ExceptionInfo *exception)
{
#define CannyEdgeImageTag  "CannyEdge/Image"

  CacheView
    *edge_view;

  CannyInfo
    element;

  char
    geometry[MagickPathExtent];

  double
    lower_threshold,
    max,
    min,
    upper_threshold;

  Image
    *edge_image;

  KernelInfo
    *kernel_info;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  MatrixInfo
    *canny_cache;

  ssize_t
    y;

  CannyTrace trace;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  /*
    Filter out noise.
  */
  (void) FormatLocaleString(geometry,MagickPathExtent,
    "blur:%.17gx%.17g;blur:%.17gx%.17g+90",radius,sigma,radius,sigma);
  kernel_info=AcquireKernelInfo(geometry,exception);
  if (kernel_info == (KernelInfo *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  edge_image=MorphologyImage(image,ConvolveMorphology,1,kernel_info,exception);
  kernel_info=DestroyKernelInfo(kernel_info);
  if (edge_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetCannyEdgeGray(edge_image,exception) == MagickFalse)
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
  status=MagickTrue;
  edge_view=AcquireVirtualCacheView(edge_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(edge_image,edge_image,edge_image->rows,1)
#endif
  for (y=0; y < (ssize_t) edge_image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(edge_view,0,y,edge_image->columns+1,2,
      exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    SetCannyGradientRow(edge_image,canny_cache,p,y);
  }
  edge_view=DestroyCacheView(edge_view);
  /*
    Non-maxima suppression, remove pixels that are not considered to be part
    of an edge.
  */
  progress=0;
  (void) GetMatrixElement(canny_cache,0,0,&element);
  max=element.intensity;
  min=element.intensity;
  edge_view=AcquireAuthenticCacheView(edge_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(edge_image,edge_image,edge_image->rows,1)
#endif
  for (y=0; y < (ssize_t) edge_image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(edge_view,0,y,edge_image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) edge_image->columns; x++)
    {
      CannyInfo
        pixel;

      pixel=SuppressCannyNonMaxima(canny_cache,x,y);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp critical (MagickCore_CannyEdgeImage)
#endif
      {
        if (pixel.intensity < min)
          min=pixel.intensity;
        if (pixel.intensity > max)
          max=pixel.intensity;
      }
      *q=(Quantum) 0;
      q+=(ptrdiff_t) GetPixelChannels(edge_image);
    }
    if (SyncCacheViewAuthenticPixels(edge_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  edge_view=DestroyCacheView(edge_view);
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
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,CannyEdgeImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  edge_view=DestroyCacheView(edge_view);
  /*
    Free resources.
  */
  canny_cache=DestroyMatrixInfo(canny_cache);
  return(edge_image);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     H o u g h L i n e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  HoughLineImage() can be used in conjunction with any binary edge extracted
%  image (we recommend Canny) to identify lines in the image. The algorithm
%  accumulates counts for every white pixel for every possible orientation (for
%  angles from 0 to 179 in 1 degree increments) and distance from the center of
%  the image to the corner (in 1 px increments) and stores the counts in an
%  accumulator matrix of angle vs distance. The size of the accumulator is
%  180x(diagonal/2). Next it searches this space for peaks in counts and
%  converts the locations of the peaks to slope and intercept in the normal
%  x,y input image space. Use  the slope/intercepts to find the endpoints
%  clipped to the bounds of the image. The lines are then drawn. The counts
%  are a measure of the length of the lines.
%
%  The format of the HoughLineImage method is:
%
%      Image *HoughLineImage(const Image *image,const size_t width,
%        const size_t height,const size_t threshold,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o width, height: find line pairs as local maxima in this neighborhood.
%
%    o threshold: the line count threshold.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline double MagickRound(double x)
{
  /*
    Round the fraction to nearest integer.
  */
  if ((x-floor(x)) < (ceil(x)-x))
    return(floor(x));
  return(ceil(x));
}

static void CopyBlobToPrimitive(Image *image,DrawInfo *draw_info)
{
  draw_info->primitive=(char *) AcquireQuantumMemory(1,(size_t)
    GetBlobSize(image)+1);
  if (draw_info->primitive != (char *) NULL)
    {
      (void) memcpy(draw_info->primitive,GetBlobStreamData(image),
        (size_t) GetBlobSize(image));
      draw_info->primitive[GetBlobSize(image)]='\0';
    }
}

static Image *RenderHoughLines(const ImageInfo *image_info,const size_t columns,
  const size_t rows,ExceptionInfo *exception)
{
#define BoundingBox  "viewbox"

  DrawInfo
    *draw_info;

  Image
    *image;

  MagickBooleanType
    status;

  /*
    Open image.
  */
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  image->columns=columns;
  image->rows=rows;
  draw_info=CloneDrawInfo(image_info,(DrawInfo *) NULL);
  draw_info->affine.sx=image->resolution.x == 0.0 ? 1.0 : image->resolution.x/
    DefaultResolution;
  draw_info->affine.sy=image->resolution.y == 0.0 ? 1.0 : image->resolution.y/
    DefaultResolution;
  image->columns=CastDoubleToSizeT(draw_info->affine.sx*image->columns);
  image->rows=CastDoubleToSizeT(draw_info->affine.sy*image->rows);
  status=SetImageExtent(image,image->columns,image->rows,exception);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  if (SetImageBackgroundColor(image,exception) == MagickFalse)
    {
      draw_info=DestroyDrawInfo(draw_info);
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Render drawing.
  */
  if (GetBlobStreamData(image) == (unsigned char *) NULL)
    draw_info->primitive=FileToString(image->filename,~0UL,exception);
  else
    CopyBlobToPrimitive(image,draw_info);
  (void) DrawImage(image,draw_info,exception);
  draw_info=DestroyDrawInfo(draw_info);
  if (CloseBlob(image) == MagickFalse)
    image=DestroyImageList(image);
  return(GetFirstImageInList(image));
}

typedef struct _HoughCell
{
  ssize_t
    x,
    y;
} HoughCell;

typedef struct _HoughWindow
{
  size_t
    width,
    height;
} HoughWindow;

static void VoteHoughLines(MatrixInfo *accumulator,const PointInfo *center,
  const HoughCell *cell,const double hough_height)
{
  const ssize_t
    x = cell->x,
    y = cell->y;

  ssize_t
    i;

  for (i=0; i < 180; i++)
  {
    double
      count,
      radius;

    radius=(((double) x-center->x)*cos(DegreesToRadians((double) i)))+
      (((double) y-center->y)*sin(DegreesToRadians((double) i)));
    (void) GetMatrixElement(accumulator,i,(ssize_t)
      MagickRound(radius+hough_height),&count);
    count++;
    (void) SetMatrixElement(accumulator,i,(ssize_t)
      MagickRound(radius+hough_height),&count);
  }
}

static MagickBooleanType WriteHoughHeader(const Image *image,const int file,
  const HoughWindow *window,const size_t threshold)
{
  char
    message[MagickPathExtent];

  const size_t
    width = window->width,
    height = window->height;

  MagickBooleanType
    status;

  status=MagickTrue;
  (void) FormatLocaleString(message,MagickPathExtent,
    "# Hough line transform: %.17gx%.17g%+.20g\n",(double) width,
    (double) height,(double) threshold);
  if (write(file,message,strlen(message)) != (ssize_t) strlen(message))
    status=MagickFalse;
  (void) FormatLocaleString(message,MagickPathExtent,
    "viewbox 0 0 %.17g %.17g\n",(double) image->columns,(double) image->rows);
  if (write(file,message,strlen(message)) != (ssize_t) strlen(message))
    status=MagickFalse;
  (void) FormatLocaleString(message,MagickPathExtent,
    "# x1,y1 x2,y2 # count angle distance\n");
  if (write(file,message,strlen(message)) != (ssize_t) strlen(message))
    status=MagickFalse;
  return(status);
}

static double GetHoughMaxima(const MatrixInfo *accumulator,
  const HoughCell *cell,const HoughWindow *window,double count)
{
  const size_t
    width = window->width,
    height = window->height;

  const ssize_t
    x = cell->x,
    y = cell->y;

  double
    maxima;

  ssize_t
    v;

  maxima=count;
  for (v=(-((ssize_t) height/2)); v <= (((ssize_t) height/2)); v++)
  {
    ssize_t
      u;

    for (u=(-((ssize_t) width/2)); u <= (((ssize_t) width/2)); u++)
    {
      if ((u == 0) && (v == 0))
        continue;
      (void) GetMatrixElement(accumulator,x+u,y+v,&count);
      if (count > maxima)
        {
          maxima=count;
          break;
        }
    }
    if (u < (ssize_t) (width/2))
      break;
  }
  return(maxima);
}

static SegmentInfo GetHoughLine(const Image *image,const ssize_t x,
  const ssize_t y,const size_t accumulator_height)
{
  SegmentInfo
    line;

  if ((x >= 45) && (x <= 135))
    {
      /*
        y = (r-x cos(t))/sin(t)
      */
      line.x1=0.0;
      line.y1=((double) (y-(accumulator_height/2.0))-((line.x1-
        (image->columns/2.0))*cos(DegreesToRadians((double) x))))/
        sin(DegreesToRadians((double) x))+(image->rows/2.0);
      line.x2=(double) image->columns;
      line.y2=((double) (y-(accumulator_height/2.0))-((line.x2-
        (image->columns/2.0))*cos(DegreesToRadians((double) x))))/
        sin(DegreesToRadians((double) x))+(image->rows/2.0);
    }
  else
    {
      /*
        x = (r-y cos(t))/sin(t)
      */
      line.y1=0.0;
      line.x1=((double) (y-(accumulator_height/2.0))-((line.y1-
        (image->rows/2.0))*sin(DegreesToRadians((double) x))))/
        cos(DegreesToRadians((double) x))+(image->columns/2.0);
      line.y2=(double) image->rows;
      line.x2=((double) (y-(accumulator_height/2.0))-((line.y2-
        (image->rows/2.0))*sin(DegreesToRadians((double) x))))/
        cos(DegreesToRadians((double) x))+(image->columns/2.0);
    }
  return(line);
}

static void SetHoughDrawOptions(const Image *image,ImageInfo *image_info)
{
  const char
    *artifact;

  artifact=GetImageArtifact(image,"background");
  if (artifact != (const char *) NULL)
    (void) SetImageOption(image_info,"background",artifact);
  artifact=GetImageArtifact(image,"fill");
  if (artifact != (const char *) NULL)
    (void) SetImageOption(image_info,"fill",artifact);
  artifact=GetImageArtifact(image,"stroke");
  if (artifact != (const char *) NULL)
    (void) SetImageOption(image_info,"stroke",artifact);
  artifact=GetImageArtifact(image,"strokewidth");
  if (artifact != (const char *) NULL)
    (void) SetImageOption(image_info,"strokewidth",artifact);
}

typedef struct _HoughSegments
{
  MatrixInfo
    *accumulator;

  const Image
    *image;

  size_t
    accumulator_width,
    accumulator_height,
    line_count;

  int
    file;
} HoughSegments;

static MagickBooleanType WriteHoughLines(const HoughSegments *segments,
  const HoughWindow window,MagickBooleanType status)
{
  char
    message[MagickPathExtent];

  ssize_t
    y;

  for (y=0; y < (ssize_t) segments->accumulator_height; y++)
  {
    ssize_t
      x;

    for (x=0; x < (ssize_t) segments->accumulator_width; x++)
    {
      const HoughCell
        cell = { x, y };

      double
        count,
        maxima;

      SegmentInfo
        line;

      (void) GetMatrixElement(segments->accumulator,x,y,&count);
      if (!(count >= (double) segments->line_count))
        continue;
      /*
        Is point a local maxima?
      */
      maxima=GetHoughMaxima(segments->accumulator,&cell,&window,count);
      (void) GetMatrixElement(segments->accumulator,x,y,&count);
      if (maxima > count)
        continue;
      line=GetHoughLine(segments->image,x,y,segments->accumulator_height);
      (void) FormatLocaleString(message,MagickPathExtent,
        "line %g,%g %g,%g  # %g %g %g\n",line.x1,line.y1,line.x2,line.y2,
        maxima,(double) x,(double) y);
      if (write(segments->file,message,strlen(message)) !=
          (ssize_t) strlen(message))
        status=MagickFalse;
    }
  }
  return(status);
}

typedef struct _HoughVotes
{
  const Image
    *image;

  MatrixInfo
    *accumulator;

  double
    hough_height;

  ssize_t
    y;
} HoughVotes;

static void VoteHoughRow(const HoughVotes *votes,
  const Quantum *magick_restrict p,PointInfo center)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) votes->image->columns; x++)
  {
    const HoughCell
      cell = { x, votes->y };

    if (GetPixelIntensity(votes->image,p) > ((double) QuantumRange/2.0))
      VoteHoughLines(votes->accumulator,&center,&cell,votes->hough_height);
    p+=(ptrdiff_t) GetPixelChannels(votes->image);
  }
}

static Image *AppendHoughAccumulator(Image *lines_image,
  MatrixInfo *accumulator,const char *artifact,ExceptionInfo *exception)
{
  if ((lines_image != (Image *) NULL) &&
      (IsStringTrue(artifact) != MagickFalse))
    {
      Image
        *accumulator_image;

      accumulator_image=MatrixToImage(accumulator,exception);
      if (accumulator_image != (Image *) NULL)
        AppendImageToList(&lines_image,accumulator_image);
    }
  return(lines_image);
}

MagickExport Image *HoughLineImage(const Image *image,const size_t width,
  const size_t height,const size_t threshold,ExceptionInfo *exception)
{
#define HoughLineImageTag  "HoughLine/Image"

  CacheView
    *image_view;

  char
    path[MagickPathExtent];

  const char
    *artifact;

  double
    hough_height;

  const HoughWindow
    window = { width, height };

  HoughSegments
    segments;

  Image
    *lines_image = NULL;

  ImageInfo
    *image_info;

  int
    file;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  MatrixInfo
    *accumulator;

  PointInfo
    center;

  ssize_t
    y;

  size_t
    accumulator_height,
    accumulator_width,
    line_count;

  /*
    Create the accumulator.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  accumulator_width=180;
  hough_height=((sqrt(2.0)*(double) (image->rows > image->columns ?
    image->rows : image->columns))/2.0);
  accumulator_height=(size_t) (2.0*hough_height);
  accumulator=AcquireMatrixInfo(accumulator_width,accumulator_height,
    sizeof(double),exception);
  if (accumulator == (MatrixInfo *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  if (NullMatrix(accumulator) == MagickFalse)
    {
      accumulator=DestroyMatrixInfo(accumulator);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Populate the accumulator.
  */
  status=MagickTrue;
  progress=0;
  center.x=(double) image->columns/2.0;
  center.y=(double) image->rows/2.0;
  image_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    HoughVotes
      votes;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    votes.image=image;
    votes.accumulator=accumulator;
    votes.hough_height=hough_height;
    votes.y=y;
    VoteHoughRow(&votes,p,center);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,CannyEdgeImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  if (status == MagickFalse)
    {
      accumulator=DestroyMatrixInfo(accumulator);
      return((Image *) NULL);
    }
  /*
    Generate line segments from accumulator.
  */
  file=AcquireUniqueFileResource(path);
  if (file == -1)
    {
      accumulator=DestroyMatrixInfo(accumulator);
      return((Image *) NULL);
    }
  status=WriteHoughHeader(image,file,&window,threshold);
  line_count=image->columns > image->rows ? image->columns/4 : image->rows/4;
  if (threshold != 0)
    line_count=threshold;
  segments.accumulator=accumulator;
  segments.image=image;
  segments.accumulator_width=accumulator_width;
  segments.accumulator_height=accumulator_height;
  segments.line_count=line_count;
  segments.file=file;
  status=WriteHoughLines(&segments,window,status);
  (void) close_utf8(file);
  /*
    Render lines to image canvas.
  */
  image_info=AcquireImageInfo();
  image_info->background_color=image->background_color;
  (void) FormatLocaleString(image_info->filename,MagickPathExtent,"%s",path);
  SetHoughDrawOptions(image,image_info);
  lines_image=RenderHoughLines(image_info,image->columns,image->rows,exception);
  artifact=GetImageArtifact(image,"hough-lines:accumulator");
  lines_image=AppendHoughAccumulator(lines_image,accumulator,artifact,
    exception);
  /*
    Free resources.
  */
  accumulator=DestroyMatrixInfo(accumulator);
  image_info=DestroyImageInfo(image_info);
  (void) RelinquishUniqueFileResource(path);
  return(GetFirstImageInList(lines_image));
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

MagickExport Image *MeanShiftImage(const Image *image,const size_t width,
  const size_t height,const double color_distance,ExceptionInfo *exception)
{
#define MaxMeanShiftIterations  100
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
  mean_image=CloneImage(image,0,0,MagickTrue,exception);
  if (mean_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(mean_image,DirectClass,exception) == MagickFalse)
    {
      mean_image=DestroyImage(mean_image);
      return((Image *) NULL);
    }
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
    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    q=GetCacheViewAuthenticPixels(mean_view,0,y,mean_image->columns,1,
      exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) mean_image->columns; x++)
    {
      PixelInfo
        mean_pixel,
        previous_pixel;

      PointInfo
        mean_location,
        previous_location;

      ssize_t
        i;

      GetPixelInfo(image,&mean_pixel);
      GetPixelInfoPixel(image,p,&mean_pixel);
      mean_location.x=(double) x;
      mean_location.y=(double) y;
      for (i=0; i < MaxMeanShiftIterations; i++)
      {
        double
          distance,
          gamma = 1.0;

        PixelInfo
          sum_pixel;

        PointInfo
          sum_location;

        ssize_t
          count,
          v;

        sum_location.x=0.0;
        sum_location.y=0.0;
        GetPixelInfo(image,&sum_pixel);
        previous_location=mean_location;
        previous_pixel=mean_pixel;
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
            status=GetOneCacheViewVirtualPixelInfo(pixel_view,(ssize_t)
              MagickRound(mean_location.x+u),(ssize_t) MagickRound(
              mean_location.y+v),&pixel,exception);
            distance=GetMeanShiftColorDistance(mean_pixel,pixel);
            if (!(distance <= (color_distance*color_distance)))
              continue;
            sum_location.x+=mean_location.x+u;
            sum_location.y+=mean_location.y+v;
            sum_pixel.red+=pixel.red;
            sum_pixel.green+=pixel.green;
            sum_pixel.blue+=pixel.blue;
            sum_pixel.alpha+=pixel.alpha;
            count++;
          }
        }
        if (count != 0)
          gamma=MagickSafeReciprocal((double) count);
        mean_location.x=gamma*sum_location.x;
        mean_location.y=gamma*sum_location.y;
        mean_pixel.red=gamma*sum_pixel.red;
        mean_pixel.green=gamma*sum_pixel.green;
        mean_pixel.blue=gamma*sum_pixel.blue;
        mean_pixel.alpha=gamma*sum_pixel.alpha;
        distance=GetMeanShiftDistance(mean_location,previous_location,
          mean_pixel,previous_pixel);
        if (distance <= 3.0)
          break;
      }
      SetMeanShiftPixel(mean_image,mean_pixel,q);
      p+=(ptrdiff_t) GetPixelChannels(image);
      q+=(ptrdiff_t) GetPixelChannels(mean_image);
    }
    if (SyncCacheViewAuthenticPixels(mean_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,MeanShiftImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  mean_view=DestroyCacheView(mean_view);
  pixel_view=DestroyCacheView(pixel_view);
  image_view=DestroyCacheView(image_view);
  return(mean_image);
}
