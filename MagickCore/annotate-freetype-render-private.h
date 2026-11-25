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

  MagickCore private methods of annotate.c: the FreeType glyph loader and
  RenderFreetype, after OutlineMethods, inside the MAGICKCORE_FREETYPE_DELEGATE
  branch; included by that file only.
*/
#ifndef MAGICKCORE_ANNOTATE_FREETYPE_RENDER_PRIVATE_H
#define MAGICKCORE_ANNOTATE_FREETYPE_RENDER_PRIVATE_H

typedef struct _GlyphLoadInput
{
  FT_Face
    face;

  FT_Int32
    flags;

  const DrawInfo
    *draw_info;

  MagickBooleanType
    status;

  DrawInfo
    *annotate_info;

  FT_Matrix
    affine;

  TypeMetric
    *metrics;
} GlyphLoadInput;

static MagickBooleanType LoadFreetypeGlyph(const GlyphLoadInput *glyph_input,
  GlyphInfo *glyph,FT_BBox *bounds)
{
  const FT_Face
    face = glyph_input->face;

  const FT_Int32
    flags = glyph_input->flags;

  const DrawInfo
    *const draw_info = glyph_input->draw_info;

  const MagickBooleanType
    status = glyph_input->status;

  DrawInfo
    *const annotate_info = glyph_input->annotate_info;

  const FT_Matrix
    affine = glyph_input->affine;

  TypeMetric
    *const metrics = glyph_input->metrics;

  FT_Error
    ft_status;

  FT_Outline
    outline;

    (*glyph).image=DestroyFreetypeGlyph((*glyph).image);
    ft_status=FT_Load_Glyph(face,(*glyph).id,flags);
    if (ft_status != 0)
      return(MagickTrue);
    ft_status=FT_Get_Glyph(face->glyph,&(*glyph).image);
    if (ft_status != 0)
      return(MagickTrue);
    outline=((FT_OutlineGlyph) (*glyph).image)->outline;
    if (IsNonOutlineGlyph((*glyph).image,outline) != MagickFalse)
      return(MagickTrue);
    ft_status=FT_Outline_Get_BBox(&outline,bounds);
    if (ft_status != 0)
      return(MagickTrue);
    ExtendGlyphMinBounds((*bounds),metrics);
    ExtendGlyphMaxBounds((*bounds),metrics);
    if (IsRenderedStroke(draw_info,status) != MagickFalse)
      TraceGlyphOutline(annotate_info,(*glyph).origin,outline,&OutlineMethods);
    FT_Vector_Transform(&(*glyph).origin,&affine);
    (void) FT_Glyph_Transform((*glyph).image,&affine,&(*glyph).origin);
    ft_status=FT_Glyph_To_Bitmap(&(*glyph).image,FT_RENDER_MODE_NORMAL,
      (FT_Vector *) NULL,MagickTrue);
    if (ft_status != 0)
      return(MagickTrue);
  return(MagickFalse);
}

typedef struct _GlyphLoopInfo
{
  Image
    *image;

  const DrawInfo
    *draw_info;

  FT_Face
    face;

  FT_Int32
    flags;

  DrawInfo
    *annotate_info;

  FT_Matrix
    affine;

  TypeMetric
    *metrics;

  const PointInfo
    *offset;

  ExceptionInfo
    *exception;

  GraphemeInfo
    *grapheme;

  char
    *p;

  FT_UInt
    missing_glyph_id;

  ssize_t
    last_character;
} GlyphLoopInfo;

typedef struct _GlyphLoopTargets
{
  GlyphInfo
    *glyph;

  FT_Vector
    *origin;

  MagickBooleanType
    *status;

  FT_UInt
    *last_glyph_id;

  ssize_t
    *code;
} GlyphLoopTargets;

static MagickBooleanType RenderFreetypeGlyph(const GlyphLoopInfo *loop,
  const ssize_t i,const GlyphLoopTargets *targets)
{
  const DrawInfo
    *const draw_info = loop->draw_info;

  GraphemeInfo
    *const grapheme = loop->grapheme;

  GlyphInfo
    *const glyph = targets->glyph;

  FT_Vector
    *const origin = targets->origin;

  MagickBooleanType
    *const status = targets->status;

  FT_UInt
    *const last_glyph_id = targets->last_glyph_id;

  ssize_t
    *const code = targets->code;

  FT_BBox
    bounds;

  FT_BitmapGlyph
    bitmap;

  PointInfo
    point;

  /*
    Render UTF-8 sequence.
  */
  (*glyph).id=GetFreetypeGlyphId(grapheme+i,loop->missing_glyph_id);
  if (((*glyph).id != 0) && ((*last_glyph_id) != 0))
    (*origin).x+=(FT_Pos) (64.0*draw_info->kerning);
  (*glyph).origin=(*origin);
  (*glyph).origin.x+=(FT_Pos) grapheme[i].x_offset;
  (*glyph).origin.y+=(FT_Pos) grapheme[i].y_offset;
  {
    const GlyphLoadInput
      glyph_input = { loop->face, loop->flags, draw_info, (*status),
        loop->annotate_info, loop->affine, loop->metrics };

    if (LoadFreetypeGlyph(&glyph_input,&(*glyph),&bounds) != MagickFalse)
      return(MagickFalse);
  }
  bitmap=(FT_BitmapGlyph) (*glyph).image;
  point=GetGlyphBitmapPoint(loop->offset,bitmap,(*origin));
  if (draw_info->render != MagickFalse)
    {
      const GlyphBitmapTarget
        glyph_target = { loop->image, draw_info, bitmap, point,
          loop->annotate_info, loop->offset, loop->exception };

      (*status)=RasterizeGlyphBitmap(&glyph_target,(*status));
    }
  if (IsInterwordSpace(draw_info,loop->p+grapheme[i].cluster,(*code)) !=
      MagickFalse)
    (*origin).x+=(FT_Pos) (64.0*draw_info->interword_spacing);
  else
    if (i == loop->last_character)
      (*origin).x+=MagickMax((FT_Pos) grapheme[i].x_advance,bounds.xMax);
    else
      (*origin).x+=(FT_Pos) grapheme[i].x_advance;
  (*origin).y+=(FT_Pos) grapheme[i].y_advance;
  SetFreetypeMetricsOrigin(loop->metrics,(*origin));
  (*last_glyph_id)=(*glyph).id;
  (*code)=GetUTFCode(loop->p+grapheme[i].cluster);
  return(MagickTrue);
}

static void LogFreetypeFont(const DrawInfo *draw_info,const char *encoding)
{
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(AnnotateEvent,GetMagickModule(),"Font %s; "
      "font-encoding %s; text-encoding %s; pointsize %g",
      draw_info->font != (char *) NULL ? draw_info->font : "none",
      encoding != (char *) NULL ? encoding : "none",
      draw_info->encoding != (char *) NULL ? draw_info->encoding : "none",
      draw_info->pointsize);
}

typedef struct _GlyphRunInfo
{
  Image
    *image;

  const DrawInfo
    *draw_info;

  FT_Face
    face;

  FT_Int32
    flags;

  DrawInfo
    *annotate_info;

  FT_Matrix
    affine;

  TypeMetric
    *metrics;

  const PointInfo
    *offset;

  ExceptionInfo
    *exception;

  GraphemeInfo
    *grapheme;

  char
    *p;

  size_t
    length;

  unsigned char
    *utf8;

  FT_Memory
    memory;

  FT_Library
    library;

  FT_StreamRec
    *stream;
} GlyphRunInfo;

static MagickBooleanType RenderFreetypeGlyphs(const GlyphRunInfo *run)
{
  DrawInfo
    *annotate_info = run->annotate_info;

  FT_UInt
    last_glyph_id,
    missing_glyph_id;

  FT_Vector
    origin;

  GlyphInfo
    glyph;

  MagickBooleanType
    status;

  ssize_t
    code,
    i,
    last_character;

  const GlyphLoopTargets
    glyph_targets = { &glyph, &origin, &status, &last_glyph_id, &code };

  glyph.id=0;
  glyph.image=(FT_Glyph) NULL;
  last_glyph_id=0;
  origin.x=0;
  origin.y=0;
  status=MagickTrue;
  missing_glyph_id=FT_Get_Char_Index(run->face,' ');
  code=0;
  last_character=(ssize_t) run->length-1;
  for (i=0; i < (ssize_t) run->length; i++)
  {
    const GlyphLoopInfo
      loop = { run->image, run->draw_info, run->face, run->flags,
        annotate_info, run->affine, run->metrics, run->offset, run->exception,
        run->grapheme, run->p, missing_glyph_id, last_character };

    if (RenderFreetypeGlyph(&loop,i,&glyph_targets) == MagickFalse)
      continue;
  }
  RelinquishFreetypeText(run->grapheme,run->utf8);
  glyph.image=DestroyFreetypeGlyph(glyph.image);
  ScaleFreetypeMetrics(run->metrics);
  /*
    Relinquish resources.
  */
  annotate_info=DestroyDrawInfo(annotate_info);
  (void) FT_Done_Face(run->face);
  FreetypeDone(run->memory,run->library,run->stream);
  return(status);
}

static MagickBooleanType RenderFreetype(const FreetypeTarget *target,
  const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
{
  Image
    *image = target->image;

  const DrawInfo
    *draw_info = target->draw_info;

  const char
    *encoding = target->encoding;

#if !defined(FT_OPEN_PATHNAME)
#define FT_OPEN_PATHNAME  ft_open_pathname
#endif

  char
    *p;

  DrawInfo
    *annotate_info;

  FT_Face
    face;

  FT_Int32
    flags;

  FT_Library
    library;

  FT_Matrix
    affine;

  FT_Memory
    memory;

  FT_StreamRec
    *stream;

  FT_UInt
    first_glyph_id;

  GraphemeInfo
    *grapheme;

  size_t
    length;

  unsigned char
    *utf8;

  const FreetypeFaceTargets
    targets = { (&memory), (&library), (&stream), (&face) };

  if (OpenFreetypeFace(target,&targets,exception) == MagickFalse)
    return(MagickFalse);
  SetFreetypeMetrics(face,metrics);
  first_glyph_id=0;
  FT_Get_First_Char(face,&first_glyph_id);
  if (IsTextWithoutGlyphs(draw_info,first_glyph_id) != MagickFalse)
    {
      (void) FT_Done_Face(face);
      FreetypeDone(memory,library,stream);
      return(MagickTrue);
    }
  /*
    Compute bounding box.
  */
  LogFreetypeFont(draw_info,encoding);
  flags=GetFreetypeLoadFlags(draw_info);
  flags=GetFreetypeHintingFlags(image,flags,exception);
  affine=GetFreetypeAffine(draw_info);
  annotate_info=CloneFreetypeAnnotateInfo(draw_info);
  if (draw_info->render != MagickFalse)
    SetFreetypeCanvas(image,exception);
  p=FindInvalidUTFCode(draw_info);
  SelectFreetypeText(draw_info,&utf8,&p);
  grapheme=(GraphemeInfo *) NULL;
  length=strlen(p);
#if defined(MAGICKCORE_RAQM_DELEGATE)
  if (ContainsRaqmParagraphSeparator((const unsigned char *) p,length) == MagickFalse)
    length=ComplexRaqmTextLayout(image,draw_info,p,length,face,&grapheme,
      exception);
  else
#endif
  length=ComplexTextLayout(draw_info,p,length,face,flags,&grapheme);
  {
    const GlyphRunInfo
      run = { image, draw_info, face, flags, annotate_info, affine, metrics,
        offset, exception, grapheme, p, length, utf8, memory, library,
        stream };

    return(RenderFreetypeGlyphs(&run));
  }
}

#endif
