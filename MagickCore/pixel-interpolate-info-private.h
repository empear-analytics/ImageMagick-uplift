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
  InterpolatePixelInfo, AlphaBlendPixelInfo to InterpolateIntegerPixelInfo;
  included by that file only.
*/
#ifndef MAGICKCORE_PIXEL_INTERPOLATE_INFO_PRIVATE_H
#define MAGICKCORE_PIXEL_INTERPOLATE_INFO_PRIVATE_H

static inline void AlphaBlendPixelInfo(const Image *image,
  const Quantum *pixel,PixelInfo *pixel_info,double *alpha)
{
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    {
      *alpha=1.0;
      pixel_info->red=(double) GetPixelRed(image,pixel);
      pixel_info->green=(double) GetPixelGreen(image,pixel);
      pixel_info->blue=(double) GetPixelBlue(image,pixel);
      pixel_info->black=0.0;
      if (image->colorspace == CMYKColorspace)
        pixel_info->black=(double) GetPixelBlack(image,pixel);
      pixel_info->alpha=(double) GetPixelAlpha(image,pixel);
      return;
    }
  *alpha=QuantumScale*(double) GetPixelAlpha(image,pixel);
  pixel_info->red=(*alpha*(double) GetPixelRed(image,pixel));
  pixel_info->green=(*alpha*(double) GetPixelGreen(image,pixel));
  pixel_info->blue=(*alpha*(double) GetPixelBlue(image,pixel));
  pixel_info->black=0.0;
  if (image->colorspace == CMYKColorspace)
    pixel_info->black=(*alpha*(double) GetPixelBlack(image,pixel));
  pixel_info->alpha=(double) GetPixelAlpha(image,pixel);
}

typedef struct _PixelInfoInterpolation
{
  const Image
    *image;

  const CacheView_
    *image_view;

  double
    x,
    y;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} PixelInfoInterpolation;

static MagickBooleanType InterpolateMeshPixelInfo(
  const PixelInfoInterpolation *grid,PixelInfo *pixels,PixelInfo *pixel,
  MagickBooleanType status)
{
  const ssize_t
    *corner;

  const Quantum
    *p;

  double
    alpha[16],
    gamma;

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
  delta.x=grid->x-grid->x_offset;
  delta.y=grid->y-grid->y_offset;
  luminance.x=GetPixelLuma(grid->image,p)-(double)
    GetPixelLuma(grid->image,p+3*GetPixelChannels(grid->image));
  luminance.y=GetPixelLuma(grid->image,p+GetPixelChannels(grid->image))-(double)
    GetPixelLuma(grid->image,p+2*GetPixelChannels(grid->image));
  AlphaBlendPixelInfo(grid->image,p,pixels+0,alpha+0);
  AlphaBlendPixelInfo(grid->image,p+GetPixelChannels(grid->image),pixels+1,
    alpha+1);
  AlphaBlendPixelInfo(grid->image,p+2*GetPixelChannels(grid->image),pixels+2,
    alpha+2);
  AlphaBlendPixelInfo(grid->image,p+3*GetPixelChannels(grid->image),pixels+3,
    alpha+3);
  corner=SelectMeshTriangle(&delta,(MagickBooleanType) (fabs((double)
    luminance.x) < fabs((double) luminance.y)));
  gamma=MeshInterpolate(&delta,alpha[corner[0]],alpha[corner[1]],
    alpha[corner[2]]);
  gamma=MagickSafeReciprocal(gamma);
  pixel->red=gamma*MeshInterpolate(&delta,pixels[corner[0]].red,
    pixels[corner[1]].red,pixels[corner[2]].red);
  pixel->green=gamma*MeshInterpolate(&delta,pixels[corner[0]].green,
    pixels[corner[1]].green,pixels[corner[2]].green);
  pixel->blue=gamma*MeshInterpolate(&delta,pixels[corner[0]].blue,
    pixels[corner[1]].blue,pixels[corner[2]].blue);
  if (grid->image->colorspace == CMYKColorspace)
    pixel->black=gamma*MeshInterpolate(&delta,pixels[corner[0]].black,
      pixels[corner[1]].black,pixels[corner[2]].black);
  gamma=MeshInterpolate(&delta,1.0,1.0,1.0);
  pixel->alpha=gamma*MeshInterpolate(&delta,pixels[corner[0]].alpha,
    pixels[corner[1]].alpha,pixels[corner[2]].alpha);
  return(status);
}

static MagickBooleanType InterpolateCubicPixelInfo(
  const PixelInfoInterpolation *grid,
  void (*weights)(const double,double (*)[4]),PixelInfo *pixels,
  PixelInfo *pixel,MagickBooleanType status)
{
  const Quantum
    *p;

  double
    alpha[16],
    cx[4],
    cy[4];

  ssize_t
    i;

  p=GetCacheViewVirtualPixels(grid->image_view,grid->x_offset-1,
    grid->y_offset-1,4,4,grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  for (i=0; i < 16L; i++)
    AlphaBlendPixelInfo(grid->image,p+i*(ssize_t) GetPixelChannels(grid->image),
      pixels+i,alpha+i);
  weights((double) (grid->x-grid->x_offset),&cx);
  weights((double) (grid->y-grid->y_offset),&cy);
  pixel->red=(cy[0]*(cx[0]*pixels[0].red+cx[1]*pixels[1].red+cx[2]*
    pixels[2].red+cx[3]*pixels[3].red)+cy[1]*(cx[0]*pixels[4].red+cx[1]*
    pixels[5].red+cx[2]*pixels[6].red+cx[3]*pixels[7].red)+cy[2]*(cx[0]*
    pixels[8].red+cx[1]*pixels[9].red+cx[2]*pixels[10].red+cx[3]*
    pixels[11].red)+cy[3]*(cx[0]*pixels[12].red+cx[1]*pixels[13].red+cx[2]*
    pixels[14].red+cx[3]*pixels[15].red));
  pixel->green=(cy[0]*(cx[0]*pixels[0].green+cx[1]*pixels[1].green+cx[2]*
    pixels[2].green+cx[3]*pixels[3].green)+cy[1]*(cx[0]*pixels[4].green+
    cx[1]*pixels[5].green+cx[2]*pixels[6].green+cx[3]*pixels[7].green)+
    cy[2]*(cx[0]*pixels[8].green+cx[1]*pixels[9].green+cx[2]*
    pixels[10].green+cx[3]*pixels[11].green)+cy[3]*(cx[0]*
    pixels[12].green+cx[1]*pixels[13].green+cx[2]*pixels[14].green+cx[3]*
    pixels[15].green));
  pixel->blue=(cy[0]*(cx[0]*pixels[0].blue+cx[1]*pixels[1].blue+cx[2]*
    pixels[2].blue+cx[3]*pixels[3].blue)+cy[1]*(cx[0]*pixels[4].blue+cx[1]*
    pixels[5].blue+cx[2]*pixels[6].blue+cx[3]*pixels[7].blue)+cy[2]*(cx[0]*
    pixels[8].blue+cx[1]*pixels[9].blue+cx[2]*pixels[10].blue+cx[3]*
    pixels[11].blue)+cy[3]*(cx[0]*pixels[12].blue+cx[1]*pixels[13].blue+
    cx[2]*pixels[14].blue+cx[3]*pixels[15].blue));
  if (grid->image->colorspace == CMYKColorspace)
    pixel->black=(cy[0]*(cx[0]*pixels[0].black+cx[1]*pixels[1].black+cx[2]*
      pixels[2].black+cx[3]*pixels[3].black)+cy[1]*(cx[0]*pixels[4].black+
      cx[1]*pixels[5].black+cx[2]*pixels[6].black+cx[3]*pixels[7].black)+
      cy[2]*(cx[0]*pixels[8].black+cx[1]*pixels[9].black+cx[2]*
      pixels[10].black+cx[3]*pixels[11].black)+cy[3]*(cx[0]*
      pixels[12].black+cx[1]*pixels[13].black+cx[2]*pixels[14].black+cx[3]*
      pixels[15].black));
  pixel->alpha=(cy[0]*(cx[0]*pixels[0].alpha+cx[1]*pixels[1].alpha+cx[2]*
    pixels[2].alpha+cx[3]*pixels[3].alpha)+cy[1]*(cx[0]*pixels[4].alpha+
    cx[1]*pixels[5].alpha+cx[2]*pixels[6].alpha+cx[3]*pixels[7].alpha)+
    cy[2]*(cx[0]*pixels[8].alpha+cx[1]*pixels[9].alpha+cx[2]*
    pixels[10].alpha+cx[3]*pixels[11].alpha)+cy[3]*(cx[0]*pixels[12].alpha+
    cx[1]*pixels[13].alpha+cx[2]*pixels[14].alpha+cx[3]*pixels[15].alpha));
  return(status);
}

static MagickBooleanType InterpolateBilinearPixelInfo(
  const PixelInfoInterpolation *grid,PixelInfo *pixels,PixelInfo *pixel,
  MagickBooleanType status)
{
  const Quantum
    *p;

  double
    alpha[16],
    gamma;

  ssize_t
    i;

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
  for (i=0; i < 4L; i++)
    AlphaBlendPixelInfo(grid->image,p+i*(ssize_t) GetPixelChannels(grid->image),
      pixels+i,alpha+i);
  delta.x=grid->x-grid->x_offset;
  delta.y=grid->y-grid->y_offset;
  epsilon.x=1.0-delta.x;
  epsilon.y=1.0-delta.y;
  gamma=((epsilon.y*(epsilon.x*alpha[0]+delta.x*alpha[1])+delta.y*
    (epsilon.x*alpha[2]+delta.x*alpha[3])));
  gamma=MagickSafeReciprocal(gamma);
  pixel->red=gamma*(epsilon.y*(epsilon.x*pixels[0].red+delta.x*
    pixels[1].red)+delta.y*(epsilon.x*pixels[2].red+delta.x*pixels[3].red));
  pixel->green=gamma*(epsilon.y*(epsilon.x*pixels[0].green+delta.x*
    pixels[1].green)+delta.y*(epsilon.x*pixels[2].green+delta.x*
    pixels[3].green));
  pixel->blue=gamma*(epsilon.y*(epsilon.x*pixels[0].blue+delta.x*
    pixels[1].blue)+delta.y*(epsilon.x*pixels[2].blue+delta.x*
    pixels[3].blue));
  if (grid->image->colorspace == CMYKColorspace)
    pixel->black=gamma*(epsilon.y*(epsilon.x*pixels[0].black+delta.x*
      pixels[1].black)+delta.y*(epsilon.x*pixels[2].black+delta.x*
      pixels[3].black));
  gamma=((epsilon.y*(epsilon.x+delta.x)+delta.y*(epsilon.x+delta.x)));
  gamma=MagickSafeReciprocal(gamma);
  pixel->alpha=gamma*(epsilon.y*(epsilon.x*pixels[0].alpha+delta.x*
    pixels[1].alpha)+delta.y*(epsilon.x*pixels[2].alpha+delta.x*
    pixels[3].alpha));
  return(status);
}

static MagickBooleanType InterpolateAveragePixelInfo(
  const PixelInfoInterpolation *grid,const PixelInterpolateMethod interpolate,
  PixelInfo *pixel,MagickBooleanType status)
{
  const Quantum
    *p;

  double
    alpha[16],
    gamma;

  PixelInfo
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
  pixel->red=0.0;
  pixel->green=0.0;
  pixel->blue=0.0;
  pixel->black=0.0;
  pixel->alpha=0.0;
  count*=count;  /* number of pixels - square of size */
  for (i=0; i < (ssize_t) count; i++)
  {
    AlphaBlendPixelInfo(grid->image,p,pixels,alpha);
    gamma=MagickSafeReciprocal(alpha[0]);
    pixel->red+=gamma*pixels[0].red;
    pixel->green+=gamma*pixels[0].green;
    pixel->blue+=gamma*pixels[0].blue;
    pixel->black+=gamma*pixels[0].black;
    pixel->alpha+=pixels[0].alpha;
    p+=(ptrdiff_t) GetPixelChannels(grid->image);
  }
  gamma=1.0/count;   /* average weighting of each pixel in area */
  pixel->red*=gamma;
  pixel->green*=gamma;
  pixel->blue*=gamma;
  pixel->black*=gamma;
  pixel->alpha*=gamma;
  return(status);
}

static void SetBlendedPixelInfo(double *alpha,const PixelInfo *pixels,
  double gamma,PixelInfo *pixel)
{
  gamma=1.0/gamma;
  alpha[0]=MagickSafeReciprocal(alpha[0]);
  pixel->red=alpha[0]*pixels[0].red;
  pixel->green=alpha[0]*pixels[0].green;  /* divide by sum of alpha */
  pixel->blue=alpha[0]*pixels[0].blue;
  pixel->black=alpha[0]*pixels[0].black;
  pixel->alpha=gamma*pixels[0].alpha;   /* divide by number of pixels */
}

static MagickBooleanType InterpolateBlendPixelInfo(
  const PixelInfoInterpolation *grid,PixelInfo *pixels,PixelInfo *pixel,
  MagickBooleanType status)
{
  const Quantum
    *p;

  double
    alpha[16],
    gamma;

  ssize_t
    i;

  p=GetCacheViewVirtualPixels(grid->image_view,grid->x_offset,grid->y_offset,
    2,2,grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  for (i=0; i < 4L; i++)
  {
    GetPixelInfoPixel(grid->image,p+i*(ssize_t) GetPixelChannels(grid->image),
      pixels+i);
    AlphaBlendPixelInfo(grid->image,p+i*(ssize_t) GetPixelChannels(grid->image),
      pixels+i,alpha+i);
  }
  gamma=1.0;  /* number of pixels blended together (its variable) */
  for (i=0; i <= 1L; i++)
  {
    if ((grid->y-grid->y_offset) >= 0.75)
      {
        alpha[i]=alpha[i+2];  /* take right pixels */
        pixels[i]=pixels[i+2];
      }
    else
      if ((grid->y-grid->y_offset) > 0.25)
        {
          gamma=2.0;  /* blend both pixels in row */
          alpha[i]+=alpha[i+2];  /* add up alpha weights */
          pixels[i].red+=pixels[i+2].red;
          pixels[i].green+=pixels[i+2].green;
          pixels[i].blue+=pixels[i+2].blue;
          pixels[i].black+=pixels[i+2].black;
          pixels[i].alpha+=pixels[i+2].alpha;
        }
  }
  if ((grid->x-grid->x_offset) >= 0.75)
    {
      alpha[0]=alpha[1];
      pixels[0]=pixels[1];
    }
  else
    if ((grid->x-grid->x_offset) > 0.25)
      {
        gamma*=2.0;  /* blend both rows */
        alpha[0]+= alpha[1];  /* add up alpha weights */
        pixels[0].red+=pixels[1].red;
        pixels[0].green+=pixels[1].green;
        pixels[0].blue+=pixels[1].blue;
        pixels[0].black+=pixels[1].black;
        pixels[0].alpha+=pixels[1].alpha;
      }
  SetBlendedPixelInfo(alpha,pixels,gamma,pixel);
  return(status);
}

static void InterpolateAreaPixelInfo(const PixelInfoInterpolation grid,
  const PixelInterpolateMethod interpolate,PixelInfo *pixel,
  MagickBooleanType *status)
{
  PixelInfo
    pixels[16];

  switch (interpolate)
  {
    case AverageInterpolatePixel:  /* nearest 4 neighbours */
    case Average9InterpolatePixel:  /* nearest 9 neighbours */
    case Average16InterpolatePixel:  /* nearest 16 neighbours */
    {
      (*status)=InterpolateAveragePixelInfo(&grid,interpolate,pixel,(*status));
      break;
    }
    case BilinearInterpolatePixel:
    default:
    {
      (*status)=InterpolateBilinearPixelInfo(&grid,pixels,pixel,(*status));
      break;
    }
    case BlendInterpolatePixel:
    {
      (*status)=InterpolateBlendPixelInfo(&grid,pixels,pixel,(*status));
      break;
    }
    case MeshInterpolatePixel:
    {
      (*status)=InterpolateMeshPixelInfo(&grid,pixels,pixel,(*status));
      break;
    }
  }
}

static void InterpolateGridPixelInfo(const PixelInfoInterpolation grid,
  const PixelInterpolateMethod interpolate,PixelInfo *pixel,
  MagickBooleanType *status)
{
  PixelInfo
    pixels[16];

  switch (interpolate)
  {
    default:
    {
      InterpolateAreaPixelInfo(grid,interpolate,pixel,status);
      break;
    }
    case CatromInterpolatePixel:
    {
      (*status)=InterpolateCubicPixelInfo(&grid,CatromWeights,pixels,pixel,
        (*status));
      break;
    }
    case SplineInterpolatePixel:
    {
      (*status)=InterpolateCubicPixelInfo(&grid,SplineWeights,pixels,pixel,
        (*status));
      break;
    }
  }
}

static MagickBooleanType InterpolateIntegerPixelInfo(
  const PixelInfoInterpolation *grid,PixelInfo *pixel,MagickBooleanType status)
{
  const Quantum
    *p;

  p=GetCacheViewVirtualPixels(grid->image_view,grid->x_offset,grid->y_offset,
    1,1,grid->exception);
  if (p == (const Quantum *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  GetPixelInfoPixel(grid->image,p,pixel);
  return(status);
}

#endif
