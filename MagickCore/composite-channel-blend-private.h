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

  MagickCore private composite blend methods of CompositeChannelColor, from
  GetColorDodgeBlend to GetPegtopLightBlend; included by composite-image.c
  only, directly before composite-channel-private.h.
*/
#ifndef MAGICKCORE_COMPOSITE_CHANNEL_BLEND_PRIVATE_H
#define MAGICKCORE_COMPOSITE_CHANNEL_BLEND_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static MagickRealType GetColorDodgeBlend(const MagickRealType S,
  const MagickRealType D)
{
  MagickRealType
    blend;

  if (S >= 1.0)
    blend=1.0;
  else
    if (D <= 0.0)
      blend=0.0;
    else
      {
        if ((1.0-S) <= 0.0)
          blend=1.0;
        else
          blend=MagickMin(1.0,D/(1.0-S));
      }
  return(blend);
}

static MagickRealType GetSoftBurnBlend(const MagickRealType Sca,
  const MagickRealType Dca)
{
  MagickRealType
    blend;

  if (RoundToUnity(Dca) <= 0.0)
    blend = 0.0;
  else
    if (RoundToUnity(Sca) >= 1.0)
      blend = 1.0;
    else
      blend=1.0-MagickMin(1.0,(1.0-RoundToUnity(Dca))/
        RoundToUnity(Sca));
  return(blend);
}

static MagickRealType GetSoftDodgeBlend(const MagickRealType Sca,
  const MagickRealType Dca)
{
  MagickRealType
    blend;

  if (RoundToUnity(Sca) <= 0.0)
    blend=RoundToUnity(Dca);
  else
    if (RoundToUnity(Dca) >= 1.0)
      blend=1.0;
    else
      blend=MagickMin(1.0,RoundToUnity(Dca)/(1.0-RoundToUnity(Sca)));
  return(blend);
}

static MagickRealType GetVividLightBlend(const MagickRealType Sa,
  const MagickRealType Sca,const MagickRealType Da,const MagickRealType Dca)
{
  MagickRealType
    blend;

  if (RoundToUnity(Sca/Sa) <= 0.0)
    blend=0.0;
  else
    if (RoundToUnity(Sca/Sa) < 0.5)
      blend=1.0-(1.0-RoundToUnity(Dca/Da))/(2.0*RoundToUnity(Sca/Sa));
  else
    if (RoundToUnity(Sca/Sa) < 1.0)
      blend=RoundToUnity(Dca/Da)/(2.0*(1.0-RoundToUnity(Sca/Sa)));
    else
      blend=1.0;
  return(blend);
}

static MagickRealType GetPegtopLightBlend(const MagickRealType Sca,
  const MagickRealType Da,const MagickRealType Dca)
{
  MagickRealType
    blend;

  if (RoundToUnity(Sca) <= 0.5)
    blend=RoundToUnity(Dca/Da)-(1.0-2.0*RoundToUnity(Sca))*
      RoundToUnity(Dca/Da)*(1.0-RoundToUnity(Dca/Da));
  else
    {
      if (RoundToUnity(Dca/Da) <= 0.25)
        blend=((16.0*RoundToUnity(Dca/Da)-12.0)*RoundToUnity(Dca/Da)+
          4.0)*RoundToUnity(Dca/Da);
      else
        blend=sqrt(RoundToUnity(Dca/Da));
      blend=RoundToUnity(Dca/Da)+(2.0*RoundToUnity(Sca)-1.0)*
        (blend-RoundToUnity(Dca/Da));
    }
  return(blend);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
