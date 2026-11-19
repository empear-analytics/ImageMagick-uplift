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

  MagickCore private methods of compare-similarity-private.h: the PHASE, PHASH, PSNR and RMSE similarity measures, from GetEdgeCorrelationSurface to GetRMSESimilarity; included by that header only.
*/
#ifndef MAGICKCORE_COMPARE_SIMILARITY_PHASE_PRIVATE_H
#define MAGICKCORE_COMPARE_SIMILARITY_PHASE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline Image *GetEdgeCorrelationSurface(const Image *image,
  ExceptionInfo *exception)
{
  Image
    *surface;

  KernelInfo
    *kernel;

  /*
    Build a spatial-domain edge/correlation surface using a LoG convolution.
  */
  kernel=AcquireKernelInfo("LoG:0,1",exception);
  if (kernel == (KernelInfo *) NULL)
    return((Image *) NULL);
  surface=MorphologyImage(image,ConvolveMorphology,1,kernel,exception);
  kernel=DestroyKernelInfo(kernel);
  return(surface);
}

static inline MagickBooleanType IsPhaseRowMissing(const Quantum *p,
  const Quantum *q,const Quantum *pm,const Quantum *qm)
{
  return(((p == (const Quantum *) NULL) || (q == (const Quantum *) NULL) ||
    (pm == (const Quantum *) NULL) || (qm == (const Quantum *) NULL)) ?
    MagickTrue : MagickFalse);
}

typedef struct _PHASEPixel
{
  Image
    *edge_image,
    *edge_reconstruct;

  const Quantum
    *p,
    *q;
} PHASEPixel;

typedef struct _PHASEPixelTargets
{
  double
    (*channel_image_sum)[MaxPixelChannels+1],
    (*channel_image_sum_squared)[MaxPixelChannels+1],
    (*channel_reconstruct_sum)[MaxPixelChannels+1],
    (*channel_reconstruct_sum_squared)[MaxPixelChannels+1],
    (*channel_correlation)[MaxPixelChannels+1];
} PHASEPixelTargets;

static inline void AccumulatePHASEPixel(const PHASEPixel *phase,
  const PHASEPixelTargets *targets)
{
  Image
    *const edge_image = phase->edge_image,
    *const edge_reconstruct = phase->edge_reconstruct;

  const Quantum
    *const p = phase->p,
    *const q = phase->q;

  double
    (*const channel_image_sum)[MaxPixelChannels+1] =
      targets->channel_image_sum,
    (*const channel_image_sum_squared)[MaxPixelChannels+1] =
      targets->channel_image_sum_squared,
    (*const channel_reconstruct_sum)[MaxPixelChannels+1] =
      targets->channel_reconstruct_sum,
    (*const channel_reconstruct_sum_squared)[MaxPixelChannels+1] =
      targets->channel_reconstruct_sum_squared,
    (*const channel_correlation)[MaxPixelChannels+1] =
      targets->channel_correlation;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(edge_image); i++)
  {
    double
      alpha,
      beta;

    PixelChannel
      channel;

    PixelTrait
      reconstruct_traits,
      traits;

    ssize_t
      offset;

    channel=GetPixelChannelChannel(edge_image,i);
    traits=GetPixelChannelTraits(edge_image,channel);
    reconstruct_traits=GetPixelChannelTraits(edge_reconstruct,channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    offset=GetPixelChannelOffset(edge_reconstruct,channel);
    if (offset < 0)
      continue;
    alpha=QuantumScale*(double) p[i];
    beta=QuantumScale*(double) q[offset];
    (*channel_image_sum)[i]+=alpha;
    (*channel_image_sum_squared)[i]+=alpha*alpha;
    (*channel_reconstruct_sum)[i]+=beta;
    (*channel_reconstruct_sum_squared)[i]+=beta*beta;
    (*channel_correlation)[i]+=alpha*beta;
  }
}

typedef struct _PHASECorrelationSums
{
  double
    area,
    *correlation,
    *image_sum,
    *image_sum_squared,
    *reconstruct_sum,
    *reconstruct_sum_squared;
} PHASECorrelationSums;

static inline double GetPHASEPearson(const PHASECorrelationSums *sums,
  const ssize_t j)
{
  const double
    area = sums->area;

  double
    *const correlation = sums->correlation,
    *const image_sum = sums->image_sum,
    *const image_sum_squared = sums->image_sum_squared,
    *const reconstruct_sum = sums->reconstruct_sum,
    *const reconstruct_sum_squared = sums->reconstruct_sum_squared;

  double
    denominator,
    image_variance,
    numerator,
    pearson,
    reconstruct_variance;

  numerator=area*correlation[j]-image_sum[j]*reconstruct_sum[j];
  image_variance=area*image_sum_squared[j]-image_sum[j]*image_sum[j];
  reconstruct_variance=area*reconstruct_sum_squared[j]-reconstruct_sum[j]*
    reconstruct_sum[j];
  if ((image_variance < MagickEpsilon) &&
      (reconstruct_variance < MagickEpsilon))
    pearson=(fabs(image_sum[j]-reconstruct_sum[j]) < MagickEpsilon) ?
      1.0 : 0.0;
  else
    {
      denominator=sqrt(image_variance)*sqrt(reconstruct_variance);
      pearson=denominator < MagickEpsilon ? 0.0 : numerator/denominator;
    }
  if (pearson < -1.0)
    pearson=(-1.0);
  if (pearson > 1.0)
    pearson=1.0;
  return(pearson);
}

typedef struct _PHASEReduction
{
  const Image
    *image,
    *reconstruct_image;

  double
    area,
    *correlation,
    *image_sum,
    *image_sum_squared,
    *reconstruct_sum,
    *reconstruct_sum_squared;
} PHASEReduction;

static inline void ReducePHASESimilarity(const PHASEReduction *reduction,
  double *similarity)
{
  const Image
    *const image = reduction->image,
    *const reconstruct_image = reduction->reconstruct_image;

  const double
    area = reduction->area;

  double
    *const correlation = reduction->correlation,
    *const image_sum = reduction->image_sum,
    *const image_sum_squared = reduction->image_sum_squared,
    *const reconstruct_sum = reduction->reconstruct_sum,
    *const reconstruct_sum_squared = reduction->reconstruct_sum_squared;

  PHASECorrelationSums
    sums;

  ssize_t
    channels = 0,
    j;

  similarity[CompositePixelChannel]=0.0;
  for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
  {
    double
      pearson;

    PixelChannel
      channel;

    PixelTrait
      reconstruct_traits,
      traits;

    channel=GetPixelChannelChannel(image,j);
    traits=GetPixelChannelTraits(image,channel);
    reconstruct_traits=GetPixelChannelTraits(reconstruct_image,channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    sums.area=area;
    sums.correlation=correlation;
    sums.image_sum=image_sum;
    sums.image_sum_squared=image_sum_squared;
    sums.reconstruct_sum=reconstruct_sum;
    sums.reconstruct_sum_squared=reconstruct_sum_squared;
    pearson=GetPHASEPearson(&sums,j);
    similarity[j]=pearson;
    similarity[CompositePixelChannel]+=pearson;
    channels++;
  }
  if (channels != 0)
    similarity[CompositePixelChannel]/=(double) channels;
}

typedef struct _PHASEEdgeSurfaces
{
  Image
    **edge_image,
    **edge_reconstruct;
} PHASEEdgeSurfaces;

static inline MagickBooleanType AcquirePHASEEdges(const Image *image,
  const Image *reconstruct_image,ExceptionInfo *exception,
  const PHASEEdgeSurfaces *targets)
{
  Image
    **const edge_image = targets->edge_image,
    **const edge_reconstruct = targets->edge_reconstruct;

  *edge_image=GetEdgeCorrelationSurface(image,exception);
  *edge_reconstruct=GetEdgeCorrelationSurface(reconstruct_image,exception);
  if ((*edge_image == (Image *) NULL) ||
      (*edge_reconstruct == (Image *) NULL))
    {
      if (*edge_image != (Image *) NULL)
        *edge_image=DestroyImage(*edge_image);
      if (*edge_reconstruct != (Image *) NULL)
        *edge_reconstruct=DestroyImage(*edge_reconstruct);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static inline MagickBooleanType GetPHASESimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  CacheView
    *edge_reconstruct_view,
    *edge_view,
    *image_view,
    *reconstruct_view;

  double
    area = 0,
    correlation[MaxPixelChannels+1] = { 0.0 },
    image_sum[MaxPixelChannels+1] = { 0.0 },
    image_sum_squared[MaxPixelChannels+1] = { 0.0 },
    reconstruct_sum[MaxPixelChannels+1] = { 0.0 },
    reconstruct_sum_squared[MaxPixelChannels+1] = { 0.0 };

  Image
    *edge_image,
    *edge_reconstruct;

  MagickBooleanType
    status = MagickTrue;

  PHASEEdgeSurfaces
    edges;

  size_t
    columns = 0,
    rows = 0;

  ssize_t
    y;

  PHASEReduction
    reduction;

  /*
    Compute a Pearson-correlation similarity between two Laplacian-of-
    Gaussian edge surfaces.
  */
  SetImageCompareBounds(image,reconstruct_image,&columns,&rows);
  edges.edge_image=(&edge_image);
  edges.edge_reconstruct=(&edge_reconstruct);
  if (AcquirePHASEEdges(image,reconstruct_image,exception,&edges) ==
      MagickFalse)
    return(MagickFalse);
  image_view=AcquireVirtualCacheView(image,exception);
  reconstruct_view=AcquireVirtualCacheView(reconstruct_image,exception);
  edge_view=AcquireVirtualCacheView(edge_image,exception);
  edge_reconstruct_view=AcquireVirtualCacheView(edge_reconstruct,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(edge_image,edge_reconstruct,rows,1)
#endif
  for (y=0; y < (ssize_t) rows; y++)
  {
    const Quantum
      *magick_restrict p,
      *magick_restrict pm,
      *magick_restrict q,
      *magick_restrict qm;

    double
      channel_area = 0,
      channel_correlation[MaxPixelChannels+1] = { 0.0 },
      channel_image_sum[MaxPixelChannels+1] = { 0.0 },
      channel_image_sum_squared[MaxPixelChannels+1] = { 0.0 },
      channel_reconstruct_sum[MaxPixelChannels+1] = { 0.0 },
      channel_reconstruct_sum_squared[MaxPixelChannels+1] = { 0.0 };

    ssize_t
      i,
      x;

    if (status == MagickFalse)
      continue;
    pm=GetCacheViewVirtualPixels(image_view,0,y,columns,1,exception);
    qm=GetCacheViewVirtualPixels(reconstruct_view,0,y,columns,1,exception);
    p=GetCacheViewVirtualPixels(edge_view,0,y,columns,1,exception);
    q=GetCacheViewVirtualPixels(edge_reconstruct_view,0,y,columns,1,exception);
    if (IsPhaseRowMissing(p,q,pm,qm) != MagickFalse)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) columns; x++)
    {
      PHASEPixel
        phase;

      PHASEPixelTargets
        targets;

      if ((GetPixelReadMask(image,pm) <= (QuantumRange/2)) ||
          (GetPixelReadMask(reconstruct_image,qm) <= (QuantumRange/2)))
        {
          p+=(ptrdiff_t) GetPixelChannels(edge_image);
          q+=(ptrdiff_t) GetPixelChannels(edge_reconstruct);
          pm+=(ptrdiff_t) GetPixelChannels(image);
          qm+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
          continue;
        }
      phase.edge_image=edge_image;
      phase.edge_reconstruct=edge_reconstruct;
      phase.p=p;
      phase.q=q;
      targets.channel_image_sum=(&channel_image_sum);
      targets.channel_image_sum_squared=(&channel_image_sum_squared);
      targets.channel_reconstruct_sum=(&channel_reconstruct_sum);
      targets.channel_reconstruct_sum_squared=
        (&channel_reconstruct_sum_squared);
      targets.channel_correlation=(&channel_correlation);
      AccumulatePHASEPixel(&phase,&targets);
      channel_area++;
      p+=(ptrdiff_t) GetPixelChannels(edge_image);
      q+=(ptrdiff_t) GetPixelChannels(edge_reconstruct);
      pm+=(ptrdiff_t) GetPixelChannels(image);
      qm+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp critical (MagickCore_GetPHASESimilarity)
#endif
    {
      area+=channel_area;
      for (i=0; i <= (ssize_t) MaxPixelChannels; i++)
      {
        correlation[i]+=channel_correlation[i];
        image_sum[i]+=channel_image_sum[i];
        image_sum_squared[i]+=channel_image_sum_squared[i];
        reconstruct_sum[i]+=channel_reconstruct_sum[i];
        reconstruct_sum_squared[i]+=channel_reconstruct_sum_squared[i];
      }
    }
  }
  edge_reconstruct_view=DestroyCacheView(edge_reconstruct_view);
  edge_view=DestroyCacheView(edge_view);
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  edge_image=DestroyImage(edge_image);
  edge_reconstruct=DestroyImage(edge_reconstruct);
  if (status == MagickFalse)
    return(MagickFalse);
  if (area < 1.0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "InsufficientImageDataInRaster","`%s'",image->filename);
      return(MagickFalse);
    }
  /*
    Reduce to a per-channel Pearson coefficient and average.
  */
  reduction.image=image;
  reduction.reconstruct_image=reconstruct_image;
  reduction.area=area;
  reduction.correlation=correlation;
  reduction.image_sum=image_sum;
  reduction.image_sum_squared=image_sum_squared;
  reduction.reconstruct_sum=reconstruct_sum;
  reduction.reconstruct_sum_squared=reconstruct_sum_squared;
  ReducePHASESimilarity(&reduction,similarity);
  return(MagickTrue);
}

static inline double AddPHashDifferences(
  const ChannelPerceptualHash *channel_phash,
  const ChannelPerceptualHash *reconstruct_phash,const ssize_t i,
  double difference)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) channel_phash[0].number_colorspaces; j++)
  {
    double
      alpha,
      beta;

    ssize_t
      k;

    for (k=0; k < MaximumNumberOfPerceptualHashes; k++)
    {
      double
        error;

      alpha=channel_phash[i].phash[j][k];
      beta=reconstruct_phash[i].phash[j][k];
      error=beta-alpha;
      if (IsNaN(error) != 0)
        error=0.0;
      difference+=error*error;
    }
  }
  return(difference);
}

static inline void NormalizePHashSimilarity(const Image *image,
  const Image *reconstruct_image,const ChannelPerceptualHash *channel_phash,
  double *similarity)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    similarity[i]=sqrt(similarity[i]/(double)
      channel_phash[0].number_colorspaces);
  }
  similarity[CompositePixelChannel]=sqrt(similarity[CompositePixelChannel]/
    (double) channel_phash[0].number_colorspaces);
}

static inline MagickBooleanType IsChannelSkipped(const PixelTrait traits,
  const PixelTrait reconstruct_traits)
{
  return((((traits & UpdatePixelTrait) == 0) ||
    ((reconstruct_traits & UpdatePixelTrait) == 0)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType GetPHASHSimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  ChannelPerceptualHash
    *channel_phash,
    *reconstruct_phash;

  const char
    *artifact;

  ssize_t
    channels = 0,
    i;

  /*
    Compute the perceptual hash similarity.
  */
  channel_phash=GetImagePerceptualHash(image,exception);
  if (channel_phash == (ChannelPerceptualHash *) NULL)
    return(MagickFalse);
  reconstruct_phash=GetImagePerceptualHash(reconstruct_image,exception);
  if (reconstruct_phash == (ChannelPerceptualHash *) NULL)
    {
      channel_phash=(ChannelPerceptualHash *) RelinquishMagickMemory(
        channel_phash);
      return(MagickFalse);
    }
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      difference = 0.0;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (IsChannelSkipped(traits,reconstruct_traits) != MagickFalse)
      continue;
    difference=AddPHashDifferences(channel_phash,reconstruct_phash,i,
      difference);
    similarity[i]+=difference;
    similarity[CompositePixelChannel]+=difference;
    channels++;
  }
  if (channels != 0)
    similarity[CompositePixelChannel]/=(double) channels;
  artifact=GetImageArtifact(image,"phash:normalize");
  if (IsStringTrue(artifact) != MagickFalse)
    NormalizePHashSimilarity(image,reconstruct_image,channel_phash,
      similarity);
  /*
    Free resources.
  */
  reconstruct_phash=(ChannelPerceptualHash *) RelinquishMagickMemory(
    reconstruct_phash);
  channel_phash=(ChannelPerceptualHash *) RelinquishMagickMemory(channel_phash);
  return(MagickTrue);
}

static inline MagickBooleanType GetPSNRSimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  MagickBooleanType
    status = MagickTrue;

  ssize_t
    i;

  /*
    Compute the peak signal-to-noise ratio similarity.
  */
  status=GetMSESimilarity(image,reconstruct_image,similarity,exception);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    similarity[i]=10.0*MagickSafeLog10(MagickSafeReciprocal(
      similarity[i]))/MagickSafePSNRRecipicol(10.0);
  }
  similarity[CompositePixelChannel]=10.0*MagickSafeLog10(
    MagickSafeReciprocal(similarity[CompositePixelChannel]))/
    MagickSafePSNRRecipicol(10.0);
  return(status);
}

static inline MagickBooleanType GetRMSESimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
#define RMSESquareRoot(x)  sqrt((x) < 0.0 ? 0.0 : (x))

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    i;

  /*
    Compute the root mean-squared error similarity.
  */
  status=GetMSESimilarity(image,reconstruct_image,similarity,exception);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    similarity[i]=RMSESquareRoot(similarity[i]);
  }
  similarity[CompositePixelChannel]=RMSESquareRoot(
    similarity[CompositePixelChannel]);
  return(status);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
