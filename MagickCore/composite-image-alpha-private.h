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

  MagickCore private composite methods: the alpha multiply and darken runs
  of the general rows, CompositeAlphaMultiplyRun and CompositeAlphaDarkenRun;
  included by composite-image.c only, directly before
  composite-image-coverage-private.h.
*/
#ifndef MAGICKCORE_COMPOSITE_IMAGE_ALPHA_PRIVATE_H
#define MAGICKCORE_COMPOSITE_IMAGE_ALPHA_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

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
