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

  FT_BBox
    bounds;

  FT_BitmapGlyph
    bitmap;

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
    first_glyph_id,
    last_glyph_id,
    missing_glyph_id;

  FT_Vector
    origin;

  GlyphInfo
    glyph;

  GraphemeInfo
    *grapheme;

  FreetypeFaceTargets
    targets;

  MagickBooleanType
    status;

  PointInfo
    point;

  ssize_t
    i;

  size_t
    length;

  ssize_t
    code,
    last_character;

  unsigned char
    *utf8;

  targets.memory=(&memory);
  targets.library=(&library);
  targets.stream=(&stream);
  targets.face=(&face);
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
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(AnnotateEvent,GetMagickModule(),"Font %s; "
      "font-encoding %s; text-encoding %s; pointsize %g",
      draw_info->font != (char *) NULL ? draw_info->font : "none",
      encoding != (char *) NULL ? encoding : "none",
      draw_info->encoding != (char *) NULL ? draw_info->encoding : "none",
      draw_info->pointsize);
  flags=GetFreetypeLoadFlags(draw_info);
  flags=GetFreetypeHintingFlags(image,flags,exception);
  glyph.id=0;
  glyph.image=(FT_Glyph) NULL;
  last_glyph_id=0;
  origin.x=0;
  origin.y=0;
  affine=GetFreetypeAffine(draw_info);
  annotate_info=CloneFreetypeAnnotateInfo(draw_info);
  status=MagickTrue;
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
  missing_glyph_id=FT_Get_Char_Index(face,' ');
  code=0;
  last_character=(ssize_t) length-1;
  for (i=0; i < (ssize_t) length; i++)
  {
    /*
      Render UTF-8 sequence.
    */
    glyph.id=GetFreetypeGlyphId(grapheme+i,missing_glyph_id);
    if ((glyph.id != 0) && (last_glyph_id != 0))
      origin.x+=(FT_Pos) (64.0*draw_info->kerning);
    glyph.origin=origin;
    glyph.origin.x+=(FT_Pos) grapheme[i].x_offset;
    glyph.origin.y+=(FT_Pos) grapheme[i].y_offset;
    {
      const GlyphLoadInput
        glyph_input = { face, flags, draw_info, status, annotate_info, affine,
          metrics };

      if (LoadFreetypeGlyph(&glyph_input,&glyph,&bounds) != MagickFalse)
        continue;
    }
    bitmap=(FT_BitmapGlyph) glyph.image;
    point=GetGlyphBitmapPoint(offset,bitmap,origin);
    if (draw_info->render != MagickFalse)
      {
        const GlyphBitmapTarget
          glyph_target = { image, draw_info, bitmap, point, annotate_info,
            offset, exception };

        status=RasterizeGlyphBitmap(&glyph_target,status);
      }
    if (IsInterwordSpace(draw_info,p+grapheme[i].cluster,code) != MagickFalse)
      origin.x+=(FT_Pos) (64.0*draw_info->interword_spacing);
    else
      if (i == last_character)
        origin.x+=MagickMax((FT_Pos) grapheme[i].x_advance,bounds.xMax);
      else
        origin.x+=(FT_Pos) grapheme[i].x_advance;
    origin.y+=(FT_Pos) grapheme[i].y_advance;
    SetFreetypeMetricsOrigin(metrics,origin);
    last_glyph_id=glyph.id;
    code=GetUTFCode(p+grapheme[i].cluster);
  }
  RelinquishFreetypeText(grapheme,utf8);
  glyph.image=DestroyFreetypeGlyph(glyph.image);
  ScaleFreetypeMetrics(metrics);
  /*
    Relinquish resources.
  */
  annotate_info=DestroyDrawInfo(annotate_info);
  (void) FT_Done_Face(face);
  FreetypeDone(memory,library,stream);
  return(status);
}

#endif
