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

  MagickCore private methods of compare-similarity-image.c: the frequency-domain similarity of SimilarityImage (HDRI with FFTW only), from ThrowDPCSimilarityException to IsFrequencyDomainSimilarity, over the image helpers of compare-similarity-frequency-images-private.h, which that file includes directly above this header; included by that file only.
*/
#ifndef MAGICKCORE_COMPARE_SIMILARITY_FREQUENCY_PRIVATE_H
#define MAGICKCORE_COMPARE_SIMILARITY_FREQUENCY_PRIVATE_H

#if defined(MAGICKCORE_HDRI_SUPPORT) && defined(MAGICKCORE_FFTW_DELEGATE)
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

typedef struct _DPCReconstructGradient
{
  Image
    **magnitude_image,
    **reconstruct_image,
    **rx_image,
    **ry_image;
} DPCReconstructGradient;

static MagickBooleanType AcquireDPCReconstructGradient(
  const Image *reconstruct,ExceptionInfo *exception,
  const DPCReconstructGradient *targets)
{
  Image
    **const magnitude_image = targets->magnitude_image,
    **const reconstruct_image = targets->reconstruct_image,
    **const rx_image = targets->rx_image,
    **const ry_image = targets->ry_image;

  /*
    Compute the cross correlation of the test and reconstruct magnitudes.
  */
  *reconstruct_image=CloneImage(reconstruct,0,0,MagickTrue,exception);
  if (*reconstruct_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Compute X and Y derivatives of reference image.
  */
  (void) SetImageVirtualPixelMethod(*reconstruct_image,EdgeVirtualPixelMethod,
    exception);
  *rx_image=SIMDerivativeImage(*reconstruct_image,"Sobel",exception);
  if (*rx_image == (Image *) NULL)
    return(MagickFalse);
  *ry_image=SIMDerivativeImage(*reconstruct_image,"Sobel:90",exception);
  *reconstruct_image=DestroyImage(*reconstruct_image);
  if (*ry_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Compute magnitude of derivatives.
  */
  *magnitude_image=SIMMagnitudeImage(*rx_image,*ry_image,exception);
  if (*magnitude_image == (Image *) NULL)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _DPCEdgeCorrection
{
  double
    *edge_factor;

  Image
    **threshold_image;
} DPCEdgeCorrection;

static MagickBooleanType GetDPCEdgeCorrection(Image *magnitude_image,
  const Image *reconstruct,ExceptionInfo *exception,
  const DPCEdgeCorrection *targets)
{
  double
    *const edge_factor = targets->edge_factor;

  Image
    **const threshold_image = targets->threshold_image;

  double
    mean = 0.0,
    standard_deviation = 0.0;

  MagickBooleanType
    status;

  /*
    Compute an edge normalization correction.
  */
  *threshold_image=CloneImage(magnitude_image,0,0,MagickTrue,exception);
  if (*threshold_image == (Image *) NULL)
    return(MagickFalse);
  status=BilevelImage(*threshold_image,0.0,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  status=GetImageMean(*threshold_image,&mean,&standard_deviation,exception);
  *threshold_image=DestroyImage(*threshold_image);
  if (status == MagickFalse)
    return(MagickFalse);
  *edge_factor=MagickSafeReciprocal(QuantumScale*mean*(double)
    reconstruct->columns*(double) reconstruct->rows)+QuantumScale;
  return(MagickTrue);
}

typedef struct _DPCReconstructUnitGradient
{
  Image
    **magnitude_image,
    **rx_image,
    **ry_image,
    **trx_image,
    **try_image;
} DPCReconstructUnitGradient;

static MagickBooleanType DivideDPCReconstructGradient(const Image *image,
  ExceptionInfo *exception,const DPCReconstructUnitGradient *targets)
{
  Image
    **const magnitude_image = targets->magnitude_image,
    **const rx_image = targets->rx_image,
    **const ry_image = targets->ry_image,
    **const trx_image = targets->trx_image,
    **const try_image = targets->try_image;

  /*
    Divide X and Y derivitives of reference image by magnitude.
  */
  *trx_image=SIMDivideByMagnitude(*rx_image,*magnitude_image,image,exception);
  *rx_image=DestroyImage(*rx_image);
  if (*trx_image == (Image *) NULL)
    return(MagickFalse);
  *rx_image=(*trx_image);
  *try_image=SIMDivideByMagnitude(*ry_image,*magnitude_image,image,exception);
  *magnitude_image=DestroyImage(*magnitude_image);
  *ry_image=DestroyImage(*ry_image);
  if (*try_image == (Image *) NULL)
    return(MagickFalse);
  *ry_image=(*try_image);
  return(MagickTrue);
}

typedef struct _DPCTargetUnitGradient
{
  Image
    **magnitude_image,
    **target_image,
    **trx_image,
    **try_image,
    **tx_image,
    **ty_image;
} DPCTargetUnitGradient;

static MagickBooleanType AcquireDPCTargetUnitGradient(const Image *image,
  ExceptionInfo *exception,const DPCTargetUnitGradient *targets)
{
  Image
    **const magnitude_image = targets->magnitude_image,
    **const target_image = targets->target_image,
    **const trx_image = targets->trx_image,
    **const try_image = targets->try_image,
    **const tx_image = targets->tx_image,
    **const ty_image = targets->ty_image;

  /*
    Compute X and Y derivatives of image.
  */
  (void) SetImageVirtualPixelMethod(*target_image,EdgeVirtualPixelMethod,
    exception);
  *tx_image=SIMDerivativeImage(*target_image,"Sobel",exception);
  if (*tx_image == (Image *) NULL)
    return(MagickFalse);
  *ty_image=SIMDerivativeImage(*target_image,"Sobel:90",exception);
  *target_image=DestroyImage(*target_image);
  if (*ty_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Compute magnitude of derivatives.
  */
  *magnitude_image=SIMMagnitudeImage(*tx_image,*ty_image,exception);
  if (*magnitude_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Divide Lx and Ly by magnitude.
  */
  *trx_image=SIMDivideByMagnitude(*tx_image,*magnitude_image,image,exception);
  *tx_image=DestroyImage(*tx_image);
  if (*trx_image == (Image *) NULL)
    return(MagickFalse);
  *tx_image=(*trx_image);
  *try_image=SIMDivideByMagnitude(*ty_image,*magnitude_image,image,exception);
  *ty_image=DestroyImage(*ty_image);
  *magnitude_image=DestroyImage(*magnitude_image);
  if (*try_image == (Image *) NULL)
    return(MagickFalse);
  *ty_image=(*try_image);
  return(MagickTrue);
}

typedef struct _DPCGradientCorrelation
{
  Image
    **rx_image,
    **ry_image,
    **trx_image,
    **try_image,
    **tx_image,
    **ty_image;
} DPCGradientCorrelation;

static MagickBooleanType CorrelateDPCGradients(const double edge_factor,
  ExceptionInfo *exception,const DPCGradientCorrelation *targets)
{
  Image
    **const rx_image = targets->rx_image,
    **const ry_image = targets->ry_image,
    **const trx_image = targets->trx_image,
    **const try_image = targets->try_image,
    **const tx_image = targets->tx_image,
    **const ty_image = targets->ty_image;

  MagickBooleanType
    status;

  /*
    Compute the cross correlation of the test and reference images.
  */
  *trx_image=SIMCrossCorrelationImage(*tx_image,*rx_image,exception);
  *rx_image=DestroyImage(*rx_image);
  *tx_image=DestroyImage(*tx_image);
  if (*trx_image == (Image *) NULL)
    return(MagickFalse);
  *try_image=SIMCrossCorrelationImage(*ty_image,*ry_image,exception);
  *ry_image=DestroyImage(*ry_image);
  *ty_image=DestroyImage(*ty_image);
  if (*try_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Evaluate dot product correlation image.
  */
  (void) SetImageArtifact(*try_image,"compose:clamp","false");
  status=CompositeImage(*trx_image,*try_image,PlusCompositeOp,MagickTrue,0,0,
    exception);
  *try_image=DestroyImage(*try_image);
  if (status == MagickFalse)
    return(MagickFalse);
  status=SIMMultiplyImage(*trx_image,edge_factor,
    (const ChannelStatistics *) NULL,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _DPCDotProduct
{
  Image
    **dot_product_image;

  double
    *maxima;

  Image
    **trx_image;
} DPCDotProduct;

static MagickBooleanType GetDPCMaxima(const Image *image,
  RectangleInfo *offset,ExceptionInfo *exception,
  const DPCDotProduct *targets)
{
  Image
    **const dot_product_image = targets->dot_product_image;

  double
    *const maxima = targets->maxima;

  Image
    **const trx_image = targets->trx_image;

  MagickBooleanType
    status;

  RectangleInfo
    geometry;

  /*
    Crop results.
  */
  SetGeometry(image,&geometry);
  geometry.width=image->columns;
  geometry.height=image->rows;
  (void) ResetImagePage(*trx_image,"0x0+0+0");
  *dot_product_image=CropImage(*trx_image,&geometry,exception);
  *trx_image=DestroyImage(*trx_image);
  if (*dot_product_image == (Image *) NULL)
    return(MagickFalse);
  (void) ResetImagePage(*dot_product_image,"0x0+0+0");
  /*
    Identify the maxima value in the image and its location.
  */
  status=GrayscaleImage(*dot_product_image,AveragePixelIntensityMethod,
    exception);
  if (status == MagickFalse)
    return(MagickFalse);
  (*dot_product_image)->depth=32;
  (*dot_product_image)->colorspace=GRAYColorspace;
  (*dot_product_image)->alpha_trait=UndefinedPixelTrait;
  status=SIMFilterImageNaNs(*dot_product_image,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  status=SIMMaximaImage(*dot_product_image,maxima,offset,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  if ((QuantumScale*(*maxima)) > 1.0)
    {
      status=SIMMultiplyImage(*dot_product_image,1.0/(QuantumScale*
        (*maxima)),(const ChannelStatistics *) NULL,exception);
      *maxima=(double) QuantumRange;
    }
  return(MagickTrue);
}

static Image *DPCSimilarityImage(const Image *image,const Image *reconstruct,
  RectangleInfo *offset,double *similarity_metric,ExceptionInfo *exception)
{
  double
    edge_factor = 0.0,
    maxima = 0.0;

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

  DPCDotProduct
    dot_product;

  DPCEdgeCorrection
    edge_correction;

  const DPCGradientCorrelation
    gradient_correlation = { &rx_image, &ry_image, &trx_image, &try_image,
      &tx_image, &ty_image };

  DPCReconstructGradient
    reconstruct_gradient;

  const DPCReconstructUnitGradient
    reconstruct_unit = { &magnitude_image, &rx_image, &ry_image, &trx_image,
      &try_image };

  const DPCTargetUnitGradient
    target_unit = { &magnitude_image, &target_image, &trx_image, &try_image,
      &tx_image, &ty_image };

  /*
    Dot product correlation-based image similarity using FFT local statistics.
  */
  target_image=CloneImage(image,0,0,MagickTrue,exception);
  if (target_image == (Image *) NULL)
    return((Image *) NULL);
  reconstruct_gradient.magnitude_image=(&magnitude_image);
  reconstruct_gradient.reconstruct_image=(&reconstruct_image);
  reconstruct_gradient.rx_image=(&rx_image);
  reconstruct_gradient.ry_image=(&ry_image);
  if (AcquireDPCReconstructGradient(reconstruct,exception,
        &reconstruct_gradient) == MagickFalse)
    ThrowDPCSimilarityException();
  edge_correction.edge_factor=(&edge_factor);
  edge_correction.threshold_image=(&threshold_image);
  if (GetDPCEdgeCorrection(magnitude_image,reconstruct,exception,
        &edge_correction) == MagickFalse)
    ThrowDPCSimilarityException();
  if (DivideDPCReconstructGradient(image,exception,&reconstruct_unit) ==
      MagickFalse)
    ThrowDPCSimilarityException();
  if (AcquireDPCTargetUnitGradient(image,exception,&target_unit) ==
      MagickFalse)
    ThrowDPCSimilarityException();
  if (CorrelateDPCGradients(edge_factor,exception,&gradient_correlation) ==
      MagickFalse)
    ThrowDPCSimilarityException();
  dot_product.dot_product_image=(&dot_product_image);
  dot_product.maxima=(&maxima);
  dot_product.trx_image=(&trx_image);
  if (GetDPCMaxima(image,offset,exception,&dot_product) == MagickFalse)
    ThrowDPCSimilarityException();
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

typedef struct _MSEAlphaTargets
{
  Image
    **alpha_image,
    **reconstruct_image;
} MSEAlphaTargets;

static MagickBooleanType AcquireMSEAlpha(const Image *image,
  const Image *reconstruct,ExceptionInfo *exception,
  const MSEAlphaTargets *targets)
{
  Image
    **const alpha_image = targets->alpha_image,
    **const reconstruct_image = targets->reconstruct_image;

  Image
    *target_image;

  MagickBooleanType
    status;

  target_image=SIMSquareImage(image,exception);
  if (target_image == (Image *) NULL)
    return(MagickFalse);
  *reconstruct_image=SIMUnityImage(image,reconstruct,exception);
  if (*reconstruct_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Create (U * test)/# pixels.
  */
  *alpha_image=SIMCrossCorrelationImage(target_image,*reconstruct_image,
    exception);
  target_image=DestroyImage(target_image);
  if (*alpha_image == (Image *) NULL)
    return(MagickFalse);
  status=SIMMultiplyImage(*alpha_image,1.0/(double) reconstruct->columns/
    (double) reconstruct->rows,(const ChannelStatistics *) NULL,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _MSEBetaTargets
{
  Image
    **beta_image,
    **reconstruct_image;
} MSEBetaTargets;

static MagickBooleanType AcquireMSEBeta(const Image *image,
  const Image *reconstruct,ExceptionInfo *exception,
  const MSEBetaTargets *targets)
{
  Image
    **const beta_image = targets->beta_image,
    **const reconstruct_image = targets->reconstruct_image;

  MagickBooleanType
    status;

  /*
    Create 2*(test * reconstruction)# pixels.
  */
  (void) CompositeImage(*reconstruct_image,reconstruct,CopyCompositeOp,
    MagickTrue,0,0,exception);
  *beta_image=SIMCrossCorrelationImage(image,*reconstruct_image,exception);
  if (*beta_image == (Image *) NULL)
    {
      *reconstruct_image=DestroyImage(*reconstruct_image);
      return(MagickFalse);
    }
  status=SIMMultiplyImage(*beta_image,-2.0/(double) reconstruct->columns/
    (double) reconstruct->rows,(const ChannelStatistics *) NULL,exception);
  *reconstruct_image=DestroyImage(*reconstruct_image);
  if (status == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _MSESumTargets
{
  ChannelStatistics
    **channel_statistics;

  Image
    **sum_image;
} MSESumTargets;

static MagickBooleanType AcquireMSESum(const Image *reconstruct,
  ExceptionInfo *exception,const MSESumTargets *targets)
{
  ChannelStatistics
    **const channel_statistics = targets->channel_statistics;

  Image
    **const sum_image = targets->sum_image;

  MagickBooleanType
    status;

  *sum_image=SIMSquareImage(reconstruct,exception);
  if (*sum_image == (Image *) NULL)
    return(MagickFalse);
  *channel_statistics=GetImageStatistics(*sum_image,exception);
  if (*channel_statistics == (ChannelStatistics *) NULL)
    return(MagickFalse);
  status=SetImageStorageClass(*sum_image,DirectClass,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  status=SIMSetImageMean(*sum_image,*channel_statistics,exception);
  *channel_statistics=(ChannelStatistics *)
    RelinquishMagickMemory(*channel_statistics);
  if (status == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _MSEMeanTargets
{
  Image
    **mean_image,
    **sum_image;
} MSEMeanTargets;

static MagickBooleanType AcquireMSEMean(Image *alpha_image,Image *beta_image,
  ExceptionInfo *exception,const MSEMeanTargets *targets)
{
  Image
    **const mean_image = targets->mean_image,
    **const sum_image = targets->sum_image;

  MagickBooleanType
    status;

  /*
    Create mean image.
  */
  AppendImageToList(sum_image,alpha_image);
  AppendImageToList(sum_image,beta_image);
  *mean_image=EvaluateImages(*sum_image,SumEvaluateOperator,exception);
  if (*mean_image == (Image *) NULL)
    return(MagickFalse);
  *sum_image=DestroyImage(*sum_image);
  status=GrayscaleImage(*mean_image,AveragePixelIntensityMethod,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _MSEMinimaTargets
{
  Image
    **mean_image;

  double
    *minima;

  Image
    **mse_image;
} MSEMinimaTargets;

static MagickBooleanType GetMSEMinima(const Image *image,
  RectangleInfo *offset,ExceptionInfo *exception,
  const MSEMinimaTargets *targets)
{
  Image
    **const mean_image = targets->mean_image;

  double
    *const minima = targets->minima;

  Image
    **const mse_image = targets->mse_image;

  MagickBooleanType
    status;

  RectangleInfo
    geometry;

  /*
    Crop to difference of reconstruction and test images.
  */
  SetGeometry(image,&geometry);
  geometry.width=image->columns;
  geometry.height=image->rows;
  (void) ResetImagePage(*mean_image,"0x0+0+0");
  *mse_image=CropImage(*mean_image,&geometry,exception);
  *mean_image=DestroyImage(*mean_image);
  if (*mse_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Identify the minima value in the correlation image and its location.
  */
  (void) ResetImagePage(*mse_image,"0x0+0+0");
  (void) ClampImage(*mse_image,exception);
  (*mse_image)->depth=32;
  (*mse_image)->colorspace=GRAYColorspace;
  (*mse_image)->alpha_trait=UndefinedPixelTrait;
  status=SIMMinimaImage(*mse_image,minima,offset,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  status=NegateImage(*mse_image,MagickFalse,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
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
    *sum_image = (Image *) NULL;

  MSEAlphaTargets
    targets;

  MSEBetaTargets
    beta_targets;

  MSEMeanTargets
    mean_targets;

  MSEMinimaTargets
    minima_targets;

  MSESumTargets
    sum_targets;

  /*
    MSE correlation-based image similarity using FFT local statistics.
  */
  targets.alpha_image=(&alpha_image);
  targets.reconstruct_image=(&reconstruct_image);
  if (AcquireMSEAlpha(image,reconstruct,exception,&targets) == MagickFalse)
    ThrowMSESimilarityException();
  beta_targets.beta_image=(&beta_image);
  beta_targets.reconstruct_image=(&reconstruct_image);
  if (AcquireMSEBeta(image,reconstruct,exception,&beta_targets) == MagickFalse)
    ThrowMSESimilarityException();
  /*
    Mean of reconstruction squared.
  */
  sum_targets.channel_statistics=(&channel_statistics);
  sum_targets.sum_image=(&sum_image);
  if (AcquireMSESum(reconstruct,exception,&sum_targets) == MagickFalse)
    ThrowMSESimilarityException();
  mean_targets.mean_image=(&mean_image);
  mean_targets.sum_image=(&sum_image);
  if (AcquireMSEMean(alpha_image,beta_image,exception,&mean_targets) ==
      MagickFalse)
    ThrowMSESimilarityException();
  minima_targets.mean_image=(&mean_image);
  minima_targets.minima=(&minima);
  minima_targets.mse_image=(&mse_image);
  if (GetMSEMinima(image,offset,exception,&minima_targets) == MagickFalse)
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

typedef struct _NCCAlphaTargets
{
  Image
    **alpha_image,
    **reconstruct_image,
    **target_image;
} NCCAlphaTargets;

static MagickBooleanType AcquireNCCAlpha(const Image *image,
  const Image *reconstruct,ExceptionInfo *exception,
  const NCCAlphaTargets *targets)
{
  Image
    **const alpha_image = targets->alpha_image,
    **const reconstruct_image = targets->reconstruct_image,
    **const target_image = targets->target_image;

  MagickBooleanType
    status;

  *target_image=SIMSquareImage(image,exception);
  if (*target_image == (Image *) NULL)
    return(MagickFalse);
  *reconstruct_image=SIMUnityImage(image,reconstruct,exception);
  if (*reconstruct_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Compute the cross correlation of the test and reconstruction images.
  */
  *alpha_image=SIMCrossCorrelationImage(*target_image,*reconstruct_image,
    exception);
  *target_image=DestroyImage(*target_image);
  if (*alpha_image == (Image *) NULL)
    return(MagickFalse);
  status=SIMMultiplyImage(*alpha_image,(double) QuantumRange*
    (double) reconstruct->columns*(double) reconstruct->rows,
    (const ChannelStatistics *) NULL,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _NCCTargetTargets
{
  Image
    **beta_image,
    **reconstruct_image,
    **target_image;
} NCCTargetTargets;

static MagickBooleanType AcquireNCCTarget(const Image *image,
  ExceptionInfo *exception,const NCCTargetTargets *targets)
{
  Image
    **const beta_image = targets->beta_image,
    **const reconstruct_image = targets->reconstruct_image,
    **const target_image = targets->target_image;

  MagickBooleanType
    status;

  /*
    Compute the cross correlation of the source and reconstruction images.
  */
  *beta_image=SIMCrossCorrelationImage(image,*reconstruct_image,exception);
  *reconstruct_image=DestroyImage(*reconstruct_image);
  if (*beta_image == (Image *) NULL)
    return(MagickFalse);
  *target_image=SIMSquareImage(*beta_image,exception);
  *beta_image=DestroyImage(*beta_image);
  if (*target_image == (Image *) NULL)
    return(MagickFalse);
  status=SIMMultiplyImage(*target_image,(double) QuantumRange,
    (const ChannelStatistics *) NULL,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _NCCVarianceTargets
{
  Image
    **alpha_image;

  ChannelStatistics
    **channel_statistics;

  Image
    **target_image,
    **variance_image;
} NCCVarianceTargets;

static MagickBooleanType AcquireNCCVariance(const Image *reconstruct,
  ExceptionInfo *exception,const NCCVarianceTargets *targets)
{
  Image
    **const alpha_image = targets->alpha_image;

  ChannelStatistics
    **const channel_statistics = targets->channel_statistics;

  Image
    **const target_image = targets->target_image,
    **const variance_image = targets->variance_image;

  MagickBooleanType
    status;

  /*
    Compute the variance of the two images.
  */
  *variance_image=SIMVarianceImage(*alpha_image,*target_image,exception);
  *target_image=DestroyImage(*target_image);
  *alpha_image=DestroyImage(*alpha_image);
  if (*variance_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Subtract the image mean.
  */
  *channel_statistics=GetImageStatistics(reconstruct,exception);
  if (*channel_statistics == (ChannelStatistics *) NULL)
    return(MagickFalse);
  status=SIMMultiplyImage(*variance_image,1.0,*channel_statistics,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _NCCCorrelationTargets
{
  ChannelStatistics
    **channel_statistics;

  Image
    **correlation_image,
    **divide_image,
    **ncc_image,
    **normalize_image,
    **variance_image;
} NCCCorrelationTargets;

static MagickBooleanType AcquireNCCCorrelation(const Image *image,
  const Image *reconstruct,ExceptionInfo *exception,
  const NCCCorrelationTargets *targets)
{
  ChannelStatistics
    **const channel_statistics = targets->channel_statistics;

  Image
    **const correlation_image = targets->correlation_image,
    **const divide_image = targets->divide_image,
    **const ncc_image = targets->ncc_image,
    **const normalize_image = targets->normalize_image,
    **const variance_image = targets->variance_image;

  RectangleInfo
    geometry;

  *normalize_image=SIMSubtractImageMean(image,reconstruct,*channel_statistics,
    exception);
  *channel_statistics=(ChannelStatistics *)
    RelinquishMagickMemory(*channel_statistics);
  if (*normalize_image == (Image *) NULL)
    return(MagickFalse);
  *correlation_image=SIMCrossCorrelationImage(image,*normalize_image,
    exception);
  *normalize_image=DestroyImage(*normalize_image);
  if (*correlation_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Divide the two images.
  */
  *divide_image=SIMDivideImage(*correlation_image,*variance_image,exception);
  *correlation_image=DestroyImage(*correlation_image);
  *variance_image=DestroyImage(*variance_image);
  if (*divide_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Crop padding.
  */
  SetGeometry(image,&geometry);
  geometry.width=image->columns;
  geometry.height=image->rows;
  (void) ResetImagePage(*divide_image,"0x0+0+0");
  *ncc_image=CropImage(*divide_image,&geometry,exception);
  *divide_image=DestroyImage(*divide_image);
  if (*ncc_image == (Image *) NULL)
    return(MagickFalse);
  return(MagickTrue);
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

  NCCAlphaTargets
    targets;

  const NCCCorrelationTargets
    correlation_targets = { &channel_statistics, &correlation_image,
      &divide_image, &ncc_image, &normalize_image, &variance_image };

  NCCTargetTargets
    beta_targets;

  NCCVarianceTargets
    variance_targets;

  /*
    NCC correlation-based image similarity with FFT local statistics.
  */
  targets.alpha_image=(&alpha_image);
  targets.reconstruct_image=(&reconstruct_image);
  targets.target_image=(&target_image);
  if (AcquireNCCAlpha(image,reconstruct,exception,&targets) == MagickFalse)
    ThrowNCCSimilarityException();
  beta_targets.beta_image=(&beta_image);
  beta_targets.reconstruct_image=(&reconstruct_image);
  beta_targets.target_image=(&target_image);
  if (AcquireNCCTarget(image,exception,&beta_targets) == MagickFalse)
    ThrowNCCSimilarityException();
  variance_targets.alpha_image=(&alpha_image);
  variance_targets.channel_statistics=(&channel_statistics);
  variance_targets.target_image=(&target_image);
  variance_targets.variance_image=(&variance_image);
  if (AcquireNCCVariance(reconstruct,exception,&variance_targets) ==
      MagickFalse)
    ThrowNCCSimilarityException();
  if (AcquireNCCCorrelation(image,reconstruct,exception,
        &correlation_targets) == MagickFalse)
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

typedef struct _PhasePaddedTargets
{
  Image
    **reconstruct_image,
    **target_image;
} PhasePaddedTargets;

static MagickBooleanType AcquirePhasePadded(const Image *image,
  const Image *reconstruct,ExceptionInfo *exception,
  const PhasePaddedTargets *targets)
{
  Image
    **const reconstruct_image = targets->reconstruct_image,
    **const target_image = targets->target_image;

  MagickBooleanType
    status;

  *target_image=CloneImage(image,0,0,MagickTrue,exception);
  if (*target_image == (Image *) NULL)
    return(MagickFalse);
  (void) ResetImagePage(*target_image,"0x0+0+0");
  GetPixelInfoRGBA((Quantum) 0,(Quantum) 0,(Quantum) 0,(Quantum) 0,
    &(*target_image)->background_color);
  status=SetImageExtent(*target_image,2*CastDoubleToSizeT(ceil((double)
    image->columns/2.0)),2*CastDoubleToSizeT(ceil((double) image->rows/2.0)),
    exception);
  if (status == MagickFalse)
    return(MagickFalse);
  /*
    Compute the cross correlation of the target and reconstruct magnitudes.
  */
  *reconstruct_image=CloneImage(reconstruct,0,0,MagickTrue,exception);
  if (*reconstruct_image == (Image *) NULL)
    return(MagickFalse);
  (void) ResetImagePage(*reconstruct_image,"0x0+0+0");
  GetPixelInfoRGBA((Quantum) 0,(Quantum) 0,(Quantum) 0,(Quantum) 0,
    &(*reconstruct_image)->background_color);
  status=SetImageExtent(*reconstruct_image,2*CastDoubleToSizeT(ceil((double)
    image->columns/2.0)),2*CastDoubleToSizeT(ceil((double) image->rows/2.0)),
    exception);
  if (status == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _PhaseMagnitudeTargets
{
  Image
    **fft_images,
    **magnitude_image,
    **reconstruct_magnitude,
    **test_magnitude;
} PhaseMagnitudeTargets;

static MagickBooleanType AcquirePhaseMagnitude(Image *target_image,
  Image *reconstruct_image,ExceptionInfo *exception,
  const PhaseMagnitudeTargets *targets)
{
  Image
    **const fft_images = targets->fft_images,
    **const magnitude_image = targets->magnitude_image,
    **const reconstruct_magnitude = targets->reconstruct_magnitude,
    **const test_magnitude = targets->test_magnitude;

  /*
    Evaluate phase coorelation image and divide by the product magnitude.
  */
  (void) SetImageArtifact(target_image,"fourier:normalize","inverse");
  *fft_images=ForwardFourierTransformImage(target_image,MagickTrue,exception);
  if (*fft_images == (Image *) NULL)
    return(MagickFalse);
  *test_magnitude=CloneImage(*fft_images,0,0,MagickTrue,exception);
  *fft_images=DestroyImageList(*fft_images);
  if (*test_magnitude == (Image *) NULL)
    return(MagickFalse);
  (void) SetImageArtifact(reconstruct_image,"fourier:normalize","inverse");
  *fft_images=ForwardFourierTransformImage(reconstruct_image,MagickTrue,
    exception);
  if (*fft_images == (Image *) NULL)
    return(MagickFalse);
  *reconstruct_magnitude=CloneImage(*fft_images,0,0,MagickTrue,exception);
  *fft_images=DestroyImageList(*fft_images);
  if (*reconstruct_magnitude == (Image *) NULL)
    return(MagickFalse);
  *magnitude_image=CloneImage(*reconstruct_magnitude,0,0,MagickTrue,
    exception);
  if (*magnitude_image == (Image *) NULL)
    return(MagickFalse);
  DisableCompositeClampUnlessSpecified(*magnitude_image);
  (void) CompositeImage(*magnitude_image,*test_magnitude,MultiplyCompositeOp,
    MagickTrue,0,0,exception);
  return(MagickTrue);
}

typedef struct _PhaseMaximaTargets
{
  Image
    **magnitude_image;

  double
    *maxima;
} PhaseMaximaTargets;

static MagickBooleanType GetPhaseMaxima(Image *phase_image,
  RectangleInfo *offset,ExceptionInfo *exception,
  const PhaseMaximaTargets *targets)
{
  Image
    **const magnitude_image = targets->magnitude_image;

  double
    *const maxima = targets->maxima;

  MagickBooleanType
    status;

  status=GrayscaleImage(phase_image,AveragePixelIntensityMethod,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  phase_image->depth=32;
  phase_image->colorspace=GRAYColorspace;
  phase_image->alpha_trait=UndefinedPixelTrait;
  status=SIMFilterImageNaNs(phase_image,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  status=SIMMaximaImage(phase_image,maxima,offset,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  *magnitude_image=DestroyImage(*magnitude_image);
  return(MagickTrue);
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

  PhaseMagnitudeTargets
    magnitude_targets;

  PhaseMaximaTargets
    maxima_targets;

  PhasePaddedTargets
    targets;

  RectangleInfo
    geometry;

  /*
    Phase correlation-based image similarity using FFT local statistics.
  */
  targets.reconstruct_image=(&reconstruct_image);
  targets.target_image=(&target_image);
  if (AcquirePhasePadded(image,reconstruct,exception,&targets) == MagickFalse)
    ThrowPhaseSimilarityException();
  magnitude_targets.fft_images=(&fft_images);
  magnitude_targets.magnitude_image=(&magnitude_image);
  magnitude_targets.reconstruct_magnitude=(&reconstruct_magnitude);
  magnitude_targets.test_magnitude=(&test_magnitude);
  if (AcquirePhaseMagnitude(target_image,reconstruct_image,exception,
        &magnitude_targets) == MagickFalse)
    ThrowPhaseSimilarityException();
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
  maxima_targets.magnitude_image=(&magnitude_image);
  maxima_targets.maxima=(&maxima);
  if (GetPhaseMaxima(phase_image,offset,exception,&maxima_targets) ==
      MagickFalse)
    ThrowPhaseSimilarityException();
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
