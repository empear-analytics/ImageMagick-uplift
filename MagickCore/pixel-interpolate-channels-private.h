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

  MagickCore private methods of pixel.c: the per-method helpers of
  InterpolatePixelChannels, InterpolateBilinearChannels to
  InterpolateAverageChannels; included by that file only.
*/
#ifndef MAGICKCORE_PIXEL_INTERPOLATE_CHANNELS_PRIVATE_H
#define MAGICKCORE_PIXEL_INTERPOLATE_CHANNELS_PRIVATE_H

typedef struct _ChannelsInterpolation
{
  const Image
    *source;

  const CacheView_
    *source_view;

  const Image
    *destination;

  double
    x,
    y;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} ChannelsInterpolation;

static MagickBooleanType InterpolateBilinearChannels(
  const ChannelsInterpolation *grid,Quantum *pixel,MagickBooleanType status)
{
  const Quantum
    *magick_restrict p;

  double
    alpha[16],
    gamma,
    pixels[16];

  ssize_t
    i;

  p=GetCacheViewVirtualPixels(grid->source_view,grid->x_offset,grid->y_offset,
    2,2,grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  for (i=0; i < (ssize_t) GetPixelChannels(grid->source); i++)
  {
    PointInfo
      delta,
      epsilon;

    PixelChannel channel = GetPixelChannelChannel(grid->source,i);
    PixelTrait traits = GetPixelChannelTraits(grid->source,channel);
    PixelTrait destination_traits=GetPixelChannelTraits(grid->destination,
      channel);
    if ((traits == UndefinedPixelTrait) ||
        (destination_traits == UndefinedPixelTrait))
      continue;
    delta.x=grid->x-grid->x_offset;
    delta.y=grid->y-grid->y_offset;
    epsilon.x=1.0-delta.x;
    epsilon.y=1.0-delta.y;
    pixels[0]=(double) p[i];
    pixels[1]=(double) p[(ssize_t) GetPixelChannels(grid->source)+i];
    pixels[2]=(double) p[2*(ssize_t) GetPixelChannels(grid->source)+i];
    pixels[3]=(double) p[3*(ssize_t) GetPixelChannels(grid->source)+i];
    if ((traits & BlendPixelTrait) == 0)
      {
        gamma=((epsilon.y*(epsilon.x+delta.x)+delta.y*(epsilon.x+delta.x)));
        gamma=MagickSafeReciprocal(gamma);
        SetPixelChannel(grid->destination,channel,ClampToQuantum(gamma*(
          epsilon.y*(epsilon.x*pixels[0]+delta.x*pixels[1])+delta.y*(epsilon.x*
          pixels[2]+delta.x*pixels[3]))),pixel);
        continue;
      }
    alpha[0]=QuantumScale*(double) GetPixelAlpha(grid->source,p);
    alpha[1]=QuantumScale*(double) GetPixelAlpha(grid->source,p+
      GetPixelChannels(grid->source));
    alpha[2]=QuantumScale*(double) GetPixelAlpha(grid->source,p+2*
      GetPixelChannels(grid->source));
    alpha[3]=QuantumScale*(double) GetPixelAlpha(grid->source,p+3*
      GetPixelChannels(grid->source));
    pixels[0]*=alpha[0];
    pixels[1]*=alpha[1];
    pixels[2]*=alpha[2];
    pixels[3]*=alpha[3];
    gamma=((epsilon.y*(epsilon.x*alpha[0]+delta.x*alpha[1])+delta.y*
      (epsilon.x*alpha[2]+delta.x*alpha[3])));
    gamma=MagickSafeReciprocal(gamma);
    SetPixelChannel(grid->destination,channel,ClampToQuantum(gamma*(epsilon.y*
      (epsilon.x*pixels[0]+delta.x*pixels[1])+delta.y*(epsilon.x*pixels[2]+
      delta.x*pixels[3]))),pixel);
  }
  return(status);
}

static MagickBooleanType InterpolateIntegerChannels(
  const ChannelsInterpolation *grid,Quantum *pixel,MagickBooleanType status)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    i;

  p=GetCacheViewVirtualPixels(grid->source_view,grid->x_offset,grid->y_offset,
    1,1,grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  for (i=0; i < (ssize_t) GetPixelChannels(grid->source); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(grid->source,i);
    PixelTrait traits = GetPixelChannelTraits(grid->source,channel);
    PixelTrait destination_traits=GetPixelChannelTraits(grid->destination,
      channel);
    if ((traits == UndefinedPixelTrait) ||
        (destination_traits == UndefinedPixelTrait))
      continue;
    SetPixelChannel(grid->destination,channel,p[i],pixel);
  }
  return(status);
}

typedef struct _ChannelAverage
{
  const Image
    *source,
    *destination;

  const Quantum
    *p;

  ssize_t
    count;
} ChannelAverage;

static void AverageInterpolateChannel(const ChannelAverage *average,
  const ssize_t i,Quantum *pixel)
{
  const Image
    *source = average->source,
    *destination = average->destination;

  const Quantum
    *p = average->p;

  const ssize_t
    count = average->count;

  double
    alpha[16],
    gamma,
    pixels[16],
    sum;

  ssize_t
    j;

  PixelChannel channel = GetPixelChannelChannel(source,i);
  PixelTrait traits = GetPixelChannelTraits(source,channel);
  PixelTrait destination_traits=GetPixelChannelTraits(destination,
    channel);
  if ((traits == UndefinedPixelTrait) ||
      (destination_traits == UndefinedPixelTrait))
    return;
  for (j=0; j < (ssize_t) count; j++)
    pixels[j]=(double) p[j*(ssize_t) GetPixelChannels(source)+i];
  sum=0.0;
  if ((traits & BlendPixelTrait) == 0)
    {
      for (j=0; j < (ssize_t) count; j++)
        sum+=pixels[j];
      sum/=count;
      SetPixelChannel(destination,channel,ClampToQuantum(sum),pixel);
      return;
    }
  for (j=0; j < (ssize_t) count; j++)
  {
    alpha[j]=QuantumScale*(double) GetPixelAlpha(source,p+j*
      (ssize_t) GetPixelChannels(source));
    pixels[j]*=alpha[j];
    gamma=MagickSafeReciprocal(alpha[j]);
    sum+=gamma*pixels[j];
  }
  sum/=count;
  SetPixelChannel(destination,channel,ClampToQuantum(sum),pixel);
}

static void AverageInterpolateChannels(const ChannelsInterpolation *grid,
  const Quantum *magick_restrict p,const ssize_t count,Quantum *pixel)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(grid->source); i++)
  {
    ChannelAverage
      average;

    average.source=grid->source;
    average.destination=grid->destination;
    average.p=p;
    average.count=count;
    AverageInterpolateChannel(&average,i,pixel);
  }
}

static void InterpolateCubicChannel(const ChannelsInterpolation *grid,
  void (*weights)(const double,double (*)[4]),const Quantum *magick_restrict p,
  const ssize_t i,Quantum *pixel)
{
  double
    alpha[16],
    cx[4],
    cy[4],
    gamma,
    pixels[16];

  ssize_t
    j;

  PixelChannel channel = GetPixelChannelChannel(grid->source,i);
  PixelTrait traits = GetPixelChannelTraits(grid->source,channel);
  PixelTrait destination_traits=GetPixelChannelTraits(grid->destination,
    channel);
  if ((traits == UndefinedPixelTrait) ||
      (destination_traits == UndefinedPixelTrait))
    return;
  if ((traits & BlendPixelTrait) == 0)
    for (j=0; j < 16; j++)
    {
      alpha[j]=1.0;
      pixels[j]=(double) p[j*(ssize_t) GetPixelChannels(grid->source)+i];
    }
  else
    for (j=0; j < 16; j++)
    {
      alpha[j]=QuantumScale*(double) GetPixelAlpha(grid->source,p+j*
        (ssize_t) GetPixelChannels(grid->source));
      pixels[j]=alpha[j]*(double)
        p[j*(ssize_t) GetPixelChannels(grid->source)+i];
    }
  weights((double) (grid->x-grid->x_offset),&cx);
  weights((double) (grid->y-grid->y_offset),&cy);
  gamma=((traits & BlendPixelTrait) ? (double) (1.0) :
    MagickSafeReciprocal(cy[0]*(cx[0]*alpha[0]+cx[1]*alpha[1]+cx[2]*
    alpha[2]+cx[3]*alpha[3])+cy[1]*(cx[0]*alpha[4]+cx[1]*alpha[5]+cx[2]*
    alpha[6]+cx[3]*alpha[7])+cy[2]*(cx[0]*alpha[8]+cx[1]*alpha[9]+cx[2]*
    alpha[10]+cx[3]*alpha[11])+cy[3]*(cx[0]*alpha[12]+cx[1]*alpha[13]+
    cx[2]*alpha[14]+cx[3]*alpha[15])));
  SetPixelChannel(grid->destination,channel,ClampToQuantum(gamma*(cy[0]*(cx[0]*
    pixels[0]+cx[1]*pixels[1]+cx[2]*pixels[2]+cx[3]*pixels[3])+cy[1]*
    (cx[0]*pixels[4]+cx[1]*pixels[5]+cx[2]*pixels[6]+cx[3]*pixels[7])+
    cy[2]*(cx[0]*pixels[8]+cx[1]*pixels[9]+cx[2]*pixels[10]+cx[3]*
    pixels[11])+cy[3]*(cx[0]*pixels[12]+cx[1]*pixels[13]+cx[2]*
    pixels[14]+cx[3]*pixels[15]))),pixel);
}

static void LoadBlendSamples(const ChannelsInterpolation *grid,
  const Quantum *magick_restrict p,const ssize_t i,const PixelChannel channel,
  double *alpha,double *pixels)
{
  ssize_t
    j;

  for (j=0; j < 4; j++)
  {
    alpha[j]=QuantumScale*(double) GetPixelAlpha(grid->source,p+j*
      (ssize_t) GetPixelChannels(grid->source));
    pixels[j]=(double) p[j*(ssize_t) GetPixelChannels(grid->source)+i];
    if (channel != AlphaPixelChannel)
      pixels[j]*=alpha[j];
  }
}

static void InterpolateBlendChannel(const ChannelsInterpolation *grid,
  const Quantum *magick_restrict p,const ssize_t i,Quantum *pixel)
{
  double
    alpha[16],
    pixels[16];

  PointInfo
    delta;

  ssize_t
    j;

  PixelChannel channel = GetPixelChannelChannel(grid->source,i);
  PixelTrait traits = GetPixelChannelTraits(grid->source,channel);
  PixelTrait destination_traits=GetPixelChannelTraits(grid->destination,
    channel);
  if ((traits == UndefinedPixelTrait) ||
      (destination_traits == UndefinedPixelTrait))
    return;
  if (grid->source->alpha_trait != BlendPixelTrait)
    for (j=0; j < 4; j++)
    {
      alpha[j]=1.0;
      pixels[j]=(double) p[j*(ssize_t) GetPixelChannels(grid->source)+i];
    }
  else
    LoadBlendSamples(grid,p,i,channel,alpha,pixels);
  delta.x=grid->x-grid->x_offset;
  delta.y=grid->y-grid->y_offset;
  SetPixelChannel(grid->destination,channel,ClampToQuantum(
    BlendInterpolateSamples(alpha,pixels,&delta,(MagickBooleanType)
    (channel != AlphaPixelChannel))),
    pixel);
}

static void InterpolateMeshChannel(const ChannelsInterpolation *grid,
  const Quantum *magick_restrict p,const ssize_t i,Quantum *pixel)
{
  const ssize_t
    *corner;

  double
    alpha[16],
    gamma,
    pixels[16];

  PointInfo
    delta,
    luminance;

  PixelChannel channel = GetPixelChannelChannel(grid->source,i);
  PixelTrait traits = GetPixelChannelTraits(grid->source,channel);
  PixelTrait destination_traits=GetPixelChannelTraits(grid->destination,
    channel);
  if ((traits == UndefinedPixelTrait) ||
      (destination_traits == UndefinedPixelTrait))
    return;
  pixels[0]=(double) p[i];
  pixels[1]=(double) p[(ssize_t) GetPixelChannels(grid->source)+i];
  pixels[2]=(double) p[2*(ssize_t) GetPixelChannels(grid->source)+i];
  pixels[3]=(double) p[3*(ssize_t) GetPixelChannels(grid->source)+i];
  if ((traits & BlendPixelTrait) == 0)
    {
      alpha[0]=1.0;
      alpha[1]=1.0;
      alpha[2]=1.0;
      alpha[3]=1.0;
    }
  else
    {
      alpha[0]=QuantumScale*(double) GetPixelAlpha(grid->source,p);
      alpha[1]=QuantumScale*(double) GetPixelAlpha(grid->source,p+
        GetPixelChannels(grid->source));
      alpha[2]=QuantumScale*(double) GetPixelAlpha(grid->source,p+2*
        GetPixelChannels(grid->source));
      alpha[3]=QuantumScale*(double) GetPixelAlpha(grid->source,p+3*
        GetPixelChannels(grid->source));
    }
  delta.x=grid->x-grid->x_offset;
  delta.y=grid->y-grid->y_offset;
  luminance.x=fabs((double) (GetPixelLuma(grid->source,p)-
    GetPixelLuma(grid->source,p+3*GetPixelChannels(grid->source))));
  luminance.y=fabs((double) (GetPixelLuma(grid->source,p+
    GetPixelChannels(grid->source))-GetPixelLuma(grid->source,p+2*
    GetPixelChannels(grid->source))));
  corner=SelectMeshTriangle(&delta,(MagickBooleanType)
    (luminance.x < luminance.y));
  gamma=MeshInterpolate(&delta,alpha[corner[0]],alpha[corner[1]],
    alpha[corner[2]]);
  gamma=MagickSafeReciprocal(gamma);
  SetPixelChannel(grid->destination,channel,ClampToQuantum(gamma*
    MeshInterpolate(&delta,pixels[corner[0]],pixels[corner[1]],
    pixels[corner[2]])),pixel);
}

static MagickBooleanType InterpolateCubicChannels(
  const ChannelsInterpolation *grid,void (*weights)(const double,double (*)[4]),
  Quantum *pixel,MagickBooleanType status)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    i;

  p=GetCacheViewVirtualPixels(grid->source_view,grid->x_offset-1,
    grid->y_offset-1,4,4,grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  for (i=0; i < (ssize_t) GetPixelChannels(grid->source); i++)
    InterpolateCubicChannel(grid,weights,p,i,pixel);
  return(status);
}

static MagickBooleanType InterpolateQuadChannels(
  const ChannelsInterpolation *grid,void (*interpolate_channel)(
  const ChannelsInterpolation *,const Quantum *magick_restrict,const ssize_t,
  Quantum *),Quantum *pixel,MagickBooleanType status)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    i;

  p=GetCacheViewVirtualPixels(grid->source_view,grid->x_offset,grid->y_offset,
    2,2,grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  for (i=0; i < (ssize_t) GetPixelChannels(grid->source); i++)
    interpolate_channel(grid,p,i,pixel);
  return(status);
}

static void InterpolateGridChannels(const ChannelsInterpolation grid,
  const PixelInterpolateMethod interpolate,Quantum *pixel,
  MagickBooleanType *status)
{
  switch (interpolate)
  {
    case BilinearInterpolatePixel:
    default:
    {
      (*status)=InterpolateBilinearChannels(&grid,pixel,(*status));
      break;
    }
    case IntegerInterpolatePixel:
    {
      (*status)=InterpolateIntegerChannels(&grid,pixel,(*status));
      break;
    }
  }
}

static void InterpolateKernelChannels(const ChannelsInterpolation grid,
  const PixelInterpolateMethod interpolate,Quantum *pixel,
  MagickBooleanType *status)
{
  switch (interpolate)
  {
    default:
    {
      InterpolateGridChannels(grid,interpolate,pixel,status);
      break;
    }
    case CatromInterpolatePixel:
    {
      (*status)=InterpolateCubicChannels(&grid,CatromWeights,pixel,(*status));
      break;
    }
    case MeshInterpolatePixel:
    {
      (*status)=InterpolateQuadChannels(&grid,InterpolateMeshChannel,pixel,
        (*status));
      break;
    }
    case SplineInterpolatePixel:
    {
      (*status)=InterpolateCubicChannels(&grid,SplineWeights,pixel,(*status));
      break;
    }
  }
}

static MagickBooleanType InterpolateAverageChannels(
  const ChannelsInterpolation *grid,const PixelInterpolateMethod interpolate,
  Quantum *pixel,MagickBooleanType status)
{
  const Quantum
    *magick_restrict p;

  PointInfo
    point;

  RectangleInfo
    window;

  ssize_t
    count;

  point.x=grid->x;
  point.y=grid->y;
  window=GetAverageWindow(interpolate,&point,grid->x_offset,grid->y_offset);
  count=(ssize_t) window.width;
  p=GetCacheViewVirtualPixels(grid->source_view,window.x,window.y,window.width,
    window.height,grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  count*=count;  /* Number of pixels to average */
  AverageInterpolateChannels(grid,p,count,pixel);
  return(status);
}

#endif
