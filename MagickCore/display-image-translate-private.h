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

  MagickCore private methods of display-image.c: the translate command, from
  XClampImageOffset to XTranslateImage; included by
  display-image-tile-private.h only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_TRANSLATE_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_TRANSLATE_PRIVATE_H

static void XClampImageOffset(XWindows *windows)
{
  if (windows->image.x < 0)
    windows->image.x=0;
  else
    if ((windows->image.x+(int) windows->image.width) > windows->image.ximage->width)
      windows->image.x=windows->image.ximage->width-(int) windows->image.width;
  if (windows->image.y < 0)
    windows->image.y=0;
  else
    if ((windows->image.y+(int) windows->image.height) > windows->image.ximage->height)
      windows->image.y=windows->image.ximage->height-(int)
        windows->image.height;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X T r a n s l a t e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XTranslateImage() translates the image within an Image window by one pixel
%  as specified by the key symbol.  If the image has a montage string the
%  translation is respect to the width and height contained within the string.
%
%  The format of the XTranslateImage method is:
%
%      void XTranslateImage(Display *display,XWindows *windows,
%        Image *image,const KeySym key_symbol)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o key_symbol: Specifies a KeySym which indicates which side of the image
%      to trim.
%
*/
static MagickBooleanType XOffsetImageWindowDown(XWindows *windows,
  const KeySym key_symbol,const unsigned int y_offset)
{
  switch ((int) key_symbol)
  {
    case XK_Prior:
    case XK_Down:
    case XK_KP_Down:
    {
      windows->image.y+=(int) y_offset;
      break;
    }
    default:
      return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType XOffsetImageWindowUpRight(XWindows *windows,
  const KeySym key_symbol,const unsigned int x_offset,
  const unsigned int y_offset)
{
  switch ((int) key_symbol)
  {
    case XK_Next:
    case XK_Up:
    case XK_KP_Up:
    {
      windows->image.y-=(int) y_offset;
      break;
    }
    case XK_Right:
    case XK_KP_Right:
    {
      windows->image.x+=(int) x_offset;
      break;
    }
    default:
    {
      return(XOffsetImageWindowDown(windows,key_symbol,y_offset));
    }
  }
  return(MagickTrue);
}

static MagickBooleanType XOffsetImageWindow(XWindows *windows,
  const KeySym key_symbol,const unsigned int x_offset,
  const unsigned int y_offset)
{
  switch ((int) key_symbol)
  {
    case XK_Home:
    case XK_KP_Home:
    {
      windows->image.x=(int) windows->image.width/2;
      windows->image.y=(int) windows->image.height/2;
      break;
    }
    case XK_Left:
    case XK_KP_Left:
    {
      windows->image.x-=(int) x_offset;
      break;
    }
    default:
    {
      return(XOffsetImageWindowUpRight(windows,key_symbol,x_offset,y_offset));
    }
  }
  return(MagickTrue);
}

static void XTranslateImage(Display *display,XWindows *windows,
  Image *image,const KeySym key_symbol)
{
  char
    text[MagickPathExtent];

  int
    x,
    y;

  unsigned int
    x_offset,
    y_offset;

  /*
    User specified a pan position offset.
  */
  x_offset=windows->image.width;
  y_offset=windows->image.height;
  if (image->montage != (char *) NULL)
    (void) XParseGeometry(image->montage,&x,&y,&x_offset,&y_offset);
  if (XOffsetImageWindow(windows,key_symbol,x_offset,y_offset) == MagickFalse)
    return;
  /*
    Check boundary conditions.
  */
  XClampImageOffset(windows);
  /*
    Refresh Image window.
  */
  (void) FormatLocaleString(text,MagickPathExtent," %ux%u%+d%+d ",
    windows->image.width,windows->image.height,windows->image.x,
    windows->image.y);
  XInfoWidget(display,windows,text);
  XCheckRefreshWindows(display,windows);
  XDrawPanRectangle(display,windows);
  XRefreshWindow(display,&windows->image,(XEvent *) NULL);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
}

#endif
