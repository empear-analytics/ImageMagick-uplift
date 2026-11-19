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

  MagickCore private composite methods: the coverage and the alpha multiply
  and darken runs of the general rows, from SetCompositeCoverageModulus to
  CompositeAlphaDarkenRun; included by composite-image.c only, directly
  before composite-image-coverage-private.h.
*/
#ifndef MAGICKCORE_COMPOSITE_IMAGE_ALPHA_PRIVATE_H
#define MAGICKCORE_COMPOSITE_IMAGE_ALPHA_PRIVATE_H

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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
