/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               DDDD   IIIII  SSSSS  TTTTT   OOO   RRRR   TTTTT               %
%               D   D    I    SS       T    O   O  R   R    T                 %
%               D   D    I     SSS     T    O   O  RRRR     T                 %
%               D   D    I       SS    T    O   O  R R      T                 %
%               DDDD   IIIII  SSSSS    T     OOO   R  R     T                 %
%                                                                             %
%                  RRRR   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                   %
%                  R   R  E      SS       I       ZZ  E                       %
%                  RRRR   EEE     SSS     I     ZZZ   EEE                     %
%                  R R    E         SS    I    ZZ     E                       %
%                  R  R   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                   %
%                                                                             %
%                                                                             %
%                      MagickCore Distort Resize Methods                      %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                              Anthony Thyssen                                %
%                                 June 2007                                   %
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
#include "MagickCore/artifact.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/distort.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/image.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/list.h"
#include "MagickCore/matrix.h"
#include "MagickCore/matrix-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/registry.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/shear.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D i s t o r t R e s i z e I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DistortResizeImage() resize image using the equivalent but slower image
%  distortion operator.  The filter is applied using a EWA cylindrical
%  resampling. But like resize the final image size is limited to whole pixels
%  with no effects by virtual-pixels on the result.
%
%  Note that images containing a transparency channel will be twice as slow to
%  resize as images one without transparency.
%
%  The format of the DistortResizeImage method is:
%
%      Image *DistortResizeImage(const Image *image,const size_t columns,
%        const size_t rows,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o columns: the number of columns in the resized image.
%
%    o rows: the number of rows in the resized image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *DistortResizeAlphaImage(const Image *image,Image *tmp_image,
  const double *distort_args,ExceptionInfo *exception)
{
  /*
    Basically we need to separate Virtual-Pixel alpha in the resized
    image, so only the actual original images alpha channel is used.

    distort alpha channel separately
  */
  Image
    *resize_alpha,
    *resize_image;

  (void) SetImageAlphaChannel(tmp_image,ExtractAlphaChannel,exception);
  (void) SetImageAlphaChannel(tmp_image,OpaqueAlphaChannel,exception);
  resize_alpha=DistortImage(tmp_image,AffineDistortion,12,distort_args,
    MagickTrue,exception),
  tmp_image=DestroyImage(tmp_image);
  if (resize_alpha == (Image *) NULL)
    return((Image *) NULL);

  /* distort the actual image containing alpha + VP alpha */
  tmp_image=CloneImage(image,0,0,MagickTrue,exception);
  if (tmp_image == (Image *) NULL)
    return((Image *) NULL);
  (void) SetImageVirtualPixelMethod(tmp_image,
    TransparentVirtualPixelMethod,exception);
  resize_image=DistortImage(tmp_image,AffineDistortion,12,distort_args,
    MagickTrue,exception),
  tmp_image=DestroyImage(tmp_image);
  if (resize_image == (Image *) NULL)
    {
      resize_alpha=DestroyImage(resize_alpha);
      return((Image *) NULL);
    }
  /* replace resize images alpha with the separately distorted alpha */
  (void) SetImageAlphaChannel(resize_image,OffAlphaChannel,exception);
  (void) SetImageAlphaChannel(resize_alpha,OffAlphaChannel,exception);
  (void) CompositeImage(resize_image,resize_alpha,CopyAlphaCompositeOp,
    MagickTrue,0,0,exception);
  resize_alpha=DestroyImage(resize_alpha);
  resize_image->alpha_trait=image->alpha_trait;
  resize_image->compose=image->compose;
  return(resize_image);
}

static Image *CropDistortResizeImage(Image *resize_image,
  const size_t columns,const size_t rows,ExceptionInfo *exception)
{
  Image
    *tmp_image;

  RectangleInfo
    crop_area;

  /*
    Clean up the results of the Distortion
  */
  crop_area.width=columns;
  crop_area.height=rows;
  crop_area.x=0;
  crop_area.y=0;

  tmp_image=resize_image;
  resize_image=CropImage(tmp_image,&crop_area,exception);
  tmp_image=DestroyImage(tmp_image);
  if (resize_image != (Image *) NULL)
    {
      resize_image->page.width=0;
      resize_image->page.height=0;
    }
  return(resize_image);
}

MagickExport Image *DistortResizeImage(const Image *image,const size_t columns,
  const size_t rows,ExceptionInfo *exception)
{
#define DistortResizeImageTag  "Distort/Image"

  Image
    *resize_image,
    *tmp_image;

  double
    distort_args[12];

  VirtualPixelMethod
    vp_save;

  /*
    Distort resize image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((columns == 0) || (rows == 0))
    return((Image *) NULL);
  /* Do not short-circuit this resize if final image size is unchanged */

  (void) memset(distort_args,0,sizeof(distort_args));
  distort_args[4]=(double) image->columns;
  distort_args[6]=(double) columns;
  distort_args[9]=(double) image->rows;
  distort_args[11]=(double) rows;

  vp_save=GetImageVirtualPixelMethod(image);

  tmp_image=CloneImage(image,0,0,MagickTrue,exception);
  if (tmp_image == (Image *) NULL)
    return((Image *) NULL);
  (void) SetImageVirtualPixelMethod(tmp_image,TransparentVirtualPixelMethod,
    exception);

  if ((image->alpha_trait & BlendPixelTrait) == 0)
    {
      /*
        Image has no alpha channel, so we are free to use it.
      */
      (void) SetImageAlphaChannel(tmp_image,SetAlphaChannel,exception);
      resize_image=DistortImage(tmp_image,AffineDistortion,12,distort_args,
        MagickTrue,exception),
      tmp_image=DestroyImage(tmp_image);
      if (resize_image == (Image *) NULL)
        return((Image *) NULL);
      (void) SetImageAlphaChannel(resize_image,OffAlphaChannel,exception);
    }
  else
    {
      /*
        Image has transparency so handle colors and alpha separately.
      */
      resize_image=DistortResizeAlphaImage(image,tmp_image,distort_args,
        exception);
      if (resize_image == (Image *) NULL)
        return((Image *) NULL);
    }
  (void) SetImageVirtualPixelMethod(resize_image,vp_save,exception);
  return(CropDistortResizeImage(resize_image,columns,rows,exception));
}
