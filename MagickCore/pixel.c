/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      PPPP   IIIII  X   X  EEEEE  L                          %
%                      P   P    I     X X   E      L                          %
%                      PPPP     I      X    EEE    L                          %
%                      P        I     X X   E      L                          %
%                      P      IIIII  X   X  EEEEE  LLLLL                      %
%                                                                             %
%                  MagickCore Methods to Import/Export Pixels                 %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                               October 1998                                  %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/property.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/pixel-private.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/statistic.h"
#include "MagickCore/stream.h"
#include "MagickCore/string_.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"

/*
  Typedef declarations.
*/

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e P i x e l C h a n n e l M a p                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquirePixelChannelMap() acquires a pixel component map.
%
%  The format of the AcquirePixelChannelMap() method is:
%
%      PixelChannelMap *AcquirePixelChannelMap(void)
%
*/
MagickExport PixelChannelMap *AcquirePixelChannelMap(void)
{
  PixelChannelMap
    *channel_map;

  ssize_t
    i;

  channel_map=(PixelChannelMap *) AcquireQuantumMemory(MaxPixelChannels+1,
    sizeof(*channel_map));
  if (channel_map == (PixelChannelMap *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) memset(channel_map,0,(MaxPixelChannels+1)*sizeof(*channel_map));
  for (i=0; i <= MaxPixelChannels; i++)
    channel_map[i].channel=(PixelChannel) i;
  return(channel_map);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C l o n e P i x e l C h a n n e l M a p                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClonePixelChannelMap() clones a pixel component map.
%
%  The format of the ClonePixelChannelMap() method is:
%
%      PixelChannelMap *ClonePixelChannelMap(PixelChannelMap *channel_map)
%
%  A description of each parameter follows:
%
%    o channel_map: the pixel component map.
%
*/
MagickExport PixelChannelMap *ClonePixelChannelMap(PixelChannelMap *channel_map)
{
  PixelChannelMap
    *clone_map;

  assert(channel_map != (PixelChannelMap *) NULL);
  clone_map=AcquirePixelChannelMap();
  if (clone_map == (PixelChannelMap *) NULL)
    return((PixelChannelMap *) NULL);
  (void) memcpy(clone_map,channel_map,MaxPixelChannels*sizeof(*channel_map));
  return(clone_map);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C l o n e P i x e l I n f o                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClonePixelInfo() makes a duplicate of the given pixel info structure, or if
%  pixel info is NULL, a new one.
%
%  The format of the ClonePixelInfo method is:
%
%      PixelInfo *ClonePixelInfo(const PixelInfo *pixel)
%
%  A description of each parameter follows:
%
%    o pixel: the pixel info.
%
*/
MagickExport PixelInfo *ClonePixelInfo(const PixelInfo *pixel)
{
  PixelInfo
    *pixel_info;

  pixel_info=(PixelInfo *) AcquireMagickMemory(sizeof(*pixel_info));
  if (pixel_info == (PixelInfo *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  *pixel_info=(*pixel);
  return(pixel_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C o n f o r m P i x e l I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConformPixelInfo() ensures the pixel conforms with the colorspace and alpha
%  attribute of the image.
%
%  The format of the ConformPixelInfo method is:
%
%      void *ConformPixelInfo((Image *image,const PixelInfo *source,
%        PixelInfo *destination,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o source: the source pixel info.
%
%    o destination: the destination pixel info.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void ConformPixelColorspace(const Image *image,PixelInfo *destination)
{
  if (image->colorspace == CMYKColorspace)
    {
      if (IssRGBCompatibleColorspace(destination->colorspace) != MagickFalse)
        ConvertRGBToCMYK(destination);
    }
  else
    if ((destination->colorspace == CMYKColorspace) &&
        (IssRGBCompatibleColorspace(image->colorspace) != MagickFalse))
      ConvertCMYKToRGB(destination);
}

MagickExport void ConformPixelInfo(Image *image,const PixelInfo *source,
  PixelInfo *destination,ExceptionInfo *exception)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(destination != (const PixelInfo *) NULL);
  *destination=(*source);
  ConformPixelColorspace(image,destination);
  if ((IsPixelInfoGray(&image->background_color) == MagickFalse) &&
      (IsGrayColorspace(image->colorspace) != MagickFalse))
    (void) TransformImageColorspace(image,sRGBColorspace,exception);
  if ((destination->alpha_trait != UndefinedPixelTrait) &&
      ((image->alpha_trait & BlendPixelTrait) == 0))
    (void) SetImageAlpha(image,OpaqueAlpha,exception);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y P i x e l C h a n n e l M a p                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyPixelChannelMap() deallocates memory associated with the pixel
%  channel map.
%
%  The format of the DestroyPixelChannelMap() method is:
%
%      PixelChannelMap *DestroyPixelChannelMap(PixelChannelMap *channel_map)
%
%  A description of each parameter follows:
%
%    o channel_map: the pixel component map.
%
*/
MagickExport PixelChannelMap *DestroyPixelChannelMap(
  PixelChannelMap *channel_map)
{
  assert(channel_map != (PixelChannelMap *) NULL);
  channel_map=(PixelChannelMap *) RelinquishMagickMemory(channel_map);
  return((PixelChannelMap *) RelinquishMagickMemory(channel_map));
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t P i x e l I n f o                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelInfo() initializes the PixelInfo structure.
%
%  The format of the GetPixelInfo method is:
%
%      GetPixelInfo(const Image *image,PixelInfo *pixel)
%
%  A description of each parameter follows:
%
%    o image: the image. (optional - may be NULL)
%
%    o pixel: Specifies a pointer to a PixelInfo structure.
%
*/
MagickExport void GetPixelInfo(const Image *image,PixelInfo *pixel)
{
  (void) memset(pixel,0,sizeof(*pixel));
  pixel->storage_class=DirectClass;
  pixel->colorspace=sRGBColorspace;
  pixel->depth=MAGICKCORE_QUANTUM_DEPTH;
  pixel->alpha_trait=UndefinedPixelTrait;
  pixel->alpha=(double) OpaqueAlpha;
  if (image == (const Image *) NULL)
    return;
  pixel->storage_class=image->storage_class;
  pixel->colorspace=image->colorspace;
  pixel->alpha_trait=image->alpha_trait;
  pixel->depth=image->depth;
  pixel->fuzz=image->fuzz;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t P i x e l I n d o I n t e n s i t y                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelInfoIntensity() returns a single sample intensity value from the red,
%  green, and blue components of a pixel based on the selected method:
%
%    Rec601Luma       0.298839R' + 0.586811G' + 0.114350B'
%    Rec601Luminance  0.298839R + 0.586811G + 0.114350B
%    Rec709Luma       0.212656R' + 0.715158G' + 0.072186B'
%    Rec709Luminance  0.212656R + 0.715158G + 0.072186B
%    Brightness       max(R', G', B')
%    Lightness        (min(R', G', B') + max(R', G', B')) / 2.0
%
%    MS               (R^2 + G^2 + B^2) / 3.0
%    RMS              sqrt((R^2 + G^2 + B^2) / 3.0
%    Average          (R + G + B') / 3.0
%
%  The format of the GetPixelInfoIntensity method is:
%
%      MagickRealType GetPixelInfoIntensity(const Image *image,
%        const Quantum *pixel)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o pixel: Specifies a pointer to a Quantum structure.
%
*/
typedef MagickRealType
  (*PixelGammaHandler)(const MagickRealType);

static const double
  rec601_weights[3] = { 0.298839, 0.586811, 0.114350 },
  rec709_weights[3] = { 0.212656, 0.715158, 0.072186 };

static MagickRealType GammaWeightedIntensity(const MagickRealType *sample,
  const PixelGammaHandler gamma,const double *weight)
{
  MagickRealType
    blue,
    green,
    red;

  red=sample[0];
  green=sample[1];
  blue=sample[2];
  if (gamma != (PixelGammaHandler) NULL)
    {
      red=gamma(red);
      green=gamma(green);
      blue=gamma(blue);
    }
  return(weight[0]*red+weight[1]*green+weight[2]*blue);
}

static MagickRealType LumaIntensityFromSamples(
  const PixelIntensityMethod method,const PixelGammaHandler encode,
  const PixelGammaHandler decode,const MagickRealType *sample)
{
  switch (method)
  {
    case Rec601LumaPixelIntensityMethod:
      return(GammaWeightedIntensity(sample,encode,rec601_weights));
    case Rec601LuminancePixelIntensityMethod:
      return(GammaWeightedIntensity(sample,decode,rec601_weights));
    case Rec709LuminancePixelIntensityMethod:
      return(GammaWeightedIntensity(sample,decode,rec709_weights));
    default:
      break;
  }
  return(GammaWeightedIntensity(sample,encode,rec709_weights));
}

static MagickRealType IntensityFromSamples(const PixelIntensityMethod method,
  const PixelGammaHandler encode,const PixelGammaHandler decode,
  const MagickRealType *sample)
{
  MagickRealType
    blue,
    green,
    red;

  red=sample[0];
  green=sample[1];
  blue=sample[2];
  switch (method)
  {
    case AveragePixelIntensityMethod:
      return((red+green+blue)/3.0);
    case BrightnessPixelIntensityMethod:
      return(MagickMax(MagickMax(red,green),blue));
    case LightnessPixelIntensityMethod:
      return((MagickMin(MagickMin(red,green),blue)+
        MagickMax(MagickMax(red,green),blue))/2.0);
    case MSPixelIntensityMethod:
      return((MagickRealType) (((double) red*red+green*green+blue*blue)/
        (3.0*(double) QuantumRange)));
    case RMSPixelIntensityMethod:
      return((MagickRealType) (sqrt((double) red*red+green*green+blue*blue)/
        sqrt(3.0)));
    default:
      break;
  }
  return(LumaIntensityFromSamples(method,encode,decode,sample));
}

MagickExport MagickRealType GetPixelInfoIntensity(
  const Image *magick_restrict image,const PixelInfo *magick_restrict pixel)
{
  MagickRealType
    sample[3];

  PixelIntensityMethod
    method;

  method=Rec709LumaPixelIntensityMethod;
  if (image != (const Image *) NULL)
    method=image->intensity;
  sample[0]=pixel->red;
  sample[1]=pixel->green;
  sample[2]=pixel->blue;
  return(IntensityFromSamples(method,
    pixel->colorspace == RGBColorspace ? EncodePixelGamma : NULL,
    pixel->colorspace == sRGBColorspace ? DecodePixelGamma : NULL,sample));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t P i x e l I n t e n s i t y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelIntensity() returns a single sample intensity value from the red,
%  green, and blue components of a pixel based on the selected method:
%
%    Rec601Luma       0.298839R' + 0.586811G' + 0.114350B'
%    Rec601Luminance  0.298839R + 0.586811G + 0.114350B
%    Rec709Luma       0.212656R' + 0.715158G' + 0.072186B'
%    Rec709Luminance  0.212656R + 0.715158G + 0.072186B
%    Brightness       max(R', G', B')
%    Lightness        (min(R', G', B') + max(R', G', B')) / 2.0
%
%    MS               (R^2 + G^2 + B^2) / 3.0
%    RMS              sqrt((R^2 + G^2 + B^2) / 3.0
%    Average          (R + G + B') / 3.0
%
%  The format of the GetPixelIntensity method is:
%
%      MagickRealType GetPixelIntensity(const Image *image,
%        const Quantum *pixel)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o pixel: Specifies a pointer to a Quantum structure.
%
*/
MagickExport MagickRealType GetPixelIntensity(
  const Image *magick_restrict image,const Quantum *magick_restrict pixel)
{
  MagickRealType
    sample[3];

  sample[0]=(MagickRealType) GetPixelRed(image,pixel);
  if (image->number_channels == 1)
    return(sample[0]);
  sample[1]=(MagickRealType) GetPixelGreen(image,pixel);
  sample[2]=(MagickRealType) GetPixelBlue(image,pixel);
  return(IntensityFromSamples(image->intensity,
    (image->colorspace == RGBColorspace) ||
    (image->colorspace == LinearGRAYColorspace) ? EncodePixelGamma : NULL,
    (image->colorspace == sRGBColorspace) ||
    (image->colorspace == GRAYColorspace) ? DecodePixelGamma : NULL,sample));
}

static void LoadInterpolatePixels(const Image *magick_restrict image,
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
    for (i=0; i < (ssize_t) count; i++)
    {
      alpha[i]=QuantumScale*(double) GetPixelAlpha(image,p+i*
        (ssize_t) GetPixelChannels(image));
      pixels[i]=alpha[i]*(double) p[i*(ssize_t) GetPixelChannels(image)+
        GetPixelChannelOffset(image,channel)];
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n t e r p o l a t e P i x e l C h a n n e l                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InterpolatePixelChannel() applies a pixel interpolation method between a
%  floating point coordinate and the pixels surrounding that coordinate.  No
%  pixel area resampling, or scaling of the result is performed.
%
%  Interpolation is restricted to just the specified channel.
%
%  The format of the InterpolatePixelChannel method is:
%
%      MagickBooleanType InterpolatePixelChannel(
%        const Image *magick_restrict image,const CacheView *image_view,
%        const PixelChannel channel,const PixelInterpolateMethod method,
%        const double x,const double y,double *pixel,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o image_view: the image view.
%
%    o channel: the pixel channel to interpolate.
%
%    o method: the pixel color interpolation method.
%
%    o x,y: A double representing the current (x,y) position of the pixel.
%
%    o pixel: return the interpolated pixel here.
%
%    o exception: return any errors or warnings in this structure.
%
*/

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

static void InterpolateGridPixelChannel(const ChannelInterpolation grid,
  const PixelInterpolateMethod interpolate,double *pixel,
  MagickBooleanType *status)
{
  switch (interpolate)
  {
    case BilinearInterpolatePixel:
    default:
    {
      (*status)=InterpolateBilinearPixelChannel(&grid,pixel,(*status));
      break;
    }
    case BlendInterpolatePixel:
    {
      (*status)=InterpolateBlendPixelChannel(&grid,pixel,(*status));
      break;
    }
    case CatromInterpolatePixel:
    {
      (*status)=InterpolateCubicPixelChannel(&grid,CatromWeights,pixel,
        (*status));
      break;
    }
    case MeshInterpolatePixel:
    {
      (*status)=InterpolateMeshPixelChannel(&grid,pixel,(*status));
      break;
    }
    case SplineInterpolatePixel:
    {
      (*status)=InterpolateCubicPixelChannel(&grid,SplineWeights,pixel,
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

static void InterpolatePointPixelChannel(const ChannelInterpolation grid,
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
      (*status)=InterpolateIntegerPixelChannel(&grid,pixel,(*status));
      break;
    }
    case NearestInterpolatePixel:
    {
      (*status)=InterpolateNearestPixelChannel(&grid,pixel,(*status));
      break;
    }
  }
}

MagickExport MagickBooleanType InterpolatePixelChannel(
  const Image *magick_restrict image,const CacheView_ *image_view,
  const PixelChannel channel,const PixelInterpolateMethod method,const double x,
  const double y,double *pixel,ExceptionInfo *exception)
{
  ChannelInterpolation
    grid;

  MagickBooleanType
    status;

  PixelInterpolateMethod
    interpolate;

  PixelTrait
    traits;

  ssize_t
    x_offset,
    y_offset;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image_view != (CacheView *) NULL);
  *pixel=0.0;
  if ((channel < 0) || (channel >= MaxPixelChannels))
    ThrowBinaryException(OptionError,"NoSuchImageChannel",image->filename);
  traits=GetPixelChannelTraits(image,channel);
  x_offset=CastDoubleToSsizeT(floor(x));
  y_offset=CastDoubleToSsizeT(floor(y));
  interpolate=method;
  if (interpolate == UndefinedInterpolatePixel)
    interpolate=image->interpolate;
  status=MagickTrue;
  grid.image=image;
  grid.image_view=image_view;
  grid.channel=channel;
  grid.traits=traits;
  grid.x=x;
  grid.y=y;
  grid.x_offset=x_offset;
  grid.y_offset=y_offset;
  grid.exception=exception;
  switch (interpolate)
  {
    case AverageInterpolatePixel:  /* nearest 4 neighbours */
    case Average9InterpolatePixel:  /* nearest 9 neighbours */
    case Average16InterpolatePixel:  /* nearest 16 neighbours */
    {
      status=InterpolateAveragePixelChannel(&grid,interpolate,pixel,status);
      break;
    }
    default:
    {
      InterpolatePointPixelChannel(grid,interpolate,pixel,&status);
      break;
    }
  }
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n t e r p o l a t e P i x e l C h a n n e l s                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InterpolatePixelChannels() applies a pixel interpolation method between a
%  floating point coordinate and the pixels surrounding that coordinate.  No
%  pixel area resampling, or scaling of the result is performed.
%
%  Interpolation is restricted to just the current channel setting of the
%  destination image into which the color is to be stored
%
%  The format of the InterpolatePixelChannels method is:
%
%      MagickBooleanType InterpolatePixelChannels(
%        const Image *magick_restrict source,const CacheView *source_view,
%        const Image *magick_restrict destination,
%        const PixelInterpolateMethod method,const double x,const double y,
%        Quantum *pixel,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o source: the source.
%
%    o source_view: the source view.
%
%    o destination: the destination image, for the interpolated color
%
%    o method: the pixel color interpolation method.
%
%    o x,y: A double representing the current (x,y) position of the pixel.
%
%    o pixel: return the interpolated pixel here.
%
%    o exception: return any errors or warnings in this structure.
%
*/
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
    for (j=0; j < 4; j++)
    {
      alpha[j]=QuantumScale*(double) GetPixelAlpha(grid->source,p+j*
        (ssize_t) GetPixelChannels(grid->source));
      pixels[j]=(double) p[j*(ssize_t) GetPixelChannels(grid->source)+i];
      if (channel != AlphaPixelChannel)
        pixels[j]*=alpha[j];
    }
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

MagickExport MagickBooleanType InterpolatePixelChannels(
  const Image *magick_restrict source,const CacheView_ *source_view,
  const Image *magick_restrict destination,const PixelInterpolateMethod method,
  const double x,const double y,Quantum *pixel,ExceptionInfo *exception)
{
  ChannelsInterpolation
    grid;

  MagickBooleanType
    status;

  PixelInterpolateMethod
    interpolate;

  assert(source != (Image *) NULL);
  assert(source->signature == MagickCoreSignature);
  assert(source_view != (CacheView *) NULL);
  status=MagickTrue;
  interpolate=method;
  if (interpolate == UndefinedInterpolatePixel)
    interpolate=source->interpolate;
  grid.source=source;
  grid.source_view=source_view;
  grid.destination=destination;
  grid.x=x;
  grid.y=y;
  grid.x_offset=CastDoubleToSsizeT(floor(x));
  grid.y_offset=CastDoubleToSsizeT(floor(y));
  grid.exception=exception;
  switch (interpolate)
  {
    case AverageInterpolatePixel:  /* nearest 4 neighbours */
    case Average9InterpolatePixel:  /* nearest 9 neighbours */
    case Average16InterpolatePixel:  /* nearest 16 neighbours */
    {
      status=InterpolateAverageChannels(&grid,interpolate,pixel,status);
      break;
    }
    default:
    {
      InterpolateKernelChannels(grid,interpolate,pixel,&status);
      break;
    }
    case BlendInterpolatePixel:
    {
      status=InterpolateQuadChannels(&grid,InterpolateBlendChannel,pixel,
        status);
      break;
    }
    case NearestInterpolatePixel:
    {
      grid.x_offset=CastDoubleToSsizeT(floor(x+0.5));
      grid.y_offset=CastDoubleToSsizeT(floor(y+0.5));
      status=InterpolateIntegerChannels(&grid,pixel,status);
      break;
    }
  }
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n t e r p o l a t e P i x e l I n f o                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InterpolatePixelInfo() applies a pixel interpolation method between a
%  floating point coordinate and the pixels surrounding that coordinate.  No
%  pixel area resampling, or scaling of the result is performed.
%
%  Interpolation is restricted to just RGBKA channels.
%
%  The format of the InterpolatePixelInfo method is:
%
%      MagickBooleanType InterpolatePixelInfo(const Image *image,
%        const CacheView *image_view,const PixelInterpolateMethod method,
%        const double x,const double y,PixelInfo *pixel,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o image_view: the image view.
%
%    o method: the pixel color interpolation method.
%
%    o x,y: A double representing the current (x,y) position of the pixel.
%
%    o pixel: return the interpolated pixel here.
%
%    o exception: return any errors or warnings in this structure.
%
*/

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

MagickExport MagickBooleanType InterpolatePixelInfo(const Image *image,
  const CacheView_ *image_view,const PixelInterpolateMethod method,
  const double x,const double y,PixelInfo *pixel,ExceptionInfo *exception)
{
  PixelInfoInterpolation
    grid;

  const Quantum
    *p;

  MagickBooleanType
    status;

  PixelInfo
    pixels[16];

  PixelInterpolateMethod
    interpolate;

  ssize_t
    x_offset,
    y_offset;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image_view != (CacheView *) NULL);
  status=MagickTrue;
  x_offset=CastDoubleToSsizeT(floor(x));
  y_offset=CastDoubleToSsizeT(floor(y));
  interpolate=method;
  if (interpolate == UndefinedInterpolatePixel)
    interpolate=image->interpolate;
  GetPixelInfoPixel(image,(const Quantum *) NULL,pixel);
  (void) memset(&pixels,0,sizeof(pixels));
  grid.image=image;
  grid.image_view=image_view;
  grid.x=x;
  grid.y=y;
  grid.x_offset=x_offset;
  grid.y_offset=y_offset;
  grid.exception=exception;
  switch (interpolate)
  {
    case BackgroundInterpolatePixel:
    {
      *pixel=image->background_color;  /* Copy PixelInfo Structure  */
      break;
    }
    default:
    {
      InterpolateGridPixelInfo(grid,interpolate,pixel,&status);
      break;
    }
    case IntegerInterpolatePixel:
    {
      status=InterpolateIntegerPixelInfo(&grid,pixel,status);
      break;
    }
    case NearestInterpolatePixel:
    {
      x_offset=CastDoubleToSsizeT(floor(x+0.5));
      y_offset=CastDoubleToSsizeT(floor(y+0.5));
      p=GetCacheViewVirtualPixels(image_view,x_offset,y_offset,1,1,exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      GetPixelInfoPixel(image,p,pixel);
      break;
    }
  }
  return(status);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e s e t P i x e l C h a n n e l M a p                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetPixelChannelMap() defines the standard pixel component map.
%
%  The format of the ResetPixelChannelMap() method is:
%
%      MagickBooleanType ResetPixelChannelMap(Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static ssize_t SetColorChannelAttributes(const Image *image,
  const PixelTrait trait,ssize_t n)
{
  if ((image->colorspace == LinearGRAYColorspace) ||
      (image->colorspace == GRAYColorspace))
    {
      SetPixelChannelAttributes(image,BluePixelChannel,trait,n);
      SetPixelChannelAttributes(image,GreenPixelChannel,trait,n);
      SetPixelChannelAttributes(image,RedPixelChannel,trait,n++);
    }
  else
    {
      SetPixelChannelAttributes(image,RedPixelChannel,trait,n++);
      SetPixelChannelAttributes(image,GreenPixelChannel,trait,n++);
      SetPixelChannelAttributes(image,BluePixelChannel,trait,n++);
    }
  if (image->colorspace == CMYKColorspace)
    SetPixelChannelAttributes(image,BlackPixelChannel,trait,n++);
  return(n);
}

static ssize_t SetExtraChannelAttributes(const Image *image,ssize_t n)
{
  if (image->alpha_trait != UndefinedPixelTrait)
    SetPixelChannelAttributes(image,AlphaPixelChannel,CopyPixelTrait,n++);
  if (image->storage_class == PseudoClass)
    SetPixelChannelAttributes(image,IndexPixelChannel,CopyPixelTrait,n++);
  if ((image->channels & ReadMaskChannel) != 0)
    SetPixelChannelAttributes(image,ReadMaskPixelChannel,CopyPixelTrait,n++);
  if ((image->channels & WriteMaskChannel) != 0)
    SetPixelChannelAttributes(image,WriteMaskPixelChannel,CopyPixelTrait,n++);
  if ((image->channels & CompositeMaskChannel) != 0)
    SetPixelChannelAttributes(image,CompositeMaskPixelChannel,CopyPixelTrait,
      n++);
  return(n);
}

static ssize_t SetMetaChannelAttributes(const Image *image,ssize_t n)
{
  PixelChannel
    meta_channel;

  ssize_t
    i;

  meta_channel=MetaPixelChannels;
  for (i=0; i < (ssize_t) image->number_meta_channels; i++)
  {
    SetPixelChannelAttributes(image,meta_channel,UpdatePixelTrait,n);
    meta_channel=(PixelChannel) (meta_channel+1);
    n++;
  }
  return(n);
}

static PixelTrait GetChannelMapTrait(const Image *image)
{
  PixelTrait
    trait;

  trait=UpdatePixelTrait;
  if (image->alpha_trait != UndefinedPixelTrait)
    trait=(PixelTrait) (trait | BlendPixelTrait);
  return(trait);
}

MagickPrivate MagickBooleanType ResetPixelChannelMap(Image *image,
  ExceptionInfo *exception)
{
  PixelTrait
    trait;

  ssize_t
    n;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  (void) memset(image->channel_map,0,MaxPixelChannels*
    sizeof(*image->channel_map));
  trait=GetChannelMapTrait(image);
  n=0;
  n=SetColorChannelAttributes(image,trait,n);
  n=SetExtraChannelAttributes(image,n);
  if (image->number_meta_channels != 0)
    {
      if (image->number_meta_channels >= (size_t) (MaxPixelChannels-MetaPixelChannels))
        {
          image->number_channels=(size_t) n;
          image->number_meta_channels=0;
          (void) SetPixelChannelMask(image,image->channel_mask);
          ThrowBinaryException(CorruptImageError,"MaximumChannelsExceeded",
            image->filename);
        }
      n=SetMetaChannelAttributes(image,n);
    }
  image->number_channels=(size_t) n;
  (void) SetPixelChannelMask(image,image->channel_mask);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t P i x e l C h a n n e l M a s k                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetPixelChannelMask() sets the pixel channel map from the specified channel
%  mask.
%
%  The format of the SetPixelChannelMask method is:
%
%      ChannelType SetPixelChannelMask(Image *image,
%        const ChannelType channel_mask)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o channel_mask: the channel mask.
%
*/

static const char *GetRedChannelLogName(const Image *image)
{
  if ((image->colorspace == LinearGRAYColorspace) ||
      (image->colorspace == GRAYColorspace))
    return("gray");
  if (image->colorspace == CMYKColorspace)
    return("cyan");
  return("red");
}

static const char *GetColorChannelLogName(const Image *image,
  const PixelChannel channel)
{
  switch (channel)
  {
    case RedPixelChannel:
      return(GetRedChannelLogName(image));
    case GreenPixelChannel:
    {
      if (image->colorspace == CMYKColorspace)
        return("magenta");
      return("green");
    }
    case BluePixelChannel:
    {
      if (image->colorspace == CMYKColorspace)
        return("yellow");
      return("blue");
    }
    default:
      break;
  }
  return((const char *) NULL);
}

static const char *GetSpecialChannelLogName(const PixelChannel channel)
{
  switch (channel)
  {
    case ReadMaskPixelChannel:
      return("read-mask");
    case WriteMaskPixelChannel:
      return("write-mask");
    case CompositeMaskPixelChannel:
      return("composite-mask");
    case MetaPixelChannels:
      return("meta");
    default:
      break;
  }
  return((const char *) NULL);
}

static const char *GetPixelChannelLogName(const Image *image,
  const PixelChannel channel)
{
  const char
    *name;

  name=GetColorChannelLogName(image,channel);
  if (name != (const char *) NULL)
    return(name);
  name=GetSpecialChannelLogName(channel);
  if (name != (const char *) NULL)
    return(name);
  switch (channel)
  {
    case BlackPixelChannel:
    {
      if (image->storage_class == PseudoClass)
        return("index");
      return("black");
    }
    case IndexPixelChannel:
      return("index");
    case AlphaPixelChannel:
      return("alpha");
    default:
      break;
  }
  return("undefined");
}

static void FormatPixelChannelTraits(const Image *image,
  const PixelChannel channel,char *traits)
{
  *traits='\0';
  if ((GetPixelChannelTraits(image,channel) & UpdatePixelTrait) != 0)
    (void) ConcatenateMagickString(traits,"update,",MagickPathExtent);
  if ((GetPixelChannelTraits(image,channel) & BlendPixelTrait) != 0)
    (void) ConcatenateMagickString(traits,"blend,",MagickPathExtent);
  if ((GetPixelChannelTraits(image,channel) & CopyPixelTrait) != 0)
    (void) ConcatenateMagickString(traits,"copy,",MagickPathExtent);
  if (*traits == '\0')
    (void) ConcatenateMagickString(traits,"undefined,",MagickPathExtent);
  traits[strlen(traits)-1]='\0';
}

static const char *GetPixelChannelLogLabel(const Image *image,
  const PixelChannel channel,char *channel_name)
{
  const char
    *name;

  name=GetPixelChannelLogName(image,channel);
  if (image->colorspace ==  UndefinedColorspace)
    {
      (void) FormatLocaleString(channel_name,MagickPathExtent,"%.17g",
        (double) channel);
      name=(const char *) channel_name;
    }
  return(name);
}

static void LogPixelChannels(const Image *image)
{
  ssize_t
    i;

  (void) LogMagickEvent(PixelEvent,GetMagickModule(),"%s[0x%08llx]",
    image->filename,(MagickOffsetType) image->channel_mask);
  for (i=0; i < (ssize_t) image->number_channels; i++)
  {
    char
      channel_name[MagickPathExtent],
      traits[MagickPathExtent];

    const char
      *name;

    PixelChannel
      channel;

    channel=GetPixelChannelChannel(image,i);
    name=GetPixelChannelLogLabel(image,channel,channel_name);
    FormatPixelChannelTraits(image,channel,traits);
    (void) LogMagickEvent(PixelEvent,GetMagickModule(),"  %.17g: %s (%s)",
      (double) i,name,traits);
  }
}

static void SetExtraChannelMaskTraits(Image *image)
{
  if (image->storage_class == PseudoClass)
    SetPixelChannelTraits(image,IndexPixelChannel,CopyPixelTrait);
  if ((image->channels & ReadMaskChannel) != 0)
    SetPixelChannelTraits(image,ReadMaskPixelChannel,CopyPixelTrait);
  if ((image->channels & WriteMaskChannel) != 0)
    SetPixelChannelTraits(image,WriteMaskPixelChannel,CopyPixelTrait);
  if ((image->channels & CompositeMaskChannel) != 0)
    SetPixelChannelTraits(image,CompositeMaskPixelChannel,CopyPixelTrait);
}

static void SetPixelChannelMaskTrait(Image *image,
  const ChannelType channel_mask,const ssize_t i)
{
#define GetChannelBit(mask,bit)  (((size_t) (mask) >> (size_t) (bit)) & 0x01)

  PixelChannel channel = GetPixelChannelChannel(image,i);
  if (GetChannelBit(channel_mask,channel) == 0)
    {
      SetPixelChannelTraits(image,channel,CopyPixelTrait);
      return;
    }
  if (channel == AlphaPixelChannel)
    {
      if ((image->alpha_trait & CopyPixelTrait) != 0)
        {
          SetPixelChannelTraits(image,channel,CopyPixelTrait);
          return;
        }
      SetPixelChannelTraits(image,channel,UpdatePixelTrait);
      return;
    }
  if (image->alpha_trait != UndefinedPixelTrait)
    {
      SetPixelChannelTraits(image,channel,(PixelTrait) (UpdatePixelTrait |
        BlendPixelTrait));
      return;
    }
  SetPixelChannelTraits(image,channel,UpdatePixelTrait);
}

MagickExport ChannelType SetPixelChannelMask(Image *image,
  const ChannelType channel_mask)
{
  ChannelType
    mask;

  ssize_t
    i;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(PixelEvent,GetMagickModule(),"%s[0x%08llx]",
      image->filename,(MagickOffsetType) channel_mask);
  mask=image->channel_mask;
  image->channel_mask=channel_mask;
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    SetPixelChannelMaskTrait(image,channel_mask,i);
  SetExtraChannelMaskTraits(image);
  if ((GetLogEventMask() & PixelEvent) != 0)
    LogPixelChannels(image);
  return(mask);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t P i x e l M e t a C h a n n e l s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetPixelMetaChannels() sets the image meta channels.
%
%  The format of the SetPixelMetaChannels method is:
%
%      MagickBooleanType SetPixelMetaChannels(Image *image,
%        const size_t number_meta_channels,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o number_meta_channels:  the number of meta channels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SetPixelMetaChannels(Image *image,
  const size_t number_meta_channels,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  if (number_meta_channels >= (size_t) (MaxPixelChannels-MetaPixelChannels))
   ThrowBinaryException(CorruptImageError,"MaximumChannelsExceeded",
     image->filename);
  image->number_meta_channels=number_meta_channels;
  status=ResetPixelChannelMap(image,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  return(SyncImagePixelCache(image,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S o r t I m a g e P i x e l s                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SortImagePixels() sorts pixels within each scanline in ascending order of
%  intensity.
%
%  The format of the SortImagePixels method is:
%
%      MagickBooleanType SortImagePixels(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SortRowPixels(const Image *image,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns-1; x++)
  {
    MagickRealType
      current,
      previous;

    ssize_t
      j;

    previous=GetPixelIntensity(image,q);
    for (j=0; j < ((ssize_t) image->columns-x-1); j++)
    {
      current=GetPixelIntensity(image,q+(j+1)*(ssize_t)
        GetPixelChannels(image));
      if (previous > current)
        {
          Quantum
            pixel[MaxPixelChannels];

          /*
            Swap adjacent pixels.
          */
          (void) memcpy(pixel,q+j*(ssize_t) GetPixelChannels(image),
            GetPixelChannels(image)*sizeof(Quantum));
          (void) memcpy(q+j*(ssize_t) GetPixelChannels(image),q+(j+1)*
            (ssize_t) GetPixelChannels(image),GetPixelChannels(image)*
            sizeof(Quantum));
          (void) memcpy(q+(j+1)*(ssize_t) GetPixelChannels(image),pixel,
            GetPixelChannels(image)*sizeof(Quantum));
        }
      else
        previous=current;
    }
  }
}

typedef struct _SortPixelsRow
{
  const Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;
} SortPixelsRow;

static MagickBooleanType SortPixelsLine(const SortPixelsRow *row,
  const ssize_t y,MagickBooleanType *status)
{
  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(row->image_view,0,y,row->image->columns,1,
    row->exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  SortRowPixels(row->image,q);
  if (SyncCacheViewAuthenticPixels(row->image_view,row->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

MagickExport MagickBooleanType SortImagePixels(Image *image,
  ExceptionInfo *exception)
{
#define SolarizeImageTag  "Solarize/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Sort image pixels.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const SortPixelsRow
      row = { image, image_view, exception };

    if (SortPixelsLine(&row,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,SolarizeImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}
