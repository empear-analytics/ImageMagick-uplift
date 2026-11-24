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

  MagickCore private image codec methods.
*/
#ifndef MAGICKCORE_CONSTITUTE_CODEC_PRIVATE_H
#define MAGICKCORE_CONSTITUTE_CODEC_PRIVATE_H

#include "MagickCore/exception-private.h"
#include "MagickCore/policy.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline MagickBooleanType IsCoderAuthorized(const char *module,
  const char *coder,const PolicyRights rights,ExceptionInfo *exception)
{
  if (IsRightsAuthorized(CoderPolicyDomain,rights,coder) == MagickFalse)
    ThrowPolicyException(coder,MagickFalse);
  if (IsRightsAuthorized(ModulePolicyDomain,rights,module) == MagickFalse)
    ThrowPolicyException(module,MagickFalse);
  return(MagickTrue);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
