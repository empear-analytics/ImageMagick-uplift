/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                QQQ   U   U   AAA   N   N  TTTTT  U   U  M   M               %
%               Q   Q  U   U  A   A  NN  N    T    U   U  MM MM               %
%               Q   Q  U   U  AAAAA  N N N    T    U   U  M M M               %
%               Q  QQ  U   U  A   A  N  NN    T    U   U  M   M               %
%                QQQQ   UUU   A   A  N   N    T     UUU   M   M               %
%                                                                             %
%                   IIIII  M   M  PPPP    OOO   RRRR   TTTTT                  %
%                     I    MM MM  P   P  O   O  R   R    T                    %
%                     I    M M M  PPPP   O   O  RRRR     T                    %
%                     I    M   M  P      O   O  R R      T                    %
%                   IIIII  M   M  P       OOO   R  R     T                    %
%                                                                             %
%                 MagickCore Methods to Import Quantum Pixels                 %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                               October 1998                                  %
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
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/property.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/color-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/geometry.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/statistic.h"
#include "MagickCore/stream.h"
#include "MagickCore/string_.h"
#include "MagickCore/utility.h"
#include "MagickCore/quantum-import-private.h"

static inline const unsigned char *GetImportPixels(
  const QuantumInfo *quantum_info,const unsigned char *magick_restrict pixels)
{
  if (pixels == (const unsigned char *) NULL)
    pixels=(const unsigned char *) GetQuantumPixels(quantum_info);
  return(pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I m p o r t Q u a n t u m P i x e l s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImportQuantumPixels() transfers one or more pixel components from a user
%  supplied buffer into the image pixel cache of an image.  The pixels are
%  expected in network byte order.  It returns the number of imported pixels.
%
%  The format of the ImportQuantumPixels method is:
%
%      size_t ImportQuantumPixels(const Image *image,CacheView *image_view,
%        QuantumInfo *quantum_info,const QuantumType quantum_type,
%        const unsigned char *magick_restrict pixels,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o image_view: the image cache view.
%
%    o quantum_info: the quantum info.
%
%    o quantum_type: Declare which pixel components to transfer (red, green,
%      blue, opacity, RGB, or RGBA).
%
%    o pixels:  The pixel components are transferred from this buffer.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport size_t ImportQuantumPixels(const Image *image,
  CacheView *image_view,QuantumInfo *quantum_info,
  const QuantumType quantum_type,const unsigned char *magick_restrict pixels,
  ExceptionInfo *exception)
{
  const unsigned char
    *magick_restrict p;

  MagickSizeType
    number_pixels;

  ssize_t
    x;

  Quantum
    *magick_restrict q;

  size_t
    extent;

  QuantumPixelSpan span;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(quantum_info != (QuantumInfo *) NULL);
  assert(quantum_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  pixels=GetImportPixels(quantum_info,pixels);
  x=0;
  p=pixels;
  GetImportPixelQueue(image,image_view,&number_pixels,&q);
  if (q == (Quantum *) NULL)
    return(0);
  ResetQuantumState(quantum_info);
  extent=GetQuantumExtent(image,quantum_info,quantum_type);
  span.number_pixels=number_pixels;
  span.p=p;
  span.q=q;

  switch (quantum_type)
  {
    case AlphaQuantum:
    {
      ImportAlphaQuantum(image,quantum_info,&span,exception);
      break;
    }
    default:
    {
      ImportIndexFamilyQuantum(image,quantum_info,quantum_type,span,exception);
      break;
    }
  }
  if (IsCbYCrQuantumType(quantum_type) != MagickFalse)
    SwapCbYCrPixels(image,image_view,number_pixels,&x);
  if (quantum_info->alpha_type == AssociatedQuantumAlpha)
    DisassociateQuantumAlpha(image,image_view,number_pixels);
  return(extent);
}
