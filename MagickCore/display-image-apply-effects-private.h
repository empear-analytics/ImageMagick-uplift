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

  MagickCore private methods of display-image.c: the image-edit and miscellany
  command appliers, from XApplyAnnotateCommand to XApplyRestoreCommand;
  included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_APPLY_EFFECTS_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_APPLY_EFFECTS_PRIVATE_H

static void XNoticeEditFailure(const XMagickCommandContext *context,
  const MagickStatusType status,const char *reason)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  if (status == MagickFalse)
    {
      XNoticeWidget(display,windows,reason,
        (*image)->filename);
      return;
    }
}

static void XApplyAnnotateCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  /*
    Annotate the image with text.
  */
  status=XAnnotateEditImage(display,resource_info,windows,*image,exception);
  XNoticeEditFailure(context,status,"Unable to annotate X image");
}

static void XApplyDrawCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  const XImageCacheContext
    edit_context = { display, resource_info, windows, image, exception };

  /*
    Draw image.
  */
  status=XDrawEditImage(&edit_context);
  XNoticeEditFailure(context,status,"Unable to draw on the X image");
}

static void XApplyColorCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  /*
    Color edit.
  */
  status=XColorEditImage(display,resource_info,windows,image,exception);
  XNoticeEditFailure(context,status,"Unable to pixel edit X image");
}

static void XApplyMatteCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  /*
    Matte edit.
  */
  status=XMatteEditImage(display,resource_info,windows,image,exception);
  XNoticeEditFailure(context,status,"Unable to matte edit X image");
}

static void XApplyCompositeCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  /*
    Composite image.
  */
  status=XCompositeImage(display,resource_info,windows,*image,
    exception);
  XNoticeEditFailure(context,status,"Unable to composite X image");
}

static void XApplyAddBorderCommand(const XMagickCommandContext *context,
  char *color)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  RectangleInfo
    page_geometry;

  Image
    *border_image;

  static char
    geometry[MagickPathExtent] = "6x6";

  const XImageCacheContext
    command_context = { display, resource_info, windows, image, exception };

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for border color and geometry.
  */
  XColorBrowserWidget(display,windows,"Select",color);
  if (*color == '\0')
    return;
  {
    const XDialogRequest
      dialog_request = { "Add Border", "Enter border geometry:", geometry };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*geometry == '\0')
    return;
  /*
    Add a border to the image.
  */
  (void) XMagickCommand(&command_context,ApplyCommand);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) QueryColorCompliance(color,AllCompliance,&(*image)->border_color,
    exception);
  (void) ParsePageGeometry(*image,geometry,&page_geometry,
    exception);
  border_image=BorderImage(*image,&page_geometry,(*image)->compose,
    exception);
  if (border_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=border_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  windows->image.window_changes.width=(int) (*image)->columns;
  windows->image.window_changes.height=(int) (*image)->rows;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyAddFrameCommand(const XMagickCommandContext *context,
  char *color)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  RectangleInfo
    page_geometry;

  FrameInfo
    frame_info;

  Image
    *frame_image;

  static char
    geometry[MagickPathExtent] = "6x6";

  const XImageCacheContext
    command_context = { display, resource_info, windows, image, exception };

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for frame color and geometry.
  */
  XColorBrowserWidget(display,windows,"Select",color);
  if (*color == '\0')
    return;
  {
    const XDialogRequest
      dialog_request = { "Add Frame", "Enter frame geometry:", geometry };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*geometry == '\0')
    return;
  /*
    Surround image with an ornamental border.
  */
  (void) XMagickCommand(&command_context,ApplyCommand);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) QueryColorCompliance(color,AllCompliance,&(*image)->matte_color,
    exception);
  (void) ParsePageGeometry(*image,geometry,&page_geometry,
    exception);
  frame_info.width=page_geometry.width;
  frame_info.height=page_geometry.height;
  frame_info.outer_bevel=page_geometry.x;
  frame_info.inner_bevel=page_geometry.y;
  frame_info.x=(ssize_t) frame_info.width;
  frame_info.y=(ssize_t) frame_info.height;
  frame_info.width=(*image)->columns+2*frame_info.width;
  frame_info.height=(*image)->rows+2*frame_info.height;
  frame_image=FrameImage(*image,&frame_info,(*image)->compose,exception);
  if (frame_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=frame_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  windows->image.window_changes.width=(int) (*image)->columns;
  windows->image.window_changes.height=(int) (*image)->rows;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XStoreEditedComment(const XMagickCommandContext *context)
{
  Image
    **const image = context->image;

  ImageInfo
    *const image_info = context->image_info;

  ExceptionInfo
    *const exception = context->exception;

  char
    *comment;

  comment=FileToString(image_info->filename,~0UL,exception);
  if (comment != (char *) NULL)
    {
      (void) SetImageProperty(*image,"comment",comment,exception);
      (*image)->taint=MagickTrue;
    }
}

static void XApplyCommentCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ImageInfo
    *const image_info = context->image_info;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  const char
    *value;

  FILE
    *file;

  int
    unique_file;

  /*
    Edit image comment.
  */
  unique_file=AcquireUniqueFileResource(image_info->filename);
  if (unique_file == -1)
    {
      XNoticeWidget(display,windows,"Unable to edit image comment",
        image_info->filename);
      return;
    }
  value=GetImageProperty(*image,"comment",exception);
  if (value == (char *) NULL)
    unique_file=close_utf8(unique_file)-1;
  else
    {
      const char
        *p;

      file=fdopen(unique_file,"w");
      if (file == (FILE *) NULL)
        {
          XNoticeWidget(display,windows,"Unable to edit image comment",
            image_info->filename);
          return;
        }
      for (p=value; *p != '\0'; p++)
        (void) fputc((int) *p,file);
      (void) fputc('\n',file);
      (void) fclose(file);
    }
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  status=InvokeDelegate(image_info,*image,"edit",(char *) NULL,
    exception);
  if (status == MagickFalse)
    XNoticeWidget(display,windows,"Unable to edit image comment",
      (char *) NULL);
  else
    XStoreEditedComment(context);
  (void) RelinquishUniqueFileResource(image_info->filename);
  XSetCursorState(display,windows,MagickFalse);
}

static Image *XApplyLaunchCommand(const XMagickCommandContext *context,
  Image *nexus)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ImageInfo
    *const image_info = context->image_info;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  char
    filename[MagickPathExtent];

  /*
    Launch program.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) AcquireUniqueFilename(filename);
  (void) FormatLocaleString((*image)->filename,MagickPathExtent,"launch:%s",
    filename);
  status=WriteImage(image_info,*image,exception);
  if (status == MagickFalse)
    XNoticeWidget(display,windows,"Unable to launch image editor",
      (char *) NULL);
  else
    {
      nexus=ReadImage(resource_info->image_info,exception);
      CatchException(exception);
      {
        const XClientReason
          reason = { windows->im_next_image, CurrentTime };

        XClientMessage(display,windows->image.id,windows->im_protocols,&reason);
      }
    }
  (void) RelinquishUniqueFileResource(filename);
  XSetCursorState(display,windows,MagickFalse);
  return(nexus);
}

static void XApplyZoomCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  /*
    Zoom image.
  */
  if (windows->magnify.mapped != MagickFalse)
    (void) XRaiseWindow(display,windows->magnify.id);
  else
    {
      /*
        Make magnify image.
      */
      XSetCursorState(display,windows,MagickTrue);
      (void) XMapRaised(display,windows->magnify.id);
      XSetCursorState(display,windows,MagickFalse);
    }
}

static void XShowImagePreview(const XMagickCommandContext *context,
  char *preview_type,char *filename)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ImageInfo
    *const image_info = context->image_info;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  Image
    *preview_image;

  PreviewType
    preview;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  preview=(PreviewType) ParseCommandOption(MagickPreviewOptions,
    MagickFalse,preview_type);
  (void) FormatImageProperty(*image,"group","%.17g",(double)
    windows->image.id);
  (void) DeleteImageProperty(*image,"label");
  (void) SetImageProperty(*image,"label","Preview",exception);
  preview_image=PreviewImage(*image,preview,exception);
  if (preview_image == (Image *) NULL)
    return;
  (void) AcquireUniqueFilename(filename);
  (void) FormatLocaleString(preview_image->filename,MagickPathExtent,
    "show:%s",filename);
  status=WriteImage(image_info,preview_image,exception);
  (void) RelinquishUniqueFileResource(filename);
  preview_image=DestroyImage(preview_image);
  if (status == MagickFalse)
    XNoticeWidget(display,windows,"Unable to show image preview",
      (*image)->filename);
  XDelay(display,1500);
  XSetCursorState(display,windows,MagickFalse);
}

static void XApplyShowPreviewCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    filename[MagickPathExtent];

  char
    **previews;

  static char
    preview_type[MagickPathExtent] = "Gamma";

  /*
    Select preview type from menu.
  */
  previews=GetCommandOptions(MagickPreviewOptions);
  if (previews == (char **) NULL)
    return;
  {
    const XListBrowserRequest
      browse_request = { (const char **) previews, "Preview",
        "Select an enhancement, effect, or F/X:", preview_type };

    XListBrowserWidget(display,windows,&windows->widget,&browse_request);
  }
  previews=DestroyStringList(previews);
  if (*preview_type == '\0')
    return;
  /*
    Show image preview.
  */
  XShowImagePreview(context,preview_type,filename);
}

static void XApplyShowHistogramCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ImageInfo
    *const image_info = context->image_info;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  char
    filename[MagickPathExtent];

  Image
    *histogram_image;

  /*
    Show image histogram.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) DeleteImageProperty(*image,"label");
  (void) FormatImageProperty(*image,"group","%.17g",(double)
    windows->image.id);
  (void) SetImageProperty(*image,"label","Histogram",exception);
  (void) AcquireUniqueFilename(filename);
  (void) FormatLocaleString((*image)->filename,MagickPathExtent,
    "histogram:%s",filename);
  status=WriteImage(image_info,*image,exception);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  histogram_image=ReadImage(image_info,exception);
  (void) RelinquishUniqueFileResource(filename);
  if (histogram_image == (Image *) NULL)
    return;
  (void) FormatLocaleString(histogram_image->filename,MagickPathExtent,
    "show:%s",filename);
  status=WriteImage(image_info,histogram_image,exception);
  histogram_image=DestroyImage(histogram_image);
  if (status == MagickFalse)
    XNoticeWidget(display,windows,"Unable to show histogram",
      (*image)->filename);
  XDelay(display,1500);
  XSetCursorState(display,windows,MagickFalse);
}

static void XApplyShowMatteCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ImageInfo
    *const image_info = context->image_info;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  char
    filename[MagickPathExtent];

  Image
    *matte_image;

  if ((*image)->alpha_trait == UndefinedPixelTrait)
    {
      XNoticeWidget(display,windows,
        "Image does not have any matte information",(*image)->filename);
      return;
    }
  /*
    Show image matte.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) FormatImageProperty(*image,"group","%.17g",(double)
    windows->image.id);
  (void) DeleteImageProperty(*image,"label");
  (void) SetImageProperty(*image,"label","Matte",exception);
  (void) AcquireUniqueFilename(filename);
  (void) FormatLocaleString((*image)->filename,MagickPathExtent,"matte:%s",
    filename);
  status=WriteImage(image_info,*image,exception);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  matte_image=ReadImage(image_info,exception);
  (void) RelinquishUniqueFileResource(filename);
  if (matte_image == (Image *) NULL)
    return;
  (void) FormatLocaleString(matte_image->filename,MagickPathExtent,
    "show:%s",filename);
  status=WriteImage(image_info,matte_image,exception);
  matte_image=DestroyImage(matte_image);
  if (status == MagickFalse)
    XNoticeWidget(display,windows,"Unable to show matte",
      (*image)->filename);
  XDelay(display,1500);
  XSetCursorState(display,windows,MagickFalse);
}

static Image *XApplyBackgroundCommand(const XMagickCommandContext *context,
  Image *nexus)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  const ImageWindowKeyInput
    window_input = { display, resource_info, windows, image, exception };

  /*
    Background image.
  */
  status=XBackgroundImage(&window_input);
  if (status == MagickFalse)
    return(nexus);
  nexus=CloneImage(*image,0,0,MagickTrue,exception);
  if (nexus != (Image *) NULL)
    {
      const XClientReason
        reason = { windows->im_next_image, CurrentTime };

      XClientMessage(display,windows->image.id,windows->im_protocols,&reason);
    }
  return(nexus);
}

static void XApplySlideShowCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  static char
    delay[MagickPathExtent] = "5";

  /*
    Display next image after pausing.
  */
  {
    const XDialogRequest
      dialog_request = { "Slide Show",
        "Pause how many 1/100ths of a second between images:", delay };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*delay == '\0')
    return;
  resource_info->delay=StringToUnsignedLong(delay);
  {
    const XClientReason
      reason = { windows->im_next_image, CurrentTime };

    XClientMessage(display,windows->image.id,windows->im_protocols,&reason);
  }
}

static Image *XApplyPreferencesCommand(const XMagickCommandContext *context,
  Image *nexus)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  /*
    Set user preferences.
  */
  status=XPreferencesWidget(display,resource_info,windows);
  if (status == MagickFalse)
    return(nexus);
  nexus=CloneImage(*image,0,0,MagickTrue,exception);
  if (nexus != (Image *) NULL)
    {
      const XClientReason
        reason = { windows->im_next_image, CurrentTime };

      XClientMessage(display,windows->image.id,windows->im_protocols,&reason);
    }
  return(nexus);
}

static void XApplyBrowseDocumentationCommand(
  const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ImageInfo
    *const image_info = context->image_info;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

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
      char
        command[MagickPathExtent];

      /*
        Display documentation using Netscape remote control.
      */
      (void) FormatLocaleString(command,MagickPathExtent,
        "openurl(%s,new-tab)",MagickAuthoritativeURL);
      mozilla_atom=XInternAtom(display,"_MOZILLA_COMMAND",MagickFalse);
      (void) XChangeProperty(display,mozilla_window,mozilla_atom,XA_STRING,
        8,PropModeReplace,(unsigned char *) command,(int) strlen(command));
      XSetCursorState(display,windows,MagickFalse);
      return;
    }
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  status=InvokeDelegate(image_info,*image,"browse",(char *) NULL,
    exception);
  if (status == MagickFalse)
    XNoticeWidget(display,windows,"Unable to browse documentation",
      (char *) NULL);
  XDelay(display,1500);
  XSetCursorState(display,windows,MagickFalse);
}

static void XApplyAddNoiseToImage(const XMagickCommandContext *context,
  char *noise_type)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  Image
    *noise_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  noise_image=AddNoiseImage(*image,(NoiseType) ParseCommandOption(
    MagickNoiseOptions,MagickFalse,noise_type),1.0,exception);
  if (noise_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=noise_image;
    }
}

static void XApplyAddNoiseCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  char
    **noises;

  static char
    noise_type[MagickPathExtent] = "Gaussian";

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Add noise to the image.
  */
  noises=GetCommandOptions(MagickNoiseOptions);
  if (noises == (char **) NULL)
    return;
  {
    const XListBrowserRequest
      browse_request = { (const char **) noises, "Add Noise",
        "Select a type of noise to add to your image:", noise_type };

    XListBrowserWidget(display,windows,&windows->widget,&browse_request);
  }
  noises=DestroyStringList(noises);
  if (*noise_type == '\0')
    return;
  XApplyAddNoiseToImage(context,noise_type);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyRestoreCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Restore Image window to its original size.
  */
  if (IsImageWindowOriginalSize(windows,image) != MagickFalse)
    {
      (void) XBell(display,0);
      return;
    }
  windows->image.window_changes.width=(int) (*image)->columns;
  windows->image.window_changes.height=(int) (*image)->rows;
  if (windows->image.crop_geometry != (char *) NULL)
    {
      windows->image.crop_geometry=(char *)
        RelinquishMagickMemory(windows->image.crop_geometry);
      windows->image.crop_geometry=(char *) NULL;
      windows->image.x=0;
      windows->image.y=0;
    }
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

#endif
