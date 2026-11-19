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
%                                                                             %
%                MagickCore Discrete Fourier Transform Methods                %
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
#if defined(MAGICKCORE_FFTW_DELEGATE)
#if defined(_MSC_VER)
#define ENABLE_FFTW_DELEGATE
#elif !defined(__cplusplus) && !defined(c_plusplus)
#define ENABLE_FFTW_DELEGATE
#endif
#endif
#if defined(ENABLE_FFTW_DELEGATE)
#if defined(MAGICKCORE_HAVE_COMPLEX_H)
#include <complex.h>
#endif
#include <fftw3.h>
#if !defined(MAGICKCORE_HAVE_CABS)
#define cabs(z)  (sqrt(z[0]*z[0]+z[1]*z[1]))
#endif
#if !defined(MAGICKCORE_HAVE_CARG)
#define carg(z)  (atan2(cimag(z),creal(z)))
#endif
#if !defined(MAGICKCORE_HAVE_CIMAG)
#define cimag(z)  (z[1])
#endif
#if !defined(MAGICKCORE_HAVE_CREAL)
#define creal(z)  (z[0])
#endif
#endif
#include "MagickCore/fourier-forward-private.h"


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     F o r w a r d F o u r i e r T r a n s f o r m I m a g e                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ForwardFourierTransformImage() implements the discrete Fourier transform
%  (DFT) of the image either as a magnitude / phase or real / imaginary image
%  pair.
%
%  The format of the ForwardFourierTransformImage method is:
%
%      Image *ForwardFourierTransformImage(const Image *image,
%        const MagickBooleanType modulus,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o modulus: if true, return as transform as a magnitude / phase pair
%      otherwise a real / imaginary image pair.
%
%    o exception: return any errors or warnings in this structure.
%
*/

MagickExport Image *ForwardFourierTransformImage(const Image *image,
  const MagickBooleanType modulus,ExceptionInfo *exception)
{
  Image
    *fourier_image;

  fourier_image=NewImageList();
#if !defined(ENABLE_FFTW_DELEGATE)
  (void) modulus;
  (void) ThrowMagickException(exception,GetMagickModule(),
    MissingDelegateWarning,"DelegateLibrarySupportNotBuiltIn","`%s' (FFTW)",
    image->filename);
#else
  {
    Image
      *magnitude_image;

    size_t
      height,
      width;

    width=image->columns;
    height=image->rows;
    width=PadFourierWidth(image,width);
    height=width;
    magnitude_image=CloneImage(image,width,height,MagickTrue,exception);
    if (magnitude_image != (Image *) NULL)
      {
        Image
          *phase_image;

        magnitude_image->storage_class=DirectClass;
        magnitude_image->depth=32UL;
        phase_image=CloneImage(image,width,height,MagickTrue,exception);
        if (phase_image == (Image *) NULL)
          magnitude_image=DestroyImage(magnitude_image);
        else
          {
            const ForwardFourierPair
              pair = { image, modulus, magnitude_image, phase_image,
                exception };

            fourier_image=ForwardFourierTransformPair(&pair,fourier_image);
          }
      }
  }
#endif
  return(fourier_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     I n v e r s e F o u r i e r T r a n s f o r m I m a g e                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InverseFourierTransformImage() implements the inverse discrete Fourier
%  transform (DFT) of the image either as a magnitude / phase or real /
%  imaginary image pair.
%
%  The format of the InverseFourierTransformImage method is:
%
%      Image *InverseFourierTransformImage(const Image *magnitude_image,
%        const Image *phase_image,const MagickBooleanType modulus,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o magnitude_image: the magnitude or real image.
%
%    o phase_image: the phase or imaginary image.
%
%    o modulus: if true, return transform as a magnitude / phase pair
%      otherwise a real / imaginary image pair.
%
%    o exception: return any errors or warnings in this structure.
%
*/

#if defined(ENABLE_FFTW_DELEGATE)
static MagickBooleanType InverseQuadrantSwap(const size_t width,
  const size_t height,const double *source,double *destination)
{
  RollExtent
    extent;

  size_t
    center;

  ssize_t
    x,
    y;

  /*
    Swap quadrants.
  */
  center=width/2+1;
  for (y=1L; y < (ssize_t) height; y++)
    for (x=0L; x < (ssize_t) (width/2L+1L); x++)
      destination[((ssize_t) height-y)*(ssize_t) center-x+(ssize_t) width/2L]=
        source[y*(ssize_t) width+x];
  for (y=0L; y < (ssize_t) height; y++)
    destination[y*(ssize_t) center]=
      source[y*(ssize_t) width+(ssize_t) width/2L];
  for (x=0L; x < (ssize_t) center; x++)
    destination[x]=source[(ssize_t) center-x-1L];
  extent.width=center;
  extent.height=height;
  return(RollFourier(&extent,0L,(ssize_t) height/-2L,destination));
}

static void ScaleInverseFourierPhase(const FourierInfo *fourier_info,
  double *phase_pixels)
{
  ssize_t
    i,
    x,
    y;

  if (fourier_info->modulus != MagickFalse)
    {
      i=0L;
      for (y=0L; y < (ssize_t) fourier_info->height; y++)
        for (x=0L; x < (ssize_t) fourier_info->width; x++)
        {
          phase_pixels[i]-=0.5;
          phase_pixels[i]*=(2.0*MagickPI);
          i++;
        }
    }
}

static inline MagickBooleanType IsFourierMemoryMissing(
  const MemoryInfo *magnitude_info,const MemoryInfo *phase_info,
  const MemoryInfo *inverse_info)
{
  return(((magnitude_info == (MemoryInfo *) NULL) ||
    (phase_info == (MemoryInfo *) NULL) ||
    (inverse_info == (MemoryInfo *) NULL)) ? MagickTrue : MagickFalse);
}

static void RelinquishInverseFourierMemory(MemoryInfo *magnitude_info,
  MemoryInfo *phase_info,MemoryInfo *inverse_info)
{
  if (magnitude_info != (MemoryInfo *) NULL)
    magnitude_info=RelinquishVirtualMemory(magnitude_info);
  if (phase_info != (MemoryInfo *) NULL)
    phase_info=RelinquishVirtualMemory(phase_info);
  if (inverse_info != (MemoryInfo *) NULL)
    inverse_info=RelinquishVirtualMemory(inverse_info);
}

typedef struct _InverseFourierPhase
{
  CacheView
    *phase_view;

  FourierInfo
    *fourier_info;

  const InverseImagePair
    *images;

  ExceptionInfo
    *exception;
} InverseFourierPhase;

static void GetInverseFourierPhase(const InverseFourierPhase *phase,
  double *phase_pixels,ssize_t i)
{
  CacheView
    *const phase_view = phase->phase_view;

  FourierInfo
    *const fourier_info = phase->fourier_info;

  const InverseImagePair
    *const images = phase->images;

  ExceptionInfo
    *const exception = phase->exception;

  const Quantum
    *p;

  ssize_t
    x,
    y;

  for (y=0L; y < (ssize_t) fourier_info->height; y++)
  {
    p=GetCacheViewVirtualPixels(phase_view,0,y,fourier_info->width,1,
      exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0L; x < (ssize_t) fourier_info->width; x++)
    {
      phase_pixels[i]=GetFourierPixel(images->phase_image,
        fourier_info->channel,p);
      i++;
      p+=(ptrdiff_t) GetPixelChannels(images->phase_image);
    }
  }
}

typedef struct _InverseFourierMemory
{
  MemoryInfo
    **magnitude_info,
    **phase_info,
    **inverse_info;
} InverseFourierMemory;

static MagickBooleanType AcquireInverseFourierMemory(
  const FourierInfo *fourier_info,const InverseImagePair *images,
  const InverseFourierMemory *targets,ExceptionInfo *exception)
{
  double
    *inverse_pixels,
    *magnitude_pixels,
    *phase_pixels;

  MemoryInfo
    **const magnitude_info = targets->magnitude_info,
    **const phase_info = targets->phase_info,
    **const inverse_info = targets->inverse_info;

  (*magnitude_info)=AcquireVirtualMemory(fourier_info->width,
    fourier_info->height*sizeof(*magnitude_pixels));
  (*phase_info)=AcquireVirtualMemory(fourier_info->width,fourier_info->height*
    sizeof(*phase_pixels));
  (*inverse_info)=AcquireVirtualMemory(fourier_info->width,
    (fourier_info->height/2+1)*sizeof(*inverse_pixels));
  if (IsFourierMemoryMissing((*magnitude_info),(*phase_info),
      (*inverse_info)) != MagickFalse)
    {
      RelinquishInverseFourierMemory((*magnitude_info),(*phase_info),
        (*inverse_info));
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",
        images->magnitude_image->filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void ReadInverseFourierPhase(FourierInfo *fourier_info,
  const InverseImagePair *images,double *phase_pixels,ExceptionInfo *exception)
{
  CacheView
    *phase_view;

  InverseFourierPhase
    phase;

  ssize_t
    i;

  i=0L;
  phase_view=AcquireVirtualCacheView(images->phase_image,exception);
  phase.phase_view=phase_view;
  phase.fourier_info=fourier_info;
  phase.images=images;
  phase.exception=exception;
  GetInverseFourierPhase(&phase,phase_pixels,i);
  ScaleInverseFourierPhase(fourier_info,phase_pixels);
  phase_view=DestroyCacheView(phase_view);
  CorrectPhaseLHS(fourier_info->width,fourier_info->height,phase_pixels);
}

static MagickBooleanType SwapInversePhaseQuadrants(
  const FourierInfo *fourier_info,double *phase_pixels,double *inverse_pixels,
  MagickBooleanType status)
{
  if (status != MagickFalse)
    status=InverseQuadrantSwap(fourier_info->width,fourier_info->height,
      phase_pixels,inverse_pixels);
  return(status);
}

typedef struct _InverseFourierSets
{
  FourierInfo
    *fourier_info;

  double
    *magnitude_pixels,
    *phase_pixels;
} InverseFourierSets;

static void MergeInverseFourierPolar(const InverseFourierSets *sets,ssize_t i,
  fftw_complex *fourier_pixels)
{
  FourierInfo
    *const fourier_info = sets->fourier_info;

  double
    *const magnitude_pixels = sets->magnitude_pixels,
    *const phase_pixels = sets->phase_pixels;

  ssize_t
    x,
    y;

  for (y=0L; y < (ssize_t) fourier_info->height; y++)
    for (x=0L; x < (ssize_t) fourier_info->center; x++)
    {
#if defined(MAGICKCORE_HAVE_COMPLEX_H)
      fourier_pixels[i]=magnitude_pixels[i]*cos(phase_pixels[i])+I*
        magnitude_pixels[i]*sin(phase_pixels[i]);
#else
      fourier_pixels[i][0]=magnitude_pixels[i]*cos(phase_pixels[i]);
      fourier_pixels[i][1]=magnitude_pixels[i]*sin(phase_pixels[i]);
#endif
      i++;
    }
}

static MagickBooleanType InverseFourier(FourierInfo *fourier_info,
  const InverseImagePair *images,fftw_complex *fourier_pixels,
  ExceptionInfo *exception)
{
  double
    *inverse_pixels,
    *magnitude_pixels,
    *phase_pixels;

  InverseFourierSets
    sets;

  MagickBooleanType
    status;

  MemoryInfo
    *inverse_info,
    *magnitude_info,
    *phase_info;

  const InverseFourierMemory
    targets = { &magnitude_info, &phase_info, &inverse_info };

  ssize_t
    i,
    x,
    y;

  /*
    Inverse fourier - read image and break down into a double array.
  */
  if (AcquireInverseFourierMemory(fourier_info,images,&targets,exception) ==
      MagickFalse)
    return(MagickFalse);
  magnitude_pixels=(double *) GetVirtualMemoryBlob(magnitude_info);
  phase_pixels=(double *) GetVirtualMemoryBlob(phase_info);
  inverse_pixels=(double *) GetVirtualMemoryBlob(inverse_info);
  GetFourierChannelPixels(fourier_info,images->magnitude_image,magnitude_pixels,
    exception);
  status=InverseQuadrantSwap(fourier_info->width,fourier_info->height,
    magnitude_pixels,inverse_pixels);
  (void) memcpy(magnitude_pixels,inverse_pixels,fourier_info->height*
    fourier_info->center*sizeof(*magnitude_pixels));
  ReadInverseFourierPhase(fourier_info,images,phase_pixels,exception);
  status=SwapInversePhaseQuadrants(fourier_info,phase_pixels,inverse_pixels,
    status);
  (void) memcpy(phase_pixels,inverse_pixels,fourier_info->height*
    fourier_info->center*sizeof(*phase_pixels));
  inverse_info=RelinquishVirtualMemory(inverse_info);
  /*
    Merge two sets.
  */
  i=0L;
  sets.fourier_info=fourier_info;
  sets.magnitude_pixels=magnitude_pixels;
  sets.phase_pixels=phase_pixels;
  if (fourier_info->modulus != MagickFalse)
    MergeInverseFourierPolar(&sets,i,fourier_pixels);
  else
    for (y=0L; y < (ssize_t) fourier_info->height; y++)
      for (x=0L; x < (ssize_t) fourier_info->center; x++)
      {
#if defined(MAGICKCORE_HAVE_COMPLEX_H)
        fourier_pixels[i]=magnitude_pixels[i]+I*phase_pixels[i];
#else
        fourier_pixels[i][0]=magnitude_pixels[i];
        fourier_pixels[i][1]=phase_pixels[i];
#endif
        i++;
      }
  magnitude_info=RelinquishVirtualMemory(magnitude_info);
  phase_info=RelinquishVirtualMemory(phase_info);
  return(status);
}

typedef struct _InverseFourierRow
{
  FourierInfo
    *fourier_info;

  Image
    *image;

  double
    *source_pixels;
} InverseFourierRow;

static void SetInverseFourierRow(const InverseFourierRow *row,ssize_t *i,
  Quantum **q)
{
  FourierInfo
    *const fourier_info = row->fourier_info;

  Image
    *const image = row->image;

  double
    *const source_pixels = row->source_pixels;

  ssize_t
    x;

  for (x=0L; x < (ssize_t) fourier_info->width; x++)
  {
    if (x < (ssize_t) image->columns)
      SetFourierPixel(image,fourier_info->channel,source_pixels[*i],*q);
    (*i)++;
    *q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void SetInverseFourierPixels(FourierInfo *fourier_info,
  Image *image,double *source_pixels,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  Quantum
    *q;

  ssize_t
    i,
    y;

  i=0L;
  image_view=AcquireAuthenticCacheView(image,exception);
  for (y=0L; y < (ssize_t) fourier_info->height; y++)
  {
    InverseFourierRow
      row;

    if (y >= (ssize_t) image->rows)
      break;
    q=GetCacheViewAuthenticPixels(image_view,0L,y,fourier_info->width >
      image->columns ? image->columns : fourier_info->width,1UL,exception);
    if (q == (Quantum *) NULL)
      break;
    row.fourier_info=fourier_info;
    row.image=image;
    row.source_pixels=source_pixels;
    SetInverseFourierRow(&row,&i,&q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
  }
  image_view=DestroyCacheView(image_view);
}

static MagickBooleanType InverseFourierTransform(FourierInfo *fourier_info,
  fftw_complex *fourier_pixels,Image *image,ExceptionInfo *exception)
{
  const char
    *value;

  double
    *source_pixels;

  fftw_plan
    fftw_c2r_plan;

  MemoryInfo
    *source_info;

  ssize_t
    i,
    x,
    y;

  /*
    Generate the inverse Fourier transform.
  */
  if ((fourier_info->width >= INT_MAX) || (fourier_info->height >= INT_MAX))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "WidthOrHeightExceedsLimit","`%s'",image->filename);
      return(MagickFalse);
    }
  source_info=AcquireVirtualMemory(fourier_info->width,fourier_info->height*
    sizeof(*source_pixels));
  if (source_info == (MemoryInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(MagickFalse);
    }
  source_pixels=(double *) GetVirtualMemoryBlob(source_info);
  value=GetImageArtifact(image,"fourier:normalize");
  if (LocaleCompare(value,"inverse") == 0)
    {
      double
        gamma;

      /*
        Normalize inverse transform.
      */
      i=0L;
      gamma=MagickSafeReciprocal((double) fourier_info->width*
        fourier_info->height);
      for (y=0L; y < (ssize_t) fourier_info->height; y++)
        for (x=0L; x < (ssize_t) fourier_info->center; x++)
        {
#if defined(MAGICKCORE_HAVE_COMPLEX_H)
          fourier_pixels[i]*=gamma;
#else
          fourier_pixels[i][0]*=gamma;
          fourier_pixels[i][1]*=gamma;
#endif
          i++;
        }
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp critical (MagickCore_InverseFourierTransform)
#endif
  fftw_c2r_plan=fftw_plan_dft_c2r_2d((int) fourier_info->width,
    (int) fourier_info->height,fourier_pixels,source_pixels,FFTW_ESTIMATE);
  fftw_execute_dft_c2r(fftw_c2r_plan,fourier_pixels,source_pixels);
  fftw_destroy_plan(fftw_c2r_plan);
  SetInverseFourierPixels(fourier_info,image,source_pixels,exception);
  source_info=RelinquishVirtualMemory(source_info);
  return(MagickTrue);
}

static MagickBooleanType InverseFourierTransformChannel(
  const InverseChannelSource *source,const PixelChannel channel,
  Image *fourier_image,ExceptionInfo *exception)
{
  const InverseImagePair
    images = { source->magnitude_image, source->phase_image };

  fftw_complex
    *inverse_pixels;

  FourierInfo
    fourier_info;

  MagickBooleanType
    status;

  MemoryInfo
    *inverse_info;

  fourier_info.width=source->magnitude_image->columns;
  fourier_info.height=source->magnitude_image->rows;
  if (IsFourierPaddingNeeded(source->magnitude_image) != MagickFalse)
    {
      size_t extent=source->magnitude_image->columns <
        source->magnitude_image->rows ? source->magnitude_image->rows :
        source->magnitude_image->columns;
      fourier_info.width=(extent & 0x01) == 1 ? extent+1UL : extent;
    }
  fourier_info.height=fourier_info.width;
  fourier_info.center=fourier_info.width/2+1;
  fourier_info.channel=channel;
  fourier_info.modulus=source->modulus;
  inverse_info=AcquireVirtualMemory(fourier_info.width,(fourier_info.height/2+
    1)*sizeof(*inverse_pixels));
  if (inverse_info == (MemoryInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",
        source->magnitude_image->filename);
      return(MagickFalse);
    }
  inverse_pixels=(fftw_complex *) GetVirtualMemoryBlob(inverse_info);
  status=InverseFourier(&fourier_info,&images,inverse_pixels,exception);
  if (status != MagickFalse)
    status=InverseFourierTransform(&fourier_info,inverse_pixels,fourier_image,
      exception);
  inverse_info=RelinquishVirtualMemory(inverse_info);
  return(status);
}

static MagickBooleanType FourierSectionChannel(
  const FourierSectionInput *input,const PixelChannel channel)
{
  if (input->inverse_source != (const InverseChannelSource *) NULL)
    return(InverseFourierTransformChannel(input->inverse_source,channel,
      input->fourier_image,input->exception));
  return(ForwardFourierTransformChannel(input->forward_source,channel,
    input->fourier_image,input->exception));
}

static void FourierSection(const FourierSectionInput *input,
  const MagickBooleanType run,const PixelChannel channel,
  MagickBooleanType *status)
{
  MagickBooleanType
    thread_status;

  thread_status=MagickTrue;
  if (run != MagickFalse)
    thread_status=FourierSectionChannel(input,channel);
  if (thread_status == MagickFalse)
    (*status)=thread_status;
}

static MagickBooleanType FourierTransformChannels(
  const FourierSectionInput *section,const Image *image,
  const MagickBooleanType is_gray)
{
  MagickBooleanType
    status;

  status=MagickTrue;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel sections
#endif
  {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp section
#endif
    {
      MagickBooleanType
        thread_status;

      if (is_gray != MagickFalse)
        thread_status=FourierSectionChannel(section,GrayPixelChannel);
      else
        thread_status=FourierSectionChannel(section,RedPixelChannel);
      if (thread_status == MagickFalse)
        status=thread_status;
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp section
#endif
    {
      FourierSection(section,
        is_gray == MagickFalse,
        GreenPixelChannel,&status);
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp section
#endif
    {
      FourierSection(section,
        is_gray == MagickFalse,
        BluePixelChannel,&status);
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp section
#endif
    {
      FourierSection(section,
        image->colorspace == CMYKColorspace,
        BlackPixelChannel,&status);
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp section
#endif
    {
      FourierSection(section,
        image->alpha_trait != UndefinedPixelTrait,
        AlphaPixelChannel,&status);
    }
  }
  return(status);
}
#endif

static MagickBooleanType CheckFourierPhaseImage(const Image *magnitude_image,
  const Image *phase_image,ExceptionInfo *exception)
{
  if (phase_image == (Image *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ImageSequenceRequired","`%s'",magnitude_image->filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

MagickExport Image *InverseFourierTransformImage(const Image *magnitude_image,
  const Image *phase_image,const MagickBooleanType modulus,
  ExceptionInfo *exception)
{
  Image
    *fourier_image;

  assert(magnitude_image != (Image *) NULL);
  assert(magnitude_image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      magnitude_image->filename);
  if (CheckFourierPhaseImage(magnitude_image,phase_image,exception) ==
      MagickFalse)
    return((Image *) NULL);
#if !defined(ENABLE_FFTW_DELEGATE)
  fourier_image=(Image *) NULL;
  (void) modulus;
  (void) ThrowMagickException(exception,GetMagickModule(),
    MissingDelegateWarning,"DelegateLibrarySupportNotBuiltIn","`%s' (FFTW)",
    magnitude_image->filename);
#else
  {
    fourier_image=CloneImage(magnitude_image,magnitude_image->columns,
      magnitude_image->rows,MagickTrue,exception);
    if (fourier_image != (Image *) NULL)
      {
        const InverseChannelSource
          source = { magnitude_image, phase_image, modulus };

        const FourierSectionInput
          section = { (const ForwardChannelSource *) NULL, &source,
            fourier_image, exception };

        MagickBooleanType
          is_gray,
          status;

        is_gray=IsImageGray(magnitude_image);
        if (is_gray != MagickFalse)
          is_gray=IsImageGray(phase_image);
        status=FourierTransformChannels(&section,magnitude_image,is_gray);
        if (status == MagickFalse)
          fourier_image=DestroyImage(fourier_image);
      }
    fftw_cleanup();
  }
#endif
  return(fourier_image);
}
