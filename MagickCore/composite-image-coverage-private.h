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

  MagickCore private composite methods: coverage, alpha and the channel and pixel composite of the general rows.
*/
#ifndef MAGICKCORE_COMPOSITE_IMAGE_COVERAGE_PRIVATE_H
#define MAGICKCORE_COMPOSITE_IMAGE_COVERAGE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
