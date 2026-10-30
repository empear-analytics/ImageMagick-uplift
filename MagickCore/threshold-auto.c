/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        TTTTT  H   H  RRRR   EEEEE  SSSSS  H   H   OOO   L      DDDD         %
%          T    H   H  R   R  E      SS     H   H  O   O  L      D   D        %
%          T    HHHHH  RRRR   EEE     SSS   HHHHH  O   O  L      D   D        %
%          T    H   H  R R    E         SS  H   H  O   O  L      D   D        %
%          T    H   H  R  R   EEEEE  SSSSS  H   H   OOO   LLLLL  DDDD         %
%                                                                             %
%                          AAA   U   U  TTTTT   OOO                           %
%                         A   A  U   U    T    O   O                          %
%                         AAAAA  U   U    T    O   O                          %
%                         A   A  U   U    T    O   O                          %
%                         A   A   UUU     T     OOO                           %
%                                                                             %
%                                                                             %
%                    MagickCore Automatic Threshold Method                    %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                 October 1996                                %
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
#include "MagickCore/blob.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/configure.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/effect.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/shear.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A u t o T h r e s h o l d I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AutoThresholdImage()  automatically performs image thresholding
%  dependent on which method you specify.
%
%  The format of the AutoThresholdImage method is:
%
%      MagickBooleanType AutoThresholdImage(Image *image,
%        const AutoThresholdMethod method,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image to auto-threshold.
%
%    o method: choose from Kapur, OTSU, or Triangle.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType IsKapurBufferMissing(
  const double *cumulative_histogram,const double *black_entropy,
  const double *white_entropy)
{
  return(((cumulative_histogram == (double *) NULL) ||
    (black_entropy == (double *) NULL) || (white_entropy == (double *) NULL)) ?
    MagickTrue : MagickFalse);
}

#define MaxIntensity  255

static size_t FindKapurThreshold(const double *black_entropy,
  const double *white_entropy)
{
  double
    maximum_entropy;

  ssize_t
    j;

  size_t
    threshold;

  maximum_entropy=black_entropy[0]+white_entropy[0];
  threshold=0;
  for (j=1; j <= MaxIntensity; j++)
    if ((black_entropy[j]+white_entropy[j]) > maximum_entropy)
      {
        maximum_entropy=black_entropy[j]+white_entropy[j];
        threshold=(size_t) j;
      }
  return(threshold);
}

static double KapurThreshold(const Image *image,const double *histogram,
  ExceptionInfo *exception)
{
  double
    *black_entropy,
    *cumulative_histogram,
    entropy,
    epsilon,
    *white_entropy;

  ssize_t
    i,
    j;

  size_t
    threshold;

  /*
    Compute optimal threshold from the entropy of the histogram.
  */
  cumulative_histogram=(double *) AcquireQuantumMemory(MaxIntensity+1UL,
    sizeof(*cumulative_histogram));
  black_entropy=(double *) AcquireQuantumMemory(MaxIntensity+1UL,
    sizeof(*black_entropy));
  white_entropy=(double *) AcquireQuantumMemory(MaxIntensity+1UL,
    sizeof(*white_entropy));
  if (IsKapurBufferMissing(cumulative_histogram,black_entropy,white_entropy) !=
      MagickFalse)
    {
      if (white_entropy != (double *) NULL)
        white_entropy=(double *) RelinquishMagickMemory(white_entropy);
      if (black_entropy != (double *) NULL)
        black_entropy=(double *) RelinquishMagickMemory(black_entropy);
      if (cumulative_histogram != (double *) NULL)
        cumulative_histogram=(double *)
          RelinquishMagickMemory(cumulative_histogram);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(-1.0);
    }
   /*
     Entropy for black and white parts of the histogram.
   */
   cumulative_histogram[0]=histogram[0];
   for (i=1; i <= MaxIntensity; i++)
     cumulative_histogram[i]=cumulative_histogram[i-1]+histogram[i];
   epsilon=MagickMinimumValue;
   for (j=0; j <= MaxIntensity; j++)
   {
     /*
       Black entropy.
     */
     black_entropy[j]=0.0;
     if (cumulative_histogram[j] > epsilon)
       {
         entropy=0.0;
         for (i=0; i <= j; i++)
           if (histogram[i] > epsilon)
             entropy-=histogram[i]/cumulative_histogram[j]*
               log(histogram[i]/cumulative_histogram[j]);
         black_entropy[j]=entropy;
       }
     /*
       White entropy.
     */
     white_entropy[j]=0.0;
     if ((1.0-cumulative_histogram[j]) > epsilon)
       {
         entropy=0.0;
         for (i=j+1; i <= MaxIntensity; i++)
           if (histogram[i] > epsilon)
             entropy-=histogram[i]/(1.0-cumulative_histogram[j])*
               log(histogram[i]/(1.0-cumulative_histogram[j]));
         white_entropy[j]=entropy;
       }
   }
  /*
    Find histogram bin with maximum entropy.
  */
  threshold=FindKapurThreshold(black_entropy,white_entropy);
  /*
    Free resources.
  */
  white_entropy=(double *) RelinquishMagickMemory(white_entropy);
  black_entropy=(double *) RelinquishMagickMemory(black_entropy);
  cumulative_histogram=(double *) RelinquishMagickMemory(cumulative_histogram);
  return(100.0*threshold/MaxIntensity);
}

static inline MagickBooleanType IsOTSUBufferMissing(const double *myu,
  const double *omega,const double *probability,const double *sigma)
{
  return(((myu == (double *) NULL) || (omega == (double *) NULL) ||
    (probability == (double *) NULL) || (sigma == (double *) NULL)) ?
    MagickTrue : MagickFalse);
}

static double FindOTSUThreshold(const double *myu,const double *omega,
  double *sigma)
{
  double
    max_sigma,
    threshold;

  ssize_t
    i;

  threshold=0;
  max_sigma=0.0;
  for (i=0; i < (ssize_t) MaxIntensity; i++)
  {
    sigma[i]=0.0;
    if ((omega[i] != 0.0) && (omega[i] != 1.0))
      sigma[i]=pow(myu[MaxIntensity]*omega[i]-myu[i],2.0)/(omega[i]*(1.0-
        omega[i]));
    if (sigma[i] > max_sigma)
      {
        max_sigma=sigma[i];
        threshold=(double) i;
      }
  }
  return(threshold);
}

static double OTSUThreshold(const Image *image,const double *histogram,
  ExceptionInfo *exception)
{
  double
    *myu,
    *omega,
    *probability,
    *sigma,
    threshold;

  ssize_t
    i;

  /*
    Compute optimal threshold from maximization of inter-class variance.
  */
  myu=(double *) AcquireQuantumMemory(MaxIntensity+1UL,sizeof(*myu));
  omega=(double *) AcquireQuantumMemory(MaxIntensity+1UL,sizeof(*omega));
  probability=(double *) AcquireQuantumMemory(MaxIntensity+1UL,
    sizeof(*probability));
  sigma=(double *) AcquireQuantumMemory(MaxIntensity+1UL,sizeof(*sigma));
  if (IsOTSUBufferMissing(myu,omega,probability,sigma) != MagickFalse)
    {
      if (sigma != (double *) NULL)
        sigma=(double *) RelinquishMagickMemory(sigma);
      if (probability != (double *) NULL)
        probability=(double *) RelinquishMagickMemory(probability);
      if (omega != (double *) NULL)
        omega=(double *) RelinquishMagickMemory(omega);
      if (myu != (double *) NULL)
        myu=(double *) RelinquishMagickMemory(myu);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(-1.0);
    }
  /*
    Calculate probability density.
  */
  for (i=0; i <= (ssize_t) MaxIntensity; i++)
    probability[i]=histogram[i];
  /*
    Generate probability of graylevels and mean value for separation.
  */
  omega[0]=probability[0];
  myu[0]=0.0;
  for (i=1; i <= (ssize_t) MaxIntensity; i++)
  {
    omega[i]=omega[i-1]+probability[i];
    myu[i]=myu[i-1]+i*probability[i];
  }
  /*
    Sigma maximization: inter-class variance and compute optimal threshold.
  */
  threshold=FindOTSUThreshold(myu,omega,sigma);
  /*
    Free resources.
  */
  myu=(double *) RelinquishMagickMemory(myu);
  omega=(double *) RelinquishMagickMemory(omega);
  probability=(double *) RelinquishMagickMemory(probability);
  sigma=(double *) RelinquishMagickMemory(sigma);
  return(100.0*threshold/MaxIntensity);
}

static double TriangleThreshold(const double *histogram)
{
  double
    a,
    b,
    c,
    count,
    distance,
    inverse_ratio,
    max_distance,
    segment,
    x1,
    x2,
    y1,
    y2;

  ssize_t
    i;

  ssize_t
    end,
    max,
    start,
    threshold;

  /*
    Compute optimal threshold with triangle algorithm.
  */
  start=0;  /* find start bin, first bin not zero count */
  for (i=0; i <= (ssize_t) MaxIntensity; i++)
    if (histogram[i] > 0.0)
      {
        start=i;
        break;
      }
  end=0;  /* find end bin, last bin not zero count */
  for (i=(ssize_t) MaxIntensity; i >= 0; i--)
    if (histogram[i] > 0.0)
      {
        end=i;
        break;
      }
  max=0;  /* find max bin, bin with largest count */
  count=0.0;
  for (i=0; i <= (ssize_t) MaxIntensity; i++)
    if (histogram[i] > count)
      {
        max=i;
        count=histogram[i];
      }
  /*
    Compute threshold at split point.
  */
  x1=(double) max;
  y1=histogram[max];
  x2=(double) end;
  if ((max-start) >= (end-max))
    x2=(double) start;
  y2=0.0;
  a=y1-y2;
  b=x2-x1;
  c=(-1.0)*(a*x1+b*y1);
  inverse_ratio=1.0/sqrt(a*a+b*b+c*c);
  threshold=0;
  max_distance=0.0;
  if (x2 == (double) start)
    for (i=start; i < max; i++)
    {
      segment=inverse_ratio*(a*i+b*histogram[i]+c);
      distance=sqrt(segment*segment);
      if ((distance > max_distance) && (segment > 0.0))
        {
          threshold=i;
          max_distance=distance;
        }
    }
  else
    for (i=end; i > max; i--)
    {
      segment=inverse_ratio*(a*i+b*histogram[i]+c);
      distance=sqrt(segment*segment);
      if ((distance > max_distance) && (segment < 0.0))
        {
          threshold=i;
          max_distance=distance;
        }
    }
  return(100.0*threshold/MaxIntensity);
}

static void FormIntensityHistogram(const Image *image,double *histogram,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  ssize_t
    y;

  image_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      double intensity = GetPixelIntensity(image,p);
      histogram[ScaleQuantumToChar(ClampToQuantum(intensity))]++;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  image_view=DestroyCacheView(image_view);
}

static void NormalizeHistogram(double *histogram)
{
  double
    gamma,
    sum;

  ssize_t
    i;

  sum=0.0;
  for (i=0; i <= (ssize_t) MaxIntensity; i++)
    sum+=histogram[i];
  gamma=MagickSafeReciprocal(sum);
  for (i=0; i <= (ssize_t) MaxIntensity; i++)
    histogram[i]=gamma*histogram[i];
}

static double DiscoverHistogramThreshold(const Image *image,
  const AutoThresholdMethod method,const double *histogram,
  ExceptionInfo *exception)
{
  double
    threshold;

  switch (method)
  {
    case KapurThresholdMethod:
    {
      threshold=KapurThreshold(image,histogram,exception);
      break;
    }
    case OTSUThresholdMethod:
    default:
    {
      threshold=OTSUThreshold(image,histogram,exception);
      break;
    }
    case TriangleThresholdMethod:
    {
      threshold=TriangleThreshold(histogram);
      break;
    }
  }
  return(threshold);
}

MagickExport MagickBooleanType AutoThresholdImage(Image *image,
  const AutoThresholdMethod method,ExceptionInfo *exception)
{
  char
    property[MagickPathExtent];

  double
    *histogram,
    threshold;

  MagickBooleanType
    status;

  /*
    Form histogram.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  histogram=(double *) AcquireQuantumMemory(MaxIntensity+1UL,
    sizeof(*histogram));
  if (histogram == (double *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  status=MagickTrue;
  (void) memset(histogram,0,(MaxIntensity+1UL)*sizeof(*histogram));
  FormIntensityHistogram(image,histogram,exception);
  /*
    Normalize histogram.
  */
  NormalizeHistogram(histogram);
  /*
    Discover threshold from histogram.
  */
  threshold=DiscoverHistogramThreshold(image,method,histogram,exception);
  histogram=(double *) RelinquishMagickMemory(histogram);
  if (threshold < 0.0)
    status=MagickFalse;
  if (status == MagickFalse)
    return(MagickFalse);
  /*
    Threshold image.
  */
  (void) FormatLocaleString(property,MagickPathExtent,"%g%%",threshold);
  (void) SetImageProperty(image,"auto-threshold:threshold",property,exception);
  if (IsStringTrue(GetImageArtifact(image,"auto-threshold:verbose")) != MagickFalse)
    (void) FormatLocaleFile(stdout,"%.*g%%\n",GetMagickPrecision(),threshold);
  return(BilevelImage(image,(double) QuantumRange*threshold/100.0,exception));
}
