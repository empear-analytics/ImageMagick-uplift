/*
  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.  You may
  obtain a copy of the License at

    https://imagemagick.org/license/

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore private methods of property.c: the 8BIM clip path tracers, from
  IsPointCorner to TraceSVGClippath (PostScript and SVG output); included by
  that file only, where the run stood.
*/
#ifndef MAGICKCORE_PROPERTY_CLIPPATH_PRIVATE_H
#define MAGICKCORE_PROPERTY_CLIPPATH_PRIVATE_H

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

#endif
