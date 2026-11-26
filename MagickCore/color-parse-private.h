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

  MagickCore private methods of color.c: the color cache loader and the
  color specification parsers, IsIncludeElementOpen to SkipColorNameSpaces;
  included by that file only.
*/
#ifndef MAGICKCORE_COLOR_PARSE_PRIVATE_H
#define MAGICKCORE_COLOR_PARSE_PRIVATE_H

#if !MAGICKCORE_ZERO_CONFIGURATION_SUPPORT
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   L o a d C o l o r C a c h e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadColorCache() loads the color configurations which provides a mapping
%  between color attributes and a color name.
%
%  The format of the LoadColorCache method is:
%
%      MagickBooleanType LoadColorCache(const ColorLoadTarget *target,
%        const char *xml,const char *filename,const size_t depth)
%
%  A description of each parameter follows:
%
%    o target: the cache to load into, and the exception structure to
%      return any errors or warnings in.
%
%    o xml:  The color list in XML format.
%
%    o filename:  The color list filename.
%
%    o depth: depth of <include /> statements.
%
*/
static inline MagickBooleanType IsIncludeElementOpen(const char *token,
  const char *q)
{
  return((((*token != '/') && (*(token+1) != '>')) && (*q != '\0')) ?
    MagickTrue : MagickFalse);
}

static void SetColorInfoCompliance(ColorInfo *color_info,const char *token)
{
  ssize_t
    compliance;

  compliance=color_info->compliance;
  if (StringLocateSubstring(token,"SVG") != (char *) NULL)
    compliance|=SVGCompliance;
  if (StringLocateSubstring(token,"X11") != (char *) NULL)
    compliance|=X11Compliance;
  if (StringLocateSubstring(token,"XPM") != (char *) NULL)
    compliance|=XPMCompliance;
  color_info->compliance=(ComplianceType) compliance;
}

static void GetIncludePath(const char *filename,const char *token,
  char *path)
{
  GetPathComponent(filename,HeadPath,path);
  if (*path != '\0')
    (void) ConcatenateMagickString(path,DirectorySeparator,
      MagickPathExtent);
  if (*token == *DirectorySeparator)
    (void) CopyMagickString(path,token,MagickPathExtent);
  else
    (void) ConcatenateMagickString(path,token,MagickPathExtent);
}

static void SetColorInfoAttribute(ColorInfo *color_info,const char *keyword,
  const char *token,ExceptionInfo *exception)
{
  if (LocaleCompare((char *) keyword,"color") == 0)
    (void) QueryColorCompliance(token,AllCompliance,&color_info->color,
      exception);
  else if (LocaleCompare((char *) keyword,"compliance") == 0)
    SetColorInfoCompliance(color_info,token);
  else if (LocaleCompare((char *) keyword,"name") == 0)
    color_info->name=ConstantString(token);
  else if (LocaleCompare((char *) keyword,"stealth") == 0)
    color_info->stealth=IsStringTrue(token);
}

static ColorInfo *AcquireColorInfo(const char *filename)
{
  ColorInfo
    *color_info;

  color_info=(ColorInfo *) AcquireCriticalMemory(sizeof(*color_info));
  (void) memset(color_info,0,sizeof(*color_info));
  color_info->path=ConstantString(filename);
  color_info->exempt=MagickFalse;
  color_info->signature=MagickCoreSignature;
  return(color_info);
}

static MagickStatusType LoadIncludedColorFile(const ColorLoadTarget *target,
  const char *filename,const char *token,const size_t depth,
  MagickStatusType status)
{
  LinkedListInfo
    *cache = target->cache;

  ExceptionInfo
    *exception = target->exception;

  if (depth >= MagickMaxRecursionDepth)
    (void) ThrowMagickException(exception,GetMagickModule(),
      ConfigureError,"IncludeElementNestedTooDeeply","`%s'",token);
  else
    {
      const ColorLoadTarget include_target = { cache, exception };

      char
        path[MagickPathExtent],
        *file_xml;

      GetIncludePath(filename,token,path);
      file_xml=FileToXML(path,~0UL);
      if (file_xml != (char *) NULL)
        {
          status&=(MagickStatusType) LoadColorCache(
            &include_target,file_xml,path,depth+1);
          file_xml=DestroyString(file_xml);
        }
    }
  return(status);
}

static const char *SkipQuotedText(const char *q)
{
  const char
    quote = (*q),
    *p = q+1;

  /*
    Return the last character of the quoted text: the closing quote, or the
    last character of the string if the quote is not closed.
  */
  while ((*p != '\0') && (*p != quote))
    p++;
  return(*p == '\0' ? p-1 : p);
}

static const char *SkipDoctypeElement(const char *q)
{
  int
    bracket_depth = 0;

  while (*q != '\0')
  {
    if (strchr("\"'",*q) != (char *) NULL)
      q=SkipQuotedText(q);
    else if (*q == '[')
      bracket_depth++;
    else if ((*q == ']') && (bracket_depth > 0))
      bracket_depth--;
    else if ((*q == '>') && (bracket_depth == 0))
      return(q+1);   /* consume final '>' */
    q++;
  }
  return(q);
}

static MagickStatusType LoadIncludeElement(const ColorLoadTarget *target,
  const char *filename,const size_t depth,const size_t extent,char *token,
  const char **q,MagickStatusType status)
{
  char
    keyword[MagickPathExtent];

  while (IsIncludeElementOpen(token,*q) != MagickFalse)
  {
    (void) CopyMagickString(keyword,token,MagickPathExtent);
    (void) GetNextToken(*q,q,extent,token);
    if (*token != '=')
      continue;
    (void) GetNextToken(*q,q,extent,token);
    if (LocaleCompare(keyword,"file") == 0)
      status=LoadIncludedColorFile(target,filename,token,depth,status);
  }
  return(status);
}

static MagickStatusType AppendColorInfoToCache(LinkedListInfo *cache,
  ColorInfo *color_info,ExceptionInfo *exception)
{
  MagickStatusType
    status;

  status=AppendValueToLinkedList(cache,color_info);
  if (status == MagickFalse)
    (void) ThrowMagickException(exception,GetMagickModule(),
      ResourceLimitError,"MemoryAllocationFailed","`%s'",color_info->name);
  return(status);
}

static const char *SkipCommentElement(const char *q,const size_t extent,
  char *token)
{
  while ((LocaleNCompare(q,"->",2) != 0) && (*q != '\0'))
    (void) GetNextToken(q,&q,extent,token);
  return(q);
}

static const char *ReadColorInfoAttribute(ColorInfo *color_info,
  const char *keyword,const char *q,const size_t extent,char *token,
  ExceptionInfo *exception)
{
  (void) GetNextToken(q,(const char **) NULL,extent,token);
  if (*token != '=')
    return(q);
  (void) GetNextToken(q,&q,extent,token);
  (void) GetNextToken(q,&q,extent,token);
  SetColorInfoAttribute(color_info,keyword,token,exception);
  return(q);
}

static inline MagickBooleanType IsElementEnd(const char *keyword)
{
  return(((LocaleCompare(keyword,"/>") == 0) ||
    (LocaleCompare(keyword,"</policy>") == 0)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType SkipMarkupElement(const char *keyword,
  const char **q,const size_t extent,char *token)
{
  if (LocaleNCompare(keyword,"<!DOCTYPE",9) == 0)
    {
      /*
        DOCTYPE element.
      */
      (*q)=SkipDoctypeElement((*q));
      return(MagickTrue);
    }
  if (LocaleNCompare(keyword,"<!--",4) == 0)
    {
      /*
        Comment element.
      */
      (*q)=SkipCommentElement((*q),extent,token);
      return(MagickTrue);
    }
  return(MagickFalse);
}

typedef struct _ColorConfigureInput
{
  const ColorLoadTarget
    *target;

  LinkedListInfo
    *cache;

  ExceptionInfo
    *exception;

  const char
    *filename;

  size_t
    depth,
    extent;

  char
    *keyword,
    *token;
} ColorConfigureInput;

static void ParseColorConfigureToken(const ColorConfigureInput *input,
  const char **q,MagickStatusType *status,ColorInfo **color_info)
{
  (void) CopyMagickString(input->keyword,input->token,MagickPathExtent);
  if (SkipMarkupElement(input->keyword,q,input->extent,input->token) !=
      MagickFalse)
    return;
  if (LocaleCompare(input->keyword,"<include") == 0)
    {
      /*
        Include element.
      */
      (*status)=LoadIncludeElement(input->target,input->filename,input->depth,
        input->extent,input->token,q,(*status));
      return;
    }
  if (LocaleCompare(input->keyword,"<color") == 0)
    {
      /*
        Color element.
      */
      (*color_info)=AcquireColorInfo(input->filename);
      return;
    }
  if ((*color_info) == (ColorInfo *) NULL)
    return;
  if (IsElementEnd(input->keyword) != MagickFalse)
    {
      (*status)=AppendColorInfoToCache(input->cache,(*color_info),
        input->exception);
      (*color_info)=(ColorInfo *) NULL;
    }
  else
    (*q)=ReadColorInfoAttribute((*color_info),input->keyword,(*q),
      input->extent,input->token,input->exception);
}

static MagickBooleanType LoadColorCache(const ColorLoadTarget *target,
  const char *xml,const char *filename,const size_t depth)
{
  LinkedListInfo
    *cache = target->cache;

  ExceptionInfo
    *exception = target->exception;

  char
    keyword[MagickPathExtent],
    *token;

  ColorInfo
    *color_info;

  const char
    *q;

  MagickStatusType
    status;

  size_t
    extent;

  /*
    Load the color map file.
  */
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Loading color file \"%s\" ...",filename);
  if (xml == (char *) NULL)
    return(MagickFalse);
  status=MagickTrue;
  color_info=(ColorInfo *) NULL;
  token=AcquireString(xml);
  extent=strlen(token)+MagickPathExtent;
  for (q=(char *) xml; *q != '\0'; )
  {
    /*
      Interpret XML.
    */
    (void) GetNextToken(q,&q,extent,token);
    if (*token == '\0')
      break;
    {
      const ColorConfigureInput
        input = { target, cache, exception, filename, depth, extent, keyword,
          token };

      ParseColorConfigureToken(&input,&q,&status,&color_info);
    }
  }
  token=(char *) RelinquishMagickMemory(token);
  return((MagickBooleanType) (status != 0));
}
#endif

static char *SkipCSSSeparators(char *q,MagickStatusType *flags)
{
  while (isspace((int) ((unsigned char) *q)) != 0)
    q++;
  if (*q == ',')
    q++;
  if (*q == '/')
    {
      *flags|=AlphaValue;
      q++;
    }
  return(q);
}

static void SetCSSComponent(GeometryInfo *geometry_info,const ssize_t i,
  const double intensity,char **q,MagickStatusType *flags)
{
  switch (i)
  {
    case 0:
    {
      geometry_info->rho=intensity;
      *flags|=RhoValue;
      if (LocaleNCompare(*q,"deg",3) == 0)
        *q+=(ptrdiff_t) 3;
      break;
    }
    case 1:
    {
      geometry_info->sigma=intensity;
      *flags|=SigmaValue;
      break;
    }
    case 2:
    {
      geometry_info->xi=intensity;
      *flags|=XiValue;
      break;
    }
    case 3:
    {
      geometry_info->psi=intensity;
      *flags|=PsiValue;
      break;
    }
    case 4:
    {
      geometry_info->chi=intensity;
      *flags|=ChiValue;
      break;
    }
  }
}

static inline MagickBooleanType IsCSSComponentOpen(const ssize_t i,
  const char *q)
{
  return(((i < 5) && (*q != ')') && (*q != '\0')) ? MagickTrue :
    MagickFalse);
}

static MagickStatusType ParseCSSColor(const char *magick_restrict color,
  GeometryInfo *geometry_info)
{
  char
    *q;

  ssize_t
    i;

  MagickStatusType
    flags;

  SetGeometryInfo(geometry_info);
  flags=NoValue;
  if ((color == (char *) NULL) || (*color == '\0'))
    return(flags);
  q=(char *) color;
  if (*q == '(')
    q++;
  for (i=0; IsCSSComponentOpen(i,q) != MagickFalse; i++)
  {
    char
      *p;

    double
      intensity;

    p=q;
    intensity=StringToFloat(p,&q);
    if (p == q)
      break;
    if (*q == '%')
      {
        intensity*=0.01*255.0;
        q++;
      }
    SetCSSComponent(geometry_info,i,intensity,&q,&flags);
    q=SkipCSSSeparators(q,&flags);
  }
  return(flags);
}

static inline MagickBooleanType IsHexColorLength(const size_t n)
{
  return((MagickBooleanType) ((n == 3) || (n == 6) || (n == 9) ||
    (n == 12) || (n == 24) || (n == 48)));
}

static inline MagickBooleanType IsInvalidHexAlphaLength(const size_t n)
{
  return(((n != 4) && (n != 8) && (n != 16) && (n != 32) && (n != 64)) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsDecimalDigitCharacter(const char c)
{
  return((MagickBooleanType) ((c >= '0') && (c <= '9')));
}

static inline MagickBooleanType IsUpperHexLetter(const char c)
{
  return((MagickBooleanType) ((c >= 'A') && (c <= 'F')));
}

static inline MagickBooleanType IsLowerHexLetter(const char c)
{
  return((MagickBooleanType) ((c >= 'a') && (c <= 'f')));
}

static inline MagickBooleanType IsHueColorspaceName(const char *colorspace)
{
  return((MagickBooleanType) ((LocaleCompare(colorspace,"HCL") == 0) ||
    (LocaleCompare(colorspace,"HSB") == 0) ||
    (LocaleCompare(colorspace,"HSL") == 0) ||
    (LocaleCompare(colorspace,"HSV") == 0) ||
    (LocaleCompare(colorspace,"HWB") == 0)));
}

static void SetOklchColorFromGeometry(PixelInfo *color,
  const GeometryInfo geometry_info,const MagickStatusType flags)
{
  if ((flags & RhoValue) != 0)
    color->red=(MagickRealType) ClampToQuantum((double) QuantumRange*
       geometry_info.rho);
  if ((flags & SigmaValue) != 0)
    color->green=(MagickRealType) ClampToQuantum((double) QuantumRange*
       geometry_info.sigma);
  if ((flags & XiValue) != 0)
    color->blue=(MagickRealType) ClampToQuantum((double) QuantumRange*
       DegreesToRadians(geometry_info.xi));
}

static void SetLabColorFromGeometry(PixelInfo *color,
  const GeometryInfo geometry_info,const MagickStatusType flags,
  const double scale)
{
  color->red=(MagickRealType) ClampToQuantum((MagickRealType)
    ((double) QuantumRange*geometry_info.rho/100.0));
  if ((flags & SigmaValue) != 0)
    color->green=(MagickRealType) ClampToQuantum((MagickRealType)
      (scale*geometry_info.sigma+((double) QuantumRange+1)/2.0));
  if ((flags & XiValue) != 0)
    color->blue=(MagickRealType) ClampToQuantum((MagickRealType)
      (scale*geometry_info.xi+((double) QuantumRange+1)/2.0));
}

static void SetScaledColorFromGeometry(PixelInfo *color,
  const GeometryInfo geometry_info,const MagickStatusType flags,
  const double scale)
{
  if ((flags & RhoValue) != 0)
    color->red=(double) ClampToQuantum((MagickRealType) (scale*
      geometry_info.rho));
  if ((flags & SigmaValue) != 0)
    color->green=(double) ClampToQuantum((MagickRealType) (scale*
      geometry_info.sigma));
  if ((flags & XiValue) != 0)
    color->blue=(double) ClampToQuantum((MagickRealType) (scale*
      geometry_info.xi));
}

static void SetPsiColorFromGeometry(PixelInfo *color,
  const GeometryInfo geometry_info,const MagickStatusType flags,
  const double scale)
{
  if (color->colorspace == CMYKColorspace)
    color->black=(double) ClampToQuantum((MagickRealType) (scale*
      geometry_info.psi));
  else
    if (color->alpha_trait != UndefinedPixelTrait)
      {
        if ((flags & AlphaValue) != 0)
          color->alpha=(double) ClampToQuantum((MagickRealType) (scale*
            geometry_info.psi));
        else
          color->alpha=(double) ClampToQuantum((MagickRealType) (
            (double) QuantumRange*geometry_info.psi));
      }
}

static void SetHueColorFromGeometry(PixelInfo *color,
  GeometryInfo geometry_info)
{
  double
    blue,
    green,
    red,
    scale;

  scale=1.0/255.0;
  geometry_info.sigma*=scale;
  geometry_info.xi*=scale;
  red=0.0;
  green=0.0;
  blue=0.0;
  switch (color->colorspace)
  {
    case HCLColorspace:
    {
      ConvertHCLToRGB(fmod(fmod(geometry_info.rho,360.0)+360.0,360.0)/
        360.0,geometry_info.sigma,geometry_info.xi,&red,&green,&blue);
      break;
    }
    case HSBColorspace:
    {
      ConvertHSBToRGB(fmod(fmod(geometry_info.rho,360.0)+360.0,360.0)/
        360.0,geometry_info.sigma,geometry_info.xi,&red,&green,&blue);
      break;
    }
    case HSLColorspace:
    {
      ConvertHSLToRGB(fmod(fmod(geometry_info.rho,360.0)+360.0,360.0)/
        360.0,geometry_info.sigma,geometry_info.xi,&red,&green,&blue);
      break;
    }
    case HSVColorspace:
    {
      ConvertHSVToRGB(fmod(fmod(geometry_info.rho,360.0)+360.0,360.0)/
        360.0,geometry_info.sigma,geometry_info.xi,&red,&green,&blue);
      break;
    }
    case HWBColorspace:
    {
      ConvertHWBToRGB(fmod(fmod(geometry_info.rho,360.0)+360.0,360.0)/
        360.0,geometry_info.sigma,geometry_info.xi,&red,&green,&blue);
      break;
    }
    default:
      break;
  }
  color->colorspace=sRGBColorspace;
  color->red=(MagickRealType) red;
  color->green=(MagickRealType) green;
  color->blue=(MagickRealType) blue;
}

static void SetGrayColorFromGeometry(PixelInfo *color,
  const GeometryInfo geometry_info,const MagickStatusType flags)
{
  color->green=color->red;
  color->blue=color->red;
  if (((flags & SigmaValue) != 0) &&
      (color->alpha_trait != UndefinedPixelTrait))
    color->alpha=(double) ClampToQuantum((double) QuantumRange*
      geometry_info.sigma);
}

static MagickBooleanType QueryColorFromCSSName(const char *name,
  const ssize_t i,PixelInfo *color,ExceptionInfo *exception)
{
  char
    *colorname;

  ColorspaceType
    colorspaceType;

  MagickBooleanType
    status;

  colorspaceType=color->colorspace;
  if (i >= (ssize_t) strlen(name))
    colorname=AcquireString(name);
  else
    colorname=AcquireString(name+i+1);
  (void) SubstituteString(&colorname,"(","");
  (void) SubstituteString(&colorname,")","");
  status=MagickFalse;
  if (LocaleCompare(name,colorname) != 0)
    status=QueryColorCompliance(colorname,AllCompliance,color,
      exception);
  color->colorspace=colorspaceType;
  if (*colorname == '\0')
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionWarning,"UnrecognizedColor","`%s'",name);
      status=MagickFalse;
    }
  colorname=DestroyString(colorname);
  return(status);
}

static void SetDeviceColorspace(const char *name,char *colorspace,
  double *scale,MagickBooleanType *icc_color)
{
  (void) CopyMagickString(colorspace,colorspace+7,MagickPathExtent);
  if (strchr(name,'%') == (char *) NULL)
    *scale=(double) QuantumRange;
  *icc_color=MagickTrue;
}

static void SetICCColorspace(const char *name,char *colorspace,ssize_t *i,
  double *scale,MagickBooleanType *icc_color)
{
  ssize_t
    j;

  (void) CopyMagickString(colorspace,name+(*i)+2,MagickPathExtent);
  for (j=0; colorspace[j] != '\0'; j++)
    if ((colorspace[j] == ' ') || (colorspace[j] == ','))
      break;
  colorspace[j--]='\0';
  *i+=j+3;
  *scale=(double) QuantumRange;
  *icc_color=MagickTrue;
}

static void SetColorFromGeometry(PixelInfo *color,
  const GeometryInfo geometry_info,const MagickStatusType flags,
  const double scale)
{
  if (color->colorspace == OklchColorspace)
     SetOklchColorFromGeometry(color,geometry_info,flags);
  else
    if (IsLabCompatibleColorspace(color->colorspace) != MagickFalse)
      SetLabColorFromGeometry(color,geometry_info,flags,scale);
    else
      SetScaledColorFromGeometry(color,geometry_info,flags,scale);
  if ((flags & AlphaValue) != 0)
    color->alpha_trait=BlendPixelTrait;
  color->alpha=(double) OpaqueAlpha;
  if ((flags & PsiValue) != 0)
    SetPsiColorFromGeometry(color,geometry_info,flags,scale);
}

static void SetColorChannelsFromGeometry(PixelInfo *color,
  const GeometryInfo geometry_info,const MagickStatusType flags,
  const char *colorspace)
{
  if (((flags & ChiValue) != 0) &&
      (color->alpha_trait != UndefinedPixelTrait))
    color->alpha=(double) ClampToQuantum((double) QuantumRange*
      geometry_info.chi);
  if ((LocaleCompare(colorspace,"gray") == 0) ||
      (LocaleCompare(colorspace,"lineargray") == 0))
    SetGrayColorFromGeometry(color,geometry_info,flags);
  if (IsHueColorspaceName(colorspace) != MagickFalse)
    SetHueColorFromGeometry(color,geometry_info);
}

static void StripColorspaceAlphaSuffix(char *colorspace,const ssize_t i,
  PixelInfo *color)
{
  color->alpha_trait=UndefinedPixelTrait;
  if ((i > 0) && (colorspace[i] == 'a'))
    {
      colorspace[i]='\0';
      color->alpha_trait=BlendPixelTrait;
    }
}

static void SetQueriedColorspace(PixelInfo *color,const ssize_t type,
  const MagickBooleanType icc_color)
{
  color->colorspace=(ColorspaceType) type;
  if ((icc_color == MagickFalse) && (color->colorspace == RGBColorspace))
    {
      color->colorspace=sRGBColorspace;  /* as required by SVG standard */
      color->depth=8;
    }
}

static MagickStatusType ParseQueriedColorGeometry(const char *name,
  const ssize_t i,GeometryInfo *geometry_info)
{
  MagickStatusType
    flags;

  if (i >= (ssize_t) strlen(name))
    flags=ParseCSSColor(name,geometry_info);
  else
    flags=ParseCSSColor(name+i+1,geometry_info);
  return(flags);
}

static MagickBooleanType QueryColorFunction(const char *name,
  const char *q,PixelInfo *color,ExceptionInfo *exception)
{
  double
    scale;

  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  ssize_t
    i,
    type;

  char
    colorspace[2*MagickPathExtent];

  MagickBooleanType
    icc_color;

  /*
    Parse color of the form rgb(100,255,0).
  */
  i=(ssize_t) (q-name);
  (void) CopyMagickString(colorspace,name,MagickPathExtent);
  colorspace[i--]='\0';
  scale=(double) ScaleCharToQuantum(1);
  icc_color=MagickFalse;
  if (LocaleNCompare(colorspace,"device-",7) == 0)
    SetDeviceColorspace(name,colorspace,&scale,&icc_color);
  if ((LocaleCompare(colorspace,"color") == 0) ||
      (LocaleCompare(colorspace,"icc-color") == 0))
    SetICCColorspace(name,colorspace,&i,&scale,&icc_color);
  LocaleLower(colorspace);
  StripColorspaceAlphaSuffix(colorspace,i,color);
  type=ParseCommandOption(MagickColorspaceOptions,MagickFalse,colorspace);
  if (type < 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionWarning,"UnrecognizedColor","`%s'",name);
      return(MagickFalse);
    }
  SetQueriedColorspace(color,type,icc_color);
  flags=ParseQueriedColorGeometry(name,i,&geometry_info);
  if (flags == 0)
    return(QueryColorFromCSSName(name,i,color,exception));
  SetColorFromGeometry(color,geometry_info,flags,scale);
  SetColorChannelsFromGeometry(color,geometry_info,flags,colorspace);
  return(MagickTrue);
}

static MagickBooleanType ReadHexRGBDigits(const char *name,const size_t n,
  PixelPacket *pixel)
{
  char
    c;

  ssize_t
    i;

  do
  {
    (*pixel).red=(*pixel).green;
    (*pixel).green=(*pixel).blue;
    (*pixel).blue=0;
    for (i=(ssize_t) (n/3-1); i >= 0; i--)
    {
      c=(*name++);
      (*pixel).blue<<=4;
      if (IsDecimalDigitCharacter(c) != MagickFalse)
        (*pixel).blue|=(unsigned int) (c-'0');
      else if (IsUpperHexLetter(c) != MagickFalse)
        (*pixel).blue|=(unsigned int) (c-('A'-10));
      else if (IsLowerHexLetter(c) != MagickFalse)
        (*pixel).blue|=(unsigned int) (c-('a'-10));
      else
        return(MagickFalse);
    }
  } while (isxdigit((int) ((unsigned char) *name)) != 0);
  return(MagickTrue);
}

static MagickBooleanType SetHexColor(PixelInfo *color,
  const PixelPacket *pixel,const size_t depth,const size_t n)
{
  QuantumAny
    range;

  color->colorspace=sRGBColorspace;
  color->depth=depth;
  color->alpha_trait=UndefinedPixelTrait;
  range=GetQuantumRange(depth);
  color->red=(double) ScaleAnyToQuantum((*pixel).red,range);
  color->green=(double) ScaleAnyToQuantum((*pixel).green,range);
  color->blue=(double) ScaleAnyToQuantum((*pixel).blue,range);
  color->alpha=(double) OpaqueAlpha;
  if ((n % 3) != 0)
    {
      color->alpha_trait=BlendPixelTrait;
      color->alpha=(double) ScaleAnyToQuantum((*pixel).alpha,range);
    }
  color->black=0.0;
  return(MagickTrue);
}

static MagickBooleanType ReadHexRGBADigits(const char *name,const size_t n,
  PixelPacket *pixel,ExceptionInfo *exception)
{
  char
    c;

  ssize_t
    i;

  if (IsInvalidHexAlphaLength(n) != MagickFalse)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionWarning,"UnrecognizedColor","`%s'",name);
      return(MagickFalse);
    }
  do
  {
    (*pixel).red=(*pixel).green;
    (*pixel).green=(*pixel).blue;
    (*pixel).blue=(*pixel).alpha;
    (*pixel).alpha=0;
    for (i=(ssize_t) (n/4-1); i >= 0; i--)
    {
      c=(*name++);
      (*pixel).alpha<<=4;
      if (IsDecimalDigitCharacter(c) != MagickFalse)
        (*pixel).alpha|=(unsigned int) (c-'0');
      else if (IsUpperHexLetter(c) != MagickFalse)
        (*pixel).alpha|=(unsigned int) c-('A'-10);
      else if (IsLowerHexLetter(c) != MagickFalse)
        (*pixel).alpha|=(unsigned int) c-('a'-10);
      else
        return(MagickFalse);
    }
  } while (isxdigit((int) ((unsigned char) *name)) != 0);
  return(MagickTrue);
}

static MagickBooleanType QueryHexRGBAColor(const char *name,const size_t n,
  PixelInfo *color,ExceptionInfo *exception)
{
  PixelPacket
    pixel = { 0, 0, 0, 0, 0 };

  size_t
    depth;

  if (ReadHexRGBADigits(name,n,&pixel,exception) == MagickFalse)
    return(MagickFalse);
  depth=4*(n/4);
  return(SetHexColor(color,&pixel,depth,n));
}

static MagickBooleanType QueryHexColor(const char *name,PixelInfo *color,
  ExceptionInfo *exception)
{
  PixelPacket
    pixel = { 0, 0, 0, 0, 0 };

  size_t
    depth,
    n;

  /*
    Parse hex color.
  */
  name++;
  for (n=0; isxdigit((int) ((unsigned char) name[n])) != 0; n++) ;
  if (IsHexColorLength(n) != MagickFalse)
    {
      if (ReadHexRGBDigits(name,n,&pixel) == MagickFalse)
        return(MagickFalse);
      depth=4*(n/3);
    }
  else
    return(QueryHexRGBAColor(name,n,color,exception));
  return(SetHexColor(color,&pixel,depth,n));
}

static void SetNamedColor(PixelInfo *color,const char *name,
  const ColorInfo *p)
{
  color->colorspace=sRGBColorspace;
  if ((LocaleNCompare(name,"gray",4) == 0) ||
      (LocaleNCompare(name,"grey",4) == 0))
    color->colorspace=GRAYColorspace;
  color->depth=8;
  color->alpha_trait=p->color.alpha != (double) OpaqueAlpha ? BlendPixelTrait :
    UndefinedPixelTrait;
  color->red=(double) p->color.red;
  color->green=(double) p->color.green;
  color->blue=(double) p->color.blue;
  color->alpha=(double) p->color.alpha;
  color->black=0.0;
}

static const char *GetQueryColorName(const char *name)
{
  if ((name == (char *) NULL) || (*name == '\0'))
    name=BackgroundColor;
  return(name);
}

static const char *SkipColorNameSpaces(const char *name)
{
  while (isspace((int) ((unsigned char) *name)) != 0)
    name++;
  return(name);
}

#endif
