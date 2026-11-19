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

  MagickCore private composite channel color methods.
*/
#ifndef MAGICKCORE_COMPOSITE_CHANNEL_PRIVATE_H
#define MAGICKCORE_COMPOSITE_CHANNEL_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static void CompositeChannelColor(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,const CompositeChannelInputs *tile,
  MagickRealType *pixel)
{
  Image
    *const source_image = row->source_image;

  const CompositeOperator
    compose = row->compose;

  const MagickRealType
    canvas_dissolve = row->canvas_dissolve,
    midpoint = row->midpoint,
    percent_luma = row->percent_luma,
    percent_chroma = row->percent_chroma,
    source_dissolve = row->source_dissolve;

  const MagickBooleanType
    compose_sync = row->compose_sync;

  const ColorspaceType
    colorspace = row->colorspace;

  const double
    white_luminance = row->white_luminance;

  const IlluminantType
    illuminant = row->illuminant;

  const GeometryInfo
    geometry_info = row->geometry_info;

  const Quantum
    *const p = authentic->p;

  const MagickRealType
    Da = authentic->Da,
    Di = authentic->Di,
    Sa = authentic->Sa,
    Si = authentic->Si;

  const PixelInfo
    *const canvas_pixel = authentic->canvas_pixel,
    *const source_pixel = authentic->source_pixel;

  const PixelChannel
    channel = tile->channel;

  const MagickRealType
    Dc = tile->Dc,
    Dca = tile->Dca,
    Sc = tile->Sc,
    Sca = tile->Sca;

  const double
    gamma = tile->gamma;

  MagickRealType
    blend = 0.0,
    D = 0.0,
    S = 0.0,
    blue = 0.0,
    chroma = 0.0,
    green = 0.0,
    hue = 0.0,
    luma = 0.0,
    red = 0.0,
    sans = 0.0;

  switch (compose)
  {
    case AlphaCompositeOp:
    {
      (*pixel)=(double) QuantumRange*Sa;
      break;
    }
    case AtopCompositeOp:
    case SrcAtopCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(Sca*Da+Dca*(1.0-Sa));
      break;
    }
    case BlendCompositeOp:
    {
      (*pixel)=gamma*(source_dissolve*Sa*Sc+canvas_dissolve*Da*Dc);
      break;
    }
    case CopyCompositeOp:
    case ReplaceCompositeOp:
    {
      (*pixel)=(double) QuantumRange*Sca;
      break;
    }
    case BlurCompositeOp:
    case DisplaceCompositeOp:
    case DistortCompositeOp:
    case SrcCompositeOp:
    {
      (*pixel)=Sc;
      break;
    }
    case BumpmapCompositeOp:
    {
      if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Dc;
          break;
        }
      (*pixel)=(QuantumScale*Si)*Dc;
      break;
    }
    case ChangeMaskCompositeOp:
    {
      (*pixel)=Dc;
      break;
    }
    case ClearCompositeOp:
    {
      (*pixel)=0.0;
      break;
    }
    case ColorBurnCompositeOp:
    {
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      if (fabs(S) < MagickEpsilon)
        blend=0.0;
      else
        blend=MagickMin(1.0,MagickMax(0.0,1.0-(1.0-D)/S));
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(Sa*Da*blend+Sa*
        (1.0-Da)*S+Da*(1.0-Sa)*D);
      break;
    }
    case ColorDodgeCompositeOp:
    {
      if (Sa > 0.0)
        S=RoundToUnity(Sca/Sa);
      else
        S=0.0;
      if (Da > 0.0)
        D=RoundToUnity(Dca/Da);
      else
        D=0.0;
      blend=GetColorDodgeBlend(S,D);
      (*pixel)=(double) QuantumRange*gamma*(Sa*Da*blend+Sca*(1.0-Da)+Dca*
        (1.0-Sa));
      break;
    }
    case ColorizeCompositeOp:
    {
      if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Dc;
          break;
        }
      if (fabs((double) QuantumRange*Da-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Sc;
          break;
        }
      ConvertRGBToGeneric(colorspace,(double) (*canvas_pixel).red,
        (double) (*canvas_pixel).green,(double) (*canvas_pixel).blue,
        white_luminance,illuminant,&sans,&sans,&luma);
      ConvertRGBToGeneric(colorspace,(double) (*source_pixel).red,
        (double) (*source_pixel).green,(double) (*source_pixel).blue,
        white_luminance,illuminant,&hue,&chroma,&sans);
      ConvertGenericToRGB(colorspace,hue,chroma,luma,
        white_luminance,illuminant,&red,&green,&blue);
      (*pixel)=GetCompositeChannelColor(tile,red,green,blue);
      break;
    }
    case CopyAlphaCompositeOp:
    case DstCompositeOp:
    {
      (*pixel)=Dc;
      break;
    }
    case CopyBlackCompositeOp:
    {
      if (channel == BlackPixelChannel)
        (*pixel)=(MagickRealType) GetPixelBlack(source_image,p);
      break;
    }
    case CopyBlueCompositeOp:
    case CopyYellowCompositeOp:
    {
      if (channel == BluePixelChannel)
        (*pixel)=(MagickRealType) GetPixelBlue(source_image,p);
      break;
    }
    case CopyGreenCompositeOp:
    case CopyMagentaCompositeOp:
    {
      if (channel == GreenPixelChannel)
        (*pixel)=(MagickRealType) GetPixelGreen(source_image,p);
      break;
    }
    case CopyRedCompositeOp:
    case CopyCyanCompositeOp:
    {
      if (channel == RedPixelChannel)
        (*pixel)=(MagickRealType) GetPixelRed(source_image,p);
      break;
    }
    case DarkenCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=RoundToUnity(MagickMin(Sca,Dca)+Sca*(1.0-Da)+Dca*
            (1.0-Sa));
          break;
        }
      (*pixel)=(double) QuantumRange*RoundToUnity(MagickMin(Sca,Dca)+Sca*
        (1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case DarkenIntensityCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=RoundToUnity(MagickMin(Sca,Dca)+Sca*(1.0-Di)+Dca*
            (1.0-Si));
          break;
        }
      (*pixel)=(double) QuantumRange*RoundToUnity(MagickMin(Sca,Dca)+Sca*
        (1.0-Di)+Dca*(1.0-Si));
      break;
    }
    case DifferenceCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
           (*pixel)=(double) QuantumRange*RoundToUnity(fabs((double) Sc-
            (double) Dc));
           break;
        }
      S=(Sa > 0.0) ? (Sca/Sa) : 0.0;
      D=(Da > 0.0) ? (Dca/Da) : 0.0;
      (*pixel)=(double) QuantumRange*RoundToUnity(fabs(S-D));
      break;
    }
    case DissolveCompositeOp:
    {
      (*pixel)=gamma*(source_dissolve*Sa*Sc-source_dissolve*Sa*
        canvas_dissolve*Da*Dc+canvas_dissolve*Da*Dc);
      break;
    }
    case DivideCompositeOp:
    {
      D=(Da > 0.0) ? (Dca/Da) : 0.0;
      S=(Sa > 0.0) ? (Sca/Sa) : 0.0;
      if (fabs(S) < MagickEpsilon)
        blend=1.0;
      else
        blend=RoundToUnity(D/S);
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(Sca*(1.0-Da)+Dca*(1.0-Sa)+
        Sa*Da*blend);
      break;
    }
    case DivideDstCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=GetChannelQuotient(Sc,Dc);
          break;
        }
      if (fabs(Dca) < MagickEpsilon)
        (*pixel)=(double) QuantumRange*gamma*(Sca+(1.0-Sa));
      else
        (*pixel)=(double) QuantumRange*gamma*(Sca/Dca+Sca*(1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case DivideSrcCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=GetChannelQuotient(Dc,Sc);
          break;
        }
      if (fabs(Dca) < MagickEpsilon)
        (*pixel)=(double) QuantumRange*gamma*(Dca+(1.0-Da));
      else
        (*pixel)=(double) QuantumRange*gamma*(Dca/Sca+Dca*(1.0-Sa)+Sca*(1.0-Da));
      break;
    }
    case DstAtopCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(Dca*Sa+Sca*(1.0-Da));
      break;
    }
    case DstInCompositeOp:
    {
      (*pixel)=(double) QuantumRange*gamma*(Dca*Sa);
      break;
    }
    case DstOutCompositeOp:
    {
      (*pixel)=(double) QuantumRange*gamma*(Dca*(1.0-Sa));
      break;
    }
    case DstOverCompositeOp:
    {
      (*pixel)=(double) QuantumRange*gamma*(Dca+Sca*(1.0-Da));
      break;
    }
    case ExclusionCompositeOp:
    {
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      blend=RoundToUnity(S+D-2.0*S*D);
      (*pixel)=(double) QuantumRange*RoundToUnity((blend*Sa+D*(1.0-Sa))*
        (Sa+Da-Sa*Da));
      break;
    }
    case FreezeCompositeOp:
    {          
      if (Dca != 0.0)
        blend=1.0-(1.0-Sca)*(1.0-Sca)/Dca;
      else
        blend=0.0;
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(blend);
      break;
    }
    case HardLightCompositeOp:
    {
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      if (S <= 0.5)
        blend=2.0*S*D;
      else
        blend=1.0-2.0*(1.0-S)*(1.0-D);
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity((1.0-Da)*Sca+
        (1.0-Sa)*Dca+blend*Sa*Da);
      break;
    }
    case HardMixCompositeOp:
    {
      double
        hardmix,
        vivid;

      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      vivid=GetHardMixVivid(S,D);
      hardmix=(vivid < 0.5) ? 0.0 : 1.0;
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(Sa*Da*hardmix+Sca*
        (1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case HueCompositeOp:
    {
      if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Dc;
          break;
        }
      if (fabs((double) QuantumRange*Da-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Sc;
          break;
        }
      ConvertRGBToGeneric(colorspace,(double) (*canvas_pixel).red,
        (double) (*canvas_pixel).green,(double) (*canvas_pixel).blue,
        white_luminance,illuminant,&hue,&chroma,&luma);
      ConvertRGBToGeneric(colorspace,(double) (*source_pixel).red,
        (double) (*source_pixel).green,(double) (*source_pixel).blue,
        white_luminance,illuminant,&hue,&sans,&sans);
      ConvertGenericToRGB(colorspace,hue,chroma,luma,
        white_luminance,illuminant,&red,&green,&blue);
      (*pixel)=GetCompositeChannelColor(tile,red,green,blue);
      break;
    }
    case InCompositeOp:
    case SrcInCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(Sca*Da);
      break;
    }
    case InterpolateCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(0.5-0.25*cos(MagickPI*Sca)-0.25*
        cos(MagickPI*Dca));
      break;
    }
    case LinearBurnCompositeOp:
    {
      /*
        LinearBurn: as defined by Abode Photoshop, according to
        http://www.simplefilter.de/en/basics/mixmods.html is:

          f(Sc,Dc) = Sc + Dc - 1
      */
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      blend=RoundToUnity(S+D-1.0);
      (*pixel)=(double) QuantumRange*RoundToUnity(Sa*Da*blend+Sca*(1.0-Da)+
        Dca*(1.0-Sa));
      break;
    }
    case LinearDodgeCompositeOp:
    {
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      blend=RoundToUnity(S+D);
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(Sa*Da*blend+
        Sca*(1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case LinearLightCompositeOp:
    {
      /*
        Linear Light (Adobe standard):
          f(Sc, Dc) = Dc + 2*Sc - 1
        Applied in linear (HDRI) space with clamping.
      */
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      blend=RoundToUnity(D+2.0*S-1.0);
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(Sa*Da*blend+Sca*
        (1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case LightenCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=MagickMax(Sc,Dc);
          break;
        }
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      (*pixel)=(double) QuantumRange*RoundToUnity(Sca*(1.0-Da)+Dca*(1.0-Sa)+
        MagickMax(S,D)*Sa*Da);
      break;
    }
    case LightenIntensityCompositeOp:
    {
      (*pixel)=Si > Di ? Sc : Dc;
      break;
    }
    case LuminizeCompositeOp:
    {
      if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Dc;
          break;
        }
      if (fabs((double) QuantumRange*Da-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Sc;
          break;
        }
      ConvertRGBToGeneric(colorspace,(double) (*canvas_pixel).red,
        (double) (*canvas_pixel).green,(double) (*canvas_pixel).blue,
        white_luminance,illuminant,&hue,&chroma,&luma);
      ConvertRGBToGeneric(colorspace,(double) (*source_pixel).red,
        (double) (*source_pixel).green,(double) (*source_pixel).blue,
        white_luminance,illuminant,&sans,&sans,&luma);
      ConvertGenericToRGB(colorspace,hue,chroma,luma,
        white_luminance,illuminant,&red,&green,&blue);
      (*pixel)=GetCompositeChannelColor(tile,red,green,blue);
      break;
    }
    case MathematicsCompositeOp:
    {
      /*
        'Mathematics' a free form user control mathematical composition
        is defined as...

          f(Sc,Dc) = A*Sc*Dc + B*Sc + C*Dc + D

        Where the arguments A,B,C,D are (currently) passed to composite
        as a command separated 'geometry' string in "compose:args" image
        artifact.

           A = a->rho,   B = a->sigma,  C = a->xi,  D = a->psi

        Applying the SVG transparency formula (see above), we get...

         Dca' = Sa*Da*f(Sc,Dc) + Sca*(1.0-Da) + Dca*(1.0-Sa)

         Dca' = A*Sca*Dca + B*Sca*Da + C*Dca*Sa + D*Sa*Da + Sca*(1.0-Da) +
           Dca*(1.0-Sa)
      */
      if (compose_sync == MagickFalse)
        {
          (*pixel)=geometry_info.rho*Sc*Dc+geometry_info.sigma*Sc+
            geometry_info.xi*Dc+geometry_info.psi;
          break;
        }
      (*pixel)=(double) QuantumRange*gamma*(geometry_info.rho*Sca*Dca+
        geometry_info.sigma*Sca*Da+geometry_info.xi*Dca*Sa+
        geometry_info.psi*Sa*Da+Sca*(1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case MinusDstCompositeOp:
    {
     if (compose_sync == MagickFalse)
        {
          (*pixel)=Dc-Sc;
          break;
        }
      (*pixel)=gamma*(Sa*Sc+Da*Dc-2.0*Da*Dc*Sa);
      break;
    }
    case MinusSrcCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=Sc-Dc;
          break;
        }
      (*pixel)=gamma*(Da*Dc+Sa*Sc-2.0*Sa*Sc*Da);
      break;
    }
    case ModulateCompositeOp:
    {
      ssize_t
        offset;

      if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Dc;
          break;
        }
      offset=(ssize_t) (Si-midpoint);
      if (offset == 0)
        {
          (*pixel)=Dc;
          break;
        }
      ConvertRGBToGeneric(colorspace,(double) (*canvas_pixel).red,
        (double) (*canvas_pixel).green,(double) (*canvas_pixel).blue,
        white_luminance,illuminant,&hue,&chroma,&luma);
      luma+=(0.01*percent_luma*offset)/midpoint;
      chroma*=0.01*percent_chroma;
      ConvertGenericToRGB(colorspace,hue,chroma,luma,
        white_luminance,illuminant,&red,&green,&blue);
      (*pixel)=GetCompositeChannelColor(tile,red,green,blue);
      break;
    }
    case ModulusAddCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=(Quantum) QuantumRange*((Sc+Dc)-floor(Sc+Dc));
          break;
        }
      (*pixel)=(Quantum) QuantumRange*((Sca+Dca)-floor(Sca+Dca));
      break;
    }
    case ModulusSubtractCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=(Quantum) QuantumRange*((Sc-Dc)-floor(Sc-Dc));
          break;
        }
      (*pixel)=(Quantum) QuantumRange*((Sca-Dca)-floor(Sca-Dca));
      break;
    }
    case MultiplyCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=(double) QuantumRange*Sc*Dc;
          break;
        }
      (*pixel)=(double) QuantumRange*gamma*(Sca*Dca+Sca*(1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case NegateCompositeOp:
    {
      D=(Da > 0.0) ? Dca/Da : 0.0;
      S=(Sa > 0.0) ? Sca/Sa : 0.0;
      (*pixel)=(double) QuantumRange*((1.0-fabs(1.0-S-D))*Da);
      break;
    }
    case NoCompositeOp:
    {
      (*pixel)=(double) QuantumRange*Dca;
      break;
    }
    case OutCompositeOp:
    case SrcOutCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(Sca*(1.0-Da));
      break;
    }
    case OverCompositeOp:
    case SrcOverCompositeOp:
    {
      if ((Sa+Da*(1.0-Sa)) <= MagickEpsilon)
        (*pixel)=0.0;
      else
        (*pixel)=(double) QuantumRange*gamma*((Sca+Dca*(1.0-Sa))/
          (Sa+Da*(1.0-Sa)));
      break;
    }
    case OverlayCompositeOp:
    {
      if ((2.0*Dca) < Da)
        {
          (*pixel)=(double) QuantumRange*gamma*(2.0*Dca*Sca+Dca*(1.0-Sa)+
            Sca*(1.0-Da));
          break;
        }
      (*pixel)=(double) QuantumRange*gamma*(Da*Sa-2.0*(Sa-Sca)*(Da-Dca)+Dca*
        (1.0-Sa)+Sca*(1.0-Da));
      break;
    }
    case PegtopLightCompositeOp:
    {
      if (fabs((double) Da) < MagickEpsilon)
        {
          (*pixel)=(double) QuantumRange*gamma*Sca;
          break;
        }
      blend=GetPegtopLightBlend(Sca,Da,Dca);
      (*pixel)=(double) QuantumRange*gamma*(RoundToUnity(blend)*Da*Sa+Dca*
        (1.0-Sa));
      break;
    }
    case PinLightCompositeOp:
    {
      /*
        Adobe Pin Light (colors in [0,1]):

          if (Cs <= 0.5)
            f = min(Cd, 2*Cs);
          else
            f = max(Cd, 2*Cs - 1);
      */
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      if (S <= 0.5)
        blend=MagickMin(D,2.0*S);
      else
        blend=MagickMax(D,2.0*S-1.0);
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(Sa*Da*blend+Sca*
        (1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case PlusCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=(double) QuantumRange*(Sc+Dc);
          break;
        }
      (*pixel)=(double) QuantumRange*(Sca+Dca);
      break;
    }
    case ReflectCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          blend=GetReflectBlend(Sc,Dc);
          (*pixel)=(double) QuantumRange*RoundToUnity(blend);
          break;
        }
      if (Sa > 0.0)
        S=Sca/Sa;
      else
        S=0.0;
      if (Da > 0.0)
        D=Dca/Da;
      else
        D=0.0;
      if (D < 1.0)
        blend=(S*S)/(1.0-D);
      else
        blend=1.0;
      (*pixel)=(double) QuantumRange*RoundToUnity((Sa+Da-Sa*Da)*blend);
      break;
    }
    case RMSECompositeOp:
    {
      double
        gray;

      if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Dc;
          break;
        }
      if (fabs((double) QuantumRange*Da-(double) TransparentAlpha) < MagickEpsilon)
        {
          (*pixel)=Sc;
          break;
        }
      gray=sqrt(
        ((*canvas_pixel).red-(*source_pixel).red)*
        ((*canvas_pixel).red-(*source_pixel).red)+
        ((*canvas_pixel).green-(*source_pixel).green)*
        ((*canvas_pixel).green-(*source_pixel).green)+
        ((*canvas_pixel).blue-(*source_pixel).blue)*
        ((*canvas_pixel).blue-(*source_pixel).blue)/3.0);
      (*pixel)=GetCompositeChannelColor(tile,gray,gray,gray);
      break;
    }
    default:
    {
      CompositeChannelColorSaturate(row,authentic,tile,pixel);
      break;
    }
  }
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
