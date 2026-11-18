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

  MagickCore private composite methods: the blend images of the saliency and seamless composites.
*/
#ifndef MAGICKCORE_COMPOSITE_IMAGE_BLEND_PRIVATE_H
#define MAGICKCORE_COMPOSITE_IMAGE_BLEND_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
  Typedef declarations.
*/
typedef struct _BlendSumWeights
{
  double
    attenuate,
    sign;
} BlendSumWeights;

typedef struct _BlendMagnitudePair
{
  const Image
    *alpha_image,
    *beta_image;
} BlendMagnitudePair;

typedef struct _BlendRequest
{
  ssize_t
    x_offset,
    y_offset;

  double
    iterations,
    residual_threshold;

  size_t
    tick;
} BlendRequest;

typedef struct _CompositePlacement
{
  MagickBooleanType
    clip_to_self;

  ssize_t
    x_offset,
    y_offset;
} CompositePlacement;

static Image *BlendConvolveImage(const Image *image,const char *kernel,
  ExceptionInfo *exception)
{
  Image
    *clone_image,
    *convolve_image;

  KernelInfo
    *kernel_info;

  /*
    Convolve image with a kernel.
  */
  kernel_info=AcquireKernelInfo(kernel,exception);
  if (kernel_info == (KernelInfo *) NULL)
    return((Image *) NULL);
  clone_image=CloneImage(image,0,0,MagickTrue,exception);
  if (clone_image == (Image *) NULL)
    {
      kernel_info=DestroyKernelInfo(kernel_info);
      return((Image *) NULL);
    }
  (void) SetImageAlphaChannel(clone_image,OffAlphaChannel,exception);
  convolve_image=ConvolveImage(clone_image,kernel_info,exception);
  kernel_info=DestroyKernelInfo(kernel_info);
  clone_image=DestroyImage(clone_image);
  return(convolve_image);
}

static inline MagickBooleanType IsBlendRowMissing(const Quantum *p,
  const Quantum *q,const Quantum *r)
{
  return(((p == (const Quantum *) NULL) || (q == (const Quantum *) NULL) ||
    (r == (Quantum *) NULL)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsBlendChannelSkipped(const PixelTrait traits,
  const PixelTrait other_traits)
{
  return(((traits == UndefinedPixelTrait) ||
    (other_traits == UndefinedPixelTrait) ||
    ((other_traits & UpdatePixelTrait) == 0)) ? MagickTrue : MagickFalse);
}

typedef struct _BlendGradients
{
  const Image
    *dx_image,
    *dy_image;
} BlendGradients;

static void SetMagnitudePixel(const BlendGradients *gradients,
  const Quantum *magick_restrict p,const Quantum *magick_restrict q,
  Quantum *magick_restrict r)
{
  const Image
    *dx_image = gradients->dx_image,
    *dy_image = gradients->dy_image;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(dx_image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(dx_image,i);
    PixelTrait traits = GetPixelChannelTraits(dx_image,channel);
    PixelTrait dy_traits = GetPixelChannelTraits(dy_image,channel);
    if (IsBlendChannelSkipped(traits,dy_traits) != MagickFalse)
      continue;
    r[i]=ClampToQuantum(hypot((double) p[i],(double)
      GetPixelChannel(dy_image,channel,q)));
  }
}

typedef struct _MagnitudeLine
{
  CacheView
    *dx_view,
    *dy_view,
    *magnitude_view;

  const Image
    *dx_image,
    *dy_image;

  Image
    *magnitude_image;

  ExceptionInfo
    *exception;
} MagnitudeLine;

static void SetMagnitudeLine(const MagnitudeLine *line,const ssize_t y,
  MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p,
    *magick_restrict q;

  Quantum
    *magick_restrict r;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(line->dx_view,0,y,line->dx_image->columns,1,
    line->exception);
  q=GetCacheViewVirtualPixels(line->dy_view,0,y,line->dx_image->columns,1,
    line->exception);
  r=GetCacheViewAuthenticPixels(line->magnitude_view,0,y,
    line->dx_image->columns,1,line->exception);
  if (IsBlendRowMissing(p,q,r) != MagickFalse)
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) line->dx_image->columns; x++)
  {
    BlendGradients
      gradients;

    gradients.dx_image=line->dx_image;
    gradients.dy_image=line->dy_image;
    SetMagnitudePixel(&gradients,p,q,r);
    p+=(ptrdiff_t) GetPixelChannels(line->dx_image);
    q+=(ptrdiff_t) GetPixelChannels(line->dy_image);
    r+=(ptrdiff_t) GetPixelChannels(line->magnitude_image);
  }
  SyncCompositeRow(line->magnitude_view,line->exception,status);
}

static Image *BlendMagnitudeImage(const Image *dx_image,const Image *dy_image,
  ExceptionInfo *exception)
{
  CacheView
    *dx_view,
    *dy_view,
    *magnitude_view;

  Image
    *magnitude_image;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    y;

  /*
    Generate the magnitude between two images.
  */
  magnitude_image=CloneImage(dx_image,0,0,MagickTrue,exception);
  if (magnitude_image == (Image *) NULL)
    return(magnitude_image);
  dx_view=AcquireVirtualCacheView(dx_image,exception);
  dy_view=AcquireVirtualCacheView(dy_image,exception);
  magnitude_view=AcquireAuthenticCacheView(magnitude_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(dx_image,magnitude_image,dx_image->rows,1)
#endif
  for (y=0; y < (ssize_t) dx_image->rows; y++)
  {
    const MagnitudeLine
      line = { dx_view, dy_view, magnitude_view, dx_image, dy_image,
        magnitude_image, exception };

    SetMagnitudeLine(&line,y,&status);
  }
  magnitude_view=DestroyCacheView(magnitude_view);
  dy_view=DestroyCacheView(dy_view);
  dx_view=DestroyCacheView(dx_view);
  if (status == MagickFalse)
    magnitude_image=DestroyImage(magnitude_image);
  return(magnitude_image);
}

typedef struct _BlendRowPointers
{
  const Quantum
    *p,
    *q,
    *r,
    *s,
    *t;
} BlendRowPointers;

static inline MagickBooleanType IsBlendRowsMissing(
  const BlendRowPointers *rows)
{
  const Quantum
    *p = rows->p,
    *q = rows->q,
    *r = rows->r,
    *s = rows->s,
    *t = rows->t;

  return(((p == (const Quantum *) NULL) || (q == (const Quantum *) NULL) ||
    (r == (const Quantum *) NULL) || (s == (const Quantum *) NULL) ||
    (t == (Quantum *) NULL)) ? MagickTrue : MagickFalse);
}

typedef struct _MaxMagnitudeInputs
{
  const Image
    *alpha_image,
    *beta_image,
    *dx_image,
    *dy_image;

  const Quantum
    *p,
    *q;
} MaxMagnitudeInputs;

static void SelectMaxMagnitudePixel(const MaxMagnitudeInputs *selection,
  const Quantum *magick_restrict r,const Quantum *magick_restrict s,
  Quantum *magick_restrict t)
{
  const Image
    *alpha_image = selection->alpha_image,
    *beta_image = selection->beta_image,
    *dx_image = selection->dx_image,
    *dy_image = selection->dy_image;

  const Quantum
    *magick_restrict p = selection->p,
    *magick_restrict q = selection->q;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(alpha_image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(alpha_image,i);
    PixelTrait traits = GetPixelChannelTraits(alpha_image,channel);
    PixelTrait beta_traits = GetPixelChannelTraits(beta_image,channel);
    if (IsBlendChannelSkipped(traits,beta_traits) != MagickFalse)
      continue;
    if (p[i] > GetPixelChannel(beta_image,channel,q))
      t[i]=GetPixelChannel(dx_image,channel,r);
    else
      t[i]=GetPixelChannel(dy_image,channel,s);
  }
}

typedef struct _MaxMagnitudeLine
{
  CacheView
    *alpha_view,
    *beta_view,
    *dx_view,
    *dy_view,
    *magnitude_view;

  const Image
    *alpha_image,
    *beta_image,
    *dx_image,
    *dy_image;

  Image
    *magnitude_image;

  ExceptionInfo
    *exception;
} MaxMagnitudeLine;

static void SelectMaxMagnitudeLine(const MaxMagnitudeLine *line,
  const ssize_t y,MagickBooleanType *status)
{
  BlendRowPointers
    rows;

  const Quantum
    *magick_restrict p,
    *magick_restrict q,
    *magick_restrict r,
    *magick_restrict s;

  Quantum
    *magick_restrict t;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(line->alpha_view,0,y,
    line->alpha_image->columns,1,line->exception);
  q=GetCacheViewVirtualPixels(line->beta_view,0,y,line->alpha_image->columns,1,
    line->exception);
  r=GetCacheViewVirtualPixels(line->dx_view,0,y,line->alpha_image->columns,1,
    line->exception);
  s=GetCacheViewVirtualPixels(line->dy_view,0,y,line->alpha_image->columns,1,
    line->exception);
  t=GetCacheViewAuthenticPixels(line->magnitude_view,0,y,
    line->alpha_image->columns,1,line->exception);
  rows.p=p;
  rows.q=q;
  rows.r=r;
  rows.s=s;
  rows.t=t;
  if (IsBlendRowsMissing(&rows) != MagickFalse)
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) line->alpha_image->columns; x++)
  {
    MaxMagnitudeInputs
      selection;

    selection.alpha_image=line->alpha_image;
    selection.beta_image=line->beta_image;
    selection.dx_image=line->dx_image;
    selection.dy_image=line->dy_image;
    selection.p=p;
    selection.q=q;
    SelectMaxMagnitudePixel(&selection,r,s,t);
    p+=(ptrdiff_t) GetPixelChannels(line->alpha_image);
    q+=(ptrdiff_t) GetPixelChannels(line->beta_image);
    r+=(ptrdiff_t) GetPixelChannels(line->dx_image);
    s+=(ptrdiff_t) GetPixelChannels(line->dy_image);
    t+=(ptrdiff_t) GetPixelChannels(line->magnitude_image);
  }
  SyncCompositeRow(line->magnitude_view,line->exception,status);
}

static Image *BlendMaxMagnitudeImage(const BlendMagnitudePair *magnitudes,
  const Image *dx_image,const Image *dy_image,ExceptionInfo *exception)
{
  const Image
    *alpha_image = magnitudes->alpha_image,
    *beta_image = magnitudes->beta_image;

  CacheView
    *alpha_view,
    *beta_view,
    *dx_view,
    *dy_view,
    *magnitude_view;

  Image
    *magnitude_image;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    y;

  /*
    Select the larger of two magnitudes.
  */
  magnitude_image=CloneImage(alpha_image,0,0,MagickTrue,exception);
  if (magnitude_image == (Image *) NULL)
    return(magnitude_image);
  alpha_view=AcquireVirtualCacheView(alpha_image,exception);
  beta_view=AcquireVirtualCacheView(beta_image,exception);
  dx_view=AcquireVirtualCacheView(dx_image,exception);
  dy_view=AcquireVirtualCacheView(dy_image,exception);
  magnitude_view=AcquireAuthenticCacheView(magnitude_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(alpha_image,magnitude_image,alpha_image->rows,1)
#endif
  for (y=0; y < (ssize_t) alpha_image->rows; y++)
  {
    const MaxMagnitudeLine
      line = { alpha_view, beta_view, dx_view, dy_view, magnitude_view,
        alpha_image, beta_image, dx_image, dy_image, magnitude_image,
        exception };

    SelectMaxMagnitudeLine(&line,y,&status);
  }
  magnitude_view=DestroyCacheView(magnitude_view);
  dy_view=DestroyCacheView(dy_view);
  dx_view=DestroyCacheView(dx_view);
  beta_view=DestroyCacheView(beta_view);
  alpha_view=DestroyCacheView(alpha_view);
  if (status == MagickFalse)
    magnitude_image=DestroyImage(magnitude_image);
  return(magnitude_image);
}

typedef struct _SumBlendInputs
{
  const Image
    *alpha_image,
    *beta_image;

  Image
    *sum_image;

  double
    attenuate,
    sign;
} SumBlendInputs;

static void SumBlendRow(const SumBlendInputs *row,
  const Quantum *magick_restrict p,const Quantum *magick_restrict q,
  Quantum *magick_restrict r)
{
  const Image
    *alpha_image = row->alpha_image,
    *beta_image = row->beta_image;

  Image
    *sum_image = row->sum_image;

  const double
    attenuate = row->attenuate,
    sign = row->sign;

  ssize_t
    x;

  for (x=0; x < (ssize_t) alpha_image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(alpha_image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(alpha_image,i);
      PixelTrait traits = GetPixelChannelTraits(alpha_image,channel);
      PixelTrait beta_traits = GetPixelChannelTraits(beta_image,channel);
      if (IsBlendChannelSkipped(traits,beta_traits) != MagickFalse)
        continue;
      r[i]=ClampToQuantum(attenuate*((double) p[i]+sign*
        (double) GetPixelChannel(beta_image,channel,q)));
    }
    p+=(ptrdiff_t) GetPixelChannels(alpha_image);
    q+=(ptrdiff_t) GetPixelChannels(beta_image);
    r+=(ptrdiff_t) GetPixelChannels(sum_image);
  }
}

static Image *FinishBlendImage(Image *sum_image,
  const MagickBooleanType status)
{
  if (status == MagickFalse)
    sum_image=DestroyImage(sum_image);
  return(sum_image);
}

static Image *BlendSumImage(const Image *alpha_image,const Image *beta_image,
  const BlendSumWeights *weights,ExceptionInfo *exception)
{
  const double
    attenuate = weights->attenuate,
    sign = weights->sign;

  CacheView
    *alpha_view,
    *beta_view,
    *sum_view;

  Image
    *sum_image;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    y;

  /*
    Add or subtract and optionally attenuate two images.
  */
  sum_image=CloneImage(alpha_image,0,0,MagickTrue,exception);
  if (sum_image == (Image *) NULL)
    return(sum_image);
  alpha_view=AcquireVirtualCacheView(alpha_image,exception);
  beta_view=AcquireVirtualCacheView(beta_image,exception);
  sum_view=AcquireAuthenticCacheView(sum_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(alpha_image,sum_image,alpha_image->rows,1)
#endif
  for (y=0; y < (ssize_t) alpha_image->rows; y++)
  {
    const Quantum
      *magick_restrict p,
      *magick_restrict q;

    Quantum
      *magick_restrict r;

    SumBlendInputs
      row;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(alpha_view,0,y,alpha_image->columns,1,
      exception);
    q=GetCacheViewVirtualPixels(beta_view,0,y,alpha_image->columns,1,exception);
    r=GetCacheViewAuthenticPixels(sum_view,0,y,alpha_image->columns,1,
      exception);
    if (IsBlendRowMissing(p,q,r) != MagickFalse)
      {
        status=MagickFalse;
        continue;
      }
    row.alpha_image=alpha_image;
    row.beta_image=beta_image;
    row.sum_image=sum_image;
    row.attenuate=attenuate;
    row.sign=sign;
    SumBlendRow(&row,p,q,r);
    SyncCompositeRow(sum_view,exception,&status);
  }
  sum_view=DestroyCacheView(sum_view);
  beta_view=DestroyCacheView(beta_view);
  alpha_view=DestroyCacheView(alpha_view);
  return(FinishBlendImage(sum_image,status));
}

typedef struct _BlendGradientSet
{
  Image
    *dx_image,
    *dy_image,
    *magnitude_image;
} BlendGradientSet;

static MagickBooleanType BlendGradientImages(const Image *image,
  BlendGradientSet *gradients,ExceptionInfo *exception)
{
  /*
    Create X and Y gradient images and the magnitude.
  */
  gradients->dx_image=BlendConvolveImage(image,"3x1:-0.5,0.0,0.5",exception);
  if (gradients->dx_image == (Image *) NULL)
    return(MagickFalse);
  gradients->dy_image=BlendConvolveImage(image,"1x3:-0.5,0.0,0.5",exception);
  if (gradients->dy_image == (Image *) NULL)
    return(MagickFalse);
  gradients->magnitude_image=BlendMagnitudeImage(gradients->dx_image,
    gradients->dy_image,exception);
  if (gradients->magnitude_image == (Image *) NULL)
    return(MagickFalse);
  return(MagickTrue);
}

static void DestroyBlendGradients(BlendGradientSet *gradients)
{
  if (gradients->magnitude_image != (Image *) NULL)
    gradients->magnitude_image=DestroyImage(gradients->magnitude_image);
  if (gradients->dy_image != (Image *) NULL)
    gradients->dy_image=DestroyImage(gradients->dy_image);
  if (gradients->dx_image != (Image *) NULL)
    gradients->dx_image=DestroyImage(gradients->dx_image);
}

static void ReleaseUnselectedGradients(BlendGradientSet *alpha,
  BlendGradientSet *beta)
{
  beta->dx_image=DestroyImage(beta->dx_image);
  alpha->dx_image=DestroyImage(alpha->dx_image);
  beta->magnitude_image=DestroyImage(beta->magnitude_image);
  alpha->magnitude_image=DestroyImage(alpha->magnitude_image);
}

static MagickBooleanType SelectBlendGradients(BlendGradientSet *alpha,
  BlendGradientSet *beta,BlendGradientSet *selected,ExceptionInfo *exception)
{
  BlendMagnitudePair magnitudes;

  /*
    Select alpha or beta gradient for larger of two magnitudes.
  */
  magnitudes.alpha_image=alpha->magnitude_image;
  magnitudes.beta_image=beta->magnitude_image;
  selected->dx_image=BlendMaxMagnitudeImage(&magnitudes,alpha->dx_image,
    beta->dx_image,exception);
  if (selected->dx_image == (Image *) NULL)
    return(MagickFalse);
  selected->dy_image=BlendMaxMagnitudeImage(&magnitudes,alpha->dy_image,
    beta->dy_image,exception);
  if (selected->dy_image == (Image *) NULL)
    return(MagickFalse);
  ReleaseUnselectedGradients(alpha,beta);
  return(MagickTrue);
}

static Image *BlendDivergenceImage(const BlendGradientSet *selected,
  ExceptionInfo *exception)
{
  Image
    *divergent_image,
    *dx_divergent,
    *dy_divergent;

  const BlendSumWeights quarter_sum = { 0.25, 1.0 };

  /*
    Create divergence of gradients dx and dy and divide by 4 as guide image.
  */
  dx_divergent=BlendConvolveImage(selected->dx_image,"3x1:-0.5,0.0,0.5",
    exception);
  if (dx_divergent == (Image *) NULL)
    return((Image *) NULL);
  dy_divergent=BlendConvolveImage(selected->dy_image,"1x3:-0.5,0.0,0.5",
    exception);
  if (dy_divergent == (Image *) NULL)
    return((Image *) NULL);
  divergent_image=BlendSumImage(dx_divergent,dy_divergent,&quarter_sum,
    exception);
  dy_divergent=DestroyImage(dy_divergent);
  dx_divergent=DestroyImage(dx_divergent);
  return(divergent_image);
}

static Image *BlendDivergentImage(const Image *alpha_image,
  const Image *beta_image,ExceptionInfo *exception)
{
  BlendGradientSet
    alpha = { (Image *) NULL, (Image *) NULL, (Image *) NULL },
    beta = { (Image *) NULL, (Image *) NULL, (Image *) NULL },
    selected = { (Image *) NULL, (Image *) NULL, (Image *) NULL };

  Image
    *divergent_image = (Image *) NULL;

  MagickBooleanType
    status;

  status=BlendGradientImages(alpha_image,&alpha,exception);
  if (status != MagickFalse)
    status=BlendGradientImages(beta_image,&beta,exception);
  if (status != MagickFalse)
    status=SelectBlendGradients(&alpha,&beta,&selected,exception);
  if (status != MagickFalse)
    divergent_image=BlendDivergenceImage(&selected,exception);
  DestroyBlendGradients(&selected);
  DestroyBlendGradients(&beta);
  DestroyBlendGradients(&alpha);
  return(divergent_image);
}

static inline MagickBooleanType IsBlendRowPairMissing(const Quantum *p,
  const Quantum *q)
{
  return(((p == (const Quantum *) NULL) || (q == (Quantum *) NULL)) ?
    MagickTrue : MagickFalse);
}

static void ClearMaskedAlphaRow(const Image *image,const Image *mask_image,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    Quantum
      alpha = GetPixelAlpha(mask_image,p);

    ssize_t
      i = GetPixelChannelOffset(image,AlphaPixelChannel);

    if (fabs((double) alpha) >= MagickEpsilon)
      q[i]=(Quantum) 0;
    p+=(ptrdiff_t) GetPixelChannels(mask_image);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType BlendMaskAlphaChannel(Image *image,
  const Image *mask_image,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *mask_view;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    y;

  /*
    Threshold the alpha channel.
  */
  if (SetImageAlpha(image,OpaqueAlpha,exception) == MagickFalse)
    return(MagickFalse);
  image_view=AcquireAuthenticCacheView(image,exception);
  mask_view=AcquireVirtualCacheView(mask_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(mask_view,0,y,image->columns,1,exception);
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (IsBlendRowPairMissing(p,q) != MagickFalse)
      {
        status=MagickFalse;
        continue;
      }
    ClearMaskedAlphaRow(image,mask_image,p,q);
    SyncCompositeRow(image_view,exception,&status);
  }
  mask_view=DestroyCacheView(mask_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

typedef struct _MeanFillInputs
{
  const Image
    *mask_image;

  Image
    *mean_image;
} MeanFillInputs;

static void FillMeanRow(const MeanFillInputs *row,const double *mean,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *mask_image = row->mask_image;

  Image
    *mean_image = row->mean_image;

  ssize_t
    x;

  for (x=0; x < (ssize_t) mean_image->columns; x++)
  {
    Quantum
      alpha = GetPixelAlpha(mask_image,p),
      mask = GetPixelReadMask(mask_image,p);

    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(mean_image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(mean_image,i);
      PixelTrait traits = GetPixelChannelTraits(mean_image,channel);
      if (traits == UndefinedPixelTrait)
        continue;
      if (mask <= (QuantumRange/2))
        q[i]=(Quantum) 0;
      else
        if (fabs((double) alpha) >= MagickEpsilon)
          q[i]=ClampToQuantum(mean[i]);
    }
    p+=(ptrdiff_t) GetPixelChannels(mask_image);
    q+=(ptrdiff_t) GetPixelChannels(mean_image);
  }
}

static void AccumulateMeanRow(const Image *image,
  const Quantum *magick_restrict p,double *mean)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,i);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if (traits == UndefinedPixelTrait)
        continue;
      mean[i]+=QuantumScale*(double) p[i];
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType SumImageChannels(const Image *image,double *mean,
  ExceptionInfo *exception)
{
  CacheView
    *alpha_view;

  ssize_t
    y;

  alpha_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    p=GetCacheViewVirtualPixels(alpha_view,0,y,image->columns,1,
      exception);
    if (p == (const Quantum *) NULL)
      break;
    AccumulateMeanRow(image,p,mean);
  }
  alpha_view=DestroyCacheView(alpha_view);
  if (y < (ssize_t) image->rows)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _MeanFillLine
{
  CacheView
    *mask_view,
    *mean_view;

  const Image
    *mask_image;

  Image
    *mean_image;

  const double
    *mean;

  ExceptionInfo
    *exception;
} MeanFillLine;

static void FillMeanLine(const MeanFillLine *line,const ssize_t y,
  MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  MeanFillInputs
    row;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(line->mask_view,0,y,line->mean_image->columns,1,
    line->exception);
  q=GetCacheViewAuthenticPixels(line->mean_view,0,y,line->mean_image->columns,
    1,line->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return;
    }
  row.mask_image=line->mask_image;
  row.mean_image=line->mean_image;
  FillMeanRow(&row,line->mean,p,q);
  SyncCompositeRow(line->mean_view,line->exception,status);
}

static Image *BlendMeanImage(Image *image,const Image *mask_image,
  ExceptionInfo *exception)
{
  CacheView
    *mask_view,
    *mean_view;

  double
    mean[MaxPixelChannels];

  Image
    *mean_image;

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    j,
    y;

  /*
    Compute the mean of the image.
  */
  (void) memset(mean,0,MaxPixelChannels*sizeof(*mean));
  if (SumImageChannels(image,mean,exception) == MagickFalse)
    return((Image *) NULL);
  for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
    mean[j]=(double) QuantumRange*mean[j]/image->columns/
      image->rows;
  /*
    Replace any unmasked pixels with the mean pixel.
  */
  mean_image=CloneImage(image,0,0,MagickTrue,exception);
  if (mean_image == (Image *) NULL)
    return(mean_image);
  mask_view=AcquireVirtualCacheView(mask_image,exception);
  mean_view=AcquireAuthenticCacheView(mean_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(mask_image,mean_image,mean_image->rows,4)
#endif
  for (y=0; y < (ssize_t) mean_image->rows; y++)
  {
    const MeanFillLine
      line = { mask_view, mean_view, mask_image, mean_image, mean, exception };

    FillMeanLine(&line,y,&status);
  }
  mask_view=DestroyCacheView(mask_view);
  mean_view=DestroyCacheView(mean_view);
  if (status == MagickFalse)
    mean_image=DestroyImage(mean_image);
  return(mean_image);
}

typedef struct _ResidualPixels
{
  const Image
    *beta_image;

  const Quantum
    *p,
    *q;

  double
    Da,
    Sa;
} ResidualPixels;

static double GetResidualDistance(const ResidualPixels *residual,
  const PixelChannel channel,const ssize_t i)
{
  double
    distance;

  if (channel == AlphaPixelChannel)
    distance=QuantumScale*((double) residual->p[i]-(double) GetPixelChannel(
      residual->beta_image,channel,residual->q));
  else
    distance=QuantumScale*(residual->Sa*(double) residual->p[i]-residual->Da*
      (double) GetPixelChannel(residual->beta_image,channel,residual->q));
  return(distance);
}

typedef struct _ChannelResidualInputs
{
  const Image
    *alpha_image,
    *beta_image;

  const Quantum
    *p,
    *q;

  double
    Da,
    Sa;
} ChannelResidualInputs;

static void AddChannelResiduals(const ChannelResidualInputs *inputs,
  double *channel_residual)
{
  const Image
    *const alpha_image = inputs->alpha_image,
    *const beta_image = inputs->beta_image;

  const Quantum
    *magick_restrict p = inputs->p,
    *magick_restrict q = inputs->q;

  const double
    Da = inputs->Da,
    Sa = inputs->Sa;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(alpha_image); i++)
  {
    double
      distance;

    ResidualPixels
      residual;

    PixelChannel channel = GetPixelChannelChannel(alpha_image,i);
    PixelTrait traits = GetPixelChannelTraits(alpha_image,channel);
    PixelTrait beta_traits = GetPixelChannelTraits(beta_image,channel);
    if (IsBlendChannelSkipped(traits,beta_traits) != MagickFalse)
      continue;
    residual.beta_image=beta_image;
    residual.p=p;
    residual.q=q;
    residual.Da=Da;
    residual.Sa=Sa;
    distance=GetResidualDistance(&residual,channel,i);
    (*channel_residual)+=distance*distance;
  }
}

typedef struct _RMSEResidualRow
{
  const Image
    *alpha_image,
    *beta_image;

  const Quantum
    *p,
    *q;

  size_t
    columns;
} RMSEResidualRow;

static void AddRMSERowResidual(const RMSEResidualRow *row,double *area,
  double *channel_residual)
{
  const Image
    *const alpha_image = row->alpha_image,
    *const beta_image = row->beta_image;

  const Quantum
    *magick_restrict p = row->p,
    *magick_restrict q = row->q;

  const size_t
    columns = row->columns;

  ssize_t
    x;

  for (x=0; x < (ssize_t) columns; x++)
  {
    ChannelResidualInputs
      inputs;

    double
      Da,
      Sa;

    if ((GetPixelReadMask(alpha_image,p) <= (QuantumRange/2)) ||
        (GetPixelReadMask(beta_image,q) <= (QuantumRange/2)))
      {
        p+=(ptrdiff_t) GetPixelChannels(alpha_image);
        q+=(ptrdiff_t) GetPixelChannels(beta_image);
        continue;
      }
    Sa=QuantumScale*(double) GetPixelAlpha(alpha_image,p);
    Da=QuantumScale*(double) GetPixelAlpha(beta_image,q);
    inputs.alpha_image=alpha_image;
    inputs.beta_image=beta_image;
    inputs.p=p;
    inputs.q=q;
    inputs.Da=Da;
    inputs.Sa=Sa;
    AddChannelResiduals(&inputs,channel_residual);
    (*area)++;
    p+=(ptrdiff_t) GetPixelChannels(alpha_image);
    q+=(ptrdiff_t) GetPixelChannels(beta_image);
  }
}

static MagickBooleanType BlendRMSEResidual(const Image *alpha_image,
  const Image *beta_image,double *residual,ExceptionInfo *exception)
{
  CacheView
    *alpha_view,
    *beta_view;

  double
    area = 0.0,
    channel_residual = 0.0;

  MagickBooleanType
    status = MagickTrue;

  size_t
    columns = MagickMax(alpha_image->columns,beta_image->columns),
    rows = MagickMax(alpha_image->rows,beta_image->rows);

  ssize_t
    y;

  alpha_view=AcquireVirtualCacheView(alpha_image,exception);
  beta_view=AcquireVirtualCacheView(beta_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    reduction(+:area) reduction(+:channel_residual) \
    magick_number_threads(alpha_image,alpha_image,rows,1)
#endif
  for (y=0; y < (ssize_t) rows; y++)
  {
    const Quantum
      *magick_restrict p,
      *magick_restrict q;

    RMSEResidualRow
      row;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(alpha_view,0,y,columns,1,exception);
    q=GetCacheViewVirtualPixels(beta_view,0,y,columns,1,exception);
    if ((p == (const Quantum *) NULL) || (q == (const Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    channel_residual=0.0;
    row.alpha_image=alpha_image;
    row.beta_image=beta_image;
    row.p=p;
    row.q=q;
    row.columns=columns;
    AddRMSERowResidual(&row,&area,&channel_residual);
  }
  beta_view=DestroyCacheView(beta_view);
  alpha_view=DestroyCacheView(alpha_view);
  area=MagickSafeReciprocal(area);
  *residual=sqrt(area*channel_residual/(double) GetImageChannels(alpha_image));
  return(status);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
