/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                CCCC  H   H   AAA   N   N  N   N  EEEEE  L                   %
%               C      H   H  A   A  NN  N  NN  N  E      L                   %
%               C      HHHHH  AAAAA  N N N  N N N  EEE    L                   %
%               C      H   H  A   A  N  NN  N  NN  E      L                   %
%                CCCC  H   H  A   A  N   N  N   N  EEEEE  LLLLL               %
%                                                                             %
%                CCCC   OOO   M   M  BBBB   IIIII  N   N  EEEEE               %
%               C      O   O  MM MM  B   B    I    NN  N  E                   %
%               C      O   O  M M M  BBBB     I    N N N  EEE                 %
%               C      O   O  M   M  B   B    I    N  NN  E                   %
%                CCCC   OOO   M   M  BBBB   IIIII  N   N  EEEEE               %
%                                                                             %
%                                                                             %
%                       MagickCore Image Combine Method                       %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                               December 2003                                 %
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
#include "MagickCore/cache-private.h"
#include "MagickCore/channel.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/image.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/version.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o m b i n e I m a g e s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CombineImages() combines one or more images into a single image.  The
%  grayscale value of the pixels of each image in the sequence is assigned in
%  order to the specified channels of the combined image.   The typical
%  ordering would be image 1 => Red, 2 => Green, 3 => Blue, etc.
%
%  The format of the CombineImages method is:
%
%      Image *CombineImages(const Image *images,const ColorspaceType colorspace,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o images: the image sequence.
%
%    o colorspace: the image colorspace.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *CombineImages(const Image *image,
  const ColorspaceType colorspace,ExceptionInfo *exception)
{
#define CombineImageTag  "Combine/Image"

  CacheView
    *combine_view;

  Image
    *combine_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  size_t
    number_channels;

  ssize_t
    y;

  /*
    Ensure the image are the same size.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  combine_image=CloneImage(image,0,0,MagickTrue,exception);
  if (combine_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(combine_image,DirectClass,exception) == MagickFalse)
    {
      combine_image=DestroyImage(combine_image);
      return((Image *) NULL);
    }
  if (colorspace != UndefinedColorspace)
    (void) SetImageColorspace(combine_image,colorspace,exception);
  else
    if (fabs(image->gamma-1.0) <= MagickEpsilon)
      (void) SetImageColorspace(combine_image,RGBColorspace,exception);
    else
      (void) SetImageColorspace(combine_image,sRGBColorspace,exception);
  number_channels=GetImageListLength(image);
  switch (combine_image->colorspace)
  {
    case UndefinedColorspace:
    case sRGBColorspace:
    {
      if (number_channels > 3)
        combine_image->alpha_trait=BlendPixelTrait;
      if (number_channels > 4)
        SetPixelMetaChannels(combine_image,number_channels-4,exception);
      break;
    }
    case LinearGRAYColorspace:
    case GRAYColorspace:
    {
      if (number_channels > 1)
        combine_image->alpha_trait=BlendPixelTrait;
      if (number_channels > 2)
        SetPixelMetaChannels(combine_image,number_channels-2,exception);
      break;
    }
    case CMYKColorspace:
    {
      if (number_channels > 4)
        combine_image->alpha_trait=BlendPixelTrait;
      if (number_channels > 5)
        SetPixelMetaChannels(combine_image,number_channels-5,exception);
      break;
    }
    default:
      break;
  }
  /*
    Combine images.
  */
  status=MagickTrue;
  progress=0;
  combine_view=AcquireAuthenticCacheView(combine_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(combine_image,combine_image,combine_image->rows,4)
#endif
  for (y=0; y < (ssize_t) combine_image->rows; y++)
  {
    CacheView
      *image_view;

    const Image
      *next;

    const Quantum
      *magick_restrict p;

    Quantum
      *pixels,
      *magick_restrict q;

    ssize_t
      i;

    if (status == MagickFalse)
      continue;
    pixels=GetCacheViewAuthenticPixels(combine_view,0,y,combine_image->columns,
      1,exception);
    if (pixels == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    next=image;
    for (i=0; i < (ssize_t) GetPixelChannels(combine_image); i++)
    {
      ssize_t
        x;

      PixelChannel channel = GetPixelChannelChannel(combine_image,i);
      PixelTrait traits = GetPixelChannelTraits(combine_image,channel);
      if (traits == UndefinedPixelTrait)
        continue;
      if (next == (Image *) NULL)
        continue;
      image_view=AcquireVirtualCacheView(next,exception);
      p=GetCacheViewVirtualPixels(image_view,0,y,next->columns,1,exception);
      if (p == (const Quantum *) NULL)
        continue;
      q=pixels;
      for (x=0; x < (ssize_t) combine_image->columns; x++)
      {
        if (x < (ssize_t) next->columns)
          {
            q[i]=(Quantum) GetPixelIntensity(next,p);
            p+=(ptrdiff_t) GetPixelChannels(next);
          }
        q+=(ptrdiff_t) GetPixelChannels(combine_image);
      }
      image_view=DestroyCacheView(image_view);
      next=GetNextImageInList(next);
    }
    if (SyncCacheViewAuthenticPixels(combine_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,CombineImageTag,progress,
          combine_image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  combine_view=DestroyCacheView(combine_view);
  if (status == MagickFalse)
    combine_image=DestroyImage(combine_image);
  else
    combine_image->type=UndefinedType;
  return(combine_image);
}
