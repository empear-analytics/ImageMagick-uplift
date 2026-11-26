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

  MagickCore private methods of property.c: the profile byte readers and the
  8BIM (Photoshop resource) property reader, from ReadPropertyByte to
  Get8BIMProperty; included by that file only, where the run stood.
*/
#ifndef MAGICKCORE_PROPERTY_8BIM_PRIVATE_H
#define MAGICKCORE_PROPERTY_8BIM_PRIVATE_H

static inline int ReadPropertyByte(const unsigned char **p,size_t *length)
{
  int
    c;

  if (*length < 1)
    return(EOF);
  c=(int) (*(*p)++);
  (*length)--;
  return(c);
}

static inline void ReadPropertyBuffer(const unsigned char **p,size_t *length,
  unsigned char *buffer,const ssize_t count)
{
  int
    c;

  ssize_t
    i;

  for (i=0; i < count; i++)
  {
    c=(int) (*(*p)++);
    (*length)--;
    buffer[i]=(unsigned char) c;
  }
}

static inline signed int ReadPropertyMSBLong(const unsigned char **p,
  size_t *length)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  unsigned char
    buffer[4];

  unsigned int
    value;

  if (*length < 4)
    return(-1);
  ReadPropertyBuffer(p,length,buffer,4);
  value=(unsigned int) buffer[0] << 24;
  value|=(unsigned int) buffer[1] << 16;
  value|=(unsigned int) buffer[2] << 8;
  value|=(unsigned int) buffer[3];
  quantum.unsigned_value=value & 0xffffffff;
  return(quantum.signed_value);
}

static inline signed short ReadPropertyMSBShort(const unsigned char **p,
  size_t *length)
{
  union
  {
    unsigned short
      unsigned_value;

    signed short
      signed_value;
  } quantum;

  unsigned char
    buffer[2];

  unsigned short
    value;

  if (*length < 2)
    return(-1);
  ReadPropertyBuffer(p,length,buffer,2);
  value=(unsigned short) buffer[0] << 8;
  value|=(unsigned short) buffer[1];
  quantum.unsigned_value=value & 0xffff;
  return(quantum.signed_value);
}

static inline MagickBooleanType IsInvalid8BIMFieldCount(const ssize_t count)
{
  return(((count != 2) && (count != 3) && (count != 4)) ? MagickTrue :
    MagickFalse);
}

typedef struct _Match8BIM
{
  const char
    *name;

  char
    *resource,
    *macroman_resource;

  ssize_t
    count;
} Match8BIM;

static inline MagickBooleanType IsUnrequestedResource(const Match8BIM *match)
{
  const char
    *const resource = match->resource,
    *const macroman_resource = match->macroman_resource,
    *const name = match->name;

  return(((resource == (char *) NULL) || (macroman_resource == (char *) NULL) ||
    ((LocaleCompare(name,resource) != 0) &&
    (LocaleCompare(name,macroman_resource) != 0))) ? MagickTrue : MagickFalse);
}

typedef struct _AttributeInfo8BIM
{
  const Image
    *image;

  const char
    *key;

  char
    *format;

  ssize_t
    count,
    id;

  ExceptionInfo
    *exception;
} AttributeInfo8BIM;

static char *Trace8BIMClippath(const AttributeInfo8BIM *attribute_info,
  char *attribute)
{
  const Image
    *const image = attribute_info->image;

  const char
    *const format = attribute_info->format;

  const ssize_t
    count = attribute_info->count;

  char
    *path;

  if (LocaleCompare(format,"svg") == 0)
    path=TraceSVGClippath((unsigned char *) attribute,(size_t) count,
      image->columns,image->rows);
  else
    path=TracePSClippath((unsigned char *) attribute,(size_t) count);
  return(path);
}

typedef struct _Cursor8BIM
{
  const unsigned char
    **info;

  size_t
    *length;
} Cursor8BIM;

static void Read8BIMResourceBytes(char **resource,const Cursor8BIM *cursor,
  const ssize_t count)
{
  const unsigned char
    **const info = cursor->info;

  size_t
    *const length = cursor->length;

  ssize_t
    i;

  for (i=0; i < (ssize_t) count; i++)
    (*resource)[i]=(char) ReadPropertyByte(&(*info),&(*length));
  (*resource)[count]='\0';
}

static void Read8BIMResourceName(char **resource,const Cursor8BIM *cursor,
  const ssize_t count)
{
  size_t
    *const length = cursor->length;

  if ((count != 0) && ((size_t) count <= (*length)))
    {
      if (~((size_t) count) >= (MagickPathExtent-1))
        *resource=(char *) AcquireQuantumMemory((size_t) count+
          MagickPathExtent,sizeof(*(*resource)));
      if (*resource != (char *) NULL)
        Read8BIMResourceBytes(resource,cursor,count);
    }
}

static void Set8BIMAttribute(const AttributeInfo8BIM *attribute_info,
  char *attribute)
{
  const Image
    *const image = attribute_info->image;

  const char
    *const key = attribute_info->key;

  const ssize_t
    id = attribute_info->id;

  ExceptionInfo
    *const exception = attribute_info->exception;

  if ((id <= 1999) || (id >= 2999))
    (void) SetImageProperty((Image *) image,key,(const char *) attribute,
      exception);
  else
    {
      char
        *path;

      path=Trace8BIMClippath(attribute_info,attribute);
      (void) SetImageProperty((Image *) image,key,(const char *) path,
        exception);
      path=DestroyString(path);
    }
}

static inline MagickBooleanType Is8BIMResourceName(const char *name)
{
  return(((*name != '\0') && (*name != '#')) ? MagickTrue : MagickFalse);
}

typedef struct _Names8BIM
{
  char
    **resource,
    **macroman_resource;
} Names8BIM;

static void Release8BIMResourceNames(const Names8BIM *names)
{
  char
    **const resource = names->resource,
    **const macroman_resource = names->macroman_resource;

  if ((*macroman_resource) != (char *) NULL)
    (*macroman_resource)=DestroyString((*macroman_resource));
  if ((*resource) != (char *) NULL)
    (*resource)=DestroyString((*resource));
}

static void Skip8BIMResourceData(const Cursor8BIM *cursor,const ssize_t count)
{
  const unsigned char
    **const info = cursor->info;

  size_t
    *const length = cursor->length;

  (*info)+=count;
  (*length)=(size_t) ((ssize_t) (*length)-MagickMin(count,(ssize_t) (*length)));
}

typedef struct _Range8BIM
{
  long
    start,
    stop;
} Range8BIM;

static MagickBooleanType Skip8BIMResourceHeader(const Range8BIM *range,
  const Cursor8BIM *cursor,ssize_t *id)
{
  const unsigned char
    **const info = cursor->info;

  size_t
    *const length = cursor->length;

  const long
    start = range->start,
    stop = range->stop;

  if (ReadPropertyByte(info,length) != (unsigned char) '8')
    return(MagickTrue);
  if (ReadPropertyByte(info,length) != (unsigned char) 'B')
    return(MagickTrue);
  if (ReadPropertyByte(info,length) != (unsigned char) 'I')
    return(MagickTrue);
  if (ReadPropertyByte(info,length) != (unsigned char) 'M')
    return(MagickTrue);
  (*id)=(ssize_t) ReadPropertyMSBShort(info,length);
  if ((*id) < (ssize_t) start)
    return(MagickTrue);
  if ((*id) > (ssize_t) stop)
    return(MagickTrue);
  return(MagickFalse);
}

static MagickBooleanType Skip8BIMUnmatchedResource(const Match8BIM *match,
  const Cursor8BIM *cursor,ssize_t *sub_number)
{
  const char
    *const name = match->name;

  const ssize_t
    count = match->count;

  if ((Is8BIMResourceName(name) != MagickFalse) &&
      (IsUnrequestedResource(match) != MagickFalse))
    {
      /*
        No name match, scroll forward and try next.
      */
      Skip8BIMResourceData(cursor,count);
      return(MagickTrue);
    }
  if ((*name == '#') && ((*sub_number) != 1))
    {
      /*
        No numbered match, scroll forward and try next.
      */
      (*sub_number)--;
      Skip8BIMResourceData(cursor,count);
      return(MagickTrue);
    }
  return(MagickFalse);
}

static void Set8BIMResource(const AttributeInfo8BIM *attribute_info,
  const Cursor8BIM *cursor,MagickBooleanType *status)
{
  const unsigned char
    **const info = cursor->info;

  char
    *attribute;

  const ssize_t
    count = attribute_info->count;

  attribute=(char *) NULL;
  if (~((size_t) count) >= (MagickPathExtent-1))
    attribute=(char *) AcquireQuantumMemory((size_t) count+MagickPathExtent,
      sizeof(*attribute));
  if (attribute != (char *) NULL)
    {
      (void) memcpy(attribute,(char *) (*info),(size_t) count);
      attribute[count]='\0';
      Skip8BIMResourceData(cursor,count);
      Set8BIMAttribute(attribute_info,attribute);
      attribute=DestroyString(attribute);
      (*status)=MagickTrue;
    }
}

static void Read8BIMResourceBlock(ssize_t *count,char **resource,
  const Cursor8BIM *cursor)
{
  const unsigned char
    **const info = cursor->info;

  size_t
    *const length = cursor->length;

  (*count)=(ssize_t) ReadPropertyByte(info,length);
  Read8BIMResourceName(resource,cursor,(*count));
  if (((*count) & 0x01) == 0)
    (void) ReadPropertyByte(info,length);
  (*count)=(ssize_t) ReadPropertyMSBLong(info,length);
}

static inline MagickBooleanType IsInvalid8BIMResourceCount(const ssize_t count,
  const size_t length)
{
  return(((count < 0) || ((size_t) count > length)) ? MagickTrue :
    MagickFalse);
}

static void Convert8BIMResourceName(char *resource,char **macroman_resource)
{
  if (resource != (char *) NULL)
    (*macroman_resource)=(char *) ConvertMacRomanToUTF8((unsigned char *)
      resource);
}

static inline MagickBooleanType Is8BIMScanPending(const size_t length,
  const MagickBooleanType status)
{
  return(((length > 0) && (status == MagickFalse)) ? MagickTrue : MagickFalse);
}

typedef struct _Request8BIM
{
  const Image
    *image;

  const char
    *key;

  ExceptionInfo
    *exception;

  const StringInfo
    *profile;

  long
    start,
    stop;

  const char
    *name;

  char
    *format;

  ssize_t
    sub_number;
} Request8BIM;

static void Get8BIMResources(const Request8BIM *request)
{
  char
    *macroman_resource = (char *) NULL,
    *resource = (char *) NULL;

  const unsigned char
    *info;

  MagickBooleanType
    status;

  size_t
    length;

  ssize_t
    count,
    id,
    sub_number = request->sub_number;

  const Cursor8BIM
    cursor = { &info, &length };

  const Names8BIM
    names = { &resource, &macroman_resource };

  status=MagickFalse;
  length=GetStringInfoLength(request->profile);
  info=GetStringInfoDatum(request->profile);
  while (Is8BIMScanPending(length,status) != MagickFalse)
  {
    AttributeInfo8BIM
      attribute_info;

    Match8BIM
      match;

    Range8BIM
      range;

    range.start=request->start;
    range.stop=request->stop;
    if (Skip8BIMResourceHeader(&range,&cursor,&id) != MagickFalse)
      continue;
    Release8BIMResourceNames(&names);
    Read8BIMResourceBlock(&count,&resource,&cursor);
    if (IsInvalid8BIMResourceCount(count,length) != MagickFalse)
      {
        length=0;
        continue;
      }
    Convert8BIMResourceName(resource,&macroman_resource);
    match.name=request->name;
    match.resource=resource;
    match.macroman_resource=macroman_resource;
    match.count=count;
    if (Skip8BIMUnmatchedResource(&match,&cursor,&sub_number) !=
        MagickFalse)
      continue;
    /*
      We have the resource of interest.
    */
    attribute_info.image=request->image;
    attribute_info.key=request->key;
    attribute_info.format=request->format;
    attribute_info.count=count;
    attribute_info.id=id;
    attribute_info.exception=request->exception;
    Set8BIMResource(&attribute_info,&cursor,&status);
  }
  Release8BIMResourceNames(&names);
}

static void Get8BIMProperty(const Image *image,const char *key,
  ExceptionInfo *exception)
{
  char
    format[MagickPathExtent],
    name[MagickPathExtent];

  const StringInfo
    *profile;

  long
    start,
    stop;

  Request8BIM
    request;

  ssize_t
    count,
    sub_number;

  /*
    There are no newlines in path names, so it's safe as terminator.
  */
  profile=GetImageProfile(image,"8bim");
  if (profile == (StringInfo *) NULL)
    return;
  count=(ssize_t) MagickSscanf(key,"8BIM:%ld,%ld:%1024[^\n]\n%1024[^\n]",
    &start,&stop,name,format);
  if (IsInvalid8BIMFieldCount(count) != MagickFalse)
    return;
  if (count < 4)
    (void) CopyMagickString(format,"SVG",MagickPathExtent);
  if (count < 3)
    *name='\0';
  sub_number=1;
  if (*name == '#')
    sub_number=(ssize_t) StringToLong(&name[1]);
  sub_number=MagickMax(sub_number,1L);
  request.image=image;
  request.key=key;
  request.exception=exception;
  request.profile=profile;
  request.start=start;
  request.stop=stop;
  request.name=name;
  request.format=format;
  request.sub_number=sub_number;
  Get8BIMResources(&request);
}

#endif
