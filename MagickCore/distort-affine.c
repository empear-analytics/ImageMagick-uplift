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
%                   AAA   FFFFF  FFFFF  IIIII  N   N  EEEEE                   %
%                  A   A  F      F        I    NN  N  E                       %
%                  AAAAA  FFF    FFF      I    N N N  EEE                     %
%                  A   A  F      F        I    N  NN  E                       %
%                  A   A  F      F      IIIII  N   N  EEEEE                   %
%                                                                             %
%                                                                             %
%                      MagickCore Affine Distort Methods                      %
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
%     A f f i n e T r a n s f o r m I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AffineTransformImage() transforms an image as dictated by the affine matrix.
%  It allocates the memory necessary for the new Image structure and returns
%  a pointer to the new image.
%
%  The format of the AffineTransformImage method is:
%
%      Image *AffineTransformImage(const Image *image,
%        AffineMatrix *affine_matrix,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o affine_matrix: the affine matrix.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *AffineTransformImage(const Image *image,
  const AffineMatrix *affine_matrix,ExceptionInfo *exception)
{
  double
    distort[6];

  Image
    *deskew_image;

  /*
    Affine transform image.
  */
  assert(image->signature == MagickCoreSignature);
  assert(affine_matrix != (AffineMatrix *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  distort[0]=affine_matrix->sx;
  distort[1]=affine_matrix->rx;
  distort[2]=affine_matrix->ry;
  distort[3]=affine_matrix->sy;
  distort[4]=affine_matrix->tx;
  distort[5]=affine_matrix->ty;
  deskew_image=DistortImage(image,AffineProjectionDistortion,6,distort,
    MagickTrue,exception);
  return(deskew_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R o t a t e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RotateImage() creates a new image that is a rotated copy of an existing
%  one.  Positive angles rotate counter-clockwise (right-hand rule), while
%  negative angles rotate clockwise.  Rotated images are usually larger than
%  the originals and have 'empty' triangular corners.  X axis.  Empty
%  triangles left over from shearing the image are filled with the background
%  color defined by member 'background_color' of the image.  RotateImage
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the RotateImage method is:
%
%      Image *RotateImage(const Image *image,const double degrees,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o degrees: Specifies the number of degrees to rotate the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *RotateImage(const Image *image,const double degrees,
  ExceptionInfo *exception)
{
  Image
    *distort_image,
    *rotate_image;

  double
    angle;

  PointInfo
    shear;

  size_t
    rotations;

  /*
    Adjust rotation angle.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  angle=fmod(degrees,360.0);
  while (angle < -45.0)
    angle+=360.0;
  for (rotations=0; angle > 45.0; rotations++)
    angle-=90.0;
  rotations%=4;
  shear.x=(-tan((double) DegreesToRadians(angle)/2.0));
  shear.y=sin((double) DegreesToRadians(angle));
  if ((fabs(shear.x) < MagickEpsilon) && (fabs(shear.y) < MagickEpsilon))
    return(IntegralRotateImage(image,rotations,exception));
  distort_image=CloneImage(image,0,0,MagickTrue,exception);
  if (distort_image == (Image *) NULL)
    return((Image *) NULL);
  (void) SetImageVirtualPixelMethod(distort_image,BackgroundVirtualPixelMethod,
    exception);
  rotate_image=DistortImage(distort_image,ScaleRotateTranslateDistortion,1,
    &degrees,MagickTrue,exception);
  distort_image=DestroyImage(distort_image);
  return(rotate_image);
}
