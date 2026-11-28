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

  MagickCore private methods of display-image.c: the tile command, from
  XLocateTile to XTileImage; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_TILE_COMMAND_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_TILE_COMMAND_PRIVATE_H

typedef struct _XTileLocationTargets
{
  unsigned int
    *width,
    *height;

  int
    *x,
    *y;

  char
    **p;
} XTileLocationTargets;

static MagickBooleanType XLocateTile(Display *display,Image *image,
  XEvent *event,const XTileLocationTargets *targets)
{
  int
    *const x = targets->x,
    *const y = targets->y;

  char
    **const p = targets->p;

  unsigned int
    *const width = targets->width,
    *const height = targets->height;

  int
    i,
    tile;

  /*
    Determine size and location of each tile in the visual image directory.
  */
  (*width)=(unsigned int) image->columns;
  (*height)=(unsigned int) image->rows;
  (*x)=0;
  (*y)=0;
  (void) XParseGeometry(image->montage,x,y,width,height);
  tile=((event->xbutton.y-(*y))/(int) (*height))*(((int) image->columns-(*x))/(int)
    (*width))+(event->xbutton.x-(*x))/(int) (*width);
  if (tile < 0)
    {
      /*
        Button press is outside any tile.
      */
      (void) XBell(display,0);
      return(MagickFalse);
    }
  /*
    Determine file name from the tile directory.
  */
  (*p)=image->directory;
  for (i=tile; (i != 0) && (*(*p) != '\0'); )
  {
    if (*(*p) == '\xff')
      i--;
    (*p)++;
  }
  if (*(*p) == '\0')
    {
      /*
        Button press is outside any tile.
      */
      (void) XBell(display,0);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void XTileDeleteImage(const XTileInput *input)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  char
    *const filename = input->filename;

  int
    status;

  /*
    Delete tile image.
  */
  if (IsPathAccessible(filename) == MagickFalse)
    {
      XNoticeWidget(display,windows,"Image file does not exist:",filename);
      return;
    }
  status=XConfirmWidget(display,windows,"Really delete tile",filename);
  if (status <= 0)
    return;
  status=ShredFile(filename) == MagickFalse ? 0 : 1;
  status|=remove_utf8(filename);
  if (status != MagickFalse)
    {
      XNoticeWidget(display,windows,"Unable to delete image file:",
        filename);
      return;
    }
  XTileUpdateImage(input);
}

static Image *XTileCommand(const XTileInput *input)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  const ModeType
    *const TileCommands = input->TileCommands;

  const int
    id = input->id;

  Image
    *tile_image;

  /*
    Perform command for the selected tile.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  tile_image=NewImageList();
  switch (TileCommands[id])
  {
    case TileLoadCommand:
    {
      tile_image=XTileLoadImage(input);
      break;
    }
    case TileNextCommand:
    {
      /*
        Display next image.
      */
      {
        const XClientReason
          reason = { windows->im_next_image, CurrentTime };

        XClientMessage(display,windows->image.id,windows->im_protocols,&reason);
      }
      break;
    }
    case TileFormerCommand:
    {
      /*
        Display former image.
      */
      {
        const XClientReason
          reason = { windows->im_former_image, CurrentTime };

        XClientMessage(display,windows->image.id,windows->im_protocols,&reason);
      }
      break;
    }
    case TileDeleteCommand:
    {
      XTileDeleteImage(input);
      break;
    }
    case TileUpdateCommand:
    {
      XTileUpdateImage(input);
      break;
    }
    default:
      break;
  }
  XSetCursorState(display,windows,MagickFalse);
  return(tile_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X T i l e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XTileImage() loads or deletes a selected tile from a visual image directory.
%  The load or delete command is chosen from a menu.
%
%  The format of the XTileImage method is:
%
%      Image *XTileImage(Display *display,XResourceInfo *resource_info,
%        XWindows *windows,Image *image,XEvent *event,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o tile_image:  XTileImage reads or deletes the tile image
%      and returns it.  A null image is returned if an error occurs.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image; returned from ReadImage.
%
%    o event: Specifies a pointer to a XEvent structure.  If it is NULL,
%      the entire image is refreshed.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *XTileImage(const XTileImageInput *tile_input)
{
  const char
    *const VerbMenu[] =
    {
      "Load",
      "Next",
      "Former",
      "Delete",
      "Update",
      (char *) NULL,
    };

  static const ModeType
    TileCommands[] =
    {
      TileLoadCommand,
      TileNextCommand,
      TileFormerCommand,
      TileDeleteCommand,
      TileUpdateCommand
    };

  char
    command[MagickPathExtent],
    filename[MagickPathExtent];

  int
    id,
    x,
    y;

  char
    *p,
    *q;

  unsigned int
    height,
    width;

  XTileInput
    input;

  const XTileLocationTargets
    location = { &width, &height, &x, &y, &p };

  XScaleTileEvent(tile_input->image,tile_input->windows,tile_input->event);
  if (XLocateTile(tile_input->display,tile_input->image,tile_input->event,
      &location) == MagickFalse)
    return((Image *) NULL);
  /*
    Select a command from the pop-up menu.
  */
  {
    const XMenuRequest
      menu_request = { "Tile Verb", VerbMenu, command };

    id=XMenuWidget(tile_input->display,tile_input->windows,&menu_request);
  }
  if (id < 0)
    return((Image *) NULL);
  q=p;
  while ((IsTileNameCharacter(q) != MagickFalse) &&
         ((size_t) (q-p+1) < sizeof(filename)))
    q++;
  (void) CopyMagickString(filename,p,(size_t) (q-p+1));
  input.display=tile_input->display;
  input.resource_info=tile_input->resource_info;
  input.windows=tile_input->windows;
  input.image=tile_input->image;
  input.exception=tile_input->exception;
  input.x=x;
  input.y=y;
  input.height=height;
  input.width=width;
  input.filename=filename;
  input.TileCommands=TileCommands;
  input.id=id;
  return(XTileCommand(&input));
}

#endif
