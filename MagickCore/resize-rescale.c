/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  RRRR   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                   %
%                  R   R  E      SS       I       ZZ  E                       %
%                  RRRR   EEE     SSS     I     ZZZ   EEE                     %
%                  R R    E         SS    I    ZZ     E                       %
%                  R  R   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                   %
%                                                                             %
%               RRRR   EEEEE  SSSSS   CCCC   AAA   L      EEEEE               %
%               R   R  E      SS     C      A   A  L      E                   %
%               RRRR   EEE     SSS   C      AAAAA  L      EEE                 %
%               R R    E         SS  C      A   A  L      E                   %
%               R  R   EEEEE  SSSSS   CCCC  A   A  LLLLL  EEEEE               %
%                                                                             %
%                                                                             %
%                      MagickCore Image Rescale Methods                       %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/accelerate-private.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/magick.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/profile-private.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resize-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#if defined(MAGICKCORE_LQR_DELEGATE)
#include <lqr.h>
#endif


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M i n i f y I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MinifyImage() is a convenience method that scales an image proportionally to
%  half its size.
%
%  The format of the MinifyImage method is:
%
%      Image *MinifyImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *MinifyImage(const Image *image,ExceptionInfo *exception)
{
  Image
    *minify_image;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  minify_image=ResizeImage(image,image->columns/2,image->rows/2,SplineFilter,
    exception);
  return(minify_image);
}





/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T h u m b n a i l I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ThumbnailImage() changes the size of an image to the given dimensions and
%  removes any associated profiles.  The goal is to produce small low cost
%  thumbnail images suited for display on the Web.
%
%  The format of the ThumbnailImage method is:
%
%      Image *ThumbnailImage(const Image *image,const size_t columns,
%        const size_t rows,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o columns: the number of columns in the scaled image.
%
%    o rows: the number of rows in the scaled image.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType IsCharInRange(const char low,
  const char *uri,const char high)
{
  return(((low <= *uri) && (*uri <= high)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsURIUnreserved(const char *uri)
{
  return(((IsCharInRange('a',uri,'z') != MagickFalse) ||
    (IsCharInRange('A',uri,'Z') != MagickFalse) ||
    (IsCharInRange('0',uri,'9') != MagickFalse) ||
    (strchr("/-_.~",*uri) != 0)) ? MagickTrue : MagickFalse);
}

static void url_encode(const char *uri,char *encode_uri)
{
  char
    *p;

  const char
    *hex = "0123456789ABCDEF";

  for (p=encode_uri; *uri != '\0'; uri++)
    if (IsURIUnreserved(uri) != MagickFalse)
      *p++=(*uri);
    else
      {
        *p++='%';
        *p++=hex[(*uri >> 4) & 0xF];
        *p++=hex[*uri & 0xF];
      }
  *p='\0';
}

static void SetThumbnailProperties(const Image *image,Image *thumbnail_image,
  ExceptionInfo *exception)
{
  char
    encode_uri[3*MagickPathExtent+1] = "/0";

  const char
    *mime_type;

  struct stat
    attributes;

  (void) DeleteImageProperty(thumbnail_image,"comment");
  url_encode(image->filename,encode_uri);
  if (*image->filename != '/')
    (void) FormatImageProperty(thumbnail_image,"Thumb::URI","./%s",encode_uri);
  else
    (void) FormatImageProperty(thumbnail_image,"Thumb::URI","file://%s",
      encode_uri);
  if (GetPathAttributes(image->filename,&attributes) != MagickFalse )
    (void) FormatImageProperty(thumbnail_image,"Thumb::MTime","%.17g",(double)
      attributes.st_mtime);
  (void) FormatImageProperty(thumbnail_image,"Thumb::Size","%.17g",
    (double) GetBlobSize(image));
  mime_type=GetImageProperty(image,"mime:type",exception);
  if (mime_type != (const char *) NULL)
    (void) SetImageProperty(thumbnail_image,"Thumb::Mimetype",mime_type,
      exception);
  (void) SetImageProperty(thumbnail_image,"software",MagickAuthoritativeURL,
    exception);
  (void) FormatImageProperty(thumbnail_image,"Thumb::Image::Width","%.17g",
    (double) image->magick_columns);
  (void) FormatImageProperty(thumbnail_image,"Thumb::Image::Height","%.17g",
    (double) image->magick_rows);
  (void) FormatImageProperty(thumbnail_image,"Thumb::Document::Pages","%.17g",
    (double) GetImageListLength(image));
}

static void StripThumbnailProfiles(Image *thumbnail_image)
{
  const char
    *name;

  for (name=GetNextImageProfile(thumbnail_image); name != (const char *) NULL; )
  {
    if ((LocaleCompare(name,"icc") != 0) && (LocaleCompare(name,"icm") != 0))
     {
       (void) DeleteImageProfile(thumbnail_image,name);
       ResetImageProfileIterator(thumbnail_image);
     }
    name=GetNextImageProfile(thumbnail_image);
  }
}

static Image *KeepReducedThumbnail(Image *clone_image,Image *thumbnail_image)
{
  if (thumbnail_image != (Image *) NULL)
    {
      clone_image=DestroyImage(clone_image);
      clone_image=thumbnail_image;
    }
  return(clone_image);
}

static inline MagickBooleanType IsReducibleBy(const ssize_t x_factor,
  const ssize_t y_factor,const ssize_t factor)
{
  return(((x_factor > factor) && (y_factor > factor)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsThumbnailResized(const Image *image,
  const size_t columns,const size_t rows)
{
  return(((columns != image->columns) || (rows != image->rows)) ?
    MagickTrue : MagickFalse);
}

MagickExport Image *ThumbnailImage(const Image *image,const size_t columns,
  const size_t rows,ExceptionInfo *exception)
{
#define SampleFactor  5

  Image
    *thumbnail_image;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  thumbnail_image=CloneImage(image,0,0,MagickTrue,exception);
  if (thumbnail_image == (Image *) NULL)
    return(thumbnail_image);
  if (IsThumbnailResized(image,columns,rows) != MagickFalse)
    {
      Image
        *clone_image = thumbnail_image;

      ssize_t
        x_factor,
        y_factor;

      x_factor=(ssize_t) (image->columns*MagickSafeReciprocal((double) 
        columns));
      y_factor=(ssize_t) (image->rows*MagickSafeReciprocal((double) rows));
      if (IsReducibleBy(x_factor,y_factor,4) != MagickFalse)
        {
          thumbnail_image=SampleImage(clone_image,4*columns,4*rows,exception);
          clone_image=KeepReducedThumbnail(clone_image,thumbnail_image);
        }
      if (IsReducibleBy(x_factor,y_factor,2) != MagickFalse)
        {
          thumbnail_image=ResizeImage(clone_image,2*columns,2*rows,BoxFilter,
            exception);
          clone_image=KeepReducedThumbnail(clone_image,thumbnail_image);
        }
      thumbnail_image=ResizeImage(clone_image,columns,rows,image->filter ==
        UndefinedFilter ? LanczosSharpFilter : image->filter,exception);
      clone_image=DestroyImage(clone_image);
      if (thumbnail_image == (Image *) NULL)
        return(thumbnail_image);
    }
  (void) ParseAbsoluteGeometry("0x0+0+0",&thumbnail_image->page);
  thumbnail_image->depth=8;
  thumbnail_image->interlace=NoInterlace;
  /*
    Strip all profiles except color profiles.
  */
  ResetImageProfileIterator(thumbnail_image);
  StripThumbnailProfiles(thumbnail_image);
  SetThumbnailProperties(image,thumbnail_image,exception);
  return(thumbnail_image);
}
