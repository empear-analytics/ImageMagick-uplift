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

  MagickCore private methods of annotate.c: the Postscript and X11 renderers,
  from RenderPostscript's helpers to RenderX11; included by that file only.
*/
#ifndef MAGICKCORE_ANNOTATE_RENDER_PRIVATE_H
#define MAGICKCORE_ANNOTATE_RENDER_PRIVATE_H

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   R e n d e r P o s t s c r i p t                                           %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RenderPostscript() renders text on the image with a Postscript font.  It
%  also returns the bounding box of the text relative to the image.
%
%  The format of the RenderPostscript method is:
%
%      MagickBooleanType RenderPostscript(Image *image,DrawInfo *draw_info,
%        const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType IsPostscriptNameChar(const unsigned char *p)
{
  return(((isalnum(*p) != 0) || (*p == '-') || (*p == '_') ||
    (*p == '.')) ? MagickTrue : MagickFalse);
}

static MagickBooleanType IsValidPostscriptFontname(const char *font)
{
  const unsigned char
    *p;

  if ((font == (const char *) NULL) || (*font == '\0'))
    return(MagickFalse);
  for (p=(const unsigned char *) font; *p != '\0'; p++)
  {
    if (IsPostscriptNameChar(p) != MagickFalse)
      continue;
    return(MagickFalse);
  }
  return(MagickTrue);
}

static PointInfo GetPostscriptExtent(const DrawInfo *draw_info)
{
  PointInfo
    extent,
    point;

  ssize_t
    i;

  size_t
    length;

  extent.x=0.0;
  extent.y=0.0;
  length=strlen(draw_info->text);
  for (i=0; i <= (ssize_t) (length+2); i++)
  {
    point.x=fabs(draw_info->affine.sx*i*draw_info->pointsize+
      draw_info->affine.ry*2.0*draw_info->pointsize);
    point.y=fabs(draw_info->affine.rx*i*draw_info->pointsize+
      draw_info->affine.sy*2.0*draw_info->pointsize);
    if (point.x > extent.x)
      extent.x=point.x;
    if (point.y > extent.y)
      extent.y=point.y;
  }
  return(extent);
}

static void SetPostscriptMetrics(const DrawInfo *draw_info,
  const Image *annotate_image,const PointInfo resolution,TypeMetric *metrics)
{
  metrics->pixels_per_em.x=(resolution.y/DefaultResolution)*
    ExpandAffine(&draw_info->affine)*draw_info->pointsize;
  metrics->pixels_per_em.y=metrics->pixels_per_em.x;
  metrics->ascent=metrics->pixels_per_em.y;
  metrics->descent=metrics->pixels_per_em.y/-5.0;
  metrics->width=(double) annotate_image->columns/
    ExpandAffine(&draw_info->affine);
  metrics->height=floor(metrics->ascent-metrics->descent+0.5);
  metrics->max_advance=metrics->pixels_per_em.x;
  metrics->bounds.x1=0.0;
  metrics->bounds.y1=metrics->descent;
  metrics->bounds.x2=metrics->ascent+metrics->descent;
  metrics->bounds.y2=metrics->ascent+metrics->descent;
  metrics->underline_position=(-2.0);
  metrics->underline_thickness=1.0;
}

static void WritePostscriptPrologue(FILE *file)
{
  (void) FormatLocaleFile(file,"%%!PS-Adobe-3.0\n");
  (void) FormatLocaleFile(file,"/ReencodeType\n");
  (void) FormatLocaleFile(file,"{\n");
  (void) FormatLocaleFile(file,"  findfont dup length\n");
  (void) FormatLocaleFile(file,
    "  dict begin { 1 index /FID ne {def} {pop pop} ifelse } forall\n");
  (void) FormatLocaleFile(file,
    "  /Encoding ISOLatin1Encoding def currentdict end definefont pop\n");
  (void) FormatLocaleFile(file,"} bind def\n");
}

static void WritePostscriptFont(FILE *file,const DrawInfo *draw_info)
{
  if (IsValidPostscriptFontname(draw_info->font) == MagickFalse)
    (void) FormatLocaleFile(file,
      "/Times-Roman-ISO dup /Times-Roman ReencodeType findfont setfont\n");
  else
    (void) FormatLocaleFile(file,
      "/%s-ISO dup /%s ReencodeType findfont setfont\n",draw_info->font,
      draw_info->font);
}

static void WritePostscriptText(FILE *file,const DrawInfo *draw_info,
  const MagickBooleanType identity,ExceptionInfo *exception)
{
  char
    *text;

  text=EscapeParenthesis(draw_info->text,MAGICK_SIZE_MAX,exception);
  if (identity == MagickFalse)
    (void) FormatLocaleFile(file,"(%s) stringwidth pop -0.5 mul -0.5 rmoveto\n",
      text);
  (void) FormatLocaleFile(file,"(%s) show\n",text);
  text=DestroyString(text);
}

static Image *ReadPostscriptImage(const DrawInfo *draw_info,
  const PointInfo extent,const char *filename,ExceptionInfo *exception)
{
  char
    geometry[MagickPathExtent];

  Image
    *annotate_image;

  ImageInfo
    *annotate_info;

  (void) FormatLocaleString(geometry,MagickPathExtent,"%.17gx%.17g+0+0!",
    floor(extent.x+0.5),floor(extent.y+0.5));
  annotate_info=AcquireImageInfo();
  (void) FormatLocaleString(annotate_info->filename,MagickPathExtent,"ps:%s",
    filename);
  (void) CloneString(&annotate_info->page,geometry);
  if (draw_info->density != (char *) NULL)
    (void) CloneString(&annotate_info->density,draw_info->density);
  annotate_info->antialias=draw_info->text_antialias;
  annotate_image=ReadImage(annotate_info,exception);
  CatchException(exception);
  annotate_info=DestroyImageInfo(annotate_info);
  (void) RelinquishUniqueFileResource(filename);
  return(annotate_image);
}

static MagickBooleanType IsIdentityAffine(const DrawInfo *draw_info)
{
  return((fabs(draw_info->affine.sx-draw_info->affine.sy) < MagickEpsilon) &&
    (fabs(draw_info->affine.rx) < MagickEpsilon) &&
    (fabs(draw_info->affine.ry) < MagickEpsilon) ? MagickTrue : MagickFalse);
}

static void SetPostscriptAlphaChannels(Image *image,Image *annotate_image,
  ExceptionInfo *exception)
{
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
  if (annotate_image->alpha_trait == UndefinedPixelTrait)
    (void) SetImageAlphaChannel(annotate_image,OpaqueAlphaChannel,
      exception);
}

static MagickBooleanType RenderPostscript(Image *image,
  const DrawInfo *draw_info,const PointInfo *offset,TypeMetric *metrics,
  ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent],
    geometry[MagickPathExtent];

  FILE
    *file;

  Image
    *annotate_image;

  int
    unique_file;

  MagickBooleanType
    identity,
    status;

  PointInfo
    extent,
    resolution;

  ssize_t
    y;

  /*
    Render label with a Postscript font.
  */
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(AnnotateEvent,GetMagickModule(),
      "Font %s; pointsize %g",draw_info->font != (char *) NULL ?
      draw_info->font : "none",draw_info->pointsize);
  file=(FILE *) NULL;
  unique_file=AcquireUniqueFileResource(filename);
  if (unique_file != -1)
    file=fdopen(unique_file,"wb");
  if ((unique_file == -1) || (file == (FILE *) NULL))
    {
      ThrowFileException(exception,FileOpenError,"UnableToOpenFile",filename);
      return(MagickFalse);
    }
  WritePostscriptPrologue(file);
  /*
    Sample to compute bounding box.
  */
  identity=IsIdentityAffine(draw_info);
  extent=GetPostscriptExtent(draw_info);
  (void) FormatLocaleFile(file,"%g %g moveto\n",identity  != MagickFalse ? 0.0 :
    extent.x/2.0,extent.y/2.0);
  (void) FormatLocaleFile(file,"%g %g scale\n",draw_info->pointsize,
    draw_info->pointsize);
  WritePostscriptFont(file,draw_info);
  (void) FormatLocaleFile(file,"[%g %g %g %g 0 0] concat\n",
    draw_info->affine.sx,-draw_info->affine.rx,-draw_info->affine.ry,
    draw_info->affine.sy);
  WritePostscriptText(file,draw_info,identity,exception);
  (void) FormatLocaleFile(file,"showpage\n");
  (void) fclose(file);
  annotate_image=ReadPostscriptImage(draw_info,extent,filename,exception);
  if (annotate_image == (Image *) NULL)
    return(MagickFalse);
  (void) NegateImage(annotate_image,MagickFalse,exception);
  resolution=GetDensityResolution(draw_info);
  if (identity == MagickFalse)
    (void) TransformImage(&annotate_image,"0x0",(char *) NULL,exception);
  else
    {
      RectangleInfo
        crop_info;

      crop_info=GetImageBoundingBox(annotate_image,exception);
      crop_info.height=(size_t) ((resolution.y/DefaultResolution)*
        ExpandAffine(&draw_info->affine)*draw_info->pointsize+0.5);
      crop_info.y=CastDoubleToSsizeT(ceil((resolution.y/DefaultResolution)*
        extent.y/8.0-0.5));
      (void) FormatLocaleString(geometry,MagickPathExtent,
        "%.17gx%.17g%+.20g%+.20g",(double) crop_info.width,(double)
        crop_info.height,(double) crop_info.x,(double) crop_info.y);
      (void) TransformImage(&annotate_image,geometry,(char *) NULL,exception);
    }
  SetPostscriptMetrics(draw_info,annotate_image,resolution,metrics);
  if (draw_info->render == MagickFalse)
    {
      annotate_image=DestroyImage(annotate_image);
      return(MagickTrue);
    }
  if (draw_info->fill.alpha != (MagickRealType) TransparentAlpha)
    {
      CacheView
        *annotate_view;

      MagickBooleanType
        sync;

      PixelInfo
        fill_color;

      /*
        Render fill color.
      */
      SetPostscriptAlphaChannels(image,annotate_image,exception);
      fill_color=draw_info->fill;
      status=MagickTrue;
      annotate_view=AcquireAuthenticCacheView(annotate_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(annotate_image,annotate_image,annotate_image->rows,4)
#endif
      for (y=0; y < (ssize_t) annotate_image->rows; y++)
      {
        ssize_t
          x;

        Quantum
          *magick_restrict q;

        if (status == MagickFalse)
          continue;
        q=GetCacheViewAuthenticPixels(annotate_view,0,y,annotate_image->columns,
          1,exception);
        if (q == (Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        for (x=0; x < (ssize_t) annotate_image->columns; x++)
        {
          GetFillColor(draw_info,x,y,&fill_color,exception);
          SetPixelAlpha(annotate_image,ClampToQuantum((((double) QuantumScale*
            GetPixelIntensity(annotate_image,q)*fill_color.alpha))),q);
          SetPixelRed(annotate_image,(const Quantum) fill_color.red,q);
          SetPixelGreen(annotate_image,(const Quantum) fill_color.green,q);
          SetPixelBlue(annotate_image,(const Quantum) fill_color.blue,q);
          q+=(ptrdiff_t) GetPixelChannels(annotate_image);
        }
        sync=SyncCacheViewAuthenticPixels(annotate_view,exception);
        if (sync == MagickFalse)
          status=MagickFalse;
      }
      annotate_view=DestroyCacheView(annotate_view);
      (void) CompositeImage(image,annotate_image,OverCompositeOp,MagickTrue,
        (ssize_t) ceil(offset->x-0.5),(ssize_t) ceil(offset->y-(metrics->ascent+
        metrics->descent)-0.5),exception);
    }
  annotate_image=DestroyImage(annotate_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   R e n d e r X 1 1                                                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RenderX11() renders text on the image with an X11 font.  It also returns the
%  bounding box of the text relative to the image.
%
%  The format of the RenderX11 method is:
%
%      MagickBooleanType RenderX11(Image *image,DrawInfo *draw_info,
%        const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType RenderX11(Image *image,const X11TextTarget *x11_text,
  ExceptionInfo *exception)
{
  const DrawInfo
    *draw_info = x11_text->draw_info;

  const PointInfo
    *offset = x11_text->offset;

  TypeMetric
    *metrics = x11_text->metrics;

  const XRenderText
    text = { draw_info, offset, metrics };

  MagickBooleanType
    status;

  if (annotate_semaphore == (SemaphoreInfo *) NULL)
    ActivateSemaphoreInfo(&annotate_semaphore);
  LockSemaphoreInfo(annotate_semaphore);
  status=XRenderImage(image,&text,exception);
  UnlockSemaphoreInfo(annotate_semaphore);
  return(status);
}

#endif
