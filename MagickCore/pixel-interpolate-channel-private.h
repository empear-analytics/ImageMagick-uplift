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
  InterpolatePixelChannel, LoadAlphaWeightedPixels to
  InterpolatePointPixelChannel; included by that file only.
*/
#ifndef MAGICKCORE_PIXEL_INTERPOLATE_CHANNEL_PRIVATE_H
#define MAGICKCORE_PIXEL_INTERPOLATE_CHANNEL_PRIVATE_H

static void LoadAlphaWeightedPixels(const Image *magick_restrict image,
  const PixelChannel channel,const Quantum *magick_restrict p,
  const ssize_t count,double *alpha,double *pixels)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) count; i++)
  {
    alpha[i]=QuantumScale*(double) GetPixelAlpha(image,p+i*
      (ssize_t) GetPixelChannels(image));
    pixels[i]=alpha[i]*(double) p[i*(ssize_t) GetPixelChannels(image)+
      GetPixelChannelOffset(image,channel)];
  }
}

static magick_attribute((__always_inline__)) inline void
  LoadInterpolatePixels(const Image *magick_restrict image,
  const PixelChannel channel,const PixelTrait traits,
  const Quantum *magick_restrict p,const ssize_t count,double *alpha,
  double *pixels)
{
  ssize_t
    i;

  if ((traits & BlendPixelTrait) == 0)
    for (i=0; i < (ssize_t) count; i++)
    {
      alpha[i]=1.0;
      pixels[i]=(double) p[i*(ssize_t) GetPixelChannels(image)+
        GetPixelChannelOffset(image,channel)];
    }
  else
    LoadAlphaWeightedPixels(image,channel,p,count,alpha,pixels);
}

static inline void CatromWeights(const double x,double (*weights)[4])
{
  double
    alpha,
    beta,
    gamma;

  /*
    Nicolas Robidoux' 10 flops (4* + 5- + 1+) refactoring of the computation
    of the standard four 1D Catmull-Rom weights. The sampling location is
    assumed between the second and third input pixel locations, and x is the
    position relative to the second input pixel location. Formulas originally
    derived for the VIPS (Virtual Image Processing System) library.
  */
  alpha=(double) 1.0-x;
  beta=(double) (-0.5)*x*alpha;
  (*weights)[0]=alpha*beta;
  (*weights)[3]=x*beta;
  /*
    The following computation of the inner weights from the outer ones work
    for all Keys cubics.
  */
  gamma=(*weights)[3]-(*weights)[0];
  (*weights)[1]=alpha-(*weights)[0]+gamma;
  (*weights)[2]=x-(*weights)[3]-gamma;
}

static inline void SplineWeights(const double x,double (*weights)[4])
{
  double
    alpha,
    beta;

  /*
    Nicolas Robidoux' 12 flops (6* + 5- + 1+) refactoring of the computation
    of the standard four 1D cubic B-spline smoothing weights. The sampling
    location is assumed between the second and third input pixel locations,
    and x is the position relative to the second input pixel location.
  */
  alpha=(double) 1.0-x;
  (*weights)[3]=(double) (1.0/6.0)*x*x*x;
  (*weights)[0]=(double) (1.0/6.0)*alpha*alpha*alpha;
  beta=(*weights)[3]-(*weights)[0];
  (*weights)[1]=alpha-(*weights)[0]+beta;
  (*weights)[2]=x-(*weights)[3]-beta;
}

static inline double MeshInterpolate(const PointInfo *delta,const double p,
  const double x,const double y)
{
  return(delta->x*x+delta->y*y+(1.0-delta->x-delta->y)*p);
}

static const ssize_t
  mesh_corners[4][3] =
  {
    { 2, 3, 0 },  /* bottom-left triangle, diagonal 0-3 */
    { 1, 0, 3 },  /* top-right triangle, diagonal 0-3 */
    { 0, 1, 2 },  /* top-left triangle, diagonal 1-2 */
    { 3, 2, 1 }   /* bottom-right triangle, diagonal 1-2 */
  };

static const ssize_t *SelectMeshTriangle(PointInfo *delta,
  const MagickBooleanType diagonal_nw_se)
{
  if (diagonal_nw_se != MagickFalse)
    {
      if (delta->x <= delta->y)
        {
          delta->y=1.0-delta->y;
          return(mesh_corners[0]);
        }
      delta->x=1.0-delta->x;
      return(mesh_corners[1]);
    }
  if (delta->x <= (1.0-delta->y))
    return(mesh_corners[2]);
  delta->x=1.0-delta->x;
  delta->y=1.0-delta->y;
  return(mesh_corners[3]);
}

typedef struct _ChannelInterpolation
{
  const Image
    *image;

  const CacheView_
    *image_view;

  PixelChannel
    channel;

  PixelTrait
    traits;

  double
    x,
    y;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} ChannelInterpolation;

static MagickBooleanType InterpolateMeshPixelChannel(
  const ChannelInterpolation *grid,double *pixel,MagickBooleanType status)
{
  const ssize_t
    *corner;

  const Quantum
    *magick_restrict p;

  double
    alpha[16],
    gamma,
    pixels[16];

  PointInfo
    delta,
    luminance;

  p=GetCacheViewVirtualPixels(grid->image_view,grid->x_offset,grid->y_offset,
    2,2,grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  LoadInterpolatePixels(grid->image,grid->channel,grid->traits,p,4,alpha,
    pixels);
  delta.x=grid->x-grid->x_offset;
  delta.y=grid->y-grid->y_offset;
  luminance.x=GetPixelLuma(grid->image,p)-(double)
    GetPixelLuma(grid->image,p+3*GetPixelChannels(grid->image));
  luminance.y=GetPixelLuma(grid->image,p+GetPixelChannels(grid->image))-(double)
    GetPixelLuma(grid->image,p+2*GetPixelChannels(grid->image));
  corner=SelectMeshTriangle(&delta,(MagickBooleanType) (fabs((double)
    luminance.x) < fabs((double) luminance.y)));
  gamma=MeshInterpolate(&delta,alpha[corner[0]],alpha[corner[1]],
    alpha[corner[2]]);
  gamma=MagickSafeReciprocal(gamma);
  *pixel=gamma*MeshInterpolate(&delta,pixels[corner[0]],pixels[corner[1]],
    pixels[corner[2]]);
  return(status);
}

static double BlendInterpolateSamples(double *alpha,double *pixels,
  const PointInfo *delta,const MagickBooleanType color)
{
  double
    gamma;

  ssize_t
    i;

  gamma=1.0;  /* number of pixels blended together (its variable) */
  for (i=0; i <= 1L; i++) {
    if (delta->y >= 0.75)
      {
        alpha[i]=alpha[i+2];  /* take right pixels */
        pixels[i]=pixels[i+2];
      }
    else
      if (delta->y > 0.25)
        {
          gamma=2.0;  /* blend both pixels in row */
          alpha[i]+=alpha[i+2];  /* add up alpha weights */
          pixels[i]+=pixels[i+2];
        }
  }
  if (delta->x >= 0.75)
    {
      alpha[0]=alpha[1];  /* take bottom row blend */
      pixels[0]=pixels[1];
    }
  else
    if (delta->x > 0.25)
      {
        gamma*=2.0;  /* blend both rows */
        alpha[0]+=alpha[1];  /* add up alpha weights */
        pixels[0]+=pixels[1];
      }
  if (color != MagickFalse)
    gamma=MagickSafeReciprocal(alpha[0]);  /* (color) 1/alpha_weights */
  else
    gamma=MagickSafeReciprocal(gamma);  /* (alpha) 1/number_of_pixels */
  return(gamma*pixels[0]);
}

static MagickBooleanType InterpolateBlendPixelChannel(
  const ChannelInterpolation *grid,double *pixel,MagickBooleanType status)
{
  const Quantum
    *magick_restrict p;

  double
    alpha[16],
    pixels[16];

  PointInfo
    delta;

  p=GetCacheViewVirtualPixels(grid->image_view,grid->x_offset,grid->y_offset,
    2,2,grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  LoadInterpolatePixels(grid->image,grid->channel,grid->traits,p,4,alpha,
    pixels);
  delta.x=grid->x-grid->x_offset;
  delta.y=grid->y-grid->y_offset;
  *pixel=BlendInterpolateSamples(alpha,pixels,&delta,(MagickBooleanType)
    (grid->channel != AlphaPixelChannel));
  return(status);
}

static MagickBooleanType InterpolateCubicPixelChannel(
  const ChannelInterpolation *grid,void (*weights)(const double,double (*)[4]),
  double *pixel,MagickBooleanType status)
{
  const Quantum
    *magick_restrict p;

  double
    alpha[16],
    cx[4],
    cy[4],
    gamma,
    pixels[16];

  p=GetCacheViewVirtualPixels(grid->image_view,grid->x_offset-1,
    grid->y_offset-1,4,4,grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  LoadInterpolatePixels(grid->image,grid->channel,grid->traits,p,16,alpha,
    pixels);
  weights((double) (grid->x-grid->x_offset),&cx);
  weights((double) (grid->y-grid->y_offset),&cy);
  gamma=(grid->channel == AlphaPixelChannel ? (double) 1.0 :
    MagickSafeReciprocal(cy[0]*(cx[0]*alpha[0]+cx[1]*alpha[1]+cx[2]*
    alpha[2]+cx[3]*alpha[3])+cy[1]*(cx[0]*alpha[4]+cx[1]*alpha[5]+cx[2]*
    alpha[6]+cx[3]*alpha[7])+cy[2]*(cx[0]*alpha[8]+cx[1]*alpha[9]+cx[2]*
    alpha[10]+cx[3]*alpha[11])+cy[3]*(cx[0]*alpha[12]+cx[1]*alpha[13]+
    cx[2]*alpha[14]+cx[3]*alpha[15])));
  *pixel=gamma*(cy[0]*(cx[0]*pixels[0]+cx[1]*pixels[1]+cx[2]*pixels[2]+
    cx[3]*pixels[3])+cy[1]*(cx[0]*pixels[4]+cx[1]*pixels[5]+cx[2]*
    pixels[6]+cx[3]*pixels[7])+cy[2]*(cx[0]*pixels[8]+cx[1]*pixels[9]+
    cx[2]*pixels[10]+cx[3]*pixels[11])+cy[3]*(cx[0]*pixels[12]+cx[1]*
    pixels[13]+cx[2]*pixels[14]+cx[3]*pixels[15]));
  return(status);
}

static MagickBooleanType InterpolateBilinearPixelChannel(
  const ChannelInterpolation *grid,double *pixel,MagickBooleanType status)
{
  const Quantum
    *magick_restrict p;

  double
    alpha[16],
    gamma,
    pixels[16];

  PointInfo
    delta,
    epsilon;

  p=GetCacheViewVirtualPixels(grid->image_view,grid->x_offset,grid->y_offset,
    2,2,grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  LoadInterpolatePixels(grid->image,grid->channel,grid->traits,p,4,alpha,
    pixels);
  delta.x=grid->x-grid->x_offset;
  delta.y=grid->y-grid->y_offset;
  epsilon.x=1.0-delta.x;
  epsilon.y=1.0-delta.y;
  gamma=((epsilon.y*(epsilon.x*alpha[0]+delta.x*alpha[1])+delta.y*
    (epsilon.x*alpha[2]+delta.x*alpha[3])));
  gamma=MagickSafeReciprocal(gamma);
  *pixel=gamma*(epsilon.y*(epsilon.x*pixels[0]+delta.x*pixels[1])+delta.y*
    (epsilon.x*pixels[2]+delta.x*pixels[3]));
  return(status);
}

static RectangleInfo GetAverageWindow(const PixelInterpolateMethod interpolate,
  const PointInfo *point,const ssize_t x_offset,const ssize_t y_offset)
{
  RectangleInfo
    window;

  window.width=2;  /* size of the area to average - default nearest 4 */
  window.height=2;
  window.x=x_offset;
  window.y=y_offset;
  if (interpolate == Average9InterpolatePixel)
    {
      window.width=3;
      window.height=3;
      window.x=CastDoubleToSsizeT(floor(point->x+0.5)-1.0);
      window.y=CastDoubleToSsizeT(floor(point->y+0.5)-1.0);
    }
  else
    if (interpolate == Average16InterpolatePixel)
      {
        window.width=4;
        window.height=4;
        window.x--;
        window.y--;
      }
  return(window);
}

static MagickBooleanType InterpolateAveragePixelChannel(
  const ChannelInterpolation *grid,const PixelInterpolateMethod interpolate,
  double *pixel,MagickBooleanType status)
{
  const Quantum
    *magick_restrict p;

  double
    alpha[16],
    gamma,
    pixels[16];

  PointInfo
    point;

  RectangleInfo
    window;

  ssize_t
    count,
    i;

  point.x=grid->x;
  point.y=grid->y;
  window=GetAverageWindow(interpolate,&point,grid->x_offset,grid->y_offset);
  count=(ssize_t) window.width;
  p=GetCacheViewVirtualPixels(grid->image_view,window.x,window.y,window.width,
    window.height,grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  count*=count;  /* Number of pixels to average */
  LoadInterpolatePixels(grid->image,grid->channel,grid->traits,p,count,alpha,
    pixels);
  for (i=0; i < (ssize_t) count; i++)
  {
    gamma=MagickSafeReciprocal(alpha[i])/count;
    *pixel+=gamma*pixels[i];
  }
  return(status);
}

static void InterpolateGridPixelChannel(const ChannelInterpolation *grid,
  const PixelInterpolateMethod interpolate,double *pixel,
  MagickBooleanType *status)
{
  switch (interpolate)
  {
    case BilinearInterpolatePixel:
    default:
    {
      (*status)=InterpolateBilinearPixelChannel(grid,pixel,(*status));
      break;
    }
    case BlendInterpolatePixel:
    {
      (*status)=InterpolateBlendPixelChannel(grid,pixel,(*status));
      break;
    }
    case CatromInterpolatePixel:
    {
      (*status)=InterpolateCubicPixelChannel(grid,CatromWeights,pixel,
        (*status));
      break;
    }
    case MeshInterpolatePixel:
    {
      (*status)=InterpolateMeshPixelChannel(grid,pixel,(*status));
      break;
    }
    case SplineInterpolatePixel:
    {
      (*status)=InterpolateCubicPixelChannel(grid,SplineWeights,pixel,
        (*status));
      break;
    }
  }
}

static MagickBooleanType InterpolateNearestPixelChannel(
  const ChannelInterpolation *grid,double *pixel,MagickBooleanType status)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x_offset,
    y_offset;

  x_offset=CastDoubleToSsizeT(floor(grid->x+0.5));
  y_offset=CastDoubleToSsizeT(floor(grid->y+0.5));
  p=GetCacheViewVirtualPixels(grid->image_view,x_offset,y_offset,1,1,
    grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  *pixel=(double) GetPixelChannel(grid->image,grid->channel,p);
  return(status);
}

static MagickBooleanType InterpolateIntegerPixelChannel(
  const ChannelInterpolation *grid,double *pixel,MagickBooleanType status)
{
  const Quantum
    *magick_restrict p;

  p=GetCacheViewVirtualPixels(grid->image_view,grid->x_offset,grid->y_offset,
    1,1,grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  *pixel=(double) GetPixelChannel(grid->image,grid->channel,p);
  return(status);
}

static void InterpolatePointPixelChannel(const ChannelInterpolation *grid,
  const PixelInterpolateMethod interpolate,double *pixel,
  MagickBooleanType *status)
{
  switch (interpolate)
  {
    default:
    {
      InterpolateGridPixelChannel(grid,interpolate,pixel,status);
      break;
    }
    case IntegerInterpolatePixel:
    {
      (*status)=InterpolateIntegerPixelChannel(grid,pixel,(*status));
      break;
    }
    case NearestInterpolatePixel:
    {
      (*status)=InterpolateNearestPixelChannel(grid,pixel,(*status));
      break;
    }
  }
}

#endif
