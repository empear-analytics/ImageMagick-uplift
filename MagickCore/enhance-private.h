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

  MagickCore private image enhancement methods.
*/
#ifndef MAGICKCORE_ENHANCE_PRIVATE_H
#define MAGICKCORE_ENHANCE_PRIVATE_H

#include "MagickCore/cache-view.h"
#include "MagickCore/image.h"
#include "MagickCore/monitor.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _ToneLine
{
  const Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;
} ToneLine;

static inline void SyncToneRow(const ToneLine *line,MagickBooleanType *status)
{
  if (SyncCacheViewAuthenticPixels(line->image_view,line->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
}

static inline MagickBooleanType GetToneRow(const ToneLine *line,const ssize_t y,
  Quantum *magick_restrict *q,MagickBooleanType *status)
{
  if ((*status) == MagickFalse)
    return(MagickFalse);
  (*q)=GetCacheViewAuthenticPixels(line->image_view,0,y,line->image->columns,1,
    line->exception);
  if ((*q) == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static inline void SetToneProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
