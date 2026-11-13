/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%           GGGG   EEEEE   OOO   M   M  EEEEE  TTTTT  RRRR   Y   Y            %
%           G      E      O   O  MM MM  E        T    R   R   Y Y             %
%           G  GG  EEE    O   O  M M M  EEE      T    RRRR     Y              %
%           G   G  E      O   O  M   M  E        T    R R      Y              %
%            GGGG  EEEEE   OOO   M   M  EEEEE    T    R  R     Y              %
%                                                                             %
%                                                                             %
%                       MagickCore Geometry Methods                           %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%  Copyright 1999 ImageMagick Studio LLC, a non-profit organization           %
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
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/geometry-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"

/*
  Define declarations.
*/
#define MagickPagesize(name,geometry) { name, sizeof(name)-1, geometry }

/*
  Structure declarations.
*/
typedef struct _PageInfo
{
  const char
    name[12];

  size_t
    extent;

  const char
    geometry[10];
} PageInfo;

static const PageInfo
  Pagesizes[] =
  {
    MagickPagesize("4x6", "288x432"),
    MagickPagesize("5x7", "360x504"),
    MagickPagesize("7x9", "504x648"),
    MagickPagesize("8x10", "576x720"),
    MagickPagesize("9x11", "648x792"),
    MagickPagesize("9x12", "648x864"),
    MagickPagesize("10x13", "720x936"),
    MagickPagesize("10x14", "720x1008"),
    MagickPagesize("11x17", "792x1224"),
    MagickPagesize("4A0", "4768x6741"),
    MagickPagesize("2A0", "3370x4768"),
    MagickPagesize("a0", "2384x3370"),
    MagickPagesize("a10", "74x105"),
    MagickPagesize("a1", "1684x2384"),
    MagickPagesize("a2", "1191x1684"),
    MagickPagesize("a3", "842x1191"),
    MagickPagesize("a4small", "595x842"),
    MagickPagesize("a4", "595x842"),
    MagickPagesize("a5", "420x595"),
    MagickPagesize("a6", "298x420"),
    MagickPagesize("a7", "210x298"),
    MagickPagesize("a8", "147x210"),
    MagickPagesize("a9", "105x147"),
    MagickPagesize("archa", "648x864"),
    MagickPagesize("archb", "864x1296"),
    MagickPagesize("archC", "1296x1728"),
    MagickPagesize("archd", "1728x2592"),
    MagickPagesize("arche", "2592x3456"),
    MagickPagesize("b0", "2920x4127"),
    MagickPagesize("b10", "91x127"),
    MagickPagesize("b1", "2064x2920"),
    MagickPagesize("b2", "1460x2064"),
    MagickPagesize("b3", "1032x1460"),
    MagickPagesize("b4", "729x1032"),
    MagickPagesize("b5", "516x729"),
    MagickPagesize("b6", "363x516"),
    MagickPagesize("b7", "258x363"),
    MagickPagesize("b8", "181x258"),
    MagickPagesize("b9", "127x181"),
    MagickPagesize("c0", "2599x3676"),
    MagickPagesize("c1", "1837x2599"),
    MagickPagesize("c2", "1298x1837"),
    MagickPagesize("c3", "918x1296"),
    MagickPagesize("c4", "649x918"),
    MagickPagesize("c5", "459x649"),
    MagickPagesize("c6", "323x459"),
    MagickPagesize("c7", "230x323"),
    MagickPagesize("csheet", "1224x1584"),
    MagickPagesize("dsheet", "1584x2448"),
    MagickPagesize("esheet", "2448x3168"),
    MagickPagesize("executive", "540x720"),
    MagickPagesize("flsa", "612x936"),
    MagickPagesize("flse", "612x936"),
    MagickPagesize("folio", "612x936"),
    MagickPagesize("halfletter", "396x612"),
    MagickPagesize("isob0", "2835x4008"),
    MagickPagesize("isob10", "88x125"),
    MagickPagesize("isob1", "2004x2835"),
    MagickPagesize("isob2", "1417x2004"),
    MagickPagesize("isob3", "1001x1417"),
    MagickPagesize("isob4", "709x1001"),
    MagickPagesize("isob5", "499x709"),
    MagickPagesize("isob6", "354x499"),
    MagickPagesize("isob7", "249x354"),
    MagickPagesize("isob8", "176x249"),
    MagickPagesize("isob9", "125x176"),
    MagickPagesize("jisb0", "1030x1456"),
    MagickPagesize("jisb1", "728x1030"),
    MagickPagesize("jisb2", "515x728"),
    MagickPagesize("jisb3", "364x515"),
    MagickPagesize("jisb4", "257x364"),
    MagickPagesize("jisb5", "182x257"),
    MagickPagesize("jisb6", "128x182"),
    MagickPagesize("ledger", "1224x792"),
    MagickPagesize("legal", "612x1008"),
    MagickPagesize("lettersmall", "612x792"),
    MagickPagesize("letter", "612x792"),
    MagickPagesize("monarch", "279x540"),
    MagickPagesize("quarto", "610x780"),
    MagickPagesize("statement", "396x612"),
    MagickPagesize("tabloid", "792x1224"),
    MagickPagesize("", "")
  };


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t P a g e G e o m e t r y                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPageGeometry() replaces any page mnemonic with the equivalent size in
%  picas.
%
%  The format of the GetPageGeometry method is:
%
%      char *GetPageGeometry(const char *page_geometry)
%
%  A description of each parameter follows.
%
%   o  page_geometry:  Specifies a pointer to an array of characters.  The
%      string is either a Postscript page name (e.g. A4) or a postscript page
%      geometry (e.g. 612x792+36+36).
%
*/
static void ExpandPageMnemonic(char *page,const char *page_geometry,
  const ssize_t i)
{
  MagickStatusType
    flags;

  RectangleInfo
    geometry;

  /*
    Replace mnemonic with the equivalent size in dots-per-inch.
  */
  (void) FormatLocaleString(page,MagickPathExtent,"%s%.80s",
    Pagesizes[i].geometry,page_geometry+Pagesizes[i].extent);
  flags=GetGeometry(page,&geometry.x,&geometry.y,&geometry.width,
    &geometry.height);
  if ((flags & GreaterValue) == 0)
    (void) ConcatenateMagickString(page,">",MagickPathExtent);
}

MagickExport char *GetPageGeometry(const char *page_geometry)
{
  char
    page[MagickPathExtent];

  ssize_t
    i;

  assert(page_geometry != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",page_geometry);
  (void) CopyMagickString(page,page_geometry,MagickPathExtent);
  for (i=0; *Pagesizes[i].name != '\0'; i++)
  {
    int
      status;

    if (Pagesizes[i].extent == 0)
      break;  /* sentinel */
    status=LocaleNCompare(Pagesizes[i].name,page_geometry,Pagesizes[i].extent);
    if (status == 0)
      {
        ExpandPageMnemonic(page,page_geometry,i);
        break;
      }
  }
  return(AcquireString(page));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     I s G e o m e t r y                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsGeometry() returns MagickTrue if the geometry specification is valid.
%  Examples are 100, 100x200, x200, 100x200+10+20, +10+20, 200%, 200x200!, etc.
%
%  The format of the IsGeometry method is:
%
%      MagickBooleanType IsGeometry(const char *geometry)
%
%  A description of each parameter follows:
%
%    o geometry: This string is the geometry specification.
%
*/
MagickExport MagickBooleanType IsGeometry(const char *geometry)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  if (geometry == (const char *) NULL)
    return(MagickFalse);
  flags=ParseGeometry(geometry,&geometry_info);
  return(flags != NoValue ? MagickTrue : MagickFalse);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  L i s t P a g e s i z e s                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListPagesizes() lists the pagesizes and their associated geometry.
%
%  The format of the ListPagesizes method is:
%
%      MagickBooleanType ListPagesizes(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to the output FILE.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ListPagesizes(FILE *file,
  ExceptionInfo *magick_unused(exception))
{
#define MaxMagickSpaces  ((int) sizeof(Pagesizes[0].name))

  const char
    *spacer = "                    ";

  ssize_t
    i;

  magick_unreferenced(exception);
  if (file == (FILE *) NULL)
    file=stdout;
  (void) FormatLocaleFile(file,"\nPagesize    Geometry \n");
  (void) FormatLocaleFile(file,"---------------------\n");
  for (i=0; *Pagesizes[i].name != '\0'; i++)
    (void) FormatLocaleFile(file,"%s%.*s%s\n",Pagesizes[i].name,
      MaxMagickSpaces-(int) Pagesizes[i].extent,spacer,Pagesizes[i].geometry);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e A b s o l u t e G e o m e t r y                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseAbsoluteGeometry() returns a region as defined by the geometry string,
%  without any modification by percentages or gravity.
%
%  It currently just a wrapper around GetGeometry(), but may be expanded in
%  the future to handle other positioning information.
%
%  The format of the ParseAbsoluteGeometry method is:
%
%      MagickStatusType ParseAbsoluteGeometry(const char *geometry,
%        RectangleInfo *region_info)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry string (e.g. "100x100+10+10").
%
%    o region_info: the region as defined by the geometry string.
%
*/
MagickExport MagickStatusType ParseAbsoluteGeometry(const char *geometry,
  RectangleInfo *region_info)
{
  MagickStatusType
    flags;

  flags=GetGeometry(geometry,&region_info->x,&region_info->y,
    &region_info->width,&region_info->height);
  return(flags);
}





/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e P a g e G e o m e t r y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParsePageGeometry() returns a region as defined by the geometry string with
%  respect to the image page (canvas) dimensions.
%
%  WARNING: Percentage dimensions remain relative to the actual image
%  dimensions, and not canvas dimensions.
%
%  The format of the ParsePageGeometry method is:
%
%      MagickStatusType ParsePageGeometry(const Image *image,
%        const char *geometry,RectangleInfo *region_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry string (e.g. "100x100+10+10").
%
%    o region_info: the region as defined by the geometry string with
%      respect to the image and its gravity.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsPageSizeScaled(const MagickStatusType flags)
{
  return(((((flags & WidthValue) != 0) || ((flags & HeightValue) != 0)) &&
    (((flags & PercentValue) != 0) || ((flags & SeparatorValue) == 0))) ?
    MagickTrue : MagickFalse);
}

MagickExport MagickStatusType ParsePageGeometry(const Image *image,
  const char *geometry,RectangleInfo *region_info,ExceptionInfo *exception)
{
  MagickStatusType
    flags;

  SetGeometry(image,region_info);
  if (image->page.width != 0)
    region_info->width=image->page.width;
  if (image->page.height != 0)
    region_info->height=image->page.height;
  flags=ParseAbsoluteGeometry(geometry,region_info);
  if (flags == NoValue)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidGeometry","`%s'",geometry);
      return(flags);
    }
  if ((flags & PercentValue) != 0)
    {
      region_info->width=image->columns;
      region_info->height=image->rows;
    }
  flags=ParseMetaGeometry(geometry,&region_info->x,&region_info->y,
    &region_info->width,&region_info->height);
  if (IsPageSizeScaled(flags) != MagickFalse)
    {
      if ((flags & WidthValue) == 0)
        region_info->width=region_info->height;
      if ((flags & HeightValue) == 0)
        region_info->height=region_info->width;
    }
  return(flags);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e R e g i o n G e o m e t r y                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseRegionGeometry() returns a region as defined by the geometry string
%  with respect to the image dimensions and aspect ratio.
%
%  This is basically a wrapper around ParseMetaGeometry.  This is typically
%  used to parse a geometry string to work out the final integer dimensions
%  for image resizing.
%
%  The format of the ParseRegionGeometry method is:
%
%      MagickStatusType ParseRegionGeometry(const Image *image,
%        const char *geometry,RectangleInfo *region_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry string (e.g. "100x100+10+10").
%
%    o region_info: the region as defined by the geometry string.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickStatusType ParseRegionGeometry(const Image *image,
  const char *geometry,RectangleInfo *region_info,ExceptionInfo *exception)
{
  MagickStatusType
    flags;

  SetGeometry(image,region_info);
  flags=ParseMetaGeometry(geometry,&region_info->x,&region_info->y,
    &region_info->width,&region_info->height);
  if (flags == NoValue)
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
      "InvalidGeometry","`%s'",geometry);
  return(flags);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t G e o m e t r y                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetGeometry() sets the geometry to its default values.
%
%  The format of the SetGeometry method is:
%
%      SetGeometry(const Image *image,RectangleInfo *geometry)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o geometry: the geometry.
%
*/
MagickExport void SetGeometry(const Image *image,RectangleInfo *geometry)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(geometry != (RectangleInfo *) NULL);
  (void) memset(geometry,0,sizeof(*geometry));
  geometry->width=image->columns;
  geometry->height=image->rows;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t G e o m e t r y I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetGeometryInfo sets the GeometryInfo structure to its default values.
%
%  The format of the SetGeometryInfo method is:
%
%      SetGeometryInfo(GeometryInfo *geometry_info)
%
%  A description of each parameter follows:
%
%    o geometry_info: the geometry info structure.
%
*/
MagickExport void SetGeometryInfo(GeometryInfo *geometry_info)
{
  assert(geometry_info != (GeometryInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  (void) memset(geometry_info,0,sizeof(*geometry_info));
}
