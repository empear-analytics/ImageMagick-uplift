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
%                       CCCC   OOO   L       OOO   RRRR                       %
%                      C      O   O  L      O   O  R   R                      %
%                      C      O   O  L      O   O  RRRR                       %
%                      C      O   O  L      O   O  R R                        %
%                       CCCC   OOO   LLLLL   OOO   R  R                       %
%                                                                             %
%                                                                             %
%                 MagickCore X11 Color Browser Widget Methods                 %
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
#include "MagickCore/widget-color-actions-private.h"
#if defined(MAGICKCORE_X11_DELEGATE)

static void InitializeColorBrowserList(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
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

static void InitializeColorBrowserText(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  char
    *const text = context->text;

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

static void UpdateColorBrowserLayout(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  size_t
    *const state = widgets->state;

  if ((*state) & UpdateConfigurationState)
    {
      InitializeColorBrowserButtons(context,widgets);
      InitializeColorBrowserReply(context,widgets);
      InitializeColorBrowserScroll(context,widgets);
      InitializeColorBrowserList(context,widgets);
      InitializeColorBrowserText(context,widgets);
      (*state)&=(unsigned int) (~UpdateConfigurationState);
    }
}

static void RedrawColorBrowserWidget(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const grab_info = widgets->grab_info,
    *const reset_info = widgets->reset_info,
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

  if ((*state) & RedrawWidgetState)
    {
      /*
        Redraw Color Browser window.
      */
      x=QuantumMargin;
      y=(*text_info).y+(int) (((*text_info).height-height) >> 1)+
        font_info->ascent;
      (void) XDrawString(display,windows->widget.id,
        windows->widget.annotate_context,x,y,ColorPatternText,
        Extent(ColorPatternText));
      (void) CopyMagickString((*text_info).text,glob_pattern,MagickPathExtent);
      XDrawWidgetText(display,&windows->widget,text_info);
      XDrawBeveledButton(display,&windows->widget,grab_info);
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
        windows->widget.annotate_context,x,y,ColornameText,
        Extent(ColornameText));
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

static void JumpColorBrowserList(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  char
    *const reply = context->reply;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  unsigned int
    *const visible_colors = widgets->visible_colors;

  char
    ***const colorlist = widgets->colorlist;

  XWidgetInfo
    *const list_info = widgets->list_info,
    *const selection_info = widgets->selection_info;

  int
    i;

  if ((*state) & JumpListState)
    {
      FontBrowserMatch
        match;

      /*
        Jump scroll to match user color.
      */
      (*list_info).id=(~0);
      match.fontlist=(*colorlist);
      match.fonts=(int) (*colors);
      match.reply=reply;
      MatchFontBrowserReply(&match,list_info,&i);
      if ((i < (*slider_info).id) ||
          (i >= (int) ((*slider_info).id+(int) (*visible_colors))))
        (*slider_info).id=i-(int) ((*visible_colors) >> 1);
      (*selection_info).id=(~0);
      (*state)|=RedrawListState;
      (*state)&=(unsigned int) (~JumpListState);
    }
}

static void RedrawColorBrowserList(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
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

  size_t
    *const colors = widgets->colors;

  unsigned int
    *const visible_colors = widgets->visible_colors;

  XWidgetInfo
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info;

  char
    ***const colorlist = widgets->colorlist;

  XWidgetInfo
    *const list_info = widgets->list_info,
    *const expose_info = widgets->expose_info,
    *const selection_info = widgets->selection_info;

  int
    i;

  if ((*state) & RedrawListState)
    {
      /*
        Determine slider id and position.
      */
      if ((*slider_info).id >= (int) ((*colors)-(*visible_colors)))
        (*slider_info).id=(int) ((*colors)-(*visible_colors));
      if (((*slider_info).id < 0) || ((*colors) <= (*visible_colors)))
        (*slider_info).id=0;
      (*slider_info).y=(*slider_info).min_y;
      if ((*colors) != 0)
        (*slider_info).y+=(int) (*slider_info).id*((*slider_info).max_y-
          (*slider_info).min_y+1)/(int) (*colors);
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
          names.fontlist=(*colorlist);
          names.fonts=(int) (*colors);
          names.visible_fonts=(*visible_colors);
          names.list_info=(*list_info);
          names.slider_info=(*slider_info);
          DrawFontBrowserNames(&names,selection_info,&i);
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

static void RedrawColorBrowserAction(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const mode_info = widgets->mode_info;

  XColor
    color;

  if ((*state) & RedrawActionState)
    {
      static char
        colorname[MagickPathExtent];

      /*
        Display the selected color in a drawing area.
      */
      color=windows->widget.pixel_info->matte_color;
      (void) XParseColor(display,windows->widget.map_info->colormap,
        (*reply_info).text,&windows->widget.pixel_info->matte_color);
      XBestPixel(display,windows->widget.map_info->colormap,(XColor *) NULL,
        (unsigned int) windows->widget.visual_info->colormap_size,
        &windows->widget.pixel_info->matte_color);
      (*mode_info).text=colorname;
      (void) FormatLocaleString((*mode_info).text,MagickPathExtent,
        "#%02x%02x%02x",windows->widget.pixel_info->matte_color.red,
        windows->widget.pixel_info->matte_color.green,
        windows->widget.pixel_info->matte_color.blue);
      XDrawBeveledButton(display,&windows->widget,mode_info);
      windows->widget.pixel_info->matte_color=color;
      (*state)&=(unsigned int) (~RedrawActionState);
    }
}

static MagickBooleanType ReloadColorList(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern,
    *const reset_pattern = context->reset_pattern;

  ExceptionInfo
    *const exception = context->exception;

  char
    *const reply = context->reply;

  XWidgetInfo
    *const action_info = widgets->action_info;

  size_t
    *const colors = widgets->colors;

  char
    ***const colorlist = widgets->colorlist;

  const ColorBrowserUpdateTargets
    update_targets = { action_info, colorlist, colors };

  ColorBrowserUpdate
    update;

  update.display=display;
  update.windows=windows;
  update.reply=reply;
  update.glob_pattern=glob_pattern;
  update.reset_pattern=reset_pattern;
  update.exception=exception;
  if (UpdateColorBrowserList(&update,&update_targets) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static void UpdateColorBrowserSlider(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
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

  size_t
    *const colors = widgets->colors;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_colors = widgets->visible_colors;

  XWidgetInfo
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info,
    *const list_info = widgets->list_info,
    *const expose_info = widgets->expose_info,
    *const text_info = widgets->text_info,
    *const selection_info = widgets->selection_info;

  /*
    Sort color list in ascending order.
  */
  (*slider_info).height=(unsigned int) ((int) (*scroll_info).height-
    (((*slider_info).min_y-(*scroll_info).y+1) << 1)+1);
  if ((*colors) > (*visible_colors))
    (*slider_info).height=(unsigned int) (((*visible_colors)*
      (*slider_info).height)/(*colors));
  (*slider_info).max_y=(*south_info).y-(int) (*south_info).bevel_width-
    (int) (*slider_info).bevel_width-2;
  (*slider_info).id=0;
  (*slider_info).y=(*slider_info).min_y;
  (*expose_info).y=(*slider_info).y;
  (*selection_info).id=(~0);
  (*list_info).id=(~0);
  (*state)|=RedrawListState;
  /*
    Redraw color name & reply.
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

static MagickBooleanType WaitColorEvent(const ColorBrowserContext *context,
  XEvent *event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

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
          ((*slider_info).id < (int) (*colors)))
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

static void HandleColorSelectionEvent(const ColorBrowserContext *context,
  XEvent event,const ColorBrowserTargets *widgets)
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
      SelectColorBrowserReply(context,event,widgets);
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
      notify.send_event=MagickTrue;
      notify.display=request->display;
      notify.requestor=request->requestor;
      notify.selection=request->selection;
      notify.target=request->target;
      notify.time=request->time;
      if (request->property == None)
        notify.property=request->target;
      else
        notify.property=request->property;
      (void) XSendEvent(request->display,request->requestor,False,
        NoEventMask,(XEvent *) &notify);
    }
    default:
      break;
  }
}

static void HandleColorKeyEvent(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
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
      PressColorBrowserKey(context,event,widgets);
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
      MoveColorBrowserButtons(context,event,widgets);
      break;
    }
    default:
    {
      HandleColorSelectionEvent(context,event,widgets);
      break;
    }
  }
}

static void HandleColorEvent(const ColorBrowserContext *context,
  const XEvent event,const FontBrowserTargets *targets,
  const ColorBrowserTargets *widgets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  switch (event.type)
  {
    case ButtonPress:
    {
      PressColorBrowserScroll(context,event,targets,widgets);
      break;
    }
    case ButtonRelease:
    {
      ReleaseColorBrowserScroll(context,event,widgets);
      break;
    }
    case ClientMessage:
    {
      ClientColorBrowserMessage(context,event,widgets);
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
      HandleColorKeyEvent(context,event,widgets);
      break;
    }
  }
}

static void RespondToColorBrowserEvents(const ColorBrowserContext *context,
  const FontBrowserTargets *targets,const ColorBrowserTargets *widgets)
{
  size_t
    *const state = widgets->state;

  XEvent
    event;

  do
  {
    UpdateColorBrowserLayout(context,widgets);
    RedrawColorBrowserWidget(context,widgets);
    if ((*state) & UpdateListState)
      {
        if (ReloadColorList(context,widgets) == MagickFalse)
          break;
        UpdateColorBrowserSlider(context,widgets);
      }
    JumpColorBrowserList(context,widgets);
    RedrawColorBrowserList(context,widgets);
    RedrawColorBrowserAction(context,widgets);
    /*
      Wait for next event.
    */
    if (WaitColorEvent(context,&event,widgets) != MagickFalse)
      continue;
    HandleColorEvent(context,event,targets,widgets);
  } while (((*state) & ExitState) == 0);
}

static void CheckColorBrowserReply(Display *display,XWindows *windows,
  char *reply)
{
  Status
    status;

  XColor
    color;

  if ((*reply == '\0') || (strchr(reply,'-') != (char *) NULL))
    return;
  status=XParseColor(display,windows->widget.map_info->colormap,reply,&color);
  if (status != False)
    return;
  XNoticeWidget(display,windows,"Color is unknown to X server:",reply);
  (void) CopyMagickString(reply,"gray",MagickPathExtent);
}

static void FreeColorBrowserList(char **colorlist,size_t colors,
  ExceptionInfo *exception)
{
  int
    i;

  /*
    Free color list.
  */
  for (i=0; i < (int) colors; i++)
    colorlist[i]=DestroyString(colorlist[i]);
  if (colorlist != (char **) NULL)
    colorlist=(char **) RelinquishMagickMemory(colorlist);
  exception=DestroyExceptionInfo(exception);
}

static void MapColorBrowserWidget(Display *display,XWindows *windows,
  MagickStatusType mask)
{
  Status
    status;

  XTextProperty
    window_name;

  XWindowChanges
    window_changes;

  (void) CopyMagickString(windows->widget.name,"Browse and Select a Color",
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
  (void) XReconfigureWMWindow(display,windows->widget.id,windows->widget.screen,
    mask,&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
}

static unsigned int WidestColorBrowserName(XFontStruct *font_info,
  char **colorlist,size_t colors)
{
  int
    i;

  unsigned int
    text_width;

  text_width=0;
  for (i=0; i < (int) colors; i++)
    if (WidgetTextWidth(font_info,colorlist[i]) > text_width)
      text_width=WidgetTextWidth(font_info,colorlist[i]);
  return(text_width);
}

static void RunColorBrowser(const ColorBrowserContext *context,
  char ***colorlist,size_t *colors,MagickStatusType *mask)
{
  unsigned int
    visible_colors;

  size_t
    delay,
    state;

  XWidgetInfo
    action_info,
    cancel_info,
    expose_info,
    grab_info,
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

  const ColorBrowserTargets
    color_widgets = { &reply_info, &state,
      &grab_info, &reset_info, &action_info, &cancel_info, &slider_info,
      colors, &scroll_info, &visible_colors, &north_info, &delay, &south_info,
      &mode_info, colorlist, &list_info, &expose_info, &text_info,
      &selection_info, mask };

  /*
    Respond to X events.
  */
  XGetWidgetInfo((char *) NULL,&mode_info);
  XGetWidgetInfo((char *) NULL,&slider_info);
  XGetWidgetInfo((char *) NULL,&north_info);
  XGetWidgetInfo((char *) NULL,&south_info);
  XGetWidgetInfo((char *) NULL,&expose_info);
  XGetWidgetInfo((char *) NULL,&selection_info);
  visible_colors=0;
  delay=SuspendTime << 2;
  state=UpdateConfigurationState;
  RespondToColorBrowserEvents(context,&targets,&color_widgets);
}

typedef struct _ColorBrowserLayout
{
  Display
    *display;

  XFontStruct
    *font_info;

  unsigned int
    width,
    height,
    text_width;
} ColorBrowserLayout;

static void PositionColorBrowserWidget(XWindows *windows,
  const ColorBrowserLayout *layout)
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
    Position Color Browser widget.
  */
  windows->widget.width=width+MagickMin(text_width,MaxTextWidth)+
    6*(unsigned int) QuantumMargin;
  windows->widget.min_width=width+MinTextWidth+4*(unsigned int) QuantumMargin;
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int)
    ((81*height) >> 2)+((13*(unsigned int) QuantumMargin) >> 1)+4;
  windows->widget.min_height=(unsigned int)
    (((23*height) >> 1)+((13*(unsigned int) QuantumMargin) >> 1)+4);
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
}

static unsigned int GetColorBrowserWidth(XFontStruct *font_info,
  const char *action)
{
  unsigned int
    width;

  width=WidgetTextWidth(font_info,(char *) action);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  if (WidgetTextWidth(font_info,ResetButtonText) > width)
    width=WidgetTextWidth(font_info,ResetButtonText);
  if (WidgetTextWidth(font_info,GrabButtonText) > width)
    width=WidgetTextWidth(font_info,GrabButtonText);
  width+=(unsigned int) QuantumMargin;
  if (WidgetTextWidth(font_info,ColorPatternText) > width)
    width=WidgetTextWidth(font_info,ColorPatternText);
  if (WidgetTextWidth(font_info,ColornameText) > width)
    width=WidgetTextWidth(font_info,ColornameText);
  return(width);
}

MagickPrivate void XColorBrowserWidget(Display *display,XWindows *windows,
  const char *action,char *reply)
{
  char
    **colorlist,
    primary_selection[MagickPathExtent],
    reset_pattern[MagickPathExtent],
    text[MagickPathExtent];

  ExceptionInfo
    *exception;

  static char
    glob_pattern[MagickPathExtent] = "*";

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  unsigned int
    height,
    text_width,
    width;

  size_t
    colors;

  XFontStruct
    *font_info;

  /*
    Get color list and sort in ascending order.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(action != (char *) NULL);
  assert(reply != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(reset_pattern,"*",MagickPathExtent);
  exception=AcquireExceptionInfo();
  colorlist=GetColorList(glob_pattern,&colors,exception);
  if (colorlist == (char **) NULL)
    {
      /*
        Pattern failed, obtain all the colors.
      */
      (void) CopyMagickString(glob_pattern,"*",MagickPathExtent);
      colorlist=GetColorList(glob_pattern,&colors,exception);
      if (colorlist == (char **) NULL)
        {
          const XDialogRequest
            dialog_request = { action, "Enter color name:", reply };

          XNoticeWidget(display,windows,"Unable to obtain colors names:",
            glob_pattern);
          (void) XDialogWidget(display,windows,&dialog_request);
          return;
        }
    }
  /*
    Determine Color Browser widget attributes.
  */
  font_info=windows->widget.font_info;
  text_width=WidestColorBrowserName(font_info,colorlist,colors);
  width=GetColorBrowserWidth(font_info,action);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  {
    const ColorBrowserLayout
      color_layout = { display, font_info, width,
        height, text_width };

    PositionColorBrowserWidget(windows,&color_layout);
  }
  /*
    Map Color Browser widget.
  */
  MapColorBrowserWidget(display,windows,mask);
  {
    const ColorBrowserContext
      color_context = { display, windows,
        glob_pattern, reset_pattern, exception, font_info, primary_selection,
        height, action, width, reply, text };

    RunColorBrowser(&color_context,&colorlist,&colors,&mask);
  }
  WithdrawBrowserWidget(display,windows);
  FreeColorBrowserList(colorlist,colors,exception);
  CheckColorBrowserReply(display,windows,reply);
}
#endif
