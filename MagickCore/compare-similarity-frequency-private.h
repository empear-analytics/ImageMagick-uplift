/*
  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.  You may
  obtain a copy of the License at

    https://imagemagick.org/license/

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore private methods of compare-similarity-image.c: the frequency-domain similarity of SimilarityImage (HDRI with FFTW only), from SIMCrossCorrelationImage to IsFrequencyDomainSimilarity; included by that file only.
*/
#ifndef MAGICKCORE_COMPARE_SIMILARITY_FREQUENCY_PRIVATE_H
#define MAGICKCORE_COMPARE_SIMILARITY_FREQUENCY_PRIVATE_H

#if defined(MAGICKCORE_HDRI_SUPPORT) && defined(MAGICKCORE_FFTW_DELEGATE)
static Image *SIMCrossCorrelationImage(const Image *alpha_image,
  const Image *beta_image,ExceptionInfo *exception)
{
  Image
    *alpha_fft = (Image *) NULL,
    *beta_fft = (Image *) NULL,
    *complex_conjugate = (Image *) NULL,
    *complex_multiplication = (Image *) NULL,
    *cross_correlation = (Image *) NULL,
    *temp_image = (Image *) NULL;

  /*
    Take the FFT of beta (reconstruction) image.
  */
  temp_image=CloneImage(beta_image,0,0,MagickTrue,exception);
  if (temp_image == (Image *) NULL)
    return((Image *) NULL);
  (void) SetImageArtifact(temp_image,"fourier:normalize","inverse");
  beta_fft=ForwardFourierTransformImage(temp_image,MagickFalse,exception);
  temp_image=DestroyImageList(temp_image);
  if (beta_fft == (Image *) NULL)
    return((Image *) NULL);
  /*
    Take the complex conjugate of beta_fft.
  */
  complex_conjugate=ComplexImages(beta_fft,ConjugateComplexOperator,exception);
  beta_fft=DestroyImageList(beta_fft);
  if (complex_conjugate == (Image *) NULL)
    return((Image *) NULL);
  /*
    Take the FFT of the alpha (test) image.
  */
  temp_image=CloneImage(alpha_image,0,0,MagickTrue,exception);
  if (temp_image == (Image *) NULL)
    {
      complex_conjugate=DestroyImageList(complex_conjugate);
      return((Image *) NULL);
    }
  (void) SetImageArtifact(temp_image,"fourier:normalize","inverse");
  alpha_fft=ForwardFourierTransformImage(temp_image,MagickFalse,exception);
  temp_image=DestroyImageList(temp_image);
  if (alpha_fft == (Image *) NULL)
    {
      complex_conjugate=DestroyImageList(complex_conjugate);
      return((Image *) NULL);
    }
  /*
    Do complex multiplication.
  */
  DisableCompositeClampUnlessSpecified(complex_conjugate);
  DisableCompositeClampUnlessSpecified(complex_conjugate->next);
  AppendImageToList(&complex_conjugate,alpha_fft);
  complex_multiplication=ComplexImages(complex_conjugate,
    MultiplyComplexOperator,exception);
  complex_conjugate=DestroyImageList(complex_conjugate);
  if (complex_multiplication == (Image *) NULL)
    return((Image *) NULL);
  /*
    Do the IFT and return the cross-correlation result.
  */
  cross_correlation=InverseFourierTransformImage(complex_multiplication,
    complex_multiplication->next,MagickFalse,exception);
  complex_multiplication=DestroyImageList(complex_multiplication);
  return(cross_correlation);
}

static Image *SIMDerivativeImage(const Image *image,const char *kernel,
  ExceptionInfo *exception)
{
  Image
    *derivative_image;

  KernelInfo
    *kernel_info;

  kernel_info=AcquireKernelInfo(kernel,exception);
  if (kernel_info == (KernelInfo *) NULL)
    return((Image *) NULL);
  derivative_image=MorphologyImage(image,ConvolveMorphology,1,kernel_info,
    exception);
  kernel_info=DestroyKernelInfo(kernel_info);
  return(derivative_image);
}

typedef struct _SIMPairRow
{
  const Image
    *image,
    *source;

  const void
    *arguments;

  ssize_t
    y;
} SIMPairRow;

typedef void
  (*SIMPairRowMethod)(const SIMPairRow *,const Quantum *magick_restrict,
    Quantum *magick_restrict);

typedef struct _SIMPairRows
{
  SIMPairRowMethod
    method;

  const void
    *arguments;
} SIMPairRows;

static void DivideRow(const SIMPairRow *row,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  const Image
    *const divide_image = row->image,
    *const denominator_image = row->source;

  ssize_t
    x;

  for (x=0; x < (ssize_t) divide_image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(divide_image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(divide_image,i);
      PixelTrait traits = GetPixelChannelTraits(divide_image,channel);
      PixelTrait denominator_traits = GetPixelChannelTraits(denominator_image,
        channel);
      if (((traits & UpdatePixelTrait) == 0) ||
          ((denominator_traits & UpdatePixelTrait) == 0))
        continue;
      q[i]=(Quantum) ((double) q[i]*MagickSafeReciprocal(QuantumScale*
        (double) GetPixelChannel(denominator_image,channel,p)));
    }
    p+=(ptrdiff_t) GetPixelChannels(denominator_image);
    q+=(ptrdiff_t) GetPixelChannels(divide_image);
  }
}
static MagickBooleanType SIMImagePairRows(Image *image,const Image *source,
  const SIMPairRows *rows,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *source_view;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    y;

  image_view=AcquireAuthenticCacheView(image,exception);
  source_view=AcquireVirtualCacheView(source,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(source,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    SIMPairRow
      row;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(source_view,0,y,source->columns,1,exception);
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    row.image=image;
    row.source=source;
    row.arguments=rows->arguments;
    row.y=y;
    rows->method(&row,p,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  source_view=DestroyCacheView(source_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}


static inline Image *SIMClonePairRows(const Image *image,const Image *source,
  const SIMPairRowMethod method,ExceptionInfo *exception)
{
  Image
    *clone_image;

  MagickBooleanType
    status = MagickTrue;

  const SIMPairRows
    rows = { method,(const void *) NULL };

  clone_image=CloneImage(image,0,0,MagickTrue,exception);
  if (clone_image == (Image *) NULL)
    return(clone_image);
  status=SIMImagePairRows(clone_image,source,&rows,exception);
  if (status == MagickFalse)
    clone_image=DestroyImage(clone_image);
  return(clone_image);
}

static Image *SIMDivideImage(const Image *numerator_image,
  const Image *denominator_image,ExceptionInfo *exception)
{
  /*
    Divide one image into another.
  */
  return(SIMClonePairRows(numerator_image,denominator_image,DivideRow,
    exception));
}

static Image *SIMDivideByMagnitude(Image *image,Image *magnitude_image,
  const Image *source_image,ExceptionInfo *exception)
{
  Image
    *divide_image,
    *result_image;

  RectangleInfo
    geometry;

  divide_image=SIMDivideImage(image,magnitude_image,exception);
  if (divide_image == (Image *) NULL)
    return((Image *) NULL);
  GetPixelInfoRGBA((Quantum) 0,(Quantum) 0,(Quantum) 0,(Quantum) 0,
    &divide_image->background_color);
  SetGeometry(source_image,&geometry);
  geometry.width=MagickMax(source_image->columns,divide_image->columns);
  geometry.height=MagickMax(source_image->rows,divide_image->rows);
  result_image=ExtentImage(divide_image,&geometry,exception);
  divide_image=DestroyImage(divide_image);
  return(result_image);
}

static void ClearRowNaNs(const Image *image,const void *arguments,
  Quantum *magick_restrict q)
{
  ssize_t
    x;

  magick_unreferenced(arguments);
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,i);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if ((traits & UpdatePixelTrait) == 0)
        continue;
      if (IsNaN((double) q[i]) != 0)
        q[i]=(Quantum) 0;
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

typedef void
  (*SIMRowMethod)(const Image *,const void *,Quantum *magick_restrict);

typedef struct _SIMRows
{
  SIMRowMethod
    method;

  const void
    *arguments;
} SIMRows;

static MagickBooleanType SIMImageRows(Image *image,const SIMRows *rows,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    y;

  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
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
    rows->method(image,rows->arguments,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

static MagickBooleanType SIMFilterImageNaNs(Image *image,
  ExceptionInfo *exception)
{
  const SIMRows
    rows = { ClearRowNaNs,(const void *) NULL };

  /*
    Square each pixel in the image.
  */
  return(SIMImageRows(image,&rows,exception));
}

static void SquarePixelChannels(const Image *square_image,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(square_image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(square_image,i);
    PixelTrait traits = GetPixelChannelTraits(square_image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    q[i]=(Quantum) (QuantumScale*(double) q[i]*(double) q[i]);
  }
}

static void SquareRow(const Image *square_image,const void *arguments,
  Quantum *magick_restrict q)
{
  ssize_t
    x;

  magick_unreferenced(arguments);
  for (x=0; x < (ssize_t) square_image->columns; x++)
  {
    SquarePixelChannels(square_image,q);
    q+=(ptrdiff_t) GetPixelChannels(square_image);
  }
}

static Image *SIMSquareImage(const Image *image,ExceptionInfo *exception)
{
  Image
    *square_image;

  MagickBooleanType
    status = MagickTrue;

  const SIMRows
    rows = { SquareRow,(const void *) NULL };

  /*
    Square each pixel in the image.
  */
  square_image=CloneImage(image,0,0,MagickTrue,exception);
  if (square_image == (Image *) NULL)
    return(square_image);
  status=SIMImageRows(square_image,&rows,exception);
  if (status == MagickFalse)
    square_image=DestroyImage(square_image);
  return(square_image);
}

static Image *SIMMagnitudeImage(Image *alpha_image,Image *beta_image,
  ExceptionInfo *exception)
{
  Image
    *magnitude_image,
    *xsq_image,
    *ysq_image;

  MagickBooleanType
    status = MagickTrue;

  (void) SetImageArtifact(alpha_image,"compose:clamp","False");
  xsq_image=SIMSquareImage(alpha_image,exception);
  if (xsq_image == (Image *) NULL)
    return((Image *) NULL);
  (void) SetImageArtifact(beta_image,"compose:clamp","False");
  ysq_image=SIMSquareImage(beta_image,exception);
  if (ysq_image == (Image *) NULL)
    {
      xsq_image=DestroyImage(xsq_image);
      return((Image *) NULL);
    }
  status=CompositeImage(xsq_image,ysq_image,PlusCompositeOp,MagickTrue,0,0,
    exception);
  magnitude_image=xsq_image;
  ysq_image=DestroyImage(ysq_image);
  if (status == MagickFalse)
    {
      magnitude_image=DestroyImage(magnitude_image);
      return((Image *) NULL);
    }
  status=EvaluateImage(magnitude_image,PowEvaluateOperator,0.5,exception);
  if (status == MagickFalse)
    {
      magnitude_image=DestroyImage(magnitude_image);
      return (Image *) NULL;
    }
  return(magnitude_image);
}

typedef struct
{
  double
    maxima;

  ssize_t
    x,
    y;
} MaximaInfo;

static MaximaInfo SeedMaximaInfo(CacheView *image_view,
  MaximaInfo maxima_info,ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict q;

  q=GetCacheViewVirtualPixels(image_view,maxima_info.x,maxima_info.y,1,1,
    exception);
  if (q != (const Quantum *) NULL)
    maxima_info.maxima=IsNaN((double) q[0]) != 0 ? 0.0 : (double) q[0];
  return(maxima_info);
}

static MaximaInfo FindRowMaxima(const Image *image,
  const Quantum *magick_restrict p,const ssize_t y,MaximaInfo channel_maxima)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      double
        pixel;

      PixelChannel channel = GetPixelChannelChannel(image,i);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if ((traits & UpdatePixelTrait) == 0)
        continue;
      pixel=(double) p[i];
      if (IsNaN(pixel) != 0)
        pixel=0.0;
      if (pixel > channel_maxima.maxima)
        {
          channel_maxima.maxima=(double) p[i];
          channel_maxima.x=x;
          channel_maxima.y=y;
        }
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(channel_maxima);
}

static MagickBooleanType SIMMaximaImage(const Image *image,double *maxima,
  RectangleInfo *offset,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status = MagickTrue;

  MaximaInfo
    maxima_info = { -MagickMaximumValue, 0, 0 };

  ssize_t
    y;

  /*
    Identify the maxima value in the image and its location.
  */
  image_view=AcquireVirtualCacheView(image,exception);
  maxima_info=SeedMaximaInfo(image_view,maxima_info,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(maxima_info,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    MaximaInfo
      channel_maxima = { -MagickMaximumValue, 0, 0 };

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    channel_maxima=maxima_info;
    channel_maxima=FindRowMaxima(image,p,y,channel_maxima);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp critical (MagickCore_SIMMaximaImage)
#endif
    if (channel_maxima.maxima > maxima_info.maxima)
      maxima_info=channel_maxima;
  }
  image_view=DestroyCacheView(image_view);
  *maxima=maxima_info.maxima;
  offset->x=maxima_info.x;
  offset->y=maxima_info.y;
  return(status);
}

typedef struct
{
  double
    minima;

  ssize_t
    x,
    y;
} MinimaInfo;

typedef struct _MinimaPixelInfo
{
  const Image
    *image;

  const Quantum
    *p;

  ssize_t
    x,
    y;
} MinimaPixelInfo;

static MinimaInfo FindPixelMinima(const MinimaPixelInfo *minima_pixel,
  MinimaInfo channel_minima)
{
  const Image
    *const image = minima_pixel->image;

  const Quantum
    *const p = minima_pixel->p;

  const ssize_t
    x = minima_pixel->x,
    y = minima_pixel->y;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      pixel;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    pixel=(double) p[i];
    if (IsNaN(pixel) != 0)
      pixel=0.0;
    if (pixel < channel_minima.minima)
      {
        channel_minima.minima=pixel;
        channel_minima.x=x;
        channel_minima.y=y;
      }
  }
  return(channel_minima);
}

static MagickBooleanType SIMMinimaImage(const Image *image,double *minima,
  RectangleInfo *offset,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  const Quantum
    *magick_restrict q;

  MagickBooleanType
    status = MagickTrue;

  MinimaInfo
    minima_info = { MagickMaximumValue, 0, 0 };

  ssize_t
    y;

  /*
    Identify the minima value in the image and its location.
  */
  image_view=AcquireVirtualCacheView(image,exception);
  q=GetCacheViewVirtualPixels(image_view,minima_info.x,minima_info.y,1,1,
    exception);
  if (q != (const Quantum *) NULL)
    minima_info.minima=IsNaN((double) q[0]) != 0 ? 0.0 : (double) q[0];
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(minima_info,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    MinimaInfo
      channel_minima = { MagickMaximumValue, 0, 0 };

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
    channel_minima=minima_info;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      MinimaPixelInfo
        minima_pixel;

      minima_pixel.image=image;
      minima_pixel.p=p;
      minima_pixel.x=x;
      minima_pixel.y=y;
      channel_minima=FindPixelMinima(&minima_pixel,channel_minima);
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp critical (MagickCore_SIMMinimaImage)
#endif
    if (channel_minima.minima < minima_info.minima)
      minima_info=channel_minima;
  }
  image_view=DestroyCacheView(image_view);
  *minima=minima_info.minima;
  offset->x=minima_info.x;
  offset->y=minima_info.y;
  return(status);
}

static void MultiplyPixelChannels(const Image *image,const double factor,
  const ChannelStatistics *channel_statistics,Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    if (channel_statistics != (const ChannelStatistics *) NULL)
      q[i]=(Quantum) (factor*(double) q[i]*QuantumScale*
        channel_statistics[channel].standard_deviation);
    else
      q[i]=(Quantum) (factor*(double) q[i]);
  }
}

typedef struct _MultiplyRowInfo
{
  double
    factor;

  const ChannelStatistics
    *channel_statistics;
} MultiplyRowInfo;

static void MultiplyRow(const Image *image,const void *arguments,
  Quantum *magick_restrict q)
{
  const MultiplyRowInfo
    *multiply_row = (const MultiplyRowInfo *) arguments;

  const double
    factor = multiply_row->factor;

  const ChannelStatistics
    *channel_statistics = multiply_row->channel_statistics;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    MultiplyPixelChannels(image,factor,channel_statistics,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType SIMMultiplyImage(Image *image,const double factor,
  const ChannelStatistics *channel_statistics,ExceptionInfo *exception)
{
  const MultiplyRowInfo
    multiply_row = { factor, channel_statistics };

  const SIMRows
    rows = { MultiplyRow,&multiply_row };

  /*
    Multiply each pixel by a factor.
  */
  return(SIMImageRows(image,&rows,exception));
}

static Image *SIMPhaseCorrelationImage(const Image *target_image,
  const Image *reconstruct_image,const Image *magnitude_image,
  ExceptionInfo *exception)
{
  Image
    *target_fft = (Image *) NULL,
    *reconstruct_fft = (Image *) NULL,
    *complex_multiplication = (Image *) NULL,
    *cross_correlation = (Image *) NULL;

  /*
    Take the FFT of the reconstruction image.
  */
  reconstruct_fft=CloneImage(reconstruct_image,0,0,MagickTrue,exception);
  if (reconstruct_fft == NULL)
    return((Image *) NULL);
  (void) SetImageArtifact(reconstruct_fft,"fourier:normalize","inverse");
  reconstruct_fft=ForwardFourierTransformImage(reconstruct_fft,MagickFalse,
    exception);
  if (reconstruct_fft == NULL)
    return((Image *) NULL);
  /*
    Take the FFT of the target image.
  */
  target_fft=CloneImage(target_image,0,0,MagickTrue,exception);
  if (target_fft == (Image *) NULL)
    {
      reconstruct_fft=DestroyImageList(reconstruct_fft);
      return((Image *) NULL);
    }
  (void) SetImageArtifact(target_fft,"fourier:normalize","inverse");
  target_fft=ForwardFourierTransformImage(target_fft,MagickFalse,exception);
  if (target_fft == (Image *) NULL)
    {
      reconstruct_fft=DestroyImageList(reconstruct_fft);
      return((Image *) NULL);
    }
  /*
    Take the complex conjugate of the reconstruction FFT.
  */
  reconstruct_fft=ComplexImages(reconstruct_fft,ConjugateComplexOperator,
    exception);
  if (reconstruct_fft == (Image *) NULL)
    {
      target_fft=DestroyImageList(target_fft);
      return((Image *) NULL);
    }
  /*
    Do complex multiplication.
  */
  AppendImageToList(&reconstruct_fft,target_fft);
  DisableCompositeClampUnlessSpecified(reconstruct_fft);
  DisableCompositeClampUnlessSpecified(reconstruct_fft->next);
  complex_multiplication=ComplexImages(reconstruct_fft,MultiplyComplexOperator,
    exception);
  reconstruct_fft=DestroyImageList(reconstruct_fft);
  if (complex_multiplication == (Image *) NULL)
    return((Image *) NULL);
  if (complex_multiplication->next != (Image *) NULL)
    {
      /*
        Normalize the cross-power spectrum by the product magnitude.
      */
      DisableCompositeClampUnlessSpecified(complex_multiplication);
      DisableCompositeClampUnlessSpecified(complex_multiplication->next);
      (void) CompositeImage(complex_multiplication,magnitude_image,
        DivideSrcCompositeOp,MagickTrue,0,0,exception);
      (void) CompositeImage(complex_multiplication->next,magnitude_image,
        DivideSrcCompositeOp,MagickTrue,0,0,exception);
    }
  /*
    Do the IFT and return the phase-correlation result.
  */
  (void) SetImageArtifact(complex_multiplication,"fourier:normalize","inverse");
  cross_correlation=InverseFourierTransformImage(complex_multiplication,
    complex_multiplication->next,MagickFalse,exception);
  complex_multiplication=DestroyImageList(complex_multiplication);
  return(cross_correlation);
}

static void SetPixelChannelsMean(const Image *image,
  const ChannelStatistics *channel_statistics,Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    q[i]=(Quantum) channel_statistics[channel].mean;
  }
}

static void SetMeanRow(const Image *image,const void *arguments,
  Quantum *magick_restrict q)
{
  const ChannelStatistics
    *channel_statistics = (const ChannelStatistics *) arguments;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    SetPixelChannelsMean(image,channel_statistics,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType SIMSetImageMean(Image *image,
  const ChannelStatistics *channel_statistics,ExceptionInfo *exception)
{
  const SIMRows
    rows = { SetMeanRow,channel_statistics };

  /*
    Set image mean.
  */
  return(SIMImageRows(image,&rows,exception));
}

static void SubtractMeanRow(const SIMPairRow *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const subtract_image = row->image,
    *const beta_image = row->source;

  const ChannelStatistics
    *const channel_statistics = (const ChannelStatistics *) row->arguments;

  const ssize_t
    y = row->y;

  ssize_t
    x;

  for (x=0; x < (ssize_t) subtract_image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(subtract_image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(subtract_image,i);
      PixelTrait traits = GetPixelChannelTraits(subtract_image,channel);
      PixelTrait beta_traits = GetPixelChannelTraits(beta_image,channel);
      if (((traits & UpdatePixelTrait) == 0) ||
          ((beta_traits & UpdatePixelTrait) == 0))
        continue;
      if ((x >= (ssize_t) beta_image->columns) ||
          (y >= (ssize_t) beta_image->rows))
        q[i]=(Quantum) 0;
      else
        q[i]=(Quantum) ((double) GetPixelChannel(beta_image,channel,p)-
          channel_statistics[channel].mean);
    }
    p+=(ptrdiff_t) GetPixelChannels(beta_image);
    q+=(ptrdiff_t) GetPixelChannels(subtract_image);
  }
}

static Image *SIMSubtractImageMean(const Image *alpha_image,
  const Image *beta_image,const ChannelStatistics *channel_statistics,
  ExceptionInfo *exception)
{
  Image
    *subtract_image;

  MagickBooleanType
    status = MagickTrue;

  const SIMPairRows
    rows = { SubtractMeanRow,channel_statistics };

  /*
    Subtract the image mean and pad.
  */
  subtract_image=CloneImage(beta_image,alpha_image->columns,alpha_image->rows,
    MagickTrue,exception);
  if (subtract_image == (Image *) NULL)
    return(subtract_image);
  status=SIMImagePairRows(subtract_image,beta_image,&rows,exception);
  if (status == MagickFalse)
    subtract_image=DestroyImage(subtract_image);
  return(subtract_image);
}

static void SetUnityRow(const Image *unity_image,
  const Image *beta_image,const ssize_t y,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) unity_image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(unity_image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(unity_image,i);
      PixelTrait traits = GetPixelChannelTraits(unity_image,channel);
      if ((traits & UpdatePixelTrait) == 0)
        continue;
      if ((x >= (ssize_t) beta_image->columns) ||
          (y >= (ssize_t) beta_image->rows))
        q[i]=(Quantum) 0;
      else
        q[i]=QuantumRange;
    }
    q+=(ptrdiff_t) GetPixelChannels(unity_image);
  }
}

static Image *SIMUnityImage(const Image *alpha_image,const Image *beta_image,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  Image
    *unity_image;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    y;

  /*
    Create a padded unity image.
  */
  unity_image=CloneImage(alpha_image,alpha_image->columns,alpha_image->rows,
    MagickTrue,exception);
  if (unity_image == (Image *) NULL)
    return(unity_image);
  if (SetImageStorageClass(unity_image,DirectClass,exception) == MagickFalse)
    return(DestroyImage(unity_image));
  image_view=AcquireAuthenticCacheView(unity_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(unity_image,unity_image,unity_image->rows,1)
#endif
  for (y=0; y < (ssize_t) unity_image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,unity_image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    SetUnityRow(unity_image,beta_image,y,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  image_view=DestroyCacheView(image_view);
  if (status == MagickFalse)
    unity_image=DestroyImage(unity_image);
  return(unity_image);
}

static void VarianceRow(const SIMPairRow *row,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  const Image
    *const variance_image = row->image,
    *const beta_image = row->source;

  ssize_t
    x;

  for (x=0; x < (ssize_t) variance_image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(variance_image); i++)
    {
      double
        error;

      PixelChannel channel = GetPixelChannelChannel(variance_image,i);
      PixelTrait traits = GetPixelChannelTraits(variance_image,channel);
      PixelTrait beta_traits = GetPixelChannelTraits(beta_image,channel);
      if (((traits & UpdatePixelTrait) == 0) ||
          ((beta_traits & UpdatePixelTrait) == 0))
        continue;
      error=(double) q[i]-(double) GetPixelChannel(beta_image,channel,p);
      q[i]=(Quantum) ((double) ClampToQuantum((double) QuantumRange*
        (sqrt(fabs(QuantumScale*error))/sqrt((double) QuantumRange))));
    }
    p+=(ptrdiff_t) GetPixelChannels(beta_image);
    q+=(ptrdiff_t) GetPixelChannels(variance_image);
  }
}

static Image *SIMVarianceImage(Image *alpha_image,const Image *beta_image,
  ExceptionInfo *exception)
{
  /*
    Compute the variance of the two images.
  */
  return(SIMClonePairRows(alpha_image,beta_image,VarianceRow,exception));
}

#define ThrowDPCSimilarityException() \
{ \
  if (dot_product_image != (Image *) NULL) \
    dot_product_image=DestroyImage(dot_product_image); \
  if (magnitude_image != (Image *) NULL) \
    magnitude_image=DestroyImage(magnitude_image); \
  if (reconstruct_image != (Image *) NULL) \
    reconstruct_image=DestroyImage(reconstruct_image); \
  if (rx_image != (Image *) NULL) \
    rx_image=DestroyImage(rx_image); \
  if (ry_image != (Image *) NULL) \
    ry_image=DestroyImage(ry_image); \
  if (target_image != (Image *) NULL) \
    target_image=DestroyImage(target_image); \
  if (threshold_image != (Image *) NULL) \
    threshold_image=DestroyImage(threshold_image); \
  if (trx_image != (Image *) NULL) \
    trx_image=DestroyImage(trx_image); \
  if (try_image != (Image *) NULL) \
    try_image=DestroyImage(try_image); \
  if (tx_image != (Image *) NULL) \
    tx_image=DestroyImage(tx_image); \
  if (ty_image != (Image *) NULL) \
    ty_image=DestroyImage(ty_image); \
  return((Image *) NULL); \
}

static Image *DPCSimilarityImage(const Image *image,const Image *reconstruct,
  RectangleInfo *offset,double *similarity_metric,ExceptionInfo *exception)
{
  double
    edge_factor = 0.0,
    maxima = 0.0,
    mean = 0.0,
    standard_deviation = 0.0;

  Image
    *dot_product_image = (Image *) NULL,
    *magnitude_image = (Image *) NULL,
    *reconstruct_image = (Image *) NULL,
    *rx_image = (Image *) NULL,
    *ry_image = (Image *) NULL,
    *trx_image = (Image *) NULL,
    *target_image = (Image *) NULL,
    *threshold_image = (Image *) NULL,
    *try_image = (Image *) NULL,
    *tx_image = (Image *) NULL,
    *ty_image = (Image *) NULL;

  MagickBooleanType
    status = MagickTrue;

  RectangleInfo
    geometry;

  /*
    Dot product correlation-based image similarity using FFT local statistics.
  */
  target_image=CloneImage(image,0,0,MagickTrue,exception);
  if (target_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Compute the cross correlation of the test and reconstruct magnitudes.
  */
  reconstruct_image=CloneImage(reconstruct,0,0,MagickTrue,exception);
  if (reconstruct_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  /*
    Compute X and Y derivatives of reference image.
  */
  (void) SetImageVirtualPixelMethod(reconstruct_image,EdgeVirtualPixelMethod,
    exception);
  rx_image=SIMDerivativeImage(reconstruct_image,"Sobel",exception);
  if (rx_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  ry_image=SIMDerivativeImage(reconstruct_image,"Sobel:90",exception);
  reconstruct_image=DestroyImage(reconstruct_image);
  if (ry_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  /*
    Compute magnitude of derivatives.
  */
  magnitude_image=SIMMagnitudeImage(rx_image,ry_image,exception);
  if (magnitude_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  /*
    Compute an edge normalization correction.
  */
  threshold_image=CloneImage(magnitude_image,0,0,MagickTrue,exception);
  if (threshold_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  status=BilevelImage(threshold_image,0.0,exception);
  if (status == MagickFalse)
    ThrowDPCSimilarityException();
  status=GetImageMean(threshold_image,&mean,&standard_deviation,exception);
  threshold_image=DestroyImage(threshold_image);
  if (status == MagickFalse)
    ThrowDPCSimilarityException();
  edge_factor=MagickSafeReciprocal(QuantumScale*mean*(double)
    reconstruct->columns*(double) reconstruct->rows)+QuantumScale;
  /*
    Divide X and Y derivitives of reference image by magnitude.
  */
  trx_image=SIMDivideByMagnitude(rx_image,magnitude_image,image,exception);
  rx_image=DestroyImage(rx_image);
  if (trx_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  rx_image=trx_image;
  try_image=SIMDivideByMagnitude(ry_image,magnitude_image,image,exception);
  magnitude_image=DestroyImage(magnitude_image);
  ry_image=DestroyImage(ry_image);
  if (try_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  ry_image=try_image;
  /*
    Compute X and Y derivatives of image.
  */
  (void) SetImageVirtualPixelMethod(target_image,EdgeVirtualPixelMethod,
    exception);
  tx_image=SIMDerivativeImage(target_image,"Sobel",exception);
  if (tx_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  ty_image=SIMDerivativeImage(target_image,"Sobel:90",exception);
  target_image=DestroyImage(target_image);
  if (ty_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  /*
    Compute magnitude of derivatives.
  */
  magnitude_image=SIMMagnitudeImage(tx_image,ty_image,exception);
  if (magnitude_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  /*
    Divide Lx and Ly by magnitude.
  */
  trx_image=SIMDivideByMagnitude(tx_image,magnitude_image,image,exception);
  tx_image=DestroyImage(tx_image);
  if (trx_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  tx_image=trx_image;
  try_image=SIMDivideByMagnitude(ty_image,magnitude_image,image,exception);
  ty_image=DestroyImage(ty_image);
  magnitude_image=DestroyImage(magnitude_image);
  if (try_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  ty_image=try_image;
  /*
    Compute the cross correlation of the test and reference images.
  */
  trx_image=SIMCrossCorrelationImage(tx_image,rx_image,exception);
  rx_image=DestroyImage(rx_image);
  tx_image=DestroyImage(tx_image);
  if (trx_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  try_image=SIMCrossCorrelationImage(ty_image,ry_image,exception);
  ry_image=DestroyImage(ry_image);
  ty_image=DestroyImage(ty_image);
  if (try_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  /*
    Evaluate dot product correlation image.
  */
  (void) SetImageArtifact(try_image,"compose:clamp","false");
  status=CompositeImage(trx_image,try_image,PlusCompositeOp,MagickTrue,0,0,
    exception);
  try_image=DestroyImage(try_image);
  if (status == MagickFalse)
    ThrowDPCSimilarityException();
  status=SIMMultiplyImage(trx_image,edge_factor,
    (const ChannelStatistics *) NULL,exception);
  if (status == MagickFalse)
    ThrowDPCSimilarityException();
  /*
    Crop results.
  */
  SetGeometry(image,&geometry);
  geometry.width=image->columns;
  geometry.height=image->rows;
  (void) ResetImagePage(trx_image,"0x0+0+0");
  dot_product_image=CropImage(trx_image,&geometry,exception);
  trx_image=DestroyImage(trx_image);
  if (dot_product_image == (Image *) NULL)
    ThrowDPCSimilarityException();
  (void) ResetImagePage(dot_product_image,"0x0+0+0");
  /*
    Identify the maxima value in the image and its location.
  */
  status=GrayscaleImage(dot_product_image,AveragePixelIntensityMethod,
    exception);
  if (status == MagickFalse)
    ThrowDPCSimilarityException();
  dot_product_image->depth=32;
  dot_product_image->colorspace=GRAYColorspace;
  dot_product_image->alpha_trait=UndefinedPixelTrait;
  status=SIMFilterImageNaNs(dot_product_image,exception);
  if (status == MagickFalse)
    ThrowDPCSimilarityException();
  status=SIMMaximaImage(dot_product_image,&maxima,offset,exception);
  if (status == MagickFalse)
    ThrowDPCSimilarityException();
  if ((QuantumScale*maxima) > 1.0)
    {
      status=SIMMultiplyImage(dot_product_image,1.0/(QuantumScale*maxima),
        (const ChannelStatistics *) NULL,exception);
      maxima=(double) QuantumRange;
    }
  *similarity_metric=QuantumScale*maxima;
  return(dot_product_image);
}

#define ThrowMSESimilarityException() \
{ \
  if (alpha_image != (Image *) NULL) \
    alpha_image=DestroyImage(alpha_image); \
  if (beta_image != (Image *) NULL) \
    beta_image=DestroyImage(beta_image); \
  if (channel_statistics != (ChannelStatistics *) NULL) \
    channel_statistics=(ChannelStatistics *) \
      RelinquishMagickMemory(channel_statistics); \
  if (mean_image != (Image *) NULL) \
    mean_image=DestroyImage(mean_image); \
  if (mse_image != (Image *) NULL) \
    mse_image=DestroyImage(mse_image); \
  if (reconstruct_image != (Image *) NULL) \
    reconstruct_image=DestroyImage(reconstruct_image); \
  if (sum_image != (Image *) NULL) \
    sum_image=DestroyImage(sum_image); \
  if (alpha_image != (Image *) NULL) \
    alpha_image=DestroyImage(alpha_image); \
  return((Image *) NULL); \
}

static Image *MSESimilarityImage(const Image *image,const Image *reconstruct,
  RectangleInfo *offset,double *similarity_metric,ExceptionInfo *exception)
{
  ChannelStatistics
    *channel_statistics = (ChannelStatistics *) NULL;

  double
    minima = 0.0;

  Image
    *alpha_image = (Image *) NULL,
    *beta_image = (Image *) NULL,
    *mean_image = (Image *) NULL,
    *mse_image = (Image *) NULL,
    *reconstruct_image = (Image *) NULL,
    *sum_image = (Image *) NULL,
    *target_image = (Image *) NULL;

  MagickBooleanType
    status = MagickTrue;

  RectangleInfo
    geometry;

  /*
    MSE correlation-based image similarity using FFT local statistics.
  */
  target_image=SIMSquareImage(image,exception);
  if (target_image == (Image *) NULL)
    ThrowMSESimilarityException();
  reconstruct_image=SIMUnityImage(image,reconstruct,exception);
  if (reconstruct_image == (Image *) NULL)
    ThrowMSESimilarityException();
  /*
    Create (U * test)/# pixels.
  */
  alpha_image=SIMCrossCorrelationImage(target_image,reconstruct_image,
    exception);
  target_image=DestroyImage(target_image);
  if (alpha_image == (Image *) NULL)
    ThrowMSESimilarityException();
  status=SIMMultiplyImage(alpha_image,1.0/(double) reconstruct->columns/(double)
    reconstruct->rows,(const ChannelStatistics *) NULL,exception);
  if (status == MagickFalse)
    ThrowMSESimilarityException();
  /*
    Create 2*(test * reconstruction)# pixels.
  */
  (void) CompositeImage(reconstruct_image,reconstruct,CopyCompositeOp,
    MagickTrue,0,0,exception);
  beta_image=SIMCrossCorrelationImage(image,reconstruct_image,exception);
  if (beta_image == (Image *) NULL)
    {
      reconstruct_image=DestroyImage(reconstruct_image);
      ThrowMSESimilarityException();
    }
  status=SIMMultiplyImage(beta_image,-2.0/(double) reconstruct->columns/(double)
    reconstruct->rows,(const ChannelStatistics *) NULL,exception);
  reconstruct_image=DestroyImage(reconstruct_image);
  if (status == MagickFalse)
    ThrowMSESimilarityException();
  /*
    Mean of reconstruction squared.
  */
  sum_image=SIMSquareImage(reconstruct,exception);
  if (sum_image == (Image *) NULL)
    ThrowMSESimilarityException();
  channel_statistics=GetImageStatistics(sum_image,exception);
  if (channel_statistics == (ChannelStatistics *) NULL)
    ThrowMSESimilarityException();
  status=SetImageStorageClass(sum_image,DirectClass,exception);
  if (status == MagickFalse)
    ThrowMSESimilarityException();
  status=SIMSetImageMean(sum_image,channel_statistics,exception);
  channel_statistics=(ChannelStatistics *)
    RelinquishMagickMemory(channel_statistics);
  if (status == MagickFalse)
    ThrowMSESimilarityException();
  /*
    Create mean image.
  */
  AppendImageToList(&sum_image,alpha_image);
  AppendImageToList(&sum_image,beta_image);
  mean_image=EvaluateImages(sum_image,SumEvaluateOperator,exception);
  if (mean_image == (Image *) NULL)
    ThrowMSESimilarityException();
  sum_image=DestroyImage(sum_image);
  status=GrayscaleImage(mean_image,AveragePixelIntensityMethod,exception);
  if (status == MagickFalse)
    ThrowMSESimilarityException();
  /*
    Crop to difference of reconstruction and test images.
  */
  SetGeometry(image,&geometry);
  geometry.width=image->columns;
  geometry.height=image->rows;
  (void) ResetImagePage(mean_image,"0x0+0+0");
  mse_image=CropImage(mean_image,&geometry,exception);
  mean_image=DestroyImage(mean_image);
  if (mse_image == (Image *) NULL)
    ThrowMSESimilarityException();
  /*
    Identify the minima value in the correlation image and its location.
  */
  (void) ResetImagePage(mse_image,"0x0+0+0");
  (void) ClampImage(mse_image,exception);
  mse_image->depth=32;
  mse_image->colorspace=GRAYColorspace;
  mse_image->alpha_trait=UndefinedPixelTrait;
  status=SIMMinimaImage(mse_image,&minima,offset,exception);
  if (status == MagickFalse)
    ThrowMSESimilarityException();
  status=NegateImage(mse_image,MagickFalse,exception);
  if (status == MagickFalse)
    ThrowMSESimilarityException();
  alpha_image=DestroyImage(alpha_image);
  beta_image=DestroyImage(beta_image);
  if ((QuantumScale*minima) < (double) FLT_EPSILON)
    minima=0.0;
  *similarity_metric=QuantumScale*minima;
  return(mse_image);
}

#define ThrowNCCSimilarityException() \
{ \
  if (alpha_image != (Image *) NULL) \
    alpha_image=DestroyImage(alpha_image); \
  if (beta_image != (Image *) NULL) \
    beta_image=DestroyImage(beta_image); \
  if (channel_statistics != (ChannelStatistics *) NULL) \
    channel_statistics=(ChannelStatistics *) \
      RelinquishMagickMemory(channel_statistics); \
  if (correlation_image != (Image *) NULL) \
    correlation_image=DestroyImage(correlation_image); \
  if (divide_image != (Image *) NULL) \
    divide_image=DestroyImage(divide_image); \
  if (ncc_image != (Image *) NULL) \
    ncc_image=DestroyImage(ncc_image); \
  if (normalize_image != (Image *) NULL) \
    normalize_image=DestroyImage(normalize_image); \
  if (reconstruct_image != (Image *) NULL) \
    reconstruct_image=DestroyImage(reconstruct_image); \
  if (target_image != (Image *) NULL) \
    target_image=DestroyImage(target_image); \
  if (variance_image != (Image *) NULL) \
    variance_image=DestroyImage(variance_image); \
  return((Image *) NULL); \
}

static Image *NCCSimilarityImage(const Image *image,const Image *reconstruct,
  RectangleInfo *offset,double *similarity_metric,ExceptionInfo *exception)
{
  ChannelStatistics
    *channel_statistics = (ChannelStatistics *) NULL;

  double
    maxima = 0.0;

  Image
    *alpha_image = (Image *) NULL,
    *beta_image = (Image *) NULL,
    *correlation_image = (Image *) NULL,
    *divide_image = (Image *) NULL,
    *ncc_image = (Image *) NULL,
    *normalize_image = (Image *) NULL,
    *reconstruct_image = (Image *) NULL,
    *target_image = (Image *) NULL,
    *variance_image = (Image *) NULL;

  MagickBooleanType
    status = MagickTrue;

  RectangleInfo
    geometry;

  /*
    NCC correlation-based image similarity with FFT local statistics.
  */
  target_image=SIMSquareImage(image,exception);
  if (target_image == (Image *) NULL)
    ThrowNCCSimilarityException();
  reconstruct_image=SIMUnityImage(image,reconstruct,exception);
  if (reconstruct_image == (Image *) NULL)
    ThrowNCCSimilarityException();
  /*
    Compute the cross correlation of the test and reconstruction images.
  */
  alpha_image=SIMCrossCorrelationImage(target_image,reconstruct_image,
    exception);
  target_image=DestroyImage(target_image);
  if (alpha_image == (Image *) NULL)
    ThrowNCCSimilarityException();
  status=SIMMultiplyImage(alpha_image,(double) QuantumRange*
    (double) reconstruct->columns*(double) reconstruct->rows,
    (const ChannelStatistics *) NULL,exception);
  if (status == MagickFalse)
    ThrowNCCSimilarityException();
  /*
    Compute the cross correlation of the source and reconstruction images.
  */
  beta_image=SIMCrossCorrelationImage(image,reconstruct_image,exception);
  reconstruct_image=DestroyImage(reconstruct_image);
  if (beta_image == (Image *) NULL)
    ThrowNCCSimilarityException();
  target_image=SIMSquareImage(beta_image,exception);
  beta_image=DestroyImage(beta_image);
  if (target_image == (Image *) NULL)
    ThrowNCCSimilarityException();
  status=SIMMultiplyImage(target_image,(double) QuantumRange,
    (const ChannelStatistics *) NULL,exception);
  if (status == MagickFalse)
    ThrowNCCSimilarityException();
  /*
    Compute the variance of the two images.
  */
  variance_image=SIMVarianceImage(alpha_image,target_image,exception);
  target_image=DestroyImage(target_image);
  alpha_image=DestroyImage(alpha_image);
  if (variance_image == (Image *) NULL)
    ThrowNCCSimilarityException();
  /*
    Subtract the image mean.
  */
  channel_statistics=GetImageStatistics(reconstruct,exception);
  if (channel_statistics == (ChannelStatistics *) NULL)
    ThrowNCCSimilarityException();
  status=SIMMultiplyImage(variance_image,1.0,channel_statistics,exception);
  if (status == MagickFalse)
    ThrowNCCSimilarityException();
  normalize_image=SIMSubtractImageMean(image,reconstruct,channel_statistics,
    exception);
  channel_statistics=(ChannelStatistics *)
    RelinquishMagickMemory(channel_statistics);
  if (normalize_image == (Image *) NULL)
    ThrowNCCSimilarityException();
  correlation_image=SIMCrossCorrelationImage(image,normalize_image,exception);
  normalize_image=DestroyImage(normalize_image);
  if (correlation_image == (Image *) NULL)
    ThrowNCCSimilarityException();
  /*
    Divide the two images.
  */
  divide_image=SIMDivideImage(correlation_image,variance_image,exception);
  correlation_image=DestroyImage(correlation_image);
  variance_image=DestroyImage(variance_image);
  if (divide_image == (Image *) NULL)
    ThrowNCCSimilarityException();
  /*
    Crop padding.
  */
  SetGeometry(image,&geometry);
  geometry.width=image->columns;
  geometry.height=image->rows;
  (void) ResetImagePage(divide_image,"0x0+0+0");
  ncc_image=CropImage(divide_image,&geometry,exception);
  divide_image=DestroyImage(divide_image);
  if (ncc_image == (Image *) NULL)
    ThrowNCCSimilarityException();
  /*
    Identify the maxima value in the image and its location.
  */
  (void) ResetImagePage(ncc_image,"0x0+0+0");
  status=GrayscaleImage(ncc_image,AveragePixelIntensityMethod,exception);
  if (status == MagickFalse)
    ThrowNCCSimilarityException();
  ncc_image->depth=32;
  ncc_image->colorspace=GRAYColorspace;
  ncc_image->alpha_trait=UndefinedPixelTrait;
  status=SIMMaximaImage(ncc_image,&maxima,offset,exception);
  if (status == MagickFalse)
    ThrowNCCSimilarityException();
  if ((QuantumScale*maxima) > 1.0)
    {
      status=SIMMultiplyImage(ncc_image,1.0/(QuantumScale*maxima),
        (const ChannelStatistics *) NULL,exception);
      maxima=(double) QuantumRange;
    }
  *similarity_metric=QuantumScale*maxima;
  return(ncc_image);
}

#define ThrowPhaseSimilarityException() \
{ \
  if (phase_image != (Image *) NULL) \
    phase_image=DestroyImage(phase_image); \
  if (gamma_image != (Image *) NULL) \
    gamma_image=DestroyImage(gamma_image); \
  if (test_magnitude != (Image *) NULL) \
    test_magnitude=DestroyImage(test_magnitude); \
  if (magnitude_image != (Image *) NULL) \
    magnitude_image=DestroyImage(magnitude_image); \
  if (reconstruct_magnitude != (Image *) NULL) \
    reconstruct_magnitude=DestroyImage(reconstruct_magnitude); \
  if (correlation_image != (Image *) NULL) \
    correlation_image=DestroyImage(correlation_image); \
  if (fft_images != (Image *) NULL) \
    fft_images=DestroyImageList(fft_images); \
  if (reconstruct_image != (Image *) NULL) \
    reconstruct_image=DestroyImage(reconstruct_image); \
  if (target_image != (Image *) NULL) \
    target_image=DestroyImage(target_image); \
  return((Image *) NULL); \
}

static Image *PhaseSimilarityImage(const Image *image,const Image *reconstruct,
  RectangleInfo *offset,double *similarity_metric,ExceptionInfo *exception)
{
  double
    maxima = 0.0;

  Image
    *correlation_image = (Image *) NULL,
    *fft_images = (Image *) NULL,
    *gamma_image = (Image *) NULL,
    *magnitude_image = (Image *) NULL,
    *phase_image = (Image *) NULL,
    *reconstruct_image = (Image *) NULL,
    *reconstruct_magnitude = (Image *) NULL,
    *target_image = (Image *) NULL,
    *test_magnitude = (Image *) NULL;

  MagickBooleanType
    status = MagickTrue;

  RectangleInfo
    geometry;

  /*
    Phase correlation-based image similarity using FFT local statistics.
  */
  target_image=CloneImage(image,0,0,MagickTrue,exception);
  if (target_image == (Image *) NULL)
    ThrowPhaseSimilarityException();
  (void) ResetImagePage(target_image,"0x0+0+0");
  GetPixelInfoRGBA((Quantum) 0,(Quantum) 0,(Quantum) 0,(Quantum) 0,
    &target_image->background_color);
  status=SetImageExtent(target_image,2*CastDoubleToSizeT(ceil((double)
    image->columns/2.0)),2*CastDoubleToSizeT(ceil((double) image->rows/2.0)),
    exception);
  if (status == MagickFalse)
    ThrowPhaseSimilarityException();
  /*
    Compute the cross correlation of the target and reconstruct magnitudes.
  */
  reconstruct_image=CloneImage(reconstruct,0,0,MagickTrue,exception);
  if (reconstruct_image == (Image *) NULL)
    ThrowPhaseSimilarityException();
  (void) ResetImagePage(reconstruct_image,"0x0+0+0");
  GetPixelInfoRGBA((Quantum) 0,(Quantum) 0,(Quantum) 0,(Quantum) 0,
    &reconstruct_image->background_color);
  status=SetImageExtent(reconstruct_image,2*CastDoubleToSizeT(ceil((double)
    image->columns/2.0)),2*CastDoubleToSizeT(ceil((double) image->rows/2.0)),
    exception);
  if (status == MagickFalse)
    ThrowPhaseSimilarityException();
  /*
    Evaluate phase coorelation image and divide by the product magnitude.
  */
  (void) SetImageArtifact(target_image,"fourier:normalize","inverse");
  fft_images=ForwardFourierTransformImage(target_image,MagickTrue,exception);
  if (fft_images == (Image *) NULL)
    ThrowPhaseSimilarityException();
  test_magnitude=CloneImage(fft_images,0,0,MagickTrue,exception);
  fft_images=DestroyImageList(fft_images);
  if (test_magnitude == (Image *) NULL)
    ThrowPhaseSimilarityException();
  (void) SetImageArtifact(reconstruct_image,"fourier:normalize","inverse");
  fft_images=ForwardFourierTransformImage(reconstruct_image,MagickTrue,
    exception);
  if (fft_images == (Image *) NULL)
    ThrowPhaseSimilarityException();
  reconstruct_magnitude=CloneImage(fft_images,0,0,MagickTrue,exception);
  fft_images=DestroyImageList(fft_images);
  if (reconstruct_magnitude == (Image *) NULL)
    ThrowPhaseSimilarityException();
  magnitude_image=CloneImage(reconstruct_magnitude,0,0,MagickTrue,exception);
  if (magnitude_image == (Image *) NULL)
    ThrowPhaseSimilarityException();
  DisableCompositeClampUnlessSpecified(magnitude_image);
  (void) CompositeImage(magnitude_image,test_magnitude,MultiplyCompositeOp,
    MagickTrue,0,0,exception);
  /*
    Compute the cross correlation of the target and reconstruction images.
  */
  correlation_image=SIMPhaseCorrelationImage(target_image,reconstruct_image,
    magnitude_image,exception);
  target_image=DestroyImage(target_image);
  reconstruct_image=DestroyImage(reconstruct_image);
  test_magnitude=DestroyImage(test_magnitude);
  reconstruct_magnitude=DestroyImage(reconstruct_magnitude);
  if (correlation_image == (Image *) NULL)
    ThrowPhaseSimilarityException();
  gamma_image=CloneImage(correlation_image,0,0,MagickTrue,exception);
  correlation_image=DestroyImage(correlation_image);
  if (gamma_image == (Image *) NULL)
    ThrowPhaseSimilarityException();
  /*
    Crop padding.
  */
  SetGeometry(image,&geometry);
  geometry.width=image->columns;
  geometry.height=image->rows;
  (void) ResetImagePage(gamma_image,"0x0+0+0");
  phase_image=CropImage(gamma_image,&geometry,exception);
  gamma_image=DestroyImage(gamma_image);
  if (phase_image == (Image *) NULL)
    ThrowPhaseSimilarityException();
  (void) ResetImagePage(phase_image,"0x0+0+0");
  /*
    Identify the maxima value in the correlation image and its location.
  */
  status=GrayscaleImage(phase_image,AveragePixelIntensityMethod,exception);
  if (status == MagickFalse)
    ThrowPhaseSimilarityException();
  phase_image->depth=32;
  phase_image->colorspace=GRAYColorspace;
  phase_image->alpha_trait=UndefinedPixelTrait;
  status=SIMFilterImageNaNs(phase_image,exception);
  if (status == MagickFalse)
    ThrowPhaseSimilarityException();
  status=SIMMaximaImage(phase_image,&maxima,offset,exception);
  if (status == MagickFalse)
    ThrowPhaseSimilarityException();
  magnitude_image=DestroyImage(magnitude_image);
  *similarity_metric=QuantumScale*maxima;
  return(phase_image);
}

static Image *PSNRSimilarityImage(const Image *image,const Image *reconstruct,
  RectangleInfo *offset,double *similarity_metric,ExceptionInfo *exception)
{
  Image
    *psnr_image = (Image *) NULL;

  psnr_image=MSESimilarityImage(image,reconstruct,offset,similarity_metric,
    exception);
  if (psnr_image == (Image *) NULL)
    return(psnr_image);
  *similarity_metric=10.0*MagickSafeLog10(MagickSafeReciprocal(
    *similarity_metric))/MagickSafePSNRRecipicol(10.0);
  return(psnr_image);
}

static Image *RMSESimilarityImage(const Image *image,const Image *reconstruct,
  RectangleInfo *offset,double *similarity_metric,ExceptionInfo *exception)
{
  Image
    *rmse_image = (Image *) NULL;

  rmse_image=MSESimilarityImage(image,reconstruct,offset,similarity_metric,
    exception);
  if (rmse_image == (Image *) NULL)
    return(rmse_image);
  *similarity_metric=sqrt(*similarity_metric);
  return(rmse_image);
}

typedef Image
  *(*FrequencySimilarityMethod)(const Image *,const Image *,RectangleInfo *,
    double *,ExceptionInfo *);

typedef struct _FrequencySimilarity
{
  MetricType
    metric;

  FrequencySimilarityMethod
    similarity_image;
} FrequencySimilarity;

static const FrequencySimilarity
  FrequencySimilarities[] =
  {
    { DotProductCorrelationErrorMetric, DPCSimilarityImage },
    { MeanSquaredErrorMetric, MSESimilarityImage },
    { NormalizedCrossCorrelationErrorMetric, NCCSimilarityImage },
    { PeakSignalToNoiseRatioErrorMetric, PSNRSimilarityImage },
    { PhaseCorrelationErrorMetric, PhaseSimilarityImage },
    { RootMeanSquaredErrorMetric, RMSESimilarityImage },
    { UndefinedErrorMetric, RMSESimilarityImage }
  };

static const FrequencySimilarity *GetFrequencySimilarity(
  const MetricType metric)
{
  size_t
    i;

  for (i=0; i < (sizeof(FrequencySimilarities)/
       sizeof(*FrequencySimilarities)); i++)
    if (FrequencySimilarities[i].metric == metric)
      return(FrequencySimilarities+i);
  return((const FrequencySimilarity *) NULL);
}

static inline MagickBooleanType IsFrequencyDomainSimilarity(
  const Image *image,const char *artifact)
{
  return((((artifact == (const char *) NULL) ||
    (IsStringTrue(artifact) != MagickFalse)) &&
    ((image->channels & ReadMaskChannel) == 0)) ? MagickTrue : MagickFalse);
}
#endif

#endif
