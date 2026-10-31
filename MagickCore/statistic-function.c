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
%           FFFFF  U   U  N   N   CCCC  TTTTT  IIIII   OOO   N   N            %
%           F      U   U  NN  N  C        T      I    O   O  NN  N            %
%           FFF    U   U  N N N  C        T      I    O   O  N N N            %
%           F      U   U  N  NN  C        T      I    O   O  N  NN            %
%           F       UUU   N   N   CCCC    T    IIIII   OOO   N   N            %
%                                                                             %
%                                                                             %
%                      MagickCore Image Function Method                       %
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     F u n c t i o n I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FunctionImage() applies a value to the image with an arithmetic, relational,
%  or logical operator to an image. Use these operations to lighten or darken
%  an image, to increase or decrease contrast in an image, or to produce the
%  "negative" of an image.
%
%  The format of the FunctionImage method is:
%
%      MagickBooleanType FunctionImage(Image *image,
%        const MagickFunction function,const ssize_t number_parameters,
%        const double *parameters,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o function: A channel function.
%
%    o parameters: one or more parameters.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static double ApplySinusoidFunction(Quantum pixel,
  const size_t number_parameters,const double *parameters)
{
  double
    amplitude,
    bias,
    frequency,
    phase,
    result;

  /*
    Sinusoid: frequency, phase, amplitude, bias.
  */
  frequency=(number_parameters >= 1) ? parameters[0] : 1.0;
  phase=(number_parameters >= 2) ? parameters[1] : 0.0;
  amplitude=(number_parameters >= 3) ? parameters[2] : 0.5;
  bias=(number_parameters >= 4) ? parameters[3] : 0.5;
  result=(double) QuantumRange*(amplitude*sin((double) (2.0*
    MagickPI*(frequency*QuantumScale*(double) pixel+phase/360.0)))+bias);
  return(result);
}

static double ApplyArcsinFunction(Quantum pixel,
  const size_t number_parameters,const double *parameters)
{
  double
    bias,
    center,
    range,
    result,
    width;

  /*
    Arcsin (pegged at range limits for invalid results): width, center,
    range, and bias.
  */
  width=(number_parameters >= 1) ? parameters[0] : 1.0;
  center=(number_parameters >= 2) ? parameters[1] : 0.5;
  range=(number_parameters >= 3) ? parameters[2] : 1.0;
  bias=(number_parameters >= 4) ? parameters[3] : 0.5;
  result=2.0*MagickSafeReciprocal(width)*(QuantumScale*(double) pixel-
    center);
  if (result <= -1.0)
    result=bias-range/2.0;
  else
    if (result >= 1.0)
      result=bias+range/2.0;
    else
      result=(double) (range/MagickPI*asin((double) result)+bias);
  result*=(double) QuantumRange;
  return(result);
}

static double ApplyArctanFunction(Quantum pixel,
  const size_t number_parameters,const double *parameters)
{
  double
    center,
    bias,
    range,
    result,
    slope;

  /*
    Arctan: slope, center, range, and bias.
  */
  slope=(number_parameters >= 1) ? parameters[0] : 1.0;
  center=(number_parameters >= 2) ? parameters[1] : 0.5;
  range=(number_parameters >= 3) ? parameters[2] : 1.0;
  bias=(number_parameters >= 4) ? parameters[3] : 0.5;
  result=MagickPI*slope*(QuantumScale*(double) pixel-center);
  result=(double) QuantumRange*(range/MagickPI*atan((double) result)+bias);
  return(result);
}

static Quantum ApplyFunction(Quantum pixel,const MagickFunction function,
  const size_t number_parameters,const double *parameters,
  ExceptionInfo *exception)
{
  double
    result;

  ssize_t
    i;

  (void) exception;
  result=0.0;
  switch (function)
  {
    case PolynomialFunction:
    {
      /*
        Polynomial: polynomial constants, highest to lowest order (e.g. c0*x^3+
        c1*x^2+c2*x+c3).
      */
      result=0.0;
      for (i=0; i < (ssize_t) number_parameters; i++)
        result=result*QuantumScale*(double) pixel+parameters[i];
      result*=(double) QuantumRange;
      break;
    }
    case SinusoidFunction:
    {
      result=ApplySinusoidFunction(pixel,number_parameters,parameters);
      break;
    }
    case ArcsinFunction:
    {
      result=ApplyArcsinFunction(pixel,number_parameters,parameters);
      break;
    }
    case ArctanFunction:
    {
      result=ApplyArctanFunction(pixel,number_parameters,parameters);
      break;
    }
    case UndefinedFunction:
      break;
  }
  return(ClampToQuantum(result));
}

static void ApplyFunctionToPixel(const Image *image,
  const MagickFunction function,const size_t number_parameters,
  const double *parameters,ExceptionInfo *exception,Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    q[i]=ApplyFunction(q[i],function,number_parameters,parameters,
      exception);
  }
}

MagickExport MagickBooleanType FunctionImage(Image *image,
  const MagickFunction function,const size_t number_parameters,
  const double *parameters,ExceptionInfo *exception)
{
#define FunctionImageTag  "Function/Image "

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
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  if (AccelerateFunctionImage(image,function,number_parameters,parameters,
        exception) != MagickFalse)
    return(MagickTrue);
#endif
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      ApplyFunctionToPixel(image,function,number_parameters,parameters,exception,
        q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,FunctionImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}
