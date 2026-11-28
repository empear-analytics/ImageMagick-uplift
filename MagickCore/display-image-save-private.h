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

  MagickCore private methods of display-image.c: the save command, from
  IsPostscriptFormat to XSaveImage; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_SAVE_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_SAVE_PRIVATE_H

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
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsPostscriptFormat(
  const ImageInfo *image_info)
{
  return((MagickBooleanType) ((LocaleCompare(image_info->magick,"EPS") == 0) ||
    (LocaleCompare(image_info->magick,"PDF") == 0) ||
    (LocaleCompare(image_info->magick,"PS") == 0) ||
    (LocaleCompare(image_info->magick,"PS2") == 0)));
}

static void XSelectPageGeometry(Display *display,XWindows *windows,
  ImageInfo *image_info)
{
  if (IsPostscriptFormat(image_info) != MagickFalse)
    {
      char
        geometry[MagickPathExtent];

      const char
        *const PageSizes[] =
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

      /*
        Request page geometry from user.
      */
      (void) CopyMagickString(geometry,PSPageGeometry,MagickPathExtent);
      if (LocaleCompare(image_info->magick,"PDF") == 0)
        (void) CopyMagickString(geometry,PSPageGeometry,MagickPathExtent);
      if (image_info->page != (char *) NULL)
        (void) CopyMagickString(geometry,image_info->page,MagickPathExtent);
      {
        const XListBrowserRequest
          browse_request = { PageSizes, "Select", "Select page geometry:",
            geometry };

        XListBrowserWidget(display,windows,&windows->widget,&browse_request);
      }
      if (*geometry != '\0')
        image_info->page=GetPageGeometry(geometry);
    }
}

static MagickBooleanType XRequestJPEGQuality(Display *display,
  XWindows *windows,Image *image,ImageInfo *image_info)
{
  if ((LocaleCompare(image_info->magick,"JPEG") == 0) ||
      (LocaleCompare(image_info->magick,"JPG") == 0))
    {
      char
        quality[MagickPathExtent];

      int
        status;

      /*
        Request JPEG quality from user.
      */
      (void) FormatLocaleString(quality,MagickPathExtent,"%.17g",(double)
        image->quality);
      {
        const XDialogRequest
          dialog_request = { "Save", "Enter JPEG quality:", quality };

        status=XDialogWidget(display,windows,&dialog_request);
      }
      if (*quality == '\0')
        return(MagickFalse);
      image->quality=StringToUnsignedLong(quality);
      image_info->interlace=status != 0 ? NoInterlace : PlaneInterlace;
    }
  return(MagickTrue);
}

static MagickBooleanType XSeekOverwritePermission(Display *display,
  XWindows *windows,const char *filename)
{
  if (IsPathAccessible(filename) != MagickFalse)
    {
      int
        status;

      /*
        File exists-- seek user's permission before overwriting.
      */
      status=XConfirmWidget(display,windows,"Overwrite",filename);
      if (status <= 0)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static void XGetSaveFilename(const XResourceInfo *resource_info,
  const Image *image,char *filename,ExceptionInfo *exception)
{
  if (resource_info->write_filename != (char *) NULL)
    (void) CopyMagickString(filename,resource_info->write_filename,
      MagickPathExtent);
  else
    {
      char
        path[MagickPathExtent];

      int
        status;

      GetPathComponent(image->filename,HeadPath,path);
      GetPathComponent(image->filename,TailPath,filename);
      if (*path != '\0')
        {
          status=chdir(path);
          if (status == -1)
            (void) ThrowMagickException(exception,GetMagickModule(),
              FileOpenError,"UnableToOpenFile","%s",path);
        }
    }
}

static MagickBooleanType XSaveImage(const XImageCommandInput *command_input)
{
  Display
    *const display = command_input->display;

  XResourceInfo
    *const resource_info = command_input->resource_info;

  XWindows
    *const windows = command_input->windows;

  Image
    *const image = command_input->image;

  ExceptionInfo
    *const exception = command_input->exception;

  char
    filename[MagickPathExtent],
    geometry[MagickPathExtent];

  Image
    *save_image;

  ImageInfo
    *image_info;

  MagickStatusType
    status;

  /*
    Request file name from user.
  */
  XGetSaveFilename(resource_info,image,filename,exception);
  XFileBrowserWidget(display,windows,"Save",filename);
  if (*filename == '\0')
    return(MagickTrue);
  if (XSeekOverwritePermission(display,windows,filename) == MagickFalse)
    return(MagickTrue);
  image_info=CloneImageInfo(resource_info->image_info);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  (void) SetImageInfo(image_info,1,exception);
  if (XRequestJPEGQuality(display,windows,image,image_info) == MagickFalse)
    return(MagickTrue);
  XSelectPageGeometry(display,windows,image_info);
  /*
    Apply image transforms.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  save_image=CloneImage(image,0,0,MagickTrue,exception);
  if (save_image == (Image *) NULL)
    return(MagickFalse);
  (void) FormatLocaleString(geometry,MagickPathExtent,"%dx%d!",
    windows->image.ximage->width,windows->image.ximage->height);
  (void) TransformImage(&save_image,windows->image.crop_geometry,geometry,
    exception);
  /*
    Write image.
  */
  (void) CopyMagickString(save_image->filename,filename,MagickPathExtent);
  status=WriteImage(image_info,save_image,exception);
  if (status != MagickFalse)
    image->taint=MagickFalse;
  save_image=DestroyImage(save_image);
  image_info=DestroyImageInfo(image_info);
  XSetCursorState(display,windows,MagickFalse);
  return((MagickBooleanType) (status != 0));
}

#endif
