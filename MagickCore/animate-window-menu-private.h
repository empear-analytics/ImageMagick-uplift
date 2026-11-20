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

  MagickCore private methods of animate-window.c: the typedefs at the top of
  its X11 branch and the animate commands, from AnimateCommand to
  XMagickCommand; included by that file only, before
  animate-window-command-private.h.
*/
#ifndef MAGICKCORE_ANIMATE_WINDOW_MENU_PRIVATE_H
#define MAGICKCORE_ANIMATE_WINDOW_MENU_PRIVATE_H

/*
  Typedef declarations.
*/
typedef enum
{
  OpenCommand,
  SaveCommand,
  PlayCommand,
  StepCommand,
  RepeatCommand,
  AutoReverseCommand,
  SlowerCommand,
  FasterCommand,
  ForwardCommand,
  ReverseCommand,
  HelpCommand,
  BrowseDocumentationCommand,
  VersionCommand,
  InfoCommand,
  QuitCommand,
  StepBackwardCommand,
  StepForwardCommand,
  NullCommand
} AnimateCommand;

typedef struct _AnimateCommandTile
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    **image;

  MagickStatusType
    *state;

  ExceptionInfo
    *exception;
} AnimateCommandTile;

/*
  Define declarations.
*/
#define AutoReverseAnimationState 0x0004U
#define ForwardAnimationState 0x0008U
#define PlayAnimationState 0x0020U
#define RepeatAnimationState 0x0040U
#define StepAnimationState 0x0080U
#define HighlightWidth  8
#define HighlightHeight  8
#define ShadowWidth  8
#define ShadowHeight  8

/*
  Constant declarations.
*/
static const char
  AnimateHelp[] =
  {
    "BUTTONS\n"
    "\n"
    "  Press any button to map or unmap the Command widget.\n"
    "\n"
    "COMMAND WIDGET\n"
    "  The Command widget lists a number of sub-menus and commands.\n"
    "  They are\n"
    "\n"
    "    Animate\n"
    "      Open...\n"
    "      Save...\n"
    "      Play\n"
    "      Step\n"
    "      Repeat\n"
    "      Auto Reverse\n"
    "    Speed\n"
    "      Slower\n"
    "      Faster\n"
    "    Direction\n"
    "      Forward\n"
    "      Reverse\n"
    "      Help\n"
    "        Overview\n"
    "        Browse Documentation\n"
    "        About Animate\n"
    "    Image Info\n"
    "    Quit\n"
    "\n"
    "  Menu items with a indented triangle have a sub-menu.  They\n"
    "  are represented above as the indented items.  To access a\n"
    "  sub-menu item, move the pointer to the appropriate menu and\n"
    "  press a button and drag.  When you find the desired sub-menu\n"
    "  item, release the button and the command is executed.  Move\n"
    "  the pointer away from the sub-menu if you decide not to\n"
    "  execute a particular command.\n"
    "\n"
    "KEYBOARD ACCELERATORS\n"
    "  Accelerators are one or two key presses that effect a\n"
    "  particular command.  The keyboard accelerators that\n"
    "  animate(1) understands is:\n"
    "\n"
    "  Ctl+O  Press to open an image from a file.\n"
    "\n"
    "  space  Press to display the next image in the sequence.\n"
    "\n"
    "  <      Press to speed-up the display of the images.  Refer to\n"
    "         -delay for more information.\n"
    "\n"
    "  >      Press to slow the display of the images.  Refer to\n"
    "         -delay for more information.\n"
    "\n"
    "  F1     Press to display helpful information about animate(1).\n"
    "\n"
    "  Find   Press to browse documentation about ImageMagick.\n"
    "\n"
    "  ?      Press to display information about the image.  Press\n"
    "         any key or button to erase the information.\n"
    "\n"
    "         This information is printed: image name;  image size;\n"
    "         and the total number of unique colors in the image.\n"
    "\n"
    "  Ctl-q  Press to discard all images and exit program.\n"
  };

static const char
  *PageSizes[] =
  {
    "Letter",
    "Tabloid",
    "Ledger",
    "Legal",
    "Statement",
    "Executive",
    "A3",
    "A4",
    "A5",
    "B4",
    "B5",
    "Folio",
    "Quarto",
    "10x14",
    (char *) NULL
  };

static const unsigned char
  HighlightBitmap[8] =
  {
    (unsigned char) 0xaa,
    (unsigned char) 0x55,
    (unsigned char) 0xaa,
    (unsigned char) 0x55,
    (unsigned char) 0xaa,
    (unsigned char) 0x55,
    (unsigned char) 0xaa,
    (unsigned char) 0x55
  },
  ShadowBitmap[8] =
  {
    (unsigned char) 0x00,
    (unsigned char) 0x00,
    (unsigned char) 0x00,
    (unsigned char) 0x00,
    (unsigned char) 0x00,
    (unsigned char) 0x00,
    (unsigned char) 0x00,
    (unsigned char) 0x00
  };

/*
  Forward declarations.
*/
static Image
  *XMagickCommand(const AnimateCommandTile *,const AnimateCommand);
static MagickBooleanType
  XSaveImage(Display *,XResourceInfo *,XWindows *,Image *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X M a g i c k C o m m a n d                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMagickCommand() makes a transform to the image or Image window as specified
%  by a user menu button or keyboard command.
%
%  The format of the XMagickCommand method is:
%
%      Image *XMagickCommand(const AnimateCommandTile *tile,
%        const AnimateCommand animate_command)
%
%  A description of each parameter follows:
%
%    o tile: holds the display (a connection to an X server; returned from
%      XOpenDisplay), the resource_info (a pointer to a X11 XResourceInfo
%      structure), the windows (a pointer to a XWindows structure), the image
%      (XMagickCommand may transform the image and return a new image
%      pointer), the state (a MagickStatusType;  XMagickCommand may return a
%      modified state) and the exception (return any errors or warnings in
%      this structure).
%
%
*/
static void XConfirmAnimateExit(Display *display,
  XResourceInfo *resource_info,XWindows *windows)
{
  int
    status;

  /*
    Confirm program exit.
  */
  status=XConfirmWidget(display,windows,"Do you really want to exit",
    resource_info->client_name);
  if (status != 0)
    XClientMessage(display,windows->image.id,windows->im_protocols,
      windows->im_exit,CurrentTime);
}

static void XRemoteBrowseDocumentation(Display *display,XWindows *windows,
  Window mozilla_window)
{
  Atom
    mozilla_atom;

  char
    command[MagickPathExtent];

  /*
    Display documentation using Netscape remote control.
  */
  (void) FormatLocaleString(command,MagickPathExtent,
    "openurl(%s,new-tab)",MagickAuthoritativeURL);
  mozilla_atom=XInternAtom(display,"_MOZILLA_COMMAND",MagickFalse);
  (void) XChangeProperty(display,mozilla_window,mozilla_atom,
    XA_STRING,8,PropModeReplace,(unsigned char *) command,
    (int) strlen(command));
  XSetCursorState(display,windows,MagickFalse);
}

static void XVersionQuitCommand(const AnimateCommandTile *tile,
  const AnimateCommand animate_command)
{
  Display
    *const display = tile->display;

  XResourceInfo
    *const resource_info = tile->resource_info;

  XWindows
    *const windows = tile->windows;

  switch (animate_command)
  {
    case VersionCommand:
    {
      XNoticeWidget(display,windows,GetMagickVersion((size_t *) NULL),
        GetMagickCopyright());
      break;
    }
    case QuitCommand:
    {
      /*
        exit program
      */
      if (resource_info->confirm_exit == MagickFalse)
        XClientMessage(display,windows->image.id,windows->im_protocols,
          windows->im_exit,CurrentTime);
      else
        XConfirmAnimateExit(display,resource_info,windows);
      break;
    }
    default:
      break;
  }
}

static void XInfoHelpCommand(const AnimateCommandTile *tile,
  const AnimateCommand animate_command)
{
  Display
    *const display = tile->display;

  XResourceInfo
    *const resource_info = tile->resource_info;

  XWindows
    *const windows = tile->windows;

  Image
    **const image = tile->image;

  ExceptionInfo
    *const exception = tile->exception;

  MagickStatusType
    status;

  switch (animate_command)
  {
    case InfoCommand:
    {
      const XDisplayImageInfoTarget
        target = { display, resource_info, windows };

      XDisplayImageInfo(&target,(Image *) NULL,*image,exception);
      break;
    }
    case HelpCommand:
    {
      /*
        User requested help.
      */
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Animate",AnimateHelp);
      }
      break;
    }
    case BrowseDocumentationCommand:
    {
      Atom
        mozilla_atom;

      Window
        mozilla_window,
        root_window;

      /*
        Browse the ImageMagick documentation.
      */
      root_window=XRootWindow(display,XDefaultScreen(display));
      mozilla_atom=XInternAtom(display,"_MOZILLA_VERSION",MagickFalse);
      mozilla_window=XWindowByProperty(display,root_window,mozilla_atom);
      if (mozilla_window != (Window) NULL)
        {
          XRemoteBrowseDocumentation(display,windows,mozilla_window);
          break;
        }
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      status=InvokeDelegate(resource_info->image_info,*image,"browse",
        (char *) NULL,exception);
      if (status == MagickFalse)
        XNoticeWidget(display,windows,"Unable to browse documentation",
          (char *) NULL);
      XDelay(display,1500);
      XSetCursorState(display,windows,MagickFalse);
      break;
    }
    default:
    {
      XVersionQuitCommand(tile,animate_command);
      break;
    }
  }
}

static void XSpeedDirectionCommand(const AnimateCommandTile *tile,
  const AnimateCommand animate_command)
{
  XResourceInfo
    *const resource_info = tile->resource_info;

  MagickStatusType
    *const state = tile->state;

  switch (animate_command)
  {
    case SlowerCommand:
    {
      resource_info->delay++;
      break;
    }
    case FasterCommand:
    {
      if (resource_info->delay == 0)
        break;
      resource_info->delay--;
      break;
    }
    case ForwardCommand:
    {
      *state=ForwardAnimationState;
      *state&=(~AutoReverseAnimationState);
      break;
    }
    case ReverseCommand:
    {
      *state&=(~ForwardAnimationState);
      *state&=(~AutoReverseAnimationState);
      break;
    }
    default:
    {
      XInfoHelpCommand(tile,animate_command);
      break;
    }
  }
}

static void XRepeatSaveCommand(const AnimateCommandTile *tile,
  const AnimateCommand animate_command)
{
  Display
    *const display = tile->display;

  XResourceInfo
    *const resource_info = tile->resource_info;

  XWindows
    *const windows = tile->windows;

  Image
    **const image = tile->image;

  MagickStatusType
    *const state = tile->state;

  ExceptionInfo
    *const exception = tile->exception;

  MagickStatusType
    status;

  switch (animate_command)
  {
    case RepeatCommand:
    {
      *state|=RepeatAnimationState;
      *state&=(~AutoReverseAnimationState);
      *state|=PlayAnimationState;
      break;
    }
    case AutoReverseCommand:
    {
      *state|=AutoReverseAnimationState;
      *state&=(~RepeatAnimationState);
      *state|=PlayAnimationState;
      break;
    }
    case SaveCommand:
    {
      /*
        Save image.
      */
      status=XSaveImage(display,resource_info,windows,*image,exception);
      if (status == MagickFalse)
        {
          char
            message[MagickPathExtent];

          (void) FormatLocaleString(message,MagickPathExtent,"%s:%s",
            exception->reason != (char *) NULL ? exception->reason : "",
            exception->description != (char *) NULL ? exception->description :
            "");
          XNoticeWidget(display,windows,"Unable to save file:",message);
          break;
        }
      break;
    }
    default:
    {
      XSpeedDirectionCommand(tile,animate_command);
      break;
    }
  }
}

static void XUpdateStepState(MagickStatusType *state,
  const AnimateCommand animate_command)
{
  *state|=StepAnimationState;
  *state&=(~PlayAnimationState);
  if (animate_command == StepBackwardCommand)
    *state&=(~ForwardAnimationState);
  if (animate_command == StepForwardCommand)
    *state|=ForwardAnimationState;
}

static void XStepCommand(const AnimateCommandTile *tile,
  const AnimateCommand animate_command)
{
  MagickStatusType
    *const state = tile->state;

  switch (animate_command)
  {
    case StepCommand:
    case StepBackwardCommand:
    case StepForwardCommand:
    {
      XUpdateStepState(state,animate_command);
      break;
    }
    default:
    {
      XRepeatSaveCommand(tile,animate_command);
      break;
    }
  }
}

static void XApplyAnimateTitle(XResourceInfo *resource_info,
  XWindows *windows,Image **image,ExceptionInfo *exception)
{
  if (resource_info->title != (char *) NULL)
    {
      char
        *title;

      title=InterpretImageProperties(resource_info->image_info,*image,
        resource_info->title,exception);
      (void) CloneString(&windows->image.name,title);
      title=DestroyString(title);
    }
}

static void XPlayCommand(const AnimateCommandTile *tile,
  const AnimateCommand animate_command)
{
  Display
    *const display = tile->display;

  XResourceInfo
    *const resource_info = tile->resource_info;

  XWindows
    *const windows = tile->windows;

  Image
    **const image = tile->image;

  MagickStatusType
    *const state = tile->state;

  ExceptionInfo
    *const exception = tile->exception;

  XTextProperty
    window_name;

  switch (animate_command)
  {
    case PlayCommand:
    {
      char
        basename[MagickPathExtent],
        name[MagickPathExtent];

      int
        status;

      /*
        Window name is the base of the filename.
      */
      *state|=PlayAnimationState;
      *state&=(~AutoReverseAnimationState);
      GetPathComponent((*image)->magick_filename,BasePath,basename);
      (void) FormatLocaleString(name,MagickPathExtent,"%s: %s",
        MagickPackageName,basename);
      (void) CloneString(&windows->image.name,name);
      XApplyAnimateTitle(resource_info,windows,image,exception);
      status=XStringListToTextProperty(&windows->image.name,1,&window_name);
      if (status == 0)
        break;
      XSetWMName(display,windows->image.id,&window_name);
      (void) XFree((void *) window_name.value);
      break;
    }
    default:
    {
      XStepCommand(tile,animate_command);
      break;
    }
  }
}

static Image *XReadImageFileList(const AnimateCommandTile *tile,
  char **filelist,const int number_files)
{
  Display
    *const display = tile->display;

  XResourceInfo
    *const resource_info = tile->resource_info;

  XWindows
    *const windows = tile->windows;

  ExceptionInfo
    *const exception = tile->exception;

  Image
    *images,
    *next;

  ImageInfo
    *read_info;

  int
    i;

  MagickBooleanType
    proceed;

  read_info=CloneImageInfo(resource_info->image_info);
  images=NewImageList();
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  for (i=0; i < number_files; i++)
  {
    (void) CopyMagickString(read_info->filename,filelist[i],MagickPathExtent);
    filelist[i]=DestroyString(filelist[i]);
    *read_info->magick='\0';
    next=ReadImage(read_info,exception);
    CatchException(exception);
    if (next != (Image *) NULL)
      AppendImageToList(&images,next);
    if (number_files <= 5)
      continue;
    proceed=SetImageProgress(images,LoadImageTag,i,(MagickSizeType)
      number_files);
    if (proceed == MagickFalse)
      break;
  }
  filelist=(char **) RelinquishMagickMemory(filelist);
  read_info=DestroyImageInfo(read_info);
  return(images);
}

static void XDestroyFileList(char **filelist,const int number_files)
{
  int
    i;

  for (i=0; i < number_files; i++)
    filelist[i]=DestroyString(filelist[i]);
  filelist=(char **) RelinquishMagickMemory(filelist);
}

static void XDestroyFailedFileList(const MagickStatusType status,
  char **filelist,const int number_files)
{
  if ((status == MagickFalse) || (number_files == 0))
    {
      XDestroyFileList(filelist,number_files);
    }
}

static MagickBooleanType XOpenAnimateFiles(const AnimateCommandTile *tile,
  Image **nexus)
{
  Display
    *const display = tile->display;

  XResourceInfo
    *const resource_info = tile->resource_info;

  XWindows
    *const windows = tile->windows;

  MagickStatusType
    *const state = tile->state;

  char
    **filelist;

  Image
    *images;

  int
    number_files;

  MagickStatusType
    status;

  static char
    filenames[MagickPathExtent] = "*";

  if (resource_info->immutable != MagickFalse)
    return(MagickTrue);
  /*
    Request file name from user.
  */
  XFileBrowserWidget(display,windows,"Animate",filenames);
  if (*filenames == '\0')
    return(MagickFalse);
  /*
    Expand the filenames.
  */
  filelist=(char **) AcquireMagickMemory(sizeof(char *));
  if (filelist == (char **) NULL)
    {
      ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
        filenames);
      return(MagickFalse);
    }
  number_files=1;
  filelist[0]=filenames;
  status=ExpandFilenames(&number_files,&filelist);
  XDestroyFailedFileList(status,filelist,number_files);
  if (number_files == 0)
    {
      ThrowXWindowException(ImageError,"NoImagesWereLoaded",filenames);
      return(MagickFalse);
    }
  if (status == MagickFalse)
    {
      ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
        filenames);
      return(MagickFalse);
    }
  images=XReadImageFileList(tile,filelist,number_files);
  if (images == (Image *) NULL)
    {
      XSetCursorState(display,windows,MagickFalse);
      ThrowXWindowException(ImageError,"NoImagesWereLoaded",filenames);
      return(MagickFalse);
    }
  (*nexus)=GetFirstImageInList(images);
  *state|=ExitState;
  return(MagickTrue);
}

static Image *XMagickCommand(const AnimateCommandTile *tile,
  const AnimateCommand animate_command)
{
  Image
    *nexus;

  /*
    Process user command.
  */
  nexus=NewImageList();
  switch (animate_command)
  {
    case OpenCommand:
    {
      if (XOpenAnimateFiles(tile,&nexus) == MagickFalse)
        return((Image *) NULL);
      break;
    }
    default:
    {
      XPlayCommand(tile,animate_command);
      break;
    }
  }
  return(nexus);
}

#endif
