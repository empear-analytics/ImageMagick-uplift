/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%           FFFFF  IIIII  L      EEEEE  N   N   AAA   M   M  EEEEE            %
%           F        I    L      E      NN  N  A   A  MM MM  E                %
%           FFF      I    L      EEE    N N N  AAAAA  M M M  EEE              %
%           F        I    L      E      N  NN  A   A  M   M  E                %
%           F      IIIII  LLLLL  EEEEE  N   N  A   A  M   M  EEEEE            %
%                                                                             %
%                                                                             %
%               MagickCore Image Filename Interpretation Method               %
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
#include "MagickCore/animate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/compress.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/display.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/histogram.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magic.h"
#include "MagickCore/magick.h"
#include "MagickCore/magick-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/module.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/random_.h"
#include "MagickCore/registry.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/timer.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/xwindow-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  I n t e r p r e t I m a g e F i l e n a m e                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InterpretImageFilename() interprets embedded characters in an image filename.
%  The filename length is returned.
%
%  The format of the InterpretImageFilename method is:
%
%      size_t InterpretImageFilename(const ImageInfo *image_info,Image *image,
%        const char *format,int value,char *filename,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image_info: the image info.
%
%    o image: the image.
%
%    o format:  A filename describing the format to use to write the numeric
%      argument. Only the first numeric format identifier is replaced.
%
%    o value:  Numeric value to substitute into format filename.
%
%    o filename:  return the formatted filename in this character buffer.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType IsUnknownFormatSpecifier(const char specifier)
{
  return(((specifier != 'd') && (specifier != 'x') && (specifier != 'o')) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsValidDigitSpecifier(const char *start,
  const size_t length)
{
  size_t
    i = 0;

  if (*start == '0')
    {
      if ((length >= 3) && (start[1] == '0'))
        return(MagickFalse);
      i=1;
    }
  for ( ; i < (length-1); i++)
    if (isdigit((int) ((unsigned char) start[i])) == 0)
      return(MagickFalse);
  return(MagickTrue);
}

static inline MagickBooleanType IsValidFormatSpecifier(const char *start,
  const char *end)
{
  char
    specifier = end[-1];

  size_t
    length = (size_t) (end-start);

  /*
    Is this a valid format specifier?
  */
  if (IsUnknownFormatSpecifier(specifier) != MagickFalse)
    return(MagickFalse);
  if ((length == 1) && (*start == specifier))
    return(MagickTrue);
  if (length >= 2)
    return(IsValidDigitSpecifier(start,length));
  return(MagickFalse);
}

static inline MagickBooleanType IsIntegerSpecifierAt(const char *cursor)
{
  return(((*cursor == 'd') || (*cursor == 'o') || (*cursor == 'x')) ?
    MagickTrue : MagickFalse);
}

static const char *GetFilenameOption(const ImageInfo *image_info,
  Image *image,const char *pattern,ExceptionInfo *exception)
{
  const char
    *option = (const char *) NULL;

  if (image != NULL)
    {
      option=GetImageProperty(image,pattern,exception);
      if (option == (const char *) NULL)
        option=GetImageArtifact(image,pattern);
    }
  if ((option == (const char *) NULL) &&
      (image_info != (ImageInfo *) NULL))
    option=GetImageOption(image_info,pattern);
  return(option);
}

MagickExport size_t InterpretImageFilename(const ImageInfo *image_info,
  Image *image,const char *format,int value,char *filename,
  ExceptionInfo *exception)
{
  char
    *literal,
    *p = filename,
    pattern[MagickPathExtent];

  const char
    *cursor = format;

  assert(format != (const char *) NULL);
  assert(filename != (char *) NULL);
  literal=(char *) GetImageRegistry(StringRegistryType,"filename:literal",
    exception);
  if (IsStringTrue(literal) != MagickFalse)
    {
      if (literal != (char *) NULL)
        literal=DestroyString(literal);
      (void) CopyMagickString(filename,format,MagickPathExtent);
      return(strlen(filename));
    }
  if (literal != (char *) NULL)
    literal=DestroyString(literal);
  while ((*cursor != '\0') && ((p-filename) < ((ssize_t) MagickPathExtent-1)))
  {
    const char
      *specifier_start,
      *start;

    if (*cursor != '%')
      {
        *p++=(*cursor++);
        continue;
      }
    start=cursor++;  /* Skip '%' */
    if (*cursor == '%')
      {
        *p++='%';
        cursor++;
        continue;
      }
    specifier_start=cursor;
    while (isdigit((int) ((unsigned char) *cursor)) != 0)
      cursor++;
    if (IsIntegerSpecifierAt(cursor) != MagickFalse)
      {
        const char
          *specifier_end = cursor+1;

        if (IsValidFormatSpecifier(specifier_start,specifier_end) != MagickFalse)
          {
            char
              format_specifier[MagickPathExtent];

            size_t
              length = (size_t) (cursor-specifier_start),
              pattern_length;

            ssize_t
              count;

            (void) snprintf(format_specifier,sizeof(format_specifier),
              "%%%.*s%c",(int) length,specifier_start,*cursor);
            count=FormatLocaleString(pattern,sizeof(pattern),format_specifier,
              value);
            pattern_length=strlen(pattern);
            if ((count <= 0) || ((size_t) count != pattern_length))
              return(0);
            if ((p-filename+(ssize_t) pattern_length) >= MagickPathExtent)
              return(0);
            (void) CopyMagickString(p,pattern,(size_t) (MagickPathExtent-
              (p-filename)));
            p+=pattern_length;
            cursor++;
            continue;
          }
        else
          {
            /*
              Invalid specifier — treat as literal.
            */
            cursor=start;
            *p++=(*cursor++);
            continue;
          }
      }
    if (*cursor == '[')
      {
        const char
          *end = strchr(cursor,']'),
          *option = (const char *) NULL;

        size_t
          extent,
          option_length;

        if (end == (const char *) NULL)
          continue;
        extent=(size_t) (end-cursor-1);
        if (extent >= sizeof(pattern))
          continue;
        (void) CopyMagickString(pattern,cursor+1,extent+1);
        pattern[extent]='\0';
        option=GetFilenameOption(image_info,image,pattern,exception);
        if (option == (const char *) NULL)
          continue;
        option_length=strlen(option);
        if ((p-filename+(ssize_t) option_length) >= MagickPathExtent)
          return(0);
        (void) CopyMagickString(p,option,(size_t) (MagickPathExtent-
          (p-filename)));
        p+=option_length;
        cursor=end+1;
        continue;
      }
    /*
      Invalid or unsupported specifier — treat as literal.
    */
    cursor=start;
    if ((p-filename+1) >= MagickPathExtent)
      return(0);
    *p++=(*cursor++);
  }
  *p='\0';
  return(strlen(filename));
}
