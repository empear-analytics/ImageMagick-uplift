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

  MagickCore private methods of display-image.c: the transform, enhance,
  effect and quit command appliers, from XApplyFlopCommand to
  XApplyReduceNoiseToImage; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_DRAW_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_DRAW_PRIVATE_H

static void XApplyFlopCommand(const XMagickCommandContext *context)
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

  int
    x,
    y;

  unsigned int
    height,
    width;

  /*
    Flop image scanlines.
  */
  XApplyFlopToImage(display,windows,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.crop_geometry != (char *) NULL)
    {
      /*
        Flop crop geometry.
      */
      width=(unsigned int) (*image)->columns;
      height=(unsigned int) (*image)->rows;
      (void) XParseGeometry(windows->image.crop_geometry,&x,&y,
        &width,&height);
      (void) FormatLocaleString(windows->image.crop_geometry,
        MagickPathExtent,"%ux%u%+d%+d",width,height,(int) (*image)->columns-
        (int) width-x,y);
    }
  if (windows->image.orphan != MagickFalse)
    return;
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyFlipToImage(Display *display,XWindows *windows,Image **image,
  ExceptionInfo *exception)
{
  Image
    *flip_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  flip_image=FlipImage(*image,exception);
  if (flip_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=flip_image;
    }
}

static void XApplyFlipCommand(const XMagickCommandContext *context)
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

  int
    x,
    y;

  unsigned int
    height,
    width;

  /*
    Flip image scanlines.
  */
  XApplyFlipToImage(display,windows,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.crop_geometry != (char *) NULL)
    {
      /*
        Flip crop geometry.
      */
      width=(unsigned int) (*image)->columns;
      height=(unsigned int) (*image)->rows;
      (void) XParseGeometry(windows->image.crop_geometry,&x,&y,
        &width,&height);
      (void) FormatLocaleString(windows->image.crop_geometry,
        MagickPathExtent,"%ux%u%+d%+d",width,height,x,(int) (*image)->rows-
        (int) height-y);
    }
  if (windows->image.orphan != MagickFalse)
    return;
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyRotateRightCommand(const XMagickCommandContext *context)
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
    Rotate image 90 degrees clockwise.
  */
  status=XRotateImage(display,resource_info,windows,90.0,image,exception);
  if (status == MagickFalse)
    {
      XNoticeWidget(display,windows,"Unable to rotate X image",
        (*image)->filename);
      return;
    }
}

static void XApplyRotateLeftCommand(const XMagickCommandContext *context)
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
    Rotate image 90 degrees counter-clockwise.
  */
  status=XRotateImage(display,resource_info,windows,-90.0,image,exception);
  if (status == MagickFalse)
    {
      XNoticeWidget(display,windows,"Unable to rotate X image",
        (*image)->filename);
      return;
    }
}

static void XApplyRotateCommand(const XMagickCommandContext *context)
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
    Rotate image.
  */
  status=XRotateImage(display,resource_info,windows,0.0,image,exception);
  if (status == MagickFalse)
    {
      XNoticeWidget(display,windows,"Unable to rotate X image",
        (*image)->filename);
      return;
    }
}

static void XApplyTrimCommand(const XMagickCommandContext *context)
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

  static char
    fuzz[MagickPathExtent];

  /*
    Query user for the fuzz factor.
  */
  (void) FormatLocaleString(fuzz,MagickPathExtent,"%g%%",100.0*
    (*image)->fuzz/((double) QuantumRange+1.0));
  {
    const XDialogRequest
      dialog_request = { "Trim", "Enter fuzz factor:", fuzz };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*fuzz == '\0')
    return;
  (*image)->fuzz=StringToDoubleInterval(fuzz,(double) QuantumRange+1.0);
  /*
    Trim image.
  */
  status=XTrimImage(display,resource_info,windows,*image,exception);
  if (status == MagickFalse)
    {
      XNoticeWidget(display,windows,"Unable to trim X image",
        (*image)->filename);
      return;
    }
}

static void XApplySaturationToImage(const XMagickCommandContext *context,
  char *modulate_factors,char *saturation_percent)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(modulate_factors,"100.0/",MagickPathExtent);
  (void) ConcatenateMagickString(modulate_factors,saturation_percent,
    MagickPathExtent);
  (void) ModulateImage(*image,modulate_factors,exception);
}

static void XApplySaturationCommand(const XMagickCommandContext *context)
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
    modulate_factors[MagickPathExtent];

  static char
    saturation_percent[MagickPathExtent] = "110";

  /*
    Query user for percent saturation change.
  */
  {
    const XDialogRequest
      dialog_request = { "Apply",
        "Enter percent change in color saturation (0-200):",
        saturation_percent };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*saturation_percent == '\0')
    return;
  /*
    Vary color saturation.
  */
  XApplySaturationToImage(context,modulate_factors,saturation_percent);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyBrightnessToImage(Display *display,XWindows *windows,
  char *modulate_factors,char *brightness_percent,Image **image,
  ExceptionInfo *exception)
{
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(modulate_factors,brightness_percent,
    MagickPathExtent);
  (void) ModulateImage(*image,modulate_factors,exception);
}

static void XApplyBrightnessCommand(const XMagickCommandContext *context)
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
    modulate_factors[MagickPathExtent];

  static char
    brightness_percent[MagickPathExtent] = "110";

  /*
    Query user for percent brightness change.
  */
  {
    const XDialogRequest
      dialog_request = { "Apply",
        "Enter percent change in color brightness (0-200):",
        brightness_percent };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*brightness_percent == '\0')
    return;
  /*
    Vary the color brightness.
  */
  XApplyBrightnessToImage(display,windows,modulate_factors,brightness_percent,
    image,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyGammaToImage(const XMagickCommandContext *context,
  char *factor)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) GammaImage(*image,strtod(factor,(char **) NULL),exception);
}

static void XApplyGammaCommand(const XMagickCommandContext *context)
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

  static char
    factor[MagickPathExtent] = "1.6";

  /*
    Query user for gamma value.
  */
  {
    const XDialogRequest
      dialog_request = { "Gamma", "Enter gamma value (e.g. 1.2):", factor };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*factor == '\0')
    return;
  /*
    Gamma correct image.
  */
  XApplyGammaToImage(context,factor);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySpiffToImage(Display *display,XWindows *windows,Image **image,
  ExceptionInfo *exception)
{
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) ContrastImage(*image,MagickTrue,exception);
}

static void XApplySpiffCommand(const XMagickCommandContext *context)
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

  /*
    Sharpen the image contrast.
  */
  XApplySpiffToImage(display,windows,image,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyDullToImage(Display *display,XWindows *windows,Image **image,
  ExceptionInfo *exception)
{
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) ContrastImage(*image,MagickFalse,exception);
}

static void XApplyDullCommand(const XMagickCommandContext *context)
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

  /*
    Dull the image contrast.
  */
  XApplyDullToImage(display,windows,image,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyContrastStretchCommand(const XMagickCommandContext *context)
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

  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  double
    black_point,
    white_point;

  static char
    levels[MagickPathExtent] = "1%";

  /*
    Query user for gamma value.
  */
  {
    const XDialogRequest
      dialog_request = { "Contrast Stretch", "Enter black and white points:",
        levels };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*levels == '\0')
    return;
  /*
    Contrast stretch image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  flags=ParseGeometry(levels,&geometry_info);
  black_point=geometry_info.rho;
  white_point=(flags & SigmaValue) != 0 ? geometry_info.sigma : black_point;
  if ((flags & PercentValue) != 0)
    {
      black_point*=(double) (*image)->columns*(*image)->rows/100.0;
      white_point*=(double) (*image)->columns*(*image)->rows/100.0;
    }
  white_point=(double) (*image)->columns*(*image)->rows-white_point;
  (void) ContrastStretchImage(*image,black_point,white_point,
    exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySigmoidalContrastCommand(const XMagickCommandContext *context)
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

  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  static char
    levels[MagickPathExtent] = "3x50%";

  /*
    Query user for gamma value.
  */
  {
    const XDialogRequest
      dialog_request = { "Sigmoidal Contrast", "Enter contrast and midpoint:",
        levels };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*levels == '\0')
    return;
  /*
    Contrast stretch image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  flags=ParseGeometry(levels,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=1.0*(double) QuantumRange/2.0;
  if ((flags & PercentValue) != 0)
    geometry_info.sigma=1.0*(double) QuantumRange*geometry_info.sigma/100.0;
  (void) SigmoidalContrastImage(*image,MagickTrue,geometry_info.rho,
    geometry_info.sigma,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyNormalizeToImage(Display *display,XWindows *windows,
  Image **image,ExceptionInfo *exception)
{
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) NormalizeImage(*image,exception);
}

static void XApplyNormalizeCommand(const XMagickCommandContext *context)
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

  /*
    Perform histogram normalization on the image.
  */
  XApplyNormalizeToImage(display,windows,image,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyEqualizeToImage(Display *display,XWindows *windows,
  Image **image,ExceptionInfo *exception)
{
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) EqualizeImage(*image,exception);
}

static void XApplyEqualizeCommand(const XMagickCommandContext *context)
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

  /*
    Perform histogram equalization on the image.
  */
  XApplyEqualizeToImage(display,windows,image,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyNegateToImage(Display *display,XWindows *windows,
  Image **image,ExceptionInfo *exception)
{
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) NegateImage(*image,MagickFalse,exception);
}

static void XApplyNegateCommand(const XMagickCommandContext *context)
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

  /*
    Negate colors in image.
  */
  XApplyNegateToImage(display,windows,image,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyGrayscaleToImage(Display *display,XWindows *windows,
  Image **image,ExceptionInfo *exception)
{
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) SetImageType(*image,(*image)->alpha_trait == UndefinedPixelTrait ?
    GrayscaleType : GrayscaleAlphaType,exception);
}

static void XApplyGrayscaleCommand(const XMagickCommandContext *context)
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

  /*
    Convert image to grayscale.
  */
  XApplyGrayscaleToImage(display,windows,image,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyResizeCommand(const XMagickCommandContext *context)
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
    geometry[MagickPathExtent];

  int
    status;

  size_t
    height,
    width;

  ssize_t
    x,
    y;

  /*
    Resize image.
  */
  width=(size_t) windows->image.ximage->width;
  height=(size_t) windows->image.ximage->height;
  x=0;
  y=0;
  (void) FormatLocaleString(geometry,MagickPathExtent,"%.17gx%.17g+0+0",
    (double) width,(double) height);
  {
    const XDialogRequest
      dialog_request = { "Resize",
        "Enter resize geometry (e.g. 640x480, 200%):", geometry };

    status=XDialogWidget(display,windows,&dialog_request);
  }
  if (*geometry == '\0')
    return;
  if (status == 0)
    (void) ConcatenateMagickString(geometry,"!",MagickPathExtent);
  (void) ParseMetaGeometry(geometry,&x,&y,&width,&height);
  windows->image.window_changes.width=(int) width;
  windows->image.window_changes.height=(int) height;
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XQuitDisplayProgram(XResourceInfo *resource_info,Display *display,
  XWindows *windows)
{
  if (resource_info->confirm_exit == MagickFalse)
    XClientMessage(display,windows->image.id,windows->im_protocols,
      windows->im_exit,CurrentTime);
  else
    {
      int
        status;

      /*
        Confirm program exit.
      */
      status=XConfirmWidget(display,windows,"Do you really want to exit",
        resource_info->client_name);
      if (status > 0)
        XClientMessage(display,windows->image.id,windows->im_protocols,
          windows->im_exit,CurrentTime);
    }
}

static void XApplyQuitCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  /*
    exit program.
  */
  XQuitDisplayProgram(resource_info,display,windows);
}

static void XApplyDespeckleToImage(Display *display,XWindows *windows,
  Image **image,ExceptionInfo *exception)
{
  Image
    *despeckle_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  despeckle_image=DespeckleImage(*image,exception);
  if (despeckle_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=despeckle_image;
    }
}

static void XApplyDespeckleCommand(const XMagickCommandContext *context)
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

  /*
    Despeckle image.
  */
  XApplyDespeckleToImage(display,windows,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyMapCommand(const XMagickCommandContext *context,
  QuantizeInfo quantize_info)
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

  Image
    *affinity_image;

  static char
    filename[MagickPathExtent] = "\0";

  /*
    Request image file name from user.
  */
  XFileBrowserWidget(display,windows,"Map",filename);
  if (*filename == '\0')
    return;
  /*
    Map image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  affinity_image=ReadImage(image_info,exception);
  if (affinity_image != (Image *) NULL)
    {
      (void) RemapImage(&quantize_info,*image,affinity_image,exception);
      affinity_image=DestroyImage(affinity_image);
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyQuantizeCommand(const XMagickCommandContext *context,
  QuantizeInfo quantize_info)
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

  int
    status;

  static char
    colors[MagickPathExtent] = "256";

  /*
    Query user for maximum number of colors.
  */
  {
    const XDialogRequest
      dialog_request = { "Quantize", "Maximum number of colors:", colors };

    status=XDialogWidget(display,windows,&dialog_request);
  }
  if (*colors == '\0')
    return;
  /*
    Color reduce the image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  quantize_info.number_colors=StringToUnsignedLong(colors);
  quantize_info.dither_method=status != 0 ? RiemersmaDitherMethod :
    NoDitherMethod;
  (void) QuantizeImage(&quantize_info,*image,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyReduceNoiseToImage(const XMagickCommandContext *context,
  MagickStatusType *flags,char *radius)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  GeometryInfo
    geometry_info;

  Image
    *noise_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(radius,&geometry_info);
  noise_image=StatisticImage(*image,NonpeakStatistic,(size_t)
    geometry_info.rho,(size_t) geometry_info.rho,exception);
  if (noise_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=noise_image;
    }
}

#endif
