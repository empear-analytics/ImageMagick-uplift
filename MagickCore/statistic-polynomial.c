/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        SSSSS  TTTTT   AAA   TTTTT  IIIII  SSSSS  TTTTT  IIIII   CCCC        %
%        SS       T    A   A    T      I    SS       T      I    C            %
%         SSS     T    AAAAA    T      I     SSS     T      I    C            %
%           SS    T    A   A    T      I       SS    T      I    C            %
%        SSSSS    T    A   A    T    IIIII  SSSSS    T    IIIII   CCCC        %
%                                                                             %
%    PPPP    OOO   L      Y   Y  N   N   OOO   M   M  IIIII   AAA   L         %
%    P   P  O   O  L       Y Y   NN  N  O   O  MM MM    I    A   A  L         %
%    PPPP   O   O  L        Y    N N N  O   O  M M M    I    AAAAA  L         %
%    P      O   O  L        Y    N  NN  O   O  M   M    I    A   A  L         %
%    P       OOO   LLLLL    Y    N   N   OOO   M   M  IIIII  A   A  LLLLL     %
%                                                                             %
%                                                                             %
%                     MagickCore Image Polynomial Methods                     %
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
#include "MagickCore/accelerate-private.h"
#include "MagickCore/animate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
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
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/list.h"
#include "MagickCore/image-private.h"
#include "MagickCore/magic.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/module.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/statistic.h"
#include "MagickCore/statistic-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/timer.h"
#include "MagickCore/utility.h"
#include "MagickCore/version.h"
#include "MagickCore/statistic-evaluate-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     P o l y n o m i a l I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PolynomialImage() returns a new image where each pixel is the sum of the
%  pixels in the image sequence after applying its corresponding terms
%  (coefficient and degree pairs).
%
%  The format of the PolynomialImage method is:
%
%      Image *PolynomialImage(const Image *images,const size_t number_terms,
%        const double *terms,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o images: the image sequence.
%
%    o number_terms: the number of terms in the list.  The actual list length
%      is 2 x number_terms + 1 (the constant).
%
%    o terms: the list of polynomial coefficients and degree pairs and a
%      constant.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SetPolynomialPixels(const Image *image,
  const PixelChannels *polynomial_pixel,Quantum *magick_restrict q)
{
  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,i);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if ((traits & UpdatePixelTrait) == 0)
        continue;
      q[i]=ClampToQuantum((double) QuantumRange*
        polynomial_pixel[x].channel[i]);
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}


typedef struct _PolynomialTerm
{
  Image
    *image;

  const Image
    *next;

  const double
    *terms;

  ssize_t
    j;
} PolynomialTerm;

static void AccumulatePolynomialRow(const PolynomialTerm *term,
  const Quantum *p,PixelChannels *polynomial_pixel)
{
  Image
    *const image = term->image;

  const Image
    *const next = term->next;

  const double
    *const terms = term->terms;

  const ssize_t
    j = term->j;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    for (i=0; i < (ssize_t) GetPixelChannels(next); i++)
    {
      MagickRealType
        coefficient,
        degree;

      PixelChannel channel = GetPixelChannelChannel(image,i);
      PixelTrait traits = GetPixelChannelTraits(next,channel);
      PixelTrait polynomial_traits = GetPixelChannelTraits(image,channel);
      if (((traits & UpdatePixelTrait) == 0) ||
          ((polynomial_traits & UpdatePixelTrait) == 0))
        continue;
      coefficient=(MagickRealType) terms[2*j];
      degree=(MagickRealType) terms[(j << 1)+1];
      polynomial_pixel[x].channel[i]+=coefficient*
        pow(QuantumScale*(double) GetPixelChannel(image,channel,p),degree);
    }
    p+=(ptrdiff_t) GetPixelChannels(next);
  }
}

typedef struct _PolynomialRows
{
  Image
    *image;

  const Image
    *images;

  size_t
    number_images,
    number_terms;

  const double
    *terms;

  ssize_t
    y;
} PolynomialRows;

static void AccumulatePolynomialImages(const PolynomialRows *rows,
  PixelChannels *polynomial_pixel,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  const Image
    *next;

  PolynomialTerm
    term;

  ssize_t
    j;

  next=rows->images;
  for (j=0; j < (ssize_t) rows->number_images; j++)
  {
    const Quantum
      *p;

    if (j >= (ssize_t) rows->number_terms)
      continue;
    image_view=AcquireVirtualCacheView(next,exception);
    p=GetCacheViewVirtualPixels(image_view,0,rows->y,rows->image->columns,1,
      exception);
    if (p == (const Quantum *) NULL)
      {
        image_view=DestroyCacheView(image_view);
        break;
      }
    term.image=rows->image;
    term.next=next;
    term.terms=rows->terms;
    term.j=j;
    AccumulatePolynomialRow(&term,p,polynomial_pixel);
    image_view=DestroyCacheView(image_view);
    next=GetNextImageInList(next);
  }
}

#define PolynomialImageTag  "Polynomial/Image"

static void SetPolynomialProgress(const Image *images,const Image *image,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  if (SetProgress(images,image,PolynomialImageTag,progress) == MagickFalse)
    (*status)=MagickFalse;
}

typedef struct _PolynomialLine
{
  Image
    *image;

  const Image
    *images;

  CacheView
    *polynomial_view;

  PixelChannels
    **magick_restrict polynomial_pixels;

  int
    id;

  size_t
    number_images,
    number_terms;

  const double
    *terms;

  ExceptionInfo
    *exception;
} PolynomialLine;

static MagickBooleanType PolynomialImageLine(const PolynomialLine *line,
  const ssize_t y,MagickBooleanType *status)
{
  Image
    *const image = line->image;

  const Image
    *const images = line->images;

  CacheView
    *const polynomial_view = line->polynomial_view;

  PixelChannels
    **magick_restrict const polynomial_pixels = line->polynomial_pixels;

  const int
    id = line->id;

  const size_t
    number_images = line->number_images,
    number_terms = line->number_terms;

  const double
    *const terms = line->terms;

  ExceptionInfo
    *const exception = line->exception;

  PixelChannels
    *polynomial_pixel;

  PolynomialRows
    rows;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=QueueCacheViewAuthenticPixels(polynomial_view,0,y,image->columns,1,
    exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  polynomial_pixel=polynomial_pixels[id];
  ClearPolynomialPixels(image,polynomial_pixel);
  rows.image=image;
  rows.images=images;
  rows.number_images=number_images;
  rows.number_terms=number_terms;
  rows.terms=terms;
  rows.y=y;
  AccumulatePolynomialImages(&rows,polynomial_pixel,exception);
  SetPolynomialPixels(image,polynomial_pixel,q);
  SyncEvaluateView(polynomial_view,exception,status);
  return(MagickTrue);
}

MagickExport Image *PolynomialImage(const Image *images,
  const size_t number_terms,const double *terms,ExceptionInfo *exception)
{
  CacheView
    *polynomial_view;

  Image
    *image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelChannels
    **magick_restrict polynomial_pixels;

  size_t
    number_images;

  ssize_t
    y;

  const EvaluateCanvasTargets
    targets = { &image, &number_images, &polynomial_pixels };

  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (AcquireEvaluateCanvas(images,&targets,exception) == MagickFalse)
    return((Image *) NULL);
  /*
    Polynomial image pixels.
  */
  status=MagickTrue;
  progress=0;
  polynomial_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const int
      id = GetOpenMPThreadId();

    const PolynomialLine
      line = { image, images, polynomial_view, polynomial_pixels, id,
        number_images, number_terms, terms, exception };

    if (PolynomialImageLine(&line,y,&status) == MagickFalse)
      continue;
    if (images->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetPolynomialProgress(images,image,progress,&status);
      }
  }
  polynomial_view=DestroyCacheView(polynomial_view);
  polynomial_pixels=DestroyPixelTLS(images,polynomial_pixels);
  if (status == MagickFalse)
    image=DestroyImage(image);
  return(image);
}
