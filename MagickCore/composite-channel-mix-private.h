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

  MagickCore private composite channel color methods: the Reflect blend, the
  channel quotient and the HardMix burn and vivid helpers of
  CompositeChannelColor and its groups. composite-image.c includes this
  header only, directly before composite-channel-groups-private.h.
*/
#ifndef MAGICKCORE_COMPOSITE_CHANNEL_MIX_PRIVATE_H
#define MAGICKCORE_COMPOSITE_CHANNEL_MIX_PRIVATE_H

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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
