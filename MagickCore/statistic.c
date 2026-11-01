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

MagickExport Image *EvaluateImages(const Image *images,
  const MagickEvaluateOperator op,ExceptionInfo *exception)
{
#define EvaluateImageTag  "Evaluate/Image"

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

  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  if (AcquireDirectImageCanvas(images,&image,exception) == MagickFalse)
    return((Image *) NULL);
  number_images=GetImageListLength(images);
  evaluate_pixels=AcquirePixelTLS(images);
  if (evaluate_pixels == (PixelChannels **) NULL)
    {
      image=DestroyImage(image);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",images->filename);
      return((Image *) NULL);
    }
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
        if (SyncCacheViewAuthenticPixels(evaluate_view,exception) == MagickFalse)
          status=MagickFalse;
        if (images->progress_monitor != (MagickProgressMonitor) NULL)
          {
            MagickBooleanType
              proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
            #pragma omp atomic
#endif
            progress++;
            proceed=SetImageProgress(images,EvaluateImageTag,progress,
              image->rows);
            if (proceed == MagickFalse)
              status=MagickFalse;
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
        if (SyncCacheViewAuthenticPixels(evaluate_view,exception) == MagickFalse)
          status=MagickFalse;
        if (images->progress_monitor != (MagickProgressMonitor) NULL)
          {
            MagickBooleanType
              proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
            #pragma omp atomic
#endif
            progress++;
            proceed=SetImageProgress(images,EvaluateImageTag,progress,
              image->rows);
            if (proceed == MagickFalse)
              status=MagickFalse;
          }
      }
    }
  DestroyEvaluateViews(number_images,image_view);
  image_view=(CacheView **) RelinquishMagickMemory(image_view);
  evaluate_view=DestroyCacheView(evaluate_view);
  evaluate_pixels=DestroyPixelTLS(images,evaluate_pixels);
  random_info=DestroyRandomInfoTLS(random_info);
  if (status == MagickFalse)
    image=DestroyImage(image);
  return(image);
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
      EvaluatePixel(image,random_info[id],&operation,q);
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
        proceed=SetImageProgress(image,EvaluateImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
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
%   G e t I m a g e E n t r o p y                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageEntropy() returns the entropy of one or more image channels.
%
%  The format of the GetImageEntropy method is:
%
%      MagickBooleanType GetImageEntropy(const Image *image,double *entropy,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o entropy: the average entropy of the selected channels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType GetImageEntropy(const Image *image,
  double *entropy,ExceptionInfo *exception)
{
  ChannelStatistics
    *channel_statistics;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  channel_statistics=GetImageStatistics(image,exception);
  if (channel_statistics == (ChannelStatistics *) NULL)
    {
      *entropy=NAN;
      return(MagickFalse);
    }
  *entropy=channel_statistics[CompositePixelChannel].entropy;
  channel_statistics=(ChannelStatistics *) RelinquishMagickMemory(
    channel_statistics);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e E x t r e m a                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageExtrema() returns the extrema of one or more image channels.
%
%  The format of the GetImageExtrema method is:
%
%      MagickBooleanType GetImageExtrema(const Image *image,size_t *minima,
%        size_t *maxima,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o minima: the minimum value in the channel.
%
%    o maxima: the maximum value in the channel.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType GetImageExtrema(const Image *image,
  size_t *minima,size_t *maxima,ExceptionInfo *exception)
{
  double
    max,
    min;

  MagickBooleanType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=GetImageRange(image,&min,&max,exception);
  *minima=(size_t) ceil(min-0.5);
  *maxima=(size_t) floor(max+0.5);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e K u r t o s i s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageKurtosis() returns the kurtosis and skewness of one or more image
%  channels.
%
%  The format of the GetImageKurtosis method is:
%
%      MagickBooleanType GetImageKurtosis(const Image *image,double *kurtosis,
%        double *skewness,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o kurtosis: the kurtosis of the channel.
%
%    o skewness: the skewness of the channel.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType GetImageKurtosis(const Image *image,
  double *kurtosis,double *skewness,ExceptionInfo *exception)
{
  ChannelStatistics
    *channel_statistics;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  channel_statistics=GetImageStatistics(image,exception);
  if (channel_statistics == (ChannelStatistics *) NULL)
    {
      *kurtosis=NAN;
      *skewness=NAN;
      return(MagickFalse);
    }
  *kurtosis=channel_statistics[CompositePixelChannel].kurtosis;
  *skewness=channel_statistics[CompositePixelChannel].skewness;
  channel_statistics=(ChannelStatistics *) RelinquishMagickMemory(
    channel_statistics);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e M e a n                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageMean() returns the mean and standard deviation of one or more image
%  channels.
%
%  The format of the GetImageMean method is:
%
%      MagickBooleanType GetImageMean(const Image *image,double *mean,
%        double *standard_deviation,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o mean: the average value in the channel.
%
%    o standard_deviation: the standard deviation of the channel.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType GetImageMean(const Image *image,double *mean,
  double *standard_deviation,ExceptionInfo *exception)
{
  ChannelStatistics
    *channel_statistics;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  channel_statistics=GetImageStatistics(image,exception);
  if (channel_statistics == (ChannelStatistics *) NULL)
    {
      *mean=NAN;
      *standard_deviation=NAN;
      return(MagickFalse);
    }
  *mean=channel_statistics[CompositePixelChannel].mean;
  *standard_deviation=
    channel_statistics[CompositePixelChannel].standard_deviation;
  channel_statistics=(ChannelStatistics *) RelinquishMagickMemory(
    channel_statistics);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e M e d i a n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageMedian() returns the median pixel of one or more image channels.
%
%  The format of the GetImageMedian method is:
%
%      MagickBooleanType GetImageMedian(const Image *image,double *median,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o median: the average value in the channel.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType GetImageMedian(const Image *image,double *median,
  ExceptionInfo *exception)
{
  ChannelStatistics
    *channel_statistics;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  channel_statistics=GetImageStatistics(image,exception);
  if (channel_statistics == (ChannelStatistics *) NULL)
    {
      *median=NAN;
      return(MagickFalse);
    }
  *median=channel_statistics[CompositePixelChannel].median;
  channel_statistics=(ChannelStatistics *) RelinquishMagickMemory(
    channel_statistics);
  return(MagickTrue);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e C h a n n e l P e r c e p t u a l H a s h                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImagePerceptualHash() returns the perceptual hash of one or more
%  image channels.
%
%  The format of the GetImagePerceptualHash method is:
%
%      ChannelPerceptualHash *GetImagePerceptualHash(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SetPerceptualHashes(ChannelPerceptualHash *perceptual_hash,
  const ChannelMoments *moments,const ssize_t i)
{
  size_t
    j;

  ssize_t
    channel;

  for (channel=0; channel <= MaxPixelChannels; channel++)
    for (j=0; j < MaximumNumberOfPerceptualHashes; j++)
      perceptual_hash[channel].phash[i][j]=(-MagickSafeLog10(fabs(
        moments[channel].invariant[j])));
}

static char *AcquirePerceptualColorspaces(const char *artifact)
{
  char
    *colorspaces;

  if (artifact != (const char *) NULL)
    colorspaces=AcquireString(artifact);
  else
    colorspaces=AcquireString("xyY,HSB");
  return(colorspaces);
}

MagickExport ChannelPerceptualHash *GetImagePerceptualHash(const Image *image,
  ExceptionInfo *exception)
{
  ChannelPerceptualHash
    *perceptual_hash;

  char
    *colorspaces,
    *p,
    *q;

  const char
    *artifact;

  MagickBooleanType
    status;

  ssize_t
    i;

  perceptual_hash=(ChannelPerceptualHash *) AcquireQuantumMemory(
    MaxPixelChannels+1UL,sizeof(*perceptual_hash));
  if (perceptual_hash == (ChannelPerceptualHash *) NULL)
    return((ChannelPerceptualHash *) NULL);
  (void) memset(perceptual_hash,0,(MaxPixelChannels+1UL)*
    sizeof(*perceptual_hash));
  artifact=GetImageArtifact(image,"phash:colorspaces");
  colorspaces=AcquirePerceptualColorspaces(artifact);
  perceptual_hash[0].number_colorspaces=0;
  perceptual_hash[0].number_channels=0;
  q=colorspaces;
  for (i=0; (p=StringToken(",",&q)) != (char *) NULL; i++)
  {
    ChannelMoments
      *moments;

    Image
      *hash_image;

    ssize_t
      colorspace;

    if (i >= MaximumNumberOfPerceptualColorspaces)
      break;
    colorspace=ParseCommandOption(MagickColorspaceOptions,MagickFalse,p);
    if (colorspace < 0)
      break;
    perceptual_hash[0].colorspace[i]=(ColorspaceType) colorspace;
    hash_image=BlurImage(image,0.0,1.0,exception);
    if (hash_image == (Image *) NULL)
      break;
    status=TransformImageColorspace(hash_image,(ColorspaceType) colorspace,
      exception);
    if (status == MagickFalse)
      {
        hash_image=DestroyImage(hash_image);
        break;
      }
    moments=GetImageMoments(hash_image,exception);
    if (moments == (ChannelMoments *) NULL)
      {
        hash_image=DestroyImage(hash_image);
        break;
      }
    perceptual_hash[0].number_colorspaces++;
    perceptual_hash[0].number_channels+=GetImageChannels(hash_image);
    SetPerceptualHashes(perceptual_hash,moments,i);
    hash_image=DestroyImage(hash_image);
    moments=(ChannelMoments *) RelinquishMagickMemory(moments);
  }
  colorspaces=DestroyString(colorspaces);
  return(perceptual_hash);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e R a n g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageRange() returns the range of one or more image channels.
%
%  The format of the GetImageRange method is:
%
%      MagickBooleanType GetImageRange(const Image *image,double *minima,
%        double *maxima,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o minima: the minimum value in the channel.
%
%    o maxima: the maximum value in the channel.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void AccumulateChannelRange(const Image *image,
  const Quantum *magick_restrict p,double *channel_maxima,
  double *channel_minima)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    if ((double) p[i] > (*channel_maxima))
      (*channel_maxima)=(double) p[i];
    if ((double) p[i] < (*channel_minima))
      (*channel_minima)=(double) p[i];
  }
}

MagickExport MagickBooleanType GetImageRange(const Image *image,double *minima,
  double *maxima,ExceptionInfo *exception)
{
  typedef struct
  {
    double
      maxima,
      minima;
  } RangeInfo;

  CacheView
    *image_view;

  MagickBooleanType
    status;

  RangeInfo
    range_info = { -MagickMaximumValue, MagickMaximumValue };

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=MagickTrue;
  image_view=AcquireVirtualCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(range_info,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    RangeInfo
      channel_range;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    channel_range=range_info;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      AccumulateChannelRange(image,p,&channel_range.maxima,
        &channel_range.minima);
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp critical (MagickCore_GetImageRange)
#endif
    {
      if (channel_range.maxima > range_info.maxima)
        range_info.maxima=channel_range.maxima;
      if (channel_range.minima < range_info.minima)
        range_info.minima=channel_range.minima;
    }
  }
  image_view=DestroyCacheView(image_view);
  *maxima=range_info.maxima;
  *minima=range_info.minima;
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e S t a t i s t i c s                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageStatistics() returns statistics for each channel in the image.  The
%  statistics include the channel depth, its minima, maxima, mean, standard
%  deviation, kurtosis and skewness.  You can access the red channel mean, for
%  example, like this:
%
%      channel_statistics=GetImageStatistics(image,exception);
%      red_mean=channel_statistics[RedPixelChannel].mean;
%
%  Use MagickRelinquishMemory() to free the statistics buffer.
%
%  The format of the GetImageStatistics method is:
%
%      ChannelStatistics *GetImageStatistics(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/

#define SwapPixels(alpha,beta) \
{ \
  Quantum gamma=(alpha); \
  (alpha)=(beta);(beta)=gamma; \
}

static void SetMedianPivot(Quantum *pixels,const ssize_t low,
  const ssize_t mid,const ssize_t high)
{
  if (pixels[mid] > pixels[high])
    SwapPixels(pixels[mid],pixels[high]);
  if (pixels[low] > pixels[high])
    SwapPixels(pixels[low], pixels[high]);
  if (pixels[mid] > pixels[low])
    SwapPixels(pixels[mid],pixels[low]);
  SwapPixels(pixels[mid],pixels[low+1]);
}

static ssize_t SortMedianPair(Quantum *pixels,const ssize_t low,
  const ssize_t high,const ssize_t median)
{
  if (pixels[low] > pixels[high])
    SwapPixels(pixels[low],pixels[high]);
  return(median);
}

static void PartitionMedianPixels(Quantum *pixels,const ssize_t low,
  ssize_t *l,ssize_t *h)
{
  for ( ; ; )
  {
    do (*l)++; while (pixels[low] > pixels[*l]);
    do (*h)--; while (pixels[*h] > pixels[low]);
    if (*h < *l)
      break;
    SwapPixels(pixels[*l],pixels[*h]);
  }
}

static ssize_t GetMedianPixel(Quantum *pixels,const size_t n)
{
  ssize_t
    low = 0,
    high = (ssize_t) n-1,
    median = (low+high)/2;

  for ( ; ; )
  {
    ssize_t
      l = low+1,
      h = high,
      mid = (low+high)/2;

    if (high <= low)
      return(median);
    if (high == (low+1))
      return(SortMedianPair(pixels,low,high,median));
    SetMedianPivot(pixels,low,mid,high);
    PartitionMedianPixels(pixels,low,&l,&h);
    SwapPixels(pixels[low],pixels[h]);
    if (h <= median)
      low=l;
    if (h >= median)
      high=h-1;
  }
}

static inline long double MagickSafeReciprocalLD(const long double x)
{
  long double
    sign;

  /*
    Return 1/x where x is perceptible (not unlimited or infinitesimal).
  */
  sign=x < 0.0 ? -1.0 : 1.0;
  if ((sign*x) >= MagickEpsilon)
    return(1.0/x);
  return(sign/MagickEpsilon);
}

static void UpdateWelfordMoments(ChannelStatistics *cs,
  const Quantum *magick_restrict p,const ssize_t i)
{
  /* Calculate running totals for Welford's method.
  */
  double
    n = cs->area,
    n1 = cs->area-1;

  long double
    delta,
    delta_n,
    delta_n2,
    term1;

  delta=(double) p[i]-cs->M1;
  delta_n=delta/n;
  delta_n2=delta_n*delta_n;
  term1=delta*delta_n*n1;
  cs->M4+=term1*delta_n2*(n*n-3.0*n+3.0)+6.0*delta_n2*cs->M2-4.0*
    delta_n*cs->M3;
  cs->M3+=term1*delta_n*(n-2.0)-3.0*delta_n*cs->M2;
  cs->M2+=term1;
  cs->M1+=delta_n;
}

static void ComputeChannelHigherMoments(ChannelStatistics *cs,
  const double adj_area)
{
  if (cs->area > 1.0)
    cs->standard_deviation=(double) sqrtl(cs->M2/((long double)
      cs->area-1.0));
  else
    cs->standard_deviation=(double) sqrtl(cs->M2/((long double)
      cs->area));
  cs->variance=cs->standard_deviation*cs->standard_deviation;
  cs->skewness=(double) (sqrtl(cs->area)*cs->M3/powl(cs->M2*adj_area,
    1.5));
  cs->kurtosis=(double) (cs->area*cs->M4/(cs->M2*cs->M2*adj_area*
    adj_area)-3.0);
}

static void ComputeChannelMoments(ChannelStatistics *cs)
{
  double
    adj_area = 1.0;

  cs->mean=0.0;
  if (cs->area > 0)
    {
      cs->mean=(double) (cs->sumLD/(long double) cs->area);
      if (cs->area > 1.0)
        adj_area=cs->area/(cs->area-1.0);
    }
  cs->sum=(double) cs->sum;
  if (cs->M2 == 0.0)
    {
      cs->standard_deviation=0.0;
      cs->variance=0.0;
      cs->skewness=0.0;
      cs->kurtosis=0.0;
    }
  else
    ComputeChannelHigherMoments(cs,adj_area);
}

static void ResetChannelStatistics(ChannelStatistics *channel_statistics)
{
  ssize_t
    i;

  for (i=0; i <= (ssize_t) MaxPixelChannels; i++)
  {
    ChannelStatistics *cs = channel_statistics+i;
    cs->area=0.0;
    cs->depth=1;
    cs->maxima=(-MagickMaximumValue);
    cs->minima=MagickMaximumValue;
    cs->sum=0.0;
    cs->sumLD=0.0;
    cs->mean=0.0;
    cs->standard_deviation=0.0;
    cs->variance=0.0;
    cs->skewness=0.0;
    cs->kurtosis=0.0;
    cs->entropy=0.0;
  }
}

static double CountHistogramBins(const Image *image,const double *histogram,
  const ssize_t i)
{
  double
    number_bins;

  ssize_t
    j;

  number_bins=0.0;
  for (j=0; j <= (ssize_t) MaxMap; j++)
    if (histogram[(ssize_t) GetPixelChannels(image)*j+i] > 0.0)
      number_bins++;
  return(number_bins);
}

static void NormalizeChannelSums(ChannelStatistics *cs)
{
  if (cs->area > 0.0)
    {
      cs->sum/=cs->area;
      cs->sum_squared/=cs->area;
      cs->sum_cubed/=cs->area;
      cs->sum_fourth_power/=cs->area;
    }
}

static void ComputeCompositeStatistics(const Image *image,
  ChannelStatistics *channel_statistics)
{
  ChannelStatistics
    *cs_comp;

  double
    channels;

  ssize_t
    i;

  cs_comp=channel_statistics+CompositePixelChannel;
  cs_comp->sum=0.0;
  cs_comp->sum_squared=0.0;
  cs_comp->sum_cubed=0.0;
  cs_comp->sum_fourth_power=0.0;
  cs_comp->maxima=(-MagickMaximumValue);
  cs_comp->minima=MagickMaximumValue;
  cs_comp->area=0.0;
  cs_comp->mean=0.0;
  cs_comp->median=0.0;
  cs_comp->variance=0.0;
  cs_comp->standard_deviation=0.0;
  cs_comp->entropy=0.0;
  cs_comp->skewness=0.0;
  cs_comp->kurtosis=0.0;
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    ChannelStatistics
      *cs;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    cs=channel_statistics+channel;
    if (cs_comp->maxima < cs->maxima)
      cs_comp->maxima=cs->maxima;
    if (cs_comp->minima > cs->minima)
      cs_comp->minima=cs->minima;
    cs_comp->sum+=cs->sum;
    cs_comp->sum_squared+=cs->sum_squared;
    cs_comp->sum_cubed+=cs->sum_cubed;
    cs_comp->sum_fourth_power+=cs->sum_fourth_power;
    cs_comp->median+=cs->median;
    cs_comp->area+=cs->area;
    cs_comp->mean+=cs->mean;
    cs_comp->variance+=cs->variance;
    cs_comp->standard_deviation+=cs->standard_deviation;
    cs_comp->skewness+=cs->skewness;
    cs_comp->kurtosis+=cs->kurtosis;
    cs_comp->entropy+=cs->entropy;
  }
  channels=(double) GetImageChannels(image);
  cs_comp->sum/=channels;
  cs_comp->sum_squared/=channels;
  cs_comp->sum_cubed/=channels;
  cs_comp->sum_fourth_power/=channels;
  cs_comp->median/=channels;
  cs_comp->area/=channels;
  cs_comp->mean/=channels;
  cs_comp->variance/=channels;
  cs_comp->standard_deviation/=channels;
  cs_comp->skewness/=channels;
  cs_comp->kurtosis/=channels;
  cs_comp->entropy/=channels;
}

static void ComputeChannelStatisticsMoments(const Image *image,
  ChannelStatistics *channel_statistics,const ssize_t i)
{
  ChannelStatistics
    *cs;

  PixelChannel channel = GetPixelChannelChannel(image,i);
  PixelTrait traits = GetPixelChannelTraits(image,channel);
  if ((traits & UpdatePixelTrait) == 0)
    return;
  cs=channel_statistics+(ssize_t) channel;
  ComputeChannelMoments(cs);
}

static void ComputeChannelEntropy(const Image *image,const double *histogram,
  ChannelStatistics *channel_statistics,const ssize_t i)
{
  ChannelStatistics
    *cs;

  double
    number_bins;

  long double
    area;

  ssize_t
    j;

  PixelChannel channel = GetPixelChannelChannel(image,i);
  PixelTrait traits = GetPixelChannelTraits(image,channel);
  if ((traits & UpdatePixelTrait) == 0)
    return;
  cs=channel_statistics+(ssize_t) channel;
  NormalizeChannelSums(cs);
  /*
    Compute pixel entropy.
  */
  number_bins=CountHistogramBins(image,histogram,i);
  area=MagickSafeReciprocalLD(channel_statistics[channel].area);
  number_bins=(double) MagickSafeReciprocalLD((long double) log2(number_bins));
  for (j=0; j <= (ssize_t) MaxMap; j++)
  {
    double
      entropy,
      count;

    count=(double) (area*histogram[(ssize_t) GetPixelChannels(image)*j+i]);
    entropy=-count*log2(count)*number_bins;
    if (IsNaN(entropy) != 0)
      continue;
    channel_statistics[channel].entropy+=(double) entropy;
    channel_statistics[CompositePixelChannel].entropy+=((double) entropy/
      GetPixelChannels(image));
  }
}

static void IncreaseChannelDepth(ChannelStatistics *channel_statistics,
  ChannelStatistics *cs)
{
  cs->depth++;
  if (cs->depth > channel_statistics[CompositePixelChannel].depth)
    channel_statistics[CompositePixelChannel].depth=cs->depth;
}

typedef struct _ChannelSample
{
  const Image
    *image;

  const Quantum
    *p;

  ssize_t
    i;
} ChannelSample;

static void AccumulateChannelSample(const ChannelSample *sample,
  ChannelStatistics *cs,double *histogram)
{
  const Image
    *const image = sample->image;

  const Quantum
    *const p = sample->p;

  const ssize_t
    i = sample->i;

  cs->area++;
  if ((double) p[i] < cs->minima)
    cs->minima=(double) p[i];
  if ((double) p[i] > cs->maxima)
    cs->maxima=(double) p[i];
  histogram[(ssize_t) GetPixelChannels(image)*ScaleQuantumToMap(
    ClampToQuantum((double) p[i]))+i]++;
  cs->sumLD+=(long double) p[i];
  /*
    sum_squared, sum_cubed and sum_fourth_power are not used in
    MagickCore or MagickWand, but are made available in
    Magick++/lib/Statistic.cpp, so we need to calculate these.
  */
  cs->sum_squared+=(double) p[i]*(double) p[i];
  cs->sum_cubed+=(double) p[i]*(double) p[i]*(double) p[i];
  cs->sum_fourth_power+=(double) p[i]*(double) p[i]*(double) p[i]*
    (double) p[i];
  UpdateWelfordMoments(cs,p,i);
}

typedef struct _MedianRow
{
  const Image
    *image;

  ssize_t
    i;
} MedianRow;

static size_t CollectMedianRow(const MedianRow *row,const Quantum *p,
  Quantum *median,size_t n)
{
  const Image
    *const image = row->image;

  const ssize_t
    i = row->i;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (GetPixelReadMask(image,p) <= (QuantumRange/2))
      {
        p+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    median[n++]=p[i];
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(n);
}

typedef struct _MedianPass
{
  const Image
    *image;

  ExceptionInfo
    *exception;

  Quantum
    *median;

  ssize_t
    i;
} MedianPass;

static void CollectMedianPass(const MedianPass *pass,ssize_t *y,size_t *n)
{
  for (*y=0; *y < (ssize_t) pass->image->rows; (*y)++)
  {
    const Quantum
      *magick_restrict p;

    MedianRow
      row;

    p=GetVirtualPixels(pass->image,0,*y,pass->image->columns,1,
      pass->exception);
    if (p == (const Quantum *) NULL)
      break;
    row.image=pass->image;
    row.i=pass->i;
    *n=CollectMedianRow(&row,p,pass->median,*n);
  }
}

static ChannelStatistics *RelinquishImageStatistics(const Image *image,
  double *histogram,ChannelStatistics *channel_statistics,
  ExceptionInfo *exception)
{
  if (histogram != (double *) NULL)
    histogram=(double *) RelinquishMagickMemory(histogram);
  if (channel_statistics != (ChannelStatistics *) NULL)
    channel_statistics=(ChannelStatistics *) RelinquishMagickMemory(
      channel_statistics);
  (void) ThrowMagickException(exception,GetMagickModule(),
    ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
  return(channel_statistics);
}

typedef struct _MedianStatistics
{
  const Image
    *image;

  ExceptionInfo
    *exception;

  ChannelStatistics
    *channel_statistics;

  MemoryInfo
    *median_info;
} MedianStatistics;

static void ComputeChannelMedians(const MedianStatistics *medians,ssize_t *y)
{
  Quantum
    *median;

  ssize_t
    i;

  median=(Quantum *) GetVirtualMemoryBlob(medians->median_info);
  for (i=0; i < (ssize_t) GetPixelChannels(medians->image); i++)
  {
    MedianPass
      pass;

    size_t
      n = 0;

    /*
      Compute median statistics for each channel.
    */
    PixelChannel channel = GetPixelChannelChannel(medians->image,i);
    PixelTrait traits = GetPixelChannelTraits(medians->image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    pass.image=medians->image;
    pass.exception=medians->exception;
    pass.median=median;
    pass.i=i;
    CollectMedianPass(&pass,y,&n);
    medians->channel_statistics[channel].median=(double) median[
      GetMedianPixel(median,n)];
  }
}

static MagickBooleanType CheckChannelDepth(
  ChannelStatistics *channel_statistics,ChannelStatistics *cs,
  const Quantum *magick_restrict p,const ssize_t i)
{
  MagickStatusType
    status;

  QuantumAny
    range;

  size_t
    depth;

  if (cs->depth != MAGICKCORE_QUANTUM_DEPTH)
    {
      depth=cs->depth;
      range=GetQuantumRange(depth);
      status=p[i] != ScaleAnyToQuantum(ScaleQuantumToAny(p[i],range),
        range) ? MagickTrue : MagickFalse;
      if (status != MagickFalse)
        {
          IncreaseChannelDepth(channel_statistics,cs);
          return(MagickFalse);
        }
    }
  return(MagickTrue);
}

static void AccumulatePixelStatistics(const Image *image,
  const Quantum *magick_restrict p,ChannelStatistics *channel_statistics,
  double *histogram)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    ChannelStatistics
      *cs;

    ChannelSample
      sample;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    cs=channel_statistics+channel;
    if (CheckChannelDepth(channel_statistics,cs,p,i) == MagickFalse)
      {
        i--;
        continue;
      }
    sample.image=image;
    sample.p=p;
    sample.i=i;
    AccumulateChannelSample(&sample,cs,histogram);
  }
}

static void AccumulateRowStatistics(const Image *image,
  const Quantum *magick_restrict p,ChannelStatistics *channel_statistics,
  double *histogram)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (GetPixelReadMask(image,p) <= (QuantumRange/2))
      {
        p+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    AccumulatePixelStatistics(image,p,channel_statistics,histogram);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void FinalizeChannelStatistics(const Image *image,
  const double *histogram,ChannelStatistics *channel_statistics)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    ComputeChannelStatisticsMoments(image,channel_statistics,i);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    ComputeChannelEntropy(image,histogram,channel_statistics,i);
}

MagickExport ChannelStatistics *GetImageStatistics(const Image *image,
  ExceptionInfo *exception)
{
  ChannelStatistics
    *channel_statistics;

  double
    *histogram;

  MedianStatistics
    medians;

  MemoryInfo
    *median_info;

  Quantum
    *median;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  histogram=(double *) AcquireQuantumMemory(MaxMap+1UL,
    MagickMax(GetPixelChannels(image),1)*sizeof(*histogram));
  channel_statistics=(ChannelStatistics *) AcquireQuantumMemory(
    MaxPixelChannels+1,sizeof(*channel_statistics));
  if ((channel_statistics == (ChannelStatistics *) NULL) ||
      (histogram == (double *) NULL))
    return(RelinquishImageStatistics(image,histogram,channel_statistics,
      exception));
  (void) memset(channel_statistics,0,(MaxPixelChannels+1)*
    sizeof(*channel_statistics));
  ResetChannelStatistics(channel_statistics);
  (void) memset(histogram,0,(MaxMap+1)*GetPixelChannels(image)*
    sizeof(*histogram));
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    /*
      Compute pixel statistics.
    */
    p=GetVirtualPixels(image,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    AccumulateRowStatistics(image,p,channel_statistics,histogram);
  }
  FinalizeChannelStatistics(image,histogram,channel_statistics);
  histogram=(double *) RelinquishMagickMemory(histogram);
  median_info=AcquireVirtualMemory(image->columns,image->rows*sizeof(*median));
  if (median_info == (MemoryInfo *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),
      ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
  else
    {
      medians.image=image;
      medians.exception=exception;
      medians.channel_statistics=channel_statistics;
      medians.median_info=median_info;
      ComputeChannelMedians(&medians,&y);
      median_info=RelinquishVirtualMemory(median_info);
    }
  ComputeCompositeStatistics(image,channel_statistics);
  if (y < (ssize_t) image->rows)
    channel_statistics=(ChannelStatistics *) RelinquishMagickMemory(
      channel_statistics);
  return(channel_statistics);
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

MagickExport Image *PolynomialImage(const Image *images,
  const size_t number_terms,const double *terms,ExceptionInfo *exception)
{
#define PolynomialImageTag  "Polynomial/Image"

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

  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (AcquireDirectImageCanvas(images,&image,exception) == MagickFalse)
    return((Image *) NULL);
  number_images=GetImageListLength(images);
  polynomial_pixels=AcquirePixelTLS(images);
  if (polynomial_pixels == (PixelChannels **) NULL)
    {
      image=DestroyImage(image);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",images->filename);
      return((Image *) NULL);
    }
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
    if (SyncCacheViewAuthenticPixels(polynomial_view,exception) == MagickFalse)
      status=MagickFalse;
    if (images->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(images,PolynomialImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  polynomial_view=DestroyCacheView(polynomial_view);
  polynomial_pixels=DestroyPixelTLS(images,polynomial_pixels);
  if (status == MagickFalse)
    image=DestroyImage(image);
  return(image);
}

