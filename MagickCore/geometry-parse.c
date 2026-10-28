/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%            GGGG  EEEEE   OOO   M   M  EEEEE  TTTTT  RRRR   Y   Y            %
%           G      E      O   O  MM MM  E        T    R   R   Y Y             %
%           G  GG  EEE    O   O  M M M  EEE      T    RRRR     Y              %
%           G   G  E      O   O  M   M  E        T    R R      Y              %
%            GGG   EEEEE   OOO   M   M  EEEEE    T    R  R     Y              %
%                                                                             %
%                      PPPP    AAA   RRRR   SSSSS  EEEEE                      %
%                      P   P  A   A  R   R  SS     E                          %
%                      PPPP   AAAAA  RRRR    SSS   EEE                        %
%                      P      A   A  R R       SS  E                          %
%                      P      A   A  R  R   SSSSS  EEEEE                      %
%                                                                             %
%                                                                             %
%                 MagickCore Geometry String Parsing Methods                  %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%  Copyright 1999 ImageMagick Studio LLC, a non-profit organization           %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/geometry-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t G e o m e t r y                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetGeometry() parses a geometry specification and returns the width,
%  height, x, and y values.  It also returns flags that indicates which
%  of the four values (width, height, x, y) were located in the string, and
%  whether the x or y values are negative.  In addition, there are flags to
%  report any meta characters (%, !, <, or >).
%
%  The value must form a proper geometry style specification of WxH+X+Y
%  of integers only, and values can not be separated by comma, colon, or
%  slash characters.  See ParseGeometry() below.
%
%  Offsets may be prefixed by multiple signs to make offset string
%  substitutions easier to handle from shell scripts.
%  For example: "-10-10", "-+10-+10", or "+-10+-10" will generate negative
%  offsets, while "+10+10", "++10++10", or "--10--10" will generate positive
%  offsets.
%
%  The format of the GetGeometry method is:
%
%      MagickStatusType GetGeometry(const char *geometry,ssize_t *x,ssize_t *y,
%        size_t *width,size_t *height)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry.
%
%    o x,y:  The x and y offset as determined by the geometry specification.
%
%    o width,height:  The width and height as determined by the geometry
%      specification.
%
*/
static inline MagickBooleanType IsGeometryWidthTerminator(const int c,
  const char *q)
{
  return(((c == 215) || (*q == 'x') || (*q == 'X') || (*q == ':') ||
    (*q == '\0')) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsGeometryHeightSeparator(const int c,
  const char *p)
{
  return(((c == 215) || (*p == 'x') || (*p == 'X') || (*p == ':')) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsGeometryHeightOnly(
  const MagickStatusType flags,const size_t *width,const size_t *height)
{
  return((((flags & SeparatorValue) != 0) && ((flags & WidthValue) == 0) &&
    (height != (size_t *) NULL) && (width != (size_t *) NULL)) ? MagickTrue :
    MagickFalse);
}

static MagickStatusType SetPercentGeometrySize(MagickStatusType flags,
  size_t *width,size_t *height)
{
  if (((flags & SeparatorValue) == 0) && ((flags & HeightValue) == 0))
    {
      if ((height != (size_t *) NULL) && (width != (size_t *) NULL))
        *height=(*width);
      flags|=HeightValue;
    }
  if (IsGeometryHeightOnly(flags,width,height) != MagickFalse)
    *width=(*height);
  return(flags);
}

static void ParseGeometryXOffset(char **p,MagickStatusType *flags,ssize_t *x)
{
  char
    *q;

  /*
    Parse x value.
  */
  while ((**p == '+') || (**p == '-'))
  {
    if (**p == '-')
      *flags^=XNegative;  /* negate sign */
    (*p)++;
  }
  q=(*p);
  if (x != (ssize_t *) NULL)
    *x=CastDoubleToSsizeT(StringToDouble(*p,p));
  if (*p != q)
    {
      *flags|=XValue;
      if (((*flags & XNegative) != 0) && (x != (ssize_t *) NULL))
        *x=CastDoubleToSsizeT(-1.0**x);
    }
}

MagickExport MagickStatusType GetGeometry(const char *geometry,ssize_t *x,
  ssize_t *y,size_t *width,size_t *height)
{
  char
    *p,
    pedantic_geometry[MagickPathExtent],
    *q;

  double
    value;

  int
    c;

  MagickStatusType
    flags;

  /*
    Remove whitespace and meta characters from geometry specification.
  */
  flags=NoValue;
  if ((geometry == (char *) NULL) || (*geometry == '\0'))
    return(flags);
  if (strlen(geometry) >= (MagickPathExtent-1))
    return(flags);
  (void) CopyMagickString(pedantic_geometry,geometry,MagickPathExtent);
  for (p=pedantic_geometry; *p != '\0'; )
  {
    if (isspace((int) ((unsigned char) *p)) != 0)
      {
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        continue;
      }
    c=(int) *p;
    switch (c)
    {
      case '%':
      {
        flags|=PercentValue;
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case '!':
      {
        flags|=AspectValue;
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case '<':
      {
        flags|=LessValue;
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case '>':
      {
        flags|=GreaterValue;
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case '#':
      {
        flags|=MaximumValue;
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case '^':
      {
        flags|=MinimumValue;
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case '@':
      {
        flags|=AreaValue;
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case '(':
      case ')':
      {
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case 'x':
      case 'X':
      {
        flags|=SeparatorValue;
        p++;
        break;
      }
      case '-':
      case ',':
      case '+':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case 215:
      case 'e':
      case 'E':
      {
        p++;
        break;
      }
      case '.':
      {
        p++;
        flags|=DecimalValue;
        break;
      }
      case ':':
      {
        p++;
        flags|=AspectRatioValue;
        break;
      }
      default:
        return(flags);
    }
  }
  /*
    Parse width, height, x, and y.
  */
  p=pedantic_geometry;
  if (*p == '\0')
    return(flags);
  q=p;
  value=StringToDouble(p,&q);
  (void) value;
  if (LocaleNCompare(p,"0x",2) == 0)
    value=(double) strtol(p,&q,10);
  if ((*p != '+') && (*p != '-'))
    {
      c=(int) ((unsigned char) *q);
      if (IsGeometryWidthTerminator(c,q) != MagickFalse)
        {
          /*
            Parse width.
          */
          q=p;
          if (width != (size_t *) NULL)
            {
              if (LocaleNCompare(p,"0x",2) == 0)
                *width=(size_t) strtol(p,&p,10);
              else
                *width=CastDoubleToSizeT(StringToDouble(p,&p)+0.5);
            }
          if (p != q)
            flags|=WidthValue;
        }
    }
  if ((*p != '+') && (*p != '-'))
    {
      c=(int) ((unsigned char) *p);
      if (IsGeometryHeightSeparator(c,p) != MagickFalse)
        {
          p++;
          if ((*p != '+') && (*p != '-'))
            {
              /*
                Parse height.
              */
              q=p;
              if (height != (size_t *) NULL)
                *height=CastDoubleToSizeT(StringToDouble(p,&p)+0.5);
              if (p != q)
                flags|=HeightValue;
            }
        }
    }
  if ((*p == '+') || (*p == '-'))
    ParseGeometryXOffset(&p,&flags,x);
  if ((*p == '+') || (*p == '-'))
    {
      /*
        Parse y value.
      */
      while ((*p == '+') || (*p == '-'))
      {
        if (*p == '-')
          flags^=YNegative;  /* negate sign */
        p++;
      }
      q=p;
      if (y != (ssize_t *) NULL)
        *y=CastDoubleToSsizeT(StringToDouble(p,&p));
      if (p != q)
        {
          flags|=YValue;
          if (((flags & YNegative) != 0) && (y != (ssize_t *) NULL))
            *y=CastDoubleToSsizeT(-1.0**y);
        }
    }
  if ((flags & PercentValue) != 0)
    flags=SetPercentGeometrySize(flags,width,height);
#if 0
  /* Debugging Geometry */
  (void) fprintf(stderr,"GetGeometry...\n");
  (void) fprintf(stderr,"Input: %s\n",geometry);
  (void) fprintf(stderr,"Flags: %c %c %s %s\n",
    (flags & WidthValue) ? 'W' : ' ',(flags & HeightValue) ? 'H' : ' ',
    (flags & XValue) ? ((flags & XNegative) ? "-X" : "+X") : "  ",
    (flags & YValue) ? ((flags & YNegative) ? "-Y" : "+Y") : "  ");
  (void) fprintf(stderr,"Geometry: %ldx%ld%+ld%+ld\n",(long) *width,(long)
    *height,(long) *x,(long) *y);
#endif
  return(flags);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e G e o m e t r y                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseGeometry() parses a geometry specification and returns the sigma,
%  rho, xi, and psi values.  It also returns flags that indicates which
%  of the four values (sigma, rho, xi, psi) were located in the string, and
%  whether the xi or pi values are negative.
%
%  In addition, it reports if there are any of meta characters (%, !, <, >, @,
%  and ^) flags present. It does not report the location of the percentage
%  relative to the values.
%
%  Values may also be separated by commas, colons, or slashes, and offsets.
%  Chroma subsampling definitions have to be in the form of a:b:c.  Offsets may
%  be prefixed by multiple signs to make offset string substitutions easier to
%  handle from shell scripts.  For example: "-10-10", "-+10-+10", or "+-10+-10"
%  will generate negative offsets, while "+10+10", "++10++10", or "--10--10"
%  will generate positive offsets.
%
%  The format of the ParseGeometry method is:
%
%      MagickStatusType ParseGeometry(const char *geometry,
%        GeometryInfo *geometry_info)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry string (e.g. "100x100+10+10").
%
%    o geometry_info:  returns the parsed width/height/x/y in this structure.
%
*/
static inline MagickBooleanType IsGeometryOffsetPrefix(const char *p)
{
  return(((*p == '+') || (*p == '-') || (*p == ',') || (*p == '/') ||
    (*p == ':')) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsGeometryOffsetSeparator(const char *p)
{
  return(((*p == ',') || (*p == '/') || (*p == ':')) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsGeometryRhoTerminator(const int c,
  const char *q)
{
  return(((c == 215) || (*q == 'x') || (*q == 'X') || (*q == ':') ||
    (*q == ',') || (*q == '/') || (*q =='\0')) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsGeometrySigmaSeparator(const int c,
  const char *p)
{
  return(((c == 215) || (*p == 'x') || (*p == 'X') || (*p == ':') ||
    (*p == ',') || (*p == '/')) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsGeometrySigmaParsable(const int c,
  const char *p,const char *q)
{
  return((((c != 215) && (*q != 'x') && (*q != 'X') && (*q != ':')) ||
    ((*p != '+') && (*p != '-'))) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsNegativeHeightGeometry(
  const MagickStatusType flags)
{
  return((((flags & RhoValue) != 0) && ((flags & SigmaValue) == 0) &&
    ((flags & XiValue) != 0) && ((flags & XiNegative) != 0)) ? MagickTrue :
    MagickFalse);
}

static void NormalizeSamplingFactor(const MagickStatusType flags,
  GeometryInfo *geometry_info)
{
  /*
    Normalize sampling factor (e.g. 4:2:2 => 2x1).
  */
  if ((flags & SigmaValue) != 0)
    geometry_info->rho*=MagickSafeReciprocal(geometry_info->sigma);
  geometry_info->sigma=1.0;
  if (((flags & XiValue) != 0) && (geometry_info->xi == 0.0))
    geometry_info->sigma=2.0;
}

static MagickStatusType ShiftNegativeHeight(MagickStatusType flags,
  GeometryInfo *geometry_info)
{
  if ((flags & PsiValue) == 0)
    {
      /*
        Support negative height values (e.g. 30x-20).
      */
      geometry_info->sigma=geometry_info->xi;
      geometry_info->xi=0.0;
      flags|=SigmaValue;
      flags&=(unsigned int) (~XiValue);
    }
  else
    if ((flags & ChiValue) == 0)
      {
        /*
          Support negative height values (e.g. 30x-20+10).
        */
        geometry_info->sigma=geometry_info->xi;
        geometry_info->xi=geometry_info->psi;
        flags|=SigmaValue;
        flags|=XiValue;
        flags&=(unsigned int) (~PsiValue);
      }
    else
      {
        /*
          Support negative height values (e.g. 30x-20+10+10).
        */
        geometry_info->sigma=geometry_info->xi;
        geometry_info->xi=geometry_info->psi;
        geometry_info->psi=geometry_info->chi;
        flags|=SigmaValue;
        flags|=XiValue;
        flags|=PsiValue;
        flags&=(unsigned int) (~ChiValue);
      }
  return(flags);
}

static void SetPercentGeometryInfo(const MagickStatusType flags,
  GeometryInfo *geometry_info)
{
  if (((flags & SeparatorValue) == 0) && ((flags & SigmaValue) == 0))
    geometry_info->sigma=geometry_info->rho;
  if (((flags & SeparatorValue) != 0) && ((flags & RhoValue) == 0))
    geometry_info->rho=geometry_info->sigma;
}

MagickExport MagickStatusType ParseGeometry(const char *geometry,
  GeometryInfo *geometry_info)
{
  char
    *p,
    pedantic_geometry[MagickPathExtent],
    *q;

  double
    value;

  GeometryInfo
    coordinate;

  int
    c;

  MagickStatusType
    flags;

  /*
    Remove whitespaces meta characters from geometry specification.
  */
  assert(geometry_info != (GeometryInfo *) NULL);
  (void) memset(geometry_info,0,sizeof(*geometry_info));
  flags=NoValue;
  if ((geometry == (char *) NULL) || (*geometry == '\0'))
    return(flags);
  if (strlen(geometry) >= (MagickPathExtent-1))
    return(flags);
  c=MagickSscanf(geometry,"%lf%*[ ,]%lf%*[ ,]%lf%*[ ,]%lf",&coordinate.rho,
    &coordinate.sigma,&coordinate.xi,&coordinate.psi);
  if (c == 4)
    {
      /*
        Special case: coordinate (e.g. 0,0 255,255).
      */
      geometry_info->rho=coordinate.rho;
      geometry_info->sigma=coordinate.sigma;
      geometry_info->xi=coordinate.xi;
      geometry_info->psi=coordinate.psi;
      flags|=RhoValue | SigmaValue | XiValue | PsiValue;
      return(flags);
    }
  (void) CopyMagickString(pedantic_geometry,geometry,MagickPathExtent);
  for (p=pedantic_geometry; *p != '\0'; )
  {
    c=(int) ((unsigned char) *p);
    if (isspace((int) ((unsigned char) c)) != 0)
      {
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        continue;
      }
    switch (c)
    {
      case '%':
      {
        flags|=PercentValue;
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case '!':
      {
        flags|=AspectValue;
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case '<':
      {
        flags|=LessValue;
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case '>':
      {
        flags|=GreaterValue;
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case '#':
      {
        flags|=MaximumValue;
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case '^':
      {
        flags|=MinimumValue;
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case '@':
      {
        flags|=AreaValue;
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case '(':
      {
        if (*(p+1) == ')')
          return(flags);
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case ')':
      {
        (void) CopyMagickString(p,p+1,MagickPathExtent);
        break;
      }
      case 'x':
      case 'X':
      {
        flags|=SeparatorValue;
        p++;
        break;
      }
      case '-':
      case '+':
      case ',':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '/':
      case 215:
      case 'e':
      case 'E':
      {
        p++;
        break;
      }
      case '.':
      {
        p++;
        flags|=DecimalValue;
        break;
      }
      case ':':
      {
        p++;
        flags|=AspectRatioValue;
        break;
      }
      default:
        return(NoValue);
    }
  }
  /*
    Parse rho, sigma, xi, psi, and optionally chi.
  */
  p=pedantic_geometry;
  if (*p == '\0')
    return(flags);
  q=p;
  value=StringToDouble(p,&q);
  if (LocaleNCompare(p,"0x",2) == 0)
    (void) strtol(p,&q,10);
  c=(int) ((unsigned char) *q);
  if (IsGeometryRhoTerminator(c,q) != MagickFalse)
    {
      /*
        Parse rho.
      */
      q=p;
      if (LocaleNCompare(p,"0x",2) == 0)
        value=(double) strtol(p,&p,10);
      else
        value=StringToDouble(p,&p);
      if (p != q)
        {
          flags|=RhoValue;
          geometry_info->rho=value;
        }
    }
  q=p;
  c=(int) ((unsigned char) *p);
  if (IsGeometrySigmaSeparator(c,p) != MagickFalse)
    {
      /*
        Parse sigma.
      */
      p++;
      while (isspace((int) ((unsigned char) *p)) != 0)
        p++;
      c=(int) ((unsigned char) *q);
      if (IsGeometrySigmaParsable(c,p,q) != MagickFalse)
        {
          q=p;
          value=StringToDouble(p,&p);
          if (p != q)
            {
              flags|=SigmaValue;
              geometry_info->sigma=value;
            }
        }
    }
  while (isspace((int) ((unsigned char) *p)) != 0)
    p++;
  if (IsGeometryOffsetPrefix(p) != MagickFalse)
    {
      /*
        Parse xi value.
      */
      if (IsGeometryOffsetSeparator(p) != MagickFalse)
        p++;
      while ((*p == '+') || (*p == '-'))
      {
        if (*p == '-')
          flags^=XiNegative;  /* negate sign */
        p++;
      }
      q=p;
      value=StringToDouble(p,&p);
      if (p != q)
        {
          flags|=XiValue;
          if ((flags & XiNegative) != 0)
            value=(-value);
          geometry_info->xi=value;
        }
      while (isspace((int) ((unsigned char) *p)) != 0)
        p++;
      if (IsGeometryOffsetPrefix(p) != MagickFalse)
        {
          /*
            Parse psi value.
          */
          if (IsGeometryOffsetSeparator(p) != MagickFalse)
            p++;
          while ((*p == '+') || (*p == '-'))
          {
            if (*p == '-')
              flags^=PsiNegative;  /* negate sign */
            p++;
          }
          q=p;
          value=StringToDouble(p,&p);
          if (p != q)
            {
              flags|=PsiValue;
              if ((flags & PsiNegative) != 0)
                value=(-value);
              geometry_info->psi=value;
            }
      }
      while (isspace((int) ((unsigned char) *p)) != 0)
        p++;
      if (IsGeometryOffsetPrefix(p) != MagickFalse)
        {
          /*
            Parse chi value.
          */
          if (IsGeometryOffsetSeparator(p) != MagickFalse)
            p++;
          while ((*p == '+') || (*p == '-'))
          {
            if (*p == '-')
              flags^=ChiNegative;  /* negate sign */
            p++;
          }
          q=p;
          value=StringToDouble(p,&p);
          if (p != q)
            {
              flags|=ChiValue;
              if ((flags & ChiNegative) != 0)
                value=(-value);
              geometry_info->chi=value;
            }
        }
    }
  if (strchr(pedantic_geometry,':') != (char *) NULL)
    NormalizeSamplingFactor(flags,geometry_info);
  if (IsNegativeHeightGeometry(flags) != MagickFalse)
    flags=ShiftNegativeHeight(flags,geometry_info);
  if ((flags & PercentValue) != 0)
    SetPercentGeometryInfo(flags,geometry_info);
#if 0
  /* Debugging Geometry */
  (void) fprintf(stderr,"ParseGeometry...\n");
  (void) fprintf(stderr,"Flags: %c %c %s %s %s\n",
    (flags & RhoValue) ? 'W' : ' ',(flags & SigmaValue) ? 'H' : ' ',
    (flags & XiValue) ? ((flags & XiNegative) ? "-X" : "+X") : "  ",
    (flags & PsiValue) ? ((flags & PsiNegative) ? "-Y" : "+Y") : "  ",
    (flags & ChiValue) ? ((flags & ChiNegative) ? "-Z" : "+Z") : "  ");
  (void) fprintf(stderr,"Geometry: %lg,%lg,%lg,%lg,%lg\n",geometry_info->rho,
    geometry_info->sigma,geometry_info->xi,geometry_info->psi,
    geometry_info->chi);
#endif
  return(flags);
}
