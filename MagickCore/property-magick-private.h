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

  MagickCore private methods of property.c: the helpers of GetMagickProperty,
  from CloneMagickPropertyValue to the letter and name handlers and their
  tables, with the WarnNoImage macros; included by that file only, where the
  run stood.
*/
#ifndef MAGICKCORE_PROPERTY_MAGICK_PRIVATE_H
#define MAGICKCORE_PROPERTY_MAGICK_PRIVATE_H

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

#endif
