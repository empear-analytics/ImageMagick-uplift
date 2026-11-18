/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%         CCCC   OOO   M   M  PPPP    OOO   SSSSS  IIIII  TTTTT  EEEEE        %
%        C      O   O  MM MM  P   P  O   O  SS       I      T    E            %
%        C      O   O  M M M  PPPP   O   O   SSS     I      T    EEE          %
%        C      O   O  M   M  P      O   O     SS    I      T    E            %
%         CCCC   OOO   M   M  P       OOO   SSSSS  IIIII    T    EEEEE        %
%                                                                             %
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%                                                                             %
%                     MagickCore Image Composite Methods                      %
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
#include "MagickCore/artifact.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/morphology.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/resample.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/composite-row-private.h"
#include "MagickCore/composite-channel-private.h"
#include "MagickCore/composite-image-blend-private.h"
#include "MagickCore/composite-image-over-private.h"
#include "MagickCore/composite-image-virtual-private.h"

static void SetCompositeCoverageModulus(const CompositeRowInputs *row,
  const MagickRealType Sa,const MagickRealType Da,MagickRealType *alpha)
{
  const CompositeOperator
    compose = row->compose;

  switch (compose)
  {
    case DstOutCompositeOp:
    {
      (*alpha)=Da*(1.0-Sa);
      break;
    }
    case XorCompositeOp:
    {
      (*alpha)=Sa+Da-2.0*Sa*Da;
      break;
    }
    case ModulusAddCompositeOp:
    {
      if ((Sa+Da) <= 1.0)
        {
          (*alpha)=(Sa+Da);
          break;
        }
      (*alpha)=((Sa+Da)-1.0);
      break;
    }
    case ModulusSubtractCompositeOp:
    {
      if ((Sa-Da) >= 0.0)
        {
          (*alpha)=(Sa-Da);
          break;
        }
      (*alpha)=((Sa-Da)+1.0);
      break;
    }
    default:
    {
      (*alpha)=1.0;
      break;
    }
  }
}

static void SetCompositeCoverageProduct(const CompositeRowInputs *row,
  const MagickRealType Sa,const MagickRealType Da,MagickRealType *alpha)
{
  const CompositeOperator
    compose = row->compose;

  switch (compose)
  {
    case DstAtopCompositeOp:
    case DstInCompositeOp:
    case InCompositeOp:
    case SrcInCompositeOp:
    {
      (*alpha)=Sa*Da;
      break;
    }
    case OutCompositeOp:
    case SrcOutCompositeOp:
    {
      (*alpha)=Sa*(1.0-Da);
      break;
    }
    default:
    {
      SetCompositeCoverageModulus(row,Sa,Da,alpha);
      break;
    }
  }
}

static void SetCompositeCoverageDissolve(const CompositeRowInputs *row,
  const MagickRealType Sa,const MagickRealType Da,MagickRealType *alpha)
{
  const CompositeOperator
    compose = row->compose;

  const MagickRealType
    canvas_dissolve = row->canvas_dissolve,
    source_dissolve = row->source_dissolve;

  switch (compose)
  {
    case DissolveCompositeOp:
    {
      (*alpha)=source_dissolve*Sa*(-canvas_dissolve*Da)+source_dissolve*Sa+
        canvas_dissolve*Da;
      break;
    }
    case DstOverCompositeOp:
    case OverCompositeOp:
    case SrcOverCompositeOp:
    {
      (*alpha)=Sa+Da-Sa*Da;
      break;
    }
    case BlendCompositeOp:
    case PlusCompositeOp:
    {
      (*alpha)=RoundToUnity(source_dissolve*Sa+canvas_dissolve*Da);
      break;
    }
    default:
    {
      SetCompositeCoverageProduct(row,Sa,Da,alpha);
      break;
    }
  }
}

static const CompositeOperator
  UnionCoverageOperators[] =
  {
    BumpmapCompositeOp,
    ColorBurnCompositeOp,
    ColorDodgeCompositeOp,
    DarkenCompositeOp,
    DifferenceCompositeOp,
    DivideCompositeOp,
    DivideDstCompositeOp,
    DivideSrcCompositeOp,
    ExclusionCompositeOp,
    FreezeCompositeOp,
    HardLightCompositeOp,
    HardMixCompositeOp,
    InterpolateCompositeOp,
    LightenCompositeOp,
    LinearBurnCompositeOp,
    LinearDodgeCompositeOp,
    LinearLightCompositeOp,
    MathematicsCompositeOp,
    MinusDstCompositeOp,
    MinusSrcCompositeOp,
    MultiplyCompositeOp,
    NegateCompositeOp,
    OverlayCompositeOp,
    PegtopLightCompositeOp,
    PinLightCompositeOp,
    ReflectCompositeOp,
    ScreenCompositeOp,
    SoftBurnCompositeOp,
    SoftDodgeCompositeOp,
    SoftLightCompositeOp,
    StampCompositeOp,
    VividLightCompositeOp
  };

static MagickBooleanType IsListedOperator(const CompositeOperator *list,
  const size_t count,const CompositeOperator compose)
{
  size_t
    i;

  for (i=0; i < count; i++)
    if (list[i] == compose)
      return(MagickTrue);
  return(MagickFalse);
}

static void SetCompositeCoverage(const CompositeRowInputs *row,
  const MagickRealType Sa,const MagickRealType Da,MagickRealType *alpha)
{
  if (row->union_coverage != MagickFalse)
    {
      (*alpha)=RoundToUnity(Sa+Da-Sa*Da);
      return;
    }
  SetCompositeCoverageDissolve(row,Sa,Da,alpha);
}

static const CompositeOperator
  CanvasAlphaOperators[] =
  {
    AtopCompositeOp,
    CopyBlackCompositeOp,
    CopyBlueCompositeOp,
    CopyCyanCompositeOp,
    CopyGreenCompositeOp,
    CopyMagentaCompositeOp,
    CopyRedCompositeOp,
    CopyYellowCompositeOp,
    SrcAtopCompositeOp,
    DstCompositeOp,
    NoCompositeOp
  };

static const CompositeOperator
  SourceAlphaOperators[] =
  {
    BlurCompositeOp,
    CopyCompositeOp,
    DisplaceCompositeOp,
    DistortCompositeOp,
    DstAtopCompositeOp,
    ReplaceCompositeOp,
    SrcCompositeOp
  };

static void CompositeAlphaMultiplyRun(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,MagickRealType *pixel)
{
  const CompositeOperator
    compose = row->compose;

  const MagickBooleanType
    compose_sync = row->compose_sync;

  const MagickRealType
    alpha = authentic->alpha,
    Da = authentic->Da,
    Sa = authentic->Sa;

  switch (compose)
  {
    case MultiplyCompositeOp:
    {
      MultiplyAlphaInputs
        multiply;

      multiply.compose_sync=compose_sync;
      multiply.alpha=alpha;
      multiply.Sa=Sa;
      multiply.Da=Da;
      (*pixel)=GetMultiplyAlpha(&multiply,(*pixel));
      break;
    }
    case NegateCompositeOp:
    {
      (*pixel)=(double) QuantumRange*((1.0-Sa-Da));
      break;
    }
    case ReflectCompositeOp:
    {
      (*pixel)=GetReflectAlpha(Sa,Da);
      break;
    }
    case StampCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(Sa+Da*Da-1.0);
      break;
    }
    case StereoCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(Sa+Da)/2;
      break;
    }
    default:
    {
      (*pixel)=(double) QuantumRange*alpha;
      break;
    }
  }
}

static void CompositeAlphaDarkenRun(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,MagickRealType *pixel)
{
  const CompositeOperator
    compose = row->compose;

  const MagickBooleanType
    compose_sync = row->compose_sync;

  const MagickRealType
    Da = authentic->Da,
    Di = authentic->Di,
    Sa = authentic->Sa,
    Si = authentic->Si;

  switch (compose)
  {
    case DarkenIntensityCompositeOp:
    {
      IntensityAlphaInputs
        intensity;

      intensity.compose_sync=compose_sync;
      intensity.Sa=Sa;
      intensity.Da=Da;
      intensity.Si=Si;
      intensity.Di=Di;
      (*pixel)=GetDarkenIntensityAlpha(&intensity,(*pixel));
      break;
    }
    case DifferenceCompositeOp:
    {
      (*pixel)=(double) QuantumRange*fabs((double) (Sa-Da));
      break;
    }
    case FreezeCompositeOp:
    {
      (*pixel)=GetFreezeAlpha(Sa,Da);
      break;
    }
    case InterpolateCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(0.5-0.25*cos(MagickPI*Sa)-0.25*
        cos(MagickPI*Da));
      break;
    }
    case LightenIntensityCompositeOp:
    {
      IntensityAlphaInputs
        intensity;

      intensity.compose_sync=compose_sync;
      intensity.Sa=Sa;
      intensity.Da=Da;
      intensity.Si=Si;
      intensity.Di=Di;
      (*pixel)=GetLightenIntensityAlpha(&intensity,(*pixel));
      break;
    }
    case ModulateCompositeOp:
    {
      (*pixel)=(double) QuantumRange*Da;
      break;
    }
    default:
    {
      CompositeAlphaMultiplyRun(row,authentic,pixel);
      break;
    }
  }
}

static void CompositeAlphaColorizeRun(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,MagickRealType *pixel)
{
  Image
    *const source_image = row->source_image;

  const CompositeOperator
    compose = row->compose;

  const MagickRealType
    Da = authentic->Da,
    Sa = authentic->Sa,
    Si = authentic->Si;

  switch (compose)
  {
    case ColorizeCompositeOp:
    case HueCompositeOp:
    case LuminizeCompositeOp:
    case RMSECompositeOp:
    case SaturateCompositeOp:
    {
      (*pixel)=GetHueAlpha(Sa,Da,(*pixel));
      break;
    }
    case CopyAlphaCompositeOp:
    {
      (*pixel)=GetCopyAlpha(source_image,Si,Sa);
      break;
    }
    default:
    {
      CompositeAlphaDarkenRun(row,authentic,pixel);
      break;
    }
  }
}

static void CompositeAlphaChannel(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,Quantum *magick_restrict q,
  MagickRealType *pixel)
{
  Image
    *const image = row->image,
    *const source_image = row->source_image;

  const CompositeOperator
    compose = row->compose;

  const Quantum
    *const p = authentic->p;

  const MagickRealType
    Da = authentic->Da,
    Sa = authentic->Sa,
    Si = authentic->Si;

  if (row->canvas_alpha != MagickFalse)
    {
      (*pixel)=(double) QuantumRange*Da;
      return;
    }
  if (row->source_alpha != MagickFalse)
    {
      (*pixel)=(double) QuantumRange*Sa;
      return;
    }
  switch (compose)
  {
    case AlphaCompositeOp:
    {
      (*pixel)=(double) QuantumRange*Sa;
      break;
    }
    case BumpmapCompositeOp:
    {
      (*pixel)=Si*Da;
      break;
    }
    case ChangeMaskCompositeOp:
    {
      ChangeMaskImages
        images;

      images.image=image;
      images.source_image=source_image;
      (*pixel)=GetChangeMaskAlpha(&images,p,q,Da);
      break;
    }
    case ClearCompositeOp:
    {
      (*pixel)=(MagickRealType) TransparentAlpha;
      break;
    }
    default:
    {
      CompositeAlphaColorizeRun(row,authentic,pixel);
      break;
    }
  }
}

typedef struct _CompositeChannelTail
{
  const CompositeRowInputs
    *row;

  const CompositePixelInputs
    *authentic;

  Quantum
    *magick_restrict q;

  ssize_t
    i;

  PixelChannel
    channel;

  PixelTrait
    traits,
    source_traits;
} CompositeChannelTail;

static void CompositeSourceChannel(const CompositeChannelTail *tail)
{
  const CompositeRowInputs
    *const row = tail->row;

  const CompositePixelInputs
    *const authentic = tail->authentic;

  Quantum
    *magick_restrict q = tail->q;

  const ssize_t
    i = tail->i;

  const PixelChannel
    channel = tail->channel;

  const PixelTrait
    source_traits = tail->source_traits,
    traits = tail->traits;

  Image
    *const source_image = row->source_image;

  const CompositeOperator
    compose = row->compose;

  const MagickBooleanType
    clamp = row->clamp;

  const Quantum
    *const p = authentic->p;

  const MagickRealType
    alpha = authentic->alpha,
    Da = authentic->Da,
    Sa = authentic->Sa;

  double
    gamma = 0.0;

  MagickRealType
    Dc = 0.0,
    Dca = 0.0,
    Sc = 0.0,
    Sca = 0.0;

  CompositeChannelInputs
    tile;

  MagickRealType
    pixel = 0.0;

  if (source_traits == UndefinedPixelTrait)
    return;
  /*
    Sc: source color.
    Dc: canvas color.
  */
  Sc=(MagickRealType) GetPixelChannel(source_image,channel,p);
  Dc=(MagickRealType) q[i];
  if ((traits & CopyPixelTrait) != 0)
    {
      /*
        Copy channel.
      */
      q[i]=ClampToQuantum(Dc);
      return;
    }
  /*
    Porter-Duff compositions:
      Sca: source normalized color multiplied by alpha.
      Dca: normalized canvas color multiplied by alpha.
  */
  Sca=QuantumScale*Sa*Sc;
  Dca=QuantumScale*Da*Dc;
  gamma=GetCompositeGamma(compose,alpha);
  pixel=Dc;
  tile.channel=channel;
  tile.Dc=Dc;
  tile.Dca=Dca;
  tile.Sc=Sc;
  tile.Sca=Sca;
  tile.gamma=gamma;
  CompositeChannelColor(row,authentic,&tile,&pixel);
  q[i]=clamp != MagickFalse ? ClampPixel(pixel) : ClampToQuantum(pixel);
}

static void CompositeImageChannels(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,Quantum *magick_restrict q)
{
  Image
    *const image = row->image,
    *const source_image = row->source_image;

  const MagickBooleanType
    clamp = row->clamp;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    CompositeChannelTail
      tail;

    MagickRealType
      pixel = 0.0;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait source_traits = GetPixelChannelTraits(source_image,channel);
    if (traits == UndefinedPixelTrait)
      continue;
    if ((channel == AlphaPixelChannel) &&
        ((traits & UpdatePixelTrait) != 0))
      {
        /*
          Set alpha channel.
        */
        CompositeAlphaChannel(row,authentic,q,&pixel);
        q[i]=clamp != MagickFalse ? ClampPixel(pixel) :
          ClampToQuantum(pixel);
        continue;
      }
    tail.row=row;
    tail.authentic=authentic;
    tail.q=q;
    tail.i=i;
    tail.channel=channel;
    tail.traits=traits;
    tail.source_traits=source_traits;
    CompositeSourceChannel(&tail);
  }
}

static const CompositeOperator
  PixelInfoInputOperators[] =
  {
    ColorizeCompositeOp,
    HueCompositeOp,
    LuminizeCompositeOp,
    ModulateCompositeOp,
    RMSECompositeOp,
    SaturateCompositeOp
  };

static const CompositeOperator
  IntensityInputOperators[] =
  {
    BumpmapCompositeOp,
    CopyAlphaCompositeOp,
    DarkenIntensityCompositeOp,
    LightenIntensityCompositeOp
  };

static void CompositeImagePixel(const CompositeRowInputs *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  Image
    *const image = row->image,
    *const source_image = row->source_image;

  CompositePixelInputs
    authentic;

  MagickRealType
    alpha = 0.0,
    Da = 0.0,
    Di = 0.0,
    Sa = 0.0,
    Si = 0.0;

  PixelInfo
    canvas_pixel,
    source_pixel;

  /*
    Authentic composite:
      Sa:  normalized source alpha.
      Da:  normalized canvas alpha.
  */
  Sa=QuantumScale*(double) GetPixelAlpha(source_image,p);
  Da=QuantumScale*(double) GetPixelAlpha(image,q);
  SetCompositeCoverage(row,Sa,Da,&alpha);
  if (row->pixel_info_inputs != MagickFalse)
    {
        Si=GetPixelIntensity(source_image,p);
        GetPixelInfoPixel(source_image,p,&source_pixel);
        GetPixelInfoPixel(image,q,&canvas_pixel);
    }
  else if (row->intensity_inputs != MagickFalse)
    {
        Si=GetPixelIntensity(source_image,p);
        Di=GetPixelIntensity(image,q);
    }
  authentic.p=p;
  authentic.alpha=alpha;
  authentic.Da=Da;
  authentic.Di=Di;
  authentic.Sa=Sa;
  authentic.Si=Si;
  authentic.canvas_pixel=(&canvas_pixel);
  authentic.source_pixel=(&source_pixel);
  CompositeImageChannels(row,&authentic,q);
}

static void CompositeImageRow(const CompositeRowInputs *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  Image
    *const image = row->image,
    *const source_image = row->source_image;

  const CompositeOperator
    compose = row->compose;

  const MagickBooleanType
    clip_to_self = row->clip_to_self;

  const ssize_t
    x_offset = row->x_offset,
    y_offset = row->y_offset,
    y = row->y;

  ExceptionInfo
    *const exception = row->exception;

  const Quantum
    *const pixels = row->pixels;

  const MagickRealType
    canvas_dissolve = row->canvas_dissolve;

  const MagickBooleanType
    clamp = row->clamp;

  const VirtualCompositeInputs
    virtual_inputs = { image, source_image, compose, canvas_dissolve, clamp,
      IsListedOperator(CanvasPixelOperators,
        sizeof(CanvasPixelOperators)/sizeof(*CanvasPixelOperators),compose) };

  PixelInfo
    canvas_pixel,
    source_pixel;

  ssize_t
    x;

  GetPixelInfo(image,&canvas_pixel);
  GetPixelInfo(source_image,&source_pixel);
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (clip_to_self != MagickFalse)
      {
        if (x < x_offset)
          {
            q+=(ptrdiff_t) GetPixelChannels(image);
            continue;
          }
        if ((x-x_offset) >= (ssize_t) source_image->columns)
          break;
      }
    if (IsOutsideSourceColumn(pixels,x,x_offset,source_image) != MagickFalse)
      {
        Quantum
          source[MaxPixelChannels];

        /*
          Virtual composite:
            Sc: source color.
            Dc: canvas color.
        */
        (void) GetOneVirtualPixel(source_image,
          CastDoubleToSsizeT((double) x-x_offset),
          CastDoubleToSsizeT((double) y-y_offset),source,exception);
        SetVirtualCompositeChannels(&virtual_inputs,source,q);
        q+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    CompositeImagePixel(row,p,q);
    p=AdvanceOverSourcePixel(source_image,pixels,p);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static SegmentInfo RotateBlurAxes(const GeometryInfo geometry_info,
  const double width,const double height)
{
  MagickRealType
    angle;

  SegmentInfo
    blur;

  /*
    Rotate vectors if a rotation angle is given.
  */
  angle=DegreesToRadians(geometry_info.xi);
  blur.x1=width*cos(angle);
  blur.x2=width*sin(angle);
  blur.y1=(-height*sin(angle));
  blur.y2=height*cos(angle);
  return(blur);
}

static SegmentInfo GetBlurAxes(const GeometryInfo geometry_info,
  const MagickStatusType flags,const double width,const double height)
{
  SegmentInfo
    blur;

  blur.x1=width;
  blur.x2=0.0;
  blur.y1=0.0;
  blur.y2=height;
  if ((flags & XValue) != 0 )
    blur=RotateBlurAxes(geometry_info,width,height);
  return(blur);
}

static void GetBlurAngleRange(const GeometryInfo geometry_info,
  const MagickStatusType flags,double *angle_start,double *angle_range)
{
  *angle_start=0.0;
  *angle_range=0.0;
  if ((flags & YValue) != 0 )
    {
      /*
        Lets set a angle range and calculate in the loop.
      */
      *angle_start=DegreesToRadians(geometry_info.xi);
      *angle_range=DegreesToRadians(geometry_info.psi)-*angle_start;
    }
}

typedef struct _BlurCanvasInputs
{
  double
    angle_range,
    angle_start,
    height,
    width;

  Image
    *canvas_image,
    *image,
    *source_image;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} BlurCanvasInputs;

static void BlurCompositeCanvas(const BlurCanvasInputs *inputs,
  SegmentInfo blur)
{
  const double
    angle_range = inputs->angle_range,
    angle_start = inputs->angle_start,
    height = inputs->height,
    width = inputs->width;

  Image
    *const canvas_image = inputs->canvas_image,
    *const image = inputs->image,
    *const source_image = inputs->source_image;

  const ssize_t
    x_offset = inputs->x_offset,
    y_offset = inputs->y_offset;

  ExceptionInfo
    *const exception = inputs->exception;

  CacheView
    *canvas_view,
    *source_view;

  PixelInfo
    pixel;

  ResampleFilter
    *resample_filter;

  /*
    Set up a gaussian cylindrical filter for EWA Blurring.

    As the minimum ellipse radius of support*1.0 the EWA algorithm
    can only produce a minimum blur of 0.5 for Gaussian (support=2.0)
    This means that even 'No Blur' will be still a little blurry! The
    solution (as well as the problem of preventing any user expert filter
    settings, is to set our own user settings, restore them afterwards.
  */
  resample_filter=AcquireResampleFilter(image,exception);
  SetResampleFilter(resample_filter,GaussianFilter);
  /*
    Perform the variable blurring of each pixel in image.
  */
  GetPixelInfo(image,&pixel);
  source_view=AcquireVirtualCacheView(source_image,exception);
  canvas_view=AcquireAuthenticCacheView(canvas_image,exception);
  {
    const BlurComposite
      blur_composite = { angle_range, angle_start, height, width,
        canvas_image, image, source_image, canvas_view, source_view,
        resample_filter, x_offset, y_offset, exception };

    BlurCompositeRows(&blur_composite,&blur,&pixel);
  }
  resample_filter=DestroyResampleFilter(resample_filter);
  source_view=DestroyCacheView(source_view);
  canvas_view=DestroyCacheView(canvas_view);
}

static inline MagickBooleanType IsRowOutsideImage(const Image *image,
  const ssize_t y,const ssize_t y_offset)
{
  return((((y+y_offset) < 0) || ((y+y_offset) >= (ssize_t) image->rows)) ?
    MagickTrue : MagickFalse);
}

typedef struct _DisplaceRows
{
  Image
    *canvas_image,
    *image,
    *source_image;

  PointInfo
    center;

  CompositeOperator
    compose;

  MagickRealType
    horizontal_scale,
    vertical_scale;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} DisplaceRows;

static void DisplaceCompositeRows(const DisplaceRows *rows)
{
  Image
    *const canvas_image = rows->canvas_image,
    *const image = rows->image,
    *const source_image = rows->source_image;

  const PointInfo
    center = rows->center;

  const CompositeOperator
    compose = rows->compose;

  const MagickRealType
    horizontal_scale = rows->horizontal_scale,
    vertical_scale = rows->vertical_scale;

  const ssize_t
    x_offset = rows->x_offset,
    y_offset = rows->y_offset;

  ExceptionInfo
    *const exception = rows->exception;

  CacheView
    *canvas_view,
    *image_view,
    *source_view;

  PixelInfo
    pixel;

  ssize_t
    y;

  GetPixelInfo(image,&pixel);
  image_view=AcquireVirtualCacheView(image,exception);
  source_view=AcquireVirtualCacheView(source_image,exception);
  canvas_view=AcquireAuthenticCacheView(canvas_image,exception);
  for (y=0; y < (ssize_t) source_image->rows; y++)
  {
    MagickBooleanType
      sync;

    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    if (IsRowOutsideImage(image,y,y_offset) != MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(source_view,0,y,source_image->columns,1,
      exception);
    q=QueueCacheViewAuthenticPixels(canvas_view,0,y,canvas_image->columns,1,
      exception);
    if (IsBlendRowPairMissing(p,q) != MagickFalse)
      break;
    {
      const DisplaceColumns
        columns = { canvas_image, image, source_image, image_view, center,
          compose, horizontal_scale, vertical_scale, x_offset, y,
          exception };

      if (DisplaceCompositeColumns(&columns,p,q,&pixel) == MagickFalse)
        break;
    }
    sync=SyncCacheViewAuthenticPixels(canvas_view,exception);
    if (sync == MagickFalse)
      break;
  }
  canvas_view=DestroyCacheView(canvas_view);
  source_view=DestroyCacheView(source_view);
  image_view=DestroyCacheView(image_view);
}

static BlendRequest GetBlendRequest(const Image *image,const ssize_t x_offset,
  const ssize_t y_offset)
{
  double
    residual_threshold = 0.0002,
    iterations = 400.0;

  BlendRequest
    request;

  size_t
    tick = 100;

  GetBlendIterations(image,&iterations,&residual_threshold,&tick);
  request.x_offset=x_offset;
  request.y_offset=y_offset;
  request.iterations=iterations;
  request.residual_threshold=residual_threshold;
  request.tick=tick;
  return(request);
}

static void GetDissolveArguments(const Image *image,GeometryInfo *geometry_info,
  MagickRealType *source_dissolve,MagickRealType *canvas_dissolve)
{
  const char
    *artifact;

  MagickStatusType
    flags;

  artifact=GetImageArtifact(image,"compose:args");
  if (artifact != (char *) NULL)
    {
      flags=ParseGeometry(artifact,geometry_info);
      SetDissolveFactors((*geometry_info),flags,source_dissolve,
        canvas_dissolve);
    }
}

static void GetDisplaceArguments(const Image *image,
  GeometryInfo *geometry_info,MagickStatusType *flags)
{
  const char
    *artifact;

  SetGeometryInfo(geometry_info);
  *flags=NoValue;
  artifact=GetImageArtifact(image,"compose:args");
  if (artifact != (char *) NULL)
    *flags=ParseGeometry(artifact,geometry_info);
}

static inline MagickBooleanType IsSourceAlphaNeeded(const Image *image,
  const Image *source_image)
{
  return(((source_image->alpha_trait == UndefinedPixelTrait) &&
    (image->alpha_trait != UndefinedPixelTrait)) ? MagickTrue : MagickFalse);
}

static void SetCanvasAlphaChannel(Image *image,ExceptionInfo *exception)
{
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
}

static void GetBlurEllipseSize(const GeometryInfo geometry_info,
  const MagickStatusType flags,double *width,double *height)
{
  *width=2.0*geometry_info.rho;
  *height=*width;
  if ((flags & HeightValue) != 0)
    *height=2.0*geometry_info.sigma;
}

typedef struct _BlurGeometryInputs
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  Image
    *canvas_image,
    *image,
    *source_image;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} BlurGeometryInputs;

static void BlurCompositeGeometry(const BlurGeometryInputs *geometry)
{
  const GeometryInfo
    geometry_info = geometry->geometry_info;

  const MagickStatusType
    flags = geometry->flags;

  Image
    *const canvas_image = geometry->canvas_image,
    *const image = geometry->image,
    *const source_image = geometry->source_image;

  const ssize_t
    x_offset = geometry->x_offset,
    y_offset = geometry->y_offset;

  ExceptionInfo
    *const exception = geometry->exception;

  double
    angle_range,
    angle_start,
    height,
    width;

  SegmentInfo
    blur;

  /*
    Users input sigma now needs to be converted to the EWA ellipse size.
    The filter defaults to a sigma of 0.5 so to make this match the users
    input the ellipse size needs to be doubled.
  */
  GetBlurEllipseSize(geometry_info,flags,&width,&height);
  /*
    Default the unrotated ellipse width and height axis vectors.
  */
  blur=GetBlurAxes(geometry_info,flags,width,height);
  GetBlurAngleRange(geometry_info,flags,&angle_start,&angle_range);
  {
    const BlurCanvasInputs
      inputs = { angle_range, angle_start, height, width, canvas_image,
        image, source_image, x_offset, y_offset, exception };

    BlurCompositeCanvas(&inputs,blur);
  }
}

typedef struct _DisplaceCanvasInputs
{
  Image
    *canvas_image,
    *image,
    *source_image;

  CompositeOperator
    compose;

  MagickStatusType
    flags;

  GeometryInfo
    geometry_info;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} DisplaceCanvasInputs;

static void DisplaceCompositeCanvas(const DisplaceCanvasInputs *canvas)
{
  Image
    *const canvas_image = canvas->canvas_image,
    *const image = canvas->image,
    *const source_image = canvas->source_image;

  const CompositeOperator
    compose = canvas->compose;

  const MagickStatusType
    flags = canvas->flags;

  const GeometryInfo
    geometry_info = canvas->geometry_info;

  const ssize_t
    x_offset = canvas->x_offset,
    y_offset = canvas->y_offset;

  ExceptionInfo
    *const exception = canvas->exception;

  DisplaceRows
    rows;

  MagickRealType
    horizontal_scale,
    vertical_scale;

  PointInfo
    center;

  {
    const DisplaceSetup
      displace = { compose, flags, geometry_info, image, source_image,
        x_offset, y_offset };

    if ((flags & (WidthValue | HeightValue)) == 0 )
      SetDisplaceScaleDefaults(&displace,&horizontal_scale,
        &vertical_scale);
    else
      SetDisplaceScaleArguments(&displace,&horizontal_scale,
        &vertical_scale);
    /*
      Determine fixed center point for absolute distortion map
       Absolute distort ==
         Displace offset relative to a fixed absolute point
         Select that point according to +X+Y user inputs.
         default = center of overlay image
         arg flag '!' = locations/percentage relative to background image
    */
    center.x=(MagickRealType) x_offset;
    center.y=(MagickRealType) y_offset;
    SetDistortCenter(&displace,&center);
  }
  /*
    Shift the pixel offset point as defined by the provided,
    displacement/distortion map.  -- Like a lens...
  */
  rows.canvas_image=canvas_image;
  rows.image=image;
  rows.source_image=source_image;
  rows.center=center;
  rows.compose=compose;
  rows.horizontal_scale=horizontal_scale;
  rows.vertical_scale=vertical_scale;
  rows.x_offset=x_offset;
  rows.y_offset=y_offset;
  rows.exception=exception;
  DisplaceCompositeRows(&rows);
}

typedef struct _BlurCanvasTargets
{
  Image
    **canvas_image;

  MagickStatusType
    *flags;

  GeometryInfo
    *geometry_info;
} BlurCanvasTargets;

static MagickBooleanType AcquireBlurCanvas(Image *image,Image *source_image,
  ExceptionInfo *exception,const BlurCanvasTargets *targets)
{
  Image
    **const canvas_image = targets->canvas_image;

  MagickStatusType
    *const flags = targets->flags;

  GeometryInfo
    *const geometry_info = targets->geometry_info;

  const char
    *artifact;

  (*canvas_image)=CloneImage(image,0,0,MagickTrue,exception);
  if ((*canvas_image) == (Image *) NULL)
    {
      source_image=DestroyImage(source_image);
      return(MagickFalse);
    }
  /*
    Gather the maximum blur sigma values from user.
  */
  (*flags)=NoValue;
  artifact=GetImageArtifact(image,"compose:args");
  if (artifact != (const char *) NULL)
    (*flags)=ParseGeometry(artifact,geometry_info);
  if (((*flags) & WidthValue) == 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "InvalidSetting","'%s' '%s'","compose:args",artifact);
      source_image=DestroyImage(source_image);
      (*canvas_image)=DestroyImage(*canvas_image);
      return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _ComposeArtifactTargets
{
  double
    *white_luminance;

  IlluminantType
    *illuminant;

  ColorspaceType
    *colorspace;

  MagickBooleanType
    *clamp,
    *compose_sync;
} ComposeArtifactTargets;

static void GetComposeArtifacts(const Image *image,
  const ComposeArtifactTargets *targets)
{
  double
    *const white_luminance = targets->white_luminance;

  IlluminantType
    *const illuminant = targets->illuminant;

  ColorspaceType
    *const colorspace = targets->colorspace;

  MagickBooleanType
    *const clamp = targets->clamp,
    *const compose_sync = targets->compose_sync;

  const char
    *artifact;

  (*white_luminance)=10000.0;
  artifact=GetImageArtifact(image,"compose:white-luminance");
  if (artifact != (const char *) NULL)
    (*white_luminance)=StringToDouble(artifact,(char **) NULL);
  artifact=GetImageArtifact(image,"compose:illuminant");
  if (artifact != (const char *) NULL)
    (*illuminant)=ParseComposeIlluminant(artifact);
  artifact=GetImageArtifact(image,"compose:colorspace");
  if (artifact != (const char *) NULL)
    (*colorspace)=ParseComposeColorspace(artifact);
  (*clamp)=MagickTrue;
  artifact=GetImageArtifact(image,"compose:clamp");
  if (artifact != (const char *) NULL)
    (*clamp)=IsStringTrue(artifact);
  (*compose_sync)=MagickTrue;
  artifact=GetImageArtifact(image,"compose:sync");
  if (artifact != (const char *) NULL)
    (*compose_sync)=IsStringTrue(artifact);
}

static MagickBooleanType IsSourceInsideCanvas(const Image *image,
  const Image *source_image,const ssize_t x_offset,const ssize_t y_offset)
{
  if ((x_offset < 0) || (y_offset < 0))
    return(MagickFalse);
  if ((x_offset+(ssize_t) source_image->columns) > (ssize_t) image->columns)
    return(MagickFalse);
  if ((y_offset+(ssize_t) source_image->rows) > (ssize_t) image->rows)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _CompositeRowHead
{
  const CompositeRowInputs
    *rows;

  CacheView
    *source_view,
    *image_view;

  ssize_t
    y;
} CompositeRowHead;

static MagickBooleanType CompositeImageRowHead(const CompositeRowHead *head,
  MagickBooleanType *status)
{
  const CompositeRowInputs
    *const rows = head->rows;

  CacheView
    *const image_view = head->image_view;

  const ssize_t
    y = head->y;

  Image
    *const image = rows->image;

  ExceptionInfo
    *const exception = rows->exception;

  const Quantum
    *magick_restrict p,
    *pixels;

  Quantum
    *magick_restrict q;

  CompositeRowInputs
    row;

  const OverSourceRow
    source = { head->source_view, rows->source_image, rows->clip_to_self,
      rows->x_offset, rows->y_offset, y, exception };

  if (SkipOverRow(&source,status) == MagickFalse)
    return(MagickFalse);
  if (FetchOverSourceRow(&source,&pixels,&p,status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  row=(*rows);
  row.y=y;
  row.pixels=pixels;
  CompositeImageRow(&row,p,q);
  SyncCompositeRow(image_view,exception,status);
  return(MagickTrue);
}

static MagickBooleanType CompositeImageRows(const CompositeRowInputs *rows)
{
  Image
    *const image = rows->image,
    *const source_image = rows->source_image;

  ExceptionInfo
    *const exception = rows->exception;

  CacheView
    *source_view,
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  status=MagickTrue;
  progress=0;
  source_view=AcquireVirtualCacheView(source_image,exception);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(source_image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const CompositeRowHead
      head = { rows, source_view, image_view, y };

    if (CompositeImageRowHead(&head,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetCompositeProgress(image,CompositeImageTag,progress,&status);
      }
  }
  source_view=DestroyCacheView(source_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

typedef struct _ComposeArgumentTargets
{
  GeometryInfo
    *geometry_info;

  MagickRealType
    *percent_luma,
    *percent_chroma,
    *amount,
    *threshold,
    *source_dissolve,
    *canvas_dissolve;
} ComposeArgumentTargets;

static void GetComposeArguments(Image *image,const CompositeOperator compose,
  ExceptionInfo *exception,const ComposeArgumentTargets *targets)
{
  GeometryInfo
    *const geometry_info = targets->geometry_info;

  MagickRealType
    *const percent_luma = targets->percent_luma,
    *const percent_chroma = targets->percent_chroma,
    *const amount = targets->amount,
    *const threshold = targets->threshold;

  switch (compose)
  {
    case CopyAlphaCompositeOp:
    case ChangeMaskCompositeOp:
    {
      /*
        Modify canvas outside the overlaid region and require an alpha
        channel to exist, to add transparency.
      */
      SetCanvasAlphaChannel(image,exception);
      break;
    }
    case MathematicsCompositeOp:
    {
      /*
        Just collect the values from "compose:args", setting.
        Unused values are set to zero automagically.

        Arguments are normally a comma separated list, so this probably should
        be changed to some 'general comma list' parser, (with a minimum
        number of values)
      */
      GetMathematicsArguments(image,geometry_info,exception);
      break;
    }
    case ModulateCompositeOp:
    {
      /*
        Determine the luma and chroma scale.
      */
      GetModulateArguments(image,geometry_info,percent_luma,percent_chroma);
      break;
    }
    case ThresholdCompositeOp:
    {
      /*
        Determine the amount and threshold.
      */
      GetThresholdArguments(image,geometry_info,amount,threshold);
      break;
    }
    default:
      break;
  }
}

static void GetBlendArguments(Image *image,const CompositeOperator compose,
  ExceptionInfo *exception,const ComposeArgumentTargets *targets)
{
  const char
    *artifact;

  GeometryInfo
    *const geometry_info = targets->geometry_info;

  MagickRealType
    *const source_dissolve = targets->source_dissolve,
    *const canvas_dissolve = targets->canvas_dissolve;

  MagickStatusType
    flags;

  switch (compose)
  {
    case DissolveCompositeOp:
    {
      /*
        Geometry arguments to dissolve factors.
      */
      GetDissolveArguments(image,geometry_info,source_dissolve,
        canvas_dissolve);
      break;
    }
    case BlendCompositeOp:
    {
      artifact=GetImageArtifact(image,"compose:args");
      if (artifact != (char *) NULL)
        {
          flags=ParseGeometry(artifact,geometry_info);
          SetBlendFactors((*geometry_info),flags,source_dissolve,
            canvas_dissolve);
        }
      break;
    }
    default:
    {
      GetComposeArguments(image,compose,exception,targets);
      break;
    }
  }
}

typedef struct _CopyCompositeInputs
{
  Image
    *image,
    *source_image;

  ssize_t
    x_offset,
    y_offset;

  CompositeSetup
    setup;

  ExceptionInfo
    *exception;
} CopyCompositeInputs;

static MagickBooleanType CompositeCopyGroups(const CopyCompositeInputs *copy,
  const CompositeOperator compose,const ComposeArgumentTargets *targets,
  MagickBooleanType *result)
{
  Image
    *const image = copy->image,
    *const source_image = copy->source_image;

  const ssize_t
    x_offset = copy->x_offset,
    y_offset = copy->y_offset;

  CompositeSetup
    setup = copy->setup;

  ExceptionInfo
    *const exception = copy->exception;

  switch (compose)
  {
    case CopyCompositeOp:
    {
      if (IsSourceInsideCanvas(image,source_image,x_offset,y_offset) ==
          MagickFalse)
        break;
      if (IsSourceAlphaNeeded(image,source_image) != MagickFalse)
        (void) SetImageAlphaChannel(source_image,OpaqueAlphaChannel,exception);
      (*result)=CompositeCopyImage(image,source_image,&setup,exception);
      return(MagickTrue);
    }
    case IntensityCompositeOp:
    {
      if (IsSourceInsideCanvas(image,source_image,x_offset,y_offset) ==
          MagickFalse)
        break;
      setup.intensity=MagickTrue;
      (*result)=CompositeCopyImage(image,source_image,&setup,exception);
      return(MagickTrue);
    }
    default:
    {
      GetBlendArguments(image,compose,exception,targets);
      break;
    }
  }
  return(MagickFalse);
}

typedef struct _BlendCompositeInputs
{
  Image
    *image;

  const Image
    *composite;

  Image
    *source_image;

  ssize_t
    x_offset,
    y_offset;

  CompositeSetup
    setup;

  ExceptionInfo
    *exception;
} BlendCompositeInputs;

static MagickBooleanType CompositeBlendGroups(const BlendCompositeInputs *blend,
  const CompositeOperator compose,const ComposeArgumentTargets *targets,
  MagickBooleanType *result)
{
  Image
    *const image = blend->image;

  const Image
    *const composite = blend->composite;

  Image
    *source_image = blend->source_image;

  const ssize_t
    x_offset = blend->x_offset,
    y_offset = blend->y_offset;

  const CompositeSetup
    setup = blend->setup;

  ExceptionInfo
    *const exception = blend->exception;

  MagickBooleanType
    returned,
    status;

  switch (compose)
  {
    case SaliencyBlendCompositeOp:
    {
      BlendRequest
        request;

      request=GetBlendRequest(image,x_offset,y_offset);
      status=SaliencyBlendImage(image,composite,&request,exception);
      source_image=DestroyImage(source_image);
      (*result)=status;
      return(MagickTrue);
    }
    case SeamlessBlendCompositeOp:
    {
      BlendRequest
        request;

      request=GetBlendRequest(image,x_offset,y_offset);
      status=SeamlessBlendImage(image,composite,&request,exception);
      source_image=DestroyImage(source_image);
      (*result)=status;
      return(MagickTrue);
    }
    default:
    {
      const CopyCompositeInputs
        copy = { image, source_image, x_offset, y_offset, setup, exception };

      if (CompositeCopyGroups(&copy,compose,targets,&returned) !=
          MagickFalse)
        {
          (*result)=returned;
          return(MagickTrue);
        }
      break;
    }
  }
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p o s i t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompositeImage() returns the second image composited onto the first
%  at the specified offset, using the specified composite method.
%
%  The format of the CompositeImage method is:
%
%      MagickBooleanType CompositeImage(Image *image,
%        const Image *source_image,const CompositeOperator compose,
%        const MagickBooleanType clip_to_self,const ssize_t x_offset,
%        const ssize_t y_offset,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the canvas image, modified by he composition
%
%    o source_image: the source image.
%
%    o compose: This operator affects how the composite is applied to
%      the image.  The operators and how they are utilized are listed here
%      http://www.w3.org/TR/SVG12/#compositing.
%
%    o clip_to_self: set to MagickTrue to limit composition to area composed.
%
%    o x_offset: the column offset of the composited image.
%
%    o y_offset: the row offset of the composited image.
%
%  Extra Controls from Image meta-data in 'image' (artifacts)
%
%    o "compose:args"
%        A string containing extra numerical arguments for specific compose
%        methods, generally expressed as a 'geometry' or a comma separated list
%        of numbers.
%
%        Compose methods needing such arguments include "BlendCompositeOp" and
%        "DisplaceCompositeOp".
%
%    o exception: return any errors or warnings in this structure.
%
*/

/*
   Composition based on the SVG specification:

   A Composition is defined by...
      Color Function :  f(Sc,Dc)  where Sc and Dc are the normalized colors
      Blending areas :  X = 1     for area of overlap, ie: f(Sc,Dc)
                        Y = 1     for source preserved
                        Z = 1     for canvas preserved

   Conversion to transparency (then optimized)
      Dca' = f(Sc, Dc)*Sa*Da + Y*Sca*(1-Da) + Z*Dca*(1-Sa)
      Da'  = X*Sa*Da + Y*Sa*(1-Da) + Z*Da*(1-Sa)

   Where...
      Sca = Sc*Sa     normalized Source color divided by Source alpha
      Dca = Dc*Da     normalized Dest color divided by Dest alpha
      Dc' = Dca'/Da'  the desired color value for this channel.

   Da' in in the follow formula as 'gamma'  The resulting alpha value.

   Most functions use a blending mode of over (X=1,Y=1,Z=1) this results in
   the following optimizations...
      gamma = Sa+Da-Sa*Da;
      gamma = 1 - QuantumScale*alpha * QuantumScale*beta;
      opacity = QuantumScale*alpha*beta;  // over blend, optimized 1-Gamma

   The above SVG definitions also define that Mathematical Composition
   methods should use a 'Over' blending mode for Alpha Channel.
   It however was not applied for composition modes of 'Plus', 'Minus',
   the modulus versions of 'Add' and 'Subtract'.

   Mathematical operator changes to be applied from IM v6.7...

    1) Modulus modes 'Add' and 'Subtract' are obsoleted and renamed
       'ModulusAdd' and 'ModulusSubtract' for clarity.

    2) All mathematical compositions work as per the SVG specification
       with regard to blending.  This now includes 'ModulusAdd' and
       'ModulusSubtract'.

    3) When the special channel flag 'sync' (synchronize channel updates)
       is turned off (enabled by default) then mathematical compositions are
       only performed on the channels specified, and are applied
       independently of each other.  In other words the mathematics is
       performed as 'pure' mathematical operations, rather than as image
       operations.
*/

MagickExport MagickBooleanType CompositeImage(Image *image,
  const Image *composite,const CompositeOperator compose,
  const MagickBooleanType clip_to_self,const ssize_t x_offset,
  const ssize_t y_offset,ExceptionInfo *exception)
{
#define CompositeImageTag  "Composite/Image"

  ColorspaceType
    colorspace = HCLColorspace;

  CompositeRowInputs
    rows;

  CompositeSetup
    setup;

  double
    white_luminance = 10000.0;

  GeometryInfo
    geometry_info;

  IlluminantType
    illuminant = D65Illuminant;

  Image
    *canvas_image,
    *source_image;

  MagickBooleanType
    clamp,
    compose_sync,
    returned,
    status;

  MagickRealType
    amount,
    canvas_dissolve,
    midpoint,
    percent_luma,
    percent_chroma,
    source_dissolve,
    threshold;

  MagickStatusType
    flags;

  const ComposeArgumentTargets
    argument_targets = { &geometry_info, &percent_luma, &percent_chroma,
      &amount, &threshold, &source_dissolve, &canvas_dissolve };

  const ComposeArtifactTargets
    targets = { &white_luminance, &illuminant, &colorspace, &clamp,
      &compose_sync };

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(composite != (Image *) NULL);
  assert(composite->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  source_image=CloneImage(composite,0,0,MagickTrue,exception);
  if (source_image == (const Image *) NULL)
    return(MagickFalse);
  (void) SetImageColorspace(source_image,image->colorspace,exception);
  if ((compose == OverCompositeOp) || (compose == SrcOverCompositeOp))
    {
      const CompositePlacement
        placement = { clip_to_self, x_offset, y_offset };

      status=CompositeOverImage(image,source_image,&placement,exception);
      source_image=DestroyImage(source_image);
      return(status);
    }
  amount=0.5;
  canvas_image=(Image *) NULL;
  canvas_dissolve=1.0;
  GetComposeArtifacts(image,&targets);
  SetGeometryInfo(&geometry_info);
  percent_luma=100.0;
  percent_chroma=100.0;
  source_dissolve=1.0;
  threshold=0.05f;
  setup.x_offset=x_offset;
  setup.y_offset=y_offset;
  setup.clamp=clamp;
  setup.intensity=MagickFalse;
  switch (compose)
  {
    case BlurCompositeOp:
    {
      BlurCanvasTargets
        blur_targets;

      /*
        Blur Image by resampling dictated by an overlay gradient map:
          X = red_channel; Y = green_channel; compose:args =
          x_scale[,y_scale[,angle]].
      */
      blur_targets.canvas_image=(&canvas_image);
      blur_targets.flags=(&flags);
      blur_targets.geometry_info=(&geometry_info);
      if (AcquireBlurCanvas(image,source_image,exception,&blur_targets) ==
          MagickFalse)
        return(MagickFalse);
      {
        const BlurGeometryInputs
          geometry = { geometry_info, flags, canvas_image, image, source_image,
            x_offset, y_offset, exception };

        BlurCompositeGeometry(&geometry);
      }
      source_image=DestroyImage(source_image);
      source_image=canvas_image;
      break;
    }
    case DisplaceCompositeOp:
    case DistortCompositeOp:
    {
      /*
        Displace/Distort based on overlay gradient map:
          X = red_channel;  Y = green_channel;
          compose:args = x_scale[,y_scale[,center.x,center.y]]
      */
      canvas_image=CloneImage(image,0,0,MagickTrue,exception);
      if (canvas_image == (Image *) NULL)
        {
          source_image=DestroyImage(source_image);
          return(MagickFalse);
        }
      GetDisplaceArguments(image,&geometry_info,&flags);
      {
        const DisplaceCanvasInputs
          canvas = { canvas_image, image, source_image, compose, flags,
            geometry_info, x_offset, y_offset, exception };

        DisplaceCompositeCanvas(&canvas);
      }
      source_image=DestroyImage(source_image);
      source_image=canvas_image;
      break;
    }
    default:
    {
      const BlendCompositeInputs
        blend = { image, composite, source_image, x_offset, y_offset, setup,
          exception };

      if (CompositeBlendGroups(&blend,compose,&argument_targets,&returned) !=
          MagickFalse)
        return(returned);
      break;
    }
  }
  /*
    Composite image.
  */
  midpoint=((MagickRealType) QuantumRange+1.0)/2;
  rows.image=image;
  rows.source_image=source_image;
  rows.compose=compose;
  rows.union_coverage=IsListedOperator(UnionCoverageOperators,
    sizeof(UnionCoverageOperators)/sizeof(*UnionCoverageOperators),compose);
  rows.canvas_alpha=IsListedOperator(CanvasAlphaOperators,
    sizeof(CanvasAlphaOperators)/sizeof(*CanvasAlphaOperators),compose);
  rows.source_alpha=IsListedOperator(SourceAlphaOperators,
    sizeof(SourceAlphaOperators)/sizeof(*SourceAlphaOperators),compose);
  rows.pixel_info_inputs=IsListedOperator(PixelInfoInputOperators,
    sizeof(PixelInfoInputOperators)/sizeof(*PixelInfoInputOperators),compose);
  rows.intensity_inputs=IsListedOperator(IntensityInputOperators,
    sizeof(IntensityInputOperators)/sizeof(*IntensityInputOperators),compose);
  rows.clip_to_self=clip_to_self;
  rows.x_offset=x_offset;
  rows.y_offset=y_offset;
  rows.exception=exception;
  rows.amount=amount;
  rows.canvas_dissolve=canvas_dissolve;
  rows.midpoint=midpoint;
  rows.percent_luma=percent_luma;
  rows.percent_chroma=percent_chroma;
  rows.source_dissolve=source_dissolve;
  rows.threshold=threshold;
  rows.clamp=clamp;
  rows.compose_sync=compose_sync;
  rows.colorspace=colorspace;
  rows.white_luminance=white_luminance;
  rows.illuminant=illuminant;
  rows.geometry_info=geometry_info;
  status=CompositeImageRows(&rows);
  if (canvas_image != (Image * ) NULL)
    canvas_image=DestroyImage(canvas_image);
  else
    source_image=DestroyImage(source_image);
  return(status);
}
