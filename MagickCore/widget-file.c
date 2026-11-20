/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%                  W   W  IIIII  DDDD    GGGG  EEEEE  TTTTT                   %
%                  W   W    I    D   D  G      E        T                     %
%                  W W W    I    D   D  G  GG  EEE      T                     %
%                  WW WW    I    D   D  G   G  E        T                     %
%                  W   W  IIIII  DDDD    GGG   EEEEE    T                     %
%                                                                             %
%                         FFFFF  IIIII  L      EEEEE                          %
%                         F        I    L      E                              %
%                         FFF      I    L      EEE                            %
%                         F        I    L      E                              %
%                         F      IIIII  LLLLL  EEEEE                          %
%                                                                             %
%                                                                             %
%                 MagickCore X11 File Browser Widget Methods                  %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                              September 1993                                 %
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
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/string_.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/xwindow-private.h"
#include "MagickCore/widget.h"
#include "MagickCore/widget-private.h"
#include "MagickCore/widget-core-private.h"
#include "MagickCore/widget-browser-private.h"
#include "MagickCore/widget-file-actions-private.h"
#if defined(MAGICKCORE_X11_DELEGATE)

static void ReleaseFileBrowserScroll(const FileBrowserContext *context,
  const XEvent event,const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const slider_info = widgets->slider_info,
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info;

  size_t
    *const delay = widgets->delay;

  if (windows->widget.mapped == MagickFalse)
    return;
  if ((*north_info).raised == MagickFalse)
    {
      /*
        User released up button.
      */
      (*delay)=SuspendTime << 2;
      (*north_info).raised=MagickTrue;
      XDrawTriangleNorth(display,&windows->widget,north_info);
    }
  if ((*south_info).raised == MagickFalse)
    {
      /*
        User released down button.
      */
      (*delay)=SuspendTime << 2;
      (*south_info).raised=MagickTrue;
      XDrawTriangleSouth(display,&windows->widget,south_info);
    }
  if ((*slider_info).active)
    {
      /*
        Stop tracking slider.
      */
      (*slider_info).active=MagickFalse;
      return;
    }
  ReleaseFileBrowserButtons(context,event,widgets);
}

#define CancelButtonText  "Cancel"
#define DirectoryText  "Directory:"
#define FilenameText  "File name:"
#define GrabButtonText  "Grab"
#define FormatButtonText  "Format"
#define HomeButtonText  "Home"
#define UpButtonText  "Up"

static void InitializeFileBrowserButtons(const FileBrowserContext *context,
  const FileBrowserWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  char
    *const reply = context->reply;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  char
    *const format = context->format;

  const unsigned int
    anomaly = context->anomaly;

  const char
    *const action = context->action;

  const unsigned int
    width = context->width;

  XWidgetInfo
    *const up_info = widgets->up_info,
    *const home_info = widgets->home_info,
    *const special_info = widgets->special_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  /*
    Initialize button information.
  */
  XGetWidgetInfo(CancelButtonText,cancel_info);
  (*cancel_info).width=width;
  (*cancel_info).height=(unsigned int) ((3*height) >> 1);
  (*cancel_info).x=(int) windows->widget.width-(int) (*cancel_info).width-
    QuantumMargin-2;
  (*cancel_info).y=(int) windows->widget.height-(int) (*cancel_info).height-
    QuantumMargin;
  XGetWidgetInfo(action,action_info);
  (*action_info).width=width;
  (*action_info).height=(unsigned int) ((3*height) >> 1);
  (*action_info).x=(*cancel_info).x-((int) (*cancel_info).width+
    (QuantumMargin >> 1)+(int) ((*action_info).bevel_width << 1));
  (*action_info).y=(*cancel_info).y;
  XGetWidgetInfo(GrabButtonText,special_info);
  (*special_info).width=width;
  (*special_info).height=(unsigned int) ((3*height) >> 1);
  (*special_info).x=(*action_info).x-((int) (*action_info).width+
    (QuantumMargin >> 1)+(int) ((*special_info).bevel_width << 1));
  (*special_info).y=(*action_info).y;
  if (anomaly == MagickFalse)
    {
      (*special_info).text=(char *) FormatButtonText;
      CopyFileBrowserFormat(reply,format);
    }
  XGetWidgetInfo(UpButtonText,up_info);
  (*up_info).width=width;
  (*up_info).height=(unsigned int) ((3*height) >> 1);
  (*up_info).x=QuantumMargin;
  (*up_info).y=(int) ((5*QuantumMargin) >> 1)+(int) height;
  XGetWidgetInfo(HomeButtonText,home_info);
  (*home_info).width=width;
  (*home_info).height=(unsigned int) ((3*height) >> 1);
  (*home_info).x=QuantumMargin;
  (*home_info).y=(*up_info).y+(int) (*up_info).height+QuantumMargin;
}

static void InitializeFileBrowserReply(const FileBrowserContext *context,
  const FileBrowserWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  char
    *const reply = context->reply;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height,
    width = context->width;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const action_info = widgets->action_info;

  /*
    Initialize reply information.
  */
  XGetWidgetInfo(reply,reply_info);
  (*reply_info).raised=MagickFalse;
  (*reply_info).bevel_width--;
  (*reply_info).width=windows->widget.width-width-(unsigned int)
    ((6*QuantumMargin) >> 1);
  (*reply_info).height=height << 1;
  (*reply_info).x=(int) width+(QuantumMargin << 1);
  (*reply_info).y=(*action_info).y-(int) (*reply_info).height-QuantumMargin;
}

static void InitializeFileBrowserScroll(const FileBrowserContext *context,
  const FileBrowserWidgets *widgets)
{
  const unsigned int
    height = context->height;

  XFontStruct
    *const font_info = context->font_info;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const up_info = widgets->up_info,
    *const slider_info = widgets->slider_info,
    *const scroll_info = widgets->scroll_info,
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info,
    *const expose_info = widgets->expose_info;

  size_t
    *const files = widgets->files;

  unsigned int
    *const visible_files = widgets->visible_files;

  int
    id;

  XGetWidgetInfo((char *) NULL,scroll_info);
  (*scroll_info).bevel_width--;
  (*scroll_info).width=height;
  (*scroll_info).height=(unsigned int)
    ((*reply_info).y-(*up_info).y-(int) (QuantumMargin >> 1));
  (*scroll_info).x=(*reply_info).x+(int) ((*reply_info).width-
    (*scroll_info).width);
  (*scroll_info).y=(*up_info).y-(int) (*reply_info).bevel_width;
  (*scroll_info).raised=MagickFalse;
  (*scroll_info).trough=MagickTrue;
  (*north_info)=(*scroll_info);
  (*north_info).raised=MagickTrue;
  (*north_info).width-=((*north_info).bevel_width << 1);
  (*north_info).height=(*north_info).width-1;
  (*north_info).x+=(int) (*north_info).bevel_width;
  (*north_info).y+=(int) (*north_info).bevel_width;
  (*south_info)=(*north_info);
  (*south_info).y=(*scroll_info).y+(int) (*scroll_info).height-(int)
    (*scroll_info).bevel_width-(int) (*south_info).height;
  id=(*slider_info).id;
  (*slider_info)=(*north_info);
  (*slider_info).id=id;
  (*slider_info).width-=2;
  (*slider_info).min_y=(*north_info).y+(int) (*north_info).height+(int)
    (*north_info).bevel_width+(int) (*slider_info).bevel_width+2;
  (*slider_info).height=(unsigned int) ((int) (*scroll_info).height-
    (((*slider_info).min_y-(*scroll_info).y+1) << 1)+4);
  (*visible_files)=(unsigned int) ((*scroll_info).height*
    MagickSafeReciprocal((double) height+(height >> 3)));
  if ((*files) > (*visible_files))
    (*slider_info).height=(unsigned int)
      (((*visible_files)*(*slider_info).height)/(*files));
  (*slider_info).max_y=(*south_info).y-(int) (*south_info).bevel_width-(int)
    (*slider_info).bevel_width-2;
  (*slider_info).x=(*scroll_info).x+(int) (*slider_info).bevel_width+1;
  (*slider_info).y=(*slider_info).min_y;
  (*expose_info)=(*scroll_info);
  (*expose_info).y=(*slider_info).y;
}

static void InitializeFileBrowserList(const FileBrowserContext *context,
  const FileBrowserWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info,
    *const list_info = widgets->list_info;

  /*
    Initialize list information.
  */
  XGetWidgetInfo((char *) NULL,list_info);
  (*list_info).raised=MagickFalse;
  (*list_info).bevel_width--;
  (*list_info).width=(unsigned int)
    ((*scroll_info).x-(*reply_info).x-(int) (QuantumMargin >> 1));
  (*list_info).height=(*scroll_info).height;
  (*list_info).x=(*reply_info).x;
  (*list_info).y=(*scroll_info).y;
  if (windows->widget.mapped == MagickFalse)
    (*state)|=JumpListState;
}

static void InitializeFileBrowserText(const FileBrowserContext *context,
  const FileBrowserWidgets *widgets)
{
  const unsigned int
    height = context->height;

  char
    *const text = context->text;

  XFontStruct
    *const font_info = context->font_info;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const list_info = widgets->list_info,
    *const text_info = widgets->text_info,
    *const selection_info = widgets->selection_info;

  /*
    Initialize text information.
  */
  *text='\0';
  XGetWidgetInfo(text,text_info);
  (*text_info).center=MagickFalse;
  (*text_info).width=(*reply_info).width;
  (*text_info).height=height;
  (*text_info).x=(*list_info).x-(int) (QuantumMargin >> 1);
  (*text_info).y=QuantumMargin;
  /*
    Initialize selection information.
  */
  XGetWidgetInfo((char *) NULL,selection_info);
  (*selection_info).center=MagickFalse;
  (*selection_info).width=(*list_info).width;
  (*selection_info).height=(unsigned int) ((9*height) >> 3);
  (*selection_info).x=(*list_info).x;
}

static void RedrawFileBrowserWidget(const FileBrowserContext *context,
  const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern,
    *const working_path = context->working_path;

  const unsigned int
    height = context->height;

  XFontStruct
    *const font_info = context->font_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const up_info = widgets->up_info,
    *const home_info = widgets->home_info,
    *const special_info = widgets->special_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info,
    *const slider_info = widgets->slider_info,
    *const scroll_info = widgets->scroll_info,
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info,
    *const list_info = widgets->list_info,
    *const text_info = widgets->text_info,
    *const selection_info = widgets->selection_info;

  int
    x,
    y;

  /*
    Redraw File Browser window.
  */
  x=QuantumMargin;
  y=(*text_info).y+(int) (((*text_info).height-height) >> 1)+
    font_info->ascent;
  (void) XDrawString(display,windows->widget.id,
    windows->widget.annotate_context,x,y,DirectoryText,
    Extent(DirectoryText));
  (void) CopyMagickString((*text_info).text,working_path,MagickPathExtent);
  (void) ConcatenateMagickString((*text_info).text,DirectorySeparator,
    MagickPathExtent);
  (void) ConcatenateMagickString((*text_info).text,glob_pattern,
    MagickPathExtent);
  XDrawWidgetText(display,&windows->widget,text_info);
  XDrawBeveledButton(display,&windows->widget,up_info);
  XDrawBeveledButton(display,&windows->widget,home_info);
  XDrawBeveledMatte(display,&windows->widget,list_info);
  XDrawBeveledMatte(display,&windows->widget,scroll_info);
  XDrawTriangleNorth(display,&windows->widget,north_info);
  XDrawBeveledButton(display,&windows->widget,slider_info);
  XDrawTriangleSouth(display,&windows->widget,south_info);
  x=QuantumMargin;
  y=(*reply_info).y+(int) (((*reply_info).height-height) >> 1)+
    font_info->ascent;
  (void) XDrawString(display,windows->widget.id,
    windows->widget.annotate_context,x,y,FilenameText,
    Extent(FilenameText));
  XDrawBeveledMatte(display,&windows->widget,reply_info);
  XDrawMatteText(display,&windows->widget,reply_info);
  XDrawBeveledButton(display,&windows->widget,special_info);
  XDrawBeveledButton(display,&windows->widget,action_info);
  XDrawBeveledButton(display,&windows->widget,cancel_info);
  XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
  (*selection_info).id=(~0);
  (*state)|=RedrawListState;
  (*state)&=(unsigned int) (~RedrawWidgetState);
}

static MagickBooleanType ReplaceFileBrowserList(
  const FileBrowserContext *context,const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern,
    *const working_path = context->working_path;

  XWidgetInfo
    *const action_info = widgets->action_info;

  size_t
    *const files = widgets->files;

  ssize_t
    *const i = widgets->i;

  char
    ***const filelist = widgets->filelist;

  char
    **checklist;

  size_t
    number_files;

  /*
    Update file list.
  */
  checklist=ListFiles(working_path,glob_pattern,&number_files);
  if (checklist == (char **) NULL)
    {
      /*
        Reply is a filename, exit.
      */
      (*action_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,action_info);
      return(MagickFalse);
    }
  for ((*i)=0; (*i) < (ssize_t) (*files); (*i)++)
    (*filelist)[(*i)]=DestroyString((*filelist)[(*i)]);
  if ((*filelist) != (char **) NULL)
    (*filelist)=(char **) RelinquishMagickMemory((*filelist));
  (*filelist)=checklist;
  (*files)=number_files;
  return(MagickTrue);
}

static void RefreshFileBrowserList(const FileBrowserContext *context,
  const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern,
    *const working_path = context->working_path;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const scroll_info = widgets->scroll_info,
    *const slider_info = widgets->slider_info,
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info,
    *const list_info = widgets->list_info,
    *const expose_info = widgets->expose_info,
    *const text_info = widgets->text_info,
    *const selection_info = widgets->selection_info;

  size_t
    *const files = widgets->files;

  unsigned int
    *const visible_files = widgets->visible_files;

  /*
    Update file list.
  */
  (*slider_info).height=(unsigned int) ((int) (*scroll_info).height-
    (((*slider_info).min_y-(*scroll_info).y+1) << 1)+1);
  if ((*files) > (*visible_files))
    (*slider_info).height=(unsigned int)
      (((*visible_files)*(*slider_info).height)/(*files));
  (*slider_info).max_y=(*south_info).y-(int) (*south_info).bevel_width-
    (int) (*slider_info).bevel_width-2;
  (*slider_info).id=0;
  (*slider_info).y=(*slider_info).min_y;
  (*expose_info).y=(*slider_info).y;
  (*selection_info).id=(~0);
  (*list_info).id=(~0);
  (*state)|=RedrawListState;
  /*
    Redraw directory name & reply.
  */
  if (IsGlob((*reply_info).text) == MagickFalse)
    {
      *(*reply_info).text='\0';
      (*reply_info).cursor=(*reply_info).text;
    }
  (void) CopyMagickString((*text_info).text,working_path,MagickPathExtent);
  (void) ConcatenateMagickString((*text_info).text,DirectorySeparator,
    MagickPathExtent);
  (void) ConcatenateMagickString((*text_info).text,glob_pattern,
    MagickPathExtent);
  XDrawWidgetText(display,&windows->widget,text_info);
  XDrawMatteText(display,&windows->widget,reply_info);
  XDrawBeveledMatte(display,&windows->widget,scroll_info);
  XDrawTriangleNorth(display,&windows->widget,north_info);
  XDrawBeveledButton(display,&windows->widget,slider_info);
  XDrawTriangleSouth(display,&windows->widget,south_info);
  XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
  (*state)&=(unsigned int) (~UpdateListState);
}

static void JumpFileBrowserList(const FileBrowserContext *context,
  const FileBrowserWidgets *widgets)
{
  char
    *const reply = context->reply;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info,
    *const list_info = widgets->list_info,
    *const selection_info = widgets->selection_info;

  unsigned int
    *const visible_files = widgets->visible_files;

  size_t
    *const files = widgets->files;

  ssize_t
    *const i = widgets->i;

  char
    ***const filelist = widgets->filelist;

  FileBrowserMatch
    match;

  /*
    Jump scroll to match user filename.
  */
  (*list_info).id=(~0);
  match.filelist=(*filelist);
  match.files=(*files);
  match.reply=reply;
  MatchFileBrowserReply(&match,list_info,i);
  if (((*i) < (ssize_t) (*slider_info).id) ||
      ((*i) >= ((*slider_info).id+(ssize_t) (*visible_files))))
    (*slider_info).id=(*i)-(int) ((*visible_files) >> 1);
  (*selection_info).id=(~0);
  (*state)|=RedrawListState;
  (*state)&=(unsigned int) (~JumpListState);
}

static void RedrawFileBrowserList(const FileBrowserContext *context,
  const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    height = context->height;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info,
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info,
    *const list_info = widgets->list_info,
    *const expose_info = widgets->expose_info,
    *const selection_info = widgets->selection_info;

  unsigned int
    *const visible_files = widgets->visible_files;

  size_t
    *const files = widgets->files;

  char
    ***const filelist = widgets->filelist;

  /*
    Determine slider id and position.
  */
  if ((*slider_info).id >= (int) ((*files)-(*visible_files)))
    (*slider_info).id=(int) ((*files)-(*visible_files));
  if (((*slider_info).id < 0) || ((*files) <= (*visible_files)))
    (*slider_info).id=0;
  (*slider_info).y=(*slider_info).min_y;
  if ((*files) > 0)
    (*slider_info).y+=((int) (*slider_info).id*((*slider_info).max_y-
      (*slider_info).min_y+1)/(int) (*files));
  if ((*slider_info).id != (*selection_info).id)
    {
      FileBrowserNames
        names;

      /*
        Redraw scroll bar and file names.
      */
      (*selection_info).id=(*slider_info).id;
      (*selection_info).y=(*list_info).y+(int) (height >> 3)+2;
      names.display=display;
      names.windows=windows;
      names.filelist=(*filelist);
      names.files=(*files);
      names.visible_files=(*visible_files);
      names.list_info=(*list_info);
      names.slider_info=(*slider_info);
      DrawFileBrowserNames(&names,selection_info);
      /*
        Update slider.
      */
      (*expose_info)=PlaceTextViewExpose((*expose_info),(*slider_info));
      XDrawTriangleNorth(display,&windows->widget,north_info);
      XDrawMatte(display,&windows->widget,expose_info);
      XDrawBeveledButton(display,&windows->widget,slider_info);
      XDrawTriangleSouth(display,&windows->widget,south_info);
      (*expose_info).y=(*slider_info).y;
    }
  (*state)&=(unsigned int) (~RedrawListState);
}

static MagickBooleanType WaitForFileBrowserEvent(
  const FileBrowserContext *context,const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info,
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info;

  size_t
    *const files = widgets->files,
    *const delay = widgets->delay;

  XEvent
    *const event = widgets->event;

  if ((*north_info).raised && (*south_info).raised)
    (void) XIfEvent(display,event,XScreenEvent,(char *) windows);
  else
    {
      /*
        Brief delay before advancing scroll bar.
      */
      XDelay(display,(*delay));
      (*delay)=SuspendTime;
      (void) XCheckIfEvent(display,event,XScreenEvent,(char *) windows);
      if (((*north_info).raised == MagickFalse) && ((*slider_info).id > 0))
        {
          /*
            Move slider up.
          */
          (*slider_info).id--;
          (*state)|=RedrawListState;
        }
      if (((*south_info).raised == MagickFalse) &&
          ((*slider_info).id < (int) (*files)))
        {
          /*
            Move slider down.
          */
          (*slider_info).id++;
          (*state)|=RedrawListState;
        }
      if ((*event).type != ButtonRelease)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType UpdateFileBrowserState(
  const FileBrowserContext *context,const FileBrowserWidgets *widgets)
{
  size_t
    *const state = widgets->state;

  if ((*state) & UpdateConfigurationState)
    {
      InitializeFileBrowserButtons(context,widgets);
      InitializeFileBrowserReply(context,widgets);
      /*
        Initialize scroll information.
      */
      InitializeFileBrowserScroll(context,widgets);
      InitializeFileBrowserList(context,widgets);
      InitializeFileBrowserText(context,widgets);
      (*state)&=(unsigned int) (~UpdateConfigurationState);
    }
  if ((*state) & RedrawWidgetState)
    RedrawFileBrowserWidget(context,widgets);
  if ((*state) & UpdateListState)
    {
      if (ReplaceFileBrowserList(context,widgets) == MagickFalse)
        return(MagickFalse);
      RefreshFileBrowserList(context,widgets);
    }
  if ((*state) & JumpListState)
    JumpFileBrowserList(context,widgets);
  if ((*state) & RedrawListState)
    RedrawFileBrowserList(context,widgets);
  return(MagickTrue);
}

static void HandleFileBrowserSelectionEvent(
  const FileBrowserContext *context,XEvent event,
  const FileBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const primary_selection = context->primary_selection;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  switch (event.type)
  {
    case MotionNotify:
    {
      MoveFileBrowserButtons(context,event,widgets);
      break;
    }
    case SelectionClear:
    {
      (*reply_info).highlight=MagickFalse;
      XDrawMatteText(display,&windows->widget,reply_info);
      break;
    }
    case SelectionNotify:
    {
      SelectFileBrowserReply(context,event,widgets);
      break;
    }
    case SelectionRequest:
    {
      XSelectionEvent
        notify;

      XSelectionRequestEvent
        *request;

      if ((*reply_info).highlight == MagickFalse)
        break;
      /*
        Set primary selection.
      */
      request=(&(event.xselectionrequest));
      (void) XChangeProperty(request->display,request->requestor,
        request->property,request->target,8,PropModeReplace,
        (unsigned char *) primary_selection,Extent(primary_selection));
      notify.type=SelectionNotify;
      notify.display=request->display;
      notify.requestor=request->requestor;
      notify.selection=request->selection;
      notify.target=request->target;
      notify.time=request->time;
      if (request->property == None)
        notify.property=request->target;
      else
        notify.property=request->property;
      (void) XSendEvent(request->display,request->requestor,False,0,
        (XEvent *) &notify);
    }
    default:
      break;
  }
}

static void HandleFileBrowserWindowEvent(
  const FileBrowserContext *context,const XEvent event,
  const FileBrowserTargets *targets,const FileBrowserWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  switch (event.type)
  {
    case ButtonPress:
    {
      PressFileBrowserScroll(context,event,targets,widgets);
      break;
    }
    case ButtonRelease:
    {
      ReleaseFileBrowserScroll(context,event,widgets);
      break;
    }
    case ClientMessage:
    {
      ClientFileBrowserMessage(context,event,widgets);
      break;
    }
    case ConfigureNotify:
    {
      (*state)=ConfigureWidgetWindow(windows,event,(*state));
      break;
    }
    case EnterNotify:
    {
      (*state)=EnterWidgetWindow(windows,event,(*state));
      break;
    }
    case Expose:
    {
      (*state)=ExposeWidgetWindow(windows,event,(*state));
      break;
    }
    default:
    {
      HandleFileBrowserSelectionEvent(context,event,widgets);
      break;
    }
  }
}

static void HandleFileBrowserKeyEvent(
  const FileBrowserContext *context,const XEvent event,
  const FileBrowserTargets *targets,const FileBrowserWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  switch (event.type)
  {
    case KeyPress:
    {
      PressFileBrowserKey(context,event,widgets);
      break;
    }
    case KeyRelease:
    {
      (*state)=ReleaseWidgetKey(windows,event,(*state));
      break;
    }
    case LeaveNotify:
    {
      (*state)=LeaveWidgetWindow(windows,event,(*state));
      break;
    }
    default:
    {
      HandleFileBrowserWindowEvent(context,event,targets,widgets);
      break;
    }
  }
}

static unsigned int GetFileBrowserWidth(XFontStruct *font_info,
  const char *action)
{
  unsigned int
    width;

  width=WidgetTextWidth(font_info,(char *) action);
  if (WidgetTextWidth(font_info,GrabButtonText) > width)
    width=WidgetTextWidth(font_info,GrabButtonText);
  if (WidgetTextWidth(font_info,FormatButtonText) > width)
    width=WidgetTextWidth(font_info,FormatButtonText);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  if (WidgetTextWidth(font_info,HomeButtonText) > width)
    width=WidgetTextWidth(font_info,HomeButtonText);
  if (WidgetTextWidth(font_info,UpButtonText) > width)
    width=WidgetTextWidth(font_info,UpButtonText);
  width+=(unsigned int) QuantumMargin;
  if (WidgetTextWidth(font_info,DirectoryText) > width)
    width=WidgetTextWidth(font_info,DirectoryText);
  if (WidgetTextWidth(font_info,FilenameText) > width)
    width=WidgetTextWidth(font_info,FilenameText);
  return(width);
}

static unsigned int WidestFileBrowserName(XFontStruct *font_info,
  char **filelist,size_t files)
{
  ssize_t
    i;

  unsigned int
    text_width;

  text_width=0;
  for (i=0; i < (ssize_t) files; i++)
    if (WidgetTextWidth(font_info,filelist[i]) > text_width)
      text_width=WidgetTextWidth(font_info,filelist[i]);
  return(text_width);
}

static void MapFileBrowserWidget(Display *display,XWindows *windows,
  MagickStatusType mask)
{
  Status
    status;

  XTextProperty
    window_name;

  XWindowChanges
    window_changes;

  /*
    Map File Browser widget.
  */
  (void) CopyMagickString(windows->widget.name,"Browse and Select a File",
    MagickPathExtent);
  status=XStringListToTextProperty(&windows->widget.name,1,&window_name);
  if (status != False)
    {
      XSetWMName(display,windows->widget.id,&window_name);
      XSetWMIconName(display,windows->widget.id,&window_name);
      (void) XFree((void *) window_name.value);
    }
  window_changes.width=(int) windows->widget.width;
  window_changes.height=(int) windows->widget.height;
  window_changes.x=windows->widget.x;
  window_changes.y=windows->widget.y;
  (void) XReconfigureWMWindow(display,windows->widget.id,
    windows->widget.screen,mask,&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
}

static void RespondToFileBrowserEvents(const FileBrowserContext *context,
  const FileBrowserTargets *targets,const FileBrowserWidgets *widgets)
{
  size_t
    *const state = widgets->state;

  XEvent
    *const event = widgets->event;

  MagickStatusType
    *const mask = widgets->mask;

  do
  {
    if (UpdateFileBrowserState(context,widgets) == MagickFalse)
      break;
    /*
      Wait for next event.
    */
    if (WaitForFileBrowserEvent(context,widgets) == MagickFalse)
      continue;
    switch ((*event).type)
    {
      case MapNotify:
      {
        (*mask)&=(unsigned int) (~CWX);
        (*mask)&=(unsigned int) (~CWY);
        break;
      }
      default:
      {
        HandleFileBrowserKeyEvent(context,(*event),targets,widgets);
        break;
      }
    }
  } while (((*state) & ExitState) == 0);
}

static unsigned int GetFileBrowserAnomaly(const char *action)
{
  unsigned int
    anomaly;

  anomaly=(LocaleCompare(action,"Composite") == 0) ||
    (LocaleCompare(action,"Open") == 0) || (LocaleCompare(action,"Map") == 0);
  return(anomaly);
}

typedef struct _FileBrowserLayout
{
  Display
    *display;

  XFontStruct
    *font_info;

  unsigned int
    width,
    height,
    text_width;
} FileBrowserLayout;

static void PositionFileBrowserWidget(XWindows *windows,
  const FileBrowserLayout *layout)
{
  Display
    *const display = layout->display;

  XFontStruct
    *const font_info = layout->font_info;

  const unsigned int
    width = layout->width,
    height = layout->height,
    text_width = layout->text_width;

  /*
    Position File Browser widget.
  */
  windows->widget.width=width+MagickMin(text_width,MaxTextWidth)+
    (unsigned int) (6*QuantumMargin);
  windows->widget.min_width=width+MinTextWidth+(unsigned int) (4*QuantumMargin);
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int)
    (((81*height) >> 2)+(unsigned int) ((13*QuantumMargin) >> 1)+4);
  windows->widget.min_height=(unsigned int)
    (((23*height) >> 1)+(unsigned int) ((13*QuantumMargin) >> 1)+4);
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
}

typedef struct _FileBrowserRunTargets
{
  size_t
    *files;

  ssize_t
    *i;

  char
    ***filelist;

  MagickStatusType
    *mask;
} FileBrowserRunTargets;

static void RunFileBrowser(const FileBrowserContext *context,
  const FileBrowserRunTargets *run_targets)
{
  size_t
    *const files = run_targets->files;

  ssize_t
    *const i = run_targets->i;

  char
    ***const filelist = run_targets->filelist;

  MagickStatusType
    *const mask = run_targets->mask;

  size_t
    delay,
    state;

  unsigned int
    visible_files;

  XEvent
    event;

  XWidgetInfo
    action_info,
    cancel_info,
    expose_info,
    special_info,
    list_info,
    home_info,
    north_info,
    reply_info,
    scroll_info,
    selection_info,
    slider_info,
    south_info,
    text_info,
    up_info;

  const FileBrowserTargets
    targets = { &reply_info, &state, &selection_info, &list_info };

  const FileBrowserWidgets
    file_widgets = { &reply_info, &state,
      &scroll_info, &slider_info, &visible_files, files, &up_info, &home_info,
      &special_info, &action_info, &cancel_info, i, filelist, &north_info,
      &south_info, &list_info, &delay, &expose_info, &text_info,
      &selection_info, &event, mask };

  /*
    Respond to X events.
  */
  XGetWidgetInfo((char *) NULL,&slider_info);
  XGetWidgetInfo((char *) NULL,&north_info);
  XGetWidgetInfo((char *) NULL,&south_info);
  XGetWidgetInfo((char *) NULL,&expose_info);
  visible_files=0;
  delay=SuspendTime << 2;
  state=UpdateConfigurationState;
  RespondToFileBrowserEvents(context,&targets,&file_widgets);
}

static void SetFileBrowserReply(char *reply,char *working_path)
{
  if (*reply != '\0')
    {
      (void) ConcatenateMagickString(working_path,DirectorySeparator,
        MagickPathExtent);
      (void) ConcatenateMagickString(working_path,reply,MagickPathExtent);
    }
  (void) CopyMagickString(reply,working_path,MagickPathExtent);
  if (*reply == '~')
    ExpandFilename(reply);
}

static void FreeFileBrowserList(char **filelist,size_t files)
{
  ssize_t
    i;

  /*
    Free file list.
  */
  for (i=0; i < (ssize_t) files; i++)
    filelist[i]=DestroyString(filelist[i]);
  if (filelist != (char **) NULL)
    filelist=(char **) RelinquishMagickMemory(filelist);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X F i l e B r o w s e r W i d g e t                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XFileBrowserWidget() displays a File Browser widget with a file query to the
%  user.  The user keys a reply and presses the Action or Cancel button to
%  exit.  The typed text is returned as the reply function parameter.
%
%  The format of the XFileBrowserWidget method is:
%
%      void XFileBrowserWidget(Display *display,XWindows *windows,
%        const char *action,char *reply)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o action: Specifies a pointer to the action of this widget.
%
%    o reply: the response from the user is returned in this parameter.
%
*/
MagickPrivate void XFileBrowserWidget(Display *display,XWindows *windows,
  const char *action,char *reply)
{
  char
    *directory,
    **filelist,
    home_directory[MagickPathExtent],
    primary_selection[MagickPathExtent],
    text[MagickPathExtent],
    working_path[MagickPathExtent];

  static char
    glob_pattern[MagickPathExtent] = "*",
    format[MagickPathExtent] = "miff";

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  size_t
    files;

  ssize_t
    i;

  unsigned int
    anomaly,
    height,
    text_width,
    width;

  XFontStruct
    *font_info;

  const FileBrowserRunTargets
    run_targets = { &files, &i, &filelist, &mask };

  /*
    Read filelist from current directory.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(action != (char *) NULL);
  assert(reply != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  directory=getcwd(home_directory,MagickPathExtent);
  (void) directory;
  (void) CopyMagickString(working_path,home_directory,MagickPathExtent);
  filelist=ListFiles(working_path,glob_pattern,&files);
  if (filelist == (char **) NULL)
    {
      /*
        Directory read failed.
      */
      XNoticeWidget(display,windows,"Unable to read directory:",working_path);
      (void) XDialogWidget(display,windows,action,"Enter filename:",reply);
      return;
    }
  /*
    Determine File Browser widget attributes.
  */
  font_info=windows->widget.font_info;
  text_width=WidestFileBrowserName(font_info,filelist,files);
  width=GetFileBrowserWidth(font_info,action);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position File Browser widget.
  */
  {
    const FileBrowserLayout
      layout = { display, font_info, width, height, text_width };

    PositionFileBrowserWidget(windows,&layout);
  }
  MapFileBrowserWidget(display,windows,mask);
  /*
    Respond to X events.
  */
  anomaly=GetFileBrowserAnomaly(action);
  {
    const FileBrowserContext
      file_context = { display, windows, reply, glob_pattern, working_path,
        primary_selection, font_info, height, format, anomaly, home_directory,
        action, width, text };

    RunFileBrowser(&file_context,&run_targets);
  }
  WithdrawBrowserWidget(display,windows);
  FreeFileBrowserList(filelist,files);
  SetFileBrowserReply(reply,working_path);
}
#endif
