/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                AAA   N   N  IIIII  M   M   AAA   TTTTT  EEEEE               %
%               A   A  NN  N    I    MM MM  A   A    T    E                   %
%               AAAAA  N N N    I    M M M  AAAAA    T    EEE                 %
%               A   A  N  NN    I    M   M  A   A    T    E                   %
%               A   A  N   N  IIIII  M   M  A   A    T    EEEEE               %
%                                                                             %
%                                                                             %
%              Methods to Interactively Animate an Image Sequence             %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                                July 1992                                    %
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
#include "MagickCore/animate.h"
#include "MagickCore/animate-private.h"
#include "MagickCore/attribute.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/layer.h"
#include "MagickCore/list.h"
#include "MagickCore/locale-private.h"
#include "MagickCore/log.h"
#include "MagickCore/image.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/widget.h"
#include "MagickCore/widget-private.h"
#include "MagickCore/xwindow.h"
#include "MagickCore/xwindow-private.h"

#if defined(MAGICKCORE_X11_DELEGATE)
/*
  Animate state declarations.
*/
#define AutoReverseAnimationState 0x0004U
#define ForwardAnimationState 0x0008U
#define HighlightState  0x0010U
#define PlayAnimationState 0x0020U
#define RepeatAnimationState 0x0040U
#define StepAnimationState 0x0080U

/*
  Static declarations.
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

/*
  Constant declarations.
*/
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
  Enumeration declarations.
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

/*
  Stipples.
*/
#define HighlightWidth  8
#define HighlightHeight  8
#define ShadowWidth  8
#define ShadowHeight  8

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
%   A n i m a t e I m a g e s                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AnimateImages() repeatedly displays an image sequence to any X window
%  screen.  It returns a value other than 0 if successful.  Check the
%  exception member of image to determine the reason for any failure.
%
%  The format of the AnimateImages method is:
%
%      MagickBooleanType AnimateImages(const ImageInfo *image_info,
%        Image *images,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType AnimateImages(const ImageInfo *image_info,
  Image *images,ExceptionInfo *exception)
{
  char
    *argv[1];

  Display
    *display;

  MagickStatusType
    status;

  XrmDatabase
    resource_database;

  XResourceInfo
    resource_info;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  display=XOpenDisplay(image_info->server_name);
  if (display == (Display *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),XServerError,
        "UnableToOpenXServer","`%s'",XDisplayName(image_info->server_name));
      return(MagickFalse);
    }
  if (exception->severity != UndefinedException)
    CatchException(exception);
  (void) XSetErrorHandler(XError);
  resource_database=XGetResourceDatabase(display,GetClientName());
  (void) memset(&resource_info,0,sizeof(XResourceInfo));
  XGetResourceInfo(image_info,resource_database,GetClientName(),&resource_info);
  if (image_info->page != (char *) NULL)
    resource_info.image_geometry=AcquireString(image_info->page);
  resource_info.immutable=MagickTrue;
  argv[0]=AcquireString(GetClientName());
  (void) XAnimateImages(display,&resource_info,argv,1,images,exception);
  (void) SetErrorHandler((ErrorHandler) NULL);
  (void) SetWarningHandler((WarningHandler) NULL);
  argv[0]=DestroyString(argv[0]);
  (void) XCloseDisplay(display);
  XDestroyResourceInfo(&resource_info);
  status=exception->severity == UndefinedException ? MagickTrue : MagickFalse;
  return(status != 0 ? MagickTrue : MagickFalse);
}

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
      XDisplayImageInfo(display,resource_info,windows,(Image *) NULL,*image,
        exception);
      break;
    }
    case HelpCommand:
    {
      /*
        User requested help.
      */
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Animate",AnimateHelp);
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
      *state|=StepAnimationState;
      *state&=(~PlayAnimationState);
      if (animate_command == StepBackwardCommand)
        *state&=(~ForwardAnimationState);
      if (animate_command == StepForwardCommand)
        *state|=ForwardAnimationState;
      break;
    }
    default:
    {
      XRepeatSaveCommand(tile,animate_command);
      break;
    }
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
      if (resource_info->title != (char *) NULL)
        {
          char
            *title;

          title=InterpretImageProperties(resource_info->image_info,*image,
            resource_info->title,exception);
          (void) CloneString(&windows->image.name,title);
          title=DestroyString(title);
        }
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X A n i m a t e B a c k g r o u n d I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XAnimateBackgroundImage() animates an image sequence in the background of
%  a window.
%
%  The format of the XAnimateBackgroundImage method is:
%
%      void XAnimateBackgroundImage(Display *display,
%        XResourceInfo *resource_info,Image *images,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o images: the image list.
%
%    o exception: return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int SceneCompare(const void *x,const void *y)
{
  const Image
    **image_1,
    **image_2;

  image_1=(const Image **) x;
  image_2=(const Image **) y;
  return((int) ((*image_1)->scene-(*image_2)->scene));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static MagickBooleanType IsTrueColorMap(const char *map_type,
  const XVisualInfo *visual_info)
{
  return(((map_type != (char *) NULL) ||
    (visual_info->klass == TrueColor) ||
    (visual_info->klass == DirectColor)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType IsColormapMismatch(const Image *next,
  const Image *images,const XVisualInfo *visual_info)
{
  return(((next->storage_class == DirectClass) ||
    (next->colors != images->colors) ||
    (next->colors > (size_t) visual_info->colormap_size)) ? MagickTrue :
    MagickFalse);
}

static ssize_t FindColormapDifference(const Image *next,const Image *images)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) images->colors; i++)
    if (IsPixelInfoEquivalent(next->colormap+i,images->colormap+i) == MagickFalse)
      break;
  return(i);
}

static void XRemapBackgroundImages(Image *images,
  const XVisualInfo *visual_info,const XResourceInfo resources,
  ExceptionInfo *exception)
{
  Image
    *next;

  ssize_t
    i;

  /*
    Determine if the sequence of images has the identical colormap.
  */
  for (next=images; next != (Image *) NULL; )
  {
    next->alpha_trait=UndefinedPixelTrait;
    if (IsColormapMismatch(next,images,visual_info) != MagickFalse)
      break;
    i=FindColormapDifference(next,images);
    if (i < (ssize_t) images->colors)
      break;
    next=GetNextImageInList(next);
  }
  if (next != (Image *) NULL)
    (void) RemapImages(resources.quantize_info,images,(Image *) NULL,
      exception);
}

static MagickBooleanType XNextBackgroundEvent(Display *display,XEvent *event)
{
  if (XEventsQueued(display,QueuedAfterFlush) > 0)
    {
      (void) XNextEvent(display,event);
      if ((*event).type == DestroyNotify)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static void XSortSceneList(Image **image_list,const size_t number_scenes)
{
  ssize_t
    scene;

  for (scene=0; scene < (ssize_t) number_scenes; scene++)
    if (image_list[scene]->scene == 0)
      break;
  if (scene == (ssize_t) number_scenes)
    qsort((void *) image_list,number_scenes,sizeof(Image *),SceneCompare);
}

typedef struct _AnimateBackgroundSetup
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  ExceptionInfo
    *exception;

  Image
    *images,
    **image_list;

  size_t
    number_scenes;

  XVisualInfo
    *visual_info;

  XStandardColormap
    *map_info;
} AnimateBackgroundSetup;

typedef struct _AnimateBackgroundTargets
{
  XWindowInfo
    *window_info;

  XResourceInfo
    *resources;

  XWindowAttributes
    *window_attributes;

  unsigned int
    *width,
    *height;

  XPixelInfo
    *pixel;
} AnimateBackgroundTargets;

static void XCreateBackgroundXImage(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets)
{
  char
    geometry[MagickPathExtent];

  Display
    *const display = setup->display;

  Image
    **const image_list = setup->image_list;

  MagickStatusType
    status;

  RectangleInfo
    geometry_info;

  XResourceInfo
    *const resources = targets->resources;

  XWindowAttributes
    *const window_attributes = targets->window_attributes;

  XWindowInfo
    *const window_info = targets->window_info;

  (*window_info).width=(unsigned int) image_list[0]->columns;
  (*window_info).height=(unsigned int) image_list[0]->rows;
  if ((image_list[0]->columns != (*window_info).width) ||
      (image_list[0]->rows != (*window_info).height))
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      image_list[0]->filename);
  (void) FormatLocaleString(geometry,MagickPathExtent,"%ux%u+0+0>",
    (*window_attributes).width,(*window_attributes).height);
  geometry_info.width=(*window_info).width;
  geometry_info.height=(*window_info).height;
  geometry_info.x=(ssize_t) (*window_info).x;
  geometry_info.y=(ssize_t) (*window_info).y;
  (void) ParseMetaGeometry(geometry,&geometry_info.x,&geometry_info.y,
    &geometry_info.width,&geometry_info.height);
  (*window_info).width=(unsigned int) geometry_info.width;
  (*window_info).height=(unsigned int) geometry_info.height;
  (*window_info).x=(int) geometry_info.x;
  (*window_info).y=(int) geometry_info.y;
  status=XMakeImage(display,resources,window_info,image_list[0],
    (*window_info).width,(*window_info).height,setup->exception);
  if (status == MagickFalse)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      setup->images->filename);
  (*window_info).x=0;
  (*window_info).y=0;
  if (setup->resource_info->debug != MagickFalse)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "Image: %s[%.17g] %.17gx%.17g ",image_list[0]->filename,(double)
        image_list[0]->scene,(double) image_list[0]->columns,(double)
        image_list[0]->rows);
      if (image_list[0]->colors != 0)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"%.17gc ",(double)
          image_list[0]->colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"%s",
        image_list[0]->magick);
    }
}

static void XAdjustBackgroundSize(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets)
{
  unsigned int
    *const height = targets->height,
    *const width = targets->width;

  XResourceInfo
    *const resources = targets->resources;

  XWindowAttributes
    *const window_attributes = targets->window_attributes;

  XWindowInfo
    *const window_info = targets->window_info;

  (*width)=(*window_info).width;
  (*height)=(*window_info).height;
  if ((*resources).backdrop != MagickFalse)
    {
      /*
        Center image on window.
      */
      (*window_info).x=(int) ((*window_attributes).width/2)-
        ((*window_info).ximage->width/2);
      (*window_info).y=(int) ((*window_attributes).height/2)-
        ((*window_info).ximage->height/2);
      (*width)=(unsigned int) (*window_attributes).width;
      (*height)=(unsigned int) (*window_attributes).height;
    }
  if ((*resources).image_geometry != (char *) NULL)
    {
      char
        default_geometry[MagickPathExtent];

      int
        flags,
        gravity;

      XSizeHints
        *size_hints;

      /*
        User specified geometry.
      */
      size_hints=XAllocSizeHints();
      if (size_hints == (XSizeHints *) NULL)
        ThrowXWindowFatalException(ResourceLimitFatalError,
          "MemoryAllocationFailed",setup->images->filename);
      size_hints->flags=0L;
      (void) FormatLocaleString(default_geometry,MagickPathExtent,"%ux%u",
        (*width),(*height));
      flags=XWMGeometry(setup->display,setup->visual_info->screen,
        (*resources).image_geometry,default_geometry,
        (*window_info).border_width,size_hints,&(*window_info).x,
        &(*window_info).y,(int *) width,(int *) height,&gravity);
      if (((flags & (XValue | YValue))) != 0)
        {
          (*width)=(unsigned int) (*window_attributes).width;
          (*height)=(unsigned int) (*window_attributes).height;
        }
      (void) XFree((void *) size_hints);
    }
}

static void XCreateBackgroundPixmaps(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets)
{
  Display
    *const display = setup->display;

  unsigned int
    *const height = targets->height,
    *const width = targets->width;

  XWindowInfo
    *const window_info = targets->window_info;

  (*window_info).pixmap=XCreatePixmap(display,(*window_info).id,
    (unsigned int) (*width),(unsigned int) (*height),(*window_info).depth);
  if ((*window_info).pixmap == (Pixmap) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXPixmap",
      setup->images->filename);
  /*
    Display pixmap on the window.
  */
  if (((unsigned int) (*width) > (*window_info).width) ||
      ((unsigned int) (*height) > (*window_info).height))
    (void) XFillRectangle(display,(*window_info).pixmap,
      (*window_info).annotate_context,0,0,(unsigned int) (*width),
      (unsigned int) (*height));
  (void) XPutImage(display,(*window_info).pixmap,
    (*window_info).annotate_context,(*window_info).ximage,0,0,
    (*window_info).x,(*window_info).y,(*window_info).width,
    (*window_info).height);
  (void) XSetWindowBackgroundPixmap(display,(*window_info).id,
    (*window_info).pixmap);
  (void) XClearWindow(display,(*window_info).id);
  /*
    Initialize image pixmaps structure.
  */
  (*window_info).pixmaps=(Pixmap *) AcquireQuantumMemory(setup->number_scenes,
    sizeof(*(*window_info).pixmaps));
  (*window_info).matte_pixmaps=(Pixmap *) AcquireQuantumMemory(
    setup->number_scenes,sizeof(*(*window_info).matte_pixmaps));
  if (((*window_info).pixmaps == (Pixmap *) NULL) ||
      ((*window_info).matte_pixmaps == (Pixmap *) NULL))
    ThrowXWindowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      setup->images->filename);
  (*window_info).pixmaps[0]=(*window_info).pixmap;
  (*window_info).matte_pixmaps[0]=(*window_info).pixmap;
}

static void XCreateSceneXImage(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets,const int scene)
{
  Image
    **const image_list = setup->image_list;

  MagickStatusType
    status;

  unsigned int
    columns,
    rows;

  XResourceInfo
    *const resources = targets->resources;

  XStandardColormap
    *const map_info = setup->map_info;

  XVisualInfo
    *const visual_info = setup->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  (*window_info).pixmap=(Pixmap) NULL;
  (*window_info).matte_pixmap=(Pixmap) NULL;
  if ((IsTrueColorMap((*resources).map_type,visual_info) != MagickFalse) &&
      (image_list[scene]->storage_class == PseudoClass))
    XGetPixelInfo(setup->display,visual_info,map_info,resources,
      image_list[scene],(*window_info).pixel_info);
  columns=(unsigned int) image_list[scene]->columns;
  rows=(unsigned int) image_list[scene]->rows;
  if ((image_list[scene]->columns != columns) ||
      (image_list[scene]->rows != rows))
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      image_list[scene]->filename);
  status=XMakeImage(setup->display,resources,window_info,image_list[scene],
    columns,rows,setup->exception);
  if (status == MagickFalse)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      setup->images->filename);
  if (setup->resource_info->debug != MagickFalse)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "Image: [%.17g] %s %.17gx%.17g ",(double) image_list[scene]->scene,
        image_list[scene]->filename,(double) columns,(double) rows);
      if (image_list[scene]->colors != 0)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"%.17gc ",(double)
          image_list[scene]->colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"%s",
        image_list[scene]->magick);
    }
}

static void XCreateScenePixmap(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets,const int scene)
{
  Display
    *const display = setup->display;

  unsigned int
    *const height = targets->height,
    *const width = targets->width;

  XWindowInfo
    *const window_info = targets->window_info;

  (*window_info).pixmap=XCreatePixmap(display,(*window_info).id,(*width),
    (*height),(*window_info).depth);
  if ((*window_info).pixmap == (Pixmap) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXPixmap",
      setup->images->filename);
  /*
    Display pixmap on the window.
  */
  if (((*width) > (*window_info).width) || ((*height) > (*window_info).height))
    (void) XFillRectangle(display,(*window_info).pixmap,
      (*window_info).annotate_context,0,0,(*width),(*height));
  (void) XPutImage(display,(*window_info).pixmap,
    (*window_info).annotate_context,(*window_info).ximage,0,0,
    (*window_info).x,(*window_info).y,(*window_info).width,
    (*window_info).height);
  (void) XSetWindowBackgroundPixmap(display,(*window_info).id,
    (*window_info).pixmap);
  (void) XClearWindow(display,(*window_info).id);
  (*window_info).pixmaps[scene]=(*window_info).pixmap;
  (*window_info).matte_pixmaps[scene]=(*window_info).matte_pixmap;
  if (setup->image_list[scene]->alpha_trait)
    (void) XClearWindow(display,(*window_info).id);
}

static void XRunBackgroundAnimation(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets)
{
  Display
    *const display = setup->display;

  Image
    **const image_list = setup->image_list;

  int
    scene;

  size_t
    delay;

  ssize_t
    iterations;

  XEvent
    event;

  XResourceInfo
    *const resources = targets->resources;

  XWindowInfo
    *const window_info = targets->window_info;

  (void) XSelectInput(display,(*window_info).id,SubstructureNotifyMask);
  event.type=Expose;
  iterations=0;
  do
  {
    for (scene=0; scene < (int) setup->number_scenes; scene++)
    {
      if (XNextBackgroundEvent(display,&event) == MagickFalse)
        break;
      (*window_info).pixmap=(*window_info).pixmaps[scene];
      (*window_info).matte_pixmap=(*window_info).matte_pixmaps[scene];
      (void) XSetWindowBackgroundPixmap(display,(*window_info).id,
        (*window_info).pixmap);
      (void) XClearWindow(display,(*window_info).id);
      (void) XSync(display,MagickFalse);
      delay=1000*image_list[scene]->delay/(size_t) MagickMax(
        image_list[scene]->ticks_per_second,1L);
      XDelay(display,(*resources).delay*(delay == 0 ? 10 : delay));
    }
    iterations++;
    if (iterations == (ssize_t) image_list[0]->iterations)
      break;
  } while (event.type != DestroyNotify);
}

static void XMakeBackgroundColormap(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets)
{
  ExceptionInfo
    *const exception = setup->exception;

  Image
    *display_image,
    **const image_list = setup->image_list;

  int
    scene;

  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resource_info = setup->resource_info,
    *const resources = targets->resources;

  XVisualInfo
    *const visual_info = setup->visual_info;

  (*resources).colormap=SharedColormap;
  display_image=image_list[0];
  for (scene=0; scene < (int) setup->number_scenes; scene++)
  {
    if (IsTrueColorMap(resource_info->map_type,visual_info) != MagickFalse)
      (void) SetImageType(image_list[scene],image_list[scene]->alpha_trait ==
        BlendPixelTrait ? TrueColorType : TrueColorAlphaType,exception);
    if ((display_image->columns < image_list[scene]->columns) &&
        (display_image->rows < image_list[scene]->rows))
      display_image=image_list[scene];
  }
  if (IsTrueColorMap(resource_info->map_type,visual_info) != MagickFalse)
    (void) SetImageType(display_image,display_image->alpha_trait !=
      BlendPixelTrait ? TrueColorType : TrueColorAlphaType,exception);
  XMakeStandardColormap(setup->display,visual_info,resources,display_image,
    setup->map_info,pixel,exception);
}

static void XInitializeBackgroundWindow(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets)
{
  Display
    *const display = setup->display;

  XGCValues
    context_values;

  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resources = targets->resources;

  XWindowInfo
    *const window_info = targets->window_info;

  context_values.background=(*pixel).background_color.pixel;
  context_values.foreground=(*pixel).foreground_color.pixel;
  (*pixel).annotate_context=XCreateGC(display,(*window_info).id,(unsigned long)
    (GCBackground | GCForeground),&context_values);
  if ((*pixel).annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      setup->images->filename);
  /*
    Initialize Image window attributes.
  */
  XGetWindowInfo(display,setup->visual_info,setup->map_info,pixel,
    (XFontStruct *) NULL,resources,window_info);
}

static void XCreateBackgroundScenes(const AnimateBackgroundSetup *setup,
  const AnimateBackgroundTargets *targets)
{
  Image
    **const image_list = setup->image_list;

  int
    scene;

  size_t
    delay;

  XResourceInfo
    *const resources = targets->resources;

  for (scene=1; scene < (int) setup->number_scenes; scene++)
  {
    /*
      Create X image.
    */
    XCreateSceneXImage(setup,targets,scene);
    /*
      Create the X pixmap.
    */
    XCreateScenePixmap(setup,targets,scene);
    delay=1000*image_list[scene]->delay/(size_t) MagickMax(
      image_list[scene]->ticks_per_second,1L);
    XDelay(setup->display,(*resources).delay*(delay == 0 ? 10 : delay));
  }
}

static Image **
XSortAnimateScenes(Image *,const size_t,ExceptionInfo *);

static void XGetBackgroundVisualType(Display *display,
  const AnimateBackgroundTargets *targets,char *visual_type)
{
  MagickStatusType
    status;

  XWindowAttributes
    *const window_attributes = targets->window_attributes;

  XWindowInfo
    *const window_info = targets->window_info;

  (*window_attributes).width=XDisplayWidth(display,XDefaultScreen(display));
  (*window_attributes).height=XDisplayHeight(display,XDefaultScreen(display));
  (void) CopyMagickString(visual_type,"default",MagickPathExtent);
  status=XGetWindowAttributes(display,(*window_info).id,
    window_attributes) != 0 ? MagickTrue : MagickFalse;
  if (status != MagickFalse)
    (void) FormatLocaleString(visual_type,MagickPathExtent,"0x%lx",
      XVisualIDFromVisual((*window_attributes).visual));
}

typedef struct _AnimateBackgroundEarlyTargets
{
  Window
    *root_window;

  XResourceInfo
    *resources;

  XWindowInfo
    *window_info;

  XPixelInfo
    *pixel;

  XStandardColormap
    **map_info;

  XVisualInfo
    **visual_info;
} AnimateBackgroundEarlyTargets;

static void XFindBackgroundWindow(Display *display,
  XResourceInfo *resource_info,const AnimateBackgroundEarlyTargets *targets)
{
  Window
    *const root_window = targets->root_window;

  XResourceInfo
    *const resources = targets->resources;

  XWindowInfo
    *const window_info = targets->window_info;

  (*resources)=(*resource_info);
  (*window_info).id=(Window) NULL;
  (*root_window)=XRootWindow(display,XDefaultScreen(display));
  if (LocaleCompare((*resources).window_id,"root") == 0)
    (*window_info).id=(*root_window);
  else
    {
      if (isdigit((int) ((unsigned char) *(*resources).window_id)) != 0)
        (*window_info).id=XWindowByID(display,(*root_window),
          (Window) strtol((char *) (*resources).window_id,(char **) NULL,0));
      if ((*window_info).id == (Window) NULL)
        (*window_info).id=
          XWindowByName(display,(*root_window),(*resources).window_id);
    }
}

static void XCreateBackgroundVisual(Display *display,const char *filename,
  char *visual_type,const AnimateBackgroundEarlyTargets *targets)
{
  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resources = targets->resources;

  XStandardColormap
    **const map_info = targets->map_info;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  if ((*visual_info) == (XVisualInfo *) NULL)
    {
      /*
        Allocate standard colormap.
      */
      (*map_info)=XAllocStandardColormap();
      if ((*map_info) == (XStandardColormap *) NULL)
        ThrowXWindowFatalException(ResourceLimitFatalError,
          "MemoryAllocationFailed",filename);
      (*map_info)->colormap=(Colormap) NULL;
      (*pixel).pixels=(unsigned long *) NULL;
      /*
        Initialize visual info.
      */
      (*resources).map_type=(char *) NULL;
      (*resources).visual_type=visual_type;
      (*visual_info)=XBestVisualInfo(display,(*map_info),resources);
      if ((*visual_info) == (XVisualInfo *) NULL)
        ThrowXWindowFatalException(XServerFatalError,"UnableToGetVisual",
          filename);
      /*
        Initialize window info.
      */
      (*window_info).ximage=(XImage *) NULL;
      (*window_info).matte_image=(XImage *) NULL;
      (*window_info).pixmap=(Pixmap) NULL;
      (*window_info).matte_pixmap=(Pixmap) NULL;
    }
}

static Image *XPrepareBackgroundImages(Display *display,Image *images,
  ExceptionInfo *exception,const AnimateBackgroundEarlyTargets *targets)
{
  Image
    *coalesce_image;

  Window
    *const root_window = targets->root_window;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  if ((*window_info).id == (*root_window))
    XDestroyWindowColors(display,(*root_window));
  coalesce_image=CoalesceImages(images,exception);
  if (coalesce_image == (Image *) NULL)
    ThrowXWindowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      images->filename);
  images=coalesce_image;
  if ((*resources).map_type == (char *) NULL)
    if (((*visual_info)->klass != TrueColor) &&
        ((*visual_info)->klass != DirectColor))
      XRemapBackgroundImages(images,(*visual_info),(*resources),exception);
  return(images);
}

static void XReleaseBackgroundImages(Display *display,Image *images,
  Image **image_list)
{
  (void) XSync(display,MagickFalse);
  image_list=(Image **) RelinquishMagickMemory(image_list);
  images=DestroyImageList(images);
}

MagickExport void XAnimateBackgroundImage(Display *display,
  XResourceInfo *resource_info,Image *images,ExceptionInfo *exception)
{
  char
    visual_type[MagickPathExtent];

  Image
    **image_list;

  size_t
    number_scenes;

  static XPixelInfo
    pixel;

  static XStandardColormap
    *map_info;

  static XVisualInfo
    *visual_info = (XVisualInfo *) NULL;

  static XWindowInfo
    window_info;

  unsigned int
    height,
    width;

  Window
    root_window;

  XResourceInfo
    resources;

  XWindowAttributes
    window_attributes;

  AnimateBackgroundSetup
    setup;

  const AnimateBackgroundTargets
    targets = { &window_info, &resources, &window_attributes, &width,
      &height, &pixel };

  const AnimateBackgroundEarlyTargets
    early_targets = { &root_window, &resources, &window_info, &pixel,
      &map_info, &visual_info };

  /*
    Determine target window.
  */
  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  XFindBackgroundWindow(display,resource_info,&early_targets);
  if (window_info.id == (Window) NULL)
    {
      ThrowXWindowException(XServerError,"NoWindowWithSpecifiedIDExists",
        resources.window_id);
      return;
    }
  /*
    Determine window visual id.
  */
  XGetBackgroundVisualType(display,&targets,visual_type);
  XCreateBackgroundVisual(display,images->filename,visual_type,
    &early_targets);
  /*
    Free previous root colors.
  */
  images=XPrepareBackgroundImages(display,images,exception,&early_targets);
  /*
    Sort images by increasing scene number.
  */
  number_scenes=GetImageListLength(images);
  image_list=XSortAnimateScenes(images,number_scenes,exception);
  setup.display=display;
  setup.resource_info=resource_info;
  setup.exception=exception;
  setup.images=images;
  setup.image_list=image_list;
  setup.number_scenes=number_scenes;
  setup.visual_info=visual_info;
  setup.map_info=map_info;
  /*
    Initialize Standard Colormap.
  */
  XMakeBackgroundColormap(&setup,&targets);
  /*
    Graphic context superclass.
  */
  XInitializeBackgroundWindow(&setup,&targets);
  /*
    Create the X image.
  */
  XCreateBackgroundXImage(&setup,&targets);
  /*
    Adjust image dimensions as specified by backdrop or geometry options.
  */
  XAdjustBackgroundSize(&setup,&targets);
  /*
    Create the X pixmap.
  */
  XCreateBackgroundPixmaps(&setup,&targets);
  XCreateBackgroundScenes(&setup,&targets);
  window_info.pixel_info=(&pixel);
  /*
    Display pixmap on the window.
  */
  XRunBackgroundAnimation(&setup,&targets);
  XReleaseBackgroundImages(display,images,image_list);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X A n i m a t e I m a g e s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XAnimateImages() displays an image via X11.
%
%  The format of the XAnimateImages method is:
%
%      Image *XAnimateImages(Display *display,XResourceInfo *resource_info,
%        char **argv,const int argc,Image *images,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o argv: Specifies the application's argument list.
%
%    o argc: Specifies the number of arguments.
%
%    o images: the image list.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType IsColormapVisual(const XVisualInfo *visual_info)
{
  return(((visual_info->klass == GrayScale) ||
    (visual_info->klass == PseudoColor) ||
    (visual_info->klass == DirectColor)) ? MagickTrue : MagickFalse);
}

static void XFocusBackdropImage(Display *display,XWindows *windows)
{
  if (windows->backdrop.id != (Window) NULL)
    (void) XSetInputFocus(display,windows->image.id,RevertToParent,
      CurrentTime);
}

static Image *XFindColormapMismatch(Image *images,
  const XVisualInfo *visual_info)
{
  Image
    *next;

  ssize_t
    i;

  for (next=images; next != (Image *) NULL; )
  {
    next->alpha_trait=UndefinedPixelTrait;
    if (IsColormapMismatch(next,images,visual_info) != MagickFalse)
      break;
    i=FindColormapDifference(next,images);
    if (i < (ssize_t) images->colors)
      break;
    next=GetNextImageInList(next);
  }
  return(next);
}

static void XPlaceTransientWindows(Display *display,XWindows *windows,
  const XEvent event)
{
  if (event.xconfigure.send_event != 0)
    {
      XWindowChanges
        window_changes;

      /*
        Position the transient windows relative of the Image window.
      */
      if ((windows->command.geometry == (char *) NULL) &&
          (windows->command.mapped == MagickFalse))
        {
            windows->command.x=event.xconfigure.x-
              (int) windows->command.width-25;
            windows->command.y=event.xconfigure.y;
            XConstrainWindowPosition(display,&windows->command);
            window_changes.x=windows->command.x;
            window_changes.y=windows->command.y;
            (void) XReconfigureWMWindow(display,windows->command.id,
              windows->command.screen,(unsigned int) (CWX | CWY),
              &window_changes);
        }
      if ((windows->widget.geometry == (char *) NULL) &&
          (windows->widget.mapped == MagickFalse))
        {
          windows->widget.x=
            event.xconfigure.x+event.xconfigure.width/10;
          windows->widget.y=
            event.xconfigure.y+event.xconfigure.height/10;
          XConstrainWindowPosition(display,&windows->widget);
          window_changes.x=windows->widget.x;
          window_changes.y=windows->widget.y;
          (void) XReconfigureWMWindow(display,windows->widget.id,
            windows->widget.screen,(unsigned int) (CWX | CWY),
            &window_changes);
        }
    }
}

static AnimateCommand XControlOpenCommand(const XEvent event,
  AnimateCommand animate_command)
{
  if ((event.xkey.state & ControlMask) == MagickFalse)
    return(animate_command);
  animate_command=OpenCommand;
  return(animate_command);
}

typedef struct _AnimateIconColormaps
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    *display_image;

  ExceptionInfo
    *exception;

  XStandardColormap
    *map_info;

  XStandardColormap
    *icon_map;

  XVisualInfo
    *icon_visual;

  XPixelInfo
    *icon_pixel;
} AnimateIconColormaps;

static void XReleaseIconColormap(const AnimateIconColormaps *icon_colormaps)
{
  Display
    *const display = icon_colormaps->display;

  XResourceInfo
    *const resource_info = icon_colormaps->resource_info;

  XWindows
    *const windows = icon_colormaps->windows;

  Image
    *const display_image = icon_colormaps->display_image;

  ExceptionInfo
    *const exception = icon_colormaps->exception;

  XStandardColormap
    *const map_info = icon_colormaps->map_info;

  XStandardColormap
    *const icon_map = icon_colormaps->icon_map;

  XVisualInfo
    *const icon_visual = icon_colormaps->icon_visual;

  XPixelInfo
    *const icon_pixel = icon_colormaps->icon_pixel;

  if (map_info->colormap == icon_map->colormap)
    XConfigureImageColormap(display,resource_info,windows,
      display_image,exception);
  (void) XFreeStandardColormap(display,icon_visual,icon_map,
    icon_pixel);
  windows->icon.mapped=MagickFalse;
}

typedef struct _AnimateImageWindow
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  XStandardColormap
    *map_info;

  Image
    **image_list;

  Image
    *display_image;

  ExceptionInfo
    *exception;
} AnimateImageWindow;

static void XMapAnimateImageWindow(const AnimateImageWindow *image_window,
  Image **nexus,Image **image,MagickStatusType *state)
{
  Display
    *const display = image_window->display;

  XResourceInfo
    *const resource_info = image_window->resource_info;

  XWindows
    *const windows = image_window->windows;

  XStandardColormap
    *const map_info = image_window->map_info;

  Image
    **const image_list = image_window->image_list;

  Image
    *const display_image = image_window->display_image;

  ExceptionInfo
    *const exception = image_window->exception;

  const AnimateCommandTile
    tile = { display, resource_info, windows, image, state, exception };

  if (windows->backdrop.id != (Window) NULL)
    (void) XInstallColormap(display,map_info->colormap);
  if (LocaleCompare(image_list[0]->magick,"LOGO") == 0)
    {
      if (LocaleCompare(display_image->filename,"LOGO") == 0)
        (*nexus)=XMagickCommand(&tile,OpenCommand);
      else
        (*state)|=ExitState;
    }
  windows->image.mapped=MagickTrue;
}

typedef struct _AnimateCommandWindow
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const char
    *const *CommandMenu;
} AnimateCommandWindow;

static MagickStatusType XPressImageWindow(
  const AnimateCommandWindow *command_window,MagickStatusType state)
{
  Display
    *const display = command_window->display;

  XResourceInfo
    *const resource_info = command_window->resource_info;

  XWindows
    *const windows = command_window->windows;

  const char
    *const *const CommandMenu = command_window->CommandMenu;

  if (resource_info->immutable != MagickFalse)
    {
      state|=ExitState;
      return(state);
    }
  /*
    Map/unmap Command widget.
  */
  if (windows->command.mapped)
    (void) XWithdrawWindow(display,windows->command.id,
      windows->command.screen);
  else
    {
      (void) XCommandWidget(display,windows,CommandMenu,
        (XEvent *) NULL);
      (void) XMapRaised(display,windows->command.id);
    }
  return(state);
}

#define MagickMenus  4
#define MaXWindows  8
#define MagickTitle  "Commands"

typedef struct _AnimateMenuSelection
{
  Display
    *display;

  XWindows
    *windows;

  const char
    *const *CommandMenu,
    *const **Menus;

  const AnimateCommand
    **Commands;

  int
    id;
} AnimateMenuSelection;

static MagickBooleanType XSelectMenuCommand(
  const AnimateMenuSelection *menu_selection,char *command,
  AnimateCommand *animate_command)
{
  Display
    *const display = menu_selection->display;

  XWindows
    *const windows = menu_selection->windows;

  const char
    *const *const CommandMenu = menu_selection->CommandMenu,
    *const **const Menus = menu_selection->Menus;

  const AnimateCommand
    **const Commands = menu_selection->Commands;

  const int
    id = menu_selection->id;

  if (id < MagickMenus)
    {
      int
        entry;

      /*
        Select a command from a pop-up menu.
      */
      entry=XMenuWidget(display,windows,CommandMenu[id],Menus[id],
        command);
      if (entry < 0)
        return(MagickFalse);
      (void) CopyMagickString(command,Menus[id][entry],MagickPathExtent);
      *animate_command=Commands[id][entry];
    }
  return(MagickTrue);
}

typedef struct _AnimateWindowColormaps
{
  Display
    *display;

  XWindows
    *windows;

  XStandardColormap
    *map_info;

  XPixelInfo
    *pixel;

  XWindowInfo
    **magick_windows;

  size_t
    number_windows;

  MagickStatusType
    context_mask;

  XEvent
    event;
} AnimateWindowColormaps;

static void XUpdateWindowColormaps(
  const AnimateWindowColormaps *window_colormaps,XGCValues *context_values)
{
  Display
    *const display = window_colormaps->display;

  XWindows
    *const windows = window_colormaps->windows;

  XStandardColormap
    *const map_info = window_colormaps->map_info;

  XPixelInfo
    *const pixel = window_colormaps->pixel;

  XWindowInfo
    **const magick_windows = window_colormaps->magick_windows;

  const size_t
    number_windows = window_colormaps->number_windows;

  const MagickStatusType
    context_mask = window_colormaps->context_mask;

  ssize_t
    i;

  /*
    Update graphic context and window colormap.
  */
  for (i=0; i < (ssize_t) number_windows; i++)
  {
    if (magick_windows[i]->id == windows->icon.id)
      continue;
    (*context_values).background=pixel->background_color.pixel;
    (*context_values).foreground=pixel->foreground_color.pixel;
    (void) XChangeGC(display,magick_windows[i]->annotate_context,
      context_mask,context_values);
    (void) XChangeGC(display,magick_windows[i]->widget_context,
      context_mask,context_values);
    (*context_values).background=pixel->foreground_color.pixel;
    (*context_values).foreground=pixel->background_color.pixel;
    (*context_values).plane_mask=
      (*context_values).background ^ (*context_values).foreground;
    (void) XChangeGC(display,magick_windows[i]->highlight_context,
      (size_t) (context_mask | GCPlaneMask),
      context_values);
    magick_windows[i]->attributes.background_pixel=
      pixel->background_color.pixel;
    magick_windows[i]->attributes.border_pixel=
      pixel->border_color.pixel;
    magick_windows[i]->attributes.colormap=map_info->colormap;
    (void) XChangeWindowAttributes(display,magick_windows[i]->id,
      (unsigned long) magick_windows[i]->mask,
      &magick_windows[i]->attributes);
  }
  if (windows->backdrop.id != (Window) NULL)
    (void) XInstallColormap(display,map_info->colormap);
}

typedef struct _AnimateDropSource
{
  Display
    *display;

  Window
    root_window;

  XResourceInfo
    *resource_info;

  ExceptionInfo
    *exception;

  XEvent
    event;

  XWindows
    *windows;
} AnimateDropSource;

typedef struct _AnimateDropTargets
{
  Image
    **nexus;

  MagickStatusType
    *state;
} AnimateDropTargets;

static MagickBooleanType XReadDroppedImage(
  const AnimateDropSource *drop_source,const AnimateDropTargets *targets)
{
  Display
    *const display = drop_source->display;

  const Window
    root_window = drop_source->root_window;

  XResourceInfo
    *const resource_info = drop_source->resource_info;

  ExceptionInfo
    *const exception = drop_source->exception;

  const XEvent
    event = drop_source->event;

  Image
    **const nexus = targets->nexus;

  MagickStatusType
    *const state = targets->state;

  Atom
    selection,
    type;

  int
    format,
    status;

  unsigned char
    *data;

  unsigned long
    after,
    length;

  /*
    Display image named by the Drag-and-Drop selection.
  */
  if ((*event.xclient.data.l != 2) && (*event.xclient.data.l != 128))
    return(MagickFalse);
  selection=XInternAtom(display,"DndSelection",MagickFalse);
  status=XGetWindowProperty(display,root_window,selection,0L,2047L,
    MagickFalse,(Atom) AnyPropertyType,&type,&format,&length,&after,
    &data);
  if ((status != Success) || (length == 0))
    return(MagickFalse);
  if (*event.xclient.data.l == 2)
    {
      /*
        Offix DND.
      */
      (void) CopyMagickString(resource_info->image_info->filename,
        (char *) data,MagickPathExtent);
    }
  else
    {
      /*
        XDND.
      */
      if (LocaleNCompare((char *) data,"file:",5) != 0)
        {
          (void) XFree((void *) data);
          return(MagickFalse);
        }
      (void) CopyMagickString(resource_info->image_info->filename,
        ((char *) data)+5,MagickPathExtent);
    }
  *nexus=ReadImage(resource_info->image_info,exception);
  CatchException(exception);
  if (*nexus != (Image *) NULL)
    *state|=ExitState;
  (void) XFree((void *) data);
  return(MagickTrue);
}

typedef struct _AnimateProtocolTargets
{
  MagickStatusType
    *state;

  XGCValues
    *context_values;
} AnimateProtocolTargets;

static MagickBooleanType XHandleMagickProtocol(
  const AnimateWindowColormaps *window_colormaps,
  const AnimateProtocolTargets *targets)
{
  XWindows
    *const windows = window_colormaps->windows;

  const XEvent
    event = window_colormaps->event;

  MagickStatusType
    *const state = targets->state;

  XGCValues
    *const context_values = targets->context_values;

  if (event.xclient.message_type == windows->im_protocols)
    {
      if (*event.xclient.data.l == (long) windows->im_update_colormap)
        {
          XUpdateWindowColormaps(window_colormaps,context_values);
          return(MagickFalse);
        }
      if (*event.xclient.data.l == (long) windows->im_exit)
        {
          *state|=ExitState;
          return(MagickFalse);
        }
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType XHandleDropProtocol(
  const AnimateDropSource *drop_source,const AnimateDropTargets *targets)
{
  XWindows
    *const windows = drop_source->windows;

  const XEvent
    event = drop_source->event;

  if (event.xclient.message_type == windows->dnd_protocols)
    {
      if (XReadDroppedImage(drop_source,targets) == MagickFalse)
        return(MagickFalse);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void XPauseAnimation(Display *display,XResourceInfo *resource_info,
  XWindows *windows,XEvent *event)
{
  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  ssize_t
    i;

  for (i=0; i < (ssize_t) resource_info->pause; i++)
  {
    int
      status;

    status=XCheckTypedWindowEvent(display,windows->image.id,KeyPress,
      event);
    if (status != 0)
      {
        int
          length;

        length=XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
          sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        *(command+length)='\0';
        if ((key_symbol == XK_q) || (key_symbol == XK_Escape))
          {
            XClientMessage(display,windows->image.id,
              windows->im_protocols,windows->im_exit,CurrentTime);
            break;
          }
      }
    MagickDelay(1000);
  }
}

typedef struct _AnimationTitle
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    **image_list;

  Image
    *image;

  ExceptionInfo
    *exception;

  ssize_t
    scene;

  size_t
    number_scenes;

  MagickStatusType
    state;
} AnimationTitle;

static void XUpdateAnimationTitle(const AnimationTitle *animation_title)
{
  Display
    *const display = animation_title->display;

  XResourceInfo
    *const resource_info = animation_title->resource_info;

  XWindows
    *const windows = animation_title->windows;

  Image
    **const image_list = animation_title->image_list;

  Image
    *const image = animation_title->image;

  ExceptionInfo
    *const exception = animation_title->exception;

  const ssize_t
    scene = animation_title->scene;

  const size_t
    number_scenes = animation_title->number_scenes;

  const MagickStatusType
    state = animation_title->state;

  char
    *p;

  int
    status;

  XTextProperty
    window_name;

  if ((state & StepAnimationState) ||
      (resource_info->title != (char *) NULL))
    {
      char
        name[MagickPathExtent];

      /*
        Update window title.
      */
      p=image_list[scene]->filename+
        strlen(image_list[scene]->filename)-1;
      while ((p > image_list[scene]->filename) && (*(p-1) != '/'))
        p--;
      (void) FormatLocaleString(name,MagickPathExtent,
        "%s: %s[%.17g of %.17g]",MagickPackageName,p,(double)
        scene+1,(double) number_scenes);
      (void) CloneString(&windows->image.name,name);
      if (resource_info->title != (char *) NULL)
        {
          char
            *title;

          title=InterpretImageProperties(resource_info->image_info,
            image,resource_info->title,exception);
          (void) CloneString(&windows->image.name,title);
          title=DestroyString(title);
        }
      status=XStringListToTextProperty(&windows->image.name,1,
        &window_name);
      if (status != Success)
        {
          XSetWMName(display,windows->image.id,&window_name);
          (void) XFree((void *) window_name.value);
        }
    }
}

typedef struct _AnimationSceneStep
{
  size_t
    number_scenes;

  Image
    **image_list;

  ssize_t
    first_scene;
} AnimationSceneStep;

typedef struct _AnimationSceneTargets
{
  ssize_t
    *scene,
    *iterations;

  MagickStatusType
    *state;

  MagickBooleanType
    *pause;
} AnimationSceneTargets;

static void XWrapSceneForward(const AnimationSceneStep *scene_step,
  const AnimationSceneTargets *targets)
{
  Image
    **const image_list = scene_step->image_list;

  const ssize_t
    first_scene = scene_step->first_scene;

  ssize_t
    *const scene = targets->scene,
    *const iterations = targets->iterations;

  MagickStatusType
    *const state = targets->state;

  MagickBooleanType
    *const pause = targets->pause;

  (*iterations)++;
  if ((*iterations) == (ssize_t) image_list[0]->iterations)
    {
      (*iterations)=0;
      (*state)|=ExitState;
    }
  if (((*state) & AutoReverseAnimationState) != 0)
    {
      (*state)&=(~ForwardAnimationState);
      (*scene)--;
    }
  else
    {
      if (((*state) & RepeatAnimationState) == 0)
        (*state)&=(~PlayAnimationState);
      (*scene)=first_scene;
      (*pause)=MagickTrue;
    }
}

static void XWrapSceneReverse(const AnimationSceneStep *scene_step,
  const AnimationSceneTargets *targets)
{
  const size_t
    number_scenes = scene_step->number_scenes;

  Image
    **const image_list = scene_step->image_list;

  const ssize_t
    first_scene = scene_step->first_scene;

  ssize_t
    *const scene = targets->scene,
    *const iterations = targets->iterations;

  MagickStatusType
    *const state = targets->state;

  MagickBooleanType
    *const pause = targets->pause;

  (*iterations)++;
  if ((*iterations) == (ssize_t) image_list[0]->iterations)
    {
      (*iterations)=0;
      (*state)&=(~RepeatAnimationState);
    }
  if ((*state) & AutoReverseAnimationState)
    {
      (*state)|=ForwardAnimationState;
      (*scene)=first_scene;
      (*pause)=MagickTrue;
    }
  else
    {
      if (((*state) & RepeatAnimationState) == MagickFalse)
        (*state)&=(~PlayAnimationState);
      (*scene)=(ssize_t) number_scenes-1;
    }
}

static inline MagickBooleanType IsAnimationPlaying(
  const MagickStatusType state)
{
  return(((state & PlayAnimationState) || (state & StepAnimationState)) ?
    MagickTrue : MagickFalse);
}

static void XAdvanceSceneForward(const AnimationSceneStep *scene_step,
  const AnimationSceneTargets *targets)
{
  const size_t
    number_scenes = scene_step->number_scenes;

  ssize_t
    *const scene = targets->scene;

  if ((*scene) < ((ssize_t) number_scenes-1))
    (*scene)++;
  else
    XWrapSceneForward(scene_step,targets);
}

static void XAdvanceScene(const AnimationSceneStep *scene_step,
  const AnimationSceneTargets *targets)
{
  const ssize_t
    first_scene = scene_step->first_scene;

  ssize_t
    *const scene = targets->scene;

  MagickStatusType
    *const state = targets->state;

  if ((*state) & ForwardAnimationState)
    {
      /*
        Forward animation:  increment scene number.
      */
      XAdvanceSceneForward(scene_step,targets);
    }
  else
    {
      /*
        Reverse animation:  decrement scene number.
      */
      if ((*scene) > first_scene)
        (*scene)--;
      else
        XWrapSceneReverse(scene_step,targets);
    }
}

typedef struct _AnimateSceneView
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  XVisualInfo
    *visual_info;

  XStandardColormap
    *map_info;

  Image
    **image_list;

  Image
    *image;

  ssize_t
    scene;
} AnimateSceneView;

static void XShowAnimationScene(const AnimateSceneView *scene_view,
  const MagickStatusType state,XEvent *event)
{
  Display
    *const display = scene_view->display;

  XResourceInfo
    *const resource_info = scene_view->resource_info;

  XWindows
    *const windows = scene_view->windows;

  XVisualInfo
    *const visual_info = scene_view->visual_info;

  XStandardColormap
    *const map_info = scene_view->map_info;

  Image
    **const image_list = scene_view->image_list;

  Image
    *const image = scene_view->image;

  const ssize_t
    scene = scene_view->scene;

  XGetPixelInfo(display,visual_info,map_info,resource_info,
    image_list[scene],windows->image.pixel_info);
  if (image != (Image *) NULL)
    {
      windows->image.ximage->width=(int) image->columns;
      windows->image.ximage->height=(int) image->rows;
    }
  windows->image.pixmap=windows->image.pixmaps[scene];
  windows->image.matte_pixmap=windows->image.matte_pixmaps[scene];
  (*event).xexpose.x=0;
  (*event).xexpose.y=0;
  (*event).xexpose.width=(int) image->columns;
  (*event).xexpose.height=(int) image->rows;
  if ((state & ExitState) == 0)
    {
      XRefreshWindow(display,&windows->image,event);
      (void) XSync(display,MagickFalse);
    }
}

static void XPauseAnimationScene(const AnimateSceneView *scene_view,
  const MagickBooleanType pause,XEvent *event)
{
  Display
    *const display = scene_view->display;

  XResourceInfo
    *const resource_info = scene_view->resource_info;

  XWindows
    *const windows = scene_view->windows;

  if (pause != MagickFalse)
    XPauseAnimation(display,resource_info,windows,event);
}

static ssize_t XLoopStartScene(const Image *image,const ssize_t scene,
  ssize_t first_scene)
{
  if ((image != (Image *) NULL) && (image->start_loop != 0))
    first_scene=scene;
  return(first_scene);
}

static void XRemapColormapMismatch(XResourceInfo *resource_info,
  XVisualInfo *visual_info,Image *images,ExceptionInfo *exception)
{
  if (resource_info->map_type == (char *) NULL)
    if ((visual_info->klass != TrueColor) &&
        (visual_info->klass != DirectColor))
      {
        Image
          *next;

        /*
          Determine if the sequence of images has the identical colormap.
        */
        next=XFindColormapMismatch(images,visual_info);
        if (next != (Image *) NULL)
          (void) RemapImages(resource_info->quantize_info,images,
            (Image *) NULL,exception);
      }
}

static void XWithdrawColormapWindows(Display *display,XWindows *windows)
{
  if (IsColormapVisual(windows->visual_info) != MagickFalse)
    {
      /*
        Withdraw windows.
      */
      if (windows->info.mapped)
        (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
      if (windows->command.mapped)
        (void) XWithdrawWindow(display,windows->command.id,
          windows->command.screen);
    }
}

static void XFreeScenePixmaps(Display *display,XWindows *windows,
  const size_t number_scenes)
{
  ssize_t
    scene;

  for (scene=1; scene < (ssize_t) number_scenes; scene++)
  {
    if (windows->image.pixmaps[scene] != (Pixmap) NULL)
      (void) XFreePixmap(display,windows->image.pixmaps[scene]);
    windows->image.pixmaps[scene]=(Pixmap) NULL;
    if (windows->image.matte_pixmaps[scene] != (Pixmap) NULL)
      (void) XFreePixmap(display,windows->image.matte_pixmaps[scene]);
    windows->image.matte_pixmaps[scene]=(Pixmap) NULL;
  }
}

typedef struct _AnimateX11Resources
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  XVisualInfo
    *visual_info;

  XStandardColormap
    *map_info;

  XPixelInfo
    *pixel;

  XVisualInfo
    *icon_visual;

  XStandardColormap
    *icon_map;

  XPixelInfo
    *icon_pixel;
} AnimateX11Resources;

static void XReleaseAnimateResources(
  const AnimateX11Resources *x11_resources)
{
  Display
    *const display = x11_resources->display;

  XResourceInfo
    *const resource_info = x11_resources->resource_info;

  XWindows
    *const windows = x11_resources->windows;

  XVisualInfo
    *const visual_info = x11_resources->visual_info;

  XStandardColormap
    *const map_info = x11_resources->map_info;

  XPixelInfo
    *const pixel = x11_resources->pixel;

  XVisualInfo
    *const icon_visual = x11_resources->icon_visual;

  XStandardColormap
    *const icon_map = x11_resources->icon_map;

  XPixelInfo
    *const icon_pixel = x11_resources->icon_pixel;

  /*
    Free X resources.
  */
  if (windows->image.mapped != MagickFalse)
    (void) XWithdrawWindow(display,windows->image.id,windows->image.screen);
  XDelay(display,SuspendTime);
  (void) XFreeStandardColormap(display,icon_visual,icon_map,icon_pixel);
  if (resource_info->map_type == (char *) NULL)
    (void) XFreeStandardColormap(display,visual_info,map_info,pixel);
  DestroyXResources();
}

static WarningHandler XSetAnimateWarningHandlers(
  const XResourceInfo *resource_info,WarningHandler warning_handler)
{
  if (warning_handler == (WarningHandler) NULL)
    {
      warning_handler=resource_info->display_warnings ?
        SetErrorHandler(XWarning) : SetErrorHandler((ErrorHandler) NULL);
      warning_handler=resource_info->display_warnings ?
        SetWarningHandler(XWarning) : SetWarningHandler((WarningHandler) NULL);
    }
  return(warning_handler);
}

typedef struct _AnimateSceneList
{
  XResourceInfo
    *resource_info;

  XVisualInfo
    *visual_info;

  Image
    **image_list;

  size_t
    number_scenes;

  ExceptionInfo
    *exception;
} AnimateSceneList;

static Image *XPrepareSceneImages(const AnimateSceneList *scene_list)
{
  XResourceInfo
    *const resource_info = scene_list->resource_info;

  XVisualInfo
    *const visual_info = scene_list->visual_info;

  Image
    **const image_list = scene_list->image_list;

  const size_t
    number_scenes = scene_list->number_scenes;

  ExceptionInfo
    *const exception = scene_list->exception;

  Image
    *display_image;

  ssize_t
    scene;

  display_image=image_list[0];
  for (scene=0; scene < (ssize_t) number_scenes; scene++)
  {
    if (IsTrueColorMap(resource_info->map_type,visual_info) != MagickFalse)
      (void) SetImageType(image_list[scene],image_list[scene]->alpha_trait ==
        BlendPixelTrait ? TrueColorType : TrueColorAlphaType,exception);
    if ((display_image->columns < image_list[scene]->columns) &&
        (display_image->rows < image_list[scene]->rows))
      display_image=image_list[scene];
  }
  return(display_image);
}

static void XCenterImageOnBackdrop(Display *display,XWindows *windows,
  XVisualInfo *visual_info)
{
  (void) XMapWindow(display,windows->backdrop.id);
  (void) XClearWindow(display,windows->backdrop.id);
  if (windows->image.id != (Window) NULL)
    {
      (void) XDestroyWindow(display,windows->image.id);
      windows->image.id=(Window) NULL;
    }
  /*
    Position image in the center the backdrop.
  */
  windows->image.flags|=USPosition;
  windows->image.x=(XDisplayWidth(display,visual_info->screen)/2)-
    ((int) windows->image.width/2);
  windows->image.y=(XDisplayHeight(display,visual_info->screen)/2)-
    ((int) windows->image.height/2);
}

typedef struct _AnimateSceneWindow
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    **image_list;

  size_t
    number_scenes;

  ExceptionInfo
    *exception;

  ssize_t
    scene;
} AnimateSceneWindow;

static void XNameSceneWindow(const AnimateSceneWindow *scene_window)
{
  XResourceInfo
    *const resource_info = scene_window->resource_info;

  XWindows
    *const windows = scene_window->windows;

  Image
    **const image_list = scene_window->image_list;

  const size_t
    number_scenes = scene_window->number_scenes;

  ExceptionInfo
    *const exception = scene_window->exception;

  const ssize_t
    scene = scene_window->scene;

  char
    *p;

  if (resource_info->title != (char *) NULL)
    {
      char
        *title;

      title=InterpretImageProperties(resource_info->image_info,
        image_list[scene],resource_info->title,exception);
      (void) CloneString(&windows->image.name,title);
      title=DestroyString(title);
    }
  else
    {
      char
        window_name[MagickPathExtent];

      p=image_list[scene]->magick_filename+
        strlen(image_list[scene]->magick_filename)-1;
      while ((p > image_list[scene]->magick_filename) && (*(p-1) != '/'))
        p--;
      (void) FormatLocaleString(window_name,MagickPathExtent,
        "%s: %s[%.17g of %.17g]",MagickPackageName,p,(double) scene+1,
        (double) number_scenes);
      (void) CloneString(&windows->image.name,window_name);
    }
}

static void XStoreSceneWindow(const AnimateSceneWindow *scene_window,
  XEvent *event)
{
  Display
    *const display = scene_window->display;

  XWindows
    *const windows = scene_window->windows;

  Image
    **const image_list = scene_window->image_list;

  const ssize_t
    scene = scene_window->scene;

  int
    status;

  XTextProperty
    window_name;

  status=XStringListToTextProperty(&windows->image.name,1,&window_name);
  if (status != Success)
    {
      XSetWMName(display,windows->image.id,&window_name);
      (void) XFree((void *) window_name.value);
    }
  windows->image.pixmaps[scene]=windows->image.pixmap;
  windows->image.matte_pixmaps[scene]=windows->image.matte_pixmap;
  if (scene == 0)
    {
      (*event).xexpose.x=0;
      (*event).xexpose.y=0;
      (*event).xexpose.width=(int) image_list[scene]->columns;
      (*event).xexpose.height=(int) image_list[scene]->rows;
      XRefreshWindow(display,&windows->image,event);
      (void) XSync(display,MagickFalse);
  }
}

static void XCountSceneColors(Image *images,XResourceInfo *resource_info)
{
  Image
    *p;

  for (p=images; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    if (p->storage_class == DirectClass)
      {
        resource_info->colors=0;
        break;
      }
    if (p->colors > resource_info->colors)
      resource_info->colors=p->colors;
  }
}

static void XClearMagickWindowIds(XWindowInfo **magick_windows,
  const size_t number_windows)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) number_windows; i++)
    magick_windows[i]->id=(Window) NULL;
}

typedef struct _AnimateCommandWidget
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  ExceptionInfo
    *exception;

  const char
    *const *CommandMenu;

  const AnimateCommand
    *CommandMenus;

  const char
    *const **Menus;

  const AnimateCommand
    **Commands;

  char
    *command;
} AnimateCommandWidget;

typedef struct _AnimateCommandTargets
{
  XEvent
    *event;

  Image
    **image;

  MagickStatusType
    *state;

  Image
    **nexus;
} AnimateCommandTargets;

static void XRunCommandWidget(const AnimateCommandWidget *command_widget,
  const AnimateCommandTargets *targets)
{
  XEvent
    *const event = targets->event;

  Image
    **const image = targets->image;

  MagickStatusType
    *const state = targets->state;

  Image
    **const nexus = targets->nexus;

  AnimateCommand
    animate_command;

  AnimateCommandTile
    tile;

  AnimateMenuSelection
    menu_selection;

  int
    id;

  id=XCommandWidget(command_widget->display,command_widget->windows,
    command_widget->CommandMenu,&(*event));
  if (id < 0)
    return;
  (void) CopyMagickString(command_widget->command,
    command_widget->CommandMenu[id],MagickPathExtent);
  animate_command=command_widget->CommandMenus[id];
  menu_selection.display=command_widget->display;
  menu_selection.windows=command_widget->windows;
  menu_selection.CommandMenu=command_widget->CommandMenu;
  menu_selection.Menus=command_widget->Menus;
  menu_selection.Commands=command_widget->Commands;
  menu_selection.id=id;
  if (XSelectMenuCommand(&menu_selection,command_widget->command,
      &animate_command) == MagickFalse)
    return;
  if (animate_command != NullCommand)
    {
      tile.display=command_widget->display;
      tile.resource_info=command_widget->resource_info;
      tile.windows=command_widget->windows;
      tile.image=image;
      tile.state=state;
      tile.exception=command_widget->exception;
      (*nexus)=XMagickCommand(&tile,animate_command);
    }
}

static void XHandleConfigureNotify(Display *display,
  XResourceInfo *resource_info,XWindows *windows,const XEvent event)
{
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Configure Notify: 0x%lx %dx%d+%d+%d %d",event.xconfigure.window,
      event.xconfigure.width,event.xconfigure.height,event.xconfigure.x,
      event.xconfigure.y,event.xconfigure.send_event);
  if (event.xconfigure.window == windows->image.id)
    {
      XPlaceTransientWindows(display,windows,event);
      /*
        Image window has a new configuration.
      */
      windows->image.width=(unsigned int) event.xconfigure.width;
      windows->image.height=(unsigned int) event.xconfigure.height;
      return;
    }
  if (event.xconfigure.window == windows->icon.id)
    {
      /*
        Icon window has a new configuration.
      */
      windows->icon.width=(unsigned int) event.xconfigure.width;
      windows->icon.height=(unsigned int) event.xconfigure.height;
      return;
    }
}

static MagickStatusType XHandleDestroyNotify(XResourceInfo *resource_info,
  XWindows *windows,const XEvent event,MagickStatusType state)
{
  /*
    Group leader has exited.
  */
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Destroy Notify: 0x%lx",event.xdestroywindow.window);
  if (event.xdestroywindow.window == windows->group_leader.id)
    {
      state|=ExitState;
      return(state);
    }
  return(state);
}

static void XRepaintExposedWindows(Display *display,XWindows *windows,
  const ssize_t scene,const XEvent event)
{
  /*
    Repaint windows that are now exposed.
  */
  if (event.xexpose.window == windows->image.id)
    {
      windows->image.pixmap=windows->image.pixmaps[scene];
      windows->image.matte_pixmap=windows->image.matte_pixmaps[scene];
      XRefreshWindow(display,&windows->image,&event);
      return;
    }
  if ((event.xexpose.window == windows->icon.id) &&
      (event.xexpose.count == 0))
    {
      XRefreshWindow(display,&windows->icon,&event);
      return;
    }
}

static void XUnmapPanelWindows(Display *display,XWindows *windows,
  const XEvent event)
{
  if (event.xunmap.window == windows->command.id)
    {
      windows->command.mapped=MagickFalse;
      return;
    }
  if (event.xunmap.window == windows->popup.id)
    {
      XFocusBackdropImage(display,windows);
      windows->popup.mapped=MagickFalse;
      return;
    }
  if (event.xunmap.window == windows->widget.id)
    {
      XFocusBackdropImage(display,windows);
      windows->widget.mapped=MagickFalse;
      return;
    }
}

static void XMapPanelWindows(XWindows *windows,const XEvent event)
{
  if (event.xmap.window == windows->command.id)
    {
      windows->command.mapped=MagickTrue;
      return;
    }
  if (event.xmap.window == windows->popup.id)
    {
      windows->popup.mapped=MagickTrue;
      return;
    }
  if (event.xmap.window == windows->widget.id)
    {
      windows->widget.mapped=MagickTrue;
      return;
    }
}

typedef struct _AnimateEventContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  XVisualInfo
    *visual_info;

  XStandardColormap
    *map_info;

  Image
    **image_list;

  Image
    *display_image;

  ExceptionInfo
    *exception;

  XResourceInfo
    *icon_resources;

  XVisualInfo
    *icon_visual;

  XStandardColormap
    *icon_map;

  XPixelInfo
    *icon_pixel;

  const char
    *const *CommandMenu;

  XPixelInfo
    *pixel;

  MagickStatusType
    context_mask;

  Window
    root_window;

  XWindowInfo
    **magick_windows;

  size_t
    number_windows;

  size_t
    number_scenes;
} AnimateEventContext;

static void XHandleUnmapNotify(const AnimateEventContext *event_context,
  const XEvent event)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  XStandardColormap
    *const map_info = event_context->map_info;

  Image
    *const display_image = event_context->display_image;

  ExceptionInfo
    *const exception = event_context->exception;

  XVisualInfo
    *const icon_visual = event_context->icon_visual;

  XStandardColormap
    *const icon_map = event_context->icon_map;

  XPixelInfo
    *const icon_pixel = event_context->icon_pixel;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Unmap Notify: 0x%lx",event.xunmap.window);
  if (event.xunmap.window == windows->backdrop.id)
    {
      windows->backdrop.mapped=MagickFalse;
      return;
    }
  if (event.xunmap.window == windows->image.id)
    {
      windows->image.mapped=MagickFalse;
      return;
    }
  if (event.xunmap.window == windows->info.id)
    {
      windows->info.mapped=MagickFalse;
      return;
    }
  if (event.xunmap.window == windows->icon.id)
    {
      AnimateIconColormaps
        icon_colormaps;

      icon_colormaps.display=display;
      icon_colormaps.resource_info=resource_info;
      icon_colormaps.windows=windows;
      icon_colormaps.display_image=display_image;
      icon_colormaps.exception=exception;
      icon_colormaps.map_info=map_info;
      icon_colormaps.icon_map=icon_map;
      icon_colormaps.icon_visual=icon_visual;
      icon_colormaps.icon_pixel=icon_pixel;
      XReleaseIconColormap(&icon_colormaps);
      return;
    }
  XUnmapPanelWindows(display,windows,event);
}

static void XHandleMapNotify(const AnimateEventContext *event_context,
  const XEvent event,const AnimateCommandTargets *targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  Image
    **const image = targets->image;

  MagickStatusType
    *const state = targets->state;

  Image
    **const nexus = targets->nexus;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Map Notify: 0x%lx",
      event.xmap.window);
  if (event.xmap.window == windows->backdrop.id)
    {
      (void) XSetInputFocus(display,event.xmap.window,RevertToParent,
        CurrentTime);
      windows->backdrop.mapped=MagickTrue;
      return;
    }
  if (event.xmap.window == windows->image.id)
    {
      AnimateImageWindow
        image_window;

      image_window.display=display;
      image_window.resource_info=resource_info;
      image_window.windows=windows;
      image_window.map_info=event_context->map_info;
      image_window.image_list=event_context->image_list;
      image_window.display_image=event_context->display_image;
      image_window.exception=event_context->exception;
      XMapAnimateImageWindow(&image_window,nexus,image,state);
      return;
    }
  if (event.xmap.window == windows->info.id)
    {
      windows->info.mapped=MagickTrue;
      return;
    }
  if (event.xmap.window == windows->icon.id)
    {
      /*
        Create an icon image.
      */
      XMakeStandardColormap(display,event_context->icon_visual,
        event_context->icon_resources,event_context->display_image,
        event_context->icon_map,event_context->icon_pixel,
        event_context->exception);
      (void) XMakeImage(display,event_context->icon_resources,&windows->icon,
        event_context->display_image,windows->icon.width,windows->icon.height,
        event_context->exception);
      (void) XSetWindowBackgroundPixmap(display,windows->icon.id,
        windows->icon.pixmap);
      (void) XClearWindow(display,windows->icon.id);
      (void) XWithdrawWindow(display,windows->info.id,
        windows->info.screen);
      windows->icon.mapped=MagickTrue;
      return;
    }
  XMapPanelWindows(windows,event);
}

static void XHandlePropertyNotify(const AnimateEventContext *event_context,
  const XEvent event,const AnimateCommandTargets *targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  ExceptionInfo
    *const exception = event_context->exception;

  MagickStatusType
    *const state = targets->state;

  Image
    **const nexus = targets->nexus;

  Atom
    type;

  int
    format,
    status;

  unsigned char
    *data;

  unsigned long
    after,
    length;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Property Notify: 0x%lx 0x%lx %d",(unsigned long)
      event.xproperty.window,(unsigned long) event.xproperty.atom,
      event.xproperty.state);
  if (event.xproperty.atom != windows->im_remote_command)
    return;
  /*
    Display image named by the remote command protocol.
  */
  status=XGetWindowProperty(display,event.xproperty.window,
    event.xproperty.atom,0L,(long) MagickPathExtent,MagickFalse,(Atom)
    AnyPropertyType,&type,&format,&length,&after,&data);
  if ((status != Success) || (length == 0))
    return;
  (void) CopyMagickString(resource_info->image_info->filename,
    (char *) data,MagickPathExtent);
  (*nexus)=ReadImage(resource_info->image_info,exception);
  CatchException(exception);
  if ((*nexus) != (Image *) NULL)
    (*state)|=ExitState;
  (void) XFree((void *) data);
}

static MagickStatusType XHandleDeleteMessage(
  const AnimateEventContext *event_context,const XEvent event,
  MagickStatusType state)
{
  Display
    *const display = event_context->display;

  XWindows
    *const windows = event_context->windows;

  XVisualInfo
    *const visual_info = event_context->visual_info;

  /*
    If client window delete message, exit.
  */
  if (event.xclient.message_type != windows->wm_protocols)
    return(state);
  if (*event.xclient.data.l == (long) windows->wm_take_focus)
    {
      (void) XSetInputFocus(display,event.xclient.window,RevertToParent,
        (Time) event.xclient.data.l[1]);
      return(state);
    }
  if (*event.xclient.data.l != (long) windows->wm_delete_window)
    return(state);
  (void) XWithdrawWindow(display,event.xclient.window,
    visual_info->screen);
  if (event.xclient.window == windows->image.id)
    {
      state|=ExitState;
      return(state);
    }
  return(state);
}

static void XHelpKeyCommand(const KeySym key_symbol,
  AnimateCommand *animate_command)
{
  switch (key_symbol)
  {
    case XK_F1:
    {
      (*animate_command)=HelpCommand;
      break;
    }
    case XK_Find:
    {
      (*animate_command)=BrowseDocumentationCommand;
      break;
    }
    case XK_question:
    {
      (*animate_command)=InfoCommand;
      break;
    }
    case XK_q:
    case XK_Escape:
    {
      (*animate_command)=QuitCommand;
      break;
    }
    default:
      break;
  }
}

static void XPlaybackKeyCommand(const KeySym key_symbol,
  AnimateCommand *animate_command)
{
  switch (key_symbol)
  {
    case XK_BackSpace:
    {
      (*animate_command)=StepBackwardCommand;
      break;
    }
    case XK_space:
    {
      (*animate_command)=StepForwardCommand;
      break;
    }
    case XK_less:
    {
      (*animate_command)=FasterCommand;
      break;
    }
    case XK_greater:
    {
      (*animate_command)=SlowerCommand;
      break;
    }
    default:
    {
      XHelpKeyCommand(key_symbol,&(*animate_command));
      break;
    }
  }
}

static void XHandleKeyPress(const AnimateEventContext *event_context,
  XEvent event,const AnimateCommandTargets *targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  ExceptionInfo
    *const exception = event_context->exception;

  Image
    **const image = targets->image;

  MagickStatusType
    *const state = targets->state;

  Image
    **const nexus = targets->nexus;

  const AnimateCommandTile
    tile = { display, resource_info, windows, image, state, exception };

  AnimateCommand
    animate_command;

  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  static int
    length;

  /*
    Respond to a user key press.
  */
  length=XLookupString((XKeyEvent *) &event.xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  *(command+length)='\0';
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Key press: 0x%lx (%c)",(unsigned long) key_symbol,*command);
  animate_command=NullCommand;
  switch (key_symbol)
  {
    case XK_o:
    {
      animate_command=XControlOpenCommand(event,animate_command);
      break;
    }
    default:
    {
      XPlaybackKeyCommand(key_symbol,&animate_command);
      break;
    }
  }
  if (animate_command != NullCommand)
    (*nexus)=XMagickCommand(&tile,animate_command);
}

static void XLogButtonRelease(XResourceInfo *resource_info,const XEvent event)
{
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Button Release: 0x%lx %u +%d+%d",event.xbutton.window,
      event.xbutton.button,event.xbutton.x,event.xbutton.y);
}

static void XLogReparentNotify(XResourceInfo *resource_info,const XEvent event)
{
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Reparent Notify: 0x%lx=>0x%lx",event.xreparent.parent,
      event.xreparent.window);
}

static void XLogUnknownEvent(XResourceInfo *resource_info,const XEvent event)
{
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Event type: %d",
      event.type);
}

static void XHandleKeyRelease(XResourceInfo *resource_info,XEvent event)
{
  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  /*
    Respond to a user key release.
  */
  (void) XLookupString((XKeyEvent *) &event.xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Key release: 0x%lx (%c)",(unsigned long) key_symbol,*command);
}

static void XHandleExpose(const AnimateEventContext *event_context,
  const XEvent event,const ssize_t scene)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Expose: 0x%lx %dx%d+%d+%d",event.xexpose.window,
      event.xexpose.width,event.xexpose.height,event.xexpose.x,
      event.xexpose.y);
  XRepaintExposedWindows(display,windows,scene,event);
}

static void XHandleButtonPress(const AnimateEventContext *event_context,
  const AnimateCommandTargets *targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  XEvent
    *const event = targets->event;

  MagickStatusType
    *const state = targets->state;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Button Press: 0x%lx %u +%d+%d",(*event).xbutton.window,
      (*event).xbutton.button,(*event).xbutton.x,(*event).xbutton.y);
  if (((*event).xbutton.button == Button3) &&
      ((*event).xbutton.state & Mod1Mask))
    {
      /*
        Convert Alt-Button3 to Button2.
      */
      (*event).xbutton.button=Button2;
      (*event).xbutton.state&=(~(unsigned int) Mod1Mask);
    }
  if ((*event).xbutton.window == windows->backdrop.id)
    {
      (void) XSetInputFocus(display,(*event).xbutton.window,RevertToParent,
        (*event).xbutton.time);
      return;
    }
  if ((*event).xbutton.window == windows->image.id)
    {
      AnimateCommandWindow
        command_window;

      command_window.display=display;
      command_window.resource_info=resource_info;
      command_window.windows=windows;
      command_window.CommandMenu=event_context->CommandMenu;
      (*state)=XPressImageWindow(&command_window,(*state));
    }
}

typedef struct _AnimateClientTargets
{
  MagickStatusType
    *state;

  XGCValues
    *context_values;

  Image
    **nexus;
} AnimateClientTargets;

static void XHandleClientMessage(const AnimateEventContext *event_context,
  const XEvent event,const AnimateClientTargets *client_targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  ExceptionInfo
    *const exception = event_context->exception;

  MagickStatusType
    *const state = client_targets->state;

  XGCValues
    *const context_values = client_targets->context_values;

  Image
    **const nexus = client_targets->nexus;

  AnimateDropSource
    drop_source;

  AnimateDropTargets
    targets;

  AnimateProtocolTargets
    protocol_targets;

  AnimateWindowColormaps
    window_colormaps;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Client Message: 0x%lx 0x%lx %d 0x%lx",(unsigned long)
      event.xclient.window,(unsigned long) event.xclient.message_type,
      event.xclient.format,(unsigned long) event.xclient.data.l[0]);
  window_colormaps.display=display;
  window_colormaps.windows=windows;
  window_colormaps.map_info=event_context->map_info;
  window_colormaps.pixel=event_context->pixel;
  window_colormaps.magick_windows=event_context->magick_windows;
  window_colormaps.number_windows=event_context->number_windows;
  window_colormaps.context_mask=event_context->context_mask;
  window_colormaps.event=event;
  protocol_targets.state=(state);
  protocol_targets.context_values=(context_values);
  if (XHandleMagickProtocol(&window_colormaps,&protocol_targets) ==
      MagickFalse)
    return;
  drop_source.display=display;
  drop_source.root_window=event_context->root_window;
  drop_source.resource_info=resource_info;
  drop_source.exception=exception;
  drop_source.event=event;
  drop_source.windows=windows;
  targets.nexus=(nexus);
  targets.state=(state);
  if (XHandleDropProtocol(&drop_source,&targets) == MagickFalse)
    return;
  (*state)=XHandleDeleteMessage(event_context,event,(*state));
}

static void XHandleEnterNotify(const AnimateEventContext *event_context,
  const XEvent event)
{
  Display
    *const display = event_context->display;

  XVisualInfo
    *const visual_info = event_context->visual_info;

  XStandardColormap
    *const map_info = event_context->map_info;

  /*
    Selectively install colormap.
  */
  if ((map_info->colormap !=
       XDefaultColormap(display,visual_info->screen)) &&
      (event.xcrossing.mode != NotifyUngrab))
    XInstallColormap(display,map_info->colormap);
}

static void XLogDisplayImage(XResourceInfo *resource_info,
  const Image *display_image)
{
  if (resource_info->debug != MagickFalse)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "Image: %s[%.17g] %.17gx%.17g ",display_image->filename,(double)
        display_image->scene,(double) display_image->columns,(double)
        display_image->rows);
      if (display_image->colors != 0)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"%.17gc ",(double)
          display_image->colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"%s",
        display_image->magick);
    }
}

static void XLogSceneImage(Image **image_list,const ssize_t scene,
  const unsigned int columns,const unsigned int rows)
{
  if (image_list[scene]->debug != MagickFalse)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "Image: [%.17g] %s %.17gx%.17g ",(double) image_list[scene]->scene,
        image_list[scene]->filename,(double) columns,(double) rows);
      if (image_list[scene]->colors != 0)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"%.17gc ",(double)
          image_list[scene]->colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"%s",
        image_list[scene]->magick);
    }
}

static void XCreateSceneWindows(const AnimateEventContext *event_context,
  const Image *images,XEvent *event)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  XVisualInfo
    *const visual_info = event_context->visual_info;

  XStandardColormap
    *const map_info = event_context->map_info;

  Image
    **const image_list = event_context->image_list;

  ExceptionInfo
    *const exception = event_context->exception;

  const size_t
    number_scenes = event_context->number_scenes;

  int
    status;

  ssize_t
    scene;

  for (scene=0; scene < (ssize_t) number_scenes; scene++)
  {
    AnimateSceneWindow
      scene_window;

    unsigned int
      columns,
      rows;

    /*
      Create X image.
    */
    windows->image.pixmap=(Pixmap) NULL;
    windows->image.matte_pixmap=(Pixmap) NULL;
    if ((IsTrueColorMap(resource_info->map_type,visual_info) != MagickFalse) &&
        (image_list[scene]->storage_class == PseudoClass))
      XGetPixelInfo(display,visual_info,map_info,resource_info,
        image_list[scene],windows->image.pixel_info);
    columns=(unsigned int) image_list[scene]->columns;
    rows=(unsigned int) image_list[scene]->rows;
    if ((image_list[scene]->columns != columns) ||
        (image_list[scene]->rows != rows))
      ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
        image_list[scene]->filename);
    status=XMakeImage(display,resource_info,&windows->image,image_list[scene],
      columns,rows,exception) == MagickFalse ? 0 : 1;
    if (status == MagickFalse)
      ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
        images->filename);
    XLogSceneImage(image_list,scene,columns,rows);
    /*
      Window name is the base of the filename.
    */
    scene_window.display=display;
    scene_window.resource_info=resource_info;
    scene_window.windows=windows;
    scene_window.image_list=image_list;
    scene_window.number_scenes=number_scenes;
    scene_window.exception=exception;
    scene_window.scene=scene;
    XNameSceneWindow(&scene_window);
    XStoreSceneWindow(&scene_window,event);
  }
}

static WarningHandler XEnterWorkingDirectory(XResourceInfo *resource_info,
  char *working_directory,ExceptionInfo *exception)
{
  WarningHandler
    warning_handler;

  int
    status;

  if (*working_directory == '\0')
    (void) CopyMagickString(working_directory,".",MagickPathExtent);
  status=chdir(working_directory);
  if (status == -1)
    (void) ThrowMagickException(exception,GetMagickModule(),FileOpenError,
      "UnableToOpenFile","%s",working_directory);
  warning_handler=resource_info->display_warnings ?
    SetErrorHandler(XWarning) : SetErrorHandler((ErrorHandler) NULL);
  warning_handler=resource_info->display_warnings ?
    SetWarningHandler(XWarning) : SetWarningHandler((WarningHandler) NULL);
  return(warning_handler);
}

static const char
  *const CommandMenu[]=
  {
    "Animate",
    "Speed",
    "Direction",
    "Help",
    "Image Info",
    "Quit",
    (char *) NULL
  },
  *const AnimateMenu[]=
  {
    "Open...",
    "Play",
    "Step",
    "Repeat",
    "Auto Reverse",
    "Save...",
    (char *) NULL
  },
  *const SpeedMenu[]=
  {
    "Faster",
    "Slower",
    (char *) NULL
  },
  *const DirectionMenu[]=
  {
    "Forward",
    "Reverse",
    (char *) NULL
  },
  *const HelpMenu[]=
  {
    "Overview",
    "Browse Documentation",
    "About Animate",
    (char *) NULL
  };

static const char
  *const *Menus[MagickMenus]=
  {
    AnimateMenu,
    SpeedMenu,
    DirectionMenu,
    HelpMenu
  };

static const AnimateCommand
  CommandMenus[]=
  {
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    InfoCommand,
    QuitCommand
  },
  AnimateCommands[]=
  {
    OpenCommand,
    PlayCommand,
    StepCommand,
    RepeatCommand,
    AutoReverseCommand,
    SaveCommand
  },
  SpeedCommands[]=
  {
    FasterCommand,
    SlowerCommand
  },
  DirectionCommands[]=
  {
    ForwardCommand,
    ReverseCommand
  },
  HelpCommands[]=
  {
    HelpCommand,
    BrowseDocumentationCommand,
    VersionCommand
  };

static const AnimateCommand
  *Commands[MagickMenus]=
  {
    AnimateCommands,
    SpeedCommands,
    DirectionCommands,
    HelpCommands
  };

static XWindows *XCreateAnimateWindows(Display *display,
  XResourceInfo *resource_info,Image *images)
{
  XWindows
    *windows;

  /*
    Initialize window structure.
  */
  XCountSceneColors(images,resource_info);
  windows=XSetWindows(XInitializeWindows(display,resource_info));
  if (windows == (XWindows *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"MemoryAllocationFailed",
      images->filename);
  return(windows);
}

static void XLoadAnimateFont(Display *display,XResourceInfo *resource_info,
  XWindows *windows)
{
  /*
    Initialize font info.
  */
  if (windows->font_info != (XFontStruct *) NULL)
    (void) XFreeFont(display,windows->font_info);
  windows->font_info=XBestFont(display,resource_info,MagickFalse);
  if (windows->font_info == (XFontStruct *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToLoadFont",
      resource_info->font);
}

static Image *XCoalesceAnimateImages(Image *images,ExceptionInfo *exception)
{
  Image
    *coalesce_image;

  coalesce_image=CoalesceImages(images,exception);
  if (coalesce_image == (Image *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"MemoryAllocationFailed",
      images->filename);
  images=coalesce_image;
  return(images);
}

static Image **XSortAnimateScenes(Image *images,const size_t number_scenes,
  ExceptionInfo *exception)
{
  Image
    **image_list;

  image_list=ImageListToArray(images,exception);
  if (image_list == (Image **) NULL)
    ThrowXWindowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      images->filename);
  XSortSceneList(image_list,number_scenes);
  return(image_list);
}

typedef struct _AnimateWindowSetup
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  XVisualInfo
    *visual_info;

  XStandardColormap
    *map_info;

  XPixelInfo
    *pixel;

  XFontStruct
    *font_info;

  Window
    root_window;

  XClassHint
    *class_hints;

  XWMHints
    *manager_hints;

  char
    **argv;

  int
    argc;

  XResourceInfo
    *icon_resources;

  XVisualInfo
    *icon_visual;

  XStandardColormap
    *icon_map;

  XPixelInfo
    *icon_pixel;

  Image
    *display_image;

  size_t
    number_scenes;
} AnimateWindowSetup;

static void XCreateContextWindow(const AnimateWindowSetup *setup)
{
  Display
    *const display = setup->display;

  XResourceInfo
    *const resource_info = setup->resource_info;

  XWindows
    *const windows = setup->windows;

  XVisualInfo
    *const visual_info = setup->visual_info;

  XStandardColormap
    *const map_info = setup->map_info;

  XPixelInfo
    *const pixel = setup->pixel;

  XFontStruct
    *const font_info = setup->font_info;

  const Window
    root_window = setup->root_window;

  XClassHint
    *const class_hints = setup->class_hints;

  XWMHints
    *const manager_hints = setup->manager_hints;

  char
    **const argv = setup->argv;

  const int
    argc = setup->argc;

  /*
    Initialize graphic context.
  */
  windows->context.id=(Window) NULL;
  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->context);
  (void) CloneString(&class_hints->res_name,resource_info->client_name);
  (void) CloneString(&class_hints->res_class,resource_info->client_name);
  class_hints->res_class[0]=(char) LocaleToUppercase((int)
    class_hints->res_class[0]);
  manager_hints->flags=InputHint | StateHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=WithdrawnState;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->context);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (context)",windows->context.id);
}

static void XCreateIconWindow(const AnimateWindowSetup *setup)
{
  Display
    *const display = setup->display;

  XResourceInfo
    *const resource_info = setup->resource_info;

  XWindows
    *const windows = setup->windows;

  const Window
    root_window = setup->root_window;

  XClassHint
    *const class_hints = setup->class_hints;

  XWMHints
    *const manager_hints = setup->manager_hints;

  char
    **const argv = setup->argv;

  const int
    argc = setup->argc;

  XResourceInfo
    *const icon_resources = setup->icon_resources;

  XVisualInfo
    *const icon_visual = setup->icon_visual;

  XStandardColormap
    *const icon_map = setup->icon_map;

  XPixelInfo
    *const icon_pixel = setup->icon_pixel;

  Image
    *const display_image = setup->display_image;

  /*
    Initialize icon window.
  */
  XGetWindowInfo(display,icon_visual,icon_map,icon_pixel,(XFontStruct *) NULL,
    icon_resources,&windows->icon);
  windows->icon.geometry=resource_info->icon_geometry;
  XBestIconSize(display,&windows->icon,display_image);
  windows->icon.attributes.colormap=
    XDefaultColormap(display,icon_visual->screen);
  windows->icon.attributes.event_mask=ExposureMask | StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=IconicState;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->icon);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (icon)",
      windows->icon.id);
}

static void XCreateInfoWindow(const AnimateWindowSetup *setup)
{
  Display
    *const display = setup->display;

  XResourceInfo
    *const resource_info = setup->resource_info;

  XWindows
    *const windows = setup->windows;

  XVisualInfo
    *const visual_info = setup->visual_info;

  XStandardColormap
    *const map_info = setup->map_info;

  XPixelInfo
    *const pixel = setup->pixel;

  XFontStruct
    *const font_info = setup->font_info;

  XClassHint
    *const class_hints = setup->class_hints;

  XWMHints
    *const manager_hints = setup->manager_hints;

  char
    **const argv = setup->argv;

  const int
    argc = setup->argc;

  /*
    Initialize Info widget.
  */
  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->info);
  (void) CloneString(&windows->info.name,"Info");
  (void) CloneString(&windows->info.icon_name,"Info");
  windows->info.border_width=1;
  windows->info.x=2;
  windows->info.y=2;
  windows->info.flags|=PPosition;
  windows->info.attributes.win_gravity=UnmapGravity;
  windows->info.attributes.event_mask=ButtonPressMask | ExposureMask |
    StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,windows->image.id,argv,argc,class_hints,manager_hints,
    &windows->info);
  windows->info.highlight_stipple=XCreateBitmapFromData(display,
    windows->info.id,(char *) HighlightBitmap,HighlightWidth,HighlightHeight);
  windows->info.shadow_stipple=XCreateBitmapFromData(display,
    windows->info.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->info.id,windows->image.id);
  if (windows->image.mapped)
    (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (info)",
      windows->info.id);
}

static void XCreateCommandWindow(const AnimateWindowSetup *setup)
{
  Display
    *const display = setup->display;

  XResourceInfo
    *const resource_info = setup->resource_info;

  XWindows
    *const windows = setup->windows;

  XVisualInfo
    *const visual_info = setup->visual_info;

  XStandardColormap
    *const map_info = setup->map_info;

  XPixelInfo
    *const pixel = setup->pixel;

  XFontStruct
    *const font_info = setup->font_info;

  const Window
    root_window = setup->root_window;

  XClassHint
    *const class_hints = setup->class_hints;

  XWMHints
    *const manager_hints = setup->manager_hints;

  char
    **const argv = setup->argv;

  const int
    argc = setup->argc;

  char
    resource_name[MagickPathExtent];

  /*
    Initialize Command widget.
  */
  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->command);
  windows->command.data=MagickMenus;
  (void) XCommandWidget(display,windows,CommandMenu,(XEvent *) NULL);
  (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.command",
    resource_info->client_name);
  windows->command.geometry=XGetResourceClass(resource_info->resource_database,
    resource_name,"geometry",(char *) NULL);
  (void) CloneString(&windows->command.name,MagickTitle);
  windows->command.border_width=0;
  windows->command.flags|=PPosition;
  windows->command.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | LeaveWindowMask |
    OwnerGrabButtonMask | StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->command);
  windows->command.highlight_stipple=XCreateBitmapFromData(display,
    windows->command.id,(char *) HighlightBitmap,HighlightWidth,
    HighlightHeight);
  windows->command.shadow_stipple=XCreateBitmapFromData(display,
    windows->command.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->command.id,windows->image.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (command)",windows->command.id);
}

static void XChangeToHomeDirectory(XResourceInfo *resource_info,
  char *working_directory,ExceptionInfo *exception)
{
  char
    *directory;

  int
    status;

  directory=getcwd(working_directory,MagickPathExtent);
  (void) directory;
  if (*resource_info->home_directory == '\0')
    (void) CopyMagickString(resource_info->home_directory,".",MagickPathExtent);
  status=chdir(resource_info->home_directory);
  if (status == -1)
    (void) ThrowMagickException(exception,GetMagickModule(),FileOpenError,
      "UnableToOpenFile","%s",resource_info->home_directory);
}

static void XReleaseAnimateWindows(const AnimateWindowSetup *setup,
  const Image *nexus)
{
  Display
    *const display = setup->display;

  XResourceInfo
    *const resource_info = setup->resource_info;

  XWindows
    *const windows = setup->windows;

  XVisualInfo
    *const visual_info = setup->visual_info;

  XStandardColormap
    *const map_info = setup->map_info;

  XPixelInfo
    *const pixel = setup->pixel;

  XVisualInfo
    *const icon_visual = setup->icon_visual;

  XStandardColormap
    *const icon_map = setup->icon_map;

  XPixelInfo
    *const icon_pixel = setup->icon_pixel;

  if (nexus == (Image *) NULL)
    {
      AnimateX11Resources
        x11_resources;

      x11_resources.display=display;
      x11_resources.resource_info=resource_info;
      x11_resources.windows=windows;
      x11_resources.visual_info=visual_info;
      x11_resources.map_info=map_info;
      x11_resources.pixel=pixel;
      x11_resources.icon_visual=icon_visual;
      x11_resources.icon_map=icon_map;
      x11_resources.icon_pixel=icon_pixel;
      XReleaseAnimateResources(&x11_resources);
    }
}

static void XWithdrawBackdropWindows(Display *display,
  XResourceInfo *resource_info,XWindows *windows)
{
  if ((resource_info->backdrop == MagickFalse) &&
      (windows->backdrop.mapped))
    {
      (void) XWithdrawWindow(display,windows->backdrop.id,\
        windows->backdrop.screen);
      (void) XDestroyWindow(display,windows->backdrop.id);
      windows->backdrop.id=(Window) NULL;
      (void) XWithdrawWindow(display,windows->image.id,windows->image.screen);
      (void) XDestroyWindow(display,windows->image.id);
      windows->image.id=(Window) NULL;
    }
}

static void XCreateGraphicContexts(const AnimateWindowSetup *setup,
  const char *filename,XGCValues *context_values,
  MagickStatusType *context_mask)
{
  Display
    *const display = setup->display;

  XWindows
    *const windows = setup->windows;

  XPixelInfo
    *const pixel = setup->pixel;

  XFontStruct
    *const font_info = setup->font_info;

  /*
    Initialize graphic context.
  */
  (*context_values).background=pixel->background_color.pixel;
  (*context_values).font=font_info->fid;
  (*context_values).foreground=pixel->foreground_color.pixel;
  (*context_values).graphics_exposures=MagickFalse;
  *context_mask=(MagickStatusType)
    (GCBackground | GCFont | GCForeground | GCGraphicsExposures);
  if (pixel->annotate_context != (GC) NULL)
    (void) XFreeGC(display,pixel->annotate_context);
  pixel->annotate_context=
    XCreateGC(display,windows->context.id,*context_mask,context_values);
  if (pixel->annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      filename);
  (*context_values).background=pixel->depth_color.pixel;
  if (pixel->widget_context != (GC) NULL)
    (void) XFreeGC(display,pixel->widget_context);
  pixel->widget_context=
    XCreateGC(display,windows->context.id,*context_mask,context_values);
  if (pixel->widget_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      filename);
  (*context_values).background=pixel->foreground_color.pixel;
  (*context_values).foreground=pixel->background_color.pixel;
  (*context_values).plane_mask=
    (*context_values).background ^ (*context_values).foreground;
  if (pixel->highlight_context != (GC) NULL)
    (void) XFreeGC(display,pixel->highlight_context);
  pixel->highlight_context=XCreateGC(display,windows->context.id,
    (size_t) (*context_mask | GCPlaneMask),context_values);
  if (pixel->highlight_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      filename);
  (void) XDestroyWindow(display,windows->context.id);
}

static void XCreateIconContext(const AnimateWindowSetup *setup,
  const char *filename,XGCValues *context_values)
{
  Display
    *const display = setup->display;

  XWindows
    *const windows = setup->windows;

  XPixelInfo
    *const icon_pixel = setup->icon_pixel;

  if (icon_pixel->annotate_context != (GC) NULL)
    (void) XFreeGC(display,icon_pixel->annotate_context);
  (*context_values).background=icon_pixel->background_color.pixel;
  (*context_values).foreground=icon_pixel->foreground_color.pixel;
  icon_pixel->annotate_context=XCreateGC(display,windows->icon.id,
    (size_t) (GCBackground | GCForeground),context_values);
  if (icon_pixel->annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      filename);
  windows->icon.annotate_context=icon_pixel->annotate_context;
}

static void XCreateImageWindowInfo(const AnimateWindowSetup *setup,
  ExceptionInfo *exception)
{
  char
    geometry[MagickPathExtent];

  Display
    *const display = setup->display;

  XResourceInfo
    *const resource_info = setup->resource_info;

  XWindows
    *const windows = setup->windows;

  XVisualInfo
    *const visual_info = setup->visual_info;

  Image
    *const display_image = setup->display_image;

  RectangleInfo
    geometry_info;

  XGetWindowInfo(display,visual_info,setup->map_info,setup->pixel,
    setup->font_info,resource_info,&windows->image);
  windows->image.shape=MagickTrue;  /* non-rectangular shape hint */
  if (resource_info->use_shared_memory == MagickFalse)
    windows->image.shared_memory=MagickFalse;
  if (resource_info->title != (char *) NULL)
    {
      char
        *title;

      title=InterpretImageProperties(resource_info->image_info,display_image,
        resource_info->title,exception);
      (void) CloneString(&windows->image.name,title);
      (void) CloneString(&windows->image.icon_name,title);
      title=DestroyString(title);
    }
  else
    {
      char
        filename[MagickPathExtent],
        window_name[MagickPathExtent];

      /*
        Window name is the base of the filename.
      */
      GetPathComponent(display_image->magick_filename,TailPath,filename);
      (void) FormatLocaleString(window_name,MagickPathExtent,
        "%s: %s[scene: %.17g frames: %.17g]",MagickPackageName,filename,(double)
        display_image->scene,(double) setup->number_scenes);
      (void) CloneString(&windows->image.name,window_name);
      (void) CloneString(&windows->image.icon_name,filename);
    }
  if (resource_info->immutable != MagickFalse)
    windows->image.immutable=MagickTrue;
  windows->image.shape=MagickTrue;
  windows->image.geometry=resource_info->image_geometry;
  (void) FormatLocaleString(geometry,MagickPathExtent,"%ux%u+0+0>!",
    XDisplayWidth(display,visual_info->screen),
    XDisplayHeight(display,visual_info->screen));
  geometry_info.width=display_image->columns;
  geometry_info.height=display_image->rows;
  geometry_info.x=0;
  geometry_info.y=0;
  (void) ParseMetaGeometry(geometry,&geometry_info.x,&geometry_info.y,
    &geometry_info.width,&geometry_info.height);
  windows->image.width=(unsigned int) geometry_info.width;
  windows->image.height=(unsigned int) geometry_info.height;
  windows->image.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | KeyPressMask |
    KeyReleaseMask | LeaveWindowMask | OwnerGrabButtonMask |
    PropertyChangeMask | StructureNotifyMask | SubstructureNotifyMask;
}

static void XCreateBackdropWindow(const AnimateWindowSetup *setup)
{
Display
    *const display = setup->display;

XResourceInfo
    *const resource_info = setup->resource_info;

XWindows
    *const windows = setup->windows;

XVisualInfo
    *const visual_info = setup->visual_info;

const Window
    root_window = setup->root_window;

XClassHint
    *const class_hints = setup->class_hints;

XWMHints
    *const manager_hints = setup->manager_hints;

char
    **const argv = setup->argv;

const int
    argc = setup->argc;

  XGetWindowInfo(display,visual_info,setup->map_info,setup->pixel,
    setup->font_info,resource_info,&windows->backdrop);
  if ((resource_info->backdrop) || (windows->backdrop.id != (Window) NULL))
    {
      /*
        Initialize backdrop window.
      */
      windows->backdrop.x=0;
      windows->backdrop.y=0;
      (void) CloneString(&windows->backdrop.name,"ImageMagick Backdrop");
      windows->backdrop.flags=(size_t) (USSize | USPosition);
      windows->backdrop.width=(unsigned int)
        XDisplayWidth(display,visual_info->screen);
      windows->backdrop.height=(unsigned int)
        XDisplayHeight(display,visual_info->screen);
      windows->backdrop.border_width=0;
      windows->backdrop.immutable=MagickTrue;
      windows->backdrop.attributes.do_not_propagate_mask=ButtonPressMask |
        ButtonReleaseMask;
      windows->backdrop.attributes.event_mask=ButtonPressMask | KeyPressMask |
        StructureNotifyMask;
      manager_hints->flags=IconWindowHint | InputHint | StateHint;
      manager_hints->icon_window=windows->icon.id;
      manager_hints->input=MagickTrue;
      manager_hints->initial_state=
        resource_info->iconic ? IconicState : NormalState;
      XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
        &windows->backdrop);
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Window id: 0x%lx (backdrop)",windows->backdrop.id);
      XCenterImageOnBackdrop(display,windows,visual_info);
    }
}

static void XCreateImageWindow(const AnimateWindowSetup *setup)
{
Display
    *const display = setup->display;

XResourceInfo
    *const resource_info = setup->resource_info;

XWindows
    *const windows = setup->windows;

const Window
    root_window = setup->root_window;

XClassHint
    *const class_hints = setup->class_hints;

XWMHints
    *const manager_hints = setup->manager_hints;

char
    **const argv = setup->argv;

const int
    argc = setup->argc;

  manager_hints->flags=IconWindowHint | InputHint | StateHint;
  manager_hints->icon_window=windows->icon.id;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=
    resource_info->iconic ? IconicState : NormalState;
  if (windows->group_leader.id != (Window) NULL)
    {
      /*
        Follow the leader.
      */
      manager_hints->flags|=(MagickStatusType) WindowGroupHint;
      manager_hints->window_group=windows->group_leader.id;
      (void) XSelectInput(display,windows->group_leader.id,StructureNotifyMask);
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Window id: 0x%lx (group leader)",windows->group_leader.id);
    }
  XMakeWindow(display,
    (Window) (resource_info->backdrop ? windows->backdrop.id : root_window),
    argv,argc,class_hints,manager_hints,&windows->image);
  (void) XChangeProperty(display,windows->image.id,windows->im_protocols,
    XA_STRING,8,PropModeReplace,(unsigned char *) NULL,0);
  if (windows->group_leader.id != (Window) NULL)
    (void) XSetTransientForHint(display,windows->image.id,
      windows->group_leader.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (image)",
      windows->image.id);
}

static void XCreatePanelWindows(const AnimateWindowSetup *setup)
{
  char
    resource_name[MagickPathExtent];

Display
    *const display = setup->display;

XResourceInfo
    *const resource_info = setup->resource_info;

XWindows
    *const windows = setup->windows;

const Window
    root_window = setup->root_window;

XClassHint
    *const class_hints = setup->class_hints;

XWMHints
    *const manager_hints = setup->manager_hints;

char
    **const argv = setup->argv;

const int
    argc = setup->argc;

  XGetWindowInfo(display,setup->visual_info,setup->map_info,setup->pixel,
    setup->font_info,resource_info,&windows->widget);
  (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.widget",
    resource_info->client_name);
  windows->widget.geometry=XGetResourceClass(resource_info->resource_database,
    resource_name,"geometry",(char *) NULL);
  windows->widget.border_width=0;
  windows->widget.flags|=PPosition;
  windows->widget.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | KeyPressMask |
    KeyReleaseMask | LeaveWindowMask | OwnerGrabButtonMask |
    StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->widget);
  windows->widget.highlight_stipple=XCreateBitmapFromData(display,
    windows->widget.id,(char *) HighlightBitmap,HighlightWidth,HighlightHeight);
  windows->widget.shadow_stipple=XCreateBitmapFromData(display,
    windows->widget.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->widget.id,windows->image.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (widget)",windows->widget.id);
  /*
    Initialize popup window.
  */
  XGetWindowInfo(display,setup->visual_info,setup->map_info,setup->pixel,
    setup->font_info,resource_info,&windows->popup);
  windows->popup.border_width=0;
  windows->popup.flags|=PPosition;
  windows->popup.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | KeyPressMask |
    KeyReleaseMask | LeaveWindowMask | StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->popup);
  windows->popup.highlight_stipple=XCreateBitmapFromData(display,
    windows->popup.id,(char *) HighlightBitmap,HighlightWidth,HighlightHeight);
  windows->popup.shadow_stipple=XCreateBitmapFromData(display,
    windows->popup.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->popup.id,windows->image.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (pop up)",windows->popup.id);
}

static void XHandleAnimateWindowEvent(
  const AnimateEventContext *event_context,XEvent event,
  const AnimateCommandTargets *command_targets)
{
  XResourceInfo
    *const resource_info = event_context->resource_info;

  switch (event.type)
  {
    case MapNotify:
    {
      XHandleMapNotify(event_context,event,command_targets);
      break;
    }
    case MappingNotify:
    {
      (void) XRefreshKeyboardMapping(&event.xmapping);
      break;
    }
    case NoExpose:
      break;
    case PropertyNotify:
    {
      XHandlePropertyNotify(event_context,event,command_targets);
      break;
    }
    case ReparentNotify:
    {
      XLogReparentNotify(resource_info,event);
      break;
    }
    case UnmapNotify:
    {
      XHandleUnmapNotify(event_context,event);
      break;
    }
    default:
    {
      XLogUnknownEvent(resource_info,event);
      break;
    }
  }
}

static void XHandleAnimateInputEvent(
  const AnimateEventContext *event_context,XEvent event,const ssize_t scene,
  const AnimateCommandTargets *command_targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XVisualInfo
    *const visual_info = event_context->visual_info;

  XStandardColormap
    *const map_info = event_context->map_info;

  switch (event.type)
  {
    case Expose:
    {
      XHandleExpose(event_context,event,scene);
      break;
    }
    case KeyPress:
    {
      XHandleKeyPress(event_context,event,command_targets);
      break;
    }
    case KeyRelease:
    {
      XHandleKeyRelease(resource_info,event);
      break;
    }
    case LeaveNotify:
    {
      /*
        Selectively uninstall colormap.
      */
      if ((map_info->colormap !=
           XDefaultColormap(display,visual_info->screen)) &&
          (event.xcrossing.mode != NotifyUngrab))
        XUninstallColormap(display,map_info->colormap);
      break;
    }
    default:
    {
      XHandleAnimateWindowEvent(event_context,event,command_targets);
      break;
    }
  }
}

static void XCreateImagePixmaps(const AnimateWindowSetup *setup,
  const char *filename)
{
Display
    *const display = setup->display;

XWindows
    *const windows = setup->windows;

const size_t
    number_scenes = setup->number_scenes;

  XWindowChanges
    window_changes;

  windows->image.x=0;
  windows->image.y=0;
  /*
    Initialize image pixmaps structure.
  */
  window_changes.width=(int) windows->image.width;
  window_changes.height=(int) windows->image.height;
  (void) XReconfigureWMWindow(display,windows->image.id,windows->command.screen,
    (unsigned int) (CWWidth | CWHeight),&window_changes);
  windows->image.pixmaps=(Pixmap *) AcquireQuantumMemory(number_scenes,
    sizeof(*windows->image.pixmaps));
  windows->image.matte_pixmaps=(Pixmap *) AcquireQuantumMemory(number_scenes,
    sizeof(*windows->image.pixmaps));
  if ((windows->image.pixmaps == (Pixmap *) NULL) ||
      (windows->image.matte_pixmaps == (Pixmap *) NULL))
    ThrowXWindowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      filename);
  if ((windows->image.mapped == MagickFalse) ||
      (windows->backdrop.id != (Window) NULL))
    (void) XMapWindow(display,windows->image.id);
}

typedef struct _AnimatePlayTargets
{
  ssize_t
    *scene,
    *iterations;

  MagickStatusType
    *state;

  Image
    **image;

  ssize_t
    *first_scene;

  XEvent
    *event;
} AnimatePlayTargets;

static void XPlayNextScene(const AnimateEventContext *event_context,
  const AnimatePlayTargets *targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  Image
    **const image_list = event_context->image_list;

  const size_t
    number_scenes = event_context->number_scenes;

  size_t
    delay;

  ssize_t
    *const scene = targets->scene,
    *const iterations = targets->iterations,
    *const first_scene = targets->first_scene;

  MagickStatusType
    *const state = targets->state;

  Image
    **const image = targets->image;

  XEvent
    *const event = targets->event;

  AnimationTitle
    animation_title;

  MagickBooleanType
    pause;

  AnimationSceneStep
    scene_step;

  AnimateSceneView
    scene_view;

  const AnimationSceneTargets
    scene_targets = { scene, iterations, state, &pause };

  pause=MagickFalse;
  delay=1000*(*image)->delay/(size_t) MagickMax((*image)->ticks_per_second,
    1L);
  XDelay(display,resource_info->delay*(delay == 0 ? 10 : delay));
  scene_step.number_scenes=number_scenes;
  scene_step.image_list=image_list;
  scene_step.first_scene=(*first_scene);
  XAdvanceScene(&scene_step,&scene_targets);
  (*scene)=MagickMax((*scene),0);
  (*image)=image_list[(*scene)];
  (*first_scene)=XLoopStartScene((*image),(*scene),(*first_scene));
  animation_title.display=display;
  animation_title.resource_info=resource_info;
  animation_title.windows=windows;
  animation_title.image_list=image_list;
  animation_title.image=(*image);
  animation_title.exception=event_context->exception;
  animation_title.scene=(*scene);
  animation_title.number_scenes=number_scenes;
  animation_title.state=(*state);
  XUpdateAnimationTitle(&animation_title);
  /*
    Copy X pixmap to Image window.
  */
  scene_view.display=display;
  scene_view.resource_info=resource_info;
  scene_view.windows=windows;
  scene_view.visual_info=event_context->visual_info;
  scene_view.map_info=event_context->map_info;
  scene_view.image_list=image_list;
  scene_view.image=(*image);
  scene_view.scene=(*scene);
  XShowAnimationScene(&scene_view,(*state),event);
  (*state)&=(~StepAnimationState);
  XPauseAnimationScene(&scene_view,pause,event);
}

static MagickBooleanType IsAnimationStep(Display *display,
  const MagickStatusType state)
{
  return(((XEventsQueued(display,QueuedAfterFlush) == 0) &&
    (IsAnimationPlaying(state) != MagickFalse)) ? MagickTrue : MagickFalse);
}

static void XNextAnimateEvent(Display *display,XWindows *windows,
  XEvent *event)
{
  time_t
    timestamp;

  timestamp=GetMagickTime();
  (void) XNextEvent(display,event);
  if (windows->image.stasis == MagickFalse)
    windows->image.stasis=(GetMagickTime()-timestamp) > 0 ?
      MagickTrue : MagickFalse;
}

static void XHandleAnimateWindowChange(
  const AnimateEventContext *event_context,XEvent event,const ssize_t scene,
  const AnimateCommandTargets *command_targets)
{
  Display
    *const display = event_context->display;

  MagickStatusType
    *const state = command_targets->state;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  switch (event.type)
  {
    case ConfigureNotify:
    {
      XHandleConfigureNotify(display,resource_info,windows,event);
      break;
    }
    case DestroyNotify:
    {
      (*state)=XHandleDestroyNotify(resource_info,windows,event,(*state));
      break;
    }
    case EnterNotify:
    {
      XHandleEnterNotify(event_context,event);
      break;
    }
    default:
    {
      XHandleAnimateInputEvent(event_context,event,scene,command_targets);
      break;
    }
  }
}

static void XRunCommandWindow(const AnimateEventContext *event_context,
  char *command,const AnimateCommandTargets *command_targets)
{
  AnimateCommandWidget
    command_widget;

  /*
    Select a command from the Command widget.
  */
  command_widget.display=event_context->display;
  command_widget.resource_info=event_context->resource_info;
  command_widget.windows=event_context->windows;
  command_widget.exception=event_context->exception;
  command_widget.CommandMenu=CommandMenu;
  command_widget.CommandMenus=CommandMenus;
  command_widget.Menus=Menus;
  command_widget.Commands=Commands;
  command_widget.command=command;
  XRunCommandWidget(&command_widget,command_targets);
}

typedef struct _AnimateEventInput
{
  const AnimateCommandTargets
    *command_targets;

  const AnimateClientTargets
    *client_targets;

  ssize_t
    scene;
} AnimateEventInput;

static void XDispatchAnimateEvent(const AnimateEventContext *event_context,
  XEvent event,const AnimateEventInput *input)
{
  const AnimateClientTargets
    *const client_targets = input->client_targets;

  const AnimateCommandTargets
    *const command_targets = input->command_targets;

  const ssize_t
    scene = input->scene;

  XResourceInfo
    *const resource_info = event_context->resource_info;

switch (event.type)
{
  case ButtonPress:
  {
    XHandleButtonPress(event_context,command_targets);
    break;
  }
  case ButtonRelease:
  {
    XLogButtonRelease(resource_info,event);
    break;
  }
  case ClientMessage:
  {
    XHandleClientMessage(event_context,event,client_targets);
    break;
  }
  default:
  {
    XHandleAnimateWindowChange(event_context,event,scene,command_targets);
    break;
  }
}
}

static Image *XRunAnimateLoop(const AnimateEventContext *event_context,
  Image **images,XGCValues *context_values,XEvent *event)
{
  char
    command[MagickPathExtent];

  Display
    *const display = event_context->display;

  ExceptionInfo
    *const exception = event_context->exception;

  Image
    *image,
    **const image_list = event_context->image_list,
    *nexus;

  AnimateEventInput
    input;

  MagickStatusType
    state;

  ssize_t
    first_scene,
    iterations,
    scene;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  const AnimateCommandTargets
    command_targets = { event, &image, &state, &nexus };

  const AnimateClientTargets
    client_targets = { &state, context_values, &nexus };

  const AnimatePlayTargets
    play_targets = { &scene, &iterations, &state, &image, &first_scene,
      event };

  const AnimateCommandTile
    tile = { display, resource_info, windows, images, (&state), exception };

  nexus=NewImageList();
  scene=0;
  first_scene=0;
  iterations=0;
  image=image_list[0];
  state=(MagickStatusType) (ForwardAnimationState | RepeatAnimationState);
  (void) XMagickCommand(&tile,PlayCommand);
  do
  {
    if (IsAnimationStep(display,state) != MagickFalse)
      {
        XPlayNextScene(event_context,&play_targets);
        continue;
      }
    /*
      Handle a window event.
    */
    XNextAnimateEvent(display,windows,event);
    if ((*event).xany.window == windows->command.id)
      {
        XRunCommandWindow(event_context,command,&command_targets);
        continue;
      }
    input.command_targets=&command_targets;
    input.client_targets=&client_targets;
    input.scene=scene;
    XDispatchAnimateEvent(event_context,(*event),&input);
  }
  while (!(state & ExitState));
  return(nexus);
}

typedef struct _AnimateExitInfo
{
  Image
    *images;

  WarningHandler
    warning_handler;

  char
    *working_directory;
} AnimateExitInfo;

static Image *XFinishAnimation(const AnimateEventContext *event_context,
  const AnimateWindowSetup *setup,const AnimateExitInfo *exit_info,
  Image *nexus)
{
  Display
    *const display = event_context->display;

  ExceptionInfo
    *const exception = event_context->exception;

  Image
    *images = exit_info->images,
    **image_list = event_context->image_list;

  const size_t
    number_scenes = event_context->number_scenes;

  const WarningHandler
    warning_handler = exit_info->warning_handler;

  char
    *const working_directory = exit_info->working_directory;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  image_list=(Image **) RelinquishMagickMemory(image_list);
  images=DestroyImageList(images);
  XWithdrawColormapWindows(display,windows);
  XWithdrawBackdropWindows(display,resource_info,windows);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  XFreeScenePixmaps(display,windows,number_scenes);
  XSetCursorState(display,windows,MagickFalse);
  windows->image.pixmaps=(Pixmap *)
    RelinquishMagickMemory(windows->image.pixmaps);
  windows->image.matte_pixmaps=(Pixmap *)
    RelinquishMagickMemory(windows->image.matte_pixmaps);
  XReleaseAnimateWindows(setup,nexus);
  (void) XSync(display,MagickFalse);
  /*
    Restore our progress monitor and warning handlers.
  */
  (void) SetErrorHandler(warning_handler);
  (void) SetWarningHandler(warning_handler);
  /*
    Change to home directory.
  */
  XChangeToHomeDirectory(resource_info,working_directory,exception);
  return(nexus);
}

static void XInitializeMagickWindows(XWindows *windows,
  XWindowInfo **magick_windows,size_t *number_windows)
{
  (*number_windows)=0;
  magick_windows[(*number_windows)++]=(&windows->icon);
  magick_windows[(*number_windows)++]=(&windows->backdrop);
  magick_windows[(*number_windows)++]=(&windows->image);
  magick_windows[(*number_windows)++]=(&windows->info);
  magick_windows[(*number_windows)++]=(&windows->command);
  magick_windows[(*number_windows)++]=(&windows->widget);
  magick_windows[(*number_windows)++]=(&windows->popup);
  XClearMagickWindowIds(magick_windows,(*number_windows));
}

typedef struct _AnimateRunInputs
{
  Image
    **image_list;

  ExceptionInfo
    *exception;

  Image
    *images,
    *nexus;

  WarningHandler
    warning_handler;

  char
    *working_directory;

  XWindowInfo
    **magick_windows;

  size_t
    number_windows;
} AnimateRunInputs;

static Image *XRunAnimateSession(const AnimateWindowSetup *setup,
  const AnimateRunInputs *inputs)
{
  Display
    *const display = setup->display;

  ExceptionInfo
    *const exception = inputs->exception;

  Image
    *images = inputs->images,
    *nexus = inputs->nexus;

  MagickStatusType
    context_mask;

  WarningHandler
    warning_handler = inputs->warning_handler;

  XEvent
    event;

  XGCValues
    context_values;

  XResourceInfo
    *const resource_info = setup->resource_info;

  XWindows
    *const windows = setup->windows;

  AnimateExitInfo
    exit_info;

  XCreateContextWindow(setup);
  XCreateGraphicContexts(setup,images->filename,&context_values,
    &context_mask);
  XCreateIconWindow(setup);
  /*
    Initialize graphic context for icon window.
  */
  XCreateIconContext(setup,images->filename,&context_values);
  /*
    Initialize Image window.
  */
  XCreateImageWindowInfo(setup,exception);
  XCreateBackdropWindow(setup);
  XCreateImageWindow(setup);
  XCreateInfoWindow(setup);
  XCreateCommandWindow(setup);
  /*
    Initialize Widget window.
  */
  XCreatePanelWindows(setup);
  /*
    Set out progress and warning handlers.
  */
  warning_handler=XSetAnimateWarningHandlers(resource_info,warning_handler);
  /*
    Initialize X image structure.
  */
  XCreateImagePixmaps(setup,images->filename);
  XSetCursorState(display,windows,MagickTrue);
  {
    const AnimateEventContext
      event_context = { display, resource_info, windows, setup->visual_info,
        setup->map_info, inputs->image_list, setup->display_image, exception,
        setup->icon_resources, setup->icon_visual, setup->icon_map,
        setup->icon_pixel, CommandMenu, setup->pixel, context_mask,
        setup->root_window, inputs->magick_windows, inputs->number_windows,
        setup->number_scenes };

    XCreateSceneWindows(&event_context,images,&event);
    XSetCursorState(display,windows,MagickFalse);
    if (windows->command.mapped)
      (void) XMapRaised(display,windows->command.id);
    /*
      Respond to events.
    */
    nexus=XRunAnimateLoop(&event_context,&images,&context_values,&event);
    exit_info.images=images;
    exit_info.warning_handler=warning_handler;
    exit_info.working_directory=inputs->working_directory;
    return(XFinishAnimation(&event_context,setup,&exit_info,nexus));
  }
}

typedef struct _AnimateStartInputs
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  char
    **argv;

  int
    argc;

  XWindows
    *windows;

  Image
    *images;

  ExceptionInfo
    *exception;

  WarningHandler
    warning_handler;

  char
    *working_directory;

  XWindowInfo
    **magick_windows;

  size_t
    number_windows;

  XVisualInfo
    *visual_info;

  XStandardColormap
    *map_info;

  XPixelInfo
    *pixel;

  XFontStruct
    *font_info;

  Window
    root_window;

  XClassHint
    *class_hints;

  XWMHints
    *manager_hints;

  XResourceInfo
    *icon_resources;

  XVisualInfo
    *icon_visual;

  XStandardColormap
    *icon_map;

  XPixelInfo
    *icon_pixel;
} AnimateStartInputs;

static Image *XStartAnimateSession(const AnimateStartInputs *start)
{
  Display
    *const display = start->display;

  ExceptionInfo
    *const exception = start->exception;

  Image
    *display_image,
    **image_list,
    *images = start->images,
    *nexus;

  size_t
    number_scenes;

  XResourceInfo
    *const resource_info = start->resource_info;

  AnimateSceneList
    scene_list;

  images=XCoalesceAnimateImages(images,exception);
  XRemapColormapMismatch(resource_info,start->visual_info,images,exception);
  /*
    Sort images by increasing scene number.
  */
  number_scenes=GetImageListLength(images);
  image_list=XSortAnimateScenes(images,number_scenes,exception);
  /*
    Initialize Standard Colormap.
  */
  nexus=NewImageList();
  scene_list.resource_info=resource_info;
  scene_list.visual_info=start->visual_info;
  scene_list.image_list=image_list;
  scene_list.number_scenes=number_scenes;
  scene_list.exception=exception;
  display_image=XPrepareSceneImages(&scene_list);
  XLogDisplayImage(resource_info,display_image);
  XMakeStandardColormap(display,start->visual_info,resource_info,display_image,
    start->map_info,start->pixel,exception);
  {
    const AnimateWindowSetup
      setup = { display, resource_info, start->windows, start->visual_info,
        start->map_info, start->pixel, start->font_info, start->root_window,
        start->class_hints, start->manager_hints, start->argv, start->argc,
        start->icon_resources, start->icon_visual, start->icon_map,
        start->icon_pixel, display_image, number_scenes };

    const AnimateRunInputs
      inputs = { image_list, exception, images, nexus, start->warning_handler,
        start->working_directory, start->magick_windows,
        start->number_windows };

    return(XRunAnimateSession(&setup,&inputs));
  }
}

MagickExport Image *XAnimateImages(Display *display,
  XResourceInfo *resource_info,char **argv,const int argc,Image *images,
  ExceptionInfo *exception)
{
  static char
    working_directory[MagickPathExtent];

  static size_t
    number_windows;

  static XWindowInfo
    *magick_windows[MaXWindows];

  WarningHandler
    warning_handler;

  Window
    root_window;

  XClassHint
    *class_hints;

  XFontStruct
    *font_info;

  XPixelInfo
    *icon_pixel,
    *pixel;

  XResourceInfo
    *icon_resources;

  XStandardColormap
    *icon_map,
    *map_info;

  XVisualInfo
    *icon_visual,
    *visual_info;

  XWindows
    *windows;

  XWMHints
    *manager_hints;

  AnimateStartInputs
    start;

  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  warning_handler=(WarningHandler) NULL;
  windows=XSetWindows((XWindows *) ~0);
  if (windows != (XWindows *) NULL)
    {
      warning_handler=XEnterWorkingDirectory(resource_info,working_directory,
        exception);
    }
  else
    {
      windows=XCreateAnimateWindows(display,resource_info,images);
      /*
        Initialize window id's.
      */
      XInitializeMagickWindows(windows,magick_windows,&number_windows);
    }
  XLoadAnimateFont(display,resource_info,windows);
  /*
    Initialize Standard Colormap.
  */
  map_info=windows->map_info;
  icon_map=windows->icon_map;
  visual_info=windows->visual_info;
  icon_visual=windows->icon_visual;
  pixel=windows->pixel_info;
  icon_pixel=windows->icon_pixel;
  font_info=windows->font_info;
  icon_resources=windows->icon_resources;
  class_hints=windows->class_hints;
  manager_hints=windows->manager_hints;
  root_window=XRootWindow(display,visual_info->screen);
  start.display=display;
  start.resource_info=resource_info;
  start.argv=argv;
  start.argc=argc;
  start.windows=windows;
  start.images=images;
  start.exception=exception;
  start.warning_handler=warning_handler;
  start.working_directory=working_directory;
  start.magick_windows=magick_windows;
  start.number_windows=number_windows;
  start.visual_info=visual_info;
  start.map_info=map_info;
  start.pixel=pixel;
  start.font_info=font_info;
  start.root_window=root_window;
  start.class_hints=class_hints;
  start.manager_hints=manager_hints;
  start.icon_resources=icon_resources;
  start.icon_visual=icon_visual;
  start.icon_map=icon_map;
  start.icon_pixel=icon_pixel;
  return(XStartAnimateSession(&start));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S a v e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XSaveImage() saves an image to a file.
%
%  The format of the XSaveImage method is:
%
%      MagickBooleanType XSaveImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o status: Method XSaveImage return True if the image is
%      written.  False is returned is there is a memory shortage or if the
%      image fails to write.
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
*/
static MagickBooleanType IsPostscriptFormat(const ImageInfo *image_info)
{
  return(((LocaleCompare(image_info->magick,"EPS") == 0) ||
    (LocaleCompare(image_info->magick,"PDF") == 0) ||
    (LocaleCompare(image_info->magick,"PS") == 0) ||
    (LocaleCompare(image_info->magick,"PS2") == 0)) ? MagickTrue : MagickFalse);
}

static void XChangeToImageDirectory(const Image *image,char *filename,
  ExceptionInfo *exception)
{
  char
    path[MagickPathExtent];

  int
    status;

  GetPathComponent(image->filename,HeadPath,path);
  GetPathComponent(image->filename,TailPath,filename);
  if (*path == '\0')
    (void) CopyMagickString(path,".",MagickPathExtent);
  status=chdir(path);
  if (status == -1)
    (void) ThrowMagickException(exception,GetMagickModule(),FileOpenError,
      "UnableToOpenFile","%s",path);
}

static void XRequestPageGeometry(Display *display,XWindows *windows,
  ImageInfo *image_info)
{
  char
    geometry[MagickPathExtent];

  /*
    Request page geometry from user.
  */
  (void) CopyMagickString(geometry,PSPageGeometry,MagickPathExtent);
  if (LocaleCompare(image_info->magick,"PDF") == 0)
    (void) CopyMagickString(geometry,PSPageGeometry,MagickPathExtent);
  if (image_info->page != (char *) NULL)
    (void) CopyMagickString(geometry,image_info->page,MagickPathExtent);
  XListBrowserWidget(display,windows,&windows->widget,PageSizes,"Select",
    "Select page geometry:",geometry);
  if (*geometry != '\0')
    image_info->page=GetPageGeometry(geometry);
}

static MagickBooleanType IsJPEGFormat(const ImageInfo *image_info)
{
  return(((LocaleCompare(image_info->magick,"JPEG") == 0) ||
    (LocaleCompare(image_info->magick,"JPG") == 0)) ? MagickTrue :
    MagickFalse);
}

static void XSetJPEGQuality(Image *image,ImageInfo *image_info,
  const char *quality,const int status)
{
  image->quality=StringToUnsignedLong(quality);
  image_info->interlace=status != MagickFalse ?  NoInterlace :
    PlaneInterlace;
}

static MagickStatusType XWriteSavedImages(ImageInfo *image_info,
  Image *image,char *filename,ExceptionInfo *exception)
{
  MagickStatusType
    status;

  image=GetFirstImageInList(image);
  status=WriteImages(image_info,image,filename,exception);
  if (status != MagickFalse)
    image->taint=MagickFalse;
  return(status);
}

static MagickBooleanType XFinishSaveImage(Display *display,
  XWindows *windows,ImageInfo *image_info,const MagickStatusType status)
{
  image_info=DestroyImageInfo(image_info);
  XSetCursorState(display,windows,MagickFalse);
  return(status != 0 ? MagickTrue : MagickFalse);
}

static MagickBooleanType XSaveImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent];

  ImageInfo
    *image_info;

  MagickStatusType
    status;

  /*
    Request file name from user.
  */
  if (resource_info->write_filename != (char *) NULL)
    (void) CopyMagickString(filename,resource_info->write_filename,
      MagickPathExtent);
  else
    XChangeToImageDirectory(image,filename,exception);
  XFileBrowserWidget(display,windows,"Save",filename);
  if (*filename == '\0')
    return(MagickTrue);
  if (IsPathAccessible(filename) != MagickFalse)
    {
      int
        status;

      /*
        File exists-- seek user's permission before overwriting.
      */
      status=XConfirmWidget(display,windows,"Overwrite",filename);
      if (status == 0)
        return(MagickTrue);
    }
  image_info=CloneImageInfo(resource_info->image_info);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  (void) SetImageInfo(image_info,1,exception);
  if (IsJPEGFormat(image_info) != MagickFalse)
    {
      char
        quality[MagickPathExtent];

      int
        status;

      /*
        Request JPEG quality from user.
      */
      (void) FormatLocaleString(quality,MagickPathExtent,"%.17g",(double)
        image_info->quality);
      status=XDialogWidget(display,windows,"Save","Enter JPEG quality:",
        quality);
      if (*quality == '\0')
        return(MagickTrue);
      XSetJPEGQuality(image,image_info,quality,status);
    }
  if (IsPostscriptFormat(image_info) != MagickFalse)
    XRequestPageGeometry(display,windows,image_info);
  /*
    Write image.
  */
  status=XWriteSavedImages(image_info,image,filename,exception);
  return(XFinishSaveImage(display,windows,image_info,status));
}
#else

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A n i m a t e I m a g e s                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AnimateImages() repeatedly displays an image sequence to any X window
%  screen.  It returns a value other than 0 if successful.  Check the
%  exception member of image to determine the reason for any failure.
%
%  The format of the AnimateImages method is:
%
%      MagickBooleanType AnimateImages(const ImageInfo *image_info,
%        Image *images)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType AnimateImages(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  (void) image_info;
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  (void) ThrowMagickException(exception,GetMagickModule(),MissingDelegateError,
    "DelegateLibrarySupportNotBuiltIn","'%s' (X11)",image->filename);
  return(MagickFalse);
}
#endif
