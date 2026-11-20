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

  MagickCore private methods of display-image.c: the effect, F/X, image-edit
  and miscellany command appliers, from XApplyReduceNoiseCommand to
  XApplyRestoreCommand; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_APPLY_EFFECTS_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_APPLY_EFFECTS_PRIVATE_H

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
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
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
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
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

static void XApplyShadeToImage(Display *display,XWindows *windows,
  char *geometry,Image **image,int status,ExceptionInfo *exception)
{
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
  XApplyShadeToImage(display,windows,geometry,image,status,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyRaiseToImage(Display *display,XWindows *windows,Image **image,
  char *bevel_width,ExceptionInfo *exception)
{
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
  (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
    exception);
  XApplyRaiseToImage(display,windows,image,bevel_width,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
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
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
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
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySolarizeToImage(Display *display,XWindows *windows,
  char *factor,Image **image,ExceptionInfo *exception)
{
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
  XApplySolarizeToImage(display,windows,factor,image,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
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

static void XApplyVignetteToImage(Display *display,XWindows *windows,
  char *geometry,Image **image,ExceptionInfo *exception)
{
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
  XApplyVignetteToImage(display,windows,geometry,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyRollToImage(Display *display,XWindows *windows,Image **image,
  char *geometry,ExceptionInfo *exception)
{
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
  (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
    exception);
  XApplyRollToImage(display,windows,image,geometry,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  windows->image.window_changes.width=(int) (*image)->columns;
  windows->image.window_changes.height=(int) (*image)->rows;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyHueToImage(Display *display,XWindows *windows,
  char *modulate_factors,char *hue_percent,Image **image,
  ExceptionInfo *exception)
{
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
  XApplyHueToImage(display,windows,modulate_factors,hue_percent,image,
    exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyEmbossToImage(Display *display,XWindows *windows,char *radius,
  Image **image,ExceptionInfo *exception)
{
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
  XApplyEmbossToImage(display,windows,radius,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyWaveToImage(Display *display,XWindows *windows,char *geometry,
  Image **image,ExceptionInfo *exception)
{
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
  XApplyWaveToImage(display,windows,geometry,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
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

static void XApplyCharcoalDrawToImage(Display *display,XWindows *windows,
  char *radius,Image **image,ExceptionInfo *exception)
{
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
  (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
    exception);
  XApplyCharcoalDrawToImage(display,windows,radius,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
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
  if (status == MagickFalse)
    {
      XNoticeWidget(display,windows,"Unable to annotate X image",
        (*image)->filename);
      return;
    }
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

  /*
    Draw image.
  */
  status=XDrawEditImage(display,resource_info,windows,image,exception);
  if (status == MagickFalse)
    {
      XNoticeWidget(display,windows,"Unable to draw on the X image",
        (*image)->filename);
      return;
    }
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
  if (status == MagickFalse)
    {
      XNoticeWidget(display,windows,"Unable to pixel edit X image",
        (*image)->filename);
      return;
    }
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
  if (status == MagickFalse)
    {
      XNoticeWidget(display,windows,"Unable to matte edit X image",
        (*image)->filename);
      return;
    }
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
  if (status == MagickFalse)
    {
      XNoticeWidget(display,windows,"Unable to composite X image",
        (*image)->filename);
      return;
    }
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
  (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
    exception);
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
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
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
  (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
    exception);
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
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
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
      XClientMessage(display,windows->image.id,windows->im_protocols,
        windows->im_next_image,CurrentTime);
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

static void XShowImagePreview(Display *display,XWindows *windows,
  char *preview_type,Image **image,char *filename,ImageInfo *image_info,
  ExceptionInfo *exception)
{
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

  Image
    **const image = context->image;

  ImageInfo
    *const image_info = context->image_info;

  ExceptionInfo
    *const exception = context->exception;

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
  XShowImagePreview(display,windows,preview_type,image,filename,image_info,
    exception);
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
    XClientMessage(display,windows->image.id,windows->im_protocols,
      windows->im_next_image,CurrentTime);
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
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_next_image,CurrentTime);
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
    XClientMessage(display,windows->image.id,windows->im_protocols,
      windows->im_next_image,CurrentTime);
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

static void XApplyAddNoiseToImage(Display *display,XWindows *windows,
  Image **image,char *noise_type,ExceptionInfo *exception)
{
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
  XApplyAddNoiseToImage(display,windows,image,noise_type,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
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
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

#endif
