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

  MagickCore private methods of annotate.c: the Raqm text layout, inside
  the MAGICKCORE_FREETYPE_DELEGATE branch, ahead of the FreeType helpers;
  included by that file only.
*/
#ifndef MAGICKCORE_ANNOTATE_RAQM_PRIVATE_H
#define MAGICKCORE_ANNOTATE_RAQM_PRIVATE_H

#if defined(MAGICKCORE_RAQM_DELEGATE)
static MagickBooleanType ContainsRaqmParagraphSeparator(const unsigned char *p,
  const size_t length)
{
  size_t 
    i;

  if ((p == (const unsigned char*) NULL) || (length == 0))
    return(MagickFalse);

  for (i = 0; i < length; i++)
  {
    /* LF or CR */
    if ((p[i] == '\n') || (p[i] == '\r'))
      return(MagickTrue);

    /* UTF-8 NEL U+0085 => C2 85 */
    if ((p[i] == 0xC2) && (i + 1 < length) && (p[i + 1] == 0x85))
      return(MagickTrue);

    /* UTF-8 LS/PS U+2028/U+2029 => E2 80 A8/A9 */
    if ((p[i] == 0xE2) && (i + 2 < length) && (p[i + 1] == 0x80) &&
        ((p[i + 2] == 0xA8) || (p[i + 2] == 0xA9)))
      return(MagickTrue);
  }

  return(MagickFalse);
}

static size_t ComplexRaqmTextLayout(const Image *image,
  const DrawInfo *draw_info,const char *text,const size_t length,
  const FT_Face face,GraphemeInfo **grapheme,ExceptionInfo *exception)
{
  const char
    *features;

  raqm_t
    *rq;

  raqm_glyph_t
    *glyphs;

  size_t
    extent;

  ssize_t
    i;

  extent=0;
  rq=raqm_create();
  if (rq == (raqm_t *) NULL)
    goto cleanup;
  if (raqm_set_text_utf8(rq,text,length) == 0)
    goto cleanup;
  if (raqm_set_par_direction(rq,(raqm_direction_t) draw_info->direction) == 0)
    goto cleanup;
  if (raqm_set_freetype_face(rq,face) == 0)
    goto cleanup;
  features=GetImageProperty(image,"type:features",exception);
  if (features != (const char *) NULL)
    {
      char
        breaker,
        quote,
        *token;

      int
        next,
        status_token;

      TokenInfo
        *token_info;

      next=0;
      token_info=AcquireTokenInfo();
      token=AcquireString("");
      status_token=Tokenizer(token_info,0,token,50,features,"",",","",'\0',
        &breaker,&next,&quote);
      while (status_token == 0)
      {
        raqm_add_font_feature(rq,token,(int) strlen(token));
        status_token=Tokenizer(token_info,0,token,50,features,"",",","",'\0',
          &breaker,&next,&quote);
      }
      token_info=DestroyTokenInfo(token_info);
      token=DestroyString(token);
    }
  if (raqm_layout(rq) == 0)
    goto cleanup;
  glyphs=raqm_get_glyphs(rq,&extent);
  if (glyphs == (raqm_glyph_t *) NULL)
    {
      extent=0;
      goto cleanup;
    }
  *grapheme=(GraphemeInfo *) AcquireQuantumMemory(extent,sizeof(**grapheme));
  if (*grapheme == (GraphemeInfo *) NULL)
    {
      extent=0;
      goto cleanup;
    }
  for (i=0; i < (ssize_t) extent; i++)
  {
    (*grapheme)[i].index=glyphs[i].index;
    (*grapheme)[i].x_offset=glyphs[i].x_offset;
    (*grapheme)[i].x_advance=glyphs[i].x_advance;
    (*grapheme)[i].y_offset=glyphs[i].y_offset;
    (*grapheme)[i].y_advance=glyphs[i].y_advance;
    (*grapheme)[i].cluster=glyphs[i].cluster;
  }

cleanup:
  raqm_destroy(rq);
  return(extent);
}
#endif

#endif
