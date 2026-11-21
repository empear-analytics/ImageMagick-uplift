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
#include "MagickCore/profile-sync-private.h"

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

static inline MagickBooleanType IsSkippable8BimName(const ssize_t count,
  const size_t *length)
{
  return(((count != 0) && ((size_t) count <= (*length))) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsOverlong8BimResource(const ssize_t count,
  const size_t *length)
{
  return(((count < 0) || ((size_t) count > (*length))) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsClipPathResource(const ssize_t id)
{
  return(((id > 1999) && (id < 2999)) ? MagickTrue : MagickFalse);
}

static void Update8BIMResource(const ClipPathResizeInfo *resize,
  unsigned char **info,size_t *length)
{
  ssize_t
    count,
    id;

  id=(ssize_t) ReadProfileMSBShort(info,length);
  count=(ssize_t) ReadProfileByte(info,length);
  if (IsSkippable8BimName(count,length) != MagickFalse)
    {
      (*info)+=count;
      (*length)-=(size_t) count;
    }
  if ((count & 0x01) == 0)
    (void) ReadProfileByte(info,length);
  count=(ssize_t) ReadProfileMSBLong(info,length);
  if (IsOverlong8BimResource(count,length) != MagickFalse)
    {
      (*length)=0;
      return;
    }
  if (IsClipPathResource(id) != MagickFalse)
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
