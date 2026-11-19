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

  MagickCore private methods of fx.c: the operator stack, the XYWH, coordinate and
  channel qualifiers, image attributes, properties and colour operands, from PeekStr
  to IsAttributeQualified; included by that file only.
*/
#ifndef MAGICKCORE_FX_TRANSLATE_ATTRIBUTES_PRIVATE_H
#define MAGICKCORE_FX_TRANSLATE_ATTRIBUTES_PRIVATE_H

static inline MagickBooleanType PeekStr (FxInfo * pfx, const char * str)
{
  SkipSpaces (pfx);

  return ((MagickBooleanType) (LocaleNCompare (pfx->pex, str, strlen(str))==0));
}

static MagickBooleanType ExpectChar (FxInfo * pfx, char c)
{
  if (PeekChar (pfx) != c) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Expected char", "'%c' at '%s'", c, SetShortExp (pfx));
    return MagickFalse;
  }
  pfx->pex++;
  return MagickTrue;
}

static inline MagickBooleanType IsUnqualifiableAttr(const ImgAttrE *pop)
{
  return((MagickBooleanType) (*pop != aPage && *pop != aPrintsize &&
    *pop != aRes));
}

static int MatchXYWHToken (FxInfo * pfx, int ret)
{
  if (LocaleCompare ("x", pfx->token)==0) ret=1;
  else if (LocaleCompare ("y", pfx->token)==0) ret=2;
  else if (LocaleCompare ("width", pfx->token)==0) ret=3;
  else if (LocaleCompare ("height", pfx->token)==0) ret=4;
  return ret;
}

static void ApplyXYWHToken (FxInfo * pfx, ImgAttrE * pop, int ret)
{
  if (*pop == aPage) (*pop) = (ImgAttrE) ((int) *pop + ret);
  else {
    if (ret > 2) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Invalid 'width' or 'height' token=", "'%s' at '%s'",
        pfx->token, SetShortExp(pfx));
    } else {
      (*pop) = (ImgAttrE) ((int) *pop + ret);
    }
  }
}

static void ReportBadXYWHToken (FxInfo * pfx, int ret)
{
  if (!ret)
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Invalid 'x' or 'y' or 'width' or 'height' token=", "'%s' at '%s'",
      pfx->token, SetShortExp(pfx));
}

static int MaybeXYWH (FxInfo * pfx, ImgAttrE * pop)
/* If ".x" or ".y" or ".width" or ".height" increments *pop and returns 1 to 4 .
   Otherwise returns 0.
*/
{
  int ret=0;

  if (IsUnqualifiableAttr(pop) != MagickFalse) return 0;

  if (PeekChar (pfx) != '.') return 0;

  if (!ExpectChar (pfx, '.')) return 0;

  (void) GetToken (pfx);
  ret = MatchXYWHToken (pfx, ret);

  ReportBadXYWHToken (pfx, ret);

  ApplyXYWHToken (pfx, pop, ret);
  pfx->pex+=pfx->lenToken;

  return ret;
}

static MagickBooleanType ExtendOperatorStack (FxInfo * pfx)
{
  pfx->numOprStack = (int) ceil (pfx->numOprStack * (1 + TableExtend));
  pfx->OperatorStack = (OperatorE*) ResizeMagickMemory (pfx->OperatorStack, (size_t) pfx->numOprStack * sizeof(OperatorE));
  if (!pfx->OperatorStack) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), ResourceLimitFatalError,
      "OprStack", "%i",
      pfx->numOprStack);
    return MagickFalse;
  }
  return MagickTrue;
}

static void TrackOperatorStackDepth (FxInfo * pfx)
{
  if (pfx->maxUsedOprStack < pfx->usedOprStack)
    pfx->maxUsedOprStack = pfx->usedOprStack;
}

static MagickBooleanType PushOperatorStack (FxInfo * pfx, int op)
{
  if (++pfx->usedOprStack >= pfx->numOprStack) {
    if (!ExtendOperatorStack (pfx))
      return MagickFalse;
  }
  pfx->OperatorStack[pfx->usedOprStack-1] = (OperatorE) op;

  TrackOperatorStackDepth (pfx);
  return MagickTrue;
}

static OperatorE GetLeadingOp (FxInfo * pfx)
{
  OperatorE op = oNull;

  if      (*pfx->pex == '-') op = oUnaryMinus;
  else if (*pfx->pex == '+') op = oUnaryPlus;
  else if (*pfx->pex == '~') op = oBitNot;
  else if (*pfx->pex == '!') op = oLogNot;
  else if (*pfx->pex == '(') op = oOpenParen;

  return op;
}

static inline MagickBooleanType OprIsUnaryPrefix (OperatorE op)
{
  return ((MagickBooleanType) (op == oUnaryMinus || op == oUnaryPlus ||
    op == oBitNot || op == oLogNot));
}

static MagickBooleanType TopOprIsUnaryPrefix (FxInfo * pfx)
{
  if (!pfx->usedOprStack) return MagickFalse;

  return OprIsUnaryPrefix (pfx->OperatorStack[pfx->usedOprStack-1]);
}

static MagickBooleanType PopOprOpenParen (FxInfo * pfx, OperatorE op)
{

  if (!pfx->usedOprStack) return MagickFalse;

  if (pfx->OperatorStack[pfx->usedOprStack-1] != op) return MagickFalse;

  pfx->usedOprStack--;

  return MagickTrue;
}

static inline MagickBooleanType IsNotCoordFunction(const int op)
{
  return((MagickBooleanType) (op != fU && op != fV && op != fS));
}

static int GetCoordQualifier (FxInfo * pfx, int op)
/* Returns -1 if invalid CoordQualifier, +1 if valid and appropriate.
*/
{
  if (IsNotCoordFunction(op) != MagickFalse) return -1;

  (void) GetToken (pfx);

  if (pfx->lenToken != 1) {
    return -1;
  }
  if (*pfx->token != 'p' && *pfx->token != 'P') return -1;
  if (!GetFunction (pfx, fP)) return -1;

  return 1;
}

static const int
  ChannelQualifiableOperators[] =
  {
    fU, fV, fP,
    fUP, fVP,
    fS
  };

static inline MagickBooleanType IsChannelQualifiableOperator(const int op)
{
  size_t
    i;

  for (i=0; i < (sizeof(ChannelQualifiableOperators)/sizeof(*ChannelQualifiableOperators)); i++)
    if (ChannelQualifiableOperators[i] == op)
      return(MagickTrue);
  return((MagickBooleanType) ((op >= (int) FirstImgAttr && op <= aNull)
    ));
}

static inline MagickBooleanType IsAttributeWithVirtualChannel(const int op,
  const ChannelT *pch)
{
  return((MagickBooleanType) (op >= (int) FirstImgAttr &&
    op <= (int) ((OperatorE)aNull) && ChanIsVirtual (pch->pixel_channel) ));
}

static PixelChannel GetChannelQualifier (FxInfo * pfx, int op)
{
  const ChannelT * pch;
  if (IsChannelQualifiableOperator(op) == MagickFalse)
    return NO_CHAN_QUAL;
  pch = &Channels[0];
  (void) GetToken (pfx);

  while (*pch->str) {
    if (LocaleCompare (pch->str, pfx->token)==0) {

      if (IsAttributeWithVirtualChannel(op,pch) != MagickFalse)
      {
        (void) ThrowMagickException (
          pfx->exception, GetMagickModule(), OptionError,
          "Can't have image attribute with channel qualifier at", "'%s' at '%s'",
          pfx->token, SetShortExp(pfx));
        return NO_CHAN_QUAL;
      }

      pfx->pex += pfx->lenToken;
      return pch->pixel_channel;
    }
    pch++;
  }
  return NO_CHAN_QUAL;
}

static inline MagickBooleanType IsQualifierRequiredAttr(const ImgAttrE ia)
{
  return((MagickBooleanType) (ia == aPage || ia == aPrintsize || ia == aRes));
}

static void NoteAttrNeedsStats (FxInfo * pfx, ImgAttrE ia)
{
      if (ImgAttrs[ia-(int) FirstImgAttr].need_stats != MagickFalse) pfx->NeedStats = MagickTrue;
}

static ImgAttrE GetImgAttrToken (FxInfo * pfx)
{
  ImgAttrE ia = aNull;
  const char * iaStr;
  for (ia = FirstImgAttr; ia < aNull; ia=(ImgAttrE) (ia+1)) {
    iaStr = ImgAttrs[ia-(int) FirstImgAttr].str;
    if (LocaleCompare (iaStr, pfx->token)==0) {
      pfx->pex += strlen(pfx->token);
      NoteAttrNeedsStats (pfx, ia);
      MaybeXYWH (pfx, &ia);
      break;
    }
  }

  if (IsQualifierRequiredAttr(ia) != MagickFalse) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Attribute", "'%s' needs qualifier at '%s'",
      iaStr, SetShortExp(pfx));
  }

  return ia;
}

static inline MagickBooleanType IsImageReferenceOperator(const int op)
{
  return((MagickBooleanType) (op == (OperatorE)fU || op == (OperatorE)fV ||
    op == (OperatorE)fP || op == (OperatorE)fS));
}

static ImgAttrE GetImgAttrQualifier (FxInfo * pfx, int op)
{
  ImgAttrE ia = aNull;
  if (IsImageReferenceOperator(op) != MagickFalse) {
    (void) GetToken (pfx);
    if (pfx->lenToken == 0) {
      return aNull;
    }
    ia = GetImgAttrToken (pfx);
  }
  return ia;
}

static MagickBooleanType IsQualifier (FxInfo * pfx)
{
  if (PeekChar (pfx) == '.') {
    pfx->pex++;
    return MagickTrue;
  }
  return MagickFalse;
}

static MagickBooleanType ParseISO860(const char* text,struct tm* tp)
{
  int
    year,
    month,
    day,
    hour,
    min,
    sec;

  memset(tp,0,sizeof(struct tm));
  if (MagickSscanf(text,"%d-%d-%dT%d:%d:%d",&year,&month,&day,&hour,&min,&sec) != 6)
    return(MagickFalse);
  tp->tm_year=year-1900;
  tp->tm_mon=month-1;
  tp->tm_mday=day;
  tp->tm_hour=hour;
  tp->tm_min=min;
  tp->tm_sec=sec;
  tp->tm_isdst=-1;
  return(MagickTrue);
}

static MagickBooleanType GetPropertySeconds (FxInfo * pfx, char ** text,
  fxFltType *val, fxFltType *seconds)
{
  struct tm tp;
  if (ParseISO860(*text,&tp) == MagickFalse) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Function 'epoch' expected date property, found ", "'%s' at '%s'",
      *text, SetShortExp(pfx));
    *text = DestroyString(*text);
    *seconds = SECONDS_ERR;
    return(MagickFalse);
  }
  *seconds = (fxFltType)mktime (&tp);
  *val = *seconds;
  return(MagickTrue);
}

static MagickBooleanType CountPropertyBracket (const char * p, int * level)
{
  if (*p == '[') (*level)++;
  else if (*p == ']') {
    if (*level == 0) return(MagickFalse);
    (*level)--;
  }
  return(MagickTrue);
}

static void ScanPropertyBrackets (int * level, char ** p)
{
  while (**p) {

    if (CountPropertyBracket (*p, &(*level)) == MagickFalse)
      break;
    (*p)++;
  }
}

static MagickBooleanType GetPropertyNumber (FxInfo * pfx,
  const char * sProperty, char ** text, fxFltType *val)
{
  char * tailptr;

  *val = strtold (*text, &tailptr);
  if (*text == tailptr) {
    *text = DestroyString(*text);
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Property", "'%s' text '%s' is not a number at '%s'",
      sProperty, *text, SetShortExp(pfx));
    *text = DestroyString(*text);
    return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType InterpretPropertyValue (FxInfo * pfx,
  const char * sProperty, fxFltType *val, fxFltType *seconds)
{
  {
    char * text;
    text = InterpretImageProperties (pfx->image->image_info, pfx->image,
       sProperty, pfx->exception);
    if (!text || !*text) {
      text = DestroyString(text);
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Unknown property", "'%s' at '%s'",
        sProperty, SetShortExp(pfx));
      return(MagickFalse);
    }

    if (seconds != NULL) {
      if (GetPropertySeconds (pfx, &text, val, seconds) == MagickFalse)
        return(MagickFalse);
    } else if (GetPropertyNumber (pfx, sProperty, &text, val) == MagickFalse)
      return(MagickFalse);
    text = DestroyString(text);
  }
  return(MagickTrue);
}

static ssize_t GetProperty (FxInfo * pfx, fxFltType *val, fxFltType *seconds)
/* Returns number of characters to swallow.
   Returns "-1" means invalid input.
   Returns "0" means no relevant input (don't swallow, but not an error).
   If *seconds is not null, sets that from assumed date-time, or SECONDS_ERR if error.
*/
{
  int level = 0;
  size_t len;
  char sProperty [MagickPathExtent];
  char * p;

  if (seconds != NULL) *seconds = SECONDS_ERR;

  if (!PeekStr (pfx, "%["))
    return 0;
  p = pfx->pex + 2;

  ScanPropertyBrackets (&level, &p);
  if (!*p || level != 0) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "After '%[' expected ']' at", "'%s'",
      SetShortExp(pfx));
    return -1;
  }

  len = (size_t) (p - pfx->pex + 1);
  if (len > MaxTokenLen) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Too much text between '%[' and ']' at", "'%s'",
      SetShortExp(pfx));
    return -1;
  }

  (void) CopyMagickString (sProperty, pfx->pex, len+1);
  sProperty[len] = '\0';
  if (InterpretPropertyValue (pfx, sProperty, val, seconds) == MagickFalse)
    return -1;
  return ((ssize_t) len);
}

static inline MagickBooleanType IsColourSpaceNamed(const ssize_t type,
  const MagickBooleanType IsIcc,const MagickBooleanType IsDev)
{
  return((MagickBooleanType) (type >= 0 || IsIcc || IsDev));
}

static ssize_t ReportColourSpaceWithoutArgs (FxInfo * pfx,
  ExceptionInfo * dummy_exception)
{
  (void) ThrowMagickException (
    pfx->exception, GetMagickModule(), OptionError,
    "colorspace but not a valid color with '(...)' at", "'%s'",
    SetShortExp(pfx));
  dummy_exception = DestroyExceptionInfo (dummy_exception);
  return -1;
}

typedef struct _ConstantColourContext
{
  FxInfo
    *pfx;

  ExceptionInfo
    *dummy_exception;

  PixelInfo
    *colour;

  fxFltType
    *v0,
    *v1,
    *v2;
} ConstantColourContext;

static MagickBooleanType TryColourFunction (
  const ConstantColourContext * context, ssize_t * result)
{
  FxInfo
    *const pfx = context->pfx;

  ExceptionInfo
    *dummy_exception = context->dummy_exception;

  PixelInfo
    *const colour = context->colour;

  fxFltType
    *const v0 = context->v0,
    *const v1 = context->v1,
    *const v2 = context->v2;

      char * q = pfx->pex + pfx->lenToken;
      while (isspace((int) ((unsigned char) *q))) q++;
      if (*q == '(') {
        size_t lenfun;
        char sFunc[MagickPathExtent];
        while (*q && *q != ')') q++;
        if (!*q) {
          (void) ThrowMagickException (
            pfx->exception, GetMagickModule(), OptionError,
            "constant color missing ')'", "at '%s'",
            SetShortExp(pfx));
          dummy_exception = DestroyExceptionInfo (dummy_exception);
          (*result)=-1;
          return(MagickTrue);
        }
        lenfun = (size_t) (q - pfx->pex + 1);
        if (lenfun > MaxTokenLen) {
          (void) ThrowMagickException (
            pfx->exception, GetMagickModule(), OptionError,
            "lenfun too long", "'%lu' at '%s'",
            (unsigned long) lenfun, SetShortExp(pfx));
          dummy_exception = DestroyExceptionInfo (dummy_exception);
          (*result)=-1;
          return(MagickTrue);
        }
        (void) CopyMagickString (sFunc, pfx->pex, lenfun+1);
        if (QueryColorCompliance (sFunc, AllCompliance, colour, dummy_exception)) {
          *v0 = QuantumScale*(*colour).red;
          *v1 = QuantumScale*(*colour).green;
          *v2 = QuantumScale*(*colour).blue;
          dummy_exception = DestroyExceptionInfo (dummy_exception);
          (*result)=(ssize_t)lenfun;
          return(MagickTrue);
        }
      } else {
        (*result)=(ReportColourSpaceWithoutArgs (pfx, dummy_exception));
        return(MagickTrue);
      }
  return(MagickFalse);
}

typedef struct _ColourSpaceNames
{
  const char
    *ColSp;

  MagickBooleanType
    IsGray,
    IsIcc,
    IsDev;
} ColourSpaceNames;

static MagickBooleanType TryColourSpaceColour (const ColourSpaceNames * names,
  const ConstantColourContext * context, ssize_t * result)
{
  FxInfo
    *const pfx = context->pfx;

  ExceptionInfo
    *dummy_exception = context->dummy_exception;

  PixelInfo
    *const colour = context->colour;

  const char
    *const ColSp = names->ColSp;

  const MagickBooleanType
    IsGray = names->IsGray,
    IsIcc = names->IsIcc,
    IsDev = names->IsDev;

  ssize_t
    returned;

  if (!QueryColorCompliance (pfx->token, AllCompliance, colour, dummy_exception) || IsGray) {
    ssize_t type = ParseCommandOption (MagickColorspaceOptions, MagickFalse, ColSp);
    if (IsColourSpaceNamed(type,IsIcc,IsDev) != MagickFalse) {
      if (TryColourFunction (context, &returned) != MagickFalse) {
        (*result)=returned;
        return(MagickTrue);
      }
    }
    if (!IsGray) {
      dummy_exception = DestroyExceptionInfo (dummy_exception);
      (*result)=0;
      return(MagickTrue);
    }
  }
  return(MagickFalse);
}

static void CopyColourSpaceName (FxInfo * pfx, char * ColSp)
{
  char
    *p;

  (void) CopyMagickString (ColSp, pfx->token, MaxTokenLen);
  p = ColSp + pfx->lenToken - 1;
  if (*p == 'a' || *p == 'A') *p = '\0';
}

static inline ssize_t GetConstantColour (FxInfo * pfx, fxFltType *v0, fxFltType *v1, fxFltType *v2)
/* Finds named colour such as "blue" and colorspace function such as "lab(10,20,30)".
   Returns number of characters to swallow.
   Return -1 means apparently a constant colour, but with an error.
   Return 0 means not a constant colour, but not an error.
*/
{
  PixelInfo
    colour;

  ExceptionInfo
    *dummy_exception = AcquireExceptionInfo ();

  MagickBooleanType
    IsGray,
    IsIcc,
    IsDev;

  ConstantColourContext
    context;

  ColourSpaceNames
    names;

  ssize_t
    returned;

  char ColSp[MagickPathExtent];
  CopyColourSpaceName (pfx, ColSp);

  (void) GetPixelInfo (pfx->image, &colour);

  /* "gray" is both a colorspace and a named colour. */

  IsGray = (MagickBooleanType) (LocaleCompare (ColSp, "gray") == 0);
  IsIcc = (MagickBooleanType) (LocaleCompare (ColSp, "icc-color") == 0);
  IsDev = (MagickBooleanType) (LocaleNCompare (ColSp, "device-", 7) == 0);

  /* QueryColorCompliance will raise a warning if it isn't a colour, so we discard any exceptions.
  */
  context.pfx=pfx;
  context.dummy_exception=dummy_exception;
  context.colour=(&colour);
  context.v0=v0;
  context.v1=v1;
  context.v2=v2;
  names.ColSp=ColSp;
  names.IsGray=IsGray;
  names.IsIcc=IsIcc;
  names.IsDev=IsDev;
  if (TryColourSpaceColour (&names, &context, &returned) != MagickFalse)
    return(returned);

  *v0 = QuantumScale*colour.red;
  *v1 = QuantumScale*colour.green;
  *v2 = QuantumScale*colour.blue;

  dummy_exception = DestroyExceptionInfo (dummy_exception);
  return (ssize_t)strlen (pfx->token);
}

static char *SkipHexDigits (char * p)
{
  while (isxdigit ((int)*p)) p++;
  return(p);
}

static void SetColourValues (const PixelInfo * colour, fxFltType *v0,
  fxFltType *v1, fxFltType *v2)
{
  *v0 = QuantumScale*colour->red;
  *v1 = QuantumScale*colour->green;
  *v2 = QuantumScale*colour->blue;
}

static inline ssize_t GetHexColour (FxInfo * pfx, fxFltType *v0, fxFltType *v1, fxFltType *v2)
/* Returns number of characters to swallow.
   Negative return means it starts with '#', but invalid hex number.
*/
{
  char * p;
  size_t len;
  PixelInfo colour;

  if (*pfx->pex != '#') return 0;

  /* find end of hex digits. */
  p = pfx->pex + 1;
  p = SkipHexDigits (p);
  if (isalpha ((int)*p)) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Bad hex number at", "'%s'",
      SetShortExp(pfx));
    return -1;
  }

  len = (size_t) (p - pfx->pex);
  if (len < 1) return 0;
  if (len >= MaxTokenLen) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Hex colour too long at", "'%s'",
      SetShortExp(pfx));
    return -1;
  }
  (void) CopyMagickString (pfx->token, pfx->pex, len+1);

  (void) GetPixelInfo (pfx->image, &colour);

  if (!QueryColorCompliance (pfx->token, AllCompliance, &colour, pfx->exception)) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "QueryColorCompliance rejected", "'%s' at '%s'",
      pfx->token, SetShortExp(pfx));
    return -1;
  }

  SetColourValues (&colour, v0, v1, v2);

  return (ssize_t) len;
}

static inline MagickBooleanType IsPadFunction(const FunctionE fe)
{
  return((MagickBooleanType) (fe == fP || fe == fS|| fe == fIf));
}

static inline MagickBooleanType IsUnexpectedLimit(const char chLimit,
  const char expChLimit)
{
  return((MagickBooleanType) (chLimit && chLimit != expChLimit &&
    chLimit != ',' ));
}

static inline MagickBooleanType IsDefaultArgsFunction(const FunctionE fe)
{
  return((MagickBooleanType) (fe == fP || fe == fS || fe == fU ||
    fe == fChannel));
}

static inline MagickBooleanType IsNoArgsFunction(const FunctionE fe,
  const int FndArgs)
{
  return((MagickBooleanType) (fe != fS && fe != fV && FndArgs == 0 &&
    Functions[fe-(int) FirstFunc].number_args == 0));
}

static const FunctionE
  UnqualifiedPixelFunctions[] =
  {
    fP, fS, fSP, fU, fUP, fV,
    fVP
  };

static inline MagickBooleanType IsUnqualifiedPixelFunction(
  const PixelChannel chQual,const FunctionE fe)
{
  size_t
    i;

  if (chQual == NO_CHAN_QUAL)
    for (i=0; i < (sizeof(UnqualifiedPixelFunctions)/sizeof(*UnqualifiedPixelFunctions)); i++)
      if (UnqualifiedPixelFunctions[i] == fe)
        return(MagickTrue);
  return(MagickFalse);
}

static inline MagickBooleanType IsUnqualifiedCoordFunction(
  const PixelChannel chQual,const FunctionE fe)
{
  return((MagickBooleanType) (chQual == NO_CHAN_QUAL && (fe == fU || fe == fV ||
    fe == fS)));
}

static inline MagickBooleanType IsAttributeQualified(FxInfo *pfx,
  const PixelChannel chQual,const ImgAttrE iaQual)
{
  return((MagickBooleanType) (IsQualifier (pfx) && chQual == NO_CHAN_QUAL &&
    iaQual != aNull));
}

#endif
