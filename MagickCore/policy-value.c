/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                  PPPP    OOO   L      IIIII   CCCC  Y   Y                   %
%                  P   P  O   O  L        I    C       Y Y                    %
%                  PPPP   O   O  L        I    C        Y                     %
%                  P      O   O  L        I    C        Y                     %
%                  P       OOO   LLLLL  IIIII   CCCC    Y                     %
%                                                                             %
%                      V   V   AAA   L      U   U  EEEEE                      %
%                      V   V  A   A  L      U   U  E                          %
%                      V   V  AAAAA  L      U   U  EEE                        %
%                       V V   A   A  L      U   U  E                          %
%                        V    A   A  LLLLL   UUU   EEEEE                      %
%                                                                             %
%                                                                             %
%                  MagickCore Security Policy Value Methods                   %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/client.h"
#include "MagickCore/configure.h"
#include "MagickCore/configure-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/linked-list-private.h"
#include "MagickCore/magick-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/policy.h"
#include "MagickCore/policy-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/resource-private.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/stream-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"
#if defined(MAGICKCORE_XML_DELEGATE)
#  include <libxml/parser.h>
#  include <libxml/tree.h>
#endif

static MagickSizeType GetPolicyLimit(const char *value)
{
  MagickSizeType
    limit;

  limit=MagickResourceInfinity;
  if (LocaleCompare("unlimited",value) != 0)
    limit=StringToMagickSizeType(value,100.0);
  return(limit);
}

static MagickBooleanType SetCachePolicyValue(const char *name,
  const char *value)
{
  if (LocaleCompare(name,"memory-map") == 0)
    {
      if (LocaleCompare(value,"anonymous") != 0)
        return(MagickFalse);
      ResetCacheAnonymousMemory();
      ResetStreamAnonymousMemory();
      return(MagickTrue);
    }
  return(MagickFalse);
}

static MagickBooleanType SetResourcePolicyValue(const char *name,
  const char *value)
{
  ssize_t
    type;

  type=ParseCommandOption(MagickResourceOptions,MagickFalse,name);
  if (type >= 0)
    {
      MagickSizeType
        limit;

      limit=GetPolicyLimit(value);
      if ((ResourceType) type == TimeResource)
        limit=(MagickSizeType) ParseMagickTimeToLive(value);
      return(SetMagickResourceLimit((ResourceType) type,limit));
    }
  return(MagickFalse);
}

static MagickBooleanType SetSystemPolicyValue(const char *name,
  const char *value)
{
  if (LocaleCompare(name,"max-memory-request") == 0)
    {
      SetMaxMemoryRequest(GetPolicyLimit(value));
      return(MagickTrue);
    }
  if (LocaleCompare(name,"max-profile-size") == 0)
    {
      SetMaxProfileSize(GetPolicyLimit(value));
      return(MagickTrue);
    }
  if (LocaleCompare(name,"memory-map") == 0)
    {
      if (LocaleCompare(value,"anonymous") != 0)
        return(MagickFalse);
      ResetVirtualAnonymousMemory();
      return(MagickTrue);
    }
  if (LocaleCompare(name,"precision") == 0)
    {
      int
        limit;

      limit=StringToInteger(value);
      SetMagickPrecision(limit);
      return(MagickTrue);
    }
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  S e t M a g i c k S e c u r i t y P o l i c y V a l u e                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetMagickSecurityPolicyValue() sets a value associated with an ImageMagick
%  security policy.  For most policies, the value must be less than any value
%  set by the security policy configuration file (i.e. policy.xml).  It returns
%  MagickFalse if the policy cannot be modified or if the policy does not parse.
%
%  The format of the SetMagickSecurityPolicyValue method is:
%
%      MagickBooleanType SetMagickSecurityPolicyValue(
%        const PolicyDomain domain,const char *name,const char *value,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o domain: the domain of the policy (e.g. system, resource).
%
%    o name: the name of the policy.
%
%    o value: the value to set the policy to.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SetMagickSecurityPolicyValue(
  const PolicyDomain domain,const char *name,const char *value,
  ExceptionInfo *exception)
{
  magick_unreferenced(exception);
  assert(exception != (ExceptionInfo *) NULL);
  if ((name == (const char *) NULL) || (value == (const char *) NULL))
    return(MagickFalse);
  switch (domain)
  {
    case CachePolicyDomain:
      return(SetCachePolicyValue(name,value));
    case ResourcePolicyDomain:
      return(SetResourcePolicyValue(name,value));
    case SystemPolicyDomain:
      return(SetSystemPolicyValue(name,value));
    default:
      break;
  }
  return(MagickFalse);
}
