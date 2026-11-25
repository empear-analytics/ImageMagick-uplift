/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%            PPPP    RRRR    OOO   PPPP   EEEEE  RRRR   TTTTT  Y   Y          %
%            P   P   R   R  O   O  P   P  E      R   R    T     Y Y           %
%            PPPP    RRRR   O   O  PPPP   EEE    RRRR     T      Y            %
%            P       R R    O   O  P      E      R R      T      Y            %
%            P       R  R    OOO   P      EEEEE  R  R     T      Y            %
%                                                                             %
%                                                                             %
%                         MagickCore Property Methods                         %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 March 2000                                  %
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
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/compare.h"
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/fx-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/histogram.h"
#include "MagickCore/image.h"
#include "MagickCore/layer.h"
#include "MagickCore/locale-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/montage.h"
#include "MagickCore/option.h"
#include "MagickCore/policy.h"
#include "MagickCore/profile.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/resource_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/signature.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"
#if defined(MAGICKCORE_LCMS_DELEGATE)
#if defined(MAGICKCORE_HAVE_LCMS2_LCMS2_H)
#include <lcms2/lcms2.h>
#elif defined(MAGICKCORE_HAVE_LCMS2_H)
#include "lcms2.h"
#elif defined(MAGICKCORE_HAVE_LCMS_LCMS_H)
#include <lcms/lcms.h>
#else
#include "lcms.h"
#endif
#endif

/*
  Define declarations.
*/
#if defined(MAGICKCORE_LCMS_DELEGATE)
#if defined(LCMS_VERSION) && (LCMS_VERSION < 2000)
#define cmsUInt32Number  DWORD
#endif
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e I m a g e P r o p e r t i e s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneImageProperties() clones all the image properties to another image.
%
%  The format of the CloneImageProperties method is:
%
%      MagickBooleanType CloneImageProperties(Image *image,
%        const Image *clone_image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o clone_image: the clone image.
%
*/

typedef char
  *(*CloneKeyFunc)(const char *),
  *(*CloneValueFunc)(const char *);

static inline void *ClonePropertyKey(void *key)
{
  return((void *) ((CloneKeyFunc) ConstantString)((const char *) key));
}

static inline void *ClonePropertyValue(void *value)
{
  return((void *) ((CloneValueFunc) ConstantString)((const char *) value));
}

MagickExport MagickBooleanType CloneImageProperties(Image *image,
  const Image *clone_image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(clone_image != (const Image *) NULL);
  assert(clone_image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    {
      (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
      (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
        clone_image->filename);
    }
  (void) CopyMagickString(image->filename,clone_image->filename,
    MagickPathExtent);
  (void) CopyMagickString(image->magick_filename,clone_image->magick_filename,
    MagickPathExtent);
  image->compression=clone_image->compression;
  image->quality=clone_image->quality;
  image->depth=clone_image->depth;
  image->matte_color=clone_image->matte_color;
  image->background_color=clone_image->background_color;
  image->border_color=clone_image->border_color;
  image->transparent_color=clone_image->transparent_color;
  image->gamma=clone_image->gamma;
  image->chromaticity=clone_image->chromaticity;
  image->rendering_intent=clone_image->rendering_intent;
  image->black_point_compensation=clone_image->black_point_compensation;
  image->units=clone_image->units;
  image->montage=(char *) NULL;
  image->directory=(char *) NULL;
  (void) CloneString(&image->geometry,clone_image->geometry);
  image->offset=clone_image->offset;
  image->resolution.x=clone_image->resolution.x;
  image->resolution.y=clone_image->resolution.y;
  image->page=clone_image->page;
  image->tile_offset=clone_image->tile_offset;
  image->extract_info=clone_image->extract_info;
  image->filter=clone_image->filter;
  image->fuzz=clone_image->fuzz;
  image->intensity=clone_image->intensity;
  image->interlace=clone_image->interlace;
  image->interpolate=clone_image->interpolate;
  image->endian=clone_image->endian;
  image->gravity=clone_image->gravity;
  image->compose=clone_image->compose;
  image->orientation=clone_image->orientation;
  image->scene=clone_image->scene;
  image->dispose=clone_image->dispose;
  image->delay=clone_image->delay;
  image->ticks_per_second=clone_image->ticks_per_second;
  image->iterations=clone_image->iterations;
  image->total_colors=clone_image->total_colors;
  image->taint=clone_image->taint;
  image->progress_monitor=clone_image->progress_monitor;
  image->client_data=clone_image->client_data;
  image->start_loop=clone_image->start_loop;
  image->error=clone_image->error;
  image->signature=clone_image->signature;
  if (clone_image->properties != (void *) NULL)
    {
      if (image->properties != (void *) NULL)
        DestroyImageProperties(image);
      image->properties=CloneSplayTree((SplayTreeInfo *)
        clone_image->properties,ClonePropertyKey,ClonePropertyValue);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e f i n e I m a g e P r o p e r t y                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DefineImageProperty() associates an assignment string of the form
%  "key=value" with an artifact or options. It is equivalent to
%  SetImageProperty().
%
%  The format of the DefineImageProperty method is:
%
%      MagickBooleanType DefineImageProperty(Image *image,const char *property,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o property: the image property.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType DefineImageProperty(Image *image,
  const char *property,ExceptionInfo *exception)
{
  char
    key[MagickPathExtent],
    value[MagickPathExtent];

  char
    *p;

  assert(image != (Image *) NULL);
  assert(property != (const char *) NULL);
  (void) CopyMagickString(key,property,MagickPathExtent-1);
  for (p=key; *p != '\0'; p++)
    if (*p == '=')
      break;
  *value='\0';
  if (*p == '=')
    (void) CopyMagickString(value,p+1,MagickPathExtent);
  *p='\0';
  return(SetImageProperty(image,key,value,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e l e t e I m a g e P r o p e r t y                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DeleteImageProperty() deletes an image property.
%
%  The format of the DeleteImageProperty method is:
%
%      MagickBooleanType DeleteImageProperty(Image *image,const char *property)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o property: the image property.
%
*/
static void CheckPropertyImage(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
}

MagickExport MagickBooleanType DeleteImageProperty(Image *image,
  const char *property)
{
  CheckPropertyImage(image);
  if (image->properties == (void *) NULL)
    return(MagickFalse);
  return(DeleteNodeFromSplayTree((SplayTreeInfo *) image->properties,property));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y I m a g e P r o p e r t i e s                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyImageProperties() destroys all properties and associated memory
%  attached to the given image.
%
%  The format of the DestroyDefines method is:
%
%      void DestroyImageProperties(Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport void DestroyImageProperties(Image *image)
{
  CheckPropertyImage(image);
  if (image->properties != (void *) NULL)
    image->properties=(void *) DestroySplayTree((SplayTreeInfo *)
      image->properties);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  F o r m a t I m a g e P r o p e r t y                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FormatImageProperty() permits formatted property/value pairs to be saved as
%  an image property.
%
%  The format of the FormatImageProperty method is:
%
%      MagickBooleanType FormatImageProperty(Image *image,const char *property,
%        const char *format,...)
%
%  A description of each parameter follows.
%
%   o  image:  The image.
%
%   o  property:  The attribute property.
%
%   o  format:  A string describing the format to use to write the remaining
%      arguments.
%
*/
MagickExport MagickBooleanType FormatImageProperty(Image *image,
  const char *property,const char *format,...)
{
  char
    value[MagickPathExtent];

  ExceptionInfo
    *exception;

  MagickBooleanType
    status;

  ssize_t
    n;

  va_list
    operands;

  va_start(operands,format);
  n=FormatLocaleStringList(value,MagickPathExtent,format,operands);
  (void) n;
  va_end(operands);
  exception=AcquireExceptionInfo();
  status=SetImageProperty(image,property,value,exception);
  exception=DestroyExceptionInfo(exception);
  return(status);
}

#include "MagickCore/property-iptc-private.h"

#include "MagickCore/property-8bim-private.h"

#include "MagickCore/property-exif-private.h"

#include "MagickCore/property-icc-private.h"

#include "MagickCore/property-xmp-private.h"

static inline MagickBooleanType IsPointCorner(const PointInfo *last,
  const PointInfo *point)
{
  return(((last[1].x == last[2].x) && (last[1].y == last[2].y) &&
    (point[0].x == point[1].x) && (point[0].y == point[1].y)) ? MagickTrue :
    MagickFalse);
}

static void FormatPSClippathClose(char *message,const PointInfo *last,
  const PointInfo *first)
{
  if (IsPointCorner(last,first) != MagickFalse)
    (void) FormatLocaleString(message,MagickPathExtent,
      "  %g %g l z\n",first[1].x,first[1].y);
  else
    if ((last[1].x == last[2].x) && (last[1].y == last[2].y))
      (void) FormatLocaleString(message,MagickPathExtent,
        "  %g %g %g %g v z\n",first[0].x,first[0].y,
        first[1].x,first[1].y);
    else
      if ((first[0].x == first[1].x) && (first[0].y == first[1].y))
        (void) FormatLocaleString(message,MagickPathExtent,
          "  %g %g %g %g y z\n",last[2].x,last[2].y,
          first[1].x,first[1].y);
      else
        (void) FormatLocaleString(message,MagickPathExtent,
          "  %g %g %g %g %g %g c z\n",last[2].x,
          last[2].y,first[0].x,first[0].y,first[1].x,first[1].y);
}

static void FormatPSClippathKnot(char *message,PointInfo *last,
  const PointInfo *point)
{
  ssize_t
    i;

  /*
    Handle special cases when Bezier curves are used to describe
    corners and straight lines.
  */
  if (IsPointCorner(last,point) != MagickFalse)
    (void) FormatLocaleString(message,MagickPathExtent,
      "  %g %g l\n",point[1].x,point[1].y);
  else
    if ((last[1].x == last[2].x) && (last[1].y == last[2].y))
      (void) FormatLocaleString(message,MagickPathExtent,
        "  %g %g %g %g v\n",point[0].x,point[0].y,
        point[1].x,point[1].y);
    else
      if ((point[0].x == point[1].x) && (point[0].y == point[1].y))
        (void) FormatLocaleString(message,MagickPathExtent,
          "  %g %g %g %g y\n",last[2].x,last[2].y,
          point[1].x,point[1].y);
      else
        (void) FormatLocaleString(message,MagickPathExtent,
          "  %g %g %g %g %g %g c\n",last[2].x,
          last[2].y,point[0].x,point[0].y,point[1].x,point[1].y);
  for (i=0; i < 3; i++)
    last[i]=point[i];
}

static void StartPSClippathSubpath(char *message,PointInfo *first,
  PointInfo *last,const PointInfo *point)
{
  ssize_t
    i;

  (void) FormatLocaleString(message,MagickPathExtent,"  %g %g m\n",
    point[1].x,point[1].y);
  for (i=0; i < 3; i++)
  {
    first[i]=point[i];
    last[i]=point[i];
  }
}

static inline void SkipClippathBytes(const unsigned char **blob,
  size_t *length,const ssize_t count)
{
  (*blob)+=count;
  (*length)=(size_t) ((ssize_t) (*length)-MagickMin(count,(ssize_t) (*length)));
}

typedef struct _PSClippathState
{
  const unsigned char
    **blob;

  size_t
    *length;

  char
    **path,
    **message;

  MagickBooleanType
    *in_subpath;

  PointInfo
    *first,
    *last,
    *point;

  ssize_t
    *knot_count;
} PSClippathState;

static void AddPSClippathKnot(const PSClippathState *state)
{
  const unsigned char
    *blob = (*state->blob);

  size_t
    length = (*state->length);

  char
    *path = (*state->path);

  char
    *const message = (*state->message);

  MagickBooleanType
    in_subpath = (*state->in_subpath);

  PointInfo
    *first = state->first,
    *last = state->last,
    *point = state->point;

  ssize_t
    knot_count = (*state->knot_count);

  ssize_t
    i,
    x,
    y;

  /*
    Add sub-path knot
  */
  for (i=0; i < 3; i++)
  {
    y=(ssize_t) ReadPropertyMSBLong(&blob,&length);
    x=(ssize_t) ReadPropertyMSBLong(&blob,&length);
    point[i].x=(double) x/4096.0/4096.0;
    point[i].y=1.0-(double) y/4096.0/4096.0;
  }
  if (in_subpath == MagickFalse)
    StartPSClippathSubpath(message,first,last,point);
  else
    FormatPSClippathKnot(message,last,point);
  (void) ConcatenateString(&path,message);
  in_subpath=MagickTrue;
  knot_count--;
  /*
    Close the subpath if there are no more knots.
  */
  if (knot_count == 0)
    {
      /*
        Same special handling as above except we compare to the
        first point in the path and close the path.
      */
      FormatPSClippathClose(message,last,first);
      (void) ConcatenateString(&path,message);
      in_subpath=MagickFalse;
    }
  (*state->blob)=blob;
  (*state->length)=length;
  (*state->path)=path;
  (*state->in_subpath)=in_subpath;
  (*state->knot_count)=knot_count;
}

static char *TracePSClippath(const unsigned char *blob,size_t length)
{
  char
    *path,
    *message;

  MagickBooleanType
    in_subpath;

  PointInfo
    first[3],
    last[3],
    point[3];

  ssize_t
    knot_count,
    selector;

  const PSClippathState
    state = { &blob, &length, &path, &message, &in_subpath, first, last,
      point, &knot_count };

  path=AcquireString((char *) NULL);
  if (path == (char *) NULL)
    return((char *) NULL);
  message=AcquireString((char *) NULL);
  (void) FormatLocaleString(message,MagickPathExtent,"/ClipImage\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,"{\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,
    "  /c {curveto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,
    "  /l {lineto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,
    "  /m {moveto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,
    "  /v {currentpoint 6 2 roll curveto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,
    "  /y {2 copy curveto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,
    "  /z {closepath} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,"  newpath\n");
  (void) ConcatenateString(&path,message);
  /*
    The clipping path format is defined in "Adobe Photoshop File Formats
    Specification" version 6.0 downloadable from adobe.com.
  */
  (void) memset(point,0,sizeof(point));
  (void) memset(first,0,sizeof(first));
  (void) memset(last,0,sizeof(last));
  knot_count=0;
  in_subpath=MagickFalse;
  while (length > 0)
  {
    selector=(ssize_t) ReadPropertyMSBShort(&blob,&length);
    switch (selector)
    {
      case 0:
      case 3:
      {
        if (knot_count != 0)
          {
            SkipClippathBytes(&blob,&length,24);
            break;
          }
        /*
          Expected subpath length record.
        */
        knot_count=(ssize_t) ReadPropertyMSBShort(&blob,&length);
        SkipClippathBytes(&blob,&length,22);
        break;
      }
      case 1:
      case 2:
      case 4:
      case 5:
      {
        if (knot_count == 0)
          {
            /*
              Unexpected subpath knot
            */
            SkipClippathBytes(&blob,&length,24);
            break;
          }
        AddPSClippathKnot(&state);
        break;
      }
      default:
      {
        SkipClippathBytes(&blob,&length,24);
        break;
      }
    }
  }
  /*
    Returns an empty PS path if the path has no knots.
  */
  (void) FormatLocaleString(message,MagickPathExtent,"  eoclip\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,"} bind def");
  (void) ConcatenateString(&path,message);
  message=DestroyString(message);
  return(path);
}

static inline MagickBooleanType IsBezierCorner(const PointInfo *last,
  const PointInfo *point)
{
  return((((last+1)->x == (last+2)->x) && ((last+1)->y == (last+2)->y) &&
    (point->x == (point+1)->x) && (point->y == (point+1)->y)) ? MagickTrue :
    MagickFalse);
}

static inline void TraceBezierCurve(char *message,PointInfo *last,
  PointInfo *point)
{
  /*
    Handle special cases when Bezier curves are used to describe
    corners and straight lines.
  */
  if (IsBezierCorner(last,point) != MagickFalse)
    (void) FormatLocaleString(message,MagickPathExtent,
      "L %g %g\n",point[1].x,point[1].y);
  else
    (void) FormatLocaleString(message,MagickPathExtent,"C %g %g %g %g %g %g\n",
      (last+2)->x,(last+2)->y,point->x,point->y,(point+1)->x,(point+1)->y);
}

static void CopyClippathKnot(PointInfo *first,PointInfo *last,
  const PointInfo *point)
{
  ssize_t
    i;

  for (i=0; i < 3; i++)
  {
    first[i]=point[i];
    last[i]=point[i];
  }
}

static void TraceSVGClippathKnot(char *message,PointInfo *last,
  PointInfo *point)
{
  ssize_t
    i;

  TraceBezierCurve(message,last,point);
  for (i=0; i < 3; i++)
    last[i]=point[i];
}

static char *TraceSVGClippath(const unsigned char *blob,size_t length,
  const size_t columns,const size_t rows)
{
  char
    *path,
    *message;

  MagickBooleanType
    in_subpath;

  PointInfo
    first[3],
    last[3],
    point[3];

  ssize_t
    i;

  ssize_t
    knot_count,
    selector,
    x,
    y;

  path=AcquireString((char *) NULL);
  if (path == (char *) NULL)
    return((char *) NULL);
  message=AcquireString((char *) NULL);
  (void) FormatLocaleString(message,MagickPathExtent,(
    "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
    "<svg xmlns=\"http://www.w3.org/2000/svg\""
    " width=\"%.17g\" height=\"%.17g\">\n"
    "<g>\n"
    "<path fill-rule=\"evenodd\" style=\"fill:#000000;stroke:#000000;"
    "stroke-width:0;shape-rendering:crispEdges\" d=\"\n"),(double) columns,
    (double) rows);
  (void) ConcatenateString(&path,message);
  (void) memset(point,0,sizeof(point));
  (void) memset(first,0,sizeof(first));
  (void) memset(last,0,sizeof(last));
  knot_count=0;
  in_subpath=MagickFalse;
  while (length != 0)
  {
    selector=(ssize_t) ReadPropertyMSBShort(&blob,&length);
    switch (selector)
    {
      case 0:
      case 3:
      {
        if (knot_count != 0)
          {
            SkipClippathBytes(&blob,&length,24);
            break;
          }
        /*
          Expected subpath length record.
        */
        knot_count=(ssize_t) ReadPropertyMSBShort(&blob,&length);
        SkipClippathBytes(&blob,&length,22);
        break;
      }
      case 1:
      case 2:
      case 4:
      case 5:
      {
        if (knot_count == 0)
          {
            /*
              Unexpected subpath knot.
            */
            SkipClippathBytes(&blob,&length,24);
            break;
          }
        /*
          Add sub-path knot
        */
        for (i=0; i < 3; i++)
        {
          y=(ssize_t) ReadPropertyMSBLong(&blob,&length);
          x=(ssize_t) ReadPropertyMSBLong(&blob,&length);
          point[i].x=(double) x*columns/4096.0/4096.0;
          point[i].y=(double) y*rows/4096.0/4096.0;
        }
        if (in_subpath == MagickFalse)
          {
            (void) FormatLocaleString(message,MagickPathExtent,"M %g %g\n",
              point[1].x,point[1].y);
            CopyClippathKnot(first,last,point);
          }
        else
          TraceSVGClippathKnot(message,last,point);
        (void) ConcatenateString(&path,message);
        in_subpath=MagickTrue;
        knot_count--;
        /*
          Close the subpath if there are no more knots.
        */
        if (knot_count == 0)
          {
            TraceBezierCurve(message,last,first);
            (void) ConcatenateString(&path,message);
            in_subpath=MagickFalse;
          }
        break;
      }
      default:
      {
        SkipClippathBytes(&blob,&length,24);
        break;
      }
    }
  }
  /*
    Return an empty SVG image if the path does not have knots.
  */
  (void) ConcatenateString(&path,"\"/>\n</g>\n</svg>\n");
  message=DestroyString(message);
  return(path);
}

static void GetProfileProperty(const Image *image,const char *property,
  ExceptionInfo *exception)
{
  if (LocaleNCompare("8bim:",property,5) == 0)
    Get8BIMProperty(image,property,exception);
  else if (LocaleNCompare("exif:",property,5) == 0)
    GetEXIFProperty(image,property,exception);
  else if ((LocaleNCompare("icc:",property,4) == 0) ||
           (LocaleNCompare("icm:",property,4) == 0))
    {
#if defined(MAGICKCORE_LCMS_DELEGATE)
      GetICCProperty(image,exception);
#endif
    }
  else if (LocaleNCompare("iptc:",property,5) == 0)
    GetIPTCProperty(image,property,exception);
  else if (LocaleNCompare("xmp:",property,4) == 0)
    GetXMPProperty(image,property);
}

static inline MagickBooleanType IsWildcardProperty(const char *property,
  const size_t property_length)
{
  return(((property_length > 2) && (*(property+(property_length-2)) == ':') &&
    (*(property+(property_length-1)) == '*')) ? MagickTrue : MagickFalse);
}

static MagickBooleanType GetStoredImageProperty(const Image *image,
  const char *property,const MagickBooleanType read_from_properties,
  const char **result)
{
  const char
    *p;

  if (read_from_properties != MagickFalse)
    {
      p=(const char *) NULL;
      if (image->properties != (void *) NULL)
        {
          p=(const char *) GetValueFromSplayTree((SplayTreeInfo *)
            image->properties,property);
          if (p != (const char *) NULL)
            {
              (*result)=p;
              return(MagickTrue);
            }
        }
      if (strchr(property,':') == (char *) NULL)
        {
          (*result)=p;
          return(MagickTrue);
        }
    }
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e P r o p e r t y                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageProperty() gets a value associated with an image property.
%
%  This includes,  profile prefixes, such as "exif:", "iptc:" and "8bim:"
%  It does not handle non-profile prefixes, such as "fx:", "option:", or
%  "artifact:".
%
%  The returned string is stored as a prosperity of the same name for faster
%  lookup later. It should NOT be freed by the caller.
%
%  The format of the GetImageProperty method is:
%
%      const char *GetImageProperty(const Image *image,const char *key,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o key: the key.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport const char *GetImageProperty(const Image *image,
  const char *property,ExceptionInfo *exception)
{
  MagickBooleanType
    read_from_properties;

  const char
    *p,
    *returned;

  size_t
    property_length;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((property == (const char *) NULL) || (*property == '\0'))
    return((const char *) NULL);
  read_from_properties=MagickTrue;
  property_length=strlen(property);
  if (IsWildcardProperty(property,property_length) != MagickFalse)
    read_from_properties=MagickFalse;
  if (GetStoredImageProperty(image,property,read_from_properties,&returned) !=
      MagickFalse)
    return(returned);
  GetProfileProperty(image,property,exception);
  if ((image->properties != (void *) NULL) &&
      (read_from_properties != MagickFalse))
    {
      p=(const char *) GetValueFromSplayTree((SplayTreeInfo *)
        image->properties,property);
      return(p);
    }
  return((const char *) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k P r o p e r t y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickProperty() gets attributes or calculated values that is associated
%  with a fixed known property name, or single letter property. It may be
%  called if no image is defined (IMv7), in which case only global image_info
%  values are available:
%
%    \n   newline
%    \r   carriage return
%    <    less-than character.
%    >    greater-than character.
%    &    ampersand character.
%    %%   a percent sign
%    %b   file size of image read in
%    %c   comment meta-data property
%    %d   directory component of path
%    %e   filename extension or suffix
%    %f   filename (including suffix)
%    %g   layer canvas page geometry   (equivalent to "%Wx%H%X%Y")
%    %h   current image height in pixels
%    %i   image filename (note: becomes output filename for "info:")
%    %k   CALCULATED: number of unique colors
%    %l   label meta-data property
%    %m   image file format (file magic)
%    %n   number of images in current image sequence
%    %o   output filename  (used for delegates)
%    %p   index of image in current image list
%    %q   quantum depth (compile-time constant)
%    %r   image class and colorspace
%    %s   scene number (from input unless re-assigned)
%    %t   filename without directory or extension (suffix)
%    %u   unique temporary filename (used for delegates)
%    %w   current width in pixels
%    %x   x resolution (density)
%    %y   y resolution (density)
%    %z   image depth (as read in unless modified, image save depth)
%    %A   image transparency channel enabled (true/false)
%    %B   file size of image in bytes
%    %C   image compression type
%    %D   image GIF dispose method
%    %G   original image size (%wx%h; before any resizes)
%    %H   page (canvas) height
%    %M   Magick filename (original file exactly as given,  including read mods)
%    %O   page (canvas) offset ( = %X%Y )
%    %P   page (canvas) size ( = %Wx%H )
%    %Q   image compression quality ( 0 = default )
%    %S   ?? scenes ??
%    %T   image time delay (in centi-seconds)
%    %U   image resolution units
%    %W   page (canvas) width
%    %X   page (canvas) x offset (including sign)
%    %Y   page (canvas) y offset (including sign)
%    %Z   unique filename (used for delegates)
%    %@   CALCULATED: trim bounding box (without actually trimming)
%    %#   CALCULATED: 'signature' hash of image values
%
%  This routine only handles specifically known properties.  It does not
%  handle special prefixed properties, profiles, or expressions. Nor does
%  it return any free-form property strings.
%
%  The returned string is stored in a structure somewhere, and should not be
%  directly freed.  If the string was generated (common) the string will be
%  stored as as either as artifact or option 'magick-property'.  These may be
%  deleted (cleaned up) when no longer required, but neither artifact or
%  option is guaranteed to exist.
%
%  The format of the GetMagickProperty method is:
%
%      const char *GetMagickProperty(ImageInfo *image_info,Image *image,
%        const char *property,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info (optional)
%
%    o image: the image (optional)
%
%    o key: the key.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static const char *CloneMagickPropertyValue(ImageInfo *image_info,
  Image *image,const char *value)
{
  if (*value != '\0')
    {
      /*
        Create a cloned copy of result.
      */
      if (image != (Image *) NULL)
        {
          (void) SetImageArtifact(image,"magick-property",value);
          return(GetImageArtifact(image,"magick-property"));
        }
      else
        {
          (void) SetImageOption(image_info,"magick-property",value);
          return(GetImageOption(image_info,"magick-property"));
        }
    }
  return((char *) NULL);
}

#define WarnNoImageReturn(format,arg) \
  if (image == (Image *) NULL ) { \
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning, \
      "NoImageForProperty",format,arg); \
    return((const char *) NULL); \
  }
#define WarnNoImageInfoReturn(format,arg) \
  if (image_info == (ImageInfo *) NULL ) { \
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning, \
      "NoImageInfoForProperty",format,arg); \
    return((const char *) NULL); \
  }

static const char *SelectMagickPropertyValue(ImageInfo *image_info,
  Image *image,const char *string,const char *value)
{
  if (string != (char *) NULL)
    return(string);
  return(CloneMagickPropertyValue(image_info,image,value));
}

static void LogMagickPropertyRequest(const ImageInfo *image_info,
  const Image *image)
{
  if ((image != (Image *) NULL) && (IsEventLogging() != MagickFalse))
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  else
    if ((image_info != (ImageInfo *) NULL) &&
        (IsEventLogging() != MagickFalse))
      (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s","no-images");
}

typedef struct _MagickPropertyRequest
{
  ImageInfo
    *image_info;

  Image
    *image;

  const char
    *property;

  char
    *value;

  ExceptionInfo
    *exception;
} MagickPropertyRequest;

typedef enum
{
  NoPropertyRequirement,
  ImagePropertyRequirement,
  ImageInfoPropertyRequirement
} MagickPropertyRequirement;

typedef const char
  *(*MagickPropertyHandler)(const MagickPropertyRequest *);

typedef struct _MagickPropertyLetterInfo
{
  char
    letter;

  MagickPropertyRequirement
    requirement;

  MagickPropertyHandler
    handler;
} MagickPropertyLetterInfo;

static const char *MagickPropertyLetterLowerB(const MagickPropertyRequest *request)
{
  (void) FormatMagickSize(request->image->extent,MagickFalse,"B",MagickPathExtent,
    request->value);
  if (request->image->extent == 0)
    (void) FormatMagickSize(GetBlobSize(request->image),MagickFalse,"B",
      MagickPathExtent,request->value);
  return((const char *) NULL);
}

static const char *MagickPropertyLetterLowerC(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=GetImageProperty(request->image,"comment",request->exception);
  if ( string == (const char *) NULL )
    string="";
  return(string);
}

static const char *MagickPropertyLetterLowerD(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  GetPathComponent(request->image->magick_filename,HeadPath,request->value);
  if (*request->value == '\0')
    string="";
  return(string);
}

static const char *MagickPropertyLetterLowerE(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  GetPathComponent(request->image->magick_filename,ExtensionPath,request->value);
  if (*request->value == '\0')
    string="";
  return(string);
}

static const char *MagickPropertyLetterLowerF(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  GetPathComponent(request->image->magick_filename,TailPath,request->value);
  if (*request->value == '\0')
    string="";
  return(string);
}

static const char *MagickPropertyLetterLowerG(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,
    "%.17gx%.17g%+.20g%+.20g",(double) request->image->page.width,(double)
    request->image->page.height,(double) request->image->page.x,(double) request->image->page.y);
  return((const char *) NULL);
}

static const char *MagickPropertyLetterLowerH(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    (request->image->rows != 0 ? request->image->rows : request->image->magick_rows));
  return((const char *) NULL);
}

static const char *MagickPropertyInput(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=request->image->filename;
  return(string);
}

static const char *MagickPropertyLetterLowerK(const MagickPropertyRequest *request)
{
  /*
    FUTURE: ensure this does not generate the formatted comment!
  */
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    GetNumberColors(request->image,(FILE *) NULL,request->exception));
  return((const char *) NULL);
}

static const char *MagickPropertyLetterLowerL(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=GetImageProperty(request->image,"label",request->exception);
  if (string == (const char *) NULL)
    string="";
  return(string);
}

static const char *MagickPropertyMagick(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=request->image->magick;
  return(string);
}

static const char *MagickPropertyLetterLowerO(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=request->image_info->filename;
  return(string);
}

static const char *MagickPropertyLetterLowerP(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    GetImageIndexInList(request->image));
  return((const char *) NULL);
}

static const char *MagickPropertyLetterLowerQ(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    MAGICKCORE_QUANTUM_DEPTH);
  return((const char *) NULL);
}

static const char *MagickPropertyLetterLowerR(const MagickPropertyRequest *request)
{
  ColorspaceType
    colorspace;

  colorspace=request->image->colorspace;
  (void) FormatLocaleString(request->value,MagickPathExtent,"%s %s %s",
    CommandOptionToMnemonic(MagickClassOptions,(ssize_t)
    request->image->storage_class),CommandOptionToMnemonic(MagickColorspaceOptions,
    (ssize_t) colorspace),request->image->alpha_trait != UndefinedPixelTrait ?
    "Alpha" : "");
  return((const char *) NULL);
}

static const char *MagickPropertyLetterLowerT(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  GetPathComponent(request->image->magick_filename,BasePath,request->value);
  if (*request->value == '\0')
    string="";
  return(string);
}

static const char *MagickPropertyUnique(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=request->image_info->unique;
  return(string);
}

static const char *MagickPropertyLetterLowerW(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    (request->image->columns != 0 ? request->image->columns : request->image->magick_columns));
  return((const char *) NULL);
}

static const char *MagickPropertyLetterLowerX(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",
    fabs(request->image->resolution.x) > MagickEpsilon ? request->image->resolution.x :
    request->image->units == PixelsPerCentimeterResolution ? DefaultResolution/2.54 :
    DefaultResolution);
  return((const char *) NULL);
}

static const char *MagickPropertyLetterLowerY(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",
    fabs(request->image->resolution.y) > MagickEpsilon ? request->image->resolution.y :
    request->image->units == PixelsPerCentimeterResolution ? DefaultResolution/2.54 :
    DefaultResolution);
  return((const char *) NULL);
}

static const char *MagickPropertyDepth(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    request->image->depth);
  return((const char *) NULL);
}

static const char *MagickPropertyLetterUpperA(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=CommandOptionToMnemonic(MagickPixelTraitOptions,(ssize_t)
    request->image->alpha_trait);
  return(string);
}

static const char *MagickPropertyLetterUpperB(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    request->image->extent);
  if (request->image->extent == 0)
    (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
      GetBlobSize(request->image));
  return((const char *) NULL);
}

static const char *MagickPropertyCompression(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=CommandOptionToMnemonic(MagickCompressOptions,(ssize_t)
    request->image->compression);
  return(string);
}

static const char *MagickPropertyLetterUpperD(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=CommandOptionToMnemonic(MagickDisposeOptions,(ssize_t)
    request->image->dispose);
  return(string);
}

static const char *MagickPropertyLetterUpperG(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17gx%.17g",(double)
    request->image->magick_columns,(double) request->image->magick_rows);
  return((const char *) NULL);
}

static const char *MagickPropertyLetterUpperH(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    request->image->page.height);
  return((const char *) NULL);
}

static const char *MagickPropertyLetterUpperM(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=request->image->magick_filename;
  return(string);
}

static const char *MagickPropertyLetterUpperO(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%+ld%+ld",(long)
    request->image->page.x,(long) request->image->page.y);
  return((const char *) NULL);
}

static const char *MagickPropertyLetterUpperP(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17gx%.17g",(double)
    request->image->page.width,(double) request->image->page.height);
  return((const char *) NULL);
}

static const char *MagickPropertyLetterUpperQ(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    (request->image->quality == 0 ? 92 : request->image->quality));
  return((const char *) NULL);
}

static const char *MagickPropertyLetterUpperT(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    request->image->delay);
  return((const char *) NULL);
}

static const char *MagickPropertyUnits(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=CommandOptionToMnemonic(MagickResolutionOptions,(ssize_t)
    request->image->units);
  return(string);
}

static const char *MagickPropertyLetterUpperW(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    request->image->page.width);
  return((const char *) NULL);
}

static const char *MagickPropertyLetterUpperX(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%+.20g",(double)
    request->image->page.x);
  return((const char *) NULL);
}

static const char *MagickPropertyLetterUpperY(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%+.20g",(double)
    request->image->page.y);
  return((const char *) NULL);
}

static const char *MagickPropertyLetterPercent(const MagickPropertyRequest *magick_unused(request))
{
  const char
    *string = (char *) NULL;

  magick_unreferenced(request);
  string="%";
  return(string);
}

static const char *MagickPropertyLetterAt(const MagickPropertyRequest *request)
{
  RectangleInfo
    page;

  page=GetImageBoundingBox(request->image,request->exception);
  (void) FormatLocaleString(request->value,MagickPathExtent,
    "%.17gx%.17g%+.20g%+.20g",(double) page.width,(double) page.height,
    (double) page.x,(double)page.y);
  return((const char *) NULL);
}

static const char *MagickPropertyLetterHash(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  /*
    Image signature.
  */
  if ((request->image->columns != 0) && (request->image->rows != 0))
    (void) SignatureImage(request->image,request->exception);
  string=GetImageProperty(request->image,"signature",request->exception);
  return(string);
}

static const char *FormatMagickPropertyListLength(
  const MagickPropertyRequest *request,const MagickBooleanType test,
  const char *format,const char *fallback)
{
  const char
    *string = (char *) NULL;

  if (test != MagickFalse)
    (void) FormatLocaleString(request->value,MagickPathExtent,format,(double)
      GetImageListLength(request->image));
  else
    string=fallback;
  return(string);
}

static const char *MagickPropertyLetterLowerN(
  const MagickPropertyRequest *request)
{
  /* "0": no images or scenes */
  return(FormatMagickPropertyListLength(request,(MagickBooleanType)
    (request->image != (Image *) NULL),"%.17g","0"));
}

static const char *MagickPropertyLetterLowerS(
  const MagickPropertyRequest *request)
{
  Image
    *const image = request->image;

  char
    *const value = request->value;

#if 0  /* this seems non-sensical -- simplifying */
      if (image_info->number_scenes != 0)
        (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
          image_info->scene);
      else if (image != (Image *) NULL)
        (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
          image->scene);
      else
          string="0";
#else
  (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
    image->scene);
#endif
  return((const char *) NULL);
}

static const char *MagickPropertyLetterUpperN(
  const MagickPropertyRequest *request)
{
  return(FormatMagickPropertyListLength(request,(MagickBooleanType)
    ((request->image != (Image *) NULL) &&
    (request->image->next == (Image *) NULL)),"%.17g\n",""));
}

static const char *MagickPropertyLetterUpperS(
  const MagickPropertyRequest *request)
{
  ImageInfo
    *const image_info = request->image_info;

  char
    *const value = request->value;

#if 0 /* What is this number? -- it makes no sense - simplifying */
      if (image_info->number_scenes == 0)
         string="2147483647";
      else if ( image != (Image *) NULL )
        (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
                image_info->scene+image_info->number_scenes);
      else
        string="0";
#else
  (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
    (image_info->number_scenes == 0 ? 2147483647 :
     image_info->number_scenes));
#endif
  return((const char *) NULL);
}

static const MagickPropertyLetterInfo
  MagickPropertyLetters[] =
  {
    { 'b', ImagePropertyRequirement, MagickPropertyLetterLowerB },
    { 'c', ImagePropertyRequirement, MagickPropertyLetterLowerC },
    { 'd', ImagePropertyRequirement, MagickPropertyLetterLowerD },
    { 'e', ImagePropertyRequirement, MagickPropertyLetterLowerE },
    { 'f', ImagePropertyRequirement, MagickPropertyLetterLowerF },
    { 'g', ImagePropertyRequirement, MagickPropertyLetterLowerG },
    { 'h', ImagePropertyRequirement, MagickPropertyLetterLowerH },
    { 'i', ImagePropertyRequirement, MagickPropertyInput },
    { 'k', ImagePropertyRequirement, MagickPropertyLetterLowerK },
    { 'l', ImagePropertyRequirement, MagickPropertyLetterLowerL },
    { 'm', ImagePropertyRequirement, MagickPropertyMagick },
    { 'n', NoPropertyRequirement, MagickPropertyLetterLowerN },
    { 'o', ImageInfoPropertyRequirement, MagickPropertyLetterLowerO },
    { 'p', ImagePropertyRequirement, MagickPropertyLetterLowerP },
    { 'q', ImagePropertyRequirement, MagickPropertyLetterLowerQ },
    { 'r', ImagePropertyRequirement, MagickPropertyLetterLowerR },
    { 's', ImagePropertyRequirement, MagickPropertyLetterLowerS },
    { 't', ImagePropertyRequirement, MagickPropertyLetterLowerT },
    { 'u', ImageInfoPropertyRequirement, MagickPropertyUnique },
    { 'w', ImagePropertyRequirement, MagickPropertyLetterLowerW },
    { 'x', ImagePropertyRequirement, MagickPropertyLetterLowerX },
    { 'y', ImagePropertyRequirement, MagickPropertyLetterLowerY },
    { 'z', ImagePropertyRequirement, MagickPropertyDepth },
    { 'A', ImagePropertyRequirement, MagickPropertyLetterUpperA },
    { 'B', ImagePropertyRequirement, MagickPropertyLetterUpperB },
    { 'C', ImagePropertyRequirement, MagickPropertyCompression },
    { 'D', ImagePropertyRequirement, MagickPropertyLetterUpperD },
    { 'G', ImagePropertyRequirement, MagickPropertyLetterUpperG },
    { 'H', ImagePropertyRequirement, MagickPropertyLetterUpperH },
    { 'M', ImagePropertyRequirement, MagickPropertyLetterUpperM },
    { 'N', NoPropertyRequirement, MagickPropertyLetterUpperN },
    { 'O', ImagePropertyRequirement, MagickPropertyLetterUpperO },
    { 'P', ImagePropertyRequirement, MagickPropertyLetterUpperP },
    { 'Q', ImagePropertyRequirement, MagickPropertyLetterUpperQ },
    { 'S', ImageInfoPropertyRequirement, MagickPropertyLetterUpperS },
    { 'T', ImagePropertyRequirement, MagickPropertyLetterUpperT },
    { 'U', ImagePropertyRequirement, MagickPropertyUnits },
    { 'W', ImagePropertyRequirement, MagickPropertyLetterUpperW },
    { 'X', ImagePropertyRequirement, MagickPropertyLetterUpperX },
    { 'Y', ImagePropertyRequirement, MagickPropertyLetterUpperY },
    { '%', NoPropertyRequirement, MagickPropertyLetterPercent },
    { '@', ImagePropertyRequirement, MagickPropertyLetterAt },
    { '#', ImagePropertyRequirement, MagickPropertyLetterHash }
  };

static const MagickPropertyLetterInfo *GetMagickPropertyLetterInfo(
  const char letter)
{
  size_t
    i;

  for (i=0; i < (sizeof(MagickPropertyLetters)/sizeof(*MagickPropertyLetters)); i++)
    if (MagickPropertyLetters[i].letter == letter)
      return(MagickPropertyLetters+i);
  return((const MagickPropertyLetterInfo *) NULL);
}

static const char *GetMagickPropertyLetter(ImageInfo *image_info,
  Image *image,const char letter,ExceptionInfo *exception)
{
  char
    value[MagickPathExtent];  /* formatted string to store as an artifact */

  const char
    *string;     /* return a string already stored somewhere */

  const MagickPropertyLetterInfo
    *info;

  MagickPropertyRequest
    request;

  LogMagickPropertyRequest(image_info,image);
  *value='\0';           /* formatted string */
  string=(char *) NULL;  /* constant string reference */
  request.image_info=image_info;
  request.image=image;
  request.property=(const char *) NULL;
  request.value=value;
  request.exception=exception;
  info=GetMagickPropertyLetterInfo(letter);
  if (info != (const MagickPropertyLetterInfo *) NULL)
    {
      if (info->requirement == ImagePropertyRequirement)
        {
          WarnNoImageReturn("\"%%%c\"",letter);
        }
      if (info->requirement == ImageInfoPropertyRequirement)
        {
          WarnNoImageInfoReturn("\"%%%c\"",letter);
        }
      string=info->handler(&request);
    }
  if (string != (char *) NULL)
    return(string);
  return(CloneMagickPropertyValue(image_info,image,value));
}

static const char *GetPathComponentProperty(const Image *image,
  const PathType type,char *value,const char *string)
{
  GetPathComponent(image->magick_filename,type,value);
  if (*value == '\0')
    string="";
  return(string);
}

static void GetProfilesProperty(const Image *image,char *value)
{
  const char
    *name;

  ResetImageProfileIterator(image);
  name=GetNextImageProfile(image);
  if (name != (char *) NULL)
    {
      (void) CopyMagickString(value,name,MagickPathExtent);
      name=GetNextImageProfile(image);
      while (name != (char *) NULL)
      {
        ConcatenateMagickString(value,",",MagickPathExtent);
        ConcatenateMagickString(value,name,MagickPathExtent);
        name=GetNextImageProfile(image);
      }
    }
}

static MagickBooleanType IsTopHullPoint(const PointInfo *point,
  const PointInfo *extreme)
{
  if (point->y < extreme->y)
    return(MagickTrue);
  if (point->y != extreme->y)
    return(MagickFalse);
  return((MagickBooleanType) (point->x < extreme->x));
}

static MagickBooleanType IsRightHullPoint(const PointInfo *point,
  const PointInfo *extreme)
{
  if (point->x > extreme->x)
    return(MagickTrue);
  if (point->x != extreme->x)
    return(MagickFalse);
  return((MagickBooleanType) (point->y < extreme->y));
}

static MagickBooleanType IsBottomHullPoint(const PointInfo *point,
  const PointInfo *extreme)
{
  if (point->y > extreme->y)
    return(MagickTrue);
  if (point->y != extreme->y)
    return(MagickFalse);
  return((MagickBooleanType) (point->x > extreme->x));
}

static MagickBooleanType IsLeftHullPoint(const PointInfo *point,
  const PointInfo *extreme)
{
  if (point->x < extreme->x)
    return(MagickTrue);
  if (point->x != extreme->x)
    return(MagickFalse);
  return((MagickBooleanType) (point->y > extreme->y));
}

typedef MagickBooleanType
  (*HullPointMethod)(const PointInfo *,const PointInfo *);

static PointInfo GetHullExtremePoint(const PointInfo *convex_hull,
  const size_t number_points,const HullPointMethod method)
{
  PointInfo
    extreme;

  ssize_t
    n;

  extreme=convex_hull[0];
  for (n=0; n < (ssize_t) number_points; n++)
    if (method(convex_hull+n,&extreme) != MagickFalse)
      extreme=convex_hull[n];
  return(extreme);
}

static const char *GetConvexHullExtremePoints(Image *image,char *value,
  const char *string,ExceptionInfo *exception)
{
  char
    *points;

  PointInfo
    extreme,
    *convex_hull;

  size_t
    number_points;

  convex_hull=GetImageConvexHull(image,&number_points,exception);
  if (convex_hull == (PointInfo *) NULL)
    return(string);
  points=AcquireString("");
  extreme=GetHullExtremePoint(convex_hull,number_points,  /* top */
    IsTopHullPoint);
  (void) FormatLocaleString(value,MagickPathExtent,"%g,%g ",
    extreme.x,extreme.y);
  (void) ConcatenateString(&points,value);
  extreme=GetHullExtremePoint(convex_hull,number_points,  /* right */
    IsRightHullPoint);
  (void) FormatLocaleString(value,MagickPathExtent,"%g,%g ",
    extreme.x,extreme.y);
  (void) ConcatenateString(&points,value);
  extreme=GetHullExtremePoint(convex_hull,number_points,  /* bottom */
    IsBottomHullPoint);
  (void) FormatLocaleString(value,MagickPathExtent,"%g,%g ",
    extreme.x,extreme.y);
  (void) ConcatenateString(&points,value);
  extreme=GetHullExtremePoint(convex_hull,number_points,  /* left */
    IsLeftHullPoint);
  (void) FormatLocaleString(value,MagickPathExtent,"%g,%g ",
    extreme.x,extreme.y);
  (void) ConcatenateString(&points,value);
  convex_hull=(PointInfo *) RelinquishMagickMemory(convex_hull);
  (void) SetImageProperty(image,"convex-hull:extreme-points",points,
    exception);
  points=DestroyString(points);
  string=GetImageProperty(image,"convex-hull:extreme-points",exception);
  return(string);
}

typedef enum
{
  ConvexHullPointList,
  MinimumBoundingBoxPointList
} PointListSide;

typedef struct _PointListInputs
{
  Image
    *image;

  char
    *value;

  const char
    *string,
    *property;

  ExceptionInfo
    *exception;
} PointListInputs;

static inline const char *GetPointListProperty(const PointListInputs *inputs,
  const PointListSide side)
{
  char
    *const value = inputs->value;

  const char
    *const property = inputs->property,
    *string = inputs->string;

  ExceptionInfo
    *const exception = inputs->exception;

  Image
    *const image = inputs->image;

  char
    *points;

  PointInfo
    *point_list;

  ssize_t
    n;

  size_t
    number_points;

  point_list=((side == ConvexHullPointList) ? GetImageConvexHull(image,
    &number_points,exception) : GetImageMinimumBoundingBox(image,
    &number_points,exception));
  if (point_list == (PointInfo *) NULL)
    return(string);
  points=AcquireString("");
  for (n=0; n < (ssize_t) number_points; n++)
  {
    (void) FormatLocaleString(value,MagickPathExtent,"%g,%g ",
      point_list[n].x,point_list[n].y);
    (void) ConcatenateString(&points,value);
  }
  point_list=(PointInfo *) RelinquishMagickMemory(point_list);
  (void) SetImageProperty(image,((side == ConvexHullPointList) ?
    "convex-hull" : "minimum-bounding-box"),points,exception);
  points=DestroyString(points);
  string=GetImageProperty(image,((side == ConvexHullPointList) ?
    "convex-hull" : property),exception);
  return(string);
}

static void FormatPaperSizeProperty(const char *property,char *value)
{
  char
    *papersize;

  *value='\0';
  papersize=GetPageGeometry(property+10);
  if (papersize != (const char *) NULL)
    {
      RectangleInfo
        page = { 0, 0, 0, 0 };

      (void) ParseAbsoluteGeometry(papersize,&page);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17gx%.17g",
        (double) page.width,(double) page.height);
      papersize=DestroyString(papersize);
    }
}

static const char *FormatMagickPropertyPrecision(char *value,
  const double number)
{
  (void) FormatLocaleString(value,MagickPathExtent,"%.*g",
    GetMagickPrecision(),number);
  return((const char *) NULL);
}

typedef struct _MagickPropertyHandlerInfo
{
  const char
    *key;

  size_t
    length;

  MagickPropertyRequirement
    requirement;

  MagickPropertyHandler
    handler;
} MagickPropertyHandlerInfo;

static const char *MagickPropertyBasename(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=GetPathComponentProperty(request->image,BasePath,request->value,string);
  return(string);
}

static const char *MagickPropertyBitDepth(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    GetImageDepth(request->image,request->exception));
  return((const char *) NULL);
}

static const char *MagickPropertyBoundingBox(const MagickPropertyRequest *request)
{
  RectangleInfo
    geometry;

  geometry=GetImageBoundingBox(request->image,request->exception);
  (void) FormatLocaleString(request->value,MagickPathExtent,"%g,%g %g,%g",
    (double) geometry.x,(double) geometry.y,
    (double) geometry.x+geometry.width,
    (double) geometry.y+geometry.height);
  return((const char *) NULL);
}

static const char *MagickPropertyChannels(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%s%s %g.%g",
    CommandOptionToMnemonic(MagickColorspaceOptions,(ssize_t)
    request->image->colorspace),request->image->alpha_trait != UndefinedPixelTrait ?
    "a" : " ",(double) request->image->number_channels,(double)
    request->image->number_meta_channels);
  LocaleLower(request->value);
  return((const char *) NULL);
}

static const char *MagickPropertyColors(const MagickPropertyRequest *request)
{
  request->image->colors=GetNumberColors(request->image,(FILE *) NULL,request->exception);
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    request->image->colors);
  return((const char *) NULL);
}

static const char *MagickPropertyColorspace(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=CommandOptionToMnemonic(MagickColorspaceOptions,(ssize_t)
    request->image->colorspace);
  return(string);
}

static const char *MagickPropertyCompose(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=CommandOptionToMnemonic(MagickComposeOptions,(ssize_t)
    request->image->compose);
  return(string);
}

static const char *GetPointListRequestProperty(
  const MagickPropertyRequest *request,const PointListSide side)
{
  const char
    *string = (char *) NULL;

  const PointListInputs
    inputs = { request->image, request->value, string, request->property,
      request->exception };

  string=GetPointListProperty(&inputs,side);
  return(string);
}

static const char *MagickPropertyConvexHull(const MagickPropertyRequest *request)
{
  return(GetPointListRequestProperty(request,ConvexHullPointList));
}

static const char *MagickPropertyConvexHullExtremePoints(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=GetConvexHullExtremePoints(request->image,request->value,string,request->exception);
  return(string);
}

static const char *MagickPropertyCopyright(const MagickPropertyRequest *request)
{
  (void) CopyMagickString(request->value,GetMagickCopyright(),MagickPathExtent);
  return((const char *) NULL);
}

static const char *MagickPropertyDirectory(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=GetPathComponentProperty(request->image,HeadPath,request->value,string);
  return(string);
}

static const char *MagickPropertyEntropy(const MagickPropertyRequest *request)
{
  double
    entropy;

  (void) GetImageEntropy(request->image,&entropy,request->exception);
  return(FormatMagickPropertyPrecision(request->value,entropy));
}

static const char *MagickPropertyExtension(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=GetPathComponentProperty(request->image,ExtensionPath,request->value,string);
  return(string);
}

static const char *MagickPropertyGamma(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.*g",
    GetMagickPrecision(),request->image->gamma);
  return((const char *) NULL);
}

static const char *MagickPropertyHeight(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",
    request->image->magick_rows != 0 ? (double) request->image->magick_rows : 256.0);
  return((const char *) NULL);
}

static const char *MagickPropertyInterlace(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=CommandOptionToMnemonic(MagickInterlaceOptions,(ssize_t)
    request->image->interlace);
  return(string);
}

static const char *MagickPropertyKurtosis(const MagickPropertyRequest *request)
{
  double
    kurtosis,
    skewness;

  (void) GetImageKurtosis(request->image,&kurtosis,&skewness,request->exception);
  return(FormatMagickPropertyPrecision(request->value,kurtosis));
}

static const char *MagickPropertyMaxima(const MagickPropertyRequest *request)
{
  double
    maximum,
    minimum;

  (void) GetImageRange(request->image,&minimum,&maximum,request->exception);
  return(FormatMagickPropertyPrecision(request->value,maximum));
}

static const char *MagickPropertyMean(const MagickPropertyRequest *request)
{
  double
    mean,
    standard_deviation;

  (void) GetImageMean(request->image,&mean,&standard_deviation,request->exception);
  return(FormatMagickPropertyPrecision(request->value,mean));
}

static const char *MagickPropertyMedian(const MagickPropertyRequest *request)
{
  double
    median;

  (void) GetImageMedian(request->image,&median,request->exception);
  return(FormatMagickPropertyPrecision(request->value,median));
}

static const char *MagickPropertyMimeType(const MagickPropertyRequest *request)
{
  const MagickInfo
    *magick_info;

  magick_info=GetMagickInfo(request->image->magick,request->exception);
  if ((magick_info != (const MagickInfo *) NULL) &&
      (GetMagickMimeType(magick_info) != (const char *) NULL))
    (void) CopyMagickString(request->value,GetMagickMimeType(magick_info),
      MagickPathExtent);
  return((const char *) NULL);
}

static const char *MagickPropertyMinima(const MagickPropertyRequest *request)
{
  double
    maximum,
    minimum;

  (void) GetImageRange(request->image,&minimum,&maximum,request->exception);
  return(FormatMagickPropertyPrecision(request->value,minimum));
}

static const char *MagickPropertyOpaque(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=CommandOptionToMnemonic(MagickBooleanOptions,(ssize_t)
    IsImageOpaque(request->image,request->exception));
  return(string);
}

static const char *MagickPropertyOrientation(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=CommandOptionToMnemonic(MagickOrientationOptions,(ssize_t)
    request->image->orientation);
  return(string);
}

static const char *MagickPropertyOutput(const MagickPropertyRequest *request)
{
  (void) CopyMagickString(request->value,request->image_info->filename,MagickPathExtent);
  return((const char *) NULL);
}

static const char *MagickPropertyPage(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17gx%.17g",
    (double) request->image->page.width,(double) request->image->page.height);
  return((const char *) NULL);
}

static const char *MagickPropertyPrintsizeX(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.*g",
    GetMagickPrecision(),(double) MagickSafeReciprocal(
      request->image->resolution.x)*request->image->columns);
  return((const char *) NULL);
}

static const char *MagickPropertyPrintsizeY(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.*g",
    GetMagickPrecision(),(double) MagickSafeReciprocal(
      request->image->resolution.y)*request->image->rows);
  return((const char *) NULL);
}

static const char *MagickPropertyProfiles(const MagickPropertyRequest *request)
{
  GetProfilesProperty(request->image,request->value);
  return((const char *) NULL);
}

static const char *MagickPropertyQuality(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    request->image->quality);
  return((const char *) NULL);
}

static const char *MagickPropertyResolutionX(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%g",
    request->image->resolution.x);
  return((const char *) NULL);
}

static const char *MagickPropertyResolutionY(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%g",
    request->image->resolution.y);
  return((const char *) NULL);
}

static const char *MagickPropertyScenes(const MagickPropertyRequest *request)
{
  /* FUTURE: equivalent to %n? */
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    GetImageListLength(request->image));
  return((const char *) NULL);
}

static const char *MagickPropertySize(const MagickPropertyRequest *request)
{
  (void) FormatMagickSize(GetBlobSize(request->image),MagickFalse,"B",
    MagickPathExtent,request->value);
  return((const char *) NULL);
}

static const char *MagickPropertySkewness(const MagickPropertyRequest *request)
{
  double
    kurtosis,
    skewness;

  (void) GetImageKurtosis(request->image,&kurtosis,&skewness,request->exception);
  return(FormatMagickPropertyPrecision(request->value,skewness));
}

static const char *MagickPropertyStandardDeviation(const MagickPropertyRequest *request)
{
  double
    mean,
    standard_deviation;

  (void) GetImageMean(request->image,&mean,&standard_deviation,request->exception);
  return(FormatMagickPropertyPrecision(request->value,standard_deviation));
}

static const char *MagickPropertyType(const MagickPropertyRequest *request)
{
  const char
    *string = (char *) NULL;

  string=CommandOptionToMnemonic(MagickTypeOptions,(ssize_t)
    IdentifyImageType(request->image,request->exception));
  return(string);
}

static const char *MagickPropertyVersion(const MagickPropertyRequest *magick_unused(request))
{
  const char
    *string = (char *) NULL;

  magick_unreferenced(request);
  string=GetMagickVersion((size_t *) NULL);
  return(string);
}

static const char *MagickPropertyWidth(const MagickPropertyRequest *request)
{
  (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
    (request->image->magick_columns != 0 ? request->image->magick_columns : 256));
  return((const char *) NULL);
}

static const char *MagickPropertyMinimumBoundingBox(
  const MagickPropertyRequest *request)
{
  return(GetPointListRequestProperty(request,MinimumBoundingBoxPointList));
}

static const char *MagickPropertyPapersize(const MagickPropertyRequest *request)
{
  FormatPaperSizeProperty(request->property,request->value);
  return((const char *) NULL);
}

static const char *MagickPropertyScene(const MagickPropertyRequest *request)
{
  Image
    *const image = request->image;

  const char
    *const property = request->property;

  ExceptionInfo
    *const exception = request->exception;

  if (request->image_info->number_scenes != 0)
    (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
      request->image_info->scene);
  else {
    WarnNoImageReturn("\"%%[%s]\"",property);
    (void) FormatLocaleString(request->value,MagickPathExtent,"%.17g",(double)
      image->scene);
  }
  return((const char *) NULL);
}

static const MagickPropertyHandlerInfo
  MagickPropertyHandlers[] =
  {
    { "basename", 0, ImagePropertyRequirement, MagickPropertyBasename },
    { "bit-depth", 0, ImagePropertyRequirement, MagickPropertyBitDepth },
    { "bounding-box", 0, ImagePropertyRequirement, MagickPropertyBoundingBox },
    { "channels", 0, ImagePropertyRequirement, MagickPropertyChannels },
    { "colors", 0, ImagePropertyRequirement, MagickPropertyColors },
    { "colorspace", 0, ImagePropertyRequirement, MagickPropertyColorspace },
    { "compose", 0, ImagePropertyRequirement, MagickPropertyCompose },
    { "compression", 0, ImagePropertyRequirement, MagickPropertyCompression },
    { "convex-hull", 0, ImagePropertyRequirement, MagickPropertyConvexHull },
    { "convex-hull:extreme-points", 0, ImagePropertyRequirement, MagickPropertyConvexHullExtremePoints },
    { "copyright", 0, NoPropertyRequirement, MagickPropertyCopyright },
    { "depth", 0, ImagePropertyRequirement, MagickPropertyDepth },
    { "directory", 0, ImagePropertyRequirement, MagickPropertyDirectory },
    { "entropy", 0, ImagePropertyRequirement, MagickPropertyEntropy },
    { "extension", 0, ImagePropertyRequirement, MagickPropertyExtension },
    { "gamma", 0, ImagePropertyRequirement, MagickPropertyGamma },
    { "height", 0, ImagePropertyRequirement, MagickPropertyHeight },
    { "input", 0, ImagePropertyRequirement, MagickPropertyInput },
    { "interlace", 0, ImagePropertyRequirement, MagickPropertyInterlace },
    { "kurtosis", 0, ImagePropertyRequirement, MagickPropertyKurtosis },
    { "magick", 0, ImagePropertyRequirement, MagickPropertyMagick },
    { "maxima", 0, ImagePropertyRequirement, MagickPropertyMaxima },
    { "max", 0, ImagePropertyRequirement, MagickPropertyMaxima },
    { "mean", 0, ImagePropertyRequirement, MagickPropertyMean },
    { "median", 0, ImagePropertyRequirement, MagickPropertyMedian },
    { "mime:type", 0, NoPropertyRequirement, MagickPropertyMimeType },
    { "minima", 0, ImagePropertyRequirement, MagickPropertyMinima },
    { "min", 0, ImagePropertyRequirement, MagickPropertyMinima },
    { "minimum-bounding-box", 20, ImagePropertyRequirement,
      MagickPropertyMinimumBoundingBox },
    { "opaque", 0, ImagePropertyRequirement, MagickPropertyOpaque },
    { "orientation", 0, ImagePropertyRequirement, MagickPropertyOrientation },
    { "output", 0, ImageInfoPropertyRequirement, MagickPropertyOutput },
    { "page", 0, ImagePropertyRequirement, MagickPropertyPage },
    { "papersize:", 10, ImagePropertyRequirement, MagickPropertyPapersize },
    { "printsize.x", 0, ImagePropertyRequirement, MagickPropertyPrintsizeX },
    { "printsize.y", 0, ImagePropertyRequirement, MagickPropertyPrintsizeY },
    { "profiles", 0, ImagePropertyRequirement, MagickPropertyProfiles },
    { "quality", 0, ImagePropertyRequirement, MagickPropertyQuality },
    { "resolution.x", 0, ImagePropertyRequirement, MagickPropertyResolutionX },
    { "resolution.y", 0, ImagePropertyRequirement, MagickPropertyResolutionY },
    { "scene", 0, ImageInfoPropertyRequirement, MagickPropertyScene },
    { "scenes", 0, ImagePropertyRequirement, MagickPropertyScenes },
    { "size", 0, ImagePropertyRequirement, MagickPropertySize },
    { "skewness", 0, ImagePropertyRequirement, MagickPropertySkewness },
    { "standard-deviation", 0, ImagePropertyRequirement, MagickPropertyStandardDeviation },
    { "type", 0, ImagePropertyRequirement, MagickPropertyType },
    { "unique", 0, ImageInfoPropertyRequirement, MagickPropertyUnique },
    { "units", 0, ImagePropertyRequirement, MagickPropertyUnits },
    { "version", 0, NoPropertyRequirement, MagickPropertyVersion },
    { "width", 0, ImagePropertyRequirement, MagickPropertyWidth }
  };

static MagickBooleanType IsMagickPropertyKey(
  const MagickPropertyHandlerInfo *info,const char *property)
{
  if (info->length != 0)
    return(LocaleNCompare(info->key,property,info->length) == 0 ? MagickTrue :
      MagickFalse);
  return(LocaleCompare(info->key,property) == 0 ? MagickTrue : MagickFalse);
}

static const MagickPropertyHandlerInfo *GetMagickPropertyHandlerInfo(
  const char *property)
{
  size_t
    i;

  for (i=0; i < (sizeof(MagickPropertyHandlers)/sizeof(*MagickPropertyHandlers)); i++)
    if ((*property == *MagickPropertyHandlers[i].key) &&
        (IsMagickPropertyKey(MagickPropertyHandlers+i,property) != MagickFalse))
      return(MagickPropertyHandlers+i);
  return((const MagickPropertyHandlerInfo *) NULL);
}

static const char *InvokeMagickPropertyHandler(
  const MagickPropertyHandlerInfo *info,const MagickPropertyRequest *request)
{
  ImageInfo
    *const image_info = request->image_info;

  Image
    *const image = request->image;

  const char
    *const property = request->property;

  ExceptionInfo
    *const exception = request->exception;

  if (info->requirement == ImagePropertyRequirement)
    {
      WarnNoImageReturn("\"%%[%s]\"",property);
    }
  if (info->requirement == ImageInfoPropertyRequirement)
    {
      WarnNoImageInfoReturn("\"%%[%s]\"",property);
    }
  return(info->handler(request));
}

#if defined(MAGICKCORE_LCMS_DELEGATE)
static inline MagickBooleanType IsICCProfileProperty(const char *property)
{
  return((LocaleCompare("profile:icc",property) == 0 ||
    LocaleCompare("profile:icm",property) == 0) ? MagickTrue : MagickFalse);
}
#endif

static const char *LookupMagickProperty(const MagickPropertyRequest *request,
  const char *string)
{
  const MagickPropertyHandlerInfo
    *info;

  info=GetMagickPropertyHandlerInfo(request->property);
  if (info != (const MagickPropertyHandlerInfo *) NULL)
    string=InvokeMagickPropertyHandler(info,request);
  return(string);
}

MagickExport const char *GetMagickProperty(ImageInfo *image_info,
  Image *image,const char *property,ExceptionInfo *exception)
{
  char
    value[MagickPathExtent];

  const char
    *string;

  MagickPropertyRequest
    request;

  assert(property != (const char *) NULL);
  assert(property[0] != '\0');
  assert(image != (Image *) NULL || image_info != (ImageInfo *) NULL );
  if (property[1] == '\0')  /* single letter property request */
    return(GetMagickPropertyLetter(image_info,image,*property,exception));
  LogMagickPropertyRequest(image_info,image);
  *value='\0';           /* formated string */
  string=(char *) NULL;  /* constant string reference */
  request.image_info=image_info;
  request.image=image;
  request.property=property;
  request.value=value;
  request.exception=exception;
  string=LookupMagickProperty(&request,string);
  switch (*property)
  {
    case 'p':
    {
#if defined(MAGICKCORE_LCMS_DELEGATE)
      if (IsICCProfileProperty(property) != MagickFalse)
        {
#if !defined(LCMS_VERSION) || (LCMS_VERSION < 2000)
#define cmsUInt32Number  DWORD
#endif

          const StringInfo
            *profile;

          cmsHPROFILE
            icc_profile;

          WarnNoImageReturn("\"%%[%s]\"",property);
          profile=GetImageProfile(image,property+8);
          if (profile == (StringInfo *) NULL)
            break;
          icc_profile=cmsOpenProfileFromMem(GetStringInfoDatum(profile),
            (cmsUInt32Number) GetStringInfoLength(profile));
          if (icc_profile != (cmsHPROFILE *) NULL)
            {
#if defined(LCMS_VERSION) && (LCMS_VERSION < 2000)
              string=cmsTakeProductName(icc_profile);
#else
              (void) cmsGetProfileInfoASCII(icc_profile,cmsInfoDescription,
                "en","US",value,MagickPathExtent);
#endif
              (void) cmsCloseProfile(icc_profile);
            }
      }
#endif
      break;
    }
  }
  return(SelectMagickPropertyValue(image_info,image,string,value));
}
#undef WarnNoImageReturn

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t N e x t I m a g e P r o p e r t y                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNextImageProperty() gets the next free-form string property name.
%
%  The format of the GetNextImageProperty method is:
%
%      char *GetNextImageProperty(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport const char *GetNextImageProperty(const Image *image)
{
  CheckPropertyImage(image);
  if (image->properties == (void *) NULL)
    return((const char *) NULL);
  return((const char *) GetNextKeyInSplayTree((SplayTreeInfo *) image->properties));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n t e r p r e t I m a g e P r o p e r t i e s                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InterpretImageProperties() replaces any embedded formatting characters with
%  the appropriate image property and returns the interpreted text.
%
%  This searches for and replaces
%     \n \r \%          replaced by newline, return, and percent resp.
%     &lt; &gt; &amp;   replaced by '<', '>', '&' resp.
%     %%                replaced by percent
%
%     %x %[x]       where 'x' is a single letter prosperity, case sensitive).
%     %[type:name]  where 'type' a is special and known prefix.
%     %[name]       where 'name' is a specifically known attribute, calculated
%                   value, or a per-image property string name, or a per-image
%                   'artifact' (as generated from a global option).
%                   It may contain ':' as long as the prefix is not special.
%
%  Single letter % substitutions will only happen if the character before the
%  percent is NOT a number. But braced substitutions will always be performed.
%  This prevents the typical usage of percent in a interpreted geometry
%  argument from being substituted when the percent is a geometry flag.
%
%  If 'glob-expressions' ('*' or '?' characters) is used for 'name' it may be
%  used as a search pattern to print multiple lines of "name=value\n" pairs of
%  the associated set of properties.
%
%  The returned string must be freed using DestroyString() by the caller.
%
%  The format of the InterpretImageProperties method is:
%
%      char *InterpretImageProperties(ImageInfo *image_info,
%        Image *image,const char *embed_text,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info. (required)
%
%    o image: the image. (optional)
%
%    o embed_text: the address of a character string containing the embedded
%      formatting characters.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsNonEmptyImage(const Image *image)
{
  return(((image != (Image *) NULL) && (image->columns != 0) &&
    (image->rows != 0)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsEscapeEnd(const char *p)
{
  return(((*p == '\0') || (*p == '\'') || (*p == '"')) ? MagickTrue :
    MagickFalse);
}

static void TruncatePatternForMessage(char *pattern,const ssize_t offset)
{
  if (offset >= 64)
    {
      pattern[61]='.';  /* truncate string for error message */
      pattern[62]='.';
      pattern[63]='.';
      pattern[64]='\0';
    }
}

static inline MagickBooleanType IsEscapedPatternCharacter(const char *p)
{
  return(((*p == '\\') && (*(p+1) != '\0')) ? MagickTrue : MagickFalse);
}

static void ScanBracedPattern(char *pattern,ssize_t *offset,ssize_t *depth,
  const char **p)
{
  for ((*offset)=0; (*offset) < (MagickPathExtent-1L) && (*(*p) != '\0'); )
  {
    if (IsEscapedPatternCharacter(*p) != MagickFalse)
      {
        /*
          Skip escaped braces within braced pattern.
        */
        pattern[(*offset)++]=(*(*p)++);
        pattern[(*offset)++]=(*(*p)++);
        continue;
      }
    if (*(*p) == '[')
      (*depth)++;
    if (*(*p) == ']')
      (*depth)--;
    if ((*depth) <= 0)
      break;
    pattern[(*offset)++]=(*(*p)++);
  }
  pattern[(*offset)]='\0';
}

static MagickStatusType EvaluatePixelExpression(FxInfo *fx_info,
  const Image *image,PixelInfo *pixel,ExceptionInfo *exception)
{
  const FxChannelPosition
    red = { RedPixelChannel, 0, 0 },
    green = { GreenPixelChannel, 0, 0 },
    blue = { BluePixelChannel, 0, 0 },
    black = { BlackPixelChannel, 0, 0 },
    alpha = { AlphaPixelChannel, 0, 0 };

  double
    value;

  MagickStatusType
    fx_status;

  fx_status=FxEvaluateChannelExpression(fx_info,&red,&value,exception);
  pixel->red=(double) QuantumRange*value;
  fx_status&=(MagickStatusType) FxEvaluateChannelExpression(fx_info,&green,
    &value,exception);
  pixel->green=(double) QuantumRange*value;
  fx_status&=(MagickStatusType) FxEvaluateChannelExpression(fx_info,&blue,
    &value,exception);
  pixel->blue=(double) QuantumRange*value;
  if (image->colorspace == CMYKColorspace)
    {
      fx_status&=(MagickStatusType) FxEvaluateChannelExpression(fx_info,
        &black,&value,exception);
      pixel->black=(double) QuantumRange*value;
    }
  fx_status&=(MagickStatusType) FxEvaluateChannelExpression(fx_info,&alpha,
    &value,exception);
  pixel->alpha=(double) QuantumRange*value;
  return(fx_status);
}

static void InterpretXMLEntity(char **q,const char **p)
{
  if (LocaleNCompare("&lt;",(*p),4) == 0)
    {
      *(*q)++='<';
      (*p)+=(ptrdiff_t) 3;
    }
  else
    if (LocaleNCompare("&gt;",(*p),4) == 0)
      {
        *(*q)++='>';
        (*p)+=(ptrdiff_t) 3;
      }
    else
      if (LocaleNCompare("&amp;",(*p),5) == 0)
        {
          *(*q)++='&';
          (*p)+=(ptrdiff_t) 4;
        }
      else
        *(*q)++=(*(*p));
}

typedef struct _InterpretText
{
  char
    **interpret_text,
    **q;

  size_t
    *extent;
} InterpretText;

static MagickBooleanType ExtendInterpretBuffer(const InterpretText *text,
  const size_t string_length)
{
  size_t
    length = (string_length);

  if ((size_t) ((*text->q)-(*text->interpret_text)+(ssize_t) length+1) >=
      (*text->extent))
    {
      (*text->extent)+=length;
      (*text->interpret_text)=(char *) ResizeQuantumMemory(
        (*text->interpret_text),(*text->extent)+MagickPathExtent,
        sizeof(*(*text->interpret_text)));
      if ((*text->interpret_text) == (char *) NULL)
        return(MagickFalse);
      (*text->q)=(*text->interpret_text)+strlen((*text->interpret_text));
    }
  return(MagickTrue);
}

static MagickBooleanType AppendInterpretKeyValue(const InterpretText *text,
  const char *key,const char *value)
{
  size_t
    length = strlen(key)+strlen(value)+2;

  if (ExtendInterpretBuffer(text,length) == MagickFalse)
    return(MagickFalse);
  (*text->q)+=(ptrdiff_t) FormatLocaleString((*text->q),(*text->extent),
    "%s=%s\n",(key),(value));
  return(MagickTrue);
}

static MagickBooleanType AppendInterpretString(const InterpretText *text,
  const char *string)
{
  size_t
    length = strlen((string));

  if (ExtendInterpretBuffer(text,length) == MagickFalse)
    return(MagickFalse);
  (void) CopyMagickString((*text->q),(string),(*text->extent));
  (*text->q)+=(ptrdiff_t) length;
  return(MagickTrue);
}

static void InterpretBackslash(char **q,const char **p)
{
  switch (*(*p+1))
  {
    case '\0':
      return;
    case 'r':  /* convert to RETURN */
    {
      *(*q)++='\r';
      (*p)++;
      return;
    }
    case 'n':  /* convert to NEWLINE */
    {
      *(*q)++='\n';
      (*p)++;
      return;
    }
    case '\n':  /* EOL removal UNIX,MacOSX */
    {
      (*p)++;
      return;
    }
    case '\r':  /* EOL removal DOS,Windows */
    {
      (*p)++;
      if (**p == '\n') /* return-newline EOL */
        (*p)++;
      return;
    }
    default:
    {
      (*p)++;
      *(*q)++=(**p);
    }
  }
}

static MagickBooleanType InterpretCharacter(char **q,const char **p)
{
  switch (**p)
  {
    case '\\':
    {
      InterpretBackslash(q,p);
      return(MagickTrue);
    }
    case '&':
    {
      InterpretXMLEntity(q,p);
      return(MagickTrue);
    }
    case '%':
      break;  /* continue to next set of handlers */
    default:
    {
      *(*q)++=(**p);  /* any thing else is 'as normal' */
      return(MagickTrue);
    }
  }
  return(MagickFalse);
}

static MagickBooleanType AppendFxValue(const InterpretText *text,
  Image *property_image,const char *pattern,ExceptionInfo *exception)
{
  const FxChannelPosition
    composite = { CompositePixelChannel, 0, 0 };

  double
    value;

  FxInfo
    *fx_info;

  MagickBooleanType
    fx_status;

  /*
    FX - value calculator.
  */
  fx_info=AcquireFxInfo(property_image,pattern+3,exception);
  if (fx_info == (FxInfo *) NULL)
    return(MagickTrue);
  fx_status=FxEvaluateChannelExpression(fx_info,&composite,&value,exception);
  fx_info=DestroyFxInfo(fx_info);
  if (fx_status != MagickFalse)
    {
      char
        result[MagickPathExtent];

      (void) FormatLocaleString(result,MagickPathExtent,"%.*g",
        GetMagickPrecision(),(double) value);
      if (AppendInterpretString(text,result) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType AppendHexValue(const InterpretText *text,
  Image *property_image,const char *pattern,ExceptionInfo *exception)
{
  FxInfo
    *fx_info;

  MagickStatusType
    fx_status;

  PixelInfo
    pixel;

  /*
    Pixel - color value calculator.
  */
  GetPixelInfo(property_image,&pixel);
  fx_info=AcquireFxInfo(property_image,pattern+4,exception);
  if (fx_info == (FxInfo *) NULL)
    return(MagickTrue);
  fx_status=EvaluatePixelExpression(fx_info,property_image,&pixel,
    exception);
  fx_info=DestroyFxInfo(fx_info);
  if (fx_status != MagickFalse)
    {
      char
        hex[MagickPathExtent];

      GetColorTuple(&pixel,MagickTrue,hex);
      if (AppendInterpretString(text,hex+1) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType AppendPixelValue(const InterpretText *text,
  Image *property_image,const char *pattern,ExceptionInfo *exception)
{
  const char
    *string;

  FxInfo
    *fx_info;

  MagickStatusType
    fx_status;

  PixelInfo
    pixel;

  /*
    Pixel - color value calculator.
  */
  GetPixelInfo(property_image,&pixel);
  fx_info=AcquireFxInfo(property_image,pattern+6,exception);
  if (fx_info == (FxInfo *) NULL)
    return(MagickTrue);
  fx_status=EvaluatePixelExpression(fx_info,property_image,&pixel,
    exception);
  fx_info=DestroyFxInfo(fx_info);
  if (fx_status != MagickFalse)
    {
      char
        name[MagickPathExtent];

      GetColorTuple(&pixel,MagickFalse,name);
      string=GetImageArtifact(property_image,"pixel:compliance");
      if (string != (char *) NULL)
        {
          ComplianceType compliance=(ComplianceType) ParseCommandOption(
            MagickComplianceOptions,MagickFalse,string);
          (void) QueryColorname(property_image,&pixel,compliance,name,
            exception);
        }
      if (AppendInterpretString(text,name) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _InterpretSources
{
  ImageInfo
    *property_info;

  Image
    *property_image;

  ExceptionInfo
    *exception;
} InterpretSources;

typedef enum
{
  OptionGlobSide,
  ArtifactGlobSide
} GlobSide;

static MagickBooleanType AppendGlobbedValues(const InterpretSources *sources,
  const InterpretText *text,const char *glob,const GlobSide side)
{
  const char
    *key,
    *string;

  Image
    *const property_image = sources->property_image;

  ImageInfo
    *const property_info = sources->property_info;

  ((side == OptionGlobSide) ? ResetImageOptionIterator(property_info) :
    ResetImageArtifactIterator(property_image));
  while ((key=((side == OptionGlobSide) ? GetNextImageOption(property_info) :
         GetNextImageArtifact(property_image))) != (const char *) NULL)
    if (GlobExpression(key,glob,MagickTrue) != MagickFalse)
      {
        string=((side == OptionGlobSide) ? GetImageOption(property_info,key) :
          GetImageArtifact(property_image,key));
        if ((string != (const char *) NULL) &&
            (AppendInterpretKeyValue(text,key,string) == MagickFalse))
          return(MagickFalse);
      }
  return(MagickTrue);
}

static MagickBooleanType AppendGlobbedProperties(const InterpretText *text,
  Image *property_image,const char *pattern,ExceptionInfo *exception)
{
  const char
    *key,
    *string;

  ResetImagePropertyIterator(property_image);
  while ((key=GetNextImageProperty(property_image)) != (const char *) NULL)
    if (GlobExpression(key,pattern,MagickTrue) != MagickFalse)
      {
        string=GetImageProperty(property_image,key,exception);
        if ((string != (const char *) NULL) &&
            (AppendInterpretKeyValue(text,key,string) == MagickFalse))
          return(MagickFalse);
      }
  return(MagickTrue);
}

static MagickBooleanType WarnUnknownProperty(ExceptionInfo *exception,
  char *pattern,const ssize_t offset)
{
  TruncatePatternForMessage(pattern,offset);
  (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
    "UnknownImageProperty","\"%%[%s]\"",pattern);
  return(MagickTrue);
}

typedef struct _BracedPattern
{
  char
    *pattern;

  ssize_t
    offset;
} BracedPattern;

static MagickBooleanType AppendOptionOrArtifact(
  const InterpretSources *sources,const InterpretText *text,
  const BracedPattern *braced,const GlobSide side)
{
  char
    *const pattern = braced->pattern;

  const char
    *string;

  const ssize_t
    offset = braced->offset;

  ExceptionInfo
    *const exception = sources->exception;

  Image
    *const property_image = sources->property_image;

  ImageInfo
    *const property_info = sources->property_info;

  /*
    Option - direct global option lookup (with globbing).
    Artifact - direct image artifact lookup (with glob).
  */
  if (IsGlob(((side == OptionGlobSide) ? pattern+7 : pattern+9)) != MagickFalse)
    return(AppendGlobbedValues(sources,text,((side == OptionGlobSide) ?
      pattern+7 : pattern+9),side));
  string=((side == OptionGlobSide) ? GetImageOption(property_info,pattern+7) :
    GetImageArtifact(property_image,pattern+9));
  if (string == (char *) NULL)
    return(WarnUnknownProperty(exception,pattern,offset));
  return(AppendInterpretString(text,string));
}

static MagickBooleanType AppendPrefixedProperty(
  const InterpretSources *sources,const InterpretText *text,
  const BracedPattern *braced)
{
  char
    *const pattern = braced->pattern;

  const char
    *string;

  const ssize_t
    offset = braced->offset;

  ExceptionInfo
    *const exception = sources->exception;

  Image
    *const property_image = sources->property_image;

  /*
    Property - direct image property lookup (with glob).
  */
  if (IsGlob(pattern+9) != MagickFalse)
    return(AppendGlobbedProperties(text,property_image,pattern,
      exception));
  string=GetImageProperty(property_image,pattern+9,exception);
  if (string == (char *) NULL)
    return(WarnUnknownProperty(exception,pattern,offset));
  return(AppendInterpretString(text,string));
}

static MagickBooleanType AppendUnprefixedValue(
  const InterpretSources *sources,const InterpretText *text,
  const BracedPattern *braced)
{
  char
    *const pattern = braced->pattern;

  const char
    *string;

  const ssize_t
    offset = braced->offset;

  ExceptionInfo
    *const exception = sources->exception;

  Image
    *const property_image = sources->property_image;

  ImageInfo
    *const property_info = sources->property_info;

  /*
    Properties without special prefix.
  */
  string=GetImageProperty(property_image,pattern,exception);
  if (string != (const char *) NULL)
    {
      if (AppendInterpretString(text,string) == MagickFalse)
        return(MagickFalse);
      (void) DeleteImageArtifact(property_image,"magick-property");
      (void) DeleteImageOption(property_info,"magick-property");
      return(MagickTrue);
    }
  if (IsGlob(pattern) != MagickFalse)
    return(AppendGlobbedProperties(text,property_image,pattern,exception));
  /*
    Known property or attribute.
  */
  string=GetMagickProperty(property_info,property_image,pattern,exception);
  if (string != (const char *) NULL)
    return(AppendInterpretString(text,string));
  /*
    Per-image artifact.
  */
  string=GetImageArtifact(property_image,pattern);
  if (string != (char *) NULL)
    return(AppendInterpretString(text,string));
  /*
    Direct option lookup.
  */
  string=GetImageOption(property_info,pattern);
  if (string != (char *) NULL)
    return(AppendInterpretString(text,string));
  return(WarnUnknownProperty(exception,pattern,offset));
}

static MagickBooleanType AppendBracedValue(
  const InterpretSources *sources,const InterpretText *text,
  const BracedPattern *braced)
{
  char
    *const pattern = braced->pattern;

  ExceptionInfo
    *const exception = sources->exception;

  Image
    *const property_image = sources->property_image;

  /*
    Special Lookup Prefixes %[prefix:...].
  */
  if (LocaleNCompare("fx:",pattern,3) == 0)
    return(AppendFxValue(text,property_image,pattern,exception));
  if (LocaleNCompare("hex:",pattern,4) == 0)
    return(AppendHexValue(text,property_image,pattern,exception));
  if (LocaleNCompare("pixel:",pattern,6) == 0)
    return(AppendPixelValue(text,property_image,pattern,exception));
  if (LocaleNCompare("option:",pattern,7) == 0)
    return(AppendOptionOrArtifact(sources,text,braced,OptionGlobSide));
  if (LocaleNCompare("artifact:",pattern,9) == 0)
    return(AppendOptionOrArtifact(sources,text,braced,ArtifactGlobSide));
  if (LocaleNCompare("property:",pattern,9) == 0)
    return(AppendPrefixedProperty(sources,text,braced));
  return(AppendUnprefixedValue(sources,text,braced));
}

static MagickBooleanType InterpretBracedEscape(
  const InterpretSources *sources,const InterpretText *text,const char **p)
{
  char
    pattern[2*MagickPathExtent] = "\0";

  BracedPattern
    braced;

  ssize_t
    depth,
    offset;

  ExceptionInfo
    *const exception = sources->exception;

  /*
    Braced Percent Escape %[...].
  */
  (*p)++;  /* advance p to just inside the opening brace */
  depth=1;
  if (**p == ']')
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "UnknownImageProperty","\"%%[]\"");
      return(MagickFalse);
    }
  ScanBracedPattern(pattern,&offset,&depth,p);
  if (depth != 0)
    {
      /*
        Check for unmatched final ']' for "%[...]".
      */
      TruncatePatternForMessage(pattern,offset);
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "UnbalancedBraces","\"%%[%s\"",pattern);
      return(MagickFalse);
    }
  braced.pattern=pattern;
  braced.offset=offset;
  return(AppendBracedValue(sources,text,&braced));
}

static MagickBooleanType ReadInterpretTextFile(const char *p,
  char **interpret_text,ExceptionInfo *exception,char **result)
{
  if ((*p == '@') && (IsPathAccessible(p+1) != MagickFalse))
    {
      /*
        Handle a '@' replace string from file.
      */
      (*interpret_text)=FileToString(p,~0UL,exception);
      if ((*interpret_text) != (char *) NULL)
        {
          (*result)=(*interpret_text);
          return(MagickTrue);
        }
    }
  return(MagickFalse);
}

typedef struct _InterpretLetterInputs
{
  ImageInfo
    *property_info;

  Image
    *image,
    *property_image;

  ExceptionInfo
    *exception;

  MagickBooleanType
    number;
} InterpretLetterInputs;

static MagickBooleanType InterpretLetterEscape(
  const InterpretLetterInputs *inputs,const InterpretText *text,
  const char **p)
{
  ImageInfo
    *const property_info = inputs->property_info;

  Image
    *const image = inputs->image,
    *const property_image = inputs->property_image;

  ExceptionInfo
    *const exception = inputs->exception;

  const MagickBooleanType
    number = inputs->number;

  char
    **const q = text->q;

  const char
    *string;

  if (number != MagickFalse)
    {
      /*
        But only if not preceded by a number!
      */
      *(*q)++='%'; /* do NOT substitute the percent */
      (*p)--;      /* back up one */
      return(MagickTrue);
    }
  string=GetMagickPropertyLetter(property_info,image,*(*p),exception);
  if (string != (char *) NULL)
    {
      if (AppendInterpretString(text,(string)) == MagickFalse)
        return(MagickFalse);
      (void) DeleteImageArtifact(property_image,"magick-property");
      (void) DeleteImageOption(property_info,"magick-property");
      return(MagickTrue);
    }
  (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
    "UnknownImageProperty","\"%%%c\"",*(*p));
  return(MagickTrue);
}

static MagickBooleanType InterpretEscapeCharacter(
  const InterpretLetterInputs *inputs,const InterpretText *text,
  const char **p)
{
  char
    **const q = text->q;

  /*
    Look for the various escapes, (and handle other specials)
  */
  *(*q)='\0';
  if (ExtendInterpretBuffer(text,(MagickPathExtent)) == MagickFalse)
    return(MagickFalse);
  if (InterpretCharacter(q,p) != MagickFalse)
    return(MagickTrue);
  (*p)++; /* advance beyond the percent */
  /*
    Doubled Percent - or percent at end of string.
  */
  if (IsEscapeEnd(*p) != MagickFalse)
    (*p)--;
  if (*(*p) == '%')
    {
      *(*q)++='%';
      return(MagickTrue);
    }
  /*
    Single letter escapes %c.
  */
  if (*(*p) != '[')
    return(InterpretLetterEscape(inputs,text,p));
  {
    const InterpretSources
      sources = { inputs->property_info, inputs->property_image,
        inputs->exception };

    return(InterpretBracedEscape(&sources,text,p));
  }
}

static void LogInterpretImageProperties(const ImageInfo *image_info,
  const Image *image)
{
  if ((image != (Image *) NULL) && (IsEventLogging() != MagickFalse))
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  else
    if ((image_info != (ImageInfo *) NULL) && (IsEventLogging() != MagickFalse))
      (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
        image_info->filename);
    else
      if (IsEventLogging() != MagickFalse)
        (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s","no image");
}

static void DestroyInterpretSources(const ImageInfo *image_info,
  const Image *image,ImageInfo *property_info,Image *property_image)
{
  if ((property_image != image) && (property_image != (Image *) NULL))
    property_image=DestroyImage(property_image);
  if ((property_info != image_info) && (property_info != (ImageInfo *) NULL))
    property_info=DestroyImageInfo(property_info);
}

static MagickBooleanType AcquireInterpretImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception,Image **property_image)
{
  if (IsNonEmptyImage(image) != MagickFalse)
    (*property_image)=image;
  else
    {
      (*property_image)=AcquireImage(image_info,exception);
      if ((*property_image) == (Image *) NULL)
        return(MagickFalse);
      (void) SetImageExtent((*property_image),1,1,exception);
      (void) SetImageBackgroundColor((*property_image),exception);
    }
  return(MagickTrue);
}

static const char *SkipInterpretSpaces(const char *embed_text)
{
  const char
    *p;

  p=embed_text;
  while ((isspace((int) ((unsigned char) *p)) != 0) && (*p != '\0'))
    p++;
  return(p);
}

static char *FinishInterpretText(const MagickBooleanType status,
  char *interpret_text)
{
  if (status == MagickFalse)
    {
      if (interpret_text != (char *) NULL)
        interpret_text=DestroyString(interpret_text);
      return((char *) NULL);
    }
  return(interpret_text);
}

typedef struct _InterpretRequest
{
  ImageInfo
    *image_info;

  Image
    *image;

  const char
    *embed_text;

  ImageInfo
    *property_info;

  ExceptionInfo
    *exception;
} InterpretRequest;

static MagickBooleanType InterpretEmbeddedText(const InterpretRequest *request,
  Image **property_image,const InterpretText *text,const char **p)
{
  ImageInfo
    *const image_info = request->image_info;

  Image
    *const image = request->image;

  const char
    *const embed_text = request->embed_text;

  ImageInfo
    *const property_info = request->property_info;

  ExceptionInfo
    *const exception = request->exception;

  char
    **const interpret_text = text->interpret_text,
    **const q = text->q;

  size_t
    *const extent = text->extent;

  MagickBooleanType
    number;

  if (AcquireInterpretImage(image_info,image,exception,property_image) ==
      MagickFalse)
    return(MagickFalse);
  (*interpret_text)=AcquireString(embed_text); /* new string with extra space */
  if ((*interpret_text) == (char *) NULL)
    return(MagickFalse);
  (*extent)=MagickPathExtent;                  /* allocated space in string */
  number=MagickFalse;                       /* is last char a number? */
  for ((*q)=(*interpret_text); *(*p)!='\0'; number=isdigit((int) ((unsigned char) *(*p))) ? MagickTrue : MagickFalse,(*p)++)
  {
    InterpretLetterInputs
      inputs;

    inputs.property_info=property_info;
    inputs.image=image;
    inputs.property_image=(*property_image);
    inputs.exception=exception;
    inputs.number=number;
    if (InterpretEscapeCharacter(&inputs,text,p) == MagickFalse)
      return(MagickFalse);
  }
  *(*q)='\0';
  return(MagickTrue);
}

MagickExport char *InterpretImageProperties(ImageInfo *image_info,Image *image,
  const char *embed_text,ExceptionInfo *exception)
{
  char
    *interpret_text = (char *) NULL,
    *q,  /* current position in interpret_text */
    *returned;

  const char
    *p;  /* position in embed_text string being expanded */

  Image
    *property_image = (Image *) NULL;

  ImageInfo
    *property_info = (ImageInfo *) NULL;

  InterpretRequest
    request;

  MagickBooleanType
    status = MagickTrue;

  size_t
    extent;  /* allocated length of interpret_text */

  const InterpretText
    text = { &interpret_text, &q, &extent };

  LogInterpretImageProperties(image_info,image);
  if (embed_text == (const char *) NULL)
    return(ConstantString(""));
  p=SkipInterpretSpaces(embed_text);
  if (*p == '\0')
    return(ConstantString(""));
  if (ReadInterpretTextFile(p,&interpret_text,exception,&returned) !=
      MagickFalse)
    return(returned);
  /*
    Translate any embedded format characters.
  */
  if (image_info != (ImageInfo *) NULL)
    property_info=image_info;
  else
    property_info=CloneImageInfo(image_info);
  request.image_info=image_info;
  request.image=image;
  request.embed_text=embed_text;
  request.property_info=property_info;
  request.exception=exception;
  if (InterpretEmbeddedText(&request,&property_image,&text,&p) == MagickFalse)
    {
      status=MagickFalse;
      goto cleanup;
    }

cleanup:
  DestroyInterpretSources(image_info,image,property_info,property_image);
  return(FinishInterpretText(status,interpret_text));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e m o v e I m a g e P r o p e r t y                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RemoveImageProperty() removes a property from the image and returns its
%  value.
%
%  In this case the ConstantString() value returned should be freed by the
%  caller when finished.
%
%  The format of the RemoveImageProperty method is:
%
%      char *RemoveImageProperty(Image *image,const char *property)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o property: the image property.
%
*/
MagickExport char *RemoveImageProperty(Image *image,const char *property)
{
  char
    *value;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->properties == (void *) NULL)
    return((char *) NULL);
  value=(char *) RemoveNodeFromSplayTree((SplayTreeInfo *) image->properties,
    property);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s e t I m a g e P r o p e r t y I t e r a t o r                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetImagePropertyIterator() resets the image properties iterator.  Use it
%  in conjunction with GetNextImageProperty() to iterate over all the values
%  associated with an image property.
%
%  The format of the ResetImagePropertyIterator method is:
%
%      void ResetImagePropertyIterator(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport void ResetImagePropertyIterator(const Image *image)
{
  CheckPropertyImage(image);
  if (image->properties == (void *) NULL)
    return;
  ResetSplayTreeIterator((SplayTreeInfo *) image->properties);
}

static const char
  *const ReadOnlyProperties[] =
  {
    "channels",
    "delay_units",
    "height",
    "kurtosis",
    "magick",
    "max",
    "mean",
    "min",
    "opaque",
    "size",
    "skewness",
    "scenes",
    "standard-deviation",
    "version",
    "width"
  };

static MagickBooleanType IsReadOnlyProperty(const char *property)
{
  size_t
    i;

  for (i=0; i < (sizeof(ReadOnlyProperties)/sizeof(*ReadOnlyProperties)); i++)
    if (LocaleCompare(ReadOnlyProperties[i],property) == 0)
      return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e P r o p e r t y                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageProperty() saves the given string value either to specific known
%  attribute or to a freeform property string.
%
%  Attempting to set a property that is normally calculated will produce
%  an exception.
%
%  The format of the SetImageProperty method is:
%
%      MagickBooleanType SetImageProperty(Image *image,const char *property,
%        const char *value,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o property: the image property.
%
%    o values: the image property values.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void LowerImageDelay(Image *image,const GeometryInfo geometry_info)
{
  if (image->delay > (size_t) floor(geometry_info.rho+0.5))
    image->delay=(size_t) floor(geometry_info.rho+0.5);
}

static MagickBooleanType SetImageDelayProperty(Image *image,
  const char *value)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  flags=ParseGeometry(value,&geometry_info);
  if ((flags & GreaterValue) != 0)
    LowerImageDelay(image,geometry_info);
  else
    if ((flags & LessValue) != 0)
      {
        if ((double) image->delay < floor(geometry_info.rho+0.5))
          image->delay=(size_t) CastDoubleToSsizeT(floor(
            geometry_info.sigma+0.5));
      }
    else
      image->delay=(size_t) floor(geometry_info.rho+0.5);
  if ((flags & SigmaValue) != 0)
    image->ticks_per_second=CastDoubleToSsizeT(floor(
      geometry_info.sigma+0.5));
  return(MagickTrue);
}

static MagickBooleanType SetImageColorspaceProperty(Image *image,
  const char *value,ExceptionInfo *exception)
{
  ssize_t
    colorspace;

  colorspace=ParseCommandOption(MagickColorspaceOptions,MagickFalse,
    value);
  if (colorspace < 0)
    return(MagickFalse); /* FUTURE: value exception?? */
  return(SetImageColorspace(image,(ColorspaceType) colorspace,exception));
}

static MagickBooleanType SetImageDensityProperty(Image *image,
  const char *value,MagickStatusType *flags)
{
  GeometryInfo
    geometry_info;

  (*flags)=ParseGeometry(value,&geometry_info);
  if (((*flags) & RhoValue) != 0)
    image->resolution.x=geometry_info.rho;
  image->resolution.y=image->resolution.x;
  if (((*flags) & SigmaValue) != 0)
    image->resolution.y=geometry_info.sigma;
  return(MagickTrue);
}

typedef void
  (*SetImageMemberMethod)(Image *,const ssize_t);

typedef struct _EnumeratedPropertyInfo
{
  const char
    *property;

  CommandOption
    option;

  SetImageMemberMethod
    set_member;
} EnumeratedPropertyInfo;

static void SetImageComposeMember(Image *image,const ssize_t value)
{
  image->compose=(CompositeOperator) value;
}

static void SetImageCompressionMember(Image *image,const ssize_t value)
{
  image->compression=(CompressionType) value;
}

static void SetImageDisposeMember(Image *image,const ssize_t value)
{
  image->dispose=(DisposeType) value;
}

static void SetImageGravityMember(Image *image,const ssize_t value)
{
  image->gravity=(GravityType) value;
}

static void SetImageIntensityMember(Image *image,const ssize_t value)
{
  image->intensity=(PixelIntensityMethod) value;
}

static void SetImageRenderingIntentMember(Image *image,const ssize_t value)
{
  image->rendering_intent=(RenderingIntent) value;
}

static void SetImageInterpolateMember(Image *image,const ssize_t value)
{
  image->interpolate=(PixelInterpolateMethod) value;
}

static void SetImageTypeMember(Image *image,const ssize_t value)
{
  image->type=(ImageType) value;
}

static void SetImageUnitsMember(Image *image,const ssize_t value)
{
  image->units=(ResolutionType) value;
}

static const EnumeratedPropertyInfo
  EnumeratedProperties[] =
  {
    { "compose", MagickComposeOptions, SetImageComposeMember },
    { "compress", MagickCompressOptions, SetImageCompressionMember },
    { "dispose", MagickDisposeOptions, SetImageDisposeMember },
    { "gravity", MagickGravityOptions, SetImageGravityMember },
    { "intensity", MagickIntensityOptions, SetImageIntensityMember },
    { "intent", MagickIntentOptions, SetImageRenderingIntentMember },
    { "interpolate", MagickInterpolateOptions, SetImageInterpolateMember },
    { "rendering-intent", MagickIntentOptions, SetImageRenderingIntentMember },
    { "type", MagickTypeOptions, SetImageTypeMember },
    { "units", MagickResolutionOptions, SetImageUnitsMember }
  };

static MagickBooleanType SetEnumeratedProperty(Image *image,
  const char *property,const char *value,MagickBooleanType *result)
{
  ssize_t
    i,
    option;

  for (i=0; i < (ssize_t) (sizeof(EnumeratedProperties)/
       sizeof(*EnumeratedProperties)); i++)
    if (LocaleCompare(EnumeratedProperties[i].property,property) == 0)
      {
        option=ParseCommandOption(EnumeratedProperties[i].option,MagickFalse,
          value);
        if (option < 0)
          {
            (*result)=MagickFalse;
            return(MagickTrue);
          }
        EnumeratedProperties[i].set_member(image,option);
        (*result)=MagickTrue;
        return(MagickTrue);
      }
  return(MagickFalse);
}

typedef MagickBooleanType
  (*ImagePropertyHandler)(Image *,const char *,ExceptionInfo *);

typedef struct _ImagePropertyHandlerInfo
{
  const char
    *key;

  ImagePropertyHandler
    handler;
} ImagePropertyHandlerInfo;

static MagickBooleanType SetImageDelayHandler(Image *image,const char *value,
  ExceptionInfo *magick_unused(exception))
{
  magick_unreferenced(exception);
  return(SetImageDelayProperty(image,value));
}

static MagickBooleanType SetImageDensityHandler(Image *image,const char *value,
  ExceptionInfo *magick_unused(exception))
{
  MagickStatusType
    flags;

  magick_unreferenced(exception);
  return(SetImageDensityProperty(image,value,&flags));
}

static MagickBooleanType SetImageDepthHandler(Image *image,const char *value,
  ExceptionInfo *magick_unused(exception))
{
  magick_unreferenced(exception);
  image->depth=StringToUnsignedLong(value);
  return(MagickTrue);
}

static MagickBooleanType SetImageGammaHandler(Image *image,const char *value,
  ExceptionInfo *magick_unused(exception))
{
  magick_unreferenced(exception);
  image->gamma=StringToDouble(value,(char **) NULL);
  return(MagickTrue);
}

static MagickBooleanType SetImageLoopHandler(Image *image,const char *value,
  ExceptionInfo *magick_unused(exception))
{
  magick_unreferenced(exception);
  image->iterations=StringToUnsignedLong(value);
  return(MagickTrue);
}

static MagickBooleanType SetImageGeometryMember(const char *value,
  RectangleInfo *member,MagickStatusType *flags)
{
  char
    *geometry;

  geometry=GetPageGeometry(value);
  (*flags)=ParseAbsoluteGeometry(geometry,member);
  geometry=DestroyString(geometry);
  return(MagickTrue);
}

static MagickBooleanType SetImagePageHandler(Image *image,const char *value,
  ExceptionInfo *magick_unused(exception))
{
  MagickStatusType
    flags;

  magick_unreferenced(exception);
  return(SetImageGeometryMember(value,&image->page,&flags));
}

static MagickBooleanType SetImageTileOffsetHandler(Image *image,const char *value,
  ExceptionInfo *magick_unused(exception))
{
  MagickStatusType
    flags;

  magick_unreferenced(exception);
  return(SetImageGeometryMember(value,&image->tile_offset,&flags));
}

static const ImagePropertyHandlerInfo
  ImagePropertyHandlers[] =
  {
    { "colorspace", SetImageColorspaceProperty },
    { "delay", SetImageDelayHandler },
    { "density", SetImageDensityHandler },
    { "depth", SetImageDepthHandler },
    { "gamma", SetImageGammaHandler },
    { "loop", SetImageLoopHandler },
    { "page", SetImagePageHandler },
    { "tile-offset", SetImageTileOffsetHandler }
  };

static ImagePropertyHandler GetImagePropertyHandler(const char *property)
{
  size_t
    i;

  for (i=0; i < (sizeof(ImagePropertyHandlers)/sizeof(*ImagePropertyHandlers)); i++)
    if (LocaleCompare(ImagePropertyHandlers[i].key,property) == 0)
      return(ImagePropertyHandlers[i].handler);
  return((ImagePropertyHandler) NULL);
}

MagickExport MagickBooleanType SetImageProperty(Image *image,
  const char *property,const char *value,ExceptionInfo *exception)
{
  ImagePropertyHandler
    handler;

  MagickBooleanType
    status;

  size_t
    property_length;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->properties == (void *) NULL)
    image->properties=NewSplayTree(CompareSplayTreeString,
      RelinquishMagickMemory,RelinquishMagickMemory);  /* create splay-tree */
  if (value == (const char *) NULL)
    return(DeleteImageProperty(image,property));  /* delete if NULL */
  if (strlen(property) <= 1)
    {
      /*
        Do not 'set' single letter properties - read only shorthand.
      */
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "SetReadOnlyProperty","`%s'",property);
      return(MagickFalse);
    }
  property_length=strlen(property);
  if (IsWildcardProperty(property,property_length) != MagickFalse)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "SetReadOnlyProperty","`%s'",property);
      return(MagickFalse);
    }
  /*
    FUTURE: binary chars or quotes in key should produce a error
    Set attributes with known names or special prefixes
    return result is found, or break to set a free form property
  */
  if (SetEnumeratedProperty(image,property,value,&status) != MagickFalse)
    return(status);
  status=MagickTrue;
  if (IsReadOnlyProperty(property) != MagickFalse)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "SetReadOnlyProperty","`%s'",property);
      return(MagickFalse);
    }
  handler=GetImagePropertyHandler(property);
  if (handler != (ImagePropertyHandler) NULL)
    return(handler(image,value,exception));
  switch (*property)
  {
#include "MagickCore/property-set-8bim-private.h"
    case 'B':
    case 'b':
    {
      if (LocaleCompare("background",property) == 0)
        {
          (void) QueryColorCompliance(value,AllCompliance,
            &image->background_color,exception);
          /* check for FUTURE: value exception?? */
          /* also add user input to splay tree */
        }
      break; /* not an attribute, add as a property */
    }
#include "MagickCore/property-set-exif-fx-private.h"
    case 'I':
    case 'i':
    {
#include "MagickCore/property-set-iptc-private.h"
      break; /* not an attribute, add as a property */
    }
    case 'P':
    case 'p':
    {
#include "MagickCore/property-set-pixel-private.h"
      break; /* not an attribute, add as a property */
    }
#include "MagickCore/property-set-xmp-private.h"
  }
  /* Default: not an attribute, add as a property */
  status=AddValueToSplayTree((SplayTreeInfo *) image->properties,
    ConstantString(property),ConstantString(value));
  /* FUTURE: error if status is bad? */
  return(status);
}
