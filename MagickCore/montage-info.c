/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               M   M   OOO   N   N  TTTTT   AAA    GGGG  EEEEE               %
%               MM MM  O   O  NN  N    T    A   A  G      E                   %
%               M M M  O   O  N N N    T    AAAAA  G  GG  EEE                 %
%               M   M  O   O  N  NN    T    A   A  G   G  E                   %
%               M   M   OOO   N   N    T    A   A   GGG   EEEEE               %
%                                                                             %
%                         IIIII  N   N  FFFFF   OOO                           %
%                           I    NN  N  F      O   O                          %
%                           I    N N N  FFF    O   O                          %
%                           I    N  NN  F      O   O                          %
%                         IIIII  N   N  F       OOO                           %
%                                                                             %
%                                                                             %
%                       MagickCore Montage Info Methods                       %
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
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/annotate.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/composite.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/quantize.h"
#include "MagickCore/property.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/visual-effects.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e M o n t a g e I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneMontageInfo() makes a copy of the given montage info structure.  If
%  NULL is specified, a new image info structure is created initialized to
%  default values.
%
%  The format of the CloneMontageInfo method is:
%
%      MontageInfo *CloneMontageInfo(const ImageInfo *image_info,
%        const MontageInfo *montage_info)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o montage_info: the montage info.
%
*/
MagickExport MontageInfo *CloneMontageInfo(const ImageInfo *image_info,
  const MontageInfo *montage_info)
{
  MontageInfo
    *clone_info;

  clone_info=(MontageInfo *) AcquireCriticalMemory(sizeof(*clone_info));
  GetMontageInfo(image_info,clone_info);
  if (montage_info == (MontageInfo *) NULL)
    return(clone_info);
  if (montage_info->geometry != (char *) NULL)
    clone_info->geometry=AcquireString(montage_info->geometry);
  if (montage_info->tile != (char *) NULL)
    clone_info->tile=AcquireString(montage_info->tile);
  if (montage_info->title != (char *) NULL)
    clone_info->title=AcquireString(montage_info->title);
  if (montage_info->frame != (char *) NULL)
    clone_info->frame=AcquireString(montage_info->frame);
  if (montage_info->texture != (char *) NULL)
    clone_info->texture=AcquireString(montage_info->texture);
  if (montage_info->font != (char *) NULL)
    clone_info->font=AcquireString(montage_info->font);
  clone_info->pointsize=montage_info->pointsize;
  clone_info->border_width=montage_info->border_width;
  clone_info->shadow=montage_info->shadow;
  clone_info->fill=montage_info->fill;
  clone_info->stroke=montage_info->stroke;
  clone_info->matte_color=montage_info->matte_color;
  clone_info->background_color=montage_info->background_color;
  clone_info->border_color=montage_info->border_color;
  clone_info->gravity=montage_info->gravity;
  (void) CopyMagickString(clone_info->filename,montage_info->filename,
    MagickPathExtent);
  clone_info->debug=IsEventLogging();
  return(clone_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y M o n t a g e I n f o                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyMontageInfo() deallocates memory associated with montage_info.
%
%  The format of the DestroyMontageInfo method is:
%
%      MontageInfo *DestroyMontageInfo(MontageInfo *montage_info)
%
%  A description of each parameter follows:
%
%    o montage_info: Specifies a pointer to an MontageInfo structure.
%
%
*/
MagickExport MontageInfo *DestroyMontageInfo(MontageInfo *montage_info)
{
  assert(montage_info != (MontageInfo *) NULL);
  assert(montage_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (montage_info->geometry != (char *) NULL)
    montage_info->geometry=(char *)
      RelinquishMagickMemory(montage_info->geometry);
  if (montage_info->tile != (char *) NULL)
    montage_info->tile=DestroyString(montage_info->tile);
  if (montage_info->title != (char *) NULL)
    montage_info->title=DestroyString(montage_info->title);
  if (montage_info->frame != (char *) NULL)
    montage_info->frame=DestroyString(montage_info->frame);
  if (montage_info->texture != (char *) NULL)
    montage_info->texture=(char *) RelinquishMagickMemory(
      montage_info->texture);
  if (montage_info->font != (char *) NULL)
    montage_info->font=DestroyString(montage_info->font);
  montage_info->signature=(~MagickCoreSignature);
  montage_info=(MontageInfo *) RelinquishMagickMemory(montage_info);
  return(montage_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M o n t a g e I n f o                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMontageInfo() initializes montage_info to default values.
%
%  The format of the GetMontageInfo method is:
%
%      void GetMontageInfo(const ImageInfo *image_info,
%        MontageInfo *montage_info)
%
%  A description of each parameter follows:
%
%    o image_info: a structure of type ImageInfo.
%
%    o montage_info: Specifies a pointer to a MontageInfo structure.
%
*/
MagickExport void GetMontageInfo(const ImageInfo *image_info,
  MontageInfo *montage_info)
{
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(montage_info != (MontageInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  (void) memset(montage_info,0,sizeof(*montage_info));
  (void) CopyMagickString(montage_info->filename,image_info->filename,
    MagickPathExtent);
  montage_info->geometry=AcquireString(DefaultTileGeometry);
  if (image_info->font != (char *) NULL)
    montage_info->font=AcquireString(image_info->font);
  montage_info->gravity=CenterGravity;
  montage_info->pointsize=image_info->pointsize;
  montage_info->fill.alpha=(MagickRealType) OpaqueAlpha;
  montage_info->stroke.alpha=(MagickRealType) TransparentAlpha;
  montage_info->matte_color=image_info->matte_color;
  montage_info->background_color=image_info->background_color;
  montage_info->border_color=image_info->border_color;
  montage_info->debug=IsEventLogging();
  montage_info->signature=MagickCoreSignature;
}
