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

  MagickCore X11 methods of widget-file.c: the entry, reply, format, button,
  key and scroll actions of XFileBrowserWidget, from SelectFileBrowserEntry to
  ReleaseFileBrowserButtons; included by that file only.
*/
#ifndef MAGICKCORE_WIDGET_FILE_ACTIONS_PRIVATE_H
#define MAGICKCORE_WIDGET_FILE_ACTIONS_PRIVATE_H

#if defined(MAGICKCORE_X11_DELEGATE)

typedef struct _FileBrowserTargets
{
  XWidgetInfo
    *reply_info;

  size_t
    *state;

  XWidgetInfo
    *selection_info,
    *list_info;
} FileBrowserTargets;

typedef struct _FileBrowserPress
{
  Display
    *display;

  XWindows
    *windows;

  char
    **filelist;

  size_t
    files;

  unsigned int
    height;

  XEvent
    event;

  XWidgetInfo
    slider_info;

  char
    *working_path,
    *reply;
} FileBrowserPress;

static void SelectFileBrowserEntry(const FileBrowserPress *press,
  const FileBrowserTargets *targets)
{
  Display
    *const display = press->display;

  XWindows
    *const windows = press->windows;

  char
    **const filelist = press->filelist;

  const size_t
    files = press->files;

  const unsigned int
    height = press->height;

  const XEvent
    event = press->event;

  const XWidgetInfo
    slider_info = press->slider_info;

  char
    *const working_path = press->working_path,
    *const reply = press->reply;

  XWidgetInfo
    *const reply_info = targets->reply_info;

  size_t
    *const state = targets->state;

  XWidgetInfo
    *const selection_info = targets->selection_info,
    *const list_info = targets->list_info;

  int
    id;

  /*
    User pressed file matte.
  */
  id=slider_info.id+(event.xbutton.y-((*list_info).y+(int)
    (height >> 1))+1)/(int) (*selection_info).height;
  if (id >= (int) files)
    return;
  (void) CopyMagickString((*reply_info).text,filelist[id],MagickPathExtent);
  (*reply_info).highlight=MagickFalse;
  (*reply_info).marker=(*reply_info).text;
  (*reply_info).cursor=(*reply_info).text+Extent((*reply_info).text);
  XDrawMatteText(display,&windows->widget,reply_info);
  if (id == (*list_info).id)
    {
      char
        *p;

      p=(*reply_info).text+strlen((*reply_info).text)-1;
      if (*p == *DirectorySeparator)
        ChopPathComponents((*reply_info).text,1);
      (void) ConcatenateMagickString(working_path,DirectorySeparator,
        MagickPathExtent);
      (void) ConcatenateMagickString(working_path,(*reply_info).text,
        MagickPathExtent);
      *reply='\0';
      (*state)|=UpdateListState;
    }
  (*selection_info).id=(~0);
  (*list_info).id=id;
  (*state)|=RedrawListState;
}

static void CopyFileBrowserFormat(char *reply,char *format)
{
  char
    *p;

  p=reply+Extent(reply)-1;
  while ((p > (reply+1)) && (*(p-1) != '.'))
    p--;
  if ((p > (reply+1)) && (*(p-1) == '.'))
    (void) CopyMagickString(format,p,MagickPathExtent);
}

static ssize_t FindFileBrowserCursor(XFontStruct *font_info,
  const XWidgetInfo reply_info,const int x)
{
  ssize_t
    i;

  for (i=1; i <= (ssize_t) Extent(reply_info.marker); i++)
    if (XTextWidth(font_info,reply_info.marker,(int) i) > x)
      break;
  return(i);
}

typedef struct _FileBrowserMatch
{
  char
    **filelist;

  size_t
    files;

  char
    *reply;
} FileBrowserMatch;

static void MatchFileBrowserReply(const FileBrowserMatch *match,
  XWidgetInfo *list_info,ssize_t *i)
{
  char
    **const filelist = match->filelist;

  const size_t
    files = match->files;

  char
    *const reply = match->reply;

  for ((*i)=0; (*i) < (ssize_t) files; (*i)++)
    if (LocaleCompare(filelist[(*i)],reply) >= 0)
      {
        (*list_info).id=(int)
          (LocaleCompare(filelist[(*i)],reply) == 0 ? (*i) : ~0);
        break;
      }
}

typedef struct _FileBrowserNames
{
  Display
    *display;

  XWindows
    *windows;

  char
    **filelist;

  size_t
    files;

  unsigned int
    visible_files;

  XWidgetInfo
    list_info,
    slider_info;
} FileBrowserNames;

static void DrawFileBrowserNames(const FileBrowserNames *names,
  XWidgetInfo *selection_info)
{
  Display
    *const display = names->display;

  XWindows
    *const windows = names->windows;

  char
    **const filelist = names->filelist;

  const size_t
    files = names->files;

  const unsigned int
    visible_files = names->visible_files;

  const XWidgetInfo
    list_info = names->list_info,
    slider_info = names->slider_info;

  ssize_t
    i;

  for (i=0; i < (ssize_t) visible_files; i++)
  {
    (*selection_info).raised=(int) (slider_info.id+i) != list_info.id ?
      MagickTrue : MagickFalse;
    (*selection_info).text=(char *) NULL;
    if ((slider_info.id+i) < (ssize_t) files)
      (*selection_info).text=filelist[slider_info.id+i];
    XDrawWidgetText(display,&windows->widget,selection_info);
    (*selection_info).y+=(int) (*selection_info).height;
  }
}

typedef struct _FileBrowserPathRelease
{
  XWindows
    *windows;

  XEvent
    event;

  char
    *working_path;
} FileBrowserPathRelease;

static size_t ReleaseFileBrowserUp(const FileBrowserPathRelease *up,
  const XWidgetInfo up_info,size_t state)
{
  XWindows
    *const windows = up->windows;

  const XEvent
    event = up->event;

  char
    *const working_path = up->working_path;

  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(up_info,event.xbutton)))
    {
      ChopPathComponents(working_path,1);
      if (*working_path == '\0')
        (void) CopyMagickString(working_path,DirectorySeparator,
          MagickPathExtent);
      state|=UpdateListState;
    }
  return(state);
}

typedef struct _FileBrowserCancel
{
  XWindows
    *windows;

  XEvent
    event;

  char
    *reply;
} FileBrowserCancel;

static size_t ReleaseFileBrowserCancel(const FileBrowserCancel *cancel,
  const XWidgetInfo cancel_info,const XWidgetInfo reply_info,size_t state)
{
  XWindows
    *const windows = cancel->windows;

  const XEvent
    event = cancel->event;

  char
    *const reply = cancel->reply;

  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(cancel_info,event.xbutton)))
    {
      *reply_info.text='\0';
      *reply='\0';
      state|=ExitState;
    }
  return(state);
}

typedef struct _FileBrowserPaths
{
  char
    *glob_pattern,
    *working_path,
    *reply;
} FileBrowserPaths;

static size_t ReadFileBrowserPath(const FileBrowserPaths *paths,
  const XWidgetInfo reply_info,size_t state)
{
  char
    *const glob_pattern = paths->glob_pattern,
    *const working_path = paths->working_path,
    *const reply = paths->reply;

  /*
    Read new directory or glob pattern.
  */
  if (*reply_info.text == '\0')
    return(state);
  if (IsGlob(reply_info.text))
    (void) CopyMagickString(glob_pattern,reply_info.text,
      MagickPathExtent);
  else
    {
      (void) ConcatenateMagickString(working_path,DirectorySeparator,
        MagickPathExtent);
      (void) ConcatenateMagickString(working_path,reply_info.text,
        MagickPathExtent);
      if (*working_path == '~')
        ExpandFilename(working_path);
      *reply='\0';
    }
  state|=UpdateListState;
  return(state);
}

typedef struct _FileBrowserFormatTargets
{
  XWidgetInfo
    *reply_info,
    *special_info;
} FileBrowserFormatTargets;

static void SelectFileBrowserFormat(Display *display,XWindows *windows,
  char *format,const FileBrowserFormatTargets *targets)
{
  XWidgetInfo
    *const reply_info = targets->reply_info,
    *const special_info = targets->special_info;

  char
    **formats;

  ExceptionInfo
    *exception;

  size_t
    number_formats;

  ssize_t
    i;

  /*
    Let user select image format.
  */
  exception=AcquireExceptionInfo();
  formats=GetMagickList("*",&number_formats,exception);
  exception=DestroyExceptionInfo(exception);
  if (formats == (char **) NULL)
    return;
  (void) XCheckDefineCursor(display,windows->widget.id,
    windows->widget.busy_cursor);
  windows->popup.x=windows->widget.x+60;
  windows->popup.y=windows->widget.y+60;
  XListBrowserWidget(display,windows,&windows->popup,
    (const char **) formats,"Select","Select image format type:",
    format);
  XSetCursorState(display,windows,MagickTrue);
  (void) XCheckDefineCursor(display,windows->widget.id,
    windows->widget.cursor);
  LocaleLower(format);
  AppendImageFormat(format,(*reply_info).text);
  (*reply_info).cursor=(*reply_info).text+Extent((*reply_info).text);
  XDrawMatteText(display,&windows->widget,reply_info);
  (*special_info).raised=MagickTrue;
  XDrawBeveledButton(display,&windows->widget,special_info);
  for (i=0; i < (ssize_t) number_formats; i++)
    formats[i]=DestroyString(formats[i]);
  formats=(char **) RelinquishMagickMemory(formats);
}

static size_t ReleaseFileBrowserGrab(const FileBrowserPathRelease *grab,
  const XWidgetInfo special_info,size_t state)
{
  XWindows
    *const windows = grab->windows;

  const XEvent
    event = grab->event;

  char
    *const working_path = grab->working_path;

  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(special_info,event.xbutton)))
    {
      (void) CopyMagickString(working_path,"x:",MagickPathExtent);
      state|=ExitState;
    }
  return(state);
}

typedef struct _FileBrowserSpecial
{
  unsigned int
    anomaly;

  Display
    *display;

  XWindows
    *windows;

  char
    *format;

  FileBrowserFormatTargets
    format_targets;

  XEvent
    event;

  char
    *working_path;
} FileBrowserSpecial;

typedef struct _FileBrowserSpecialTargets
{
  size_t
    *state;

  XWidgetInfo
    *special_info;
} FileBrowserSpecialTargets;

static MagickBooleanType ReleaseFileBrowserSpecial(
  const FileBrowserSpecial *special,const FileBrowserSpecialTargets *targets)
{
  const unsigned int
    anomaly = special->anomaly;

  Display
    *const display = special->display;

  XWindows
    *const windows = special->windows;

  char
    *const format = special->format;

  const FileBrowserFormatTargets
    format_targets = special->format_targets;

  const XEvent
    event = special->event;

  char
    *const working_path = special->working_path;

  size_t
    *const state = targets->state;

  XWidgetInfo
    *const special_info = targets->special_info;

  if ((*special_info).raised == MagickFalse)
    {
      FileBrowserPathRelease
        grab;

      if (anomaly == MagickFalse)
        {
          SelectFileBrowserFormat(display,windows,format,
            &format_targets);
          return(MagickFalse);
        }
      grab.windows=windows;
      grab.event=event;
      grab.working_path=working_path;
      (*state)=ReleaseFileBrowserGrab(&grab,(*special_info),(*state));
      (*special_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,special_info);
    }
  return(MagickTrue);
}

typedef struct _FileBrowserWidgets
{
  XWidgetInfo
    *reply_info;

  size_t
    *state;

  XWidgetInfo
    *scroll_info,
    *slider_info;

  unsigned int
    *visible_files;

  size_t
    *files;

  XWidgetInfo
    *up_info,
    *home_info,
    *special_info,
    *action_info,
    *cancel_info;

  ssize_t
    *i;

  char
    ***filelist;

  XWidgetInfo
    *north_info,
    *south_info,
    *list_info;

  size_t
    *delay;

  XWidgetInfo
    *expose_info,
    *text_info,
    *selection_info;

  XEvent
    *event;

  MagickStatusType
    *mask;
} FileBrowserWidgets;

typedef struct _FileBrowserContext
{
  Display
    *display;

  XWindows
    *windows;

  char
    *reply,
    *glob_pattern,
    *working_path,
    *primary_selection;

  XFontStruct
    *font_info;

  unsigned int
    height;

  char
    *format;

  unsigned int
    anomaly;

  char
    *home_directory;

  const char
    *action;

  unsigned int
    width;

  char
    *text;
} FileBrowserContext;

static void ClientFileBrowserMessage(const FileBrowserContext *context,
  const XEvent event,const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  /*
    If client window delete message, exit.
  */
  if (event.xclient.message_type != windows->wm_protocols)
    return;
  if (*event.xclient.data.l == (int) windows->wm_take_focus)
    {
      (void) XSetInputFocus(display,event.xclient.window,RevertToParent,
        (Time) event.xclient.data.l[1]);
      return;
    }
  if (*event.xclient.data.l != (int) windows->wm_delete_window)
    return;
  if (event.xclient.window == windows->widget.id)
    {
      *(*reply_info).text='\0';
      (*state)|=ExitState;
      return;
    }
}

static void SelectFileBrowserReply(const FileBrowserContext *context,
  const XEvent event,const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  Status
    status;

  Atom
    type;

  int
    format;

  unsigned char
    *data;

  unsigned long
    after,
    length;

  /*
    Obtain response from primary selection.
  */
  if (event.xselection.property == (Atom) None)
    return;
  status=XGetWindowProperty(display,event.xselection.requestor,
    event.xselection.property,0L,2047L,MagickTrue,XA_STRING,&type,
    &format,&length,&after,&data);
  if (IsUnusableSelection(status,type,format,length) != MagickFalse)
    return;
  if ((Extent((*reply_info).text)+(int) length) >= (MagickPathExtent-1))
    (void) XBell(display,0);
  else
    {
      /*
        Insert primary selection in reply text.
      */
      *(data+length)='\0';
      XEditText(display,reply_info,(KeySym) XK_Insert,(char *) data,
        (*state));
      XDrawMatteText(display,&windows->widget,reply_info);
      (*state)|=JumpListState;
      (*state)|=RedrawActionState;
    }
  (void) XFree((void *) data);
}

static void PressFileBrowserKey(const FileBrowserContext *context,
  const XEvent event,const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const reply = context->reply,
    *const glob_pattern = context->glob_pattern,
    *const working_path = context->working_path;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info,
    *const slider_info = widgets->slider_info;

  unsigned int
    *const visible_files = widgets->visible_files;

  size_t
    *const files = widgets->files;

  static char
    command[MagickPathExtent];

  static int
    length;

  static KeySym
    key_symbol;

  /*
    Respond to a user key press.
  */
  if (event.xkey.window != windows->widget.id)
    return;
  length=XLookupString((XKeyEvent *) &event.xkey,command,
    (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  *(command+length)='\0';
  if (AreaIsActive((*scroll_info),event.xkey))
    {
      /*
        Move slider.
      */
      (*slider_info)=MoveFontBrowserSlider((*slider_info),key_symbol,
        (*visible_files),(int) (*files));
      (*state)|=RedrawListState;
      return;
    }
  if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
    {
      FileBrowserPaths
        paths;

      paths.glob_pattern=glob_pattern;
      paths.working_path=working_path;
      paths.reply=reply;
      (*state)=ReadFileBrowserPath(&paths,(*reply_info),(*state));
      return;
    }
  if (key_symbol == XK_Control_L)
    {
      (*state)|=ControlState;
      return;
    }
  (*reply_info)=ApplyListBrowserControlKey((*reply_info),key_symbol,(*state));
  XEditText(display,reply_info,key_symbol,command,(*state));
  XDrawMatteText(display,&windows->widget,reply_info);
  (*state)|=JumpListState;
}

static void ToggleFileBrowserButtons(const FileBrowserContext *context,
  const XEvent event,const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const up_info = widgets->up_info,
    *const home_info = widgets->home_info,
    *const special_info = widgets->special_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  if ((*up_info).raised == MatteIsActive((*up_info),event.xmotion))
    {
      /*
        Up button status changed.
      */
      (*up_info).raised=!(*up_info).raised;
      XDrawBeveledButton(display,&windows->widget,up_info);
      return;
    }
  if ((*home_info).raised == MatteIsActive((*home_info),event.xmotion))
    {
      /*
        Home button status changed.
      */
      (*home_info).raised=!(*home_info).raised;
      XDrawBeveledButton(display,&windows->widget,home_info);
      return;
    }
  if ((*special_info).raised == MatteIsActive((*special_info),event.xmotion))
    {
      /*
        Grab button status changed.
      */
      (*special_info).raised=!(*special_info).raised;
      XDrawBeveledButton(display,&windows->widget,special_info);
      return;
    }
  if ((*action_info).raised == MatteIsActive((*action_info),event.xmotion))
    {
      /*
        Action button status changed.
      */
      (*action_info).raised=(*action_info).raised == MagickFalse ?
        MagickTrue : MagickFalse;
      XDrawBeveledButton(display,&windows->widget,action_info);
      return;
    }
  if ((*cancel_info).raised == MatteIsActive((*cancel_info),event.xmotion))
    {
      /*
        Cancel button status changed.
      */
      (*cancel_info).raised=(*cancel_info).raised == MagickFalse ?
        MagickTrue : MagickFalse;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      return;
    }
}

static void MoveFileBrowserButtons(const FileBrowserContext *context,
  XEvent event,const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const files = widgets->files;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
  if ((*slider_info).active)
    {
      /*
        Move slider matte.
      */
      (*slider_info)=TrackFontBrowserSlider((*slider_info),event,
        (int) (*files));
      (*state)|=RedrawListState;
      return;
    }
  if ((*state) & InactiveWidgetState)
    return;
  ToggleFileBrowserButtons(context,event,widgets);
}

static void PressFileBrowserReply(const FileBrowserContext *context,
  const XEvent event,const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const primary_selection = context->primary_selection;

  XFontStruct
    *const font_info = context->font_info;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  ssize_t
    *const i = widgets->i;

  int
    x;

  if (MatteIsActive((*reply_info),event.xbutton) == MagickFalse)
    return;
  if (event.xbutton.button != Button2)
    {
      static Time
        click_time;

      ListBrowserClick
        click;

      /*
        Move text cursor to position of button press.
      */
      x=event.xbutton.x-(*reply_info).x-(int) (QuantumMargin >> 2);
      (*i)=FindFileBrowserCursor(font_info,(*reply_info),x);
      (*reply_info).cursor=(*reply_info).marker+(*i)-1;
      click.display=display;
      click.window_info=(&windows->widget);
      click.event=event;
      click.click_time=click_time;
      (*reply_info)=HighlightListBrowserReply(&click,primary_selection,
        (*reply_info));
      XDrawMatteText(display,&windows->widget,reply_info);
      click_time=event.xbutton.time;
      return;
    }
  /*
    Request primary selection.
  */
  (void) XConvertSelection(display,XA_PRIMARY,XA_STRING,XA_STRING,
    windows->widget.id,event.xbutton.time);
}

static void PressFileBrowserActions(const FileBrowserContext *context,
  const XEvent event,const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  if (MatteIsActive((*action_info),event.xbutton))
    {
      /*
        User pressed action button.
      */
      (*action_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,action_info);
      return;
    }
  if (MatteIsActive((*cancel_info),event.xbutton))
    {
      /*
        User pressed Cancel button.
      */
      (*cancel_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      return;
    }
  PressFileBrowserReply(context,event,widgets);
}

static void PressFileBrowserButtons(const FileBrowserContext *context,
  const XEvent event,const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const up_info = widgets->up_info,
    *const home_info = widgets->home_info,
    *const special_info = widgets->special_info;

  if (MatteIsActive((*up_info),event.xbutton))
    {
      /*
        User pressed Up button.
      */
      (*up_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,up_info);
      return;
    }
  if (MatteIsActive((*home_info),event.xbutton))
    {
      /*
        User pressed Home button.
      */
      (*home_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,home_info);
      return;
    }
  if (MatteIsActive((*special_info),event.xbutton))
    {
      /*
        User pressed Special button.
      */
      (*special_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,special_info);
      return;
    }
  PressFileBrowserActions(context,event,widgets);
}

static void ChooseFileBrowserEntry(const FileBrowserContext *context,
  const XEvent event,const FileBrowserTargets *targets,
  const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const reply = context->reply,
    *const working_path = context->working_path;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const files = widgets->files;

  char
    ***const filelist = widgets->filelist;

  FileBrowserPress
    entry;

  entry.display=display;
  entry.windows=windows;
  entry.filelist=(*filelist);
  entry.files=(*files);
  entry.height=height;
  entry.event=event;
  entry.slider_info=(*slider_info);
  entry.working_path=working_path;
  entry.reply=reply;
  SelectFileBrowserEntry(&entry,targets);
}

static void PressFileBrowserScroll(const FileBrowserContext *context,
  const XEvent event,const FileBrowserTargets *targets,
  const FileBrowserWidgets *widgets)
{
  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info,
    *const slider_info = widgets->slider_info;

  unsigned int
    *const visible_files = widgets->visible_files;

  size_t
    *const files = widgets->files;

  XWidgetInfo
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info,
    *const list_info = widgets->list_info;

  if (MatteIsActive((*slider_info),event.xbutton))
    {
      /*
        Track slider.
      */
      (*slider_info).active=MagickTrue;
      return;
    }
  if ((MatteIsActive((*north_info),event.xbutton)) && ((*slider_info).id > 0))
    {
      /*
        Move slider up.
      */
      (*north_info).raised=MagickFalse;
      (*slider_info).id--;
      (*state)|=RedrawListState;
      return;
    }
  if ((MatteIsActive((*south_info),event.xbutton)) &&
      ((*slider_info).id < (int) (*files)))
    {
      /*
        Move slider down.
      */
      (*south_info).raised=MagickFalse;
      (*slider_info).id++;
      (*state)|=RedrawListState;
      return;
    }
  if (MatteIsActive((*scroll_info),event.xbutton))
    {
      /*
        Move slider.
      */
      (*slider_info)=PageListBrowserSlider((*slider_info),event,
        (*visible_files));
      (*state)|=RedrawListState;
      return;
    }
  if (MatteIsActive((*list_info),event.xbutton))
    {
      ChooseFileBrowserEntry(context,event,targets,widgets);
      return;
    }
  PressFileBrowserButtons(context,event,widgets);
}

static void ReleaseFileBrowserActions(const FileBrowserContext *context,
  const XEvent event,const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const reply = context->reply,
    *const working_path = context->working_path,
    *const format = context->format;

  const unsigned int
    anomaly = context->anomaly;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const special_info = widgets->special_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  FileBrowserSpecial
    special;

  ListBrowserRelease
    release;

  const FileBrowserFormatTargets
    format_targets = { reply_info, special_info };

  const FileBrowserSpecialTargets
    special_targets = { state, special_info };

  special.anomaly=anomaly;
  special.display=display;
  special.windows=windows;
  special.format=format;
  special.format_targets=format_targets;
  special.event=event;
  special.working_path=working_path;
  if (ReleaseFileBrowserSpecial(&special,&special_targets) == MagickFalse)
    return;
  release.display=display;
  release.window_info=(&windows->widget);
  release.event=event;
  ReleaseListBrowserAction(&release,(*reply_info),action_info,state);
  if ((*cancel_info).raised == MagickFalse)
    {
      FileBrowserCancel
        cancel;

      cancel.windows=windows;
      cancel.event=event;
      cancel.reply=reply;
      (*state)=ReleaseFileBrowserCancel(&cancel,(*cancel_info),(*reply_info),
        (*state));
      (*cancel_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
    }
}

static void ReleaseFileBrowserButtons(const FileBrowserContext *context,
  const XEvent event,const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const working_path = context->working_path,
    *const home_directory = context->home_directory;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const up_info = widgets->up_info,
    *const home_info = widgets->home_info;

  if ((*up_info).raised == MagickFalse)
    {
      FileBrowserPathRelease
        up;

      up.windows=windows;
      up.event=event;
      up.working_path=working_path;
      (*state)=ReleaseFileBrowserUp(&up,(*up_info),(*state));
      (*up_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,up_info);
    }
  if ((*home_info).raised == MagickFalse)
    {
      FontBrowserBack
        back;

      back.windows=windows;
      back.event=event;
      back.back_info=(*home_info);
      back.glob_pattern=working_path;
      back.back_pattern=home_directory;
      (*state)=ReleaseFontBrowserBack(&back,(*state));
      (*home_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,home_info);
    }
  ReleaseFileBrowserActions(context,event,widgets);
}

#endif

#endif
