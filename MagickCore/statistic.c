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
%                                                                             %
%                     MagickCore Image Statistical Methods                    %
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
%     E v a l u a t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EvaluateImage() applies a value to the image with an arithmetic, relational,
%  or logical operator to an image. Use these operations to lighten or darken
%  an image, to increase or decrease contrast in an image, or to produce the
%  "negative" of an image.
%
%  The format of the EvaluateImage method is:
%
%      MagickBooleanType EvaluateImage(Image *image,
%        const MagickEvaluateOperator op,const double value,
%        ExceptionInfo *exception)
%      MagickBooleanType EvaluateImages(Image *images,
%        const MagickEvaluateOperator op,const double value,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o op: A channel op.
%
%    o value: A value value.
%
%    o exception: return any errors or warnings in this structure.
%
*/

typedef struct _PixelChannels
{
  double
    channel[MaxPixelChannels];
} PixelChannels;

typedef struct _EvaluateOperation
{
  const MagickEvaluateOperator
    op;

  const double
    value;

  const MagickBooleanType
    clamp;
} EvaluateOperation;

static PixelChannels **DestroyPixelTLS(const Image *images,
  PixelChannels **pixels)
{
  ssize_t
    i;

  size_t
    rows;

  assert(pixels != (PixelChannels **) NULL);
  rows=MagickMax(GetImageListLength(images),(size_t)
    GetMagickResourceLimit(ThreadResource));
  for (i=0; i < (ssize_t) rows; i++)
    if (pixels[i] != (PixelChannels *) NULL)
      pixels[i]=(PixelChannels *) RelinquishMagickMemory(pixels[i]);
  pixels=(PixelChannels **) RelinquishMagickMemory(pixels);
  return(pixels);
}

static void ClearPixelChannelsRow(PixelChannels **pixels,const ssize_t i,
  const size_t columns)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) columns; j++)
  {
    ssize_t
      k;

    for (k=0; k < MaxPixelChannels; k++)
      pixels[i][j].channel[k]=0.0;
  }
}

static PixelChannels **AcquirePixelTLS(const Image *images)
{
  const Image
    *next;

  PixelChannels
    **pixels;

  ssize_t
    i;

  size_t
    columns,
    number_images,
    rows;

  number_images=GetImageListLength(images);
  rows=MagickMax(number_images,(size_t) GetMagickResourceLimit(ThreadResource));
  pixels=(PixelChannels **) AcquireQuantumMemory(rows,sizeof(*pixels));
  if (pixels == (PixelChannels **) NULL)
    return((PixelChannels **) NULL);
  (void) memset(pixels,0,rows*sizeof(*pixels));
  columns=MagickMax(number_images,MaxPixelChannels);
  for (next=images; next != (Image *) NULL; next=next->next)
    columns=MagickMax(next->columns,columns);
  for (i=0; i < (ssize_t) rows; i++)
  {
    pixels[i]=(PixelChannels *) AcquireQuantumMemory(columns,sizeof(**pixels));
    if (pixels[i] == (PixelChannels *) NULL)
      return(DestroyPixelTLS(images,pixels));
    ClearPixelChannelsRow(pixels,i,columns);
  }
  return(pixels);
}

static inline double EvaluateMax(const double x,const double y)
{
  if (x > y)
    return(x);
  return(y);
}

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int IntensityCompare(const void *x,const void *y)
{
  const PixelChannels
    *color_1,
    *color_2;

  double
    distance;

  ssize_t
    i;

  color_1=(const PixelChannels *) x;
  color_2=(const PixelChannels *) y;
  distance=0.0;
  for (i=0; i < MaxPixelChannels; i++)
    distance+=color_1->channel[i]-(double) color_2->channel[i];
  return(distance < 0.0 ? -1 : distance > 0.0 ? 1 : 0);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static double EvaluatePow(const Quantum pixel,const double value,
  double result)
{
  if (fabs(value) <= MagickEpsilon)
    return(result);
  if (((double) pixel < 0.0) && ((value-floor(value)) > MagickEpsilon))
    result=(double) -((double) QuantumRange*pow(-(QuantumScale*(double)
      pixel),value));
  else
    result=(double) QuantumRange*pow(QuantumScale*(double) pixel,value);
  return(result);
}

static double EvaluateLog(const Quantum pixel,const double value,
  double result)
{
  if ((QuantumScale*(double) pixel) >= MagickEpsilon)
    result=(double) QuantumRange*log(QuantumScale*value*
      (double) pixel+1.0)/log((double) (value+1.0));
  return(result);
}

static double EvaluateLeftShift(const Quantum pixel,const double value)
{
  double
    result;

  ssize_t
    i;

  result=(double) pixel;
  for (i=0; i < (ssize_t) value; i++)
    result*=2.0;
  return(result);
}

static void ApplyBasicEvaluateOperator(const Quantum pixel,
  const MagickEvaluateOperator op,const double value,double *result)
{
  switch (op)
  {
    case UndefinedEvaluateOperator:
      break;
    case AbsEvaluateOperator:
    {
      (*result)=(double) fabs((double) pixel+value);
      break;
    }
    case AddEvaluateOperator:
    {
      (*result)=(double) pixel+value;
      break;
    }
    case AddModulusEvaluateOperator:
    {
      /*
        This returns a 'floored modulus' of the addition which is a positive
        result.  It differs from % or fmod() that returns a 'truncated modulus'
        result, where floor() is replaced by trunc() and could return a
        negative result (which is clipped).
      */
      (*result)=(double) pixel+value;
      (*result)-=((double) QuantumRange+1.0)*floor((*result)/((double)
        QuantumRange+1.0));
      break;
    }
    case AndEvaluateOperator:
    {
      (*result)=(double) ((ssize_t) pixel & (ssize_t) (value+0.5));
      break;
    }
    case CosineEvaluateOperator:
    {
      (*result)=(double) QuantumRange*(0.5*cos((double) (2.0*MagickPI*
        QuantumScale*(double) pixel*value))+0.5);
      break;
    }
    default:
      break;
  }
}

static void ApplyThresholdEvaluateOperator(const Quantum pixel,
  const MagickEvaluateOperator op,const double value,double *result)
{
  switch (op)
  {
    default:
    {
      ApplyBasicEvaluateOperator(pixel,op,value,result);
      break;
    }
    case ThresholdEvaluateOperator:
    {
      (*result)=(double) (((double) pixel <= value) ? 0 : QuantumRange);
      break;
    }
    case ThresholdBlackEvaluateOperator:
    {
      (*result)=(double) (((double) pixel <= value) ? 0 : pixel);
      break;
    }
    case ThresholdWhiteEvaluateOperator:
    {
      (*result)=(double) (((double) pixel > value) ? QuantumRange : pixel);
      break;
    }
  }
}

static void ApplyBitwiseEvaluateOperator(const Quantum pixel,
  const MagickEvaluateOperator op,const double value,double *result)
{
  ssize_t
    i;

  switch (op)
  {
    default:
    {
      ApplyThresholdEvaluateOperator(pixel,op,value,result);
      break;
    }
    case LeftShiftEvaluateOperator:
    {
      (*result)=EvaluateLeftShift(pixel,value);
      break;
    }
    case OrEvaluateOperator:
    {
      (*result)=(double) ((ssize_t) pixel | (ssize_t) (value+0.5));
      break;
    }
    case RightShiftEvaluateOperator:
    {
      (*result)=(double) pixel;
      for (i=0; i < (ssize_t) value; i++)
        (*result)/=2.0;
      break;
    }
    case XorEvaluateOperator:
    {
      (*result)=(double) ((ssize_t) pixel ^ (ssize_t) (value+0.5));
      break;
    }
  }
}

static void ApplyTranscendentalEvaluateOperator(const Quantum pixel,
  const MagickEvaluateOperator op,const double value,double *result)
{
  switch (op)
  {
    default:
    {
      ApplyBitwiseEvaluateOperator(pixel,op,value,result);
      break;
    }
    case ExponentialEvaluateOperator:
    {
      (*result)=(double) QuantumRange*exp(value*QuantumScale*(double) pixel);
      break;
    }
    case InverseLogEvaluateOperator:
    {
      (*result)=(double) QuantumRange*pow((value+1.0),QuantumScale*(double)
        pixel-1.0)*MagickSafeReciprocal(value);
      break;
    }
    case LogEvaluateOperator:
    {
      (*result)=EvaluateLog(pixel,value,(*result));
      break;
    }
    case PowEvaluateOperator:
    {
      (*result)=EvaluatePow(pixel,value,(*result));
      break;
    }
    case SineEvaluateOperator:
    {
      (*result)=(double) QuantumRange*(0.5*sin((double) (2.0*MagickPI*
        QuantumScale*(double) pixel*value))+0.5);
      break;
    }
  }
}

static void ApplyArithmeticEvaluateOperator(const Quantum pixel,
  const MagickEvaluateOperator op,const double value,double *result)
{
  switch (op)
  {
    default:
    {
      ApplyTranscendentalEvaluateOperator(pixel,op,value,result);
      break;
    }
    case DivideEvaluateOperator:
    {
      (*result)=(double) pixel/(value == 0.0 ? 1.0 : value);
      break;
    }
    case MultiplyEvaluateOperator:
    {
      (*result)=(double) pixel*value;
      break;
    }
    case RootMeanSquareEvaluateOperator:
    {
      (*result)=((double) pixel*(double) pixel+value);
      break;
    }
    case SubtractEvaluateOperator:
    {
      (*result)=(double) pixel-value;
      break;
    }
    case SumEvaluateOperator:
    {
      (*result)=(double) pixel+value;
      break;
    }
  }
}

typedef struct _EvaluateOperands
{
  RandomInfo
    *random_info;

  Quantum
    pixel;

  MagickEvaluateOperator
    op;

  double
    value;
} EvaluateOperands;

static void ApplyNoiseEvaluateOperator(const EvaluateOperands *operands,
  double *result)
{
  RandomInfo
    *const random_info = operands->random_info;

  const Quantum
    pixel = operands->pixel;

  const MagickEvaluateOperator
    op = operands->op;

  const double
    value = operands->value;

  switch (op)
  {
    default:
    {
      ApplyArithmeticEvaluateOperator(pixel,op,value,result);
      break;
    }
    case GaussianNoiseEvaluateOperator:
    {
      (*result)=(double) GenerateDifferentialNoise(random_info,pixel,
        GaussianNoise,value);
      break;
    }
    case ImpulseNoiseEvaluateOperator:
    {
      (*result)=(double) GenerateDifferentialNoise(random_info,pixel,
        ImpulseNoise,value);
      break;
    }
    case LaplacianNoiseEvaluateOperator:
    {
      (*result)=(double) GenerateDifferentialNoise(random_info,pixel,
        LaplacianNoise,value);
      break;
    }
    case MultiplicativeNoiseEvaluateOperator:
    {
      (*result)=(double) GenerateDifferentialNoise(random_info,pixel,
        MultiplicativeGaussianNoise,value);
      break;
    }
    case PoissonNoiseEvaluateOperator:
    {
      (*result)=(double) GenerateDifferentialNoise(random_info,pixel,
        PoissonNoise,value);
      break;
    }
    case UniformNoiseEvaluateOperator:
    {
      (*result)=(double) GenerateDifferentialNoise(random_info,pixel,
        UniformNoise,value);
      break;
    }
  }
}

static double ApplyEvaluateOperator(RandomInfo *random_info,const Quantum pixel,
  const MagickEvaluateOperator op,const double value)
{
  double
    result;

  result=0.0;
  switch (op)
  {
    default:
    {
      EvaluateOperands
        operands;

      operands.random_info=random_info;
      operands.pixel=pixel;
      operands.op=op;
      operands.value=value;
      ApplyNoiseEvaluateOperator(&operands,&result);
      break;
    }
    case MaxEvaluateOperator:
    {
      result=(double) EvaluateMax((double) pixel,value);
      break;
    }
    case MeanEvaluateOperator:
    {
      result=(double) pixel+value;
      break;
    }
    case MedianEvaluateOperator:
    {
      result=(double) pixel+value;
      break;
    }
    case MinEvaluateOperator:
    {
      result=MagickMin((double) pixel,value);
      break;
    }
    case SetEvaluateOperator:
    {
      result=value;
      break;
    }
  }
  return(result);
}

static Image *AcquireImageCanvas(const Image *images,ExceptionInfo *exception)
{
  const Image
    *p,
    *q;

  size_t
    columns,
    rows;

  q=images;
  columns=images->columns;
  rows=images->rows;
  for (p=images; p != (Image *) NULL; p=p->next)
  {
    if (p->number_channels > q->number_channels)
      q=p;
    if (p->columns > columns)
      columns=p->columns;
    if (p->rows > rows)
      rows=p->rows;
  }
  return(CloneImage(q,columns,rows,MagickTrue,exception));
}

static void ClearPolynomialPixels(const Image *image,
  PixelChannels *polynomial_pixel)
{
  ssize_t
    i,
    j;

  for (j=0; j < (ssize_t) image->columns; j++)
    for (i=0; i < MaxPixelChannels; i++)
      polynomial_pixel[j].channel[i]=0.0;
}

static void ScaleEvaluateProduct(const Image *image,
  const size_t number_images,PixelChannels *evaluate_pixel,const ssize_t x)
{
  ssize_t
    i,
    j;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    for (j=0; j < ((ssize_t) number_images-1); j++)
      evaluate_pixel[x].channel[i]*=QuantumScale;
  }
}

static void NormalizeEvaluatePixels(const Image *image,
  const MagickEvaluateOperator op,const size_t number_images,
  PixelChannels *evaluate_pixel)
{
  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    switch (op)
    {
      case MeanEvaluateOperator:
      {
        for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
          evaluate_pixel[x].channel[i]/=(double) number_images;
        break;
      }
      case MultiplyEvaluateOperator:
      {
        ScaleEvaluateProduct(image,number_images,evaluate_pixel,x);
        break;
      }
      case RootMeanSquareEvaluateOperator:
      {
        for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
          evaluate_pixel[x].channel[i]=sqrt(evaluate_pixel[x].channel[i]/
            number_images);
        break;
      }
      default:
        break;
    }
  }
}

static void SetEvaluateMedianPixel(const Image *image,
  const PixelChannels *evaluate_pixel,const size_t number_images,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    q[i]=ClampToQuantum(evaluate_pixel[number_images/2].channel[i]);
  }
}

static void SetEvaluatePixels(const Image *image,
  const PixelChannels *evaluate_pixel,Quantum *magick_restrict q)
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
      q[i]=ClampToQuantum(evaluate_pixel[x].channel[i]);
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

typedef struct _EvaluateRow
{
  Image
    *image;

  const Image
    *next;

  RandomInfo
    **magick_restrict random_info;

  int
    id;

  MagickEvaluateOperator
    op;

  ssize_t
    j;
} EvaluateRow;

static void AccumulateEvaluateRow(const EvaluateRow *row,const Quantum **p,
  PixelChannels *evaluate_pixel)
{
  Image
    *const image = row->image;

  const Image
    *const next = row->next;

  RandomInfo
    **magick_restrict const random_info = row->random_info;

  const int
    id = row->id;

  const MagickEvaluateOperator
    op = row->op;

  const ssize_t
    j = row->j;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    for (i=0; i < (ssize_t) GetPixelChannels(next); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,i);
      PixelTrait traits = GetPixelChannelTraits(next,channel);
      PixelTrait evaluate_traits = GetPixelChannelTraits(image,channel);
      if (((traits & UpdatePixelTrait) == 0) ||
          ((evaluate_traits & UpdatePixelTrait) == 0))
        continue;
      evaluate_pixel[x].channel[i]=ApplyEvaluateOperator(
        random_info[id],GetPixelChannel(next,channel,p[j]),j == 0 ?
        AddEvaluateOperator : op,evaluate_pixel[x].channel[i]);
    }
    p[j]+=(ptrdiff_t) GetPixelChannels(next);
  }
}

static void AccumulateEvaluateMedianPixel(const EvaluateRow *row,
  const Quantum **p,PixelChannels *evaluate_pixel)
{
  ssize_t
    i;

  for (i=0; i < MaxPixelChannels; i++)
    evaluate_pixel[row->j].channel[i]=0.0;
  for (i=0; i < (ssize_t) GetPixelChannels(row->image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(row->image,i);
    PixelTrait traits = GetPixelChannelTraits(row->next,channel);
    PixelTrait evaluate_traits = GetPixelChannelTraits(row->image,channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((evaluate_traits & UpdatePixelTrait) == 0))
      continue;
    evaluate_pixel[row->j].channel[i]=ApplyEvaluateOperator(
      row->random_info[row->id],GetPixelChannel(row->next,channel,p[row->j]),
      row->op,evaluate_pixel[row->j].channel[i]);
  }
  p[row->j]+=(ptrdiff_t) GetPixelChannels(row->next);
}

typedef struct _EvaluateMedian
{
  Image
    *image;

  const Image
    *images;

  size_t
    number_images;

  RandomInfo
    **magick_restrict random_info;

  int
    id;

  MagickEvaluateOperator
    op;
} EvaluateMedian;

static void EvaluateMedianRow(const EvaluateMedian *median,const Quantum **p,
  PixelChannels *evaluate_pixel,Quantum *magick_restrict q)
{
  ssize_t
    j,
    x;

  for (x=0; x < (ssize_t) median->image->columns; x++)
  {
    const Image
      *next;

    EvaluateRow
      row;

    next=median->images;
    for (j=0; j < (ssize_t) median->number_images; j++)
    {
      row.image=median->image;
      row.next=next;
      row.random_info=median->random_info;
      row.id=median->id;
      row.op=median->op;
      row.j=j;
      AccumulateEvaluateMedianPixel(&row,p,evaluate_pixel);
      next=GetNextImageInList(next);
    }
    qsort((void *) evaluate_pixel,median->number_images,sizeof(*evaluate_pixel),
      IntensityCompare);
    SetEvaluateMedianPixel(median->image,evaluate_pixel,median->number_images,
      q);
    q+=(ptrdiff_t) GetPixelChannels(median->image);
  }
}

typedef struct _EvaluateImagesRows
{
  Image
    *image;

  const Image
    *images;

  size_t
    number_images;

  RandomInfo
    **magick_restrict random_info;

  int
    id;

  MagickEvaluateOperator
    op;
} EvaluateImagesRows;

static void AccumulateEvaluateRows(const EvaluateImagesRows *rows,
  const Quantum **p,PixelChannels *evaluate_pixel)
{
  Image
    *const image = rows->image;

  const Image
    *const images = rows->images;

  const size_t
    number_images = rows->number_images;

  RandomInfo
    **magick_restrict const random_info = rows->random_info;

  const int
    id = rows->id;

  const MagickEvaluateOperator
    op = rows->op;

  const Image
    *next;

  EvaluateRow
    row;

  ssize_t
    j;

  next=images;
  for (j=0; j < (ssize_t) number_images; j++)
  {
    row.image=image;
    row.next=next;
    row.random_info=random_info;
    row.id=id;
    row.op=op;
    row.j=j;
    AccumulateEvaluateRow(&row,p,evaluate_pixel);
    next=GetNextImageInList(next);
  }
}

typedef struct _EvaluateFetch
{
  CacheView
    **image_view;

  Image
    *image;

  ssize_t
    y;

  size_t
    number_images;
} EvaluateFetch;

static ssize_t GetEvaluateRows(const EvaluateFetch *fetch,const Quantum **p,
  ExceptionInfo *exception)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) fetch->number_images; j++)
  {
    p[j]=GetCacheViewVirtualPixels(fetch->image_view[j],0,fetch->y,
      fetch->image->columns,1,exception);
    if (p[j] == (const Quantum *) NULL)
      break;
  }
  return(j);
}

static void AcquireEvaluateViews(const Image *images,
  const size_t number_images,CacheView **image_view,ExceptionInfo *exception)
{
  const Image
    *view;

  ssize_t
    n;

  view=images;
  for (n=0; n < (ssize_t) number_images; n++)
  {
    image_view[n]=AcquireVirtualCacheView(view,exception);
    view=GetNextImageInList(view);
  }
}

static void DestroyEvaluateViews(const size_t number_images,
  CacheView **image_view)
{
  ssize_t
    n;

  for (n=0; n < (ssize_t) number_images; n++)
    image_view[n]=DestroyCacheView(image_view[n]);
}

static MagickBooleanType AcquireDirectImageCanvas(const Image *images,
  Image **image,ExceptionInfo *exception)
{
  *image=AcquireImageCanvas(images,exception);
  if (*image == (Image *) NULL)
    return(MagickFalse);
  if (SetImageStorageClass(*image,DirectClass,exception) == MagickFalse)
    {
      *image=DestroyImage(*image);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType SetProgress(const Image *images,
  const Image *image,const char *tag,const MagickOffsetType progress)
{
  MagickBooleanType
    proceed,
    status;

  status=MagickTrue;
  proceed=SetImageProgress(images,tag,progress,image->rows);
  if (proceed == MagickFalse)
    status=MagickFalse;
  return(status);
}

static Image *FinishEvaluateImage(Image *image,
  const MagickBooleanType status)
{
  if (status == MagickFalse)
    image=DestroyImage(image);
  return(image);
}

typedef struct _EvaluateCanvasTargets
{
  Image
    **image;

  size_t
    *number_images;

  PixelChannels
    **magick_restrict *evaluate_pixels;
} EvaluateCanvasTargets;

static MagickBooleanType AcquireEvaluateCanvas(const Image *images,
  const EvaluateCanvasTargets *targets,ExceptionInfo *exception)
{
  Image
    **const image = targets->image;

  size_t
    *const number_images = targets->number_images;

  PixelChannels
    **magick_restrict *const evaluate_pixels = targets->evaluate_pixels;

  if (AcquireDirectImageCanvas(images,image,exception) == MagickFalse)
    return(MagickFalse);
  *number_images=GetImageListLength(images);
  *evaluate_pixels=AcquirePixelTLS(images);
  if (*evaluate_pixels == (PixelChannels **) NULL)
    {
      *image=DestroyImage(*image);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",images->filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

#define EvaluateImageTag  "Evaluate/Image"

static void SetEvaluateProgress(const Image *images,const Image *image,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  if (SetProgress(images,image,EvaluateImageTag,progress) == MagickFalse)
    (*status)=MagickFalse;
}

static void SyncEvaluateView(CacheView *evaluate_view,
  ExceptionInfo *exception,MagickBooleanType *status)
{
  if (SyncCacheViewAuthenticPixels(evaluate_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

MagickExport Image *EvaluateImages(const Image *images,
  const MagickEvaluateOperator op,ExceptionInfo *exception)
{
  CacheView
    *evaluate_view,
    **image_view;

  Image
    *image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelChannels
    **magick_restrict evaluate_pixels;

  RandomInfo
    **magick_restrict random_info;

  size_t
    number_images;

  ssize_t
    y;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
  unsigned long
    key;
#endif

  const EvaluateCanvasTargets
    targets = { &image, &number_images, &evaluate_pixels };

  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  if (AcquireEvaluateCanvas(images,&targets,exception) == MagickFalse)
    return((Image *) NULL);
  image_view=(CacheView **) AcquireQuantumMemory(number_images,
    sizeof(*image_view));
  if (image_view == (CacheView **) NULL)
    {
      image=DestroyImage(image);
      evaluate_pixels=DestroyPixelTLS(images,evaluate_pixels);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",images->filename);
      return(image);
    }
  AcquireEvaluateViews(images,number_images,image_view,exception);
  /*
    Evaluate image pixels.
  */
  status=MagickTrue;
  progress=0;
  random_info=AcquireRandomInfoTLS();
  evaluate_view=AcquireAuthenticCacheView(image,exception);
  if (op == MedianEvaluateOperator)
    {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      key=GetRandomSecretKey(random_info[0]);
      #pragma omp parallel for schedule(static) shared(progress,status) \
        magick_number_threads(image,images,image->rows,key == ~0UL)
#endif
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        const int
          id = GetOpenMPThreadId();

        const Quantum
          **p;

        EvaluateFetch
          fetch;

        EvaluateMedian
          median;

        PixelChannels
          *evaluate_pixel;

        Quantum
          *magick_restrict q;

        ssize_t
          j;

        if (status == MagickFalse)
          continue;
        p=(const Quantum **) AcquireQuantumMemory(number_images,sizeof(*p));
        if (p == (const Quantum **) NULL)
          {
            status=MagickFalse;
            (void) ThrowMagickException(exception,GetMagickModule(),
              ResourceLimitError,"MemoryAllocationFailed","`%s'",
              images->filename);
            continue;
          }
        fetch.image_view=image_view;
        fetch.image=image;
        fetch.y=y;
        fetch.number_images=number_images;
        j=GetEvaluateRows(&fetch,p,exception);
        q=QueueCacheViewAuthenticPixels(evaluate_view,0,y,image->columns,1,
          exception);
        if ((j < (ssize_t) number_images) || (q == (Quantum *) NULL))
          {
            status=MagickFalse;
            continue;
          }
        evaluate_pixel=evaluate_pixels[id];
        median.image=image;
        median.images=images;
        median.number_images=number_images;
        median.random_info=random_info;
        median.id=id;
        median.op=op;
        EvaluateMedianRow(&median,p,evaluate_pixel,q);
        p=(const Quantum **) RelinquishMagickMemory((void *) p);
        SyncEvaluateView(evaluate_view,exception,&status);
        if (images->progress_monitor != (MagickProgressMonitor) NULL)
          {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
            #pragma omp atomic
#endif
            progress++;
            SetEvaluateProgress(images,image,progress,&status);
          }
      }
    }
  else
    {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      key=GetRandomSecretKey(random_info[0]);
      #pragma omp parallel for schedule(static) shared(progress,status) \
        magick_number_threads(image,images,image->rows,key == ~0UL)
#endif
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        const int
          id = GetOpenMPThreadId();

        const Quantum
          **p;

        EvaluateFetch
          fetch;

        EvaluateImagesRows
          rows;

        PixelChannels
          *evaluate_pixel;

        Quantum
          *magick_restrict q;

        ssize_t
          j;

        if (status == MagickFalse)
          continue;
        p=(const Quantum **) AcquireQuantumMemory(number_images,sizeof(*p));
        if (p == (const Quantum **) NULL)
          {
            status=MagickFalse;
            (void) ThrowMagickException(exception,GetMagickModule(),
              ResourceLimitError,"MemoryAllocationFailed","`%s'",
              images->filename);
            continue;
          }
        fetch.image_view=image_view;
        fetch.image=image;
        fetch.y=y;
        fetch.number_images=number_images;
        j=GetEvaluateRows(&fetch,p,exception);
        q=QueueCacheViewAuthenticPixels(evaluate_view,0,y,image->columns,1,
          exception);
        if ((j < (ssize_t) number_images) || (q == (Quantum *) NULL))
          {
            status=MagickFalse;
            continue;
          }
        evaluate_pixel=evaluate_pixels[id];
        ClearPolynomialPixels(image,evaluate_pixel);
        rows.image=image;
        rows.images=images;
        rows.number_images=number_images;
        rows.random_info=random_info;
        rows.id=id;
        rows.op=op;
        AccumulateEvaluateRows(&rows,p,evaluate_pixel);
        NormalizeEvaluatePixels(image,op,number_images,evaluate_pixel);
        SetEvaluatePixels(image,evaluate_pixel,q);
        p=(const Quantum **) RelinquishMagickMemory((void *) p);
        SyncEvaluateView(evaluate_view,exception,&status);
        if (images->progress_monitor != (MagickProgressMonitor) NULL)
          {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
            #pragma omp atomic
#endif
            progress++;
            SetEvaluateProgress(images,image,progress,&status);
          }
      }
    }
  DestroyEvaluateViews(number_images,image_view);
  image_view=(CacheView **) RelinquishMagickMemory(image_view);
  evaluate_view=DestroyCacheView(evaluate_view);
  evaluate_pixels=DestroyPixelTLS(images,evaluate_pixels);
  random_info=DestroyRandomInfoTLS(random_info);
  return(FinishEvaluateImage(image,status));
}

static void EvaluatePixel(const Image *image,RandomInfo *random_info,
  const EvaluateOperation *operation,Quantum *magick_restrict q)
{
  const MagickEvaluateOperator
    op = operation->op;

  const double
    value = operation->value;

  const MagickBooleanType
    clamp = operation->clamp;

  double
    result;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    result=ApplyEvaluateOperator(random_info,q[i],op,value);
    if (op == MeanEvaluateOperator)
      result/=2.0;
    q[i]=clamp != MagickFalse ? ClampPixel(result) : ClampToQuantum(result);
  }
}

static MagickBooleanType GetEvaluateClamp(const Image *image)
{
  const char
    *artifact;

  MagickBooleanType
    clamp;

  clamp=MagickFalse;
  artifact=GetImageArtifact(image,"evaluate:clamp");
  if (artifact != (const char *) NULL)
    clamp=IsStringTrue(artifact);
  return(clamp);
}

static void EvaluateRowPixels(const Image *image,RandomInfo *random_info,
  const EvaluateOperation operation,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    EvaluatePixel(image,random_info,&operation,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

MagickExport MagickBooleanType EvaluateImage(Image *image,
  const MagickEvaluateOperator op,const double value,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    clamp,
    status;

  MagickOffsetType
    progress;

  RandomInfo
    **magick_restrict random_info;

  ssize_t
    y;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
  unsigned long
    key;
#endif

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  status=MagickTrue;
  progress=0;
  clamp=GetEvaluateClamp(image);
  random_info=AcquireRandomInfoTLS();
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  key=GetRandomSecretKey(random_info[0]);
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,key == ~0UL)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const int
      id = GetOpenMPThreadId();

    const EvaluateOperation
      operation = { op, value, clamp };

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    EvaluateRowPixels(image,random_info[id],operation,q);
    SyncEvaluateView(image_view,exception,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEvaluateProgress(image,image,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  random_info=DestroyRandomInfoTLS(random_info);
  return(status);
}



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

    PixelChannels
      *polynomial_pixel;

    PolynomialRows
      rows;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=QueueCacheViewAuthenticPixels(polynomial_view,0,y,image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
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
    SyncEvaluateView(polynomial_view,exception,&status);
    if (images->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        if (SetProgress(images,image,PolynomialImageTag,progress) ==
            MagickFalse)
          status=MagickFalse;
      }
  }
  polynomial_view=DestroyCacheView(polynomial_view);
  polynomial_pixels=DestroyPixelTLS(images,polynomial_pixels);
  if (status == MagickFalse)
    image=DestroyImage(image);
  return(image);
}

