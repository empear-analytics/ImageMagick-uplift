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

  MagickCore private methods of annotate.c: the FreeType helpers, from
  IsKernedGrapheme to the GlyphInfo typedef, inside the
  MAGICKCORE_FREETYPE_DELEGATE branch; included by that file only.
*/
#ifndef MAGICKCORE_ANNOTATE_FREETYPE_PRIVATE_H
#define MAGICKCORE_ANNOTATE_FREETYPE_PRIVATE_H

static inline MagickBooleanType IsKernedGrapheme(const ssize_t index,
  const ssize_t last_glyph)
{
  return(((index != 0) && (last_glyph != 0)) ? MagickTrue : MagickFalse);
}

typedef struct _TextLayoutInfo
{
  const DrawInfo
    *draw_info;

  FT_Face
    face;

  FT_Int32
    flags;
} TextLayoutInfo;

static size_t ComplexTextLayout(const TextLayoutInfo *layout,const char *text,
  const size_t length,GraphemeInfo **grapheme)
{
  const DrawInfo
    *const draw_info = layout->draw_info;

  const FT_Face
    face = layout->face;

  const FT_Int32
    flags = layout->flags;

  const char
    *p;

  ssize_t
    i;

  ssize_t
    last_glyph;

  /*
    Simple layout for bi-directional text (right-to-left or left-to-right).
  */
  *grapheme=(GraphemeInfo *) AcquireQuantumMemory(length+1,sizeof(**grapheme));
  if (*grapheme == (GraphemeInfo *) NULL)
    return(0);
  last_glyph=0;
  p=text;
  for (i=0; GetUTFCode(p) != 0; p+=(ptrdiff_t) GetUTFOctets(p), i++)
  {
    (*grapheme)[i].index=(ssize_t) FT_Get_Char_Index(face,(FT_ULong)
      GetUTFCode(p));
    (*grapheme)[i].x_offset=0;
    (*grapheme)[i].y_offset=0;
    if ((IsKernedGrapheme((*grapheme)[i].index,last_glyph) != MagickFalse) &&
        (FT_HAS_KERNING(face)))
      {
        FT_Error
          ft_status;

        FT_Vector
          kerning;

        ft_status=FT_Get_Kerning(face,(FT_UInt) last_glyph,(FT_UInt)
          (*grapheme)[i].index,ft_kerning_default,&kerning);
        if (ft_status == 0)
          (*grapheme)[i-1].x_advance+=(FT_Pos) ((draw_info->direction ==
            RightToLeftDirection ? -1.0 : 1.0)*kerning.x);
      }
    (void) FT_Load_Glyph(face,(FT_UInt) (*grapheme)[i].index,flags);
    (*grapheme)[i].x_advance=face->glyph->advance.x;
    (*grapheme)[i].y_advance=face->glyph->advance.y;
    (*grapheme)[i].cluster=(size_t) (p-text);
    last_glyph=(*grapheme)[i].index;
  }
  return((size_t) i);
}

static void FreetypeCloseStream(FT_Stream stream)
{
  FILE
    *file;

  file=(FILE *) stream->descriptor.pointer;
  if (file != (FILE *) NULL)
    (void) fclose(file);
  stream->descriptor.pointer=NULL;
}

static unsigned long FreetypeReadStream(FT_Stream stream,unsigned long offset,
  unsigned char *buffer,unsigned long count)
{
  FILE
    *file;

  unsigned long
    result;

  file=(FILE *) stream->descriptor.pointer;
  if (file == (FILE *) NULL)
    return(0);
  if (offset > stream->size)
    result=1;
  else
    result=(unsigned long) fseek(file,(off_t) offset,SEEK_SET);
  if (count == 0) /* seek operation */
    return(result);
  if (result != 0)
    return(0);
  return((unsigned long) fread(buffer,1,count,file));
}

static inline MagickBooleanType IsEmptyOutline(FT_Outline outline)
{
  return((outline.n_points == 0) || (outline.n_contours <= 0) ? MagickTrue :
    MagickFalse);
}

static int TraceCubicBezier(FT_Vector *p,FT_Vector *q,FT_Vector *to,
  DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MagickPathExtent];

  affine=draw_info->affine;
  (void) FormatLocaleString(path,MagickPathExtent,"C%g,%g %g,%g %g,%g",
    affine.tx+p->x/64.0,affine.ty-p->y/64.0,affine.tx+q->x/64.0,affine.ty-
    q->y/64.0,affine.tx+to->x/64.0,affine.ty-to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

static int TraceLineTo(FT_Vector *to,DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MagickPathExtent];

  affine=draw_info->affine;
  (void) FormatLocaleString(path,MagickPathExtent,"L%g,%g",affine.tx+to->x/64.0,
    affine.ty-to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

static int TraceMoveTo(FT_Vector *to,DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MagickPathExtent];

  affine=draw_info->affine;
  (void) FormatLocaleString(path,MagickPathExtent,"M%g,%g",affine.tx+to->x/64.0,
    affine.ty-to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

static int TraceQuadraticBezier(FT_Vector *control,FT_Vector *to,
  DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MagickPathExtent];

  affine=draw_info->affine;
  (void) FormatLocaleString(path,MagickPathExtent,"Q%g,%g %g,%g",affine.tx+
    control->x/64.0,affine.ty-control->y/64.0,affine.tx+to->x/64.0,affine.ty-
    to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

#if FREETYPE_MAJOR == 2 && FREETYPE_MINOR >= 10
static inline const char *FreetypeErrorMessage(FT_Error ft_status)
{
  return(FT_Error_String(ft_status));
#else
static inline const char *FreetypeErrorMessage(
  FT_Error magick_unused(ft_status))
{
  magick_unreferenced(ft_status);
  return((const char *) NULL);
#endif
}

static void *FreetypeAlloc(FT_Memory magick_unused(memory),long extent)
{
  magick_unreferenced(memory);
  if (extent <= 0)
    return((void *) NULL);
  return(AcquireQuantumMemory((size_t) extent,1));
}

static void *FreetypeRealloc(FT_Memory magick_unused(memory),long length,
  long extent,void *block)
{
  void
    *cloned_block,
    *new_block;

  magick_unreferenced(memory);
  if (extent <= 0)
    return(RelinquishMagickMemory(block));
  if (block == (void *) NULL)
    return(AcquireQuantumMemory((size_t) extent,1));
  cloned_block=AcquireQuantumMemory((size_t) length,1);
  if (cloned_block == (void *) NULL)
    return((void *) NULL);
  (void) memcpy(cloned_block,block,(size_t) length);
  new_block=ResizeQuantumMemory(cloned_block,(size_t) extent,1);
  if (new_block == (void *) NULL)
    return((void *) NULL);
  block=RelinquishMagickMemory(block);
  return(new_block);
}

static void FreetypeFree(FT_Memory magick_unused(memory),void *block)
{
  magick_unreferenced(memory);
  (void) RelinquishMagickMemory(block);
}

static FT_Memory FreetypeAcquireMemoryManager(void)
{
  FT_Memory
    memory;

  memory=(FT_Memory) AcquireCriticalMemory(sizeof(*memory));
  memset(memory,0,sizeof(*memory));
  memory->alloc=(&FreetypeAlloc);
  memory->realloc=(&FreetypeRealloc);
  memory->free=(&FreetypeFree);
  return(memory);
}

static void FreetypeDone(FT_Memory memory,FT_Library library,
  FT_StreamRec *stream)
{
  (void) FT_Done_Library(library);
  stream=(FT_StreamRec *) RelinquishMagickMemory(stream);
  memory=(FT_Memory) RelinquishMagickMemory(memory);
}

static FT_Error FreetypeInit(FT_Memory memory,FT_Library *alibrary)
{
  FT_Error
    ft_status;

  ft_status=FT_New_Library(memory,alibrary);
  if (ft_status != 0)
    RelinquishMagickMemory(memory);
  else
    {
      FT_Add_Default_Modules(*alibrary);
      FT_Set_Default_Properties(*alibrary);
    }
  return(ft_status);
}

static inline MagickBooleanType IsUnauthorizedFontFile(
  const DrawInfo *draw_info)
{
  return(((draw_info->font != (char *) NULL) && (*draw_info->font == '@') &&
    (IsRightsAuthorized(PathPolicyDomain,ReadPolicyRights,draw_info->font) ==
    MagickFalse)) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsTextWithoutGlyphs(const DrawInfo *draw_info,
  const FT_UInt first_glyph_id)
{
  return(((draw_info->text == (char *) NULL) || (*draw_info->text == '\0') ||
    (first_glyph_id == 0)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsRenderedStroke(const DrawInfo *draw_info,
  const MagickBooleanType status)
{
  return((((draw_info->stroke.alpha != (MagickRealType) TransparentAlpha) ||
    (draw_info->stroke_pattern != (Image *) NULL)) &&
    ((status != MagickFalse) &&
    (draw_info->render != MagickFalse))) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsVisibleStroke(const DrawInfo *draw_info,
  const MagickBooleanType status)
{
  return((((draw_info->stroke.alpha != (MagickRealType) TransparentAlpha) ||
    (draw_info->stroke_pattern != (Image *) NULL)) &&
    (status != MagickFalse)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsInterwordSpace(const DrawInfo *draw_info,
  const char *text,const ssize_t code)
{
  return(((fabs(draw_info->interword_spacing) >= MagickEpsilon) &&
    (IsUTFSpace(GetUTFCode(text)) != MagickFalse) &&
    (IsUTFSpace((int) code) == MagickFalse)) ? MagickTrue : MagickFalse);
}

static void SetFreetypeMetrics(const FT_Face face,TypeMetric *metrics)
{
  metrics->pixels_per_em.x=face->size->metrics.x_ppem;
  metrics->pixels_per_em.y=face->size->metrics.y_ppem;
  metrics->ascent=(double) face->size->metrics.ascender/64.0;
  metrics->descent=(double) face->size->metrics.descender/64.0;
  if (face->size->metrics.ascender == 0)
    {
      /*
        Sanitize buggy ascender and descender values.
      */
      metrics->ascent=face->size->metrics.y_ppem;
      if (face->size->metrics.descender == 0)
        metrics->descent=face->size->metrics.y_ppem/-3.5;
    }
  metrics->width=0;
  metrics->origin.x=0;
  metrics->origin.y=0;
  metrics->height=(double) face->size->metrics.height/64.0;
  metrics->max_advance=0.0;
  if (face->size->metrics.max_advance > MagickEpsilon)
    metrics->max_advance=(double) face->size->metrics.max_advance/64.0;
  metrics->bounds.x1=0.0;
  metrics->bounds.y1=metrics->descent;
  metrics->bounds.x2=metrics->ascent+metrics->descent;
  metrics->bounds.y2=metrics->ascent+metrics->descent;
  metrics->underline_position=face->underline_position*
    (metrics->pixels_per_em.x*MagickSafeReciprocal(face->units_per_EM));
  metrics->underline_thickness=face->underline_thickness*
    (metrics->pixels_per_em.x*MagickSafeReciprocal(face->units_per_EM));
}

static void SetFreetypeCanvas(Image *image,ExceptionInfo *exception)
{
  if (image->storage_class != DirectClass)
    (void) SetImageStorageClass(image,DirectClass,exception);
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
}

static void DrawFreetypeStroke(Image *image,DrawInfo *annotate_info,
  const PointInfo *offset,ExceptionInfo *exception)
{
  /*
    Draw text stroke.
  */
  annotate_info->linejoin=RoundJoin;
  annotate_info->affine.tx=offset->x;
  annotate_info->affine.ty=offset->y;
  (void) ConcatenateString(&annotate_info->primitive,"'");
  if (strlen(annotate_info->primitive) > 7)
    (void) DrawImage(image,annotate_info,exception);
  (void) CloneString(&annotate_info->primitive,"path '");
}

typedef struct _GlyphRasterInfo
{
  FT_BitmapGlyph
    bitmap;

  unsigned char
    *r;
} GlyphRasterInfo;

static double GetGlyphFillOpacity(const GlyphRasterInfo *raster,
  const ssize_t n,const ssize_t x,const ssize_t y)
{
  const FT_BitmapGlyph
    bitmap = raster->bitmap;

  unsigned char
    *const r = raster->r;

  double
    fill_opacity;

  fill_opacity=1.0;
  if (bitmap->bitmap.buffer != (unsigned char *) NULL)
    {
      if (bitmap->bitmap.pixel_mode == ft_pixel_mode_grays)
        fill_opacity=(double) (r[n])/(bitmap->bitmap.num_grays-1);
      else
        if (bitmap->bitmap.pixel_mode == ft_pixel_mode_mono)
          fill_opacity=((r[(x >> 3)+y*bitmap->bitmap.pitch] &
            (1 << (~x & 0x07)))) == 0 ? 0.0 : 1.0;
    }
  return(fill_opacity);
}

static Quantum *SkipGlyphPixel(const Image *image,Quantum *q)
{
  if (q != (Quantum *) NULL)
    q+=(ptrdiff_t) GetPixelChannels(image);
  return(q);
}

static MagickBooleanType SyncGlyphPixel(const MagickBooleanType active,
  CacheView *image_view,ExceptionInfo *exception)
{
  MagickBooleanType
    status,
    sync;

  status=MagickTrue;
  if (active == MagickFalse)
    {
      sync=SyncCacheViewAuthenticPixels(image_view,exception);
      if (sync == MagickFalse)
        status=MagickFalse;
    }
  return(status);
}

static char *FindInvalidUTFCode(const DrawInfo *draw_info)
{
  char
    *p;

  for (p=draw_info->text; GetUTFCode(p) != 0; p+=(ptrdiff_t) GetUTFOctets(p))
    if (GetUTFCode(p) < 0)
      break;
  return(p);
}

static void SelectFreetypeText(const DrawInfo *draw_info,
  unsigned char **utf8,char **p)
{
  *utf8=(unsigned char *) NULL;
  if (GetUTFCode(*p) == 0)
    *p=draw_info->text;
  else
    {
      *utf8=ConvertLatin1ToUTF8((unsigned char *) draw_info->text);
      if (*utf8 != (unsigned char *) NULL)
        *p=(char *) *utf8;
    }
}

static FT_Matrix GetFreetypeAffine(const DrawInfo *draw_info)
{
  FT_Matrix
    affine;

  affine.xx=65536L;
  affine.yx=0L;
  affine.xy=0L;
  affine.yy=65536L;
  if (draw_info->render != MagickFalse)
    {
      affine.xx=(FT_Fixed) (65536L*draw_info->affine.sx+0.5);
      affine.yx=(FT_Fixed) (-65536L*draw_info->affine.rx+0.5);
      affine.xy=(FT_Fixed) (-65536L*draw_info->affine.ry+0.5);
      affine.yy=(FT_Fixed) (65536L*draw_info->affine.sy+0.5);
    }
  return(affine);
}

static void RelinquishFreetypeText(GraphemeInfo *grapheme,unsigned char *utf8)
{
  if (grapheme != (GraphemeInfo *) NULL)
    grapheme=(GraphemeInfo *) RelinquishMagickMemory(grapheme);
  if (utf8 != (unsigned char *) NULL)
    utf8=(unsigned char *) RelinquishMagickMemory(utf8);
}

static FT_Int32 GetFreetypeHintingFlags(Image *image,FT_Int32 flags,
  ExceptionInfo *exception)
{
  const char
    *value;

  value=GetImageProperty(image,"type:hinting",exception);
  if ((value != (const char *) NULL) && (LocaleCompare(value,"off") == 0))
    flags|=FT_LOAD_NO_HINTING;
  return(flags);
}

static inline MagickBooleanType IsTransparentFill(const DrawInfo *draw_info)
{
  return(((draw_info->fill.alpha == (MagickRealType) TransparentAlpha) &&
    (draw_info->fill_pattern == (Image *) NULL) &&
    (draw_info->stroke.alpha == (MagickRealType) TransparentAlpha) &&
    (draw_info->stroke_pattern == (Image *) NULL)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsOffsetOutside(const ssize_t offset,
  const size_t extent)
{
  return(((offset < 0) || (offset >= (ssize_t) extent)) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsNonOutlineGlyph(const FT_Glyph glyph_image,
  FT_Outline outline)
{
  return(((glyph_image->format != FT_GLYPH_FORMAT_OUTLINE) &&
    (IsEmptyOutline(outline) == MagickFalse)) ? MagickTrue : MagickFalse);
}

static void ExtendGlyphMinBounds(const FT_BBox bounds,TypeMetric *metrics)
{
  if ((bounds.xMin < metrics->bounds.x1) && (bounds.xMin != 0))
    metrics->bounds.x1=(double) bounds.xMin;
  if ((bounds.yMin < metrics->bounds.y1) && (bounds.yMin != 0))
    metrics->bounds.y1=(double) bounds.yMin;
}

static void ExtendGlyphMaxBounds(const FT_BBox bounds,TypeMetric *metrics)
{
  if ((bounds.xMax > metrics->bounds.x2) && (bounds.xMax != 0))
    metrics->bounds.x2=(double) bounds.xMax;
  if ((bounds.yMax > metrics->bounds.y2) && (bounds.yMax != 0))
    metrics->bounds.y2=(double) bounds.yMax;
}

static FT_Encoding SelectAdobeBig5Encoding(const char *encoding,
  FT_Encoding encoding_type)
{
  if (LocaleCompare(encoding,"AdobeCustom") == 0)
    encoding_type=FT_ENCODING_ADOBE_CUSTOM;
  if (LocaleCompare(encoding,"AdobeExpert") == 0)
    encoding_type=FT_ENCODING_ADOBE_EXPERT;
  if (LocaleCompare(encoding,"AdobeStandard") == 0)
    encoding_type=FT_ENCODING_ADOBE_STANDARD;
  if (LocaleCompare(encoding,"AppleRoman") == 0)
    encoding_type=FT_ENCODING_APPLE_ROMAN;
  if (LocaleCompare(encoding,"BIG5") == 0)
    encoding_type=FT_ENCODING_BIG5;
  return(encoding_type);
}

static FT_Encoding SelectCjkLatinEncoding(const char *encoding,
  FT_Encoding encoding_type)
{
  (void) encoding;
#if defined(FT_ENCODING_PRC)
  if (LocaleCompare(encoding,"GB2312") == 0)
    encoding_type=FT_ENCODING_PRC;
#endif
#if defined(FT_ENCODING_JOHAB)
  if (LocaleCompare(encoding,"Johab") == 0)
    encoding_type=FT_ENCODING_JOHAB;
#endif
#if defined(FT_ENCODING_ADOBE_LATIN_1)
  if (LocaleCompare(encoding,"Latin-1") == 0)
    encoding_type=FT_ENCODING_ADOBE_LATIN_1;
#endif
#if defined(FT_ENCODING_ADOBE_LATIN_2)
  if (LocaleCompare(encoding,"Latin-2") == 0)
    encoding_type=FT_ENCODING_OLD_LATIN_2;
#endif
  return(encoding_type);
}

static void BlendGlyphAlpha(const Image *image,double fill_opacity,
  Quantum *q)
{
  double
    Sa,
    Da;

  Da=1.0-(QuantumScale*(double) GetPixelAlpha(image,q));
  Sa=fill_opacity;
  fill_opacity=(1.0-RoundToUnity(Sa+Da-Sa*Da))*(double) QuantumRange;
  SetPixelAlpha(image,(const Quantum) fill_opacity,q);
}

static void ScaleFreetypeMetrics(TypeMetric *metrics)
{
  /*
    Determine font metrics.
  */
  metrics->bounds.x1/=64.0;
  metrics->bounds.y1/=64.0;
  metrics->bounds.x2/=64.0;
  metrics->bounds.y2/=64.0;
  metrics->origin.x/=64.0;
  metrics->origin.y/=64.0;
  metrics->width=ceil(metrics->width/64.0);
}

static void SetFreetypeMetricsOrigin(TypeMetric *metrics,
  const FT_Vector origin)
{
  metrics->origin.x=(double) origin.x;
  metrics->origin.y=(double) origin.y;
  if (metrics->origin.x > metrics->width)
    metrics->width=metrics->origin.x;
}

static FT_StreamRec *AcquireFreetypeStream(const char *pathname)
{
  FT_StreamRec
    *stream;

  struct stat
    attributes;

  stream=(FT_StreamRec *) AcquireCriticalMemory(sizeof(*stream));
  (void) memset(stream,0,sizeof(*stream));
  if (stat(pathname,&attributes) == 0)
    stream->size=attributes.st_size >= 0 ? (unsigned long)
      attributes.st_size : 0;
  stream->descriptor.pointer=fopen_utf8(pathname,"rb");
  stream->read=(&FreetypeReadStream);
  stream->close=(&FreetypeCloseStream);
  return(stream);
}

static PointInfo GetGlyphBitmapPoint(const PointInfo *offset,
  const FT_BitmapGlyph bitmap,const FT_Vector origin)
{
  PointInfo
    point;

  point.x=offset->x+bitmap->left;
  if (bitmap->bitmap.pixel_mode == ft_pixel_mode_mono)
    point.x+=(origin.x/64.0);
  point.y=offset->y-bitmap->top;
  return(point);
}

static FT_Glyph DestroyFreetypeGlyph(FT_Glyph glyph_image)
{
  if (glyph_image != (FT_Glyph) NULL)
    {
      FT_Done_Glyph(glyph_image);
      glyph_image=(FT_Glyph) NULL;
    }
  return(glyph_image);
}

static void TraceGlyphOutline(DrawInfo *annotate_info,const FT_Vector origin,
  FT_Outline outline,const FT_Outline_Funcs *outline_methods)
{
  /*
    Trace the glyph.
  */
  annotate_info->affine.tx=origin.x/64.0;
  annotate_info->affine.ty=(-origin.y/64.0);
  if (IsEmptyOutline(outline) == MagickFalse)
    (void) FT_Outline_Decompose(&outline,outline_methods,annotate_info);
}

typedef struct _GlyphRows
{
  Image
    *image;

  CacheView
    *image_view;

  FT_BitmapGlyph
    bitmap;

  PointInfo
    point;

  unsigned char
    *r;

  MagickBooleanType
    transparent_fill;

  const DrawInfo
    *draw_info;

  ExceptionInfo
    *exception;
} GlyphRows;

static Quantum *GetGlyphRowPixels(const GlyphRows *rows,
  const ssize_t x_offset,const ssize_t y_offset,MagickBooleanType *active)
{
  Image
    *const image = rows->image;

  CacheView
    *const image_view = rows->image_view;

  const FT_BitmapGlyph
    bitmap = rows->bitmap;

  ExceptionInfo
    *const exception = rows->exception;

  Quantum
    *magick_restrict q;

  q=(Quantum *) NULL;
  if (IsOffsetOutside(x_offset,image->columns) != MagickFalse)
    *active=MagickFalse;
  else
    {
      q=GetCacheViewAuthenticPixels(image_view,x_offset,y_offset,
        bitmap->bitmap.width,1,exception);
      *active=q != (Quantum *) NULL ? MagickTrue : MagickFalse;
    }
  return(q);
}

static double GetGlyphPixelOpacity(const GlyphRows *rows,const ssize_t n,
  const ssize_t x,const ssize_t y)
{
  const FT_BitmapGlyph
    bitmap = rows->bitmap;

  unsigned char
    *const r = rows->r;

  const DrawInfo
    *const draw_info = rows->draw_info;

  double
    fill_opacity;

  GlyphRasterInfo
    raster;

  raster.bitmap=bitmap;
  raster.r=r;
  fill_opacity=GetGlyphFillOpacity(&raster,n,x,y);
  if (draw_info->text_antialias == MagickFalse)
    fill_opacity=fill_opacity >= 0.5 ? 1.0 : 0.0;
  return(fill_opacity);
}

typedef struct _GlyphPixel
{
  ssize_t
    x_offset,
    y_offset;

  double
    fill_opacity;

  MagickBooleanType
    active;
} GlyphPixel;

static void CompositeGlyphPixel(const GlyphRows *rows,const GlyphPixel *pixel,
  Quantum *magick_restrict q,MagickBooleanType *status)
{
  Image
    *const image = rows->image;

  const DrawInfo
    *const draw_info = rows->draw_info;

  ExceptionInfo
    *const exception = rows->exception;

  const ssize_t
    x_offset = pixel->x_offset,
    y_offset = pixel->y_offset;

  double
    fill_opacity = pixel->fill_opacity;

  PixelInfo
    fill_color;

  if (rows->transparent_fill == MagickFalse)
    {
      GetPixelInfo(image,&fill_color);
      GetFillColor(draw_info,x_offset,y_offset,&fill_color,exception);
      fill_opacity=fill_opacity*fill_color.alpha;
      CompositePixelOver(image,&fill_color,fill_opacity,q,
        GetPixelAlpha(image,q),q);
    }
  else
    BlendGlyphAlpha(image,fill_opacity,q);
  if (SyncGlyphPixel(pixel->active,rows->image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

static Quantum *FetchGlyphPixel(const GlyphRows *rows,const GlyphPixel *pixel,
  Quantum *magick_restrict q)
{
  if (pixel->active == MagickFalse)
    q=GetCacheViewAuthenticPixels(rows->image_view,pixel->x_offset,
      pixel->y_offset,1,1,rows->exception);
  return(q);
}

static void SyncGlyphRow(CacheView *image_view,ExceptionInfo *exception,
  MagickBooleanType *status)
{
  MagickBooleanType
    sync;

  sync=SyncCacheViewAuthenticPixels(image_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
}

static void RasterizeGlyphRow(const GlyphRows *rows,const ssize_t y,
  MagickBooleanType *status)
{
  Image
    *const image = rows->image;

  CacheView
    *const image_view = rows->image_view;

  const FT_BitmapGlyph
    bitmap = rows->bitmap;

  const PointInfo
    point = rows->point;

  ExceptionInfo
    *const exception = rows->exception;

  double
    fill_opacity;

  MagickBooleanType
    active;

  Quantum
    *magick_restrict q;

  ssize_t
    x,
    n,
    x_offset,
    y_offset;

  if ((*status) == MagickFalse)
    return;
  x_offset=CastDoubleToSsizeT(ceil(point.x-0.5));
  y_offset=CastDoubleToSsizeT(ceil(point.y+y-0.5));
  if (IsOffsetOutside(y_offset,image->rows) != MagickFalse)
    return;
  q=GetGlyphRowPixels(rows,x_offset,y_offset,&active);
  n=y*bitmap->bitmap.pitch;
  for (x=0; x < (ssize_t) bitmap->bitmap.width; x++, n++)
  {
    x_offset=CastDoubleToSsizeT(ceil(point.x+x-0.5));
    if (IsOffsetOutside(x_offset,image->columns) != MagickFalse)
      {
        q=SkipGlyphPixel(image,q);
        continue;
      }
    fill_opacity=GetGlyphPixelOpacity(rows,n,x,y);
    {
      const GlyphPixel
        pixel = { x_offset, y_offset, fill_opacity, active };

      q=FetchGlyphPixel(rows,&pixel,q);
      if (q == (Quantum *) NULL)
        continue;
      CompositeGlyphPixel(rows,&pixel,q,status);
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  SyncGlyphRow(image_view,exception,status);
}

static void SelectFreetypeCharmap(FT_Face face,FT_Encoding encoding_type)
{
  FT_Error
    ft_status;

  ft_status=FT_Select_Charmap(face,encoding_type);
  if ((ft_status != 0) && (face->num_charmaps != 0))
    (void) FT_Set_Charmap(face,face->charmaps[0]);
}

typedef struct _GlyphBitmapTarget
{
  Image
    *image;

  const DrawInfo
    *draw_info;

  FT_BitmapGlyph
    bitmap;

  PointInfo
    point;

  DrawInfo
    *annotate_info;

  const PointInfo
    *offset;

  ExceptionInfo
    *exception;
} GlyphBitmapTarget;

static MagickBooleanType RasterizeGlyphBitmap(const GlyphBitmapTarget *target,
  MagickBooleanType status)
{
  Image
    *const image = target->image;

  const DrawInfo
    *const draw_info = target->draw_info;

  const FT_BitmapGlyph
    bitmap = target->bitmap;

  const PointInfo
    point = target->point;

  DrawInfo
    *const annotate_info = target->annotate_info;

  const PointInfo
    *const offset = target->offset;

  ExceptionInfo
    *const exception = target->exception;

  ssize_t
    y;

  CacheView
    *image_view;

  MagickBooleanType
    transparent_fill;

  unsigned char
    *r;

  /*
    Rasterize the glyph.
  */
  transparent_fill=IsTransparentFill(draw_info);
  r=bitmap->bitmap.buffer;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,bitmap->bitmap.rows,4)
#endif
  for (y=0; y < (ssize_t) bitmap->bitmap.rows; y++)
  {
    const GlyphRows rows = { image, image_view, bitmap, point, r,
      transparent_fill, draw_info, exception };

    RasterizeGlyphRow(&rows,y,&status);
  }
  image_view=DestroyCacheView(image_view);
  if (IsVisibleStroke(draw_info,status) != MagickFalse)
    DrawFreetypeStroke(image,annotate_info,offset,exception);
  return(status);
}

static FT_Int32 GetFreetypeLoadFlags(const DrawInfo *draw_info)
{
  FT_Int32
    flags;

  flags=FT_LOAD_DEFAULT;
  if (draw_info->render == MagickFalse)
    flags=FT_LOAD_NO_BITMAP;
  if (draw_info->text_antialias == MagickFalse)
    flags|=FT_LOAD_TARGET_MONO;
  else
    {
#if defined(FT_LOAD_TARGET_LIGHT)
      flags|=FT_LOAD_TARGET_LIGHT;
#elif defined(FT_LOAD_TARGET_LCD)
      flags|=FT_LOAD_TARGET_LCD;
#endif
    }
  return(flags);
}

static DrawInfo *CloneFreetypeAnnotateInfo(const DrawInfo *draw_info)
{
  DrawInfo
    *annotate_info;

  annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  if (annotate_info->dash_pattern != (double *) NULL)
    annotate_info->dash_pattern[0]=0.0;
  (void) CloneString(&annotate_info->primitive,"path '");
  return(annotate_info);
}

static FT_Encoding SelectFreetypeEncoding(const char *encoding,
  FT_Encoding encoding_type)
{
  encoding_type=SelectAdobeBig5Encoding(encoding,encoding_type);
  encoding_type=SelectCjkLatinEncoding(encoding,encoding_type);
  if (LocaleCompare(encoding,"None") == 0)
    encoding_type=FT_ENCODING_NONE;
  if (LocaleCompare(encoding,"SJIScode") == 0)
    encoding_type=FT_ENCODING_SJIS;
  if (LocaleCompare(encoding,"Symbol") == 0)
    encoding_type=FT_ENCODING_MS_SYMBOL;
  if (LocaleCompare(encoding,"Unicode") == 0)
    encoding_type=FT_ENCODING_UNICODE;
  if (LocaleCompare(encoding,"Wansung") == 0)
    encoding_type=FT_ENCODING_WANSUNG;
  return(encoding_type);
}

static void SelectFreetypeFontPath(const DrawInfo *draw_info,
  FT_Open_Args *args,FT_Long *face_index,ExceptionInfo *exception)
{
  if (draw_info->font == (char *) NULL)
    {
      const TypeInfo *type_info = GetTypeInfo("*",exception);
      if (type_info != (const TypeInfo *) NULL)
        args->pathname=ConstantString(type_info->glyphs);
    }
  else
    if (*draw_info->font != '@')
      args->pathname=ConstantString(draw_info->font);
    else
      {
        /*
          Extract face index, e.g. @msgothic[1].
        */
        ImageInfo *image_info = AcquireImageInfo();
        (void) CopyMagickString(image_info->filename,draw_info->font+1,
          MagickPathExtent);
        (void) SetImageInfo(image_info,0,exception);
        *face_index=(FT_Long) image_info->scene;
        args->pathname=ConstantString(image_info->filename);
        image_info=DestroyImageInfo(image_info);
     }
}

static FT_UInt GetFreetypeGlyphId(const GraphemeInfo *grapheme,
  const FT_UInt missing_glyph_id)
{
  FT_UInt
    id;

  id=(FT_UInt) grapheme->index;
  if (id == 0)
    id=missing_glyph_id;
  return(id);
}

#define ThrowFreetypeErrorException(tag,ft_status,value) \
{ \
  const char *error_string = FreetypeErrorMessage(ft_status); \
  if (error_string != (const char *) NULL) \
    (void) ThrowMagickException(exception,GetMagickModule(),TypeError, \
      tag,"`%s (%s)'",value, error_string); \
  else \
    (void) ThrowMagickException(exception,GetMagickModule(),TypeError, \
      tag,"`%s'",value); \
}

typedef struct _FreetypeFaceTargets
{
  FT_Memory
    *memory;

  FT_Library
    *library;

  FT_StreamRec
    **stream;

  FT_Face
    *face;
} FreetypeFaceTargets;

static MagickBooleanType ConfigureFreetypeFace(const FreetypeTarget *target,
  const FreetypeFaceTargets *targets,ExceptionInfo *exception)
{
  FT_Memory
    *const memory = targets->memory;

  FT_Library
    *const library = targets->library;

  FT_StreamRec
    **const stream = targets->stream;

  FT_Face
    *const face = targets->face;

  const DrawInfo
    *draw_info = target->draw_info;

  const char
    *encoding = target->encoding;

  FT_Encoding
    encoding_type;

  FT_Error
    ft_status;

  PointInfo
    resolution;

  if ((draw_info->metrics != (char *) NULL) &&
      (IsPathAccessible(draw_info->metrics) != MagickFalse))
    (void) FT_Attach_File(*face,draw_info->metrics);
  encoding_type=FT_ENCODING_UNICODE;
  SelectFreetypeCharmap(*face,encoding_type);
  if (encoding != (const char *) NULL)
    {
      encoding_type=SelectFreetypeEncoding(encoding,encoding_type);
      ft_status=FT_Select_Charmap(*face,encoding_type);
      if (ft_status != 0)
        {
          (void) FT_Done_Face(*face);
          FreetypeDone(*memory,*library,*stream);
          ThrowFreetypeErrorException("UnrecognizedFontEncoding",ft_status,
            encoding);
          return(MagickFalse);
        }
    }
  /*
    Set text size.
  */
  resolution=GetDensityResolution(draw_info);
  ft_status=FT_Set_Char_Size(*face,(FT_F26Dot6) (64.0*draw_info->pointsize),
    (FT_F26Dot6) (64.0*draw_info->pointsize),(FT_UInt) resolution.x,
    (FT_UInt) resolution.y);
  if (ft_status != 0)
    {
      (void) FT_Done_Face(*face);
      FreetypeDone(*memory,*library,*stream);
      ThrowFreetypeErrorException("UnableToReadFont",ft_status,
        draw_info->font);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType OpenFreetypeFace(const FreetypeTarget *target,
  const FreetypeFaceTargets *targets,ExceptionInfo *exception)
{
  FT_Memory
    *const memory = targets->memory;

  FT_Library
    *const library = targets->library;

  FT_StreamRec
    **const stream = targets->stream;

  FT_Face
    *const face = targets->face;

  Image
    *image = target->image;

  const DrawInfo
    *draw_info = target->draw_info;

  FT_Error
    ft_status;

  FT_Long
    face_index;

  FT_Open_Args
    args;

  /*
    Initialize Truetype library.
  */
  if (IsUnauthorizedFontFile(draw_info) != MagickFalse)
    ThrowPolicyException(draw_info->font,MagickFalse);
  *memory=FreetypeAcquireMemoryManager();
  if (*memory == (FT_Memory) NULL)
    ThrowBinaryException(ResourceLimitError,"UnableToInitializeFreetypeLibrary",
      image->filename);
  ft_status=FreetypeInit(*memory,library);
  if (ft_status != 0)
    {
      ThrowFreetypeErrorException("UnableToInitializeFreetypeLibrary",ft_status,
        image->filename);
      return(MagickFalse);
    }
  /*
    Open font face.
  */
  face_index=(FT_Long) draw_info->face;
  (void) memset(&args,0,sizeof(args));
  SelectFreetypeFontPath(draw_info,&args,&face_index,exception);
  /*
    Configure streaming interface.
  */
  *stream=AcquireFreetypeStream(args.pathname);
  args.flags=FT_OPEN_STREAM;
  args.stream=*stream;
  *face=(FT_Face) NULL;
  ft_status=FT_Open_Face(*library,&args,face_index,face);
  if (ft_status != 0)
    {
      FreetypeDone(*memory,*library,*stream);
      ThrowFreetypeErrorException("UnableToReadFont",ft_status,args.pathname);
      args.pathname=DestroyString(args.pathname);
      return(MagickFalse);
    }
  args.pathname=DestroyString(args.pathname);
  return(ConfigureFreetypeFace(target,targets,exception));
}

typedef struct _GlyphInfo
{
  FT_UInt
    id;

  FT_Vector
    origin;

  FT_Glyph
    image;
} GlyphInfo;

#endif
