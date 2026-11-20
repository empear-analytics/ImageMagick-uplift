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

  MagickCore private X11 widget methods.
*/
#ifndef MAGICKCORE_WIDGET_PRIVATE_H
#define MAGICKCORE_WIDGET_PRIVATE_H

#include "MagickCore/string_.h"
#include "MagickCore/xwindow-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(MAGICKCORE_X11_DELEGATE)
typedef struct _XProgressMonitorSpan
{
  const MagickOffsetType
    offset;

  const MagickSizeType
    span;
} XProgressMonitorSpan;

typedef struct _XHelpView
{
  Display
    *display;

  const XResourceInfo
    *resource_info;

  XWindows
    *windows;

  MagickBooleanType
    mono;
} XHelpView;

typedef struct _XListBrowserRequest
{
  const char
    *const *list,
    *action,
    *query;

  char
    *reply;
} XListBrowserRequest;

typedef struct _XMenuRequest
{
  const char
    *title,
    *const *selections;

  char
    *item;
} XMenuRequest;

extern MagickPrivate int
  XCommandWidget(Display *,XWindows *,const char *const *,XEvent *),
  XConfirmWidget(Display *,XWindows *,const char *,const char *),
  XDialogWidget(Display *,XWindows *,const char *,const char *,char *),
  XMenuWidget(Display *,XWindows *,const XMenuRequest *);

extern MagickPrivate MagickBooleanType
  XPreferencesWidget(Display *,XResourceInfo *,XWindows *);

extern MagickPrivate void
  DestroyXWidget(void),
  XColorBrowserWidget(Display *,XWindows *,const char *,char *),
  XFileBrowserWidget(Display *,XWindows *,const char *,char *),
  XFontBrowserWidget(Display *,XWindows *,const char *,char *),
  XInfoWidget(Display *,XWindows *,const char *),
  XListBrowserWidget(Display *,XWindows *,XWindowInfo *,
    const XListBrowserRequest *),
  XNoticeWidget(Display *,XWindows *,const char *,const char *),
  XProgressMonitorWidget(Display *,XWindows *,const char *,
    const XProgressMonitorSpan *),
  XTextViewWidget(const XHelpView *,const char *,const char **);

static inline void XTextViewHelp(const XHelpView *view,const char *title,
  const char *help)
{
  char
    **help_list;

  ssize_t
    i;

  help_list=StringToList(help);
  if (help_list == (char **) NULL)
    return;
  XTextViewWidget(view,title,(const char **) help_list);
  for (i=0; help_list[i] != (char *) NULL; i++)
    help_list[i]=DestroyString(help_list[i]);
  help_list=(char **) RelinquishMagickMemory(help_list);
}

#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
