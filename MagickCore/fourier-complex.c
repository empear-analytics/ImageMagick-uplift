/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               FFFFF   OOO   U   U  RRRR   IIIII  EEEEE  RRRR                %
%               F      O   O  U   U  R   R    I    E      R   R               %
%               FFF    O   O  U   U  RRRR     I    EEE    RRRR                %
%               F      O   O  U   U  R R      I    E      R R                 %
%               F       OOO    UUU   R  R   IIIII  EEEEE  R  R                %
%                                                                             %
%                CCCC   OOO   M   M  PPPP   L      EEEEE  X   X               %
%               C      O   O  MM MM  P   P  L      E       X X                %
%               C      O   O  M M M  PPPP   L      EEE      X                 %
%               C      O   O  M   M  P      L      E       X X                %
%                CCCC   OOO   M   M  P      LLLLL  EEEEE  X   X               %
%                                                                             %
%                                                                             %
%                     MagickCore Complex Image Arithmetic                     %
%                                                                             %
%                              Software Design                                %
%                                Sean Burke                                   %
%                               Fred Weinhaus                                 %
%                                   Cristy                                    %
%                                 July 2009                                   %
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
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/fourier.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"

typedef struct _ComplexRowPointers
{
  const Quantum
    *Ai,
    *Ar,
    *Bi,
    *Br,
    *Ci,
    *Cr;
} ComplexRowPointers;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o m p l e x I m a g e s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ComplexImages() performs complex mathematics on an image sequence.
%
%  The format of the ComplexImages method is:
%
%      MagickBooleanType ComplexImages(Image *images,const ComplexOperator op,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o op: A complex operator.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsComplexRowMissing(
  const ComplexRowPointers *pointers)
{
  return(((pointers->Ar == (const Quantum *) NULL) ||
    (pointers->Ai == (const Quantum *) NULL) ||
    (pointers->Br == (const Quantum *) NULL) ||
    (pointers->Bi == (const Quantum *) NULL) ||
    (pointers->Cr == (Quantum *) NULL) ||
    (pointers->Ci == (Quantum *) NULL)) ? MagickTrue : MagickFalse);
}

typedef struct _ComplexUnaryOperands
{
  ComplexOperator
    op;

  double
    ai,
    ar;
} ComplexUnaryOperands;

static void ApplyUnaryComplexOperator(const ComplexUnaryOperands *unary,
  double *cr,double *ci)
{
  const ComplexOperator
    op = unary->op;

  const double
    ai = unary->ai,
    ar = unary->ar;

  switch (op)
  {
    case ConjugateComplexOperator:
    default:
    {
      (*cr)=ar;
      (*ci)=(-ai);
      break;
    }
    case MagnitudePhaseComplexOperator:
    {
      (*cr)=sqrt(ar*ar+ai*ai);
      (*ci)=atan2((double) ai,(double) ar)/(2.0*MagickPI)+0.5;
      break;
    }
    case RealImaginaryComplexOperator:
    {
      (*cr)=ar*cos(2.0*MagickPI*(ai-0.5));
      (*ci)=ar*sin(2.0*MagickPI*(ai-0.5));
      break;
    }
  }
}

typedef struct _ComplexPixelOperands
{
  ComplexOperator
    op;

  double
    ai,
    ar,
    bi,
    br,
    snr;
} ComplexPixelOperands;

static void ApplyComplexOperator(const ComplexPixelOperands *operands,
  double *cr,double *ci)
{
  const ComplexOperator
    op = operands->op;

  const double
    ai = operands->ai,
    ar = operands->ar,
    bi = operands->bi,
    br = operands->br,
    snr = operands->snr;

  switch (op)
  {
    case AddComplexOperator:
    {
      (*cr)=ar+br;
      (*ci)=ai+bi;
      break;
    }
    default:
    {
      ComplexUnaryOperands
        unary;

      unary.op=op;
      unary.ai=ai;
      unary.ar=ar;
      ApplyUnaryComplexOperator(&unary,&(*cr),&(*ci));
      break;
    }
    case DivideComplexOperator:
    {
      (*cr)=MagickSafeReciprocal(br*br+bi*bi+snr)*(ar*br+ai*bi);
      (*ci)=MagickSafeReciprocal(br*br+bi*bi+snr)*(ai*br-ar*bi);
      break;
    }
    case MultiplyComplexOperator:
    {
      (*cr)=(ar*br-ai*bi);
      (*ci)=(ai*br+ar*bi);
      break;
    }
    case SubtractComplexOperator:
    {
      (*cr)=ar-br;
      (*ci)=ai-bi;
      break;
    }
  }
}

typedef struct _ComplexChannelOperands
{
  const Quantum
    *Ai,
    *Ar,
    *Bi,
    *Br;

  Quantum
    *Ci,
    *Cr;

  size_t
    number_channels;

  ComplexOperator
    op;

  double
    snr;
} ComplexChannelOperands;

static void ApplyComplexOperatorToChannels(
  const ComplexChannelOperands *channels)
{
  const Quantum
    *const Ai = channels->Ai,
    *const Ar = channels->Ar,
    *const Bi = channels->Bi,
    *const Br = channels->Br;

  Quantum
    *const Ci = channels->Ci,
    *const Cr = channels->Cr;

  const size_t
    number_channels = channels->number_channels;

  const ComplexOperator
    op = channels->op;

  const double
    snr = channels->snr;

  ssize_t
    i;

  for (i=0; i < (ssize_t) number_channels; i++)
  {
    double
      ai = QuantumScale*(double) Ai[i],
      ar = QuantumScale*(double) Ar[i],
      bi = QuantumScale*(double) Bi[i],
      br = QuantumScale*(double) Br[i],
      ci,
      cr;

    ComplexPixelOperands
      operands;

    operands.op=op;
    operands.ai=ai;
    operands.ar=ar;
    operands.bi=bi;
    operands.br=br;
    operands.snr=snr;
    ApplyComplexOperator(&operands,&cr,&ci);
    Cr[i]=ClampToQuantum((double) QuantumRange*cr);
    Ci[i]=ClampToQuantum((double) QuantumRange*ci);
  }
}

static void SelectComplexOperandImages(const Image *images,
  const Image **Br_image,const Image **Bi_image)
{
  *Br_image=images;
  *Bi_image=images->next;
  if ((images->next->next != (Image *) NULL) &&
      (images->next->next->next != (Image *) NULL))
    {
      *Br_image=images->next->next;
      *Bi_image=images->next->next->next;
    }
}

static double GetComplexSNR(const Image *images)
{
  const char
    *artifact;

  double
    snr;

  artifact=GetImageArtifact(images,"complex:snr");
  snr=0.0;
  if (artifact != (const char *) NULL)
    snr=StringToDouble(artifact,(char **) NULL);
  return(snr);
}

static MagickBooleanType SyncComplexViews(CacheView *Ci_view,
  CacheView *Cr_view,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=MagickTrue;
  if (SyncCacheViewAuthenticPixels(Ci_view,exception) == MagickFalse)
    status=MagickFalse;
  if (SyncCacheViewAuthenticPixels(Cr_view,exception) == MagickFalse)
    status=MagickFalse;
  return(status);
}

static MagickBooleanType CheckComplexSequence(const Image *images,
  ExceptionInfo *exception)
{
  if (images->next == (Image *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ImageSequenceRequired","`%s'",images->filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _ComplexRowInputs
{
  CacheView
    *Ai_view,
    *Ar_view,
    *Bi_view,
    *Br_view,
    *Ci_view,
    *Cr_view;

  const Image
    *Ai_image,
    *Ar_image,
    *Bi_image,
    *Br_image,
    *Ci_image,
    *Cr_image;

  size_t
    columns,
    number_channels;

  ComplexOperator
    op;

  double
    snr;

  ExceptionInfo
    *exception;
} ComplexRowInputs;

static MagickBooleanType ComplexImageRow(const ComplexRowInputs *rows,
  const ssize_t y,MagickBooleanType *status)
{
  const Quantum
    *magick_restrict Ai,
    *magick_restrict Ar,
    *magick_restrict Bi,
    *magick_restrict Br;

  ComplexRowPointers
    pointers;

  Quantum
    *magick_restrict Ci,
    *magick_restrict Cr;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  Ar=GetCacheViewVirtualPixels(rows->Ar_view,0,y,rows->columns,1,
    rows->exception);
  Ai=GetCacheViewVirtualPixels(rows->Ai_view,0,y,rows->columns,1,
    rows->exception);
  Br=GetCacheViewVirtualPixels(rows->Br_view,0,y,rows->columns,1,
    rows->exception);
  Bi=GetCacheViewVirtualPixels(rows->Bi_view,0,y,rows->columns,1,
    rows->exception);
  Cr=QueueCacheViewAuthenticPixels(rows->Cr_view,0,y,rows->columns,1,
    rows->exception);
  Ci=QueueCacheViewAuthenticPixels(rows->Ci_view,0,y,rows->columns,1,
    rows->exception);
  pointers.Ai=Ai;
  pointers.Ar=Ar;
  pointers.Bi=Bi;
  pointers.Br=Br;
  pointers.Ci=Ci;
  pointers.Cr=Cr;
  if (IsComplexRowMissing(&pointers) != MagickFalse)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  for (x=0; x < (ssize_t) rows->columns; x++)
  {
    ComplexChannelOperands
      channels;

    channels.Ai=Ai;
    channels.Ar=Ar;
    channels.Bi=Bi;
    channels.Br=Br;
    channels.Ci=Ci;
    channels.Cr=Cr;
    channels.number_channels=rows->number_channels;
    channels.op=rows->op;
    channels.snr=rows->snr;
    ApplyComplexOperatorToChannels(&channels);
    Ar+=(ptrdiff_t) GetPixelChannels(rows->Ar_image);
    Ai+=(ptrdiff_t) GetPixelChannels(rows->Ai_image);
    Br+=(ptrdiff_t) GetPixelChannels(rows->Br_image);
    Bi+=(ptrdiff_t) GetPixelChannels(rows->Bi_image);
    Cr+=(ptrdiff_t) GetPixelChannels(rows->Cr_image);
    Ci+=(ptrdiff_t) GetPixelChannels(rows->Ci_image);
  }
  if (SyncComplexViews(rows->Ci_view,rows->Cr_view,rows->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static void ReportComplexProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

static MagickBooleanType TryCloneComplexImages(const Image *images,
  ExceptionInfo *exception,Image **complex_images,Image **result)
{
  Image
    *image;

  image=CloneImage(images,0,0,MagickTrue,exception);
  if (image == (Image *) NULL)
    {
      (*result)=(Image *) NULL;
      return(MagickTrue);
    }
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    {
      image=DestroyImageList(image);
      (*result)=image;
      return(MagickTrue);
    }
  image->depth=32UL;
  *complex_images=NewImageList();
  AppendImageToList(complex_images,image);
  image=CloneImage(images->next,0,0,MagickTrue,exception);
  if (image == (Image *) NULL)
    {
      *complex_images=DestroyImageList(*complex_images);
      (*result)=*complex_images;
      return(MagickTrue);
    }
  image->depth=32UL;
  AppendImageToList(complex_images,image);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    {
      *complex_images=DestroyImageList(*complex_images);
      (*result)=*complex_images;
      return(MagickTrue);
    }
  return(MagickFalse);
}

static void DestroyComplexViewPair(CacheView *real_view,
  CacheView *imaginary_view)
{
  real_view=DestroyCacheView(real_view);
  imaginary_view=DestroyCacheView(imaginary_view);
}

static Image *FinishComplexImages(Image *complex_images,
  const MagickBooleanType status)
{
  if (status == MagickFalse)
    complex_images=DestroyImageList(complex_images);
  return(complex_images);
}

MagickExport Image *ComplexImages(const Image *images,const ComplexOperator op,
  ExceptionInfo *exception)
{
#define ComplexImageTag  "Complex/Image"

  CacheView
    *Ai_view,
    *Ar_view,
    *Bi_view,
    *Br_view,
    *Ci_view,
    *Cr_view;

  const Image
    *Ai_image,
    *Ar_image,
    *Bi_image,
    *Br_image;

  double
    snr;

  Image
    *Ci_image,
    *complex_images,
    *Cr_image,
    *returned;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  size_t
    columns,
    number_channels,
    rows;

  ssize_t
    y;

  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  if (CheckComplexSequence(images,exception) == MagickFalse)
    return((Image *) NULL);
  if (TryCloneComplexImages(images,exception,&complex_images,&returned) !=
      MagickFalse)
    return(returned);
  /*
    Apply complex mathematics to image pixels.
  */
  snr=GetComplexSNR(images);
  Ar_image=images;
  Ai_image=images->next;
  SelectComplexOperandImages(images,&Br_image,&Bi_image);
  Cr_image=complex_images;
  Ci_image=complex_images->next;
  number_channels=MagickMin(MagickMin(MagickMin(
    Ar_image->number_channels,Ai_image->number_channels),MagickMin(
    Br_image->number_channels,Bi_image->number_channels)),MagickMin(
    Cr_image->number_channels,Ci_image->number_channels));
  Ar_view=AcquireVirtualCacheView(Ar_image,exception);
  Ai_view=AcquireVirtualCacheView(Ai_image,exception);
  Br_view=AcquireVirtualCacheView(Br_image,exception);
  Bi_view=AcquireVirtualCacheView(Bi_image,exception);
  Cr_view=AcquireAuthenticCacheView(Cr_image,exception);
  Ci_view=AcquireAuthenticCacheView(Ci_image,exception);
  status=MagickTrue;
  progress=0;
  columns=MagickMin(Cr_image->columns,Ci_image->columns);
  rows=MagickMin(Cr_image->rows,Ci_image->rows);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(Cr_image,complex_images,rows,1L)
#endif
  for (y=0; y < (ssize_t) rows; y++)
  {
    const ComplexRowInputs
      inputs = { Ai_view, Ar_view, Bi_view, Br_view, Ci_view, Cr_view,
        Ai_image, Ar_image, Bi_image, Br_image, Ci_image, Cr_image, columns,
        number_channels, op, snr, exception };

    if (ComplexImageRow(&inputs,y,&status) == MagickFalse)
      continue;
    if (images->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportComplexProgress(images,ComplexImageTag,progress,&status);
      }
  }
  DestroyComplexViewPair(Cr_view,Ci_view);
  DestroyComplexViewPair(Br_view,Bi_view);
  DestroyComplexViewPair(Ar_view,Ai_view);
  return(FinishComplexImages(complex_images,status));
}
