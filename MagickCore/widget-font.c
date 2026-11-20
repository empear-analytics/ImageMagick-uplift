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
%                         FFFFF   OOO   N   N  TTTTT                          %
%                         F      O   O  NN  N    T                            %
%                         FFF    O   O  N N N    T                            %
%                         F      O   O  N  NN    T                            %
%                         F       OOO   N   N    T                            %
%                                                                             %
%                                                                             %
%                 MagickCore X11 Font Browser Widget Methods                  %
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
#include "MagickCore/widget-font-actions-private.h"
#if defined(MAGICKCORE_X11_DELEGATE)

static void InitializeFontBrowserScroll(const FontBrowserContext *context,
  const FontBrowserWidgets *widgets)
{
  const unsigned int
    height = context->height;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const back_info = widgets->back_info,
    *const slider_info = widgets->slider_info;

  int
    *const fonts = widgets->fonts;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_fonts = widgets->visible_fonts;

  XWidgetInfo
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info;

  XFontStruct
    *const font_info = *widgets->font_info;

  XWidgetInfo
    *const expose_info = widgets->expose_info;

  int
    id;

  /*
    Initialize scroll information.
  */
  XGetWidgetInfo((char *) NULL,scroll_info);
  (*scroll_info).bevel_width--;
  (*scroll_info).width=height;
  (*scroll_info).height=(unsigned int)
    ((*reply_info).y-(*back_info).y-(int) (QuantumMargin >> 1));
  (*scroll_info).x=(*reply_info).x+(int) ((*reply_info).width-
    (*scroll_info).width);
  (*scroll_info).y=(*back_info).y-(int) (*reply_info).bevel_width;
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
  (*visible_fonts)=(unsigned int) ((*scroll_info).height*
    MagickSafeReciprocal((double) height+(height >> 3)));
  if ((*fonts) > (int) (*visible_fonts))
    (*slider_info).height=((*visible_fonts)*(*slider_info).height)/
    (unsigned int)
      (*fonts);
  (*slider_info).max_y=(*south_info).y-(int) (*south_info).bevel_width-
    (int) (*slider_info).bevel_width-2;
  (*slider_info).x=(*scroll_info).x+(int) (*slider_info).bevel_width+1;
  (*slider_info).y=(*slider_info).min_y;
  (*expose_info)=(*scroll_info);
  (*expose_info).y=(*slider_info).y;
}

static void InitializeFontBrowserList(const FontBrowserContext *context,
  const FontBrowserWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  XFontStruct
    *const font_info = *widgets->font_info;

  XWidgetInfo
    *const list_info = widgets->list_info;

  /*
    Initialize list information.
  */
  XGetWidgetInfo((char *) NULL,list_info);
  (*list_info).raised=MagickFalse;
  (*list_info).bevel_width--;
  (*list_info).width=(unsigned int) ((*scroll_info).x-(*reply_info).x-
    (QuantumMargin >> 1));
  (*list_info).height=(*scroll_info).height;
  (*list_info).x=(*reply_info).x;
  (*list_info).y=(*scroll_info).y;
  if (windows->widget.mapped == MagickFalse)
    (*state)|=JumpListState;
}

static void InitializeFontBrowserText(const FontBrowserContext *context,
  const FontBrowserWidgets *widgets)
{
  const unsigned int
    height = context->height;

  char
    *const text = context->text;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  XFontStruct
    *const font_info = *widgets->font_info;

  XWidgetInfo
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
  (*text_info).x=(*list_info).x-(QuantumMargin >> 1);
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

static void UpdateFontBrowserLayout(const FontBrowserContext *context,
  const FontBrowserWidgets *widgets)
{
  size_t
    *const state = widgets->state;

  if ((*state) & UpdateConfigurationState)
    {
      InitializeFontBrowserButtons(context,widgets);
      InitializeFontBrowserReply(context,widgets);
      InitializeFontBrowserScroll(context,widgets);
      InitializeFontBrowserList(context,widgets);
      InitializeFontBrowserText(context,widgets);
      (*state)&=(unsigned int) (~UpdateConfigurationState);
    }
}

static void RedrawFontBrowserWidget(const FontBrowserContext *context,
  const FontBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const back_info = widgets->back_info,
    *const reset_info = widgets->reset_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info,
    *const slider_info = widgets->slider_info,
    *const scroll_info = widgets->scroll_info,
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info;

  XFontStruct
    *const font_info = *widgets->font_info;

  XWidgetInfo
    *const list_info = widgets->list_info,
    *const text_info = widgets->text_info,
    *const selection_info = widgets->selection_info;

  int
    x,
    y;

  if ((*state) & RedrawWidgetState)
    {
      /*
        Redraw Font Browser window.
      */
      x=QuantumMargin;
      y=(*text_info).y+(int) (((*text_info).height-height) >> 1)+
        font_info->ascent;
      (void) XDrawString(display,windows->widget.id,
        windows->widget.annotate_context,x,y,FontPatternText,
        Extent(FontPatternText));
      (void) CopyMagickString((*text_info).text,glob_pattern,MagickPathExtent);
      XDrawWidgetText(display,&windows->widget,text_info);
      XDrawBeveledButton(display,&windows->widget,back_info);
      XDrawBeveledButton(display,&windows->widget,reset_info);
      XDrawBeveledMatte(display,&windows->widget,list_info);
      XDrawBeveledMatte(display,&windows->widget,scroll_info);
      XDrawTriangleNorth(display,&windows->widget,north_info);
      XDrawBeveledButton(display,&windows->widget,slider_info);
      XDrawTriangleSouth(display,&windows->widget,south_info);
      x=QuantumMargin;
      y=(*reply_info).y+(int) (((*reply_info).height-height) >> 1)+
        font_info->ascent;
      (void) XDrawString(display,windows->widget.id,
        windows->widget.annotate_context,x,y,FontnameText,
        Extent(FontnameText));
      XDrawBeveledMatte(display,&windows->widget,reply_info);
      XDrawMatteText(display,&windows->widget,reply_info);
      XDrawBeveledButton(display,&windows->widget,action_info);
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
      (*selection_info).id=(~0);
      (*state)|=RedrawActionState;
      (*state)|=RedrawListState;
      (*state)&=(unsigned int) (~RedrawWidgetState);
    }
}

static void JumpFontBrowserList(const FontBrowserContext *context,
  const FontBrowserWidgets *widgets)
{
  char
    *const reply = context->reply;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  int
    *const fonts = widgets->fonts;

  unsigned int
    *const visible_fonts = widgets->visible_fonts;

  int
    *const i = widgets->i;

  char
    ***const fontlist = widgets->fontlist;

  XWidgetInfo
    *const list_info = widgets->list_info,
    *const selection_info = widgets->selection_info;

  if ((*state) & JumpListState)
    {
      FontBrowserMatch
        match;

      /*
        Jump scroll to match user font.
      */
      (*list_info).id=(~0);
      match.fontlist=(*fontlist);
      match.fonts=(*fonts);
      match.reply=reply;
      MatchFontBrowserReply(&match,list_info,i);
      if (((*i) < (*slider_info).id) || ((*i) >= ((*slider_info).id+
        (int) (*visible_fonts))))
        (*slider_info).id=(*i)-((int) (*visible_fonts) >> 1);
      (*selection_info).id=(~0);
      (*state)|=RedrawListState;
      (*state)&=(unsigned int) (~JumpListState);
    }
}

static void RedrawFontBrowserList(const FontBrowserContext *context,
  const FontBrowserWidgets *widgets)
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
    *const slider_info = widgets->slider_info;

  int
    *const fonts = widgets->fonts;

  unsigned int
    *const visible_fonts = widgets->visible_fonts;

  XWidgetInfo
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info;

  int
    *const i = widgets->i;

  char
    ***const fontlist = widgets->fontlist;

  XWidgetInfo
    *const list_info = widgets->list_info,
    *const expose_info = widgets->expose_info,
    *const selection_info = widgets->selection_info;

  if ((*state) & RedrawListState)
    {
      /*
        Determine slider id and position.
      */
      if ((*slider_info).id >= ((*fonts)-(int) (*visible_fonts)))
        (*slider_info).id=(*fonts)-(int) (*visible_fonts);
      if (((*slider_info).id < 0) || ((*fonts) <= (int) (*visible_fonts)))
        (*slider_info).id=0;
      (*slider_info).y=(*slider_info).min_y;
      if ((*fonts) > 0)
        (*slider_info).y+=
          (*slider_info).id*((*slider_info).max_y-(*slider_info).min_y+1)/
          (*fonts);
      if ((*slider_info).id != (*selection_info).id)
        {
          FontBrowserNames
            names;

          /*
            Redraw scroll bar and file names.
          */
          (*selection_info).id=(*slider_info).id;
          (*selection_info).y=(*list_info).y+(int) (height >> 3)+2;
          names.display=display;
          names.windows=windows;
          names.fontlist=(*fontlist);
          names.fonts=(*fonts);
          names.visible_fonts=(*visible_fonts);
          names.list_info=(*list_info);
          names.slider_info=(*slider_info);
          DrawFontBrowserNames(&names,selection_info,i);
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
}

static void RedrawFontBrowserAction(const FontBrowserContext *context,
  const FontBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XFontStruct
    **const font_info = widgets->font_info;

  XWidgetInfo
    *const mode_info = widgets->mode_info;

  if ((*state) & RedrawActionState)
    {
      XFontStruct
        *save_info;

      /*
        Display the selected font in a drawing area.
      */
      save_info=windows->widget.font_info;
      (*font_info)=XLoadQueryFont(display,(*reply_info).text);
      if ((*font_info) != (XFontStruct *) NULL)
        {
          windows->widget.font_info=(*font_info);
          (void) XSetFont(display,windows->widget.widget_context,
            (*font_info)->fid);
        }
      XDrawBeveledButton(display,&windows->widget,mode_info);
      windows->widget.font_info=save_info;
      if ((*font_info) != (XFontStruct *) NULL)
        {
          (void) XSetFont(display,windows->widget.widget_context,
            windows->widget.font_info->fid);
          (void) XFreeFont(display,(*font_info));
        }
      XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
      XDrawMatteText(display,&windows->widget,reply_info);
      (*state)&=(unsigned int) (~RedrawActionState);
    }
}

static MagickBooleanType ReloadFontList(const FontBrowserContext *context,
  const FontBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern,
    *const back_pattern = context->back_pattern,
    *const reply = context->reply;

  XWidgetInfo
    *const action_info = widgets->action_info;

  int
    *const fonts = widgets->fonts;

  char
    ***const fontlist = widgets->fontlist,
    ***const listhead = widgets->listhead;

  const FontBrowserUpdateTargets
    update_targets = { action_info, fontlist, fonts };

  FontBrowserUpdate
    update;

  /*
    Update font list.
  */
  update.display=display;
  update.windows=windows;
  update.reply=reply;
  update.glob_pattern=glob_pattern;
  update.back_pattern=back_pattern;
  update.listhead=(*listhead);
  if (UpdateFontBrowserList(&update,&update_targets) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType SortFontList(const FontBrowserContext *context,
  const FontBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  int
    *const fonts = widgets->fonts,
    *const i = widgets->i;

  char
    ***const fontlist = widgets->fontlist,
    ***const listhead = widgets->listhead;

  /*
    Sort font list in ascending order.
  */
  (*listhead)=(*fontlist);
  (*fontlist)=(char **) AcquireQuantumMemory((size_t) (*fonts),
    sizeof(*(*fontlist)));
  if ((*fontlist) == (char **) NULL)
    {
      XNoticeWidget(display,windows,"MemoryAllocationFailed",
        "UnableToViewFonts");
      return(MagickFalse);
    }
  for ((*i)=0; (*i) < (*fonts); (*i)++)
    (*fontlist)[(*i)]=(*listhead)[(*i)];
  qsort((void *) (*fontlist),(size_t) (*fonts),sizeof(*(*fontlist)),
    FontCompare);
  return(MagickTrue);
}

static void UpdateFontBrowserSlider(const FontBrowserContext *context,
  const FontBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  int
    *const fonts = widgets->fonts;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_fonts = widgets->visible_fonts;

  XWidgetInfo
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info,
    *const list_info = widgets->list_info,
    *const expose_info = widgets->expose_info,
    *const text_info = widgets->text_info,
    *const selection_info = widgets->selection_info;

  (*slider_info).height=(unsigned int) ((int) (*scroll_info).height-
    (((*slider_info).min_y-(*scroll_info).y+1) << 1)+1);
  if ((*fonts) > (int) (*visible_fonts))
    (*slider_info).height=((*visible_fonts)*(*slider_info).height)/
    (unsigned int)
      (*fonts);
  (*slider_info).max_y=(*south_info).y-(int) (*south_info).bevel_width-(int)
    (*slider_info).bevel_width-2;
  (*slider_info).id=0;
  (*slider_info).y=(*slider_info).min_y;
  (*expose_info).y=(*slider_info).y;
  (*selection_info).id=(~0);
  (*list_info).id=(~0);
  (*state)|=RedrawListState;
  /*
    Redraw font name & reply.
  */
  *(*reply_info).text='\0';
  (*reply_info).cursor=(*reply_info).text;
  (void) CopyMagickString((*text_info).text,glob_pattern,MagickPathExtent);
  XDrawWidgetText(display,&windows->widget,text_info);
  XDrawMatteText(display,&windows->widget,reply_info);
  XDrawBeveledMatte(display,&windows->widget,scroll_info);
  XDrawTriangleNorth(display,&windows->widget,north_info);
  XDrawBeveledButton(display,&windows->widget,slider_info);
  XDrawTriangleSouth(display,&windows->widget,south_info);
  XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
  (*state)&=(unsigned int) (~UpdateListState);
}

static MagickBooleanType WaitFontEvent(const FontBrowserContext *context,
  XEvent *event,const FontBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  int
    *const fonts = widgets->fonts;

  XWidgetInfo
    *const north_info = widgets->north_info;

  size_t
    *const delay = widgets->delay;

  XWidgetInfo
    *const south_info = widgets->south_info;

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
          ((*slider_info).id < (*fonts)))
        {
          /*
            Move slider down.
          */
          (*slider_info).id++;
          (*state)|=RedrawListState;
        }
      if ((*event).type != ButtonRelease)
        return(MagickTrue);
    }
  return(MagickFalse);
}

static void HandleFontSelectionEvent(const FontBrowserContext *context,
  XEvent event,const FontBrowserWidgets *widgets)
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
    case SelectionClear:
    {
      (*reply_info).highlight=MagickFalse;
      XDrawMatteText(display,&windows->widget,reply_info);
      break;
    }
    case SelectionNotify:
    {
      SelectFontBrowserReply(context,event,widgets);
      break;
    }
    case SelectionRequest:
    {
      XSelectionEvent
        notify;

      XSelectionRequestEvent
        *request;

      /*
        Set XA_PRIMARY selection.
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

static void HandleFontKeyEvent(const FontBrowserContext *context,
  XEvent event,const FontBrowserWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  MagickStatusType
    *const mask = widgets->mask;

  switch (event.type)
  {
    case KeyPress:
    {
      PressFontBrowserKey(context,event,widgets);
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
    case MapNotify:
    {
      (*mask)&=(unsigned int) (~CWX);
      (*mask)&=(unsigned int) (~CWY);
      break;
    }
    case MotionNotify:
    {
      MoveFontBrowserButtons(context,event,widgets);
      break;
    }
    default:
    {
      HandleFontSelectionEvent(context,event,widgets);
      break;
    }
  }
}

static void HandleFontEvent(const FontBrowserContext *context,
  const XEvent event,const FontBrowserTargets *targets,
  const FontBrowserWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  switch (event.type)
  {
    case ButtonPress:
    {
      PressFontBrowserScroll(context,event,targets,widgets);
      break;
    }
    case ButtonRelease:
    {
      ReleaseFontBrowserScroll(context,event,widgets);
      break;
    }
    case ClientMessage:
    {
      ClientFontBrowserMessage(context,event,widgets);
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
      HandleFontKeyEvent(context,event,widgets);
      break;
    }
  }
}

static MagickBooleanType RespondToFontEvents(const FontBrowserContext *context,
  const FontBrowserTargets *targets,const FontBrowserWidgets *widgets)
{
  size_t
    *const state = widgets->state;

  XEvent
    event;

  do
  {
    UpdateFontBrowserLayout(context,widgets);
    RedrawFontBrowserWidget(context,widgets);
    if ((*state) & UpdateListState)
      {
        if (ReloadFontList(context,widgets) == MagickFalse)
          break;
        if (SortFontList(context,widgets) == MagickFalse)
          return(MagickFalse);
        UpdateFontBrowserSlider(context,widgets);
      }
    JumpFontBrowserList(context,widgets);
    RedrawFontBrowserList(context,widgets);
    RedrawFontBrowserAction(context,widgets);
    /*
      Wait for next event.
    */
    if (WaitFontEvent(context,&event,widgets) != MagickFalse)
      continue;
    HandleFontEvent(context,event,targets,widgets);
  } while (((*state) & ExitState) == 0);
  return(MagickTrue);
}

static void FreeFontBrowserList(char **listhead,char **fontlist)
{
  /*
    Free font list.
  */
  (void) XFreeFontNames(listhead);
  fontlist=(char **) RelinquishMagickMemory(fontlist);
}

static unsigned int GetFontBrowserWidth(XFontStruct *font_info,
  const char *action)
{
  unsigned int
    width;

  width=WidgetTextWidth(font_info,(char *) action);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  if (WidgetTextWidth(font_info,ResetButtonText) > width)
    width=WidgetTextWidth(font_info,ResetButtonText);
  if (WidgetTextWidth(font_info,BackButtonText) > width)
    width=WidgetTextWidth(font_info,BackButtonText);
  width+=(unsigned int) QuantumMargin;
  if (WidgetTextWidth(font_info,FontPatternText) > width)
    width=WidgetTextWidth(font_info,FontPatternText);
  if (WidgetTextWidth(font_info,FontnameText) > width)
    width=WidgetTextWidth(font_info,FontnameText);
  return(width);
}

static unsigned int WidestFontBrowserName(XFontStruct *font_info,
  char **fontlist,int fonts)
{
  int
    i;

  unsigned int
    text_width;

  text_width=0;
  for (i=0; i < fonts; i++)
    if (WidgetTextWidth(font_info,fontlist[i]) > text_width)
      text_width=WidgetTextWidth(font_info,fontlist[i]);
  return(text_width);
}

static void MapFontBrowserWidget(Display *display,XWindows *windows,
  MagickStatusType mask)
{
  Status
    status;

  XTextProperty
    window_name;

  XWindowChanges
    window_changes;

  /*
    Map Font Browser widget.
  */
  (void) CopyMagickString(windows->widget.name,"Browse and Select a Font",
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

typedef struct _FontBrowserRunTargets
{
  int
    *fonts,
    *i;

  XFontStruct
    **font_info;

  char
    ***fontlist,
    ***listhead;

  MagickStatusType
    *mask;
} FontBrowserRunTargets;

static MagickBooleanType RunFontBrowser(const FontBrowserContext *context,
  const FontBrowserRunTargets *run_targets)
{
  int
    *const fonts = run_targets->fonts,
    *const i = run_targets->i;

  XFontStruct
    **const font_info = run_targets->font_info;

  char
    ***const fontlist = run_targets->fontlist,
    ***const listhead = run_targets->listhead;

  MagickStatusType
    *const mask = run_targets->mask;

  unsigned int
    visible_fonts;

  size_t
    delay,
    state;

  XWidgetInfo
    action_info,
    back_info,
    cancel_info,
    expose_info,
    list_info,
    mode_info,
    north_info,
    reply_info,
    reset_info,
    scroll_info,
    selection_info,
    slider_info,
    south_info,
    text_info;

  const FontBrowserTargets
    targets = { &reply_info, &state, &selection_info, &list_info };

  const FontBrowserWidgets
    font_widgets = { &reply_info, &state, &back_info,
      &reset_info, &action_info, &cancel_info, &slider_info, fonts,
      &scroll_info, &visible_fonts, &north_info, &delay, &south_info, i,
      font_info, fontlist, &list_info, &mode_info, &expose_info, &text_info,
      &selection_info, listhead, mask };

  /*
    Respond to X events.
  */
  XGetWidgetInfo((char *) NULL,&slider_info);
  XGetWidgetInfo((char *) NULL,&north_info);
  XGetWidgetInfo((char *) NULL,&south_info);
  XGetWidgetInfo((char *) NULL,&expose_info);
  XGetWidgetInfo((char *) NULL,&selection_info);
  visible_fonts=0;
  delay=SuspendTime << 2;
  state=UpdateConfigurationState;
  if (RespondToFontEvents(context,&targets,&font_widgets) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType AcquireFontList(Display *display,XWindows *windows,
  char *glob_pattern,const FontBrowserRunTargets *run_targets)
{
  int
    *const fonts = run_targets->fonts,
    *const i = run_targets->i;

  char
    ***const fontlist = run_targets->fontlist,
    ***const listhead = run_targets->listhead;

  (*fontlist)=XListFonts(display,glob_pattern,32767,fonts);
  if ((*fonts) == 0)
    {
      /*
        Pattern failed, obtain all the fonts.
      */
      XNoticeWidget(display,windows,"Unable to obtain fonts names:",
        glob_pattern);
      (void) CopyMagickString(glob_pattern,"*",MagickPathExtent);
      (*fontlist)=XListFonts(display,glob_pattern,32767,fonts);
      if ((*fontlist) == (char **) NULL)
        {
          XNoticeWidget(display,windows,"Unable to obtain fonts names:",
            glob_pattern);
          return(MagickFalse);
        }
    }
  /*
    Sort font list in ascending order.
  */
  (*listhead)=(*fontlist);
  (*fontlist)=(char **) AcquireQuantumMemory((size_t) (*fonts),
    sizeof(*(*fontlist)));
  if ((*fontlist) == (char **) NULL)
    {
      XNoticeWidget(display,windows,"MemoryAllocationFailed",
        "UnableToViewFonts");
      return(MagickFalse);
    }
  for ((*i)=0; (*i) < (*fonts); (*i)++)
    (*fontlist)[(*i)]=(*listhead)[(*i)];
  qsort((void *) (*fontlist),(size_t) (*fonts),sizeof(*(*fontlist)),
    FontCompare);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X F o n t B r o w s e r W i d g e t                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XFontBrowserWidget() displays a Font Browser widget with a font query to the
%  user.  The user keys a reply and presses the Action or Cancel button to
%  exit.  The typed text is returned as the reply function parameter.
%
%  The format of the XFontBrowserWidget method is:
%
%      void XFontBrowserWidget(Display *display,XWindows *windows,
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
%
*/

MagickPrivate void XFontBrowserWidget(Display *display,XWindows *windows,
  const char *action,char *reply)
{
  char
    back_pattern[MagickPathExtent],
    **fontlist,
    **listhead,
    primary_selection[MagickPathExtent] = "",
    reset_pattern[MagickPathExtent],
    text[MagickPathExtent];

  int
    fonts;

  int
    i;

  static char
    glob_pattern[MagickPathExtent] = "*";

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  unsigned int
    height,
    text_width,
    width;

  XFontStruct
    *font_info;

  const FontBrowserRunTargets
    run_targets = { &fonts, &i, &font_info, &fontlist, &listhead, &mask };

  /*
    Get font list and sort in ascending order.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(action != (char *) NULL);
  assert(reply != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(back_pattern,glob_pattern,MagickPathExtent);
  (void) CopyMagickString(reset_pattern,"*",MagickPathExtent);
  if (AcquireFontList(display,windows,glob_pattern,&run_targets) == MagickFalse)
    return;
  /*
    Determine Font Browser widget attributes.
  */
  font_info=windows->widget.font_info;
  text_width=WidestFontBrowserName(font_info,fontlist,fonts);
  width=GetFontBrowserWidth(font_info,action);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position Font Browser widget.
  */
  windows->widget.width=width+MagickMin(text_width,MaxTextWidth)+(unsigned int)
    (6*QuantumMargin);
  windows->widget.min_width=width+MinTextWidth+(unsigned int) (4*QuantumMargin);
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int)
    (((85*(int) height) >> 2)+((13*QuantumMargin) >> 1)+4);
  windows->widget.min_height=(unsigned int)
    (((27*(int) height) >> 1)+((13*QuantumMargin) >> 1)+4);
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
  MapFontBrowserWidget(display,windows,mask);
  {
    const FontBrowserContext
      font_context = { display, windows,
        glob_pattern, back_pattern, reset_pattern, primary_selection, height,
        action, width, reply, text };

    if (RunFontBrowser(&font_context,&run_targets) == MagickFalse)
      return;
  }
  WithdrawBrowserWidget(display,windows);
  FreeFontBrowserList(listhead,fontlist);
}
#endif
