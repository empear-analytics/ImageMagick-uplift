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

  MagickCore private methods of display-image.c: the dialog-command runner and
  the filter, effect and F/X command appliers, from XRunDialogCommand to
  XApplyCharcoalDrawCommand; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_APPLY_FILTERS_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_APPLY_FILTERS_PRIVATE_H

typedef void
  (*XDialogApplyMethod)(const XMagickCommandContext *,MagickStatusType *,
    char *);

typedef struct _XDialogCommand
{
  XDialogRequest
    request;

  XDialogApplyMethod
    apply;
} XDialogCommand;

static void XRunDialogCommand(const XMagickCommandContext *context,
  const XDialogCommand *command,MagickStatusType *flags)
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
    *const reply = command->request.reply;

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for the command's argument, then apply the command.
  */
  (void) XDialogWidget(display,windows,&command->request);
  if (*reply == '\0')
    return;
  command->apply(context,flags,reply);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyReduceNoiseCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  static char
    radius[MagickPathExtent] = "0";

  const XDialogCommand
    command = { { "Reduce Noise", "Enter the noise radius:", radius },
      XApplyReduceNoiseToImage };

  /*
    Query user for noise radius.
    Reduce noise in the image.
  */
  XRunDialogCommand(context,&command,flags);
}

static void XApplySharpenToImage(const XMagickCommandContext *context,
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
    *sharp_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(radius,&geometry_info);
  sharp_image=SharpenImage(*image,geometry_info.rho,geometry_info.sigma,
    exception);
  if (sharp_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=sharp_image;
    }
}

static void XApplySharpenCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  static char
    radius[MagickPathExtent] = "0.0x1.0";

  const XDialogCommand
    command = { { "Sharpen", "Enter the sharpen radius and standard deviation:",
      radius }, XApplySharpenToImage };

  /*
    Query user for sharpen radius.
    Sharpen image scanlines.
  */
  XRunDialogCommand(context,&command,flags);
}

static void XApplyBlurToImage(const XMagickCommandContext *context,
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
    *blur_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(radius,&geometry_info);
  blur_image=BlurImage(*image,geometry_info.rho,geometry_info.sigma,
    exception);
  if (blur_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=blur_image;
    }
}

static void XApplyBlurCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  static char
    radius[MagickPathExtent] = "0.0x1.0";

  const XDialogCommand
    command = { { "Blur", "Enter the blur radius and standard deviation:",
      radius }, XApplyBlurToImage };

  /*
    Query user for blur radius.
    Blur an image.
  */
  XRunDialogCommand(context,&command,flags);
}

static void XApplyThresholdToImage(const XMagickCommandContext *context,
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

  double
    threshold;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  threshold=StringToDoubleInterval(factor,(double) QuantumRange+1.0);
  (void) BilevelImage(*image,threshold,exception);
}

static void XApplyThresholdCommand(const XMagickCommandContext *context)
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
    factor[MagickPathExtent] = "128";

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for threshold value.
  */
  {
    const XDialogRequest
      dialog_request = { "Threshold", "Enter threshold value:", factor };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*factor == '\0')
    return;
  /*
    Gamma correct image.
  */
  XApplyThresholdToImage(context,factor);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyEdgeDetectToImage(const XMagickCommandContext *context,
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
    *edge_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(radius,&geometry_info);
  edge_image=EdgeImage(*image,geometry_info.rho,exception);
  if (edge_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=edge_image;
    }
}

static void XApplyEdgeDetectCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  static char
    radius[MagickPathExtent] = "0";

  const XDialogCommand
    command = { { "Detect Edges", "Enter the edge detect radius:", radius },
      XApplyEdgeDetectToImage };

  /*
    Query user for edge factor.
    Detect edge in image.
  */
  XRunDialogCommand(context,&command,flags);
}

static void XApplySpreadToImage(const XMagickCommandContext *context,
  MagickStatusType *flags,char *amount)
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
    *spread_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(amount,&geometry_info);
  spread_image=EdgeImage(*image,geometry_info.rho,exception);
  if (spread_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=spread_image;
    }
}

static void XApplySpreadCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  static char
    amount[MagickPathExtent] = "2";

  const XDialogCommand
    command = { { "Spread", "Enter the displacement amount:", amount },
      XApplySpreadToImage };

  /*
    Query user for spread amount.
    Displace image pixels by a random amount.
  */
  XRunDialogCommand(context,&command,flags);
}

static void XApplyShadeToImage(const XMagickCommandContext *context,
  char *geometry,int status)
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

  MagickStatusType
    flags;

  Image
    *shade_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  flags=ParseGeometry(geometry,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=1.0;
  shade_image=ShadeImage(*image,(MagickBooleanType) (status != 0),
    geometry_info.rho,geometry_info.sigma,exception);
  if (shade_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=shade_image;
    }
}

static void XApplyShadeCommand(const XMagickCommandContext *context)
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
    geometry[MagickPathExtent] = "30x30";

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for the shade geometry.
  */
  {
    const XDialogRequest
      dialog_request = { "Shade",
        "Enter the azimuth and elevation of the light source:", geometry };

    status=XDialogWidget(display,windows,&dialog_request);
  }
  if (*geometry == '\0')
    return;
  /*
    Shade image pixels.
  */
  XApplyShadeToImage(context,geometry,status);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyRaiseToImage(const XMagickCommandContext *context,
  char *bevel_width)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  RectangleInfo
    page_geometry;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) ParsePageGeometry(*image,bevel_width,&page_geometry,
    exception);
  (void) RaiseImage(*image,&page_geometry,MagickTrue,exception);
}

static void XApplyRaiseCommand(const XMagickCommandContext *context)
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
    bevel_width[MagickPathExtent] = "10";

  const XImageCacheContext
    command_context = { display, resource_info, windows, image, exception };

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for bevel width.
  */
  {
    const XDialogRequest
      dialog_request = { "Raise", "Bevel width:", bevel_width };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*bevel_width == '\0')
    return;
  /*
    Raise an image.
  */
  (void) XMagickCommand(&command_context,ApplyCommand);
  XApplyRaiseToImage(context,bevel_width);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySegmentToImage(const XMagickCommandContext *context,
  char *threshold)
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

  MagickStatusType
    flags;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  flags=ParseGeometry(threshold,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=1.0;
  (void) SegmentImage(*image,sRGBColorspace,MagickFalse,geometry_info.rho,
    geometry_info.sigma,exception);
}

static void XApplySegmentCommand(const XMagickCommandContext *context)
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
    threshold[MagickPathExtent] = "1.0x1.5";

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for smoothing threshold.
  */
  {
    const XDialogRequest
      dialog_request = { "Segment", "Smooth threshold:", threshold };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*threshold == '\0')
    return;
  /*
    Segment an image.
  */
  XApplySegmentToImage(context,threshold);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySepiaToneToImage(const XMagickCommandContext *context,
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

  double
    threshold;

  Image
    *sepia_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  threshold=StringToDoubleInterval(factor,(double) QuantumRange+1.0);
  sepia_image=SepiaToneImage(*image,threshold,exception);
  if (sepia_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=sepia_image;
    }
}

static void XApplySepiaToneCommand(const XMagickCommandContext *context)
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
    factor[MagickPathExtent] = "80%";

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for sepia-tone factor.
  */
  {
    const XDialogRequest
      dialog_request = { "Sepia Tone",
        "Enter the sepia tone factor (0 - 99.9%):", factor };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*factor == '\0')
    return;
  /*
    Sepia tone image pixels.
  */
  XApplySepiaToneToImage(context,factor);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySolarizeToImage(const XMagickCommandContext *context,
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

  double
    threshold;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  threshold=StringToDoubleInterval(factor,(double) QuantumRange+1.0);
  (void) SolarizeImage(*image,threshold,exception);
}

static void XApplySolarizeCommand(const XMagickCommandContext *context)
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
    factor[MagickPathExtent] = "60%";

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for solarize factor.
  */
  {
    const XDialogRequest
      dialog_request = { "Solarize", "Enter the solarize factor (0 - 99.9%):",
        factor };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*factor == '\0')
    return;
  /*
    Solarize image pixels.
  */
  XApplySolarizeToImage(context,factor);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySwirlToImage(const XMagickCommandContext *context,
  MagickStatusType *flags,char *degrees)
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
    *swirl_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(degrees,&geometry_info);
  swirl_image=SwirlImage(*image,geometry_info.rho,(*image)->interpolate,
    exception);
  if (swirl_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=swirl_image;
    }
}

static void XApplySwirlCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  static char
    degrees[MagickPathExtent] = "60";

  const XDialogCommand
    command = { { "Swirl", "Enter the swirl angle:", degrees },
      XApplySwirlToImage };

  /*
    Query user for swirl angle.
    Swirl image pixels about the center.
  */
  XRunDialogCommand(context,&command,flags);
}

static void XApplyImplodeToImage(const XMagickCommandContext *context,
  MagickStatusType *flags,char *factor)
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
    *implode_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(factor,&geometry_info);
  implode_image=ImplodeImage(*image,geometry_info.rho,(*image)->interpolate,
    exception);
  if (implode_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=implode_image;
    }
}

static void XApplyImplodeCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  static char
    factor[MagickPathExtent] = "0.3";

  const XDialogCommand
    command = { { "Implode",
      "Enter the implosion/explosion factor (-1.0 - 1.0):", factor },
      XApplyImplodeToImage };

  /*
    Query user for implode factor.
    Implode image pixels about the center.
  */
  XRunDialogCommand(context,&command,flags);
}

static void XApplyVignetteToImage(const XMagickCommandContext *context,
  char *geometry)
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

  MagickStatusType
    flags;

  Image
    *vignette_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  flags=ParseGeometry(geometry,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=1.0;
  if ((flags & XiValue) == 0)
    geometry_info.xi=0.1*(*image)->columns;
  if ((flags & PsiValue) == 0)
    geometry_info.psi=0.1*(*image)->rows;
  vignette_image=VignetteImage(*image,geometry_info.rho,0.0,(ssize_t)
    ceil(geometry_info.xi-0.5),(ssize_t) ceil(geometry_info.psi-0.5),
    exception);
  if (vignette_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=vignette_image;
    }
}

static void XApplyVignetteCommand(const XMagickCommandContext *context)
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
    geometry[MagickPathExtent] = "0x20";

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for the vignette geometry.
  */
  {
    const XDialogRequest
      dialog_request = { "Vignette",
        "Enter the radius, sigma, and x and y offsets:", geometry };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*geometry == '\0')
    return;
  /*
    Soften the edges of the image in vignette style
  */
  XApplyVignetteToImage(context,geometry);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyRollToImage(const XMagickCommandContext *context,
  char *geometry)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  RectangleInfo
    page_geometry;

  Image
    *roll_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) ParsePageGeometry(*image,geometry,&page_geometry,
    exception);
  roll_image=RollImage(*image,page_geometry.x,page_geometry.y,
    exception);
  if (roll_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=roll_image;
    }
}

static void XApplyRollCommand(const XMagickCommandContext *context)
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
    geometry[MagickPathExtent] = "+2+2";

  const XImageCacheContext
    command_context = { display, resource_info, windows, image, exception };

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for the roll geometry.
  */
  {
    const XDialogRequest
      dialog_request = { "Roll", "Enter roll geometry:", geometry };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*geometry == '\0')
    return;
  /*
    Roll image.
  */
  (void) XMagickCommand(&command_context,ApplyCommand);
  XApplyRollToImage(context,geometry);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  windows->image.window_changes.width=(int) (*image)->columns;
  windows->image.window_changes.height=(int) (*image)->rows;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyHueToImage(const XMagickCommandContext *context,
  char *modulate_factors,char *hue_percent)
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
  (void) CopyMagickString(modulate_factors,"100.0/100.0/",MagickPathExtent);
  (void) ConcatenateMagickString(modulate_factors,hue_percent,
    MagickPathExtent);
  (void) ModulateImage(*image,modulate_factors,exception);
}

static void XApplyHueCommand(const XMagickCommandContext *context)
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
    hue_percent[MagickPathExtent] = "110";

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for percent hue change.
  */
  {
    const XDialogRequest
      dialog_request = { "Apply", "Enter percent change in image hue (0-200):",
        hue_percent };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*hue_percent == '\0')
    return;
  /*
    Vary the image hue.
  */
  XApplyHueToImage(context,modulate_factors,hue_percent);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyEmbossToImage(const XMagickCommandContext *context,
  char *radius)
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

  MagickStatusType
    flags;

  Image
    *emboss_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  flags=ParseGeometry(radius,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=1.0;
  emboss_image=EmbossImage(*image,geometry_info.rho,geometry_info.sigma,
    exception);
  if (emboss_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=emboss_image;
    }
}

static void XApplyEmbossCommand(const XMagickCommandContext *context)
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
    radius[MagickPathExtent] = "0.0x1.0";

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for emboss radius.
  */
  {
    const XDialogRequest
      dialog_request = { "Emboss",
        "Enter the emboss radius and standard deviation:", radius };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*radius == '\0')
    return;
  /*
    Reduce noise in the image.
  */
  XApplyEmbossToImage(context,radius);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyWaveToImage(const XMagickCommandContext *context,
  char *geometry)
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

  MagickStatusType
    flags;

  Image
    *wave_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  flags=ParseGeometry(geometry,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=1.0;
  wave_image=WaveImage(*image,geometry_info.rho,geometry_info.sigma,
    (*image)->interpolate,exception);
  if (wave_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=wave_image;
    }
}

static void XApplyWaveCommand(const XMagickCommandContext *context)
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
    geometry[MagickPathExtent] = "25x150";

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for the wave geometry.
  */
  {
    const XDialogRequest
      dialog_request = { "Wave", "Enter the amplitude and length of the wave:",
        geometry };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*geometry == '\0')
    return;
  /*
    Alter an image along a sine wave.
  */
  XApplyWaveToImage(context,geometry);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyOilPaintToImage(const XMagickCommandContext *context,
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
    *paint_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(radius,&geometry_info);
  paint_image=OilPaintImage(*image,geometry_info.rho,geometry_info.sigma,
    exception);
  if (paint_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=paint_image;
    }
}

static void XApplyOilPaintCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  static char
    radius[MagickPathExtent] = "0";

  const XDialogCommand
    command = { { "Oil Paint", "Enter the mask radius:", radius },
      XApplyOilPaintToImage };

  /*
    Query user for circular neighborhood radius.
    OilPaint image scanlines.
  */
  XRunDialogCommand(context,&command,flags);
}

static void XApplyCharcoalDrawToImage(const XMagickCommandContext *context,
  char *radius)
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

  MagickStatusType
    flags;

  Image
    *charcoal_image;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  flags=ParseGeometry(radius,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=geometry_info.rho;
  charcoal_image=CharcoalImage(*image,geometry_info.rho,geometry_info.sigma,
    exception);
  if (charcoal_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=charcoal_image;
    }
}

static void XApplyCharcoalDrawCommand(const XMagickCommandContext *context)
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
    radius[MagickPathExtent] = "0x1";

  const XImageCacheContext
    command_context = { display, resource_info, windows, image, exception };

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Query user for charcoal radius.
  */
  {
    const XDialogRequest
      dialog_request = { "Charcoal Draw",
        "Enter the charcoal radius and sigma:", radius };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*radius == '\0')
    return;
  /*
    Charcoal the image.
  */
  (void) XMagickCommand(&command_context,ApplyCommand);
  XApplyCharcoalDrawToImage(context,radius);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(&cmap_target,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

#endif
