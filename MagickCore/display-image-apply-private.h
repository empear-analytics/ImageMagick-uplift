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

  MagickCore private methods of display-image.c: the remaining command
  appliers, the command-group dispatchers of XMagickCommand and the magnify
  window, from XApplyReduceNoiseCommand to XMoveMagnifyRightDown; included by
  that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_APPLY_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_APPLY_PRIVATE_H

static void XApplyReduceNoiseCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
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
    radius[MagickPathExtent] = "0";

  /*
    Query user for noise radius.
  */
  (void) XDialogWidget(display,windows,"Reduce Noise",
    "Enter the noise radius:",radius);
  if (*radius == '\0')
    return;
  /*
    Reduce noise in the image.
  */
  XApplyReduceNoiseToImage(display,windows,flags,radius,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySharpenToImage(Display *display,XWindows *windows,
  MagickStatusType *flags,char *radius,Image **image,ExceptionInfo *exception)
{
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
    Query user for sharpen radius.
  */
  (void) XDialogWidget(display,windows,"Sharpen",
    "Enter the sharpen radius and standard deviation:",radius);
  if (*radius == '\0')
    return;
  /*
    Sharpen image scanlines.
  */
  XApplySharpenToImage(display,windows,flags,radius,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyBlurToImage(Display *display,XWindows *windows,
  MagickStatusType *flags,char *radius,Image **image,ExceptionInfo *exception)
{
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
    Query user for blur radius.
  */
  (void) XDialogWidget(display,windows,"Blur",
    "Enter the blur radius and standard deviation:",radius);
  if (*radius == '\0')
    return;
  /*
    Blur an image.
  */
  XApplyBlurToImage(display,windows,flags,radius,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyThresholdToImage(Display *display,XWindows *windows,
  char *factor,Image **image,ExceptionInfo *exception)
{
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
  (void) XDialogWidget(display,windows,"Threshold",
    "Enter threshold value:",factor);
  if (*factor == '\0')
    return;
  /*
    Gamma correct image.
  */
  XApplyThresholdToImage(display,windows,factor,image,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyEdgeDetectToImage(Display *display,XWindows *windows,
  MagickStatusType *flags,char *radius,Image **image,ExceptionInfo *exception)
{
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
    radius[MagickPathExtent] = "0";

  /*
    Query user for edge factor.
  */
  (void) XDialogWidget(display,windows,"Detect Edges",
    "Enter the edge detect radius:",radius);
  if (*radius == '\0')
    return;
  /*
    Detect edge in image.
  */
  XApplyEdgeDetectToImage(display,windows,flags,radius,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySpreadToImage(Display *display,XWindows *windows,
  MagickStatusType *flags,char *amount,Image **image,ExceptionInfo *exception)
{
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
    amount[MagickPathExtent] = "2";

  /*
    Query user for spread amount.
  */
  (void) XDialogWidget(display,windows,"Spread",
    "Enter the displacement amount:",amount);
  if (*amount == '\0')
    return;
  /*
    Displace image pixels by a random amount.
  */
  XApplySpreadToImage(display,windows,flags,amount,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
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
  status=XDialogWidget(display,windows,"Shade",
    "Enter the azimuth and elevation of the light source:",geometry);
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
  (void) XDialogWidget(display,windows,"Raise","Bevel width:",bevel_width);
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

static void XApplySegmentToImage(Display *display,XWindows *windows,
  char *threshold,Image **image,ExceptionInfo *exception)
{
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
  (void) XDialogWidget(display,windows,"Segment","Smooth threshold:",
    threshold);
  if (*threshold == '\0')
    return;
  /*
    Segment an image.
  */
  XApplySegmentToImage(display,windows,threshold,image,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySepiaToneToImage(Display *display,XWindows *windows,
  char *factor,Image **image,ExceptionInfo *exception)
{
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
  (void) XDialogWidget(display,windows,"Sepia Tone",
    "Enter the sepia tone factor (0 - 99.9%):",factor);
  if (*factor == '\0')
    return;
  /*
    Sepia tone image pixels.
  */
  XApplySepiaToneToImage(display,windows,factor,image,exception);
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
  (void) XDialogWidget(display,windows,"Solarize",
    "Enter the solarize factor (0 - 99.9%):",factor);
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

static void XApplySwirlToImage(Display *display,XWindows *windows,
  MagickStatusType *flags,char *degrees,Image **image,ExceptionInfo *exception)
{
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
    degrees[MagickPathExtent] = "60";

  /*
    Query user for swirl angle.
  */
  (void) XDialogWidget(display,windows,"Swirl","Enter the swirl angle:",
    degrees);
  if (*degrees == '\0')
    return;
  /*
    Swirl image pixels about the center.
  */
  XApplySwirlToImage(display,windows,flags,degrees,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyImplodeToImage(Display *display,XWindows *windows,
  MagickStatusType *flags,char *factor,Image **image,ExceptionInfo *exception)
{
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
    factor[MagickPathExtent] = "0.3";

  /*
    Query user for implode factor.
  */
  (void) XDialogWidget(display,windows,"Implode",
    "Enter the implosion/explosion factor (-1.0 - 1.0):",factor);
  if (*factor == '\0')
    return;
  /*
    Implode image pixels about the center.
  */
  XApplyImplodeToImage(display,windows,flags,factor,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
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
  (void) XDialogWidget(display,windows,"Vignette",
    "Enter the radius, sigma, and x and y offsets:",geometry);
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
  (void) XDialogWidget(display,windows,"Roll","Enter roll geometry:",
    geometry);
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
  (void) XDialogWidget(display,windows,"Apply",
    "Enter percent change in image hue (0-200):",hue_percent);
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
  (void) XDialogWidget(display,windows,"Emboss",
    "Enter the emboss radius and standard deviation:",radius);
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
  (void) XDialogWidget(display,windows,"Wave",
    "Enter the amplitude and length of the wave:",geometry);
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

static void XApplyOilPaintToImage(Display *display,XWindows *windows,
  MagickStatusType *flags,char *radius,Image **image,ExceptionInfo *exception)
{
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
    radius[MagickPathExtent] = "0";

  /*
    Query user for circular neighborhood radius.
  */
  (void) XDialogWidget(display,windows,"Oil Paint",
    "Enter the mask radius:",radius);
  if (*radius == '\0')
    return;
  /*
    OilPaint image scanlines.
  */
  XApplyOilPaintToImage(display,windows,flags,radius,image,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
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
  (void) XDialogWidget(display,windows,"Charcoal Draw",
    "Enter the charcoal radius and sigma:",radius);
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
  (void) XDialogWidget(display,windows,"Add Border",
    "Enter border geometry:",geometry);
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
  (void) XDialogWidget(display,windows,"Add Frame","Enter frame geometry:",
    geometry);
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
  XListBrowserWidget(display,windows,&windows->widget,
    (const char **) previews,"Preview",
    "Select an enhancement, effect, or F/X:",preview_type);
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
  (void) XDialogWidget(display,windows,"Slide Show",
    "Pause how many 1/100ths of a second between images:",delay);
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
  XListBrowserWidget(display,windows,&windows->widget,
    (const char **) noises,"Add Noise",
    "Select a type of noise to add to your image:",noise_type);
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

static void XApplySaveCommand(const XMagickCommandContext *context)
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
      return;
    }
}

static Image *XApplyNewCommand(const XMagickCommandContext *context,
  Image *nexus)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  ImageInfo
    *const image_info = context->image_info;

  ExceptionInfo
    *const exception = context->exception;

  int
    status;

  static char
    color[MagickPathExtent] = "gray",
    geometry[MagickPathExtent] = "640x480";

  static const char
    *format = "gradient";

  /*
    Query user for canvas geometry.
  */
  status=XDialogWidget(display,windows,"New","Enter image geometry:",
    geometry);
  if (*geometry == '\0')
    return(nexus);
  if (status == 0)
    format="xc";
  XColorBrowserWidget(display,windows,"Select",color);
  if (*color == '\0')
    return(nexus);
  /*
    Create canvas.
  */
  (void) FormatLocaleString(image_info->filename,MagickPathExtent,
    "%s:%s",format,color);
  (void) CloneString(&image_info->size,geometry);
  nexus=ReadImage(image_info,exception);
  CatchException(exception);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_next_image,CurrentTime);
  return(nexus);
}

static void XApplyShearCommand(const XMagickCommandContext *context,char *color)
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

  Image
    *shear_image;

  static char
    geometry[MagickPathExtent] = "45.0x45.0";

  /*
    Query user for shear color and geometry.
  */
  XColorBrowserWidget(display,windows,"Select",color);
  if (*color == '\0')
    return;
  (void) XDialogWidget(display,windows,"Shear","Enter shear geometry:",
    geometry);
  if (*geometry == '\0')
    return;
  /*
    Shear image.
  */
  (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
    exception);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) QueryColorCompliance(color,AllCompliance,
    &(*image)->background_color,exception);
  flags=ParseGeometry(geometry,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=geometry_info.rho;
  shear_image=ShearImage(*image,geometry_info.rho,geometry_info.sigma,
    exception);
  if (shear_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=shear_image;
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

static void XApplyCropAndScale(char *image_geometry,XWindows *windows,
  Image **image,Display *display,XResourceInfo *resource_info,
  ExceptionInfo *exception)
{
  (void) FormatLocaleString(image_geometry,MagickPathExtent,"%dx%d!",
    windows->image.ximage->width,windows->image.ximage->height);
  (void) TransformImage(image,windows->image.crop_geometry,image_geometry,
    exception);
  if (windows->image.crop_geometry != (char *) NULL)
    windows->image.crop_geometry=(char *) RelinquishMagickMemory(
      windows->image.crop_geometry);
  windows->image.x=0;
  windows->image.y=0;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyApplyCommand(const XMagickCommandContext *context)
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
    image_geometry[MagickPathExtent];

  if (IsImageWindowUncropped(windows,image) != MagickFalse)
    return;
  /*
    Apply size transforms to image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  /*
    Crop and/or scale displayed image.
  */
  XApplyCropAndScale(image_geometry,windows,image,display,resource_info,
    exception);
}

static void XApplyRegionOfInterestCommand(const XMagickCommandContext *context)
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
    Apply an image processing technique to a region of interest.
  */
  (void) XROIImage(display,resource_info,windows,image,exception);
}

static void XApplySessionCommands(const XMagickCommandContext *context,
  const DisplayCommand command,Image **nexus)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  switch (command)
  {
    case SlideShowCommand:
      XApplySlideShowCommand(context);
      break;
    case PreferencesCommand:
      (*nexus)=XApplyPreferencesCommand(context,(*nexus));
      break;
    case HelpCommand:
    {
      /*
        User requested help.
      */
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Display",DisplayHelp);
      break;
    }
    case BrowseDocumentationCommand:
      XApplyBrowseDocumentationCommand(context);
      break;
    case VersionCommand:
    {
      XNoticeWidget(display,windows,GetMagickVersion((size_t *) NULL),
        GetMagickCopyright());
      break;
    }
    case SaveToUndoBufferCommand:
      break;
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
}

static void XApplyMiscellanyCommands(const XMagickCommandContext *context,
  const DisplayCommand command,Image **nexus)
{
  switch (command)
  {
    case InfoCommand:
      break;
    case ZoomCommand:
      XApplyZoomCommand(context);
      break;
    case ShowPreviewCommand:
      XApplyShowPreviewCommand(context);
      break;
    case ShowHistogramCommand:
      XApplyShowHistogramCommand(context);
      break;
    case ShowMatteCommand:
      XApplyShowMatteCommand(context);
      break;
    case BackgroundCommand:
      (*nexus)=XApplyBackgroundCommand(context,(*nexus));
      break;
    default:
    {
      XApplySessionCommands(context,command,&(*nexus));
      break;
    }
  }
}

static void XApplyImageEditCommands(const XMagickCommandContext *context,
  const DisplayCommand command,Image **nexus)
{
  char
    *const color = context->color;

  switch (command)
  {
    case CompositeCommand:
      XApplyCompositeCommand(context);
      break;
    case AddBorderCommand:
      XApplyAddBorderCommand(context,color);
      break;
    case AddFrameCommand:
      XApplyAddFrameCommand(context,color);
      break;
    case CommentCommand:
      XApplyCommentCommand(context);
      break;
    case LaunchCommand:
      (*nexus)=XApplyLaunchCommand(context,(*nexus));
      break;
    case RegionOfInterestCommand:
      XApplyRegionOfInterestCommand(context);
      break;
    default:
    {
      XApplyMiscellanyCommands(context,command,&(*nexus));
      break;
    }
  }
}

static void XApplyPaintCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case OilPaintCommand:
      XApplyOilPaintCommand(context,&(*flags));
      break;
    case CharcoalDrawCommand:
      XApplyCharcoalDrawCommand(context);
      break;
    case AnnotateCommand:
      XApplyAnnotateCommand(context);
      break;
    case DrawCommand:
      XApplyDrawCommand(context);
      break;
    case ColorCommand:
      XApplyColorCommand(context);
      break;
    case MatteCommand:
      XApplyMatteCommand(context);
      break;
    default:
    {
      XApplyImageEditCommands(context,command,&(*nexus));
      break;
    }
  }
}

static void XApplyFXCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case SepiaToneCommand:
      XApplySepiaToneCommand(context);
      break;
    case SolarizeCommand:
      XApplySolarizeCommand(context);
      break;
    case SwirlCommand:
      XApplySwirlCommand(context,&(*flags));
      break;
    case ImplodeCommand:
      XApplyImplodeCommand(context,&(*flags));
      break;
    case VignetteCommand:
      XApplyVignetteCommand(context);
      break;
    case WaveCommand:
      XApplyWaveCommand(context);
      break;
    default:
    {
      XApplyPaintCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyEffectsCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case ThresholdCommand:
      XApplyThresholdCommand(context);
      break;
    case EdgeDetectCommand:
      XApplyEdgeDetectCommand(context,&(*flags));
      break;
    case SpreadCommand:
      XApplySpreadCommand(context,&(*flags));
      break;
    case ShadeCommand:
      XApplyShadeCommand(context);
      break;
    case RaiseCommand:
      XApplyRaiseCommand(context);
      break;
    case SegmentCommand:
      XApplySegmentCommand(context);
      break;
    default:
    {
      XApplyFXCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyFilterCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case DespeckleCommand:
      XApplyDespeckleCommand(context);
      break;
    case EmbossCommand:
      XApplyEmbossCommand(context);
      break;
    case ReduceNoiseCommand:
      XApplyReduceNoiseCommand(context,&(*flags));
      break;
    case AddNoiseCommand:
      XApplyAddNoiseCommand(context);
      break;
    case SharpenCommand:
      XApplySharpenCommand(context,&(*flags));
      break;
    case BlurCommand:
      XApplyBlurCommand(context,&(*flags));
      break;
    default:
    {
      XApplyEffectsCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyColorMapCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  QuantizeInfo
    *const quantize_info = context->quantize_info;

  switch (command)
  {
    case NegateCommand:
      XApplyNegateCommand(context);
      break;
    case GrayscaleCommand:
      XApplyGrayscaleCommand(context);
      break;
    case MapCommand:
      XApplyMapCommand(context,(*quantize_info));
      break;
    case QuantizeCommand:
      XApplyQuantizeCommand(context,(*quantize_info));
      break;
    default:
    {
      XApplyFilterCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyContrastCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case SpiffCommand:
      XApplySpiffCommand(context);
      break;
    case DullCommand:
      XApplyDullCommand(context);
      break;
    case ContrastStretchCommand:
      XApplyContrastStretchCommand(context);
      break;
    case SigmoidalContrastCommand:
      XApplySigmoidalContrastCommand(context);
      break;
    case NormalizeCommand:
      XApplyNormalizeCommand(context);
      break;
    case EqualizeCommand:
      XApplyEqualizeCommand(context);
      break;
    default:
    {
      XApplyColorMapCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyToneCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case HueCommand:
      XApplyHueCommand(context);
      break;
    case SaturationCommand:
      XApplySaturationCommand(context);
      break;
    case BrightnessCommand:
      XApplyBrightnessCommand(context);
      break;
    case GammaCommand:
      XApplyGammaCommand(context);
      break;
    default:
    {
      XApplyContrastCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyTransformCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  char
    *const color = context->color;

  switch (command)
  {
    case RotateRightCommand:
      XApplyRotateRightCommand(context);
      break;
    case RotateLeftCommand:
      XApplyRotateLeftCommand(context);
      break;
    case RotateCommand:
      XApplyRotateCommand(context);
      break;
    case ShearCommand:
      XApplyShearCommand(context,color);
      break;
    case RollCommand:
      XApplyRollCommand(context);
      break;
    case TrimCommand:
      XApplyTrimCommand(context);
      break;
    default:
    {
      XApplyToneCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyCropFlipCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case CropCommand:
      XApplyCropCommand(context);
      break;
    case ChopCommand:
      XApplyChopCommand(context);
      break;
    case FlopCommand:
      XApplyFlopCommand(context);
      break;
    case FlipCommand:
      XApplyFlipCommand(context);
      break;
    default:
    {
      XApplyTransformCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyViewCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case ApplyCommand:
      XApplyApplyCommand(context);
      break;
    case RefreshCommand:
      XApplyRefreshCommand(context);
      break;
    case RestoreCommand:
      XApplyRestoreCommand(context);
      break;
    default:
    {
      XApplyCropFlipCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyResizeCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case HalfSizeCommand:
      XApplyHalfSizeCommand(context);
      break;
    case OriginalSizeCommand:
      XApplyOriginalSizeCommand(context);
      break;
    case DoubleSizeCommand:
      XApplyDoubleSizeCommand(context);
      break;
    case ResizeCommand:
      XApplyResizeCommand(context);
      break;
    default:
    {
      XApplyViewCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyEditCommands(const XMagickCommandContext *context,
  const DisplayCommand command,MagickStatusType *flags,Image **nexus)
{
  switch (command)
  {
    case CutCommand:
      XApplyCutCommand(context);
      break;
    case CopyCommand:
      XApplyCopyCommand(context);
      break;
    case PasteCommand:
      XApplyPasteCommand(context);
      break;
    default:
    {
      XApplyResizeCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static void XApplyFileCommands(const XMagickCommandContext *context,
  const DisplayCommand command,Image **nexus,MagickStatusType *flags)
{
  switch (command)
  {
    case SaveCommand:
      XApplySaveCommand(context);
      break;
    case PrintCommand:
      XApplyPrintCommand(context);
      break;
    case DeleteCommand:
      XApplyDeleteCommand(context);
      break;
    case NewCommand:
      (*nexus)=XApplyNewCommand(context,(*nexus));
      break;
    case VisualDirectoryCommand:
      (*nexus)=XApplyVisualDirectoryCommand(context);
      break;
    case QuitCommand:
      XApplyQuitCommand(context);
      break;
    default:
    {
      XApplyEditCommands(context,command,&(*flags),&(*nexus));
      break;
    }
  }
}

static Image *XMagickCommand(Display *display,XResourceInfo *resource_info,
  XWindows *windows,const DisplayCommand command,Image **image,
  ExceptionInfo *exception)
{
  GeometryInfo
    geometry_info;

  Image
    *nexus;

  ImageInfo
    *image_info;

  MagickStatusType
    flags;

  QuantizeInfo
    quantize_info;

  static char
    color[MagickPathExtent] = "gray";

  XMagickCommandContext
    context;

  const XImageCacheContext
    cache_context = { display, resource_info, windows, image, exception };

  /*
    Process user command.
  */
  XCheckRefreshWindows(display,windows);
  XImageCache(&cache_context,command);
  nexus=NewImageList();
  windows->image.window_changes.width=windows->image.ximage->width;
  windows->image.window_changes.height=windows->image.ximage->height;
  image_info=CloneImageInfo(resource_info->image_info);
  SetGeometryInfo(&geometry_info);
  GetQuantizeInfo(&quantize_info);
  context.display=display;
  context.resource_info=resource_info;
  context.windows=windows;
  context.image=image;
  context.image_info=image_info;
  context.exception=exception;
  context.color=color;
  context.quantize_info=&quantize_info;
  switch (command)
  {
    case OpenCommand:
      nexus=XApplyOpenCommand(&context);
      break;
    case NextCommand:
      XApplyNextCommand(&context);
      break;
    case FormerCommand:
      XApplyFormerCommand(&context);
      break;
    case SelectCommand:
      nexus=XApplySelectCommand(&context);
      break;
    default:
    {
      XApplyFileCommands(&context,command,&nexus,&flags);
      break;
    }
  }
  image_info=DestroyImageInfo(image_info);
  return(nexus);
}

static void XUpdateMagnifyInfo(Display *display,XWindows *windows,const int x,
  const int y)
{
  char
    text[MagickPathExtent];

  if (windows->info.mapped != MagickFalse)
    {
      if ((x < (windows->info.x+(int) windows->info.width)) &&
          (y < (windows->info.y+(int) windows->info.height)))
        (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
    }
  else
    if ((x > (windows->info.x+(int) windows->info.width)) ||
        (y > (windows->info.y+(int) windows->info.height)))
      (void) XMapWindow(display,windows->info.id);
  if (windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(text,MagickPathExtent," %+d%+d ",
        windows->magnify.x,windows->magnify.y);
      XInfoWidget(display,windows,text);
    }
}

static void XClampMagnifyPosition(XWindows *windows,int *x,int *y)
{
  if ((*x) < 0)
    (*x)=0;
  else
    if ((*x) >= (int) windows->image.width)
      (*x)=(int) windows->image.width-1;
  if ((*y) < 0)
    (*y)=0;
  else
   if ((*y) >= (int) windows->image.height)
     (*y)=(int) windows->image.height-1;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X M a g n i f y I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMagnifyImage() magnifies portions of the image as indicated by the pointer.
%  The magnified portion is displayed in a separate window.
%
%  The format of the XMagnifyImage method is:
%
%      void XMagnifyImage(Display *display,XWindows *windows,XEvent *event,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o event: Specifies a pointer to a XEvent structure.  If it is NULL,
%      the entire image is refreshed.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XTrackMagnifyButton(Display *display,XWindows *windows,
  XEvent *event,ExceptionInfo *exception)
{
  int
    x,
    y;

  size_t
    state;

  (void) XCheckDefineCursor(display,windows->image.id,windows->magnify.cursor);
  state=DefaultState;
  x=event->xbutton.x;
  y=event->xbutton.y;
  windows->magnify.x=(int) windows->image.x+x;
  windows->magnify.y=(int) windows->image.y+y;
  do
  {
    /*
      Map and unmap Info widget as text cursor crosses its boundaries.
    */
    XUpdateMagnifyInfo(display,windows,x,y);
    /*
      Wait for next event.
    */
    XScreenEvent(display,windows,event,exception);
    switch (event->type)
    {
      case ButtonPress:
        break;
      case ButtonRelease:
      {
        /*
          User has finished magnifying image.
        */
        x=event->xbutton.x;
        y=event->xbutton.y;
        state|=ExitState;
        break;
      }
      case Expose:
        break;
      case MotionNotify:
      {
        x=event->xmotion.x;
        y=event->xmotion.y;
        break;
      }
      default:
        break;
    }
    /*
      Check boundary conditions.
    */
    XClampMagnifyPosition(windows,&x,&y);
  } while ((state & ExitState) == 0);
}

static void XMagnifyImage(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  /*
    Update magnified image until the mouse button is released.
  */
  XTrackMagnifyButton(display,windows,event,exception);
  /*
    Display magnified image.
  */
  XSetCursorState(display,windows,MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X M a g n i f y W i n d o w C o m m a n d                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMagnifyWindowCommand() moves the image within an Magnify window by one
%  pixel as specified by the key symbol.
%
%  The format of the XMagnifyWindowCommand method is:
%
%      void XMagnifyWindowCommand(Display *display,XWindows *windows,
%        const MagickStatusType state,const KeySym key_symbol,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o state: key mask.
%
%    o key_symbol: Specifies a KeySym which indicates which side of the image
%      to trim.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _MagnifyDigitKey
{
  int
    key,
    base;
} MagnifyDigitKey;

static const MagnifyDigitKey
  MagnifyDigitKeys[] =
  {
    { XK_0, XK_0 },
    { XK_1, XK_0 },
    { XK_2, XK_0 },
    { XK_3, XK_0 },
    { XK_4, XK_0 },
    { XK_5, XK_0 },
    { XK_6, XK_0 },
    { XK_7, XK_0 },
    { XK_8, XK_0 },
    { XK_9, XK_0 },
    { XK_KP_0, XK_KP_0 },
    { XK_KP_1, XK_KP_0 },
    { XK_KP_2, XK_KP_0 },
    { XK_KP_3, XK_KP_0 },
    { XK_KP_4, XK_KP_0 },
    { XK_KP_5, XK_KP_0 },
    { XK_KP_6, XK_KP_0 },
    { XK_KP_7, XK_KP_0 },
    { XK_KP_8, XK_KP_0 },
    { XK_KP_9, XK_KP_0 }
  };

static void XSelectMagnifyDigit(XWindows *windows,const KeySym key_symbol)
{
  const int
    key = (int) key_symbol;

  size_t
    i;

  for (i=0; i < (sizeof(MagnifyDigitKeys)/sizeof(*MagnifyDigitKeys)); i++)
    if (MagnifyDigitKeys[i].key == key)
      {
        windows->magnify.data=(key_symbol-MagnifyDigitKeys[i].base);
        break;
      }
}

static void XMoveMagnifyRightDown(XWindows *windows,const unsigned int quantum,
  const KeySym key_symbol)
{
  switch ((int) key_symbol)
  {
    case XK_Right:
    case XK_KP_Right:
    {
      if (windows->magnify.x < (int) (windows->image.ximage->width-1))
        windows->magnify.x+=(int) quantum;
      break;
    }
    case XK_Down:
    case XK_KP_Down:
    {
      if (windows->magnify.y < (int) (windows->image.ximage->height-1))
        windows->magnify.y+=(int) quantum;
      break;
    }
    default:
    {
      XSelectMagnifyDigit(windows,key_symbol);
      break;
    }
  }
}

#endif
