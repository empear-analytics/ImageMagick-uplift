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

  MagickCore private methods of annotate.c: the type lookup and RenderType,
  from GetDefaultTypeInfo to GetDensityResolution; included by that file only.
*/
#ifndef MAGICKCORE_ANNOTATE_TYPE_PRIVATE_H
#define MAGICKCORE_ANNOTATE_TYPE_PRIVATE_H

static const TypeInfo *GetDefaultTypeInfo(const DrawInfo *draw_info,
  const TypeInfo *type_info)
{
  ExceptionInfo
    *sans_exception;

  /*
    Search for a default font.
  */
  sans_exception=AcquireExceptionInfo();
  if (type_info == (const TypeInfo *) NULL)
    type_info=GetTypeInfoByFamily((const char *) NULL,draw_info->style,
      draw_info->stretch,draw_info->weight,sans_exception);
  if (type_info == (const TypeInfo *) NULL)
    type_info=GetTypeInfo("*",sans_exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  return(type_info);
}

static const TypeInfo *GetTypeInfoByFamilyList(const DrawInfo *draw_info,
  const TypeInfo *type_info,ExceptionInfo *exception)
{
  char
    **family;

  int
    number_families;

  ssize_t
    i;

  /*
    Parse font family list.
  */
  family=StringToArgv(draw_info->family,&number_families);
  for (i=1; i < (ssize_t) number_families; i++)
  {
    (void) SubstituteString(&family[i],",","");
    type_info=GetTypeInfoByFamily(family[i],draw_info->style,
      draw_info->stretch,draw_info->weight,exception);
    if ((type_info != (const TypeInfo *) NULL) &&
        (LocaleCompare(family[i],type_info->family) == 0))
      break;
  }
  for (i=0; i < (ssize_t) number_families; i++)
    family[i]=DestroyString(family[i]);
  family=(char **) RelinquishMagickMemory(family);
  if (type_info == (const TypeInfo *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),TypeWarning,
      "UnableToReadFont","`%s'",draw_info->family);
  return(type_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   R e n d e r T y p e                                                       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RenderType() renders text on the image.  It also returns the bounding box of
%  the text relative to the image.
%
%  The format of the RenderType method is:
%
%      MagickBooleanType RenderType(const RenderTarget *target,
%        const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o target: the image and the draw info.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static const TypeInfo *GetFamilyTypeInfo(const DrawInfo *draw_info,
  const TypeInfo *type_info,ExceptionInfo *exception)
{
  if (strpbrk(draw_info->family,",'\"") == (char *) NULL)
    type_info=GetTypeInfoByFamily(draw_info->family,draw_info->style,
      draw_info->stretch,draw_info->weight,exception);
  if (type_info == (const TypeInfo *) NULL)
    type_info=GetTypeInfoByFamilyList(draw_info,type_info,exception);
  return(type_info);
}

static void SetTypeInfoFace(DrawInfo *annotate_info,
  const TypeInfo *type_info)
{
  annotate_info->face=type_info->face;
  if (type_info->metrics != (char *) NULL)
    (void) CloneString(&annotate_info->metrics,type_info->metrics);
  if (type_info->glyphs != (char *) NULL)
    (void) CloneString(&annotate_info->font,type_info->glyphs);
}

static MagickBooleanType RenderTargetFreetype(const RenderTarget *target,
  const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
{
  Image
    *image = target->image;

  const DrawInfo
    *draw_info = target->draw_info;

  MagickBooleanType
    status;

  const FreetypeTarget ft_font = { image, draw_info, draw_info->encoding };
  status=RenderFreetype(&ft_font,offset,metrics,exception);
  return(status);
}

static MagickBooleanType RenderType(const RenderTarget *target,
  const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
{
  Image
    *image = target->image;

  const DrawInfo
    *draw_info = target->draw_info;

  char
    *font;

  const TypeInfo
    *type_info;

  DrawInfo
    *annotate_info;

  MagickBooleanType
    status;

  FreetypeTarget freetype;

  type_info=(const TypeInfo *) NULL;
  if (draw_info->font != (char *) NULL)
    {
      if (*draw_info->font == '@')
        return(RenderTargetFreetype(target,offset,metrics,exception));
      if (*draw_info->font == '-')
        return(RenderX11(image,draw_info,offset,metrics,exception));
      if (*draw_info->font == '^')
        return(RenderPostscript(image,draw_info,offset,metrics,exception));
      if (IsPathAccessible(draw_info->font) != MagickFalse)
        return(RenderTargetFreetype(target,offset,metrics,exception));
      type_info=GetTypeInfo(draw_info->font,exception);
      if (type_info == (const TypeInfo *) NULL)
        (void) ThrowMagickException(exception,GetMagickModule(),TypeWarning,
          "UnableToReadFont","`%s'",draw_info->font);
    }
  if ((type_info == (const TypeInfo *) NULL) &&
      (draw_info->family != (const char *) NULL))
    type_info=GetFamilyTypeInfo(draw_info,type_info,exception);
  font=GetPolicyValue("system:font");
  if (font != (char *) NULL)
    {
      if (IsPathAccessible(font) != MagickFalse)
        {
          /*
            Render with default system font.
          */
          annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
          annotate_info->font=font;
          freetype.image=image;
          freetype.draw_info=annotate_info;
          freetype.encoding=annotate_info->encoding;
          status=RenderFreetype(&freetype,offset,metrics,exception);
          annotate_info=DestroyDrawInfo(annotate_info);
          return(status);
        }
      font=DestroyString(font);
    }
  if (type_info == (const TypeInfo *) NULL)
    type_info=GetDefaultTypeInfo(draw_info,type_info);
  if (type_info == (const TypeInfo *) NULL)
    return(RenderTargetFreetype(target,offset,metrics,exception));
  annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  SetTypeInfoFace(annotate_info,type_info);
  freetype.image=image;
  freetype.draw_info=annotate_info;
  freetype.encoding=type_info->encoding;
  status=RenderFreetype(&freetype,offset,metrics,exception);
  annotate_info=DestroyDrawInfo(annotate_info);
  return(status);
}

static PointInfo ParseDensityResolution(const DrawInfo *draw_info,
  PointInfo resolution)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    geometry_flags;

  geometry_flags=ParseGeometry(draw_info->density,&geometry_info);
  if ((geometry_flags & RhoValue) != 0)
    resolution.x=geometry_info.rho;
  resolution.y=resolution.x;
  if ((geometry_flags & SigmaValue) != 0)
    resolution.y=geometry_info.sigma;
  return(resolution);
}

static PointInfo GetDensityResolution(const DrawInfo *draw_info)
{
  PointInfo
    resolution;

  resolution.x=DefaultResolution;
  resolution.y=DefaultResolution;
  if (draw_info->density != (char *) NULL)
    resolution=ParseDensityResolution(draw_info,resolution);
  return(resolution);
}

#endif
