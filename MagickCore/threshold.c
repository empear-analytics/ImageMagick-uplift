/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%       TTTTT  H   H  RRRR   EEEEE  SSSSS  H   H   OOO   L      DDDD          %
%         T    H   H  R   R  E      SS     H   H  O   O  L      D   D         %
%         T    HHHHH  RRRR   EEE     SSS   HHHHH  O   O  L      D   D         %
%         T    H   H  R R    E         SS  H   H  O   O  L      D   D         %
%         T    H   H  R  R   EEEEE  SSSSS  H   H   OOO   LLLLL  DDDD          %
%                                                                             %
%                                                                             %
%                      MagickCore Image Threshold Methods                     %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                 October 1996                                %
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
#include "MagickCore/blob.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/configure.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/effect.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/shear.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"

/*
  Define declarations.
*/
#define ThresholdsFilename  "thresholds.xml"

/*
  Typedef declarations.
*/
struct _ThresholdMap
{
  char
    *map_id,
    *description;

  size_t
    width,
    height;

  ssize_t
    divisor,
    *levels;
};

/*
  Static declarations.
*/
#if MAGICKCORE_ZERO_CONFIGURATION_SUPPORT
  #include "MagickCore/threshold-map.h"
#else
static const char *const
  BuiltinMap=
    "<?xml version=\"1.0\"?>"
    "<thresholds>"
    "  <threshold map=\"threshold\" alias=\"1x1\">"
    "    <description>Threshold 1x1 (non-dither)</description>"
    "    <levels width=\"1\" height=\"1\" divisor=\"2\">"
    "        1"
    "    </levels>"
    "  </threshold>"
    "  <threshold map=\"checks\" alias=\"2x1\">"
    "    <description>Checkerboard 2x1 (dither)</description>"
    "    <levels width=\"2\" height=\"2\" divisor=\"3\">"
    "       1 2"
    "       2 1"
    "    </levels>"
    "  </threshold>"
    "</thresholds>";
#endif

/*
  Forward declarations.
*/
static ThresholdMap
  *GetThresholdMapFile(const char *,const char *,const char *,ExceptionInfo *);




/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  D e s t r o y T h r e s h o l d M a p                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyThresholdMap() de-allocate the given ThresholdMap
%
%  The format of the ListThresholdMaps method is:
%
%      ThresholdMap *DestroyThresholdMap(Threshold *map)
%
%  A description of each parameter follows.
%
%    o map:    Pointer to the Threshold map to destroy
%
*/
static void DestroyThresholdMapMembers(ThresholdMap *map)
{
  if (map->map_id != (char *) NULL)
    map->map_id=DestroyString(map->map_id);
  if (map->description != (char *) NULL)
    map->description=DestroyString(map->description);
  if (map->levels != (ssize_t *) NULL)
    map->levels=(ssize_t *) RelinquishMagickMemory(map->levels);
}

MagickExport ThresholdMap *DestroyThresholdMap(ThresholdMap *map)
{
  assert(map != (ThresholdMap *) NULL);
  DestroyThresholdMapMembers(map);
  map=(ThresholdMap *) RelinquishMagickMemory(map);
  return(map);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t T h r e s h o l d M a p                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetThresholdMap() loads and searches one or more threshold map files for the
%  map matching the given name or alias.
%
%  The format of the GetThresholdMap method is:
%
%      ThresholdMap *GetThresholdMap(const char *map_id,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o map_id:  ID of the map to look for.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport ThresholdMap *GetThresholdMap(const char *map_id,
  ExceptionInfo *exception)
{
  ThresholdMap
    *map;

  map=GetThresholdMapFile(BuiltinMap,"built-in",map_id,exception);
  if (map != (ThresholdMap *) NULL)
    return(map);
#if !MAGICKCORE_ZERO_CONFIGURATION_SUPPORT
  {
    const StringInfo
      *option;

    LinkedListInfo
      *options;

    options=GetConfigureOptions(ThresholdsFilename,exception);
    option=(const StringInfo *) GetNextValueInLinkedList(options);
    while (option != (const StringInfo *) NULL)
    {
      map=GetThresholdMapFile((const char *) GetStringInfoDatum(option),
        GetStringInfoPath(option),map_id,exception);
      if (map != (ThresholdMap *) NULL)
        break;
      option=(const StringInfo *) GetNextValueInLinkedList(options);
    }
    options=DestroyConfigureOptions(options);
  }
#endif
  return(map);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  G e t T h r e s h o l d M a p F i l e                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetThresholdMapFile() look for a given threshold map name or alias in the
%  given XML file data, and return the allocated the map when found.
%
%  The format of the ListThresholdMaps method is:
%
%      ThresholdMap *GetThresholdMap(const char *xml,const char *filename,
%         const char *map_id,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o xml:  The threshold map list in XML format.
%
%    o filename:  The threshold map XML filename.
%
%    o map_id:  ID of the map to look for in XML list.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SetThresholdMapId(ThresholdMap *map,XMLTreeInfo *threshold)
{
  const char
    *attribute;

  attribute=GetXMLTreeAttribute(threshold,"map");
  if (attribute != (char *) NULL)
    map->map_id=ConstantString(attribute);
}

static void ResetThresholdMap(ThresholdMap *map)
{
  map->map_id=(char *) NULL;
  map->description=(char *) NULL;
  map->levels=(ssize_t *) NULL;
}

static ThresholdMap *AcquireThresholdMap(XMLTreeInfo *threshold,
  XMLTreeInfo *description)
{
  const char
    *content;

  ThresholdMap
    *map;

  map=(ThresholdMap *) AcquireCriticalMemory(sizeof(*map));
  ResetThresholdMap(map);
  SetThresholdMapId(map,threshold);
  content=GetXMLTreeContent(description);
  if (content != (char *) NULL)
    map->description=ConstantString(content);
  return(map);
}

static inline MagickBooleanType IsThresholdMapAttribute(const char *attribute,
  const char *map_id)
{
  return(((attribute != (char *) NULL) &&
    (LocaleCompare(map_id,attribute) == 0)) ? MagickTrue : MagickFalse);
}

static XMLTreeInfo *FindThresholdMap(XMLTreeInfo *thresholds,
  const char *map_id)
{
  const char
    *attribute;

  XMLTreeInfo
    *threshold;

  for (threshold=GetXMLTreeChild(thresholds,"threshold");
       threshold != (XMLTreeInfo *) NULL;
       threshold=GetNextXMLTreeTag(threshold))
  {
    attribute=GetXMLTreeAttribute(threshold,"map");
    if (IsThresholdMapAttribute(attribute,map_id) != MagickFalse)
      break;
    attribute=GetXMLTreeAttribute(threshold,"alias");
    if (IsThresholdMapAttribute(attribute,map_id) != MagickFalse)
      break;
  }
  return(threshold);
}

static MagickBooleanType CheckThresholdLevel(const ThresholdMap *map,
  const ssize_t i,const char *map_id,ExceptionInfo *exception)
{
  if ((map->levels[i] < 0) || (map->levels[i] > map->divisor))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlInvalidContent", "<level> %.17g out of range, map \"%s\"",
        (double) map->levels[i],map_id);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType CheckThresholdLevelsEnd(const char *content,
  const char *map_id,ExceptionInfo *exception)
{
  char
    *p;

  double
    value;

  value=(double) strtol(content,&p,10);
  (void) value;
  if (p != content)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlInvalidContent", "<level> too many values, map \"%s\"",map_id);
     return(MagickFalse);
   }
  return(MagickTrue);
}

static MagickBooleanType ReadThresholdLevels(ThresholdMap *map,
  XMLTreeInfo *levels,const char *map_id,ExceptionInfo *exception)
{
  char
    *p;

  const char
    *content;

  ssize_t
    i;

  content=GetXMLTreeContent(levels);
  if (content == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingContent", "<levels>, map \"%s\"",map_id);
      return(MagickFalse);
    }
  map->levels=(ssize_t *) AcquireQuantumMemory((size_t) map->width,map->height*
    sizeof(*map->levels));
  if (map->levels == (ssize_t *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"UnableToAcquireThresholdMap");
  for (i=0; i < (ssize_t) (map->width*map->height); i++)
  {
    map->levels[i]=(ssize_t) strtol(content,&p,10);
    if (p == content)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "XmlInvalidContent", "<level> too few values, map \"%s\"",map_id);
        return(MagickFalse);
      }
    if (CheckThresholdLevel(map,i,map_id,exception) == MagickFalse)
      return(MagickFalse);
    content=p;
  }
  if (CheckThresholdLevelsEnd(content,map_id,exception) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType ReadThresholdLevelsDivisor(ThresholdMap *map,
  XMLTreeInfo *levels,const char *map_id,ExceptionInfo *exception)
{
  const char
    *attribute;

  attribute=GetXMLTreeAttribute(levels,"divisor");
  if (attribute == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingAttribute", "<levels divisor>, map \"%s\"",map_id);
      return(MagickFalse);
    }
  map->divisor=(ssize_t) StringToLong(attribute);
  if (map->divisor < 2)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlInvalidAttribute", "<levels divisor>, map \"%s\"",map_id);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType ReadThresholdLevelsSize(ThresholdMap *map,
  XMLTreeInfo *levels,const char *map_id,ExceptionInfo *exception)
{
  const char
    *attribute;

  attribute=GetXMLTreeAttribute(levels,"width");
  if (attribute == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingAttribute", "<levels width>, map \"%s\"",map_id);
      return(MagickFalse);
    }
  map->width=StringToUnsignedLong(attribute);
  if (map->width == 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
       "XmlInvalidAttribute", "<levels width>, map \"%s\"",map_id);
      return(MagickFalse);
    }
  attribute=GetXMLTreeAttribute(levels,"height");
  if (attribute == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingAttribute", "<levels height>, map \"%s\"",map_id);
      return(MagickFalse);
    }
  map->height=StringToUnsignedLong(attribute);
  if (map->height == 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlInvalidAttribute", "<levels height>, map \"%s\"",map_id);
      return(MagickFalse);
    }
  if (ReadThresholdLevelsDivisor(map,levels,map_id,exception) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static ThresholdMap *GetThresholdMapFile(const char *xml,const char *filename,
  const char *map_id,ExceptionInfo *exception)
{
  ThresholdMap
    *map;

  XMLTreeInfo
    *description,
    *levels,
    *threshold,
    *thresholds;

  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Loading threshold map file \"%s\" ...",filename);
  map=(ThresholdMap *) NULL;
  thresholds=NewXMLTree(xml,exception);
  if (thresholds == (XMLTreeInfo *) NULL)
    return(map);
  threshold=FindThresholdMap(thresholds,map_id);
  if (threshold == (XMLTreeInfo *) NULL)
    {
      thresholds=DestroyXMLTree(thresholds);
      return(map);
    }
  description=GetXMLTreeChild(threshold,"description");
  if (description == (XMLTreeInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingElement", "<description>, map \"%s\"",map_id);
      thresholds=DestroyXMLTree(thresholds);
      return(map);
    }
  levels=GetXMLTreeChild(threshold,"levels");
  if (levels == (XMLTreeInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingElement", "<levels>, map \"%s\"", map_id);
      thresholds=DestroyXMLTree(thresholds);
      return(map);
    }
  map=AcquireThresholdMap(threshold,description);
  if (ReadThresholdLevelsSize(map,levels,map_id,exception) == MagickFalse)
    {
      thresholds=DestroyXMLTree(thresholds);
      map=DestroyThresholdMap(map);
      return(map);
    }
  if (ReadThresholdLevels(map,levels,map_id,exception) == MagickFalse)
    {
      thresholds=DestroyXMLTree(thresholds);
      map=DestroyThresholdMap(map);
      return(map);
    }
  thresholds=DestroyXMLTree(thresholds);
  return(map);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  L i s t T h r e s h o l d M a p F i l e                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListThresholdMapFile() lists the threshold maps and their descriptions
%  in the given XML file data.
%
%  The format of the ListThresholdMaps method is:
%
%      MagickBooleanType ListThresholdMaps(FILE *file,const char*xml,
%         const char *filename,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to the output FILE.
%
%    o xml:  The threshold map list in XML format.
%
%    o filename:  The threshold map XML filename.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void PrintThresholdMapHeader(FILE *file)
{
  (void) FormatLocaleFile(file,"%-16s %-12s %s\n","Map","Alias","Description");
  (void) FormatLocaleFile(file,
    "----------------------------------------------------\n");
}

MagickBooleanType ListThresholdMapFile(FILE *file,const char *xml,
  const char *filename,ExceptionInfo *exception)
{
  const char
    *alias,
    *content,
    *map;

  XMLTreeInfo
    *description,
    *threshold,
    *thresholds;

  assert( xml != (char *) NULL );
  assert( file != (FILE *) NULL );
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Loading threshold map file \"%s\" ...",filename);
  thresholds=NewXMLTree(xml,exception);
  if ( thresholds == (XMLTreeInfo *) NULL )
    return(MagickFalse);
  PrintThresholdMapHeader(file);
  threshold=GetXMLTreeChild(thresholds,"threshold");
  for ( ; threshold != (XMLTreeInfo *) NULL;
          threshold=GetNextXMLTreeTag(threshold))
  {
    map=GetXMLTreeAttribute(threshold,"map");
    if (map == (char *) NULL)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "XmlMissingAttribute", "<map>");
        thresholds=DestroyXMLTree(thresholds);
        return(MagickFalse);
      }
    alias=GetXMLTreeAttribute(threshold,"alias");
    description=GetXMLTreeChild(threshold,"description");
    if (description == (XMLTreeInfo *) NULL)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "XmlMissingElement", "<description>, map \"%s\"",map);
        thresholds=DestroyXMLTree(thresholds);
        return(MagickFalse);
      }
    content=GetXMLTreeContent(description);
    if (content == (char *) NULL)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "XmlMissingContent", "<description>, map \"%s\"", map);
        thresholds=DestroyXMLTree(thresholds);
        return(MagickFalse);
      }
    (void) FormatLocaleFile(file,"%-16s %-12s %s\n",map,alias ? alias : "",
      content);
  }
  thresholds=DestroyXMLTree(thresholds);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L i s t T h r e s h o l d M a p s                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListThresholdMaps() lists the threshold maps and their descriptions
%  as defined by "threshold.xml" to a file.
%
%  The format of the ListThresholdMaps method is:
%
%      MagickBooleanType ListThresholdMaps(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to the output FILE.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ListThresholdMaps(FILE *file,
  ExceptionInfo *exception)
{
  const StringInfo
    *option;

  LinkedListInfo
    *options;

  MagickStatusType
    status;

  status=MagickTrue;
  if (file == (FILE *) NULL)
    file=stdout;
  options=GetConfigureOptions(ThresholdsFilename,exception);
  (void) FormatLocaleFile(file,
    "\n   Threshold Maps for Ordered Dither Operations\n");
  option=(const StringInfo *) GetNextValueInLinkedList(options);
  while (option != (const StringInfo *) NULL)
  {
    (void) FormatLocaleFile(file,"\nPath: %s\n\n",GetStringInfoPath(option));
    status&=(MagickStatusType) ListThresholdMapFile(file,(const char *)
      GetStringInfoDatum(option),GetStringInfoPath(option),exception);
    option=(const StringInfo *) GetNextValueInLinkedList(options);
  }
  options=DestroyConfigureOptions(options);
  return(status != 0 ? MagickTrue : MagickFalse);
}

