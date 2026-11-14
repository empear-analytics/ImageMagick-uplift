/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               PPPP   RRRR    OOO   FFFFF  IIIII  L      EEEEE               %
%               P   P  R   R  O   O  F        I    L      E                   %
%               PPPP   RRRR   O   O  FFF      I    L      EEE                 %
%               P      R R    O   O  F        I    L      E                   %
%               P      R  R    OOO   F      IIIII  LLLLL  EEEEE               %
%                                                                             %
%                         SSSSS  Y   Y  N   N   CCCC                          %
%                         SS      Y Y   NN  N  C                              %
%                          SSS     Y    N N N  C                              %
%                            SS    Y    N  NN  C                              %
%                         SSSSS    Y    N   N   CCCC                          %
%                                                                             %
%                                                                             %
%                                Sync Profiles                                %
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
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/color.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/configure.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/option-private.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile.h"
#include "MagickCore/profile-private.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#if defined(MAGICKCORE_LCMS_DELEGATE)
#include <wchar.h>
#if defined(MAGICKCORE_HAVE_LCMS_LCMS2_H)
#include <lcms/lcms2.h>
#else
#include "lcms2.h"
#endif
#endif
#if defined(MAGICKCORE_XML_DELEGATE)
#  include <libxml/parser.h>
#  include <libxml/tree.h>
#endif

/*
  Typedef declarations.
*/
typedef struct _ClipPathResizeInfo
{
  size_t
    old_columns,
    old_rows;

  const RectangleInfo
    *new_geometry;
} ClipPathResizeInfo;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S y n c I m a g e P r o f i l e s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncImageProfiles() synchronizes image properties with the image profiles.
%
%  The format of the SyncImageProfiles method is:
%
%      void SyncImageProfiles(Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/

static inline int ReadProfileByte(unsigned char **p,size_t *length)
{
  int
    c;

  if (*length < 1)
    return(EOF);
  c=(int) (*(*p)++);
  (*length)--;
  return(c);
}

static inline signed short ReadProfileShort(const EndianType endian,
  unsigned char *buffer)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  unsigned short
    value;

  if (endian == LSBEndian)
    {
      value=(unsigned short) buffer[1] << 8;
      value|=(unsigned short) buffer[0];
      quantum.unsigned_value=value & 0xffff;
      return((signed short) quantum.signed_value);
    }
  value=(unsigned short) buffer[0] << 8;
  value|=(unsigned short) buffer[1];
  quantum.unsigned_value=value & 0xffff;
  return((signed short) quantum.signed_value);
}

static inline signed int ReadProfileLong(const EndianType endian,
  unsigned char *buffer)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  unsigned int
    value;

  if (endian == LSBEndian)
    {
      value=(unsigned int) buffer[3] << 24;
      value|=(unsigned int) buffer[2] << 16;
      value|=(unsigned int) buffer[1] << 8;
      value|=(unsigned int) buffer[0];
      quantum.unsigned_value=value & 0xffffffff;
      return(quantum.signed_value);
    }
  value=(unsigned int) buffer[0] << 24;
  value|=(unsigned int) buffer[1] << 16;
  value|=(unsigned int) buffer[2] << 8;
  value|=(unsigned int) buffer[3];
  quantum.unsigned_value=value & 0xffffffff;
  return(quantum.signed_value);
}

static inline signed int ReadProfileMSBLong(unsigned char **p,size_t *length)
{
  signed int
    value;

  if (*length < 4)
    return(0);
  value=ReadProfileLong(MSBEndian,*p);
  (*length)-=4;
  *p+=4;
  return(value);
}

static inline signed short ReadProfileMSBShort(unsigned char **p,
  size_t *length)
{
  signed short
    value;

  if (*length < 2)
    return(0);
  value=ReadProfileShort(MSBEndian,*p);
  (*length)-=2;
  *p+=2;
  return(value);
}

static inline void WriteProfileLong(const EndianType endian,
  const size_t value,unsigned char *p)
{
  unsigned char
    buffer[4];

  if (endian == LSBEndian)
    {
      buffer[0]=(unsigned char) value;
      buffer[1]=(unsigned char) (value >> 8);
      buffer[2]=(unsigned char) (value >> 16);
      buffer[3]=(unsigned char) (value >> 24);
      (void) memcpy(p,buffer,4);
      return;
    }
  buffer[0]=(unsigned char) (value >> 24);
  buffer[1]=(unsigned char) (value >> 16);
  buffer[2]=(unsigned char) (value >> 8);
  buffer[3]=(unsigned char) value;
  (void) memcpy(p,buffer,4);
}

static void WriteProfileShort(const EndianType endian,
  const unsigned short value,unsigned char *p)
{
  unsigned char
    buffer[2];

  if (endian == LSBEndian)
    {
      buffer[0]=(unsigned char) value;
      buffer[1]=(unsigned char) (value >> 8);
      (void) memcpy(p,buffer,2);
      return;
    }
  buffer[0]=(unsigned char) (value >> 8);
  buffer[1]=(unsigned char) value;
  (void) memcpy(p,buffer,2);
}

static inline MagickBooleanType IsUnwritableDimension(const ssize_t format,
  const int components)
{
  return(((components != 1) || ((format != 3) && (format != 4))) ? MagickTrue :
    MagickFalse);
}

#define MaxDirectoryStack  16
#define EXIF_DELIMITER  "\n"
#define EXIF_NUM_FORMATS  12
#define TAG_IMAGE_WIDTH  0x0100
#define TAG_IMAGE_LENGTH  0x0101
#define TAG_EXIF_OFFSET  0x8769
#define TAG_INTEROP_OFFSET  0xa005
#define TAG_PIXEL_X_DIMENSION  0xa002
#define TAG_PIXEL_Y_DIMENSION  0xa003

static inline MagickBooleanType IsExifDirectoryOffset(
  const unsigned short tag_value,const ssize_t format,const int components,
  const size_t number_bytes)
{
  return((((tag_value == TAG_EXIF_OFFSET) ||
    (tag_value == TAG_INTEROP_OFFSET)) && (format == 4) &&
    (components == 1) && (number_bytes == 4)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsNextDirectoryOffset(const ssize_t offset,
  const size_t length,const ssize_t level)
{
  return(((offset != 0) && ((size_t) offset < (length-1)) &&
    (level < (MaxDirectoryStack-2))) ? MagickTrue : MagickFalse);
}

typedef struct _ExifTagInfo
{
  const Image
    *image;

  EndianType
    endian;

  unsigned char
    *directory,
    *ifd0;

  unsigned short
    tag_value;

  ssize_t
    format;

  int
    components;

  size_t
    number_bytes;

  unsigned char
    *q,
    *p;
} ExifTagInfo;

static void SyncExifResolution(const ExifTagInfo *exif_tag,
  const double resolution)
{
  const EndianType
    endian = exif_tag->endian;

  unsigned char
    *const directory = exif_tag->directory;

  unsigned char
    *const ifd0 = exif_tag->ifd0;

  const size_t
    number_bytes = exif_tag->number_bytes;

  unsigned char
    *const p = exif_tag->p;

  if (directory != ifd0)
    return;
  (void) WriteProfileLong(endian,(size_t) (resolution+0.5),p);
  if (number_bytes == 8)
    (void) WriteProfileLong(endian,1UL,p+4);
}

static void SyncExifOrientation(const ExifTagInfo *exif_tag)
{
  const Image
    *const image = exif_tag->image;

  const EndianType
    endian = exif_tag->endian;

  unsigned char
    *const directory = exif_tag->directory;

  unsigned char
    *const ifd0 = exif_tag->ifd0;

  const size_t
    number_bytes = exif_tag->number_bytes;

  unsigned char
    *const p = exif_tag->p;

  if (directory != ifd0)
    return;
  if (number_bytes == 4)
    {
      (void) WriteProfileLong(endian,(size_t) image->orientation,p);
      return;
    }
  (void) WriteProfileShort(endian,(unsigned short) image->orientation,
    p);
}

static void SyncExifIfd0Tag(const unsigned short tag_value,
  const ExifTagInfo exif_tag,const Image *image)
{
  switch (tag_value)
  {
    case 0x011a:
    {
      SyncExifResolution(&exif_tag,image->resolution.x);
      break;
    }
    case 0x011b:
    {
      SyncExifResolution(&exif_tag,image->resolution.y);
      break;
    }
    case 0x0112:
    {
      SyncExifOrientation(&exif_tag);
      break;
    }
    default:
      break;
  }
}

typedef struct _ExifEntryInfo
{
  EndianType
    endian;

  unsigned char
    *exif;

  size_t
    length,
    number_bytes;

  unsigned char
    *q;
} ExifEntryInfo;

static MagickBooleanType LocateExifEntryValue(const ExifEntryInfo *exif_entry,
  unsigned char **p)
{
  const EndianType
    endian = exif_entry->endian;

  unsigned char
    *const exif = exif_entry->exif;

  const size_t
    length = exif_entry->length,
    number_bytes = exif_entry->number_bytes;

  unsigned char
    *const q = exif_entry->q;

  ssize_t
    offset;

  if (number_bytes <= 4)
    *p=q+8;
  else
    {
      /*
        The directory entry contains an offset.
      */
      offset=(ssize_t) ReadProfileLong(endian,q+8);
      if ((offset < 0) ||
          ((size_t) (offset+(ssize_t) number_bytes) > length))
        return(MagickFalse);
      if (~length < number_bytes)
        return(MagickFalse);  /* prevent overflow */
      *p=(unsigned char *) (exif+offset);
    }
  return(MagickTrue);
}

typedef struct _DirectoryInfo
{
  unsigned char
    *directory;

  size_t
    entry;
} DirectoryInfo;

static inline void WriteProfileDimension(const ExifTagInfo *exif_tag,
  const size_t dimension)
{
  const EndianType
    endian = exif_tag->endian;

  const ssize_t
    format = exif_tag->format;

  const int
    components = exif_tag->components;

  unsigned char
    *const entry = exif_tag->q;

  unsigned char
    *const value = exif_tag->p;

  if (IsUnwritableDimension(format,components) != MagickFalse)
    return;
  if ((format == 3) && (dimension <= MAGICK_USHORT_MAX))
    {
      WriteProfileShort(endian,(unsigned short) dimension,value);
      return;
    }
  if (dimension > UINT32_MAX)
    return;
  if (format == 3)
    WriteProfileShort(endian,4,entry+2);
  WriteProfileLong(endian,dimension,value);
}

static void SyncExifDimension(const ExifTagInfo *exif_tag,unsigned char *ifd,
  const int width_tag)
{
  const Image
    *const image = exif_tag->image;

  unsigned char
    *const directory = exif_tag->directory;

  const unsigned short
    tag_value = exif_tag->tag_value;

  if (directory == ifd)
    WriteProfileDimension(exif_tag,tag_value == width_tag ?
      image->columns : image->rows);
}

static MagickBooleanType ReadExifMarker(unsigned char **exif,size_t *length,
  MagickBooleanType *broke)
{
  (*broke)=MagickFalse;
  if (ReadProfileByte(exif,length) != 0x45)
    return(MagickFalse);
  if (ReadProfileByte(exif,length) != 0x78)
    return(MagickFalse);
  if (ReadProfileByte(exif,length) != 0x69)
    return(MagickFalse);
  if (ReadProfileByte(exif,length) != 0x66)
    return(MagickFalse);
  if (ReadProfileByte(exif,length) != 0x00)
    return(MagickFalse);
  if (ReadProfileByte(exif,length) != 0x00)
    return(MagickFalse);
  (*broke)=MagickTrue;
  return(MagickTrue);
}

static void SkipToExifHeader(unsigned char **exif,size_t *length)
{
  MagickBooleanType
    broke = MagickFalse;

  while ((*length) != 0)
  {
    if (ReadExifMarker(exif,length,&broke) == MagickFalse)
      continue;
    if (broke != MagickFalse)
      break;
  }
}

static MagickBooleanType ReadExifHeader(unsigned char **exif,size_t *length,
  EndianType *endian)
{
  ssize_t
    id;

  if ((*length) < 16)
    return(MagickFalse);
  id=(ssize_t) ReadProfileShort(LSBEndian,(*exif));
  if ((id != 0x4949) && (id != 0x4D4D))
    {
      SkipToExifHeader(exif,length);
      if ((*length) < 16)
        return(MagickFalse);
      id=(ssize_t) ReadProfileShort(LSBEndian,(*exif));
    }
  (*endian)=LSBEndian;
  if (id == 0x4949)
    (*endian)=LSBEndian;
  else
    if (id == 0x4D4D)
      (*endian)=MSBEndian;
    else
      return(MagickFalse);
  if (ReadProfileShort((*endian),(*exif)+2) != 0x002a)
    return(MagickFalse);
  return(MagickTrue);
}

static void SyncExifResolutionUnit(const ExifTagInfo *exif_tag)
{
  if (exif_tag->directory != exif_tag->ifd0)
    return;
  if (exif_tag->number_bytes == 4)
    {
      (void) WriteProfileLong(exif_tag->endian,((size_t)
        exif_tag->image->units)+1,exif_tag->p);
      return;
    }
  (void) WriteProfileShort(exif_tag->endian,(unsigned short)
    (exif_tag->image->units+1),exif_tag->p);
}

static void SyncExifTag(const ExifTagInfo *exif_tag,
  unsigned char *exif_directory)
{
  const Image
    *const image = exif_tag->image;

  unsigned char
    *const ifd0 = exif_tag->ifd0;

  const unsigned short
    tag_value = exif_tag->tag_value;

  switch (tag_value)
  {
    case TAG_IMAGE_WIDTH:
    case TAG_IMAGE_LENGTH:
    {
      SyncExifDimension(exif_tag,ifd0,TAG_IMAGE_WIDTH);
      break;
    }
    case TAG_PIXEL_X_DIMENSION:
    case TAG_PIXEL_Y_DIMENSION:
    {
      SyncExifDimension(exif_tag,exif_directory,TAG_PIXEL_X_DIMENSION);
      break;
    }
    case 0x0128:
    {
      SyncExifResolutionUnit(exif_tag);
      break;
    }
    default:
    {
      SyncExifIfd0Tag(tag_value,*exif_tag,image);
      break;
    }
  }
}

typedef struct _ExifWalkInfo
{
  const Image
    *image;

  EndianType
    endian;

  unsigned char
    *exif;

  size_t
    length;

  unsigned char
    *directory,
    *ifd0;

  SplayTreeInfo
    *exif_resources;
} ExifWalkInfo;

typedef struct _ExifWalkTargets
{
  ssize_t
    *level;

  size_t
    *entry;

  unsigned char
    **exif_directory;

  DirectoryInfo
    *directory_stack;
} ExifWalkTargets;

typedef struct _ExifEntryTargets
{
  unsigned char
    **q;

  unsigned short
    *tag_value;

  ssize_t
    *format;

  int
    *components;

  size_t
    *number_bytes;
} ExifEntryTargets;

static MagickBooleanType ReadExifEntry(const ExifWalkInfo *walk,
  const size_t entry,const ExifEntryTargets *targets)
{
  const EndianType
    endian = walk->endian;

  unsigned char
    *const exif = walk->exif;

  const size_t
    length = walk->length;

  unsigned char
    *const directory = walk->directory;

  SplayTreeInfo
    *const exif_resources = walk->exif_resources;

  unsigned char
    **const q = targets->q;

  unsigned short
    *const tag_value = targets->tag_value;

  ssize_t
    *const format = targets->format;

  int
    *const components = targets->components;

  size_t
    *const number_bytes = targets->number_bytes;

  static int
    format_bytes[] = {0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8};

  (*q)=(unsigned char *) (directory+2+(12*entry));
  if ((*q) > (exif+length-12))
    return(MagickFalse);  /* corrupt EXIF */
  if (GetValueFromSplayTree(exif_resources,(*q)) == (*q))
    return(MagickFalse);
  (void) AddValueToSplayTree(exif_resources,(*q),(*q));
  (*tag_value)=(unsigned short) ReadProfileShort(endian,(*q));
  (*format)=(ssize_t) ReadProfileShort(endian,(*q)+2);
  if (((*format) < 0) || (((*format)-1) >= EXIF_NUM_FORMATS))
    return(MagickFalse);
  (*components)=(int) ReadProfileLong(endian,(*q)+4);
  if ((*components) < 0)
    return(MagickFalse);  /* corrupt EXIF */
  (*number_bytes)=(size_t) (*components)*(size_t) format_bytes[(*format)];
  if ((ssize_t) (*number_bytes) < (*components))
    return(MagickFalse);  /* prevent overflow */
  return(MagickTrue);
}

static MagickBooleanType PushExifDirectory(const ExifWalkInfo *walk,
  const ExifTagInfo *exif_tag,const size_t number_entries,
  const ExifWalkTargets *targets)
{
  const EndianType
    endian = walk->endian;

  unsigned char
    *const exif = walk->exif;

  const size_t
    length = walk->length;

  unsigned char
    *const directory = walk->directory,
    *const ifd0 = walk->ifd0;

  const unsigned short
    tag_value = exif_tag->tag_value;

  const ssize_t
    format = exif_tag->format;

  const int
    components = exif_tag->components;

  const size_t
    number_bytes = exif_tag->number_bytes;

  unsigned char
    *const p = exif_tag->p;

  ssize_t
    *const level = targets->level;

  size_t
    *const entry = targets->entry;

  unsigned char
    **const exif_directory = targets->exif_directory;

  DirectoryInfo
    *const directory_stack = targets->directory_stack;

  ssize_t
    offset;

  if (IsExifDirectoryOffset(tag_value,format,components,number_bytes) ==
      MagickFalse)
    return(MagickFalse);
  offset=(ssize_t) ReadProfileLong(endian,p);
  if (((size_t) offset >= (length-1)) ||
      ((*level) >= (MaxDirectoryStack-2)))
    return(MagickFalse);
  if ((directory == ifd0) && (tag_value == TAG_EXIF_OFFSET))
    (*exif_directory)=exif+offset;
  directory_stack[(*level)].directory=directory;
  (*entry)++;
  directory_stack[(*level)].entry=(*entry);
  (*level)++;
  directory_stack[(*level)].directory=exif+offset;
  directory_stack[(*level)].entry=0;
  (*level)++;
  if ((directory+2+(12*number_entries)) > (exif+length))
    return(MagickTrue);
  offset=(ssize_t) ReadProfileLong(endian,directory+2+(12*
    number_entries));
  if (IsNextDirectoryOffset(offset,length,(*level)) != MagickFalse)
    {
      directory_stack[(*level)].directory=exif+offset;
      directory_stack[(*level)].entry=0;
      (*level)++;
    }
  return(MagickTrue);
}

static MagickBooleanType SyncExifEntry(const ExifWalkInfo *walk,
  const size_t number_entries,const ExifWalkTargets *targets)
{
  const Image
    *const image = walk->image;

  const EndianType
    endian = walk->endian;

  unsigned char
    *const exif = walk->exif;

  const size_t
    length = walk->length;

  unsigned char
    *const directory = walk->directory,
    *const ifd0 = walk->ifd0;

  size_t
    *const entry = targets->entry;

  unsigned char
    **const exif_directory = targets->exif_directory;

  ExifEntryInfo
    exif_entry;

  ExifTagInfo
    exif_tag;

  int
    components;

  unsigned char
    *p,
    *q;

  size_t
    number_bytes;

  ssize_t
    format;

  unsigned short
    tag_value;

  const ExifEntryTargets
    entry_targets = { &q, &tag_value, &format, &components, &number_bytes };

  if (ReadExifEntry(walk,(*entry),&entry_targets) == MagickFalse)
    return(MagickTrue);
  exif_entry.endian=endian;
  exif_entry.exif=exif;
  exif_entry.length=length;
  exif_entry.number_bytes=number_bytes;
  exif_entry.q=q;
  if (LocateExifEntryValue(&exif_entry,&p) == MagickFalse)
    return(MagickFalse);
  exif_tag.image=image;
  exif_tag.endian=endian;
  exif_tag.directory=directory;
  exif_tag.ifd0=ifd0;
  exif_tag.tag_value=tag_value;
  exif_tag.format=format;
  exif_tag.components=components;
  exif_tag.number_bytes=number_bytes;
  exif_tag.q=q;
  exif_tag.p=p;
  SyncExifTag(&exif_tag,(*exif_directory));
  if (PushExifDirectory(walk,&exif_tag,number_entries,targets) !=
      MagickFalse)
    return(MagickTrue);
  return(MagickFalse);
}

static void SyncExifDirectory(const ExifWalkInfo *walk,
  const ExifWalkTargets *targets)
{
  const EndianType
    endian = walk->endian;

  unsigned char
    *const exif = walk->exif;

  const size_t
    length = walk->length;

  unsigned char
    *const directory = walk->directory;

  size_t
    *const entry = targets->entry;

  size_t
    directory_length,
    number_entries;

  /*
    Determine how many entries there are in the current IFD.
  */
  directory_length=length-(size_t) (directory-exif);
  if (directory_length < 6)
    return;
  number_entries=(size_t) (unsigned short) ReadProfileShort(endian,
    directory);
  if (number_entries > ((directory_length-6)/12))
    return;
  for ( ; (*entry) < number_entries; (*entry)++)
  {
    if (SyncExifEntry(walk,number_entries,targets) != MagickFalse)
      break;
  }
}

static inline MagickBooleanType IsInvalidExifOffset(const ssize_t offset,
  const size_t length)
{
  return(((offset < 0) || ((size_t) offset >= length)) ? MagickTrue :
    MagickFalse);
}

static void PopExifDirectory(const DirectoryInfo *directory_stack,
  ssize_t *level,unsigned char **directory,size_t *entry)
{
  if ((*level) > 0)
    {
      (*level)--;
      (*directory)=directory_stack[(*level)].directory;
      (*entry)=directory_stack[(*level)].entry;
    }
}

static inline MagickBooleanType IsOutsideExifProfile(
  const unsigned char *directory,const unsigned char *exif,const size_t length)
{
  return(((directory < exif) || (directory > (exif+length-2))) ? MagickTrue :
    MagickFalse);
}

static void SyncExifProfile(const Image *image,unsigned char *exif,
  size_t length)
{
  DirectoryInfo
    directory_stack[MaxDirectoryStack] = { { 0, 0 } };

  EndianType
    endian;

  ExifWalkInfo
    walk;

  size_t
    entry;

  SplayTreeInfo
    *exif_resources;

  ssize_t
    level,
    offset;

  unsigned char
    *directory,
    *exif_directory,
    *ifd0;

  const ExifWalkTargets
    targets = { &level, &entry, &exif_directory, directory_stack };

  if (ReadExifHeader(&exif,&length,&endian) == MagickFalse)
    return;
  /*
    This the offset to the first IFD.
  */
  offset=(ssize_t) ReadProfileLong(endian,exif+4);
  if (IsInvalidExifOffset(offset,length) != MagickFalse)
    return;
  directory=exif+offset;
  exif_directory=(unsigned char *) NULL;
  ifd0=directory;
  level=0;
  entry=0;
  exif_resources=NewSplayTree((int (*)(const void *,const void *)) NULL,
    (void *(*)(void *)) NULL,(void *(*)(void *)) NULL);
  do
  {
    PopExifDirectory(directory_stack,&level,&directory,&entry);
    if (IsOutsideExifProfile(directory,exif,length) != MagickFalse)
      break;
    walk.image=image;
    walk.endian=endian;
    walk.exif=exif;
    walk.length=length;
    walk.directory=directory;
    walk.ifd0=ifd0;
    walk.exif_resources=exif_resources;
    SyncExifDirectory(&walk,&targets);
  } while (level > 0);
  exif_resources=DestroySplayTree(exif_resources);
  return;
}

static void WriteResolutionResource(const Image *image,unsigned char *p)
{
  if (image->units == PixelsPerCentimeterResolution)
    WriteProfileLong(MSBEndian,(unsigned int) CastDoubleToSsizeT(
      image->resolution.x*2.54*65536.0),p);
  else
    WriteProfileLong(MSBEndian,(unsigned int) CastDoubleToSsizeT(
      image->resolution.x*65536.0),p);
  WriteProfileShort(MSBEndian,(unsigned short) image->units,p+4);
  if (image->units == PixelsPerCentimeterResolution)
    WriteProfileLong(MSBEndian,(unsigned int) CastDoubleToSsizeT(
      image->resolution.y*2.54*65536.0),p+8);
  else
    WriteProfileLong(MSBEndian,(unsigned int) CastDoubleToSsizeT(
      image->resolution.y*65536.0),p+8);
  WriteProfileShort(MSBEndian,(unsigned short) image->units,p+12);
}

static inline MagickBooleanType IsInvalid8BimNameCount(const ssize_t count,
  const size_t length)
{
  return(((count >= (ssize_t) length) || (count < 0)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsInvalid8BimResourceCount(const ssize_t count,
  const size_t length)
{
  return(((count > (ssize_t) length) || (count < 0)) ? MagickTrue :
    MagickFalse);
}

typedef struct _ProfileCursorTargets
{
  unsigned char
    **p;

  size_t
    *length;
} ProfileCursorTargets;

static MagickBooleanType Read8BimSignature(
  const ProfileCursorTargets *targets)
{
  unsigned char
    **const p = targets->p;

  size_t
    *const length = targets->length;

  if (ReadProfileByte(p,length) != 0x38)
    return(MagickFalse);
  if (ReadProfileByte(p,length) != 0x42)
    return(MagickFalse);
  if (ReadProfileByte(p,length) != 0x49)
    return(MagickFalse);
  if (ReadProfileByte(p,length) != 0x4D)
    return(MagickFalse);
  return(MagickTrue);
}

static void Sync8BimResource(const Image *image,const unsigned short id,
  const ssize_t count,unsigned char *p)
{
  if ((id == 0x3ED) && (count == 16))
    WriteResolutionResource(image,p);
  if (id == 0x0422)
    SyncExifProfile(image,p,(size_t) count);
}

static void Sync8BimProfile(const Image *image,const StringInfo *profile)
{
  size_t
    length;

  ssize_t
    count;

  unsigned char
    *p;

  unsigned short
    id;

  length=GetStringInfoLength(profile);
  p=GetStringInfoDatum(profile);
  while (length != 0)
  {
    ProfileCursorTargets
      targets;

    targets.p=(&p);
    targets.length=(&length);
    if (Read8BimSignature(&targets) == MagickFalse)
      continue;
    if (length < 7)
      return;
    id=(unsigned short) ReadProfileMSBShort(&p,&length);
    count=(ssize_t) ReadProfileByte(&p,&length);
    if (IsInvalid8BimNameCount(count,length) != MagickFalse)
      return;
    p+=(ptrdiff_t) count;
    length-=(size_t) count;
    if ((*p & 0x01) == 0)
      (void) ReadProfileByte(&p,&length);
    count=(ssize_t) ReadProfileMSBLong(&p,&length);
    if (IsInvalid8BimResourceCount(count,length) != MagickFalse)
      return;
    Sync8BimResource(image,id,count,p);
    p+=(ptrdiff_t) count;
    length-=(size_t) count;
  }
  return;
}

static void ReplaceXMPValue(StringInfo *profile,size_t start,size_t end,
  const char *value)
{
  char
    *datum;

  size_t
    length,
    new_length,
    value_length;

  length=GetStringInfoLength(profile);
  value_length=strlen(value);
  new_length=length-(end-start)+value_length;
  if (new_length > length)
    SetStringInfoLength(profile,new_length);
  datum=(char *) GetStringInfoDatum(profile);
  (void) memmove(datum+start+value_length,datum+end,length-end);
  (void) memcpy(datum+start,value,value_length);
  *(datum+new_length)='\0';
  if (new_length < length)
    {
      SetStringInfoLength(profile,new_length);
      datum=(char *) GetStringInfoDatum(profile);
    }
}

static inline MagickBooleanType IsMissingXMPTag(const char *pos,
  const char *datum,const size_t tag_length,const size_t length)
{
  return(((pos == (char *) NULL) || ((pos-datum) < 1) || (*(pos-1) != '<') ||
    (((size_t) (pos-datum)+tag_length) > length) ||
    (*(pos+tag_length) != '>')) ? MagickTrue : MagickFalse);
}

static MagickBooleanType GetXMPOffsets(const StringInfo *profile,
  const char *tag,size_t *start,size_t *end)
{
  char
    *datum,
    *pos;

  size_t
    length,
    tag_length;

  datum=(char *) GetStringInfoDatum(profile);
  length=GetStringInfoLength(profile);
  pos=strstr(datum,tag);
  tag_length=strlen(tag);
  if (IsMissingXMPTag(pos,datum,tag_length,length) != MagickFalse)
    return(MagickFalse);
  *start=(size_t) (pos-datum)+tag_length+1;
  pos=strstr(datum+*start,"<");
  if (pos == (char *) NULL)
    return(MagickFalse);
  *end=(size_t) (pos-datum);
  return(MagickTrue);
}

static MagickBooleanType TryAccumulate(const double term,
  const unsigned long previous,const unsigned long before_previous,
  unsigned long *result)
{
  double
    accumulated;

  *result=0;
  if (term > (double) MAGICK_ULONG_MAX)
    return(MagickFalse);
  accumulated=(term*previous)+before_previous;
  if ((accumulated < 0.0) || (accumulated > (double) MAGICK_ULONG_MAX))
    return(MagickFalse);
  *result=(unsigned long) accumulated;
  return(MagickTrue);
}

static void GetXMPContinuedFraction(double value,
  unsigned long *numerator,unsigned long *denominator)
{
  double
    remainder;

  unsigned long
    den,
    new_den,
    new_num,
    num,
    prev_den,
    prev_num;

  ssize_t
    i;

  num=1;
  prev_num=0;
  den=0;
  prev_den=1;
  remainder=value;
  for (i=0; i < 128; i++)
  {
    double
      approximation,
      fraction,
      term;

    term=floor(remainder);
    if (TryAccumulate(term,num,prev_num,&new_num) == MagickFalse)
      break;
    if (TryAccumulate(term,den,prev_den,&new_den) == MagickFalse)
      break;
    if (new_den == 0)
      break;
    prev_num=num;
    num=new_num;
    prev_den=den;
    den=new_den;
    approximation=num/(double) den;
    if (fabs(approximation-value) <= MagickEpsilon)
      break;
    fraction=remainder-term;
    if (fraction <= 0.0)
      break;
    remainder=1.0/fraction;
  }
  *numerator=num;
  *denominator=(den == 0 ? 1 : den);
}

static void GetXMPNumeratorAndDenominator(double value,
  unsigned long *numerator,unsigned long *denominator)
{
  *numerator=0;
  *denominator=1;
  if (value <= MagickEpsilon)
    return;
  if (value > (double) MAGICK_ULONG_MAX)
    {
      *numerator=MAGICK_ULONG_MAX;
      *denominator=1;
      return;
    }
  if (floor(value) == value)
    {
      *numerator=(unsigned long) value;
      *denominator=1;
      return;
    }
  GetXMPContinuedFraction(value,numerator,denominator);
}

static void SyncXMPProfile(const Image *image,StringInfo *profile)
{
  char
    value[MagickPathExtent];

  size_t
    end,
    start;

  unsigned long
    denominator,
    numerator;

  *value='\0';
  if (GetXMPOffsets(profile,"tiff:XResolution",&start,&end) != MagickFalse)
    {
      GetXMPNumeratorAndDenominator(image->resolution.x,&numerator,
        &denominator);
      (void) FormatLocaleString(value,MagickPathExtent,"%lu/%lu",numerator,
        denominator);
      ReplaceXMPValue(profile,start,end,value);
    }
  if (GetXMPOffsets(profile,"tiff:YResolution",&start,&end) != MagickFalse)
    {
      if ((fabs(image->resolution.x-image->resolution.y) > MagickEpsilon) ||
          (*value == '\0'))
        {
          GetXMPNumeratorAndDenominator(image->resolution.y,&numerator,
            &denominator);
          (void) FormatLocaleString(value,MagickPathExtent,"%lu/%lu",
            numerator,denominator);
        }
      ReplaceXMPValue(profile,start,end,value);
    }
  if (GetXMPOffsets(profile,"tiff:ResolutionUnit",&start,&end) != MagickFalse)
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%d",
        ((int) image->units)+1);
      ReplaceXMPValue(profile,start,end,value);
    }
  if (GetXMPOffsets(profile,"tiff:Orientation",&start,&end) != MagickFalse)
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%d",
        (int) image->orientation);
      ReplaceXMPValue(profile,start,end,value);
    }
}

MagickPrivate void SyncImageProfiles(Image *image)
{
  StringInfo
    *profile;

  profile=(StringInfo *) GetImageProfile(image,"8BIM");
  if (profile != (StringInfo *) NULL)
    Sync8BimProfile(image,profile);
  profile=(StringInfo *) GetImageProfile(image,"EXIF");
  if (profile != (StringInfo *) NULL)
    SyncExifProfile(image,GetStringInfoDatum(profile),GetStringInfoLength(
      profile));
  profile=(StringInfo *) GetImageProfile(image,"XMP");
  if (profile != (StringInfo *) NULL)
    SyncXMPProfile(image,profile);
}

static void UpdateClipPathKnot(const ClipPathResizeInfo *resize,
  unsigned char **blob,size_t *length)
{
  ssize_t
    i;

  for (i=0; i < 3; i++)
  {
    double
      x,
      y;

    signed int
      xx,
      yy;

    y=(double) ReadProfileMSBLong(&(*blob),&(*length));
    y=y*resize->old_rows/4096.0/4096.0;
    y-=resize->new_geometry->y;
    yy=(signed int) ((y*4096*4096)/resize->new_geometry->height);
    WriteProfileLong(MSBEndian,(size_t) yy,(*blob)-4);
    x=(double) ReadProfileMSBLong(&(*blob),&(*length));
    x=x*resize->old_columns/4096.0/4096.0;
    x-=resize->new_geometry->x;
    xx=(signed int) ((x*4096*4096)/resize->new_geometry->width);
    WriteProfileLong(MSBEndian,(size_t) xx,(*blob)-4);
  }
}

static void SkipClipPathRecord(const ssize_t selector,unsigned char **blob,
  size_t *length)
{
  switch (selector)
  {
    case 6:
    case 7:
    case 8:
    default:
    {
      (*blob)+=24;
      (*length)-=(size_t) MagickMin((*length),24);
      break;
    }
  }
}

typedef struct _ClipPathTargets
{
  unsigned char
    **blob;

  size_t
    *length;

  ssize_t
    *knot_count;
} ClipPathTargets;

static void ReadClipPathSubpathLength(const ClipPathTargets *targets)
{
  unsigned char
    **const blob = targets->blob;

  size_t
    *const length = targets->length;

  ssize_t
    *const knot_count = targets->knot_count;

  if ((*knot_count) != 0)
    {
      (*blob)+=24;
      (*length)-=(size_t) MagickMin((*length),24U);
      return;
    }
  /*
    Expected subpath length record.
  */
  (*knot_count)=(ssize_t) ReadProfileMSBShort(blob,length);
  (*blob)+=22;
  (*length)-=(size_t) MagickMin((*length),22);
}

static void UpdateClipPathSubpathKnot(const ClipPathResizeInfo *resize,
  const ClipPathTargets *targets)
{
  unsigned char
    **const blob = targets->blob;

  size_t
    *const length = targets->length;

  ssize_t
    *const knot_count = targets->knot_count;

  if ((*knot_count) == 0)
    {
      /*
        Unexpected subpath knot.
      */
      (*blob)+=24;
      (*length)-=(size_t) MagickMin((*length),24);
      return;
    }
  /*
    Add sub-path knot
  */
  UpdateClipPathKnot(resize,blob,length);
  (*knot_count)--;
}

static inline MagickBooleanType IsClipPathKnotSelector(const ssize_t selector)
{
  switch (selector)
  {
    case 1:
    case 2:
    case 4:
    case 5:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static void UpdateClipPath(unsigned char *blob,size_t length,
  const ClipPathResizeInfo *resize)
{
  ssize_t
    knot_count,
    selector;

  const ClipPathTargets
    targets = { &blob, &length, &knot_count };

  knot_count=0;
  while (length != 0)
  {
    selector=(ssize_t) ReadProfileMSBShort(&blob,&length);
    if (IsClipPathKnotSelector(selector) != MagickFalse)
      {
        UpdateClipPathSubpathKnot(resize,&targets);
        continue;
      }
    switch (selector)
    {
      case 0:
      case 3:
      {
        ReadClipPathSubpathLength(&targets);
        break;
      }
      default:
      {
        SkipClipPathRecord(selector,&blob,&length);
        break;
      }
    }
  }
}

static MagickBooleanType Skip8BIMClipPathMarker(unsigned char **info,
  size_t *length)
{
  if (ReadProfileByte(info,length) != (unsigned char) '8')
    return(MagickTrue);
  if (ReadProfileByte(info,length) != (unsigned char) 'B')
    return(MagickTrue);
  if (ReadProfileByte(info,length) != (unsigned char) 'I')
    return(MagickTrue);
  if (ReadProfileByte(info,length) != (unsigned char) 'M')
    return(MagickTrue);
  return(MagickFalse);
}

static void Update8BIMResource(const ClipPathResizeInfo *resize,
  unsigned char **info,size_t *length)
{
  ssize_t
    count,
    id;

  id=(ssize_t) ReadProfileMSBShort(info,length);
  count=(ssize_t) ReadProfileByte(info,length);
  if ((count != 0) && ((size_t) count <= (*length)))
    {
      (*info)+=count;
      (*length)-=(size_t) count;
    }
  if ((count & 0x01) == 0)
    (void) ReadProfileByte(info,length);
  count=(ssize_t) ReadProfileMSBLong(info,length);
  if ((count < 0) || ((size_t) count > (*length)))
    {
      (*length)=0;
      return;
    }
  if ((id > 1999) && (id < 2999))
    UpdateClipPath((*info),(size_t) count,resize);
  (*info)+=count;
  (*length)-=(size_t) MagickMin((*length),(size_t) count);
}

MagickPrivate void Update8BIMClipPath(const Image *image,
  const size_t old_columns,const size_t old_rows,
  const RectangleInfo *new_geometry)
{
  const ClipPathResizeInfo
    resize = { old_columns, old_rows, new_geometry };

  const StringInfo
    *profile;

  size_t
    length;

  unsigned char
    *info;

  assert(image != (Image *) NULL);
  assert(new_geometry != (RectangleInfo *) NULL);
  profile=GetImageProfile(image,"8bim");
  if (profile == (StringInfo *) NULL)
    return;
  length=GetStringInfoLength(profile);
  info=GetStringInfoDatum(profile);
  while (length > 0)
  {
    if (Skip8BIMClipPathMarker(&info,&length) != MagickFalse)
      continue;
    Update8BIMResource(&resize,&info,&length);
  }
}
