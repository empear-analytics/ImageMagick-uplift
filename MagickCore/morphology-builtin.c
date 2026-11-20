/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%    M   M   OOO   RRRR   PPPP   H   H   OOO   L       OOO    GGGG  Y   Y     %
%    MM MM  O   O  R   R  P   P  H   H  O   O  L      O   O  G       Y Y      %
%    M M M  O   O  RRRR   PPPP   HHHHH  O   O  L      O   O  G  GG    Y       %
%    M   M  O   O  R R    P      H   H  O   O  L      O   O  G   G    Y       %
%    M   M   OOO   R  R   P      H   H   OOO   LLLLL   OOO    GGG     Y       %
%                                                                             %
%               BBBB   U   U  IIIII  L      TTTTT  IIIII  N   N               %
%               B   B  U   U    I    L        T      I    NN  N               %
%               BBBB   U   U    I    L        T      I    N N N               %
%               B   B  U   U    I    L        T      I    N  NN               %
%               BBBB    UUU   IIIII  LLLLL    T    IIIII  N   N               %
%                                                                             %
%                                                                             %
%                MagickCore Built-In Morphology Kernel Methods                %
%                                                                             %
%                              Software Design                                %
%                              Anthony Thyssen                                %
%                               January 2010                                  %
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
% Morphology is the application of various kernels, of any size or shape, to an
% image in various ways (typically binary, but not always).
%
% Convolution (weighted sum or average) is just one specific type of
% morphology. Just one that is very common for image blurring and sharpening
% effects.  Not only 2D Gaussian blurring, but also 2-pass 1D Blurring.
%
% This module provides not only a general morphology function, and the ability
% to apply more advanced or iterative morphologies, but also functions for the
% generation of many different types of kernel arrays from user supplied
% arguments. Prehaps even the generation of a kernel from a small image.
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/morphology.h"
#include "MagickCore/morphology-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/prepress.h"
#include "MagickCore/quantize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/registry.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/morphology-apply-private.h"
#include "MagickCore/morphology-apply-kernel-private.h"
#include "MagickCore/morphology-builtin-shape-private.h"
#include "MagickCore/morphology-builtin-type-private.h"
#include "MagickCore/morphology-builtin-base-private.h"

static void InitializeBaseKernel(KernelInfo *kernel,const KernelInfoType type)
{
  (void) memset(kernel,0,sizeof(*kernel));
  kernel->minimum = kernel->maximum = kernel->angle = 0.0;
  kernel->negative_range = kernel->positive_range = 0.0;
  kernel->type = type;
  kernel->next = (KernelInfo *) NULL;
  kernel->signature=MagickCoreSignature;
}

MagickExport KernelInfo *AcquireKernelBuiltIn(const KernelInfoType type,
  const GeometryInfo *args,ExceptionInfo *exception)
{
  BuiltInKernelInputs
    builtin;

  KernelInfo
    *kernel;

  double
    nan = sqrt(-1.0);  /* Special Value : Not A Number */

  /* Generate a new empty kernel if needed */
  kernel=(KernelInfo *) NULL;
  switch(type) {
    case UndefinedKernel:    /* These should not call this function */
    case UserDefinedKernel:
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "InvalidOption","`%s'","Should not call this function");
      return((KernelInfo *) NULL);
    default:
      break;
  }
  if (IsBaseKernelNeeded(type) != MagickFalse)
    {
      /* Generate the base Kernel Structure */
      kernel=(KernelInfo *) AcquireMagickMemory(sizeof(*kernel));
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      InitializeBaseKernel(kernel,type);
    }

  builtin.args=args;
  builtin.nan=nan;
  builtin.exception=exception;
  return(AcquireKernelOfType(kernel,type,&builtin));
}
