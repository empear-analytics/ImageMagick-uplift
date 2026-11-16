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
%                      H   H   OOO   U   U   GGGG  H   H                      %
%                      H   H  O   O  U   U  G      H   H                      %
%                      HHHHH  O   O  U   U  G  GG  HHHHH                      %
%                      H   H  O   O  U   U  G   G  H   H                      %
%                      H   H   OOO    UUU    GGG   H   H                      %
%                                                                             %
%                                                                             %
%                   MagickCore Hough Line Detection Method                    %
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
  Define declarations.
*/
#define CannyEdgeImageTag  "CannyEdge/Image"

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

static void ScaleHoughDrawing(Image *image,DrawInfo *draw_info)
{
  draw_info->affine.sx=image->resolution.x == 0.0 ? 1.0 : image->resolution.x/
    DefaultResolution;
  draw_info->affine.sy=image->resolution.y == 0.0 ? 1.0 : image->resolution.y/
    DefaultResolution;
  image->columns=CastDoubleToSizeT(draw_info->affine.sx*image->columns);
  image->rows=CastDoubleToSizeT(draw_info->affine.sy*image->rows);
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
  ScaleHoughDrawing(image,draw_info);
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

typedef struct _HoughOutput
{
  const Image
    *image;

  MatrixInfo
    *accumulator;

  size_t
    accumulator_width,
    accumulator_height,
    threshold;
} HoughOutput;

static size_t GetHoughLineCount(const Image *image,const size_t threshold)
{
  size_t
    line_count;

  line_count=image->columns > image->rows ? image->columns/4 : image->rows/4;
  if (threshold != 0)
    line_count=threshold;
  return(line_count);
}

static MagickBooleanType WriteHoughFile(const HoughOutput *output,
  const HoughWindow window,char *path)
{
  const Image
    *const image = output->image;

  MatrixInfo
    *const accumulator = output->accumulator;

  const size_t
    accumulator_width = output->accumulator_width,
    accumulator_height = output->accumulator_height,
    threshold = output->threshold;

  HoughSegments
    segments;

  int
    file;

  MagickBooleanType
    status;

  size_t
    line_count;

  file=AcquireUniqueFileResource(path);
  if (file == -1)
    return(MagickFalse);
  status=WriteHoughHeader(image,file,&window,threshold);
  line_count=GetHoughLineCount(image,threshold);
  segments.accumulator=accumulator;
  segments.image=image;
  segments.accumulator_width=accumulator_width;
  segments.accumulator_height=accumulator_height;
  segments.line_count=line_count;
  segments.file=file;
  status=WriteHoughLines(&segments,window,status);
  (void) close_utf8(file);
  return(MagickTrue);
}

static Image *RenderHoughImage(const Image *image,const char *path,
  MatrixInfo *accumulator,ExceptionInfo *exception)
{
  const char
    *artifact;

  Image
    *lines_image = NULL;

  ImageInfo
    *image_info;

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

typedef struct _HoughResult
{
  const Image
    *image;

  MatrixInfo
    *accumulator;

  size_t
    accumulator_width,
    accumulator_height,
    threshold;

  HoughWindow
    window;

  MagickBooleanType
    status;

  ExceptionInfo
    *exception;
} HoughResult;

static Image *RenderHoughVotes(const HoughResult *votes_result)
{
  const Image
    *const image = votes_result->image;

  MatrixInfo
    *accumulator = votes_result->accumulator;

  const size_t
    accumulator_width = votes_result->accumulator_width,
    accumulator_height = votes_result->accumulator_height,
    threshold = votes_result->threshold;

  const HoughWindow
    window = votes_result->window;

  const MagickBooleanType
    status = votes_result->status;

  ExceptionInfo
    *const exception = votes_result->exception;

  char
    path[MagickPathExtent];

  HoughOutput
    output;

  if (status == MagickFalse)
    {
      accumulator=DestroyMatrixInfo(accumulator);
      return((Image *) NULL);
    }
  /*
    Generate line segments from accumulator.
  */
  output.image=image;
  output.accumulator=accumulator;
  output.accumulator_width=accumulator_width;
  output.accumulator_height=accumulator_height;
  output.threshold=threshold;
  if (WriteHoughFile(&output,window,path) == MagickFalse)
    {
      accumulator=DestroyMatrixInfo(accumulator);
      return((Image *) NULL);
    }
  return(RenderHoughImage(image,path,accumulator,exception));
}

typedef struct _HoughAccumulator
{
  const Image
    *image;

  MatrixInfo
    *accumulator;

  double
    hough_height;

  size_t
    accumulator_width,
    accumulator_height,
    threshold;

  HoughWindow
    window;

  ExceptionInfo
    *exception;
} HoughAccumulator;

static Image *VoteHoughAccumulator(const HoughAccumulator *accumulator_votes)
{
  const Image
    *const image = accumulator_votes->image;

  CacheView
    *image_view;

  HoughResult
    votes_result;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PointInfo
    center;

  ssize_t
    y;

  /*
    Populate the accumulator_votes->accumulator.
  */
  status=MagickTrue;
  progress=0;
  center.x=(double) image->columns/2.0;
  center.y=(double) image->rows/2.0;
  image_view=AcquireVirtualCacheView(image,accumulator_votes->exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    HoughVotes
      votes;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,
      accumulator_votes->exception);
    if (p == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    votes.image=image;
    votes.accumulator=accumulator_votes->accumulator;
    votes.hough_height=accumulator_votes->hough_height;
    votes.y=y;
    VoteHoughRow(&votes,p,center);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetFeatureProgress(image,CannyEdgeImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  votes_result.image=image;
  votes_result.accumulator=accumulator_votes->accumulator;
  votes_result.accumulator_width=accumulator_votes->accumulator_width;
  votes_result.accumulator_height=accumulator_votes->accumulator_height;
  votes_result.threshold=accumulator_votes->threshold;
  votes_result.window=accumulator_votes->window;
  votes_result.status=status;
  votes_result.exception=accumulator_votes->exception;
  return(RenderHoughVotes(&votes_result));
}

MagickExport Image *HoughLineImage(const Image *image,const size_t width,
  const size_t height,const size_t threshold,ExceptionInfo *exception)
{
#define HoughLineImageTag  "HoughLine/Image"

  double
    hough_height;

  const HoughWindow
    window = { width, height };

  HoughAccumulator
    accumulator_votes;

  MatrixInfo
    *accumulator;

  size_t
    accumulator_height,
    accumulator_width;

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
  accumulator_votes.image=image;
  accumulator_votes.accumulator=accumulator;
  accumulator_votes.hough_height=hough_height;
  accumulator_votes.accumulator_width=accumulator_width;
  accumulator_votes.accumulator_height=accumulator_height;
  accumulator_votes.threshold=threshold;
  accumulator_votes.window=window;
  accumulator_votes.exception=exception;
  return(VoteHoughAccumulator(&accumulator_votes));
}
