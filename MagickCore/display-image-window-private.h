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

  MagickCore private methods of display-image.c: the image reload, timer and
  command menu helpers of XDisplayImage and its constant menus, from
  XReloadModifiedImage to Menus; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_WINDOW_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_WINDOW_PRIVATE_H

static Image *XReloadModifiedImage(XResourceInfo *resource_info,
  const Image *display_image,size_t *state,ExceptionInfo *exception)
{
  Image
    *nexus;

  (void) FormatLocaleString(
    resource_info->image_info->filename,MagickPathExtent,
    "%s:%s",display_image->magick,
    display_image->filename);
  nexus=ReadImage(resource_info->image_info,exception);
  if (nexus != (Image *) NULL)
    *state|=NextImageState | ExitState;
  return(nexus);
}

#define MagnifySize  256  /* must be a power of 2 */
#define MagickMenus  10
#define MagickTitle  "Commands"

static void XHandleProtocolMessage(const DisplayEventContext *context,
  const XEvent event,XGCValues *context_values)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = context->state;

  const char
    *const *const CommandMenu = context->CommandMenu;

  if (*event.xclient.data.l == (long) windows->im_update_widget)
    {
      (void) CloneString(&windows->command.name,MagickTitle);
      windows->command.data=MagickMenus;
      (void) XCommandWidget(display,windows,CommandMenu,
        (XEvent *) NULL);
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_update_colormap)
    {
      XUpdateWindowColormaps(context,context_values);
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_former_image)
    {
      *state|=FormerImageState | ExitState;
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_next_image)
    {
      *state|=NextImageState | ExitState;
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_retain_colors)
    {
      *state|=RetainColorsState;
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_exit)
    {
      *state|=ExitState;
      return;
    }
}

static void XReloadModifiedDisplay(const DisplayEventContext *context,
  Image *display_image,Image **nexus,time_t *timer)
{
  XResourceInfo
    *const resource_info = context->resource_info;

  ExceptionInfo
    *const exception = context->exception;

  size_t
    *const state = context->state;

  const time_t
    update_time = context->update_time;

  MagickBooleanType
    status;

  struct stat
    attributes;

  size_t
    delay;

  /*
    Determine if image file was modified.
  */
  status=GetPathAttributes(display_image->filename,&attributes);
  if ((status != MagickFalse) &&
      (update_time != attributes.st_mtime))
    {
      /*
        Redisplay image.
      */
      (*nexus)=XReloadModifiedImage(resource_info,display_image,
        state,exception);
    }
  delay=display_image->delay/(size_t) MagickMax(
    display_image->ticks_per_second,1L);
  (*timer)=GetMagickTime()+(time_t) (delay == 0 ? 1 : delay)+1;
}

static void XUpdateImageTimer(const DisplayEventContext *context,
  Image *display_image,Image **nexus,time_t *timer)
{
  XResourceInfo
    *const resource_info = context->resource_info;

  size_t
    *const state = context->state;

  if ((*timer) < GetMagickTime())
    {
      if (resource_info->update == MagickFalse)
        *state|=NextImageState | ExitState;
      else
        {
          XReloadModifiedDisplay(context,display_image,nexus,timer);
        }
    }
}

static MagickBooleanType XCheckImageTimer(const DisplayEventContext *context,
  Image *display_image,Image **nexus,time_t *timer)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  if ((display_image->delay != 0) || (resource_info->update != 0))
    {
      XUpdateImageTimer(context,display_image,nexus,timer);
      if (XEventsQueued(display,QueuedAfterFlush) == 0)
        {
          /*
            Do not block if delay > 0.
          */
          XDelay(display,SuspendTime << 2);
          return(MagickTrue);
        }
    }
  return(MagickFalse);
}

static MagickBooleanType XSelectCommandMenuEntry(
  const DisplayEventContext *context,const int id,
  DisplayCommand *display_command)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const CommandMenu = context->CommandMenu;

  char
    *const command = context->command;

  const char *const
    **const Menus = context->Menus;

  DisplayCommand
    **const Commands = context->Commands;

  int
    entry;

  if (id < MagickMenus)
    {
      /*
        Select a command from a pop-up menu.
      */
      {
        const XMenuRequest
          menu_request = { CommandMenu[id], Menus[id], command };

        entry=XMenuWidget(display,windows,&menu_request);
      }
      if (entry < 0)
        return(MagickTrue);
      (void) CopyMagickString(command,Menus[id][entry],MagickPathExtent);
      (*display_command)=Commands[id][entry];
    }
  return(MagickFalse);
}

static const char
  *const CommandMenu[] =
  {
    "File",
    "Edit",
    "View",
    "Transform",
    "Enhance",
    "Effects",
    "F/X",
    "Image Edit",
    "Miscellany",
    "Help",
    (char *) NULL
  },
  *const FileMenu[] =
  {
    "Open...",
    "Next",
    "Former",
    "Select...",
    "Save...",
    "Print...",
    "Delete...",
    "New...",
    "Visual Directory...",
    "Quit",
    (char *) NULL
  },
  *const EditMenu[] =
  {
    "Undo",
    "Redo",
    "Cut",
    "Copy",
    "Paste",
    (char *) NULL
  },
  *const ViewMenu[] =
  {
    "Half Size",
    "Original Size",
    "Double Size",
    "Resize...",
    "Apply",
    "Refresh",
    "Restore",
    (char *) NULL
  },
  *const TransformMenu[] =
  {
    "Crop",
    "Chop",
    "Flop",
    "Flip",
    "Rotate Right",
    "Rotate Left",
    "Rotate...",
    "Shear...",
    "Roll...",
    "Trim Edges",
    (char *) NULL
  },
  *const EnhanceMenu[] =
  {
    "Hue...",
    "Saturation...",
    "Brightness...",
    "Gamma...",
    "Spiff",
    "Dull",
    "Contrast Stretch...",
    "Sigmoidal Contrast...",
    "Normalize",
    "Equalize",
    "Negate",
    "Grayscale",
    "Map...",
    "Quantize...",
    (char *) NULL
  },
  *const EffectsMenu[] =
  {
    "Despeckle",
    "Emboss",
    "Reduce Noise",
    "Add Noise...",
    "Sharpen...",
    "Blur...",
    "Threshold...",
    "Edge Detect...",
    "Spread...",
    "Shade...",
    "Raise...",
    "Segment...",
    (char *) NULL
  },
  *const FXMenu[] =
  {
    "Solarize...",
    "Sepia Tone...",
    "Swirl...",
    "Implode...",
    "Vignette...",
    "Wave...",
    "Oil Paint...",
    "Charcoal Draw...",
    (char *) NULL
  },
  *const ImageEditMenu[] =
  {
    "Annotate...",
    "Draw...",
    "Color...",
    "Matte...",
    "Composite...",
    "Add Border...",
    "Add Frame...",
    "Comment...",
    "Launch...",
    "Region of Interest...",
    (char *) NULL
  },
  *const MiscellanyMenu[] =
  {
    "Image Info",
    "Zoom Image",
    "Show Preview...",
    "Show Histogram",
    "Show Matte",
    "Background...",
    "Slide Show...",
    "Preferences...",
    (char *) NULL
  },
  *const HelpMenu[] =
  {
    "Overview",
    "Browse Documentation",
    "About Display",
    (char *) NULL
  },
  *const ShortCutsMenu[] =
  {
    "Next",
    "Former",
    "Open...",
    "Save...",
    "Print...",
    "Undo",
    "Restore",
    "Image Info",
    "Quit",
    (char *) NULL
  },
  *const VirtualMenu[] =
  {
    "Image Info",
    "Print",
    "Next",
    "Quit",
    (char *) NULL
  };

static const char
  *const *Menus[MagickMenus] =
  {
    FileMenu,
    EditMenu,
    ViewMenu,
    TransformMenu,
    EnhanceMenu,
    EffectsMenu,
    FXMenu,
    ImageEditMenu,
    MiscellanyMenu,
    HelpMenu
  };

#endif
