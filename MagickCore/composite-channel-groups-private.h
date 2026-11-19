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

  MagickCore private composite channel color methods: the blend helpers, the
  row, pixel and channel input structs, and the selections that
  CompositeChannelColor in composite-channel-private.h reaches through its
  default group. composite-image.c includes this header only, directly
  before composite-channel-private.h.
*/
#ifndef MAGICKCORE_COMPOSITE_CHANNEL_GROUPS_PRIVATE_H
#define MAGICKCORE_COMPOSITE_CHANNEL_GROUPS_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static MagickRealType GetReflectBlend(const MagickRealType Sc,
  const MagickRealType Dc)
{
  MagickRealType
    blend;

  if (Dc < 1.0)
    blend=(Sc*Sc)/(1.0-Dc);
  else
    blend=1.0;
  return(blend);
}

static MagickRealType GetChannelQuotient(const MagickRealType dividend,
  const MagickRealType divisor)
{
  MagickRealType
    pixel;

  if (fabs(divisor) < MagickEpsilon)
    pixel=(double) QuantumRange;
  else
    pixel=(double) QuantumRange*(dividend/divisor);
  return(pixel);
}

static double GetHardMixBurn(const MagickRealType S,const MagickRealType D)
{
  double
    vivid;

  if (S <= MagickEpsilon)
    vivid=0.0;
  else
    vivid=1.0-(1.0-D)/(2.0*S);
  return(vivid);
}

static double GetHardMixVivid(const MagickRealType S,
  const MagickRealType D)
{
  double
    vivid;

  if (S <= 0.5)
    vivid=GetHardMixBurn(S,D);
  else
    {
      if ((1.0-(2.0*S-1.0)) <= MagickEpsilon)
        vivid=1.0;
      else
        vivid=D/(1.0-(2.0*S-1.0));
    }
  return(vivid);
}

typedef struct _CompositeRowInputs
{
  Image
    *image,
    *source_image;

  CompositeOperator
    compose;

  MagickBooleanType
    clip_to_self;

  ssize_t
    x_offset,
    y_offset,
    y;

  ExceptionInfo
    *exception;

  const Quantum
    *pixels;

  MagickRealType
    amount,
    canvas_dissolve,
    midpoint,
    percent_luma,
    percent_chroma,
    source_dissolve,
    threshold;

  MagickBooleanType
    clamp,
    compose_sync;

  ColorspaceType
    colorspace;

  double
    white_luminance;

  IlluminantType
    illuminant;

  GeometryInfo
    geometry_info;

  MagickBooleanType
    union_coverage,
    canvas_alpha,
    source_alpha,
    pixel_info_inputs,
    intensity_inputs;
} CompositeRowInputs;

typedef struct _CompositePixelInputs
{
  const Quantum
    *p;

  MagickRealType
    alpha,
    Da,
    Di,
    Sa,
    Si;

  const PixelInfo
    *canvas_pixel,
    *source_pixel;
} CompositePixelInputs;

typedef struct _CompositeChannelInputs
{
  PixelChannel
    channel;

  MagickRealType
    Dc,
    Dca,
    Sc,
    Sca;

  double
    gamma;
} CompositeChannelInputs;

static MagickRealType GetCompositeChannelColor(
  const CompositeChannelInputs *tile,const MagickRealType red,
  const MagickRealType green,const MagickRealType blue)
{
  const PixelChannel
    channel = tile->channel;

  const MagickRealType
    Dc = tile->Dc;

  MagickRealType
    pixel;

  switch (channel)
  {
    case RedPixelChannel: pixel=red; break;
    case GreenPixelChannel: pixel=green; break;
    case BluePixelChannel: pixel=blue; break;
    default: pixel=Dc; break;
  }
  return(pixel);
}

static void CompositeChannelColorVividLightToXor(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,const CompositeChannelInputs *tile,
  MagickRealType *pixel)
{
  const CompositeOperator
    compose = row->compose;

  const MagickRealType
    Da = authentic->Da,
    Sa = authentic->Sa;

  const MagickRealType
    Dca = tile->Dca,
    Sc = tile->Sc,
    Sca = tile->Sca;

  const double
    gamma = tile->gamma;

  MagickRealType
    blend = 0.0;

  switch (compose)
  {
    case VividLightCompositeOp:
    {
      if ((fabs((double) Sa) < MagickEpsilon) ||
          (fabs((double) Da) < MagickEpsilon))
        {
          (*pixel)=(double) QuantumRange*gamma*(Sa*Da+Sca*(1.0-Da)+Dca*
            (1.0-Sa));
          break;
        }
      blend=GetVividLightBlend(Sa,Sca,Da,Dca);
      (*pixel)=(double) QuantumRange*gamma*(Sa*Da*RoundToUnity(blend)+Sca*
        (1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    case XorCompositeOp:
    {
      (*pixel)=(double) QuantumRange*(Sca*(1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    default:
    {
      (*pixel)=Sc;
      break;
    }
  }
}

static void CompositeChannelColorStereoToThreshold(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,const CompositeChannelInputs *tile,
  MagickRealType *pixel)
{
  Image
    *const source_image = row->source_image;

  const CompositeOperator
    compose = row->compose;

  const MagickRealType
    amount = row->amount,
    threshold = row->threshold;

  const Quantum
    *const p = authentic->p;

  const MagickRealType
    Da = authentic->Da,
    Sa = authentic->Sa;

  const PixelChannel
    channel = tile->channel;

  const MagickRealType
    Dca = tile->Dca,
    Sca = tile->Sca;

  MagickRealType
    blend = 0.0,
    D = 0.0,
    S = 0.0;

  switch (compose)
  {
    case StereoCompositeOp:
    {
      if (channel == RedPixelChannel)
        (*pixel)=(MagickRealType) GetPixelRed(source_image,p);
      break;
    }
    case ThresholdCompositeOp:
    {
      S=(Sa > 0.0) ? (Sca/Sa) : 0.0;
      D=(Da > 0.0) ? (Dca/Da) : 0.0;
      if (fabs(2.0*(S-D)) < threshold)
        blend=D;
      else
        blend=D+(S-D)*amount;
      (*pixel)=(double) QuantumRange*(Sa*Da*RoundToUnity(blend)+Sca*
        (1.0-Da)+Dca*(1.0-Sa));
      break;
    }
    default:
    {
      CompositeChannelColorVividLightToXor(row,authentic,tile,pixel);
      break;
    }
  }
}

static void CompositeChannelColorSoftBurnToStamp(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,const CompositeChannelInputs *tile,
  MagickRealType *pixel)
{
  const CompositeOperator
    compose = row->compose;

  const MagickRealType
    Da = authentic->Da,
    Sa = authentic->Sa;

  const MagickRealType
    Dca = tile->Dca,
    Sca = tile->Sca;

  const double
    gamma = tile->gamma;

  MagickRealType
    blend = 0.0;

  switch (compose)
  {
    case SoftBurnCompositeOp:
    {
      blend=GetSoftBurnBlend(Sca,Dca);
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(blend);
      break;
    }
    case SoftDodgeCompositeOp:
    {
      blend=GetSoftDodgeBlend(Sca,Dca);
      (*pixel)=(double) QuantumRange*gamma*RoundToUnity(blend);
      break;
    }
    case SoftLightCompositeOp:
    {
      if (RoundToUnity(Sca) <= 0.5)
        {
          (*pixel)=(double) QuantumRange*gamma*(RoundToUnity(Dca)*Sa+Da*
           (RoundToUnity(Dca)-(1.0-2.0*RoundToUnity(Sca))*
           RoundToUnity(Dca)*(1.0-RoundToUnity(Dca)))+RoundToUnity(Sca)*
           (1.0-Da)+RoundToUnity(Dca)*(1.0-Sa));
          break;
        }
      if (RoundToUnity(Dca) > 0.25)
        blend=sqrt(RoundToUnity(Dca));
      else
        blend=((16.0*RoundToUnity(Dca)-12.0)*RoundToUnity(Dca)+4.0)*
          RoundToUnity(Dca);
      (*pixel)=(double) QuantumRange*gamma*(RoundToUnity(Dca)*Sa+Da*
        (RoundToUnity(Dca)+(2.0*RoundToUnity(Sca)-1.0)*
        (RoundToUnity(blend)-RoundToUnity(Dca)))+RoundToUnity(Sca)*
        (1.0-Da)+RoundToUnity(Dca)*(1.0-Sa));
      break;
    }
    case StampCompositeOp:
    {
      (*pixel)=(double) QuantumRange*RoundToUnity(Sca+Dca-1.0);
      break;
    }
    default:
    {
      CompositeChannelColorStereoToThreshold(row,authentic,tile,pixel);
      break;
    }
  }
}

static void CompositeChannelColorScreen(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,const CompositeChannelInputs *tile,
  MagickRealType *pixel)
{
  const CompositeOperator
    compose = row->compose;

  const MagickBooleanType
    compose_sync = row->compose_sync;

  const MagickRealType
    Da = authentic->Da,
    Sa = authentic->Sa;

  const MagickRealType
    Dc = tile->Dc,
    Dca = tile->Dca,
    Sc = tile->Sc,
    Sca = tile->Sca;

  MagickRealType
    blend = 0.0,
    D = 0.0,
    S = 0.0;

  switch (compose)
  {
    case ScreenCompositeOp:
    {
      if (compose_sync == MagickFalse)
        {
          (*pixel)=(double) QuantumRange*RoundToUnity(Sc+Dc-Sc*Dc);
          break;
        }
      S=(Sa > 0.0) ? RoundToUnity(Sca/Sa) : 0.0;
      D=(Da > 0.0) ? RoundToUnity(Dca/Da) : 0.0;
      blend=RoundToUnity(S+D-S*D);
      (*pixel)=(double) QuantumRange*RoundToUnity(Sa*Da*blend+Sca*(1.0-Da)+
        Dca*(1.0-Sa));
      break;
    }
    default:
    {
      CompositeChannelColorSoftBurnToStamp(row,authentic,tile,pixel);
      break;
    }
  }
}

static void CompositeChannelColorSaturate(const CompositeRowInputs *row,
  const CompositePixelInputs *authentic,const CompositeChannelInputs *tile,
  MagickRealType *pixel)
{
  const CompositeOperator
    compose = row->compose;

  const ColorspaceType
    colorspace = row->colorspace;

  const double
    white_luminance = row->white_luminance;

  const IlluminantType
    illuminant = row->illuminant;

  const MagickRealType
    Da = authentic->Da,
    Sa = authentic->Sa;

  const PixelInfo
    *const canvas_pixel = authentic->canvas_pixel,
    *const source_pixel = authentic->source_pixel;

  const MagickRealType
    Dc = tile->Dc,
    Sc = tile->Sc;

  MagickRealType
    blue = 0.0,
    chroma = 0.0,
    green = 0.0,
    hue = 0.0,
    luma = 0.0,
    red = 0.0,
    sans = 0.0;

  switch (compose)
  {
    case SaturateCompositeOp:
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
        white_luminance,illuminant,&sans,&chroma,&sans);
      ConvertGenericToRGB(colorspace,hue,chroma,luma,
        white_luminance,illuminant,&red,&green,&blue);
      (*pixel)=GetCompositeChannelColor(tile,red,green,blue);
      break;
    }
    default:
    {
      CompositeChannelColorScreen(row,authentic,tile,pixel);
      break;
    }
  }
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
