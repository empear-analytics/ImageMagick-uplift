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

  MagickCore X11 methods of widget-list.c: the entry counting and matching,
  the list drawing, the entry selection and the slider page move of
  XListBrowserWidget, from CountListBrowserEntries to
  MoveListBrowserSliderByPage; included by that file only, above widget-list-
  actions-private.h.
*/
#ifndef MAGICKCORE_WIDGET_LIST_ENTRIES_PRIVATE_H
#define MAGICKCORE_WIDGET_LIST_ENTRIES_PRIVATE_H

#if defined(MAGICKCORE_X11_DELEGATE)

static unsigned int CountListBrowserEntries(const char *const *list)
{
  unsigned int
    entries;

  for (entries=0; ; entries++)
    if (list[entries] == (char *) NULL)
      break;
  return(entries);
}

static unsigned int WidestListBrowserEntry(XFontStruct *font_info,
  const char *const *list,const unsigned int entries,unsigned int text_width)
{
  int
    i;

  for (i=0; i < (int) entries; i++)
    if (WidgetTextWidth(font_info,(char *) list[i]) > text_width)
      text_width=WidgetTextWidth(font_info,(char *) list[i]);
  return(text_width);
}

typedef struct _ListBrowserReplyMatch
{
  const char
    *const *list;

  char
    *reply;

  unsigned int
    entries,
    visible_entries;
} ListBrowserReplyMatch;

static void MatchListBrowserReply(const ListBrowserReplyMatch *match,
  XWidgetInfo *list_info,XWidgetInfo *slider_info)
{
  const char
    *const *const list = match->list;

  char
    *const reply = match->reply;

  const unsigned int
    entries = match->entries,
    visible_entries = match->visible_entries;

  int
    i;

  for (i=0; i < (int) entries; i++)
    if (LocaleCompare(list[i],reply) == 0)
      {
        (*list_info).id=i;
        (*slider_info).id=i-(int) (visible_entries >> 1);
        if ((*slider_info).id < 0)
          (*slider_info).id=0;
      }
}

typedef struct _ListBrowserRedraw
{
  Display
    *display;

  XWindowInfo
    *window_info;

  const char
    *const *list;

  unsigned int
    entries,
    height,
    visible_entries;

  const XWidgetInfo
    *list_info;

  XWidgetInfo
    north_info,
    slider_info,
    south_info;
} ListBrowserRedraw;

static void RedrawListBrowserList(const ListBrowserRedraw *redraw,
  XWidgetInfo *selection_info,XWidgetInfo *expose_info)
{
  Display
    *const display = redraw->display;

  XWindowInfo
    *const window_info = redraw->window_info;

  const char
    *const *const list = redraw->list;

  const unsigned int
    entries = redraw->entries,
    height = redraw->height,
    visible_entries = redraw->visible_entries;

  const XWidgetInfo
    *const list_info = redraw->list_info;

  const XWidgetInfo
    north_info = redraw->north_info,
    slider_info = redraw->slider_info,
    south_info = redraw->south_info;

  int
    i;

  /*
    Redraw scroll bar and file names.
  */
  (*selection_info).id=slider_info.id;
  (*selection_info).y=(*list_info).y+(int) (height >> 3)+2;
  for (i=0; i < (int) visible_entries; i++)
  {
    (*selection_info).raised=(slider_info.id+i) != (*list_info).id ?
      MagickTrue : MagickFalse;
    (*selection_info).text=(char *) NULL;
    if ((slider_info.id+i) < (int) entries)
      (*selection_info).text=(char *) list[slider_info.id+i];
    XDrawWidgetText(display,window_info,&(*selection_info));
    (*selection_info).y+=(int) (*selection_info).height;
  }
  /*
    Update slider.
  */
  if (slider_info.y > (*expose_info).y)
    {
      (*expose_info).height=(unsigned int) (slider_info.y-(*expose_info).y);
      (*expose_info).y=slider_info.y-(int) (*expose_info).height-(int)
        slider_info.bevel_width-1;
    }
  else
    {
      (*expose_info).height=(unsigned int) ((*expose_info).y-slider_info.y);
      (*expose_info).y=slider_info.y+(int) slider_info.height+(int)
        slider_info.bevel_width+1;
    }
  XDrawTriangleNorth(display,window_info,&north_info);
  XDrawMatte(display,window_info,&(*expose_info));
  XDrawBeveledButton(display,window_info,&slider_info);
  XDrawTriangleSouth(display,window_info,&south_info);
  (*expose_info).y=slider_info.y;
}

typedef struct _ListBrowserTargets
{
  XWidgetInfo
    *reply_info,
    *selection_info,
    *action_info;

  size_t
    *state;

  XWidgetInfo
    *list_info;
} ListBrowserTargets;

typedef struct _ListBrowserPress
{
  Display
    *display;

  XWindowInfo
    *window_info;

  const char
    *const *list;

  unsigned int
    entries,
    height;

  XEvent
    event;

  XWidgetInfo
    slider_info;
} ListBrowserPress;

static void SelectListBrowserEntry(const ListBrowserPress *press,
  const ListBrowserTargets *targets)
{
  Display
    *const display = press->display;

  XWindowInfo
    *const window_info = press->window_info;

  const char
    *const *const list = press->list;

  const unsigned int
    entries = press->entries,
    height = press->height;

  const XEvent
    event = press->event;

  const XWidgetInfo
    slider_info = press->slider_info;

  XWidgetInfo
    *const reply_info = targets->reply_info,
    *const selection_info = targets->selection_info,
    *const action_info = targets->action_info;

  size_t
    *const state = targets->state;

  XWidgetInfo
    *const list_info = targets->list_info;

  int
    id;

  /*
    User pressed list matte.
  */
  id=slider_info.id+(event.xbutton.y-((*list_info).y+(int)
    (height >> 1))+1)/(int) (*selection_info).height;
  if (id >= (int) entries)
    return;
  (void) CopyMagickString((*reply_info).text,list[id],MagickPathExtent);
  (*reply_info).highlight=MagickFalse;
  (*reply_info).marker=(*reply_info).text;
  (*reply_info).cursor=(*reply_info).text+Extent((*reply_info).text);
  XDrawMatteText(display,window_info,reply_info);
  (*selection_info).id=(~0);
  if (id == (*list_info).id)
    {
      (*action_info).raised=MagickFalse;
      XDrawBeveledButton(display,window_info,action_info);
      (*state)|=ExitState;
    }
  (*list_info).id=id;
  (*state)|=RedrawListState;
}

static void MoveListBrowserSliderByPage(const KeySym key_symbol,
  const unsigned int visible_entries,const unsigned int entries,
  XWidgetInfo *slider_info)
{
  switch ((int) key_symbol)
  {
    case XK_Prior:
    case XK_KP_Prior:
    {
      (*slider_info).id-=(int) visible_entries;
      break;
    }
    case XK_Next:
    case XK_KP_Next:
    {
      (*slider_info).id+=(int) visible_entries;
      break;
    }
    case XK_End:
    case XK_KP_End:
    {
      (*slider_info).id=(int) entries;
      break;
    }
    default:
      break;
  }
}

#endif

#endif
