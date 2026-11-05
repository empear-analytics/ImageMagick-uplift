/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      PPPP    AAA   IIIII  N   N  TTTTT                      %
%                      P   P  A   A    I    NN  N    T                        %
%                      PPPP   AAAAA    I    N N N    T                        %
%                      P      A   A    I    N  NN    T                        %
%                      P      A   A  IIIII  N   N    T                        %
%                                                                             %
%                                                                             %
%                        Methods to Paint on an Image                         %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1998                                   %
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

/*
 Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/draw.h"
#include "MagickCore/draw-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/resource_.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     G r a d i e n t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GradientImage() applies a continuously smooth color transitions along a
%  vector from one color to another.
%
%  Note, the interface of this method will change in the future to support
%  more than one transition.
%
%  The format of the GradientImage method is:
%
%      MagickBooleanType GradientImage(Image *image,const GradientType type,
%        const SpreadMethod method,const PixelInfo *start_color,
%        const PixelInfo *stop_color,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o type: the gradient type: linear or radial.
%
%    o spread: the gradient spread method: pad, reflect, or repeat.
%
%    o start_color: the start color.
%
%    o stop_color: the stop color.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsGradientDefaultDirection(const Image *image)
{
  return(((GetImageArtifact(image,"gradient:angle") == (const char *) NULL) &&
    (GetImageArtifact(image,"gradient:direction") == (const char *) NULL) &&
    (GetImageArtifact(image,"gradient:extent") == (const char *) NULL) &&
    (GetImageArtifact(image,"gradient:vector") ==
    (const char *) NULL)) ? MagickTrue : MagickFalse);
}

static void SetGradientSouthDirection(GradientInfo *gradient,
  const Image *image,const GravityType direction)
{
  switch (direction)
  {
    case SouthWestGravity:
    {
      gradient->gradient_vector.x1=(double) image->columns-1;
      gradient->gradient_vector.y1=0.0;
      gradient->gradient_vector.x2=0.0;
      gradient->gradient_vector.y2=(double) image->rows-1;
      break;
    }
    case SouthGravity:
    {
      gradient->gradient_vector.x1=0.0;
      gradient->gradient_vector.y1=0.0;
      gradient->gradient_vector.x2=0.0;
      gradient->gradient_vector.y2=(double) image->rows-1;
      break;
    }
    case SouthEastGravity:
    {
      gradient->gradient_vector.x1=0.0;
      gradient->gradient_vector.y1=0.0;
      gradient->gradient_vector.x2=(double) image->columns-1;
      gradient->gradient_vector.y2=(double) image->rows-1;
      break;
    }
    default:
      break;
  }
}

static void SetGradientDirection(GradientInfo *gradient,const Image *image,
  const char *artifact)
{
  GravityType
    direction;

  direction=(GravityType) ParseCommandOption(MagickGravityOptions,
    MagickFalse,artifact);
  switch (direction)
  {
    case NorthWestGravity:
    {
      gradient->gradient_vector.x1=(double) image->columns-1;
      gradient->gradient_vector.y1=(double) image->rows-1;
      gradient->gradient_vector.x2=0.0;
      gradient->gradient_vector.y2=0.0;
      break;
    }
    case NorthGravity:
    {
      gradient->gradient_vector.x1=0.0;
      gradient->gradient_vector.y1=(double) image->rows-1;
      gradient->gradient_vector.x2=0.0;
      gradient->gradient_vector.y2=0.0;
      break;
    }
    case NorthEastGravity:
    {
      gradient->gradient_vector.x1=0.0;
      gradient->gradient_vector.y1=(double) image->rows-1;
      gradient->gradient_vector.x2=(double) image->columns-1;
      gradient->gradient_vector.y2=0.0;
      break;
    }
    case WestGravity:
    {
      gradient->gradient_vector.x1=(double) image->columns-1;
      gradient->gradient_vector.y1=0.0;
      gradient->gradient_vector.x2=0.0;
      gradient->gradient_vector.y2=0.0;
      break;
    }
    case EastGravity:
    {
      gradient->gradient_vector.x1=0.0;
      gradient->gradient_vector.y1=0.0;
      gradient->gradient_vector.x2=(double) image->columns-1;
      gradient->gradient_vector.y2=0.0;
      break;
    }
    default:
    {
      SetGradientSouthDirection(gradient,image,direction);
      break;
    }
  }
}

static void SetGradientAngleVector(GradientInfo *gradient,const Image *image)
{
  double
    sine,
    cosine,
    distance;

  /*
    Reference https://drafts.csswg.org/css-images-3/#linear-gradients.
  */
  sine=sin((double) DegreesToRadians(gradient->angle-90.0));
  cosine=cos((double) DegreesToRadians(gradient->angle-90.0));
  distance=fabs((double) (image->columns-1.0)*cosine)+
    fabs((double) (image->rows-1.0)*sine);
  gradient->gradient_vector.x1=0.5*((image->columns-1.0)-distance*cosine);
  gradient->gradient_vector.y1=0.5*((image->rows-1.0)-distance*sine);
  gradient->gradient_vector.x2=0.5*((image->columns-1.0)+distance*cosine);
  gradient->gradient_vector.y2=0.5*((image->rows-1.0)+distance*sine);
}

static void SetGradientExtent(GradientInfo *gradient,const Image *image,
  const char *artifact)
{
  if (LocaleCompare(artifact,"Circle") == 0)
    {
      gradient->radii.x=(double) MagickMax((image->columns-1.0),
        (image->rows-1.0))/2.0;
      gradient->radii.y=gradient->radii.x;
    }
  if (LocaleCompare(artifact,"Diagonal") == 0)
    {
      gradient->radii.x=(double) (sqrt((double) (image->columns-1.0)*
        (image->columns-1.0)+(image->rows-1.0)*(image->rows-1.0)))/2.0;
      gradient->radii.y=gradient->radii.x;
    }
  if (LocaleCompare(artifact,"Ellipse") == 0)
    {
      gradient->radii.x=(double) (image->columns-1.0)/2.0;
      gradient->radii.y=(double) (image->rows-1.0)/2.0;
    }
  if (LocaleCompare(artifact,"Maximum") == 0)
    {
      gradient->radii.x=(double) MagickMax((image->columns-1.0),
        (image->rows-1.0))/2.0;
      gradient->radii.y=gradient->radii.x;
    }
  if (LocaleCompare(artifact,"Minimum") == 0)
    {
      gradient->radii.x=(double) (MagickMin((image->columns-1.0),
        (image->rows-1.0)))/2.0;
      gradient->radii.y=gradient->radii.x;
    }
}

static void SetGradientRadii(GradientInfo *gradient,const Image *image)
{
  const char
    *artifact;

  gradient->radii.x=(double) MagickMax((image->columns-1.0),(image->rows-1.0))/
    2.0;
  gradient->radii.y=gradient->radii.x;
  artifact=GetImageArtifact(image,"gradient:extent");
  if (artifact != (const char *) NULL)
    SetGradientExtent(gradient,image,artifact);
  artifact=GetImageArtifact(image,"gradient:radii");
  if (artifact != (const char *) NULL)
    (void) MagickSscanf(artifact,"%lf%*[ ,]%lf",&gradient->radii.x,
      &gradient->radii.y);
  gradient->radius=MagickMax(gradient->radii.x,gradient->radii.y);
}

static void SetGradientVector(GradientInfo *gradient,const Image *image,
  const GradientType type)
{
  const char
    *artifact;

  gradient->gradient_vector.x2=(double) image->columns-1;
  gradient->gradient_vector.y2=(double) image->rows-1;
  artifact=GetImageArtifact(image,"gradient:direction");
  if (artifact != (const char *) NULL)
    SetGradientDirection(gradient,image,artifact);
  artifact=GetImageArtifact(image,"gradient:angle");
  if (artifact != (const char *) NULL)
    gradient->angle=StringToDouble(artifact,(char **) NULL);
  artifact=GetImageArtifact(image,"gradient:vector");
  if (artifact != (const char *) NULL)
    (void) MagickSscanf(artifact,"%lf%*[ ,]%lf%*[ ,]%lf%*[ ,]%lf",
      &gradient->gradient_vector.x1,&gradient->gradient_vector.y1,
      &gradient->gradient_vector.x2,&gradient->gradient_vector.y2);
  if (IsGradientDefaultDirection(image) != MagickFalse)
    if ((type == LinearGradient) && (gradient->gradient_vector.y2 != 0.0))
      gradient->gradient_vector.x2=0.0;
}

MagickExport MagickBooleanType GradientImage(Image *image,
  const GradientType type,const SpreadMethod method,const StopInfo *stops,
  const size_t number_stops,ExceptionInfo *exception)
{
  const char
    *artifact;

  DrawInfo
    *draw_info;

  GradientInfo
    *gradient;

  MagickBooleanType
    status;

  /*
    Set gradient start-stop end points.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(stops != (const StopInfo *) NULL);
  assert(number_stops > 0);
  if (IsEventLogging() != MagickFalse) 
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  draw_info=AcquireDrawInfo();
  gradient=(&draw_info->gradient);
  gradient->type=type;
  gradient->bounding_box.width=image->columns;
  gradient->bounding_box.height=image->rows;
  artifact=GetImageArtifact(image,"gradient:bounding-box");
  if (artifact != (const char *) NULL)
    (void) ParseAbsoluteGeometry(artifact,&gradient->bounding_box);
  SetGradientVector(gradient,image,type);
  gradient->center.x=(double) gradient->gradient_vector.x2/2.0;
  gradient->center.y=(double) gradient->gradient_vector.y2/2.0;
  artifact=GetImageArtifact(image,"gradient:center");
  if (artifact != (const char *) NULL)
    (void) MagickSscanf(artifact,"%lf%*[ ,]%lf",&gradient->center.x,
      &gradient->center.y);
  artifact=GetImageArtifact(image,"gradient:angle");
  if ((type == LinearGradient) && (artifact != (const char *) NULL))
    SetGradientAngleVector(gradient,image);
  SetGradientRadii(gradient,image);
  gradient->spread=method;
  /*
    Define the gradient to fill between the stops.
  */
  gradient->number_stops=number_stops;
  gradient->stops=(StopInfo *) AcquireQuantumMemory(gradient->number_stops,
    sizeof(*gradient->stops));
  if (gradient->stops == (StopInfo *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  (void) memcpy(gradient->stops,stops,(size_t) number_stops*sizeof(*stops));
  /*
    Draw a gradient on the image.
  */
  status=DrawGradientImage(image,draw_info,exception);
  draw_info=DestroyDrawInfo(draw_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     O i l P a i n t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OilPaintImage() applies a special effect filter that simulates an oil
%  painting.  Each pixel is replaced by the most frequent color occurring
%  in a circular region defined by radius.
%
%  The format of the OilPaintImage method is:
%
%      Image *OilPaintImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the circular neighborhood.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static size_t **DestroyHistogramTLS(size_t **histogram)
{
  ssize_t
    i;

  assert(histogram != (size_t **) NULL);
  for (i=0; i < (ssize_t) GetMagickResourceLimit(ThreadResource); i++)
    if (histogram[i] != (size_t *) NULL)
      histogram[i]=(size_t *) RelinquishMagickMemory(histogram[i]);
  histogram=(size_t **) RelinquishMagickMemory(histogram);
  return(histogram);
}

static size_t **AcquireHistogramTLS(const size_t count)
{
  ssize_t
    i;

  size_t
    **histogram,
    number_threads;

  number_threads=(size_t) GetMagickResourceLimit(ThreadResource);
  histogram=(size_t **) AcquireQuantumMemory(number_threads,sizeof(*histogram));
  if (histogram == (size_t **) NULL)
    return((size_t **) NULL);
  (void) memset(histogram,0,number_threads*sizeof(*histogram));
  for (i=0; i < (ssize_t) number_threads; i++)
  {
    histogram[i]=(size_t *) AcquireQuantumMemory(count,sizeof(**histogram));
    if (histogram[i] == (size_t *) NULL)
      return(DestroyHistogramTLS(histogram));
  }
  return(histogram);
}

#define NumberPaintBins  256
#define OilPaintImageTag  "OilPaint/Image"

static ssize_t GetMostFrequentOffset(const Image *linear_image,
  const Quantum *magick_restrict p,size_t *histogram,const size_t width)
{
  size_t
    count;

  ssize_t
    j,
    k,
    n,
    u,
    v;

  k=0;
  j=0;
  count=0;
  (void) memset(histogram,0,NumberPaintBins* sizeof(*histogram));
  for (v=0; v < (ssize_t) width; v++)
  {
    for (u=0; u < (ssize_t) width; u++)
    {
      n=(ssize_t) ScaleQuantumToChar(ClampToQuantum(GetPixelIntensity(
        linear_image,p+(ssize_t) GetPixelChannels(linear_image)*(u+k))));
      histogram[n]++;
      if (histogram[n] > count)
        {
          j=k+u;
          count=histogram[n];
        }
    }
    k+=(ssize_t) (linear_image->columns+width);
  }
  return(j);
}

typedef struct _OilPaintImages
{
  const Image
    *linear_image,
    *paint_image;

  ssize_t
    center;
} OilPaintImages;

static inline void SetOilPaintChannels(const OilPaintImages *images,
  const Quantum *magick_restrict p,Quantum *magick_restrict q,const ssize_t j)
{
  const Image
    *linear_image = images->linear_image,
    *paint_image = images->paint_image;

  const ssize_t
    center = images->center;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(linear_image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(linear_image,i);
    PixelTrait traits = GetPixelChannelTraits(linear_image,channel);
    PixelTrait paint_traits=GetPixelChannelTraits(paint_image,channel);
    if ((traits == UndefinedPixelTrait) ||
        (paint_traits == UndefinedPixelTrait))
      continue;
    if ((paint_traits & CopyPixelTrait) != 0)
      {
        SetPixelChannel(paint_image,channel,p[center+i],q);
        continue;
      }
    SetPixelChannel(paint_image,channel,p[j*(ssize_t)
      GetPixelChannels(linear_image)+i],q);
  }
}

static Image *DestroyOilPaintClones(Image *linear_image,Image *paint_image)
{
  if (linear_image != (Image *) NULL)
    linear_image=DestroyImage(linear_image);
  if (paint_image != (Image *) NULL)
    linear_image=DestroyImage(paint_image);
  return((Image *) NULL);
}

typedef struct _OilPaintRowInfo
{
  CacheView
    *image_view,
    *paint_view;

  ExceptionInfo
    *exception;

  Image
    *linear_image,
    *paint_image;

  size_t
    *histogram,
    width;

  ssize_t
    center;
} OilPaintRowInfo;

static void OilPaintRow(const OilPaintRowInfo *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) row->linear_image->columns; x++)
  {
    OilPaintImages
      images;

    ssize_t
      j;

    /*
      Assign most frequent color.
    */
    j=GetMostFrequentOffset(row->linear_image,p,row->histogram,row->width);
    images.linear_image=row->linear_image;
    images.paint_image=row->paint_image;
    images.center=row->center;
    SetOilPaintChannels(&images,p,q,j);
    p+=(ptrdiff_t) GetPixelChannels(row->linear_image);
    q+=(ptrdiff_t) GetPixelChannels(row->paint_image);
  }
}

static MagickBooleanType PrepareOilPaintImage(Image *linear_image,
  Image *paint_image,ExceptionInfo *exception)
{
  if (SetImageStorageClass(paint_image,DirectClass,exception) == MagickFalse)
    {
      linear_image=DestroyImage(linear_image);
      paint_image=DestroyImage(paint_image);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType OilPaintLine(const OilPaintRowInfo *row,
  const ssize_t y,MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(row->image_view,-((ssize_t) row->width/2L),y-
    (ssize_t) (row->width/2L),row->linear_image->columns+row->width,row->width,
    row->exception);
  q=QueueCacheViewAuthenticPixels(row->paint_view,0,y,row->paint_image->columns,
    1,row->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  OilPaintRow(row,p,q);
  if (SyncCacheViewAuthenticPixels(row->paint_view,row->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

typedef struct _PaintRows
{
  const Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;

  ssize_t
    y;
} PaintRows;

static MagickBooleanType GetPaintRow(const PaintRows *rows,
  Quantum *magick_restrict *q,MagickBooleanType *status)
{
  if ((*status) == MagickFalse)
    return(MagickFalse);
  (*q)=GetCacheViewAuthenticPixels(rows->image_view,0,rows->y,
    rows->image->columns,1,rows->exception);
  if ((*q) == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void SyncPaintRow(CacheView *view,ExceptionInfo *exception,
  MagickBooleanType *status)
{
  if (SyncCacheViewAuthenticPixels(view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

static void SetPaintProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

typedef struct _OilPaintRows
{
  Image
    *linear_image,
    *paint_image;

  size_t
    **histograms,
    width;

  ExceptionInfo
    *exception;
} OilPaintRows;

static MagickBooleanType OilPaintImageRows(const OilPaintRows *rows)
{
  CacheView
    *image_view,
    *paint_view;

  Image
    *const linear_image = rows->linear_image,
    *const paint_image = rows->paint_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  size_t
    **const histograms = rows->histograms;

  const size_t
    width = rows->width;

  ExceptionInfo
    *const exception = rows->exception;

  ssize_t
    center,
    y;

  /*
    Oil paint image.
  */
  status=MagickTrue;
  progress=0;
  center=(ssize_t) (GetPixelChannels(linear_image)*(linear_image->columns+
    width)*(width/2L)+GetPixelChannels(linear_image)*(width/2L));
  image_view=AcquireVirtualCacheView(linear_image,exception);
  paint_view=AcquireAuthenticCacheView(paint_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(linear_image,paint_image,linear_image->rows,1)
#endif
  for (y=0; y < (ssize_t) linear_image->rows; y++)
  {
    OilPaintRowInfo
      row;

    row.image_view=image_view;
    row.paint_view=paint_view;
    row.exception=exception;
    row.linear_image=linear_image;
    row.paint_image=paint_image;
    row.histogram=histograms[GetOpenMPThreadId()];
    row.width=width;
    row.center=center;
    if (OilPaintLine(&row,y,&status) == MagickFalse)
      continue;
    if (linear_image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetPaintProgress(linear_image,OilPaintImageTag,progress,&status);
      }
  }
  paint_view=DestroyCacheView(paint_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

MagickExport Image *OilPaintImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  Image
    *linear_image,
    *paint_image;

  MagickBooleanType
    status;

  OilPaintRows
    rows;

  size_t
    **histograms,
    width;

  /*
    Initialize painted image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse) 
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  width=GetOptimalKernelWidth2D(radius,sigma);
  linear_image=CloneImage(image,0,0,MagickTrue,exception);
  paint_image=CloneImage(image,0,0,MagickTrue,exception);
  if ((linear_image == (Image *) NULL) || (paint_image == (Image *) NULL))
    return(DestroyOilPaintClones(linear_image,paint_image));
  if (PrepareOilPaintImage(linear_image,paint_image,exception) == MagickFalse)
    return((Image *) NULL);
  histograms=AcquireHistogramTLS(NumberPaintBins);
  if (histograms == (size_t **) NULL)
    {
      linear_image=DestroyImage(linear_image);
      paint_image=DestroyImage(paint_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Oil paint image.
  */
  rows.linear_image=linear_image;
  rows.paint_image=paint_image;
  rows.histograms=histograms;
  rows.width=width;
  rows.exception=exception;
  status=OilPaintImageRows(&rows);
  histograms=DestroyHistogramTLS(histograms);
  linear_image=DestroyImage(linear_image);
  if (status == MagickFalse)
    paint_image=DestroyImage(paint_image);
  return(paint_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     O p a q u e P a i n t I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OpaquePaintImage() changes any pixel that matches color with the color
%  defined by fill argument.
%
%  By default color must match a particular pixel color exactly.  However, in
%  many cases two colors may differ by a small amount.  Fuzz defines how much
%  tolerance is acceptable to consider two colors as the same.  For example,
%  set fuzz to 10 and the color red at intensities of 100 and 102 respectively
%  are now interpreted as the same color.
%
%  The format of the OpaquePaintImage method is:
%
%      MagickBooleanType OpaquePaintImage(Image *image,const PixelInfo *target,
%        const PixelInfo *fill,const MagickBooleanType invert,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o target: the RGB value of the target color.
%
%    o fill: the replacement color.
%
%    o invert: paint any pixel that does not match the target color.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline void SetUpdatableFillChannels(const Image *image,
  const PixelInfo *conform_fill,Quantum *magick_restrict q)
{
  PixelTrait
    traits;

  traits=GetPixelChannelTraits(image,RedPixelChannel);
  if ((traits & UpdatePixelTrait) != 0)
    SetPixelRed(image,(Quantum) conform_fill->red,q);
  traits=GetPixelChannelTraits(image,GreenPixelChannel);
  if ((traits & UpdatePixelTrait) != 0)
    SetPixelGreen(image,(Quantum) conform_fill->green,q);
  traits=GetPixelChannelTraits(image,BluePixelChannel);
  if ((traits & UpdatePixelTrait) != 0)
    SetPixelBlue(image,(Quantum) conform_fill->blue,q);
  traits=GetPixelChannelTraits(image,BlackPixelChannel);
  if ((traits & UpdatePixelTrait) != 0)
    SetPixelBlack(image,(Quantum) conform_fill->black,q);
  traits=GetPixelChannelTraits(image,AlphaPixelChannel);
  if ((traits & UpdatePixelTrait) != 0)
    SetPixelAlpha(image,(Quantum) conform_fill->alpha,q);
}

typedef struct _OpaquePaintRow
{
  Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;

  MagickBooleanType
    invert;

  PixelInfo
    conform_fill,
    conform_target,
    zero;
} OpaquePaintRow;

static void OpaquePaintRowPixels(const OpaquePaintRow *row,
  Quantum *magick_restrict q)
{
  Image
    *image = row->image;

  const MagickBooleanType
    invert = row->invert;

  const PixelInfo
    conform_fill = row->conform_fill,
    conform_target = row->conform_target,
    zero = row->zero;

  PixelInfo
    pixel;

  ssize_t
    x;

  pixel=zero;
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    GetPixelInfoPixel(image,q,&pixel);
    if (IsFuzzyEquivalencePixelInfo(&pixel,&conform_target) != invert)
      SetUpdatableFillChannels(image,&conform_fill,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType OpaquePaintLine(const OpaquePaintRow *row,
  const ssize_t y,MagickBooleanType *status)
{
  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(row->image_view,0,y,row->image->columns,1,
    row->exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  OpaquePaintRowPixels(row,q);
  if (SyncCacheViewAuthenticPixels(row->image_view,row->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

MagickExport MagickBooleanType OpaquePaintImage(Image *image,
  const PixelInfo *target,const PixelInfo *fill,const MagickBooleanType invert,
  ExceptionInfo *exception)
{
#define OpaquePaintImageTag  "Opaque/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    conform_fill,
    conform_target,
    zero;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(target != (PixelInfo *) NULL);
  assert(fill != (PixelInfo *) NULL);
  if (IsEventLogging() != MagickFalse) 
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  ConformPixelInfo(image,fill,&conform_fill,exception);
  ConformPixelInfo(image,target,&conform_target,exception);
  /*
    Make image color opaque.
  */
  status=MagickTrue;
  progress=0;
  GetPixelInfo(image,&zero);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    OpaquePaintRow
      row;

    row.image=image;
    row.image_view=image_view;
    row.exception=exception;
    row.invert=invert;
    row.conform_fill=conform_fill;
    row.conform_target=conform_target;
    row.zero=zero;
    if (OpaquePaintLine(&row,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetPaintProgress(image,OpaquePaintImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     T r a n s p a r e n t P a i n t I m a g e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransparentPaintImage() changes the opacity value associated with any pixel
%  that matches color to the value defined by opacity.
%
%  By default color must match a particular pixel color exactly.  However, in
%  many cases two colors may differ by a small amount.  Fuzz defines how much
%  tolerance is acceptable to consider two colors as the same.  For example,
%  set fuzz to 10 and the color red at intensities of 100 and 102 respectively
%  are now interpreted as the same color.
%
%  The format of the TransparentPaintImage method is:
%
%      MagickBooleanType TransparentPaintImage(Image *image,
%        const PixelInfo *target,const Quantum opacity,
%        const MagickBooleanType invert,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o target: the target color.
%
%    o opacity: the replacement opacity value.
%
%    o invert: paint any pixel that does not match the target color.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _TransparentPaintRow
{
  Image
    *image;

  const PixelInfo
    *target;

  Quantum
    opacity;

  MagickBooleanType
    invert;

  PixelInfo
    zero;
} TransparentPaintRow;

static void TransparentPaintRowPixels(const TransparentPaintRow *row,
  Quantum *magick_restrict q)
{
  Image
    *image = row->image;

  const PixelInfo
    *target = row->target;

  const Quantum
    opacity = row->opacity;

  const MagickBooleanType
    invert = row->invert;

  const PixelInfo
    zero = row->zero;

  PixelInfo
    pixel;

  ssize_t
    x;

  pixel=zero;
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    GetPixelInfoPixel(image,q,&pixel);
    if (IsFuzzyEquivalencePixelInfo(&pixel,target) != invert)
      SetPixelAlpha(image,opacity,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType PrepareTransparentImage(Image *image,
  ExceptionInfo *exception)
{
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
  return(MagickTrue);
}

MagickExport MagickBooleanType TransparentPaintImage(Image *image,
  const PixelInfo *target,const Quantum opacity,const MagickBooleanType invert,
  ExceptionInfo *exception)
{
#define TransparentPaintImageTag  "Transparent/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    zero;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(target != (PixelInfo *) NULL);
  if (IsEventLogging() != MagickFalse) 
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (PrepareTransparentImage(image,exception) == MagickFalse)
    return(MagickFalse);
  /*
    Make image color transparent.
  */
  status=MagickTrue;
  progress=0;
  GetPixelInfo(image,&zero);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const PaintRows
      rows = { image,image_view,exception,y };

    Quantum
      *magick_restrict q;

    TransparentPaintRow
      row;

    if (GetPaintRow(&rows,&q,&status) == MagickFalse)
      continue;
    row.image=image;
    row.target=target;
    row.opacity=opacity;
    row.invert=invert;
    row.zero=zero;
    TransparentPaintRowPixels(&row,q);
    SyncPaintRow(image_view,exception,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetPaintProgress(image,TransparentPaintImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     T r a n s p a r e n t P a i n t I m a g e C h r o m a                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransparentPaintImageChroma() changes the opacity value associated with any
%  pixel that matches color to the value defined by opacity.
%
%  As there is one fuzz value for the all the channels, TransparentPaintImage()
%  is not suitable for the operations like chroma, where the tolerance for
%  similarity of two color component (RGB) can be different. Thus we define
%  this method to take two target pixels (one low and one high) and all the
%  pixels of an image which are lying between these two pixels are made
%  transparent.
%
%  The format of the TransparentPaintImageChroma method is:
%
%      MagickBooleanType TransparentPaintImageChroma(Image *image,
%        const PixelInfo *low,const PixelInfo *high,const Quantum opacity,
%        const MagickBooleanType invert,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o low: the low target color.
%
%    o high: the high target color.
%
%    o opacity: the replacement opacity value.
%
%    o invert: paint any pixel that does not match the target color.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsPixelInChromaRange(const PixelInfo *pixel,
  const PixelInfo *low,const PixelInfo *high)
{
  return(((pixel->red >= low->red) && (pixel->red <= high->red) &&
    (pixel->green >= low->green) && (pixel->green <= high->green) &&
    (pixel->blue  >= low->blue) && (pixel->blue <= high->blue)) ? MagickTrue :
    MagickFalse);
}

MagickExport MagickBooleanType TransparentPaintImageChroma(Image *image,
  const PixelInfo *low,const PixelInfo *high,const Quantum opacity,
  const MagickBooleanType invert,ExceptionInfo *exception)
{
#define TransparentPaintImageTag  "Transparent/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(high != (PixelInfo *) NULL);
  assert(low != (PixelInfo *) NULL);
  if (IsEventLogging() != MagickFalse) 
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
  /*
    Make image color transparent.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const PaintRows
      rows = { image,image_view,exception,y };

    MagickBooleanType
      match;

    PixelInfo
      pixel;

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (GetPaintRow(&rows,&q,&status) == MagickFalse)
      continue;
    GetPixelInfo(image,&pixel);
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      GetPixelInfoPixel(image,q,&pixel);
      match=IsPixelInChromaRange(&pixel,low,high);
      if (match != invert)
        SetPixelAlpha(image,opacity,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    SyncPaintRow(image_view,exception,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetPaintProgress(image,TransparentPaintImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}
