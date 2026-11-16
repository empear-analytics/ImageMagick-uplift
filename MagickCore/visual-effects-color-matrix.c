/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  V   V  IIIII  SSSSS  U   U   AAA   L                       %
%                  V   V    I    SS     U   U  A   A  L                       %
%                  V   V    I     SSS   U   U  AAAAA  L                       %
%                   V V     I       SS  U   U  A   A  L                       %
%                    V    IIIII  SSSSS   UUU   A   A  LLLLL                   %
%                                                                             %
%               EEEEE  FFFFF  FFFFF  EEEEE   CCCC  TTTTT  SSSSS               %
%               E      F      F      E      C        T    SS                  %
%               EEE    FFF    FFF    EEE    C        T     SSS                %
%               E      F      F      E      C        T       SS               %
%               EEEEE  F      F      EEEEE   CCCC    T    SSSSS               %
%                                                                             %
%                       CCCC   OOO   L       OOO   RRRR                       %
%                      C      O   O  L      O   O  R   R                      %
%                      C      O   O  L      O   O  RRRR                       %
%                      C      O   O  L      O   O  R R                        %
%                       CCCC   OOO   LLLLL   OOO   R  R                       %
%                                                                             %
%                  M   M   AAA   TTTTT  RRRR   IIIII  X   X                   %
%                  MM MM  A   A    T    R   R    I     X X                    %
%                  M M M  AAAAA    T    RRRR     I      X                     %
%                  M   M  A   A    T    R R      I     X X                    %
%                  M   M  A   A    T    R  R   IIIII  X   X                   %
%                                                                             %
%                                                                             %
%                       MagickCore Color Matrix Method                        %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                 October 1996                                %
%                                                                             %
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
#include "MagickCore/accelerate-private.h"
#include "MagickCore/annotate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/decorate.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/layer.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/visual-effects.h"
#include "MagickCore/visual-effects-pixel-methods-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o l o r M a t r i x I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ColorMatrixImage() applies color transformation to an image. This method
%  permits saturation changes, hue rotation, luminance to alpha, and various
%  other effects.  Although variable-sized transformation matrices can be used,
%  typically one uses a 5x5 matrix for an RGBA image and a 6x6 for CMYKA
%  (or RGBA with offsets).  The matrix is similar to those used by Adobe Flash
%  except offsets are in column 6 rather than 5 (in support of CMYKA images)
%  and offsets are normalized (divide Flash offset by 255).
%
%  The format of the ColorMatrixImage method is:
%
%      Image *ColorMatrixImage(const Image *image,
%        const KernelInfo *color_matrix,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o color_matrix:  the color matrix.  It is the callers responsibility to
%      ensure the matrix has at least width * height values.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static ssize_t LoadColorMatrixRow(const KernelInfo *color_matrix,
  double ColorMatrix[][6],const ssize_t v,ssize_t i)
{
  ssize_t
    u;

  for (u=0; u < (ssize_t) color_matrix->width; u++)
  {
    if ((v < 6) && (u < 6))
      ColorMatrix[v][u]=color_matrix->values[i];
    i++;
  }
  return(i);
}

static void LoadColorMatrix(const KernelInfo *color_matrix,
  double ColorMatrix[][6])
{
  ssize_t
    i,
    v;

  i=0;
  for (v=0; v < (ssize_t) color_matrix->height; v++)
    i=LoadColorMatrixRow(color_matrix,ColorMatrix,v,i);
}

static double GetColorMatrixSum(const Image *image,
  const Quantum *magick_restrict p,const double *row)
{
  double
    sum;

  sum=row[0]*(double) GetPixelRed(image,p)+row[1]*
    (double) GetPixelGreen(image,p)+row[2]*(double)
    GetPixelBlue(image,p);
  if (image->colorspace == CMYKColorspace)
    sum+=row[3]*(double) GetPixelBlack(image,p);
  if (image->alpha_trait != UndefinedPixelTrait)
    sum+=row[4]*(double) GetPixelAlpha(image,p);
  sum+=(double) QuantumRange*row[5];
  return(sum);
}

static PixelInfo SetColorMatrixChannel(PixelInfo pixel,const ssize_t h,
  const double sum)
{
  switch (h)
  {
    case 0: pixel.red=sum; break;
    case 1: pixel.green=sum; break;
    case 2: pixel.blue=sum; break;
    case 3: pixel.black=sum; break;
    case 4: pixel.alpha=sum; break;
    default: break;
  }
  return(pixel);
}

static void LogColorMatrix(double ColorMatrix[][6])
{
  char
    format[MagickPathExtent],
    *message;

  ssize_t
    u,
    v;

  (void) LogMagickEvent(TransformEvent,GetMagickModule(),
    "  ColorMatrix image with color matrix:");
  message=AcquireString("");
  for (v=0; v < 6; v++)
  {
    *message='\0';
    (void) FormatLocaleString(format,MagickPathExtent,"%.17g: ",(double) v);
    (void) ConcatenateString(&message,format);
    for (u=0; u < 6; u++)
    {
      (void) FormatLocaleString(format,MagickPathExtent,"%+f ",
        ColorMatrix[v][u]);
      (void) ConcatenateString(&message,format);
    }
    (void) LogMagickEvent(TransformEvent,GetMagickModule(),"%s",message);
  }
  message=DestroyString(message);
}

typedef struct _ColorMatrixRow
{
  const Image
    *image;

  Image
    *color_image;

  const KernelInfo
    *color_matrix;

  CacheView
    *image_view,
    *color_view;

  ExceptionInfo
    *exception;
} ColorMatrixRow;

static PixelInfo ApplyColorMatrixPixel(const ColorMatrixRow *color_row,
  double ColorMatrix[][6],const Quantum *magick_restrict p,PixelInfo pixel)
{
  const Image
    *const image = color_row->image;

  const KernelInfo
    *const color_matrix = color_row->color_matrix;

  ssize_t
    h;

  size_t
    height;

  height=color_matrix->height > 6 ? 6UL : color_matrix->height;
  for (h=0; h < (ssize_t) height; h++)
  {
    double
      sum;

    sum=GetColorMatrixSum(image,p,ColorMatrix[h]);
    pixel=SetColorMatrixChannel(pixel,h,sum);
  }
  return(pixel);
}

static void ApplyColorMatrixRow(const ColorMatrixRow *color_row,
  double ColorMatrix[][6],const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  const Image
    *const image = color_row->image;

  Image
    *const color_image = color_row->color_image;

  PixelInfo
    pixel;

  ssize_t
    x;

  GetPixelInfo(image,&pixel);
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    GetPixelInfoPixel(image,p,&pixel);
    pixel=ApplyColorMatrixPixel(color_row,ColorMatrix,p,pixel);
    SetPixelViaPixelInfo(color_image,&pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(color_image);
  }
}

static MagickBooleanType ColorMatrixImageRow(
  const ColorMatrixRow *color_row,double ColorMatrix[][6],const ssize_t y,
  MagickBooleanType *status)
{
  const Image
    *const image = color_row->image;

  Image
    *const color_image = color_row->color_image;

  CacheView
    *const image_view = color_row->image_view,
    *const color_view = color_row->color_view;

  ExceptionInfo
    *const exception = color_row->exception;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
  q=GetCacheViewAuthenticPixels(color_view,0,y,color_image->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  ApplyColorMatrixRow(color_row,ColorMatrix,p,q);
  if (SyncCacheViewAuthenticPixels(color_view,exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static void LogColorMatrixIfDebug(const Image *image,double ColorMatrix[][6])
{
  if (image->debug != MagickFalse)
    LogColorMatrix(ColorMatrix);
}

MagickExport Image *ColorMatrixImage(const Image *image,
  const KernelInfo *color_matrix,ExceptionInfo *exception)
{
#define ColorMatrixImageTag  "ColorMatrix/Image"

  CacheView
    *color_view,
    *image_view;

  double
    ColorMatrix[6][6] =
    {
      { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
      { 0.0, 1.0, 0.0, 0.0, 0.0, 0.0 },
      { 0.0, 0.0, 1.0, 0.0, 0.0, 0.0 },
      { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0 },
      { 0.0, 0.0, 0.0, 0.0, 1.0, 0.0 },
      { 0.0, 0.0, 0.0, 0.0, 0.0, 1.0 }
    };

  Image
    *color_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Map given color_matrix, into a 6x6 matrix   RGBKA and a constant
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  LoadColorMatrix(color_matrix,ColorMatrix);
  /*
    Initialize color image.
  */
  if (CloneEffectImage(image,&color_image,exception) == MagickFalse)
    return((Image *) NULL);
  LogColorMatrixIfDebug(image,ColorMatrix);
  /*
    Apply the ColorMatrix to image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  color_view=AcquireAuthenticCacheView(color_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,color_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const ColorMatrixRow
      color_row = { image, color_image, color_matrix, image_view, color_view,
        exception };

    if (ColorMatrixImageRow(&color_row,ColorMatrix,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(image,ColorMatrixImageTag,progress,&status);
      }
  }
  color_view=DestroyCacheView(color_view);
  image_view=DestroyCacheView(image_view);
  return(ReleaseFailedImage(color_image,status));
}
