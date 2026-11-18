/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%         CCCC   OOO   M   M  PPPP    OOO   SSSSS  IIIII  TTTTT  EEEEE        %
%        C      O   O  MM MM  P   P  O   O  SS       I      T    E            %
%        C      O   O  M M M  PPPP   O   O   SSS     I      T    EEE          %
%        C      O   O  M   M  P      O   O     SS    I      T    E            %
%         CCCC   OOO   M   M  P       OOO   SSSSS  IIIII    T    EEEEE        %
%                                                                             %
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%                                                                             %
%                     MagickCore Image Composite Methods                      %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
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
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/morphology.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/resample.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/composite-row-private.h"
#include "MagickCore/composite-channel-private.h"
#include "MagickCore/composite-image-blend-private.h"
#include "MagickCore/composite-image-over-private.h"
#include "MagickCore/composite-image-virtual-private.h"
#include "MagickCore/composite-image-coverage-private.h"

static SegmentInfo RotateBlurAxes(const GeometryInfo geometry_info,
  const double width,const double height)
{
  MagickRealType
    angle;

  SegmentInfo
    blur;

  /*
    Rotate vectors if a rotation angle is given.
  */
  angle=DegreesToRadians(geometry_info.xi);
  blur.x1=width*cos(angle);
  blur.x2=width*sin(angle);
  blur.y1=(-height*sin(angle));
  blur.y2=height*cos(angle);
  return(blur);
}

static SegmentInfo GetBlurAxes(const GeometryInfo geometry_info,
  const MagickStatusType flags,const double width,const double height)
{
  SegmentInfo
    blur;

  blur.x1=width;
  blur.x2=0.0;
  blur.y1=0.0;
  blur.y2=height;
  if ((flags & XValue) != 0 )
    blur=RotateBlurAxes(geometry_info,width,height);
  return(blur);
}

static void GetBlurAngleRange(const GeometryInfo geometry_info,
  const MagickStatusType flags,double *angle_start,double *angle_range)
{
  *angle_start=0.0;
  *angle_range=0.0;
  if ((flags & YValue) != 0 )
    {
      /*
        Lets set a angle range and calculate in the loop.
      */
      *angle_start=DegreesToRadians(geometry_info.xi);
      *angle_range=DegreesToRadians(geometry_info.psi)-*angle_start;
    }
}

typedef struct _BlurCanvasInputs
{
  double
    angle_range,
    angle_start,
    height,
    width;

  Image
    *canvas_image,
    *image,
    *source_image;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} BlurCanvasInputs;

static void BlurCompositeCanvas(const BlurCanvasInputs *inputs,
  SegmentInfo blur)
{
  const double
    angle_range = inputs->angle_range,
    angle_start = inputs->angle_start,
    height = inputs->height,
    width = inputs->width;

  Image
    *const canvas_image = inputs->canvas_image,
    *const image = inputs->image,
    *const source_image = inputs->source_image;

  const ssize_t
    x_offset = inputs->x_offset,
    y_offset = inputs->y_offset;

  ExceptionInfo
    *const exception = inputs->exception;

  CacheView
    *canvas_view,
    *source_view;

  PixelInfo
    pixel;

  ResampleFilter
    *resample_filter;

  /*
    Set up a gaussian cylindrical filter for EWA Blurring.

    As the minimum ellipse radius of support*1.0 the EWA algorithm
    can only produce a minimum blur of 0.5 for Gaussian (support=2.0)
    This means that even 'No Blur' will be still a little blurry! The
    solution (as well as the problem of preventing any user expert filter
    settings, is to set our own user settings, restore them afterwards.
  */
  resample_filter=AcquireResampleFilter(image,exception);
  SetResampleFilter(resample_filter,GaussianFilter);
  /*
    Perform the variable blurring of each pixel in image.
  */
  GetPixelInfo(image,&pixel);
  source_view=AcquireVirtualCacheView(source_image,exception);
  canvas_view=AcquireAuthenticCacheView(canvas_image,exception);
  {
    const BlurComposite
      blur_composite = { angle_range, angle_start, height, width,
        canvas_image, image, source_image, canvas_view, source_view,
        resample_filter, x_offset, y_offset, exception };

    BlurCompositeRows(&blur_composite,&blur,&pixel);
  }
  resample_filter=DestroyResampleFilter(resample_filter);
  source_view=DestroyCacheView(source_view);
  canvas_view=DestroyCacheView(canvas_view);
}

static inline MagickBooleanType IsRowOutsideImage(const Image *image,
  const ssize_t y,const ssize_t y_offset)
{
  return((((y+y_offset) < 0) || ((y+y_offset) >= (ssize_t) image->rows)) ?
    MagickTrue : MagickFalse);
}

typedef struct _DisplaceRows
{
  Image
    *canvas_image,
    *image,
    *source_image;

  PointInfo
    center;

  CompositeOperator
    compose;

  MagickRealType
    horizontal_scale,
    vertical_scale;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} DisplaceRows;

static void DisplaceCompositeRows(const DisplaceRows *rows)
{
  Image
    *const canvas_image = rows->canvas_image,
    *const image = rows->image,
    *const source_image = rows->source_image;

  const PointInfo
    center = rows->center;

  const CompositeOperator
    compose = rows->compose;

  const MagickRealType
    horizontal_scale = rows->horizontal_scale,
    vertical_scale = rows->vertical_scale;

  const ssize_t
    x_offset = rows->x_offset,
    y_offset = rows->y_offset;

  ExceptionInfo
    *const exception = rows->exception;

  CacheView
    *canvas_view,
    *image_view,
    *source_view;

  PixelInfo
    pixel;

  ssize_t
    y;

  GetPixelInfo(image,&pixel);
  image_view=AcquireVirtualCacheView(image,exception);
  source_view=AcquireVirtualCacheView(source_image,exception);
  canvas_view=AcquireAuthenticCacheView(canvas_image,exception);
  for (y=0; y < (ssize_t) source_image->rows; y++)
  {
    MagickBooleanType
      sync;

    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    if (IsRowOutsideImage(image,y,y_offset) != MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(source_view,0,y,source_image->columns,1,
      exception);
    q=QueueCacheViewAuthenticPixels(canvas_view,0,y,canvas_image->columns,1,
      exception);
    if (IsBlendRowPairMissing(p,q) != MagickFalse)
      break;
    {
      const DisplaceColumns
        columns = { canvas_image, image, source_image, image_view, center,
          compose, horizontal_scale, vertical_scale, x_offset, y,
          exception };

      if (DisplaceCompositeColumns(&columns,p,q,&pixel) == MagickFalse)
        break;
    }
    sync=SyncCacheViewAuthenticPixels(canvas_view,exception);
    if (sync == MagickFalse)
      break;
  }
  canvas_view=DestroyCacheView(canvas_view);
  source_view=DestroyCacheView(source_view);
  image_view=DestroyCacheView(image_view);
}

static BlendRequest GetBlendRequest(const Image *image,const ssize_t x_offset,
  const ssize_t y_offset)
{
  double
    residual_threshold = 0.0002,
    iterations = 400.0;

  BlendRequest
    request;

  size_t
    tick = 100;

  GetBlendIterations(image,&iterations,&residual_threshold,&tick);
  request.x_offset=x_offset;
  request.y_offset=y_offset;
  request.iterations=iterations;
  request.residual_threshold=residual_threshold;
  request.tick=tick;
  return(request);
}

static void GetDissolveArguments(const Image *image,GeometryInfo *geometry_info,
  MagickRealType *source_dissolve,MagickRealType *canvas_dissolve)
{
  const char
    *artifact;

  MagickStatusType
    flags;

  artifact=GetImageArtifact(image,"compose:args");
  if (artifact != (char *) NULL)
    {
      flags=ParseGeometry(artifact,geometry_info);
      SetDissolveFactors((*geometry_info),flags,source_dissolve,
        canvas_dissolve);
    }
}

static void GetDisplaceArguments(const Image *image,
  GeometryInfo *geometry_info,MagickStatusType *flags)
{
  const char
    *artifact;

  SetGeometryInfo(geometry_info);
  *flags=NoValue;
  artifact=GetImageArtifact(image,"compose:args");
  if (artifact != (char *) NULL)
    *flags=ParseGeometry(artifact,geometry_info);
}

static inline MagickBooleanType IsSourceAlphaNeeded(const Image *image,
  const Image *source_image)
{
  return(((source_image->alpha_trait == UndefinedPixelTrait) &&
    (image->alpha_trait != UndefinedPixelTrait)) ? MagickTrue : MagickFalse);
}

static void SetCanvasAlphaChannel(Image *image,ExceptionInfo *exception)
{
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
}

static void GetBlurEllipseSize(const GeometryInfo geometry_info,
  const MagickStatusType flags,double *width,double *height)
{
  *width=2.0*geometry_info.rho;
  *height=*width;
  if ((flags & HeightValue) != 0)
    *height=2.0*geometry_info.sigma;
}

typedef struct _BlurGeometryInputs
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  Image
    *canvas_image,
    *image,
    *source_image;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} BlurGeometryInputs;

static void BlurCompositeGeometry(const BlurGeometryInputs *geometry)
{
  const GeometryInfo
    geometry_info = geometry->geometry_info;

  const MagickStatusType
    flags = geometry->flags;

  Image
    *const canvas_image = geometry->canvas_image,
    *const image = geometry->image,
    *const source_image = geometry->source_image;

  const ssize_t
    x_offset = geometry->x_offset,
    y_offset = geometry->y_offset;

  ExceptionInfo
    *const exception = geometry->exception;

  double
    angle_range,
    angle_start,
    height,
    width;

  SegmentInfo
    blur;

  /*
    Users input sigma now needs to be converted to the EWA ellipse size.
    The filter defaults to a sigma of 0.5 so to make this match the users
    input the ellipse size needs to be doubled.
  */
  GetBlurEllipseSize(geometry_info,flags,&width,&height);
  /*
    Default the unrotated ellipse width and height axis vectors.
  */
  blur=GetBlurAxes(geometry_info,flags,width,height);
  GetBlurAngleRange(geometry_info,flags,&angle_start,&angle_range);
  {
    const BlurCanvasInputs
      inputs = { angle_range, angle_start, height, width, canvas_image,
        image, source_image, x_offset, y_offset, exception };

    BlurCompositeCanvas(&inputs,blur);
  }
}

typedef struct _DisplaceCanvasInputs
{
  Image
    *canvas_image,
    *image,
    *source_image;

  CompositeOperator
    compose;

  MagickStatusType
    flags;

  GeometryInfo
    geometry_info;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} DisplaceCanvasInputs;

static void DisplaceCompositeCanvas(const DisplaceCanvasInputs *canvas)
{
  Image
    *const canvas_image = canvas->canvas_image,
    *const image = canvas->image,
    *const source_image = canvas->source_image;

  const CompositeOperator
    compose = canvas->compose;

  const MagickStatusType
    flags = canvas->flags;

  const GeometryInfo
    geometry_info = canvas->geometry_info;

  const ssize_t
    x_offset = canvas->x_offset,
    y_offset = canvas->y_offset;

  ExceptionInfo
    *const exception = canvas->exception;

  DisplaceRows
    rows;

  MagickRealType
    horizontal_scale,
    vertical_scale;

  PointInfo
    center;

  {
    const DisplaceSetup
      displace = { compose, flags, geometry_info, image, source_image,
        x_offset, y_offset };

    if ((flags & (WidthValue | HeightValue)) == 0 )
      SetDisplaceScaleDefaults(&displace,&horizontal_scale,
        &vertical_scale);
    else
      SetDisplaceScaleArguments(&displace,&horizontal_scale,
        &vertical_scale);
    /*
      Determine fixed center point for absolute distortion map
       Absolute distort ==
         Displace offset relative to a fixed absolute point
         Select that point according to +X+Y user inputs.
         default = center of overlay image
         arg flag '!' = locations/percentage relative to background image
    */
    center.x=(MagickRealType) x_offset;
    center.y=(MagickRealType) y_offset;
    SetDistortCenter(&displace,&center);
  }
  /*
    Shift the pixel offset point as defined by the provided,
    displacement/distortion map.  -- Like a lens...
  */
  rows.canvas_image=canvas_image;
  rows.image=image;
  rows.source_image=source_image;
  rows.center=center;
  rows.compose=compose;
  rows.horizontal_scale=horizontal_scale;
  rows.vertical_scale=vertical_scale;
  rows.x_offset=x_offset;
  rows.y_offset=y_offset;
  rows.exception=exception;
  DisplaceCompositeRows(&rows);
}

typedef struct _BlurCanvasTargets
{
  Image
    **canvas_image;

  MagickStatusType
    *flags;

  GeometryInfo
    *geometry_info;
} BlurCanvasTargets;

static MagickBooleanType AcquireBlurCanvas(Image *image,Image *source_image,
  ExceptionInfo *exception,const BlurCanvasTargets *targets)
{
  Image
    **const canvas_image = targets->canvas_image;

  MagickStatusType
    *const flags = targets->flags;

  GeometryInfo
    *const geometry_info = targets->geometry_info;

  const char
    *artifact;

  (*canvas_image)=CloneImage(image,0,0,MagickTrue,exception);
  if ((*canvas_image) == (Image *) NULL)
    {
      source_image=DestroyImage(source_image);
      return(MagickFalse);
    }
  /*
    Gather the maximum blur sigma values from user.
  */
  (*flags)=NoValue;
  artifact=GetImageArtifact(image,"compose:args");
  if (artifact != (const char *) NULL)
    (*flags)=ParseGeometry(artifact,geometry_info);
  if (((*flags) & WidthValue) == 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "InvalidSetting","'%s' '%s'","compose:args",artifact);
      source_image=DestroyImage(source_image);
      (*canvas_image)=DestroyImage(*canvas_image);
      return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _ComposeArtifactTargets
{
  double
    *white_luminance;

  IlluminantType
    *illuminant;

  ColorspaceType
    *colorspace;

  MagickBooleanType
    *clamp,
    *compose_sync;
} ComposeArtifactTargets;

static void GetComposeArtifacts(const Image *image,
  const ComposeArtifactTargets *targets)
{
  double
    *const white_luminance = targets->white_luminance;

  IlluminantType
    *const illuminant = targets->illuminant;

  ColorspaceType
    *const colorspace = targets->colorspace;

  MagickBooleanType
    *const clamp = targets->clamp,
    *const compose_sync = targets->compose_sync;

  const char
    *artifact;

  (*white_luminance)=10000.0;
  artifact=GetImageArtifact(image,"compose:white-luminance");
  if (artifact != (const char *) NULL)
    (*white_luminance)=StringToDouble(artifact,(char **) NULL);
  artifact=GetImageArtifact(image,"compose:illuminant");
  if (artifact != (const char *) NULL)
    (*illuminant)=ParseComposeIlluminant(artifact);
  artifact=GetImageArtifact(image,"compose:colorspace");
  if (artifact != (const char *) NULL)
    (*colorspace)=ParseComposeColorspace(artifact);
  (*clamp)=MagickTrue;
  artifact=GetImageArtifact(image,"compose:clamp");
  if (artifact != (const char *) NULL)
    (*clamp)=IsStringTrue(artifact);
  (*compose_sync)=MagickTrue;
  artifact=GetImageArtifact(image,"compose:sync");
  if (artifact != (const char *) NULL)
    (*compose_sync)=IsStringTrue(artifact);
}

static MagickBooleanType IsSourceInsideCanvas(const Image *image,
  const Image *source_image,const ssize_t x_offset,const ssize_t y_offset)
{
  if ((x_offset < 0) || (y_offset < 0))
    return(MagickFalse);
  if ((x_offset+(ssize_t) source_image->columns) > (ssize_t) image->columns)
    return(MagickFalse);
  if ((y_offset+(ssize_t) source_image->rows) > (ssize_t) image->rows)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _CompositeRowHead
{
  const CompositeRowInputs
    *rows;

  CacheView
    *source_view,
    *image_view;

  ssize_t
    y;
} CompositeRowHead;

static MagickBooleanType CompositeImageRowHead(const CompositeRowHead *head,
  MagickBooleanType *status)
{
  const CompositeRowInputs
    *const rows = head->rows;

  CacheView
    *const image_view = head->image_view;

  const ssize_t
    y = head->y;

  Image
    *const image = rows->image;

  ExceptionInfo
    *const exception = rows->exception;

  const Quantum
    *magick_restrict p,
    *pixels;

  Quantum
    *magick_restrict q;

  CompositeRowInputs
    row;

  const OverSourceRow
    source = { head->source_view, rows->source_image, rows->clip_to_self,
      rows->x_offset, rows->y_offset, y, exception };

  if (SkipOverRow(&source,status) == MagickFalse)
    return(MagickFalse);
  if (FetchOverSourceRow(&source,&pixels,&p,status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  row=(*rows);
  row.y=y;
  row.pixels=pixels;
  CompositeImageRow(&row,p,q);
  SyncCompositeRow(image_view,exception,status);
  return(MagickTrue);
}

static MagickBooleanType CompositeImageRows(const CompositeRowInputs *rows)
{
  Image
    *const image = rows->image,
    *const source_image = rows->source_image;

  ExceptionInfo
    *const exception = rows->exception;

  CacheView
    *source_view,
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  status=MagickTrue;
  progress=0;
  source_view=AcquireVirtualCacheView(source_image,exception);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(source_image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const CompositeRowHead
      head = { rows, source_view, image_view, y };

    if (CompositeImageRowHead(&head,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetCompositeProgress(image,CompositeImageTag,progress,&status);
      }
  }
  source_view=DestroyCacheView(source_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

typedef struct _ComposeArgumentTargets
{
  GeometryInfo
    *geometry_info;

  MagickRealType
    *percent_luma,
    *percent_chroma,
    *amount,
    *threshold,
    *source_dissolve,
    *canvas_dissolve;
} ComposeArgumentTargets;

static void GetComposeArguments(Image *image,const CompositeOperator compose,
  ExceptionInfo *exception,const ComposeArgumentTargets *targets)
{
  GeometryInfo
    *const geometry_info = targets->geometry_info;

  MagickRealType
    *const percent_luma = targets->percent_luma,
    *const percent_chroma = targets->percent_chroma,
    *const amount = targets->amount,
    *const threshold = targets->threshold;

  switch (compose)
  {
    case CopyAlphaCompositeOp:
    case ChangeMaskCompositeOp:
    {
      /*
        Modify canvas outside the overlaid region and require an alpha
        channel to exist, to add transparency.
      */
      SetCanvasAlphaChannel(image,exception);
      break;
    }
    case MathematicsCompositeOp:
    {
      /*
        Just collect the values from "compose:args", setting.
        Unused values are set to zero automagically.

        Arguments are normally a comma separated list, so this probably should
        be changed to some 'general comma list' parser, (with a minimum
        number of values)
      */
      GetMathematicsArguments(image,geometry_info,exception);
      break;
    }
    case ModulateCompositeOp:
    {
      /*
        Determine the luma and chroma scale.
      */
      GetModulateArguments(image,geometry_info,percent_luma,percent_chroma);
      break;
    }
    case ThresholdCompositeOp:
    {
      /*
        Determine the amount and threshold.
      */
      GetThresholdArguments(image,geometry_info,amount,threshold);
      break;
    }
    default:
      break;
  }
}

static void GetBlendArguments(Image *image,const CompositeOperator compose,
  ExceptionInfo *exception,const ComposeArgumentTargets *targets)
{
  const char
    *artifact;

  GeometryInfo
    *const geometry_info = targets->geometry_info;

  MagickRealType
    *const source_dissolve = targets->source_dissolve,
    *const canvas_dissolve = targets->canvas_dissolve;

  MagickStatusType
    flags;

  switch (compose)
  {
    case DissolveCompositeOp:
    {
      /*
        Geometry arguments to dissolve factors.
      */
      GetDissolveArguments(image,geometry_info,source_dissolve,
        canvas_dissolve);
      break;
    }
    case BlendCompositeOp:
    {
      artifact=GetImageArtifact(image,"compose:args");
      if (artifact != (char *) NULL)
        {
          flags=ParseGeometry(artifact,geometry_info);
          SetBlendFactors((*geometry_info),flags,source_dissolve,
            canvas_dissolve);
        }
      break;
    }
    default:
    {
      GetComposeArguments(image,compose,exception,targets);
      break;
    }
  }
}

typedef struct _CopyCompositeInputs
{
  Image
    *image,
    *source_image;

  ssize_t
    x_offset,
    y_offset;

  CompositeSetup
    setup;

  ExceptionInfo
    *exception;
} CopyCompositeInputs;

static MagickBooleanType CompositeCopyGroups(const CopyCompositeInputs *copy,
  const CompositeOperator compose,const ComposeArgumentTargets *targets,
  MagickBooleanType *result)
{
  Image
    *const image = copy->image,
    *const source_image = copy->source_image;

  const ssize_t
    x_offset = copy->x_offset,
    y_offset = copy->y_offset;

  CompositeSetup
    setup = copy->setup;

  ExceptionInfo
    *const exception = copy->exception;

  switch (compose)
  {
    case CopyCompositeOp:
    {
      if (IsSourceInsideCanvas(image,source_image,x_offset,y_offset) ==
          MagickFalse)
        break;
      if (IsSourceAlphaNeeded(image,source_image) != MagickFalse)
        (void) SetImageAlphaChannel(source_image,OpaqueAlphaChannel,exception);
      (*result)=CompositeCopyImage(image,source_image,&setup,exception);
      return(MagickTrue);
    }
    case IntensityCompositeOp:
    {
      if (IsSourceInsideCanvas(image,source_image,x_offset,y_offset) ==
          MagickFalse)
        break;
      setup.intensity=MagickTrue;
      (*result)=CompositeCopyImage(image,source_image,&setup,exception);
      return(MagickTrue);
    }
    default:
    {
      GetBlendArguments(image,compose,exception,targets);
      break;
    }
  }
  return(MagickFalse);
}

typedef struct _BlendCompositeInputs
{
  Image
    *image;

  const Image
    *composite;

  Image
    *source_image;

  ssize_t
    x_offset,
    y_offset;

  CompositeSetup
    setup;

  ExceptionInfo
    *exception;
} BlendCompositeInputs;

static MagickBooleanType CompositeBlendGroups(const BlendCompositeInputs *blend,
  const CompositeOperator compose,const ComposeArgumentTargets *targets,
  MagickBooleanType *result)
{
  Image
    *const image = blend->image;

  const Image
    *const composite = blend->composite;

  Image
    *source_image = blend->source_image;

  const ssize_t
    x_offset = blend->x_offset,
    y_offset = blend->y_offset;

  const CompositeSetup
    setup = blend->setup;

  ExceptionInfo
    *const exception = blend->exception;

  MagickBooleanType
    returned,
    status;

  switch (compose)
  {
    case SaliencyBlendCompositeOp:
    {
      BlendRequest
        request;

      request=GetBlendRequest(image,x_offset,y_offset);
      status=SaliencyBlendImage(image,composite,&request,exception);
      source_image=DestroyImage(source_image);
      (*result)=status;
      return(MagickTrue);
    }
    case SeamlessBlendCompositeOp:
    {
      BlendRequest
        request;

      request=GetBlendRequest(image,x_offset,y_offset);
      status=SeamlessBlendImage(image,composite,&request,exception);
      source_image=DestroyImage(source_image);
      (*result)=status;
      return(MagickTrue);
    }
    default:
    {
      const CopyCompositeInputs
        copy = { image, source_image, x_offset, y_offset, setup, exception };

      if (CompositeCopyGroups(&copy,compose,targets,&returned) !=
          MagickFalse)
        {
          (*result)=returned;
          return(MagickTrue);
        }
      break;
    }
  }
  return(MagickFalse);
}

typedef struct _CanvasCompositeInputs
{
  Image
    *image;

  const Image
    *composite;

  ssize_t
    x_offset,
    y_offset;

  CompositeSetup
    setup;

  ExceptionInfo
    *exception;

  Image
    **canvas_image,
    **source_image;

  GeometryInfo
    *geometry_info;
} CanvasCompositeInputs;

static MagickBooleanType CompositeDisplaceGroups(
  const CanvasCompositeInputs *canvas_inputs,const CompositeOperator compose,
  const ComposeArgumentTargets *targets,MagickBooleanType *result)
{
  Image
    *const image = canvas_inputs->image;

  const Image
    *const composite = canvas_inputs->composite;

  const ssize_t
    x_offset = canvas_inputs->x_offset,
    y_offset = canvas_inputs->y_offset;

  const CompositeSetup
    setup = canvas_inputs->setup;

  ExceptionInfo
    *const exception = canvas_inputs->exception;

  Image
    **const canvas_image = canvas_inputs->canvas_image,
    **const source_image = canvas_inputs->source_image;

  GeometryInfo
    *const geometry_info = canvas_inputs->geometry_info;

  MagickBooleanType
    returned;

  MagickStatusType
    flags;

  switch (compose)
  {
    case DisplaceCompositeOp:
    case DistortCompositeOp:
    {
      /*
        Displace/Distort based on overlay gradient map:
          X = red_channel;  Y = green_channel;
          compose:args = x_scale[,y_scale[,center.x,center.y]]
      */
      (*canvas_image)=CloneImage(image,0,0,MagickTrue,exception);
      if ((*canvas_image) == (Image *) NULL)
        {
          (*source_image)=DestroyImage((*source_image));
          (*result)=MagickFalse;
          return(MagickTrue);
        }
      GetDisplaceArguments(image,geometry_info,&flags);
      {
        const DisplaceCanvasInputs
          canvas = { (*canvas_image), image, (*source_image), compose, flags,
            (*geometry_info), x_offset, y_offset, exception };

        DisplaceCompositeCanvas(&canvas);
      }
      (*source_image)=DestroyImage((*source_image));
      (*source_image)=(*canvas_image);
      break;
    }
    default:
    {
      const BlendCompositeInputs
        blend = { image, composite, (*source_image), x_offset, y_offset, setup,
          exception };

      if (CompositeBlendGroups(&blend,compose,targets,&returned) !=
          MagickFalse)
        {
          (*result)=returned;
          return(MagickTrue);
        }
      break;
    }
  }
  return(MagickFalse);
}

static MagickBooleanType CompositeBlurGroups(
  const CanvasCompositeInputs *canvas_inputs,const CompositeOperator compose,
  const ComposeArgumentTargets *argument_targets,MagickBooleanType *result)
{
  Image
    *const image = canvas_inputs->image;

  const ssize_t
    x_offset = canvas_inputs->x_offset,
    y_offset = canvas_inputs->y_offset;

  ExceptionInfo
    *const exception = canvas_inputs->exception;

  Image
    **const canvas_image = canvas_inputs->canvas_image,
    **const source_image = canvas_inputs->source_image;

  GeometryInfo
    *const geometry_info = canvas_inputs->geometry_info;

  MagickBooleanType
    returned;

  MagickStatusType
    flags;

  switch (compose)
  {
    case BlurCompositeOp:
    {
      BlurCanvasTargets
        blur_targets;

      /*
        Blur Image by resampling dictated by an overlay gradient map:
          X = red_channel; Y = green_channel; compose:args =
          x_scale[,y_scale[,angle]].
      */
      blur_targets.canvas_image=(canvas_image);
      blur_targets.flags=(&flags);
      blur_targets.geometry_info=(geometry_info);
      if (AcquireBlurCanvas(image,(*source_image),exception,&blur_targets) ==
          MagickFalse)
        {
          (*result)=MagickFalse;
          return(MagickTrue);
        }
      {
        const BlurGeometryInputs
          geometry = { (*geometry_info), flags, (*canvas_image), image,
            (*source_image), x_offset, y_offset, exception };

        BlurCompositeGeometry(&geometry);
      }
      (*source_image)=DestroyImage((*source_image));
      (*source_image)=(*canvas_image);
      break;
    }
    default:
    {
      if (CompositeDisplaceGroups(canvas_inputs,compose,argument_targets,
          &returned) != MagickFalse)
        {
          (*result)=returned;
          return(MagickTrue);
        }
      break;
    }
  }
  return(MagickFalse);
}

static void SetCompositeRowOperators(CompositeRowInputs *rows,
  const CompositeOperator compose)
{
  (*rows).union_coverage=IsListedOperator(UnionCoverageOperators,
    sizeof(UnionCoverageOperators)/sizeof(*UnionCoverageOperators),compose);
  (*rows).canvas_alpha=IsListedOperator(CanvasAlphaOperators,
    sizeof(CanvasAlphaOperators)/sizeof(*CanvasAlphaOperators),compose);
  (*rows).source_alpha=IsListedOperator(SourceAlphaOperators,
    sizeof(SourceAlphaOperators)/sizeof(*SourceAlphaOperators),compose);
  (*rows).pixel_info_inputs=IsListedOperator(PixelInfoInputOperators,
    sizeof(PixelInfoInputOperators)/sizeof(*PixelInfoInputOperators),compose);
  (*rows).intensity_inputs=IsListedOperator(IntensityInputOperators,
    sizeof(IntensityInputOperators)/sizeof(*IntensityInputOperators),compose);
}

static CompositeSetup GetCompositeSetup(const ssize_t x_offset,
  const ssize_t y_offset,const MagickBooleanType clamp)
{
  CompositeSetup
    setup;

  setup.x_offset=x_offset;
  setup.y_offset=y_offset;
  setup.clamp=clamp;
  setup.intensity=MagickFalse;
  return(setup);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p o s i t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompositeImage() returns the second image composited onto the first
%  at the specified offset, using the specified composite method.
%
%  The format of the CompositeImage method is:
%
%      MagickBooleanType CompositeImage(Image *image,
%        const Image *source_image,const CompositeOperator compose,
%        const MagickBooleanType clip_to_self,const ssize_t x_offset,
%        const ssize_t y_offset,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the canvas image, modified by he composition
%
%    o source_image: the source image.
%
%    o compose: This operator affects how the composite is applied to
%      the image.  The operators and how they are utilized are listed here
%      http://www.w3.org/TR/SVG12/#compositing.
%
%    o clip_to_self: set to MagickTrue to limit composition to area composed.
%
%    o x_offset: the column offset of the composited image.
%
%    o y_offset: the row offset of the composited image.
%
%  Extra Controls from Image meta-data in 'image' (artifacts)
%
%    o "compose:args"
%        A string containing extra numerical arguments for specific compose
%        methods, generally expressed as a 'geometry' or a comma separated list
%        of numbers.
%
%        Compose methods needing such arguments include "BlendCompositeOp" and
%        "DisplaceCompositeOp".
%
%    o exception: return any errors or warnings in this structure.
%
*/

/*
   Composition based on the SVG specification:

   A Composition is defined by...
      Color Function :  f(Sc,Dc)  where Sc and Dc are the normalized colors
      Blending areas :  X = 1     for area of overlap, ie: f(Sc,Dc)
                        Y = 1     for source preserved
                        Z = 1     for canvas preserved

   Conversion to transparency (then optimized)
      Dca' = f(Sc, Dc)*Sa*Da + Y*Sca*(1-Da) + Z*Dca*(1-Sa)
      Da'  = X*Sa*Da + Y*Sa*(1-Da) + Z*Da*(1-Sa)

   Where...
      Sca = Sc*Sa     normalized Source color divided by Source alpha
      Dca = Dc*Da     normalized Dest color divided by Dest alpha
      Dc' = Dca'/Da'  the desired color value for this channel.

   Da' in in the follow formula as 'gamma'  The resulting alpha value.

   Most functions use a blending mode of over (X=1,Y=1,Z=1) this results in
   the following optimizations...
      gamma = Sa+Da-Sa*Da;
      gamma = 1 - QuantumScale*alpha * QuantumScale*beta;
      opacity = QuantumScale*alpha*beta;  // over blend, optimized 1-Gamma

   The above SVG definitions also define that Mathematical Composition
   methods should use a 'Over' blending mode for Alpha Channel.
   It however was not applied for composition modes of 'Plus', 'Minus',
   the modulus versions of 'Add' and 'Subtract'.

   Mathematical operator changes to be applied from IM v6.7...

    1) Modulus modes 'Add' and 'Subtract' are obsoleted and renamed
       'ModulusAdd' and 'ModulusSubtract' for clarity.

    2) All mathematical compositions work as per the SVG specification
       with regard to blending.  This now includes 'ModulusAdd' and
       'ModulusSubtract'.

    3) When the special channel flag 'sync' (synchronize channel updates)
       is turned off (enabled by default) then mathematical compositions are
       only performed on the channels specified, and are applied
       independently of each other.  In other words the mathematics is
       performed as 'pure' mathematical operations, rather than as image
       operations.
*/

MagickExport MagickBooleanType CompositeImage(Image *image,
  const Image *composite,const CompositeOperator compose,
  const MagickBooleanType clip_to_self,const ssize_t x_offset,
  const ssize_t y_offset,ExceptionInfo *exception)
{
#define CompositeImageTag  "Composite/Image"

  CompositeRowInputs
    rows;

  CompositeSetup
    setup;

  Image
    *canvas_image,
    *source_image;

  MagickBooleanType
    returned,
    status;

  const ComposeArgumentTargets
    argument_targets = { &rows.geometry_info, &rows.percent_luma,
      &rows.percent_chroma, &rows.amount, &rows.threshold,
      &rows.source_dissolve, &rows.canvas_dissolve };

  const ComposeArtifactTargets
    targets = { &rows.white_luminance, &rows.illuminant, &rows.colorspace,
      &rows.clamp, &rows.compose_sync };

  rows.colorspace=HCLColorspace;
  rows.white_luminance=10000.0;
  rows.illuminant=D65Illuminant;
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(composite != (Image *) NULL);
  assert(composite->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  source_image=CloneImage(composite,0,0,MagickTrue,exception);
  if (source_image == (const Image *) NULL)
    return(MagickFalse);
  (void) SetImageColorspace(source_image,image->colorspace,exception);
  if ((compose == OverCompositeOp) || (compose == SrcOverCompositeOp))
    {
      const CompositePlacement
        placement = { clip_to_self, x_offset, y_offset };

      status=CompositeOverImage(image,source_image,&placement,exception);
      source_image=DestroyImage(source_image);
      return(status);
    }
  rows.amount=0.5;
  canvas_image=(Image *) NULL;
  rows.canvas_dissolve=1.0;
  GetComposeArtifacts(image,&targets);
  SetGeometryInfo(&rows.geometry_info);
  rows.percent_luma=100.0;
  rows.percent_chroma=100.0;
  rows.source_dissolve=1.0;
  rows.threshold=0.05f;
  setup=GetCompositeSetup(x_offset,y_offset,rows.clamp);
  {
    const CanvasCompositeInputs
      canvas_inputs = { image, composite, x_offset, y_offset, setup,
        exception, &canvas_image, &source_image, &rows.geometry_info };

    if (CompositeBlurGroups(&canvas_inputs,compose,&argument_targets,
        &returned) != MagickFalse)
      return(returned);
  }
  /*
    Composite image.
  */
  rows.midpoint=((MagickRealType) QuantumRange+1.0)/2;
  rows.image=image;
  rows.source_image=source_image;
  rows.compose=compose;
  SetCompositeRowOperators(&rows,compose);
  rows.clip_to_self=clip_to_self;
  rows.x_offset=x_offset;
  rows.y_offset=y_offset;
  rows.exception=exception;
  status=CompositeImageRows(&rows);
  if (canvas_image != (Image * ) NULL)
    canvas_image=DestroyImage(canvas_image);
  else
    source_image=DestroyImage(source_image);
  return(status);
}
