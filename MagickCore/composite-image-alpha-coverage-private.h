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

  MagickCore private composite methods: the coverage of the general rows,
  from SetCompositeCoverageModulus to the SourceAlphaOperators table;
  included by composite-image.c only, directly before
  composite-image-alpha-private.h.
*/
#ifndef MAGICKCORE_COMPOSITE_IMAGE_ALPHA_COVERAGE_PRIVATE_H
#define MAGICKCORE_COMPOSITE_IMAGE_ALPHA_COVERAGE_PRIVATE_H

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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
