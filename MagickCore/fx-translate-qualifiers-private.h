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

  MagickCore private methods of fx.c: the function qualifiers, the opening of function
  calls and their argument addressing, from IsChannelQualifiedAttr to
  AddArgumentAddressing; included by that file only.
*/
#ifndef MAGICKCORE_FX_TRANSLATE_QUALIFIERS_PRIVATE_H
#define MAGICKCORE_FX_TRANSLATE_QUALIFIERS_PRIVATE_H

static inline MagickBooleanType IsChannelQualifiedAttr(const ImgAttrE iaQual,
  const PixelChannel chQual)
{
  return((MagickBooleanType) (iaQual != aNull && chQual != NO_CHAN_QUAL));
}

static inline MagickBooleanType IsUnqualifiedAccess(
  const MagickBooleanType coordQual,const PixelChannel chQual,
  const ImgAttrE iaQual)
{
  return((MagickBooleanType) (!coordQual && chQual == NO_CHAN_QUAL &&
    iaQual == aNull));
}

static inline MagickBooleanType IsCompositeOnlyAccess(
  const MagickBooleanType coordQual,const PixelChannel chQual,
  const ImgAttrE iaQual)
{
  return((MagickBooleanType) (!coordQual && chQual == CompositePixelChannel &&
    iaQual == aNull));
}

static const FunctionE
  PixelAccessFunctions[] =
  {
    fP, fU, fU0, fUP,
    fV, fVP, fS, fSP
  };

static inline MagickBooleanType IsPixelAccessFunction(const FunctionE fe)
{
  size_t
    i;

  for (i=0; i < (sizeof(PixelAccessFunctions)/sizeof(*PixelAccessFunctions)); i++)
    if (PixelAccessFunctions[i] == fe)
      return(MagickTrue);
  return(MagickFalse);
}

static inline MagickBooleanType IsQualifiedCoordFunction(const ImgAttrE iaQual,
  const FunctionE fe)
{
  return((MagickBooleanType) (iaQual != aNull && (fe == fU || fe == fV ||
    fe == fS)));
}

static inline MagickBooleanType IsCoordFunction(const FunctionE fe)
{
  return((MagickBooleanType) (fe == fU || fe == fV || fe == fS));
}

static MagickBooleanType AddEpochSeconds (FxInfo * pfx, fxFltType seconds,
  ssize_t lenOptArt, OperatorE pushOp)
{
  (void) AddElement (pfx, seconds, oNull);
  pfx->pex += lenOptArt;
  if (!ExpectChar (pfx, ')')) return MagickFalse;
  if (!PopOprOpenParen (pfx, pushOp)) return MagickFalse;
  return MagickTrue;
}

static void AddFirstArgAddressing (FxInfo * pfx, FunctionE fe)
{
  if (fe==fWhile || fe==fIf) {
    (void) AddAddressingElement (pfx, rIfZeroGoto, NULL_ADDRESS); /* address will be ndx2+1 */
  } else if (fe==fDo) {
    (void) AddAddressingElement (pfx, rIfZeroGoto, NULL_ADDRESS); /* address will be ndx2+1 */
  } else if (fe==fFor) {
    pfx->Elements[pfx->usedElements-1].do_push = MagickFalse;
  }
}

static void AddSecondArgAddressing (FxInfo * pfx, FunctionE fe, int ndx0)
{
  if (fe==fWhile) {
    pfx->Elements[pfx->usedElements-1].do_push = MagickFalse;
    (void) AddAddressingElement (pfx, rGotoChk, ndx0);
  } else if (fe==fDo) {
    pfx->Elements[pfx->usedElements-1].do_push = MagickFalse;
    (void) AddAddressingElement (pfx, rGotoChk, ndx0 + 1);
  } else if (fe==fFor) {
    (void) AddAddressingElement (pfx, rIfZeroGoto, NULL_ADDRESS); /* address will be ndx3 */
    pfx->Elements[pfx->usedElements-1].do_push = MagickTrue; /* we may need return from for() */
    (void) AddAddressingElement (pfx, rZerStk, NULL_ADDRESS);
  } else if (fe==fIf) {
    (void) AddAddressingElement (pfx, rGoto, NULL_ADDRESS); /* address will be ndx3 */
  }
}

static int AddDefaultArgs (FxInfo * pfx, FunctionE fe, int FndArgs)
{
  while (FndArgs < Functions[fe-(int) FirstFunc].number_args) {
    (void) AddElement (pfx, (fxFltType) 0, oNull);
    FndArgs++;
  }
  return FndArgs;
}

static MagickBooleanType ReportTooManyArgs (FxInfo * pfx, FunctionE fe,
  const char * funStr, int FndArgs)
{
  if (fe==fChannel) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "For function", "'%s' expected up to %i arguments, found '%i' at '%s'",
      funStr, Functions[fe-(int) FirstFunc].number_args, FndArgs, SetShortExp(pfx));
  } else {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "For function", "'%s' expected %i arguments, found '%i' at '%s'",
      funStr, Functions[fe-(int) FirstFunc].number_args, FndArgs, SetShortExp(pfx));
  }
  return MagickFalse;
}

static MagickBooleanType CheckAttrChannelQualifier (FxInfo * pfx,
  ImgAttrE iaQual, PixelChannel chQual)
{
  if (ImgAttrs[iaQual-(int) FirstImgAttr].need_stats == MagickFalse) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Can't have image attribute ", "'%s' with channel qualifier '%s' at '%s'",
      ImgAttrs[iaQual-(int) FirstImgAttr].str,
      pfx->token, SetShortExp(pfx));
    return MagickFalse;
  } else {
    if (ChanIsVirtual (chQual)) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Can't have statistical image attribute ", "'%s' with virtual channel qualifier '%s' at '%s'",
        ImgAttrs[iaQual-(int) FirstImgAttr].str,
        pfx->token, SetShortExp(pfx));
      return MagickFalse;
    }
  }
  return MagickTrue;
}

typedef struct _CoordQualifierTargets
{
  FunctionE
    *fe;

  const char
    **funStr;

  PixelChannel
    *chQual;

  char
    *expChLimit;
} CoordQualifierTargets;

static MagickBooleanType ApplyCoordQualifier (FxInfo * pfx,
  const MagickBooleanType coordQual,const CoordQualifierTargets *targets)
{
  FunctionE
    *const fe = targets->fe;

  const char
    **const funStr = targets->funStr;

  PixelChannel
    *const chQual = targets->chQual;

  char
    *const expChLimit = targets->expChLimit;

  if (coordQual) {

    /* Remove last element, which should be fP */
    ElementT * pel = &pfx->Elements[pfx->usedElements-1];
    if (pel->operator_index != fP) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Bug: For function", "'%s' last element not 'p' at '%s'",
        *funStr, SetShortExp(pfx));
      return(MagickFalse);
    }
    *chQual = pel->channel_qual;
    *expChLimit = (pel->is_relative) ? ']' : '}';
    pfx->usedElements--;
    if (*fe == fU) *fe = fUP;
    else if (*fe == fV) *fe = fVP;
    else if (*fe == fS) *fe = fSP;
    *funStr = Functions[*fe-(int) FirstFunc].str;
  }
  return(MagickTrue);
}

typedef struct _QualifierCheck
{
  const char
    *funStr;

  MagickBooleanType
    coordQual;

  PixelChannel
    chQual;

  ImgAttrE
    iaQual;
} QualifierCheck;

static MagickBooleanType CheckFunctionQualifiers (const QualifierCheck *check,
  FxInfo * pfx)
{
  const char
    *const funStr = check->funStr;

  const MagickBooleanType
    coordQual = check->coordQual;

  const PixelChannel
    chQual = check->chQual;

  const ImgAttrE
    iaQual = check->iaQual;

    if (coordQual && iaQual != aNull) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "For function", "'%s', can't have qualifiers 'p' and image attribute '%s' at '%s'",
        funStr, pfx->token, SetShortExp(pfx));
      return MagickFalse;
    }
    if (IsUnqualifiedAccess(coordQual,chQual,iaQual) != MagickFalse) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "For function", "'%s', bad qualifier '%s' at '%s'",
        funStr, pfx->token, SetShortExp(pfx));
      return MagickFalse;
    }
    if (IsCompositeOnlyAccess(coordQual,chQual,iaQual) != MagickFalse) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "For function", "'%s', bad composite qualifier '%s' at '%s'",
        funStr, pfx->token, SetShortExp(pfx));
      return MagickFalse;
    }

    if (IsHslChannelQualifier(chQual) != MagickFalse) {
      pfx->NeedHsl = MagickTrue;

      if (iaQual >= FirstImgAttr && iaQual < aNull) {
        (void) ThrowMagickException (
          pfx->exception, GetMagickModule(), OptionError,
          "Can't have image attribute with HLS qualifier at", "'%s'",
          SetShortExp(pfx));
        return MagickFalse;
      }
    }
  return MagickTrue;
}

typedef struct _FunctionQualifiers
{
  MagickBooleanType
    *coordQual;

  FunctionE
    *fe;

  const char
    **funStr;

  PixelChannel
    *chQual;

  char
    *expChLimit;

  ImgAttrE
    *iaQual;
} FunctionQualifiers;

static MagickBooleanType QualifyCoordFunction (FxInfo * pfx,
  const FunctionQualifiers *qualifiers)
{
  MagickBooleanType
    *const coordQual = qualifiers->coordQual;

  FunctionE
    *const fe = qualifiers->fe;

  const char
    **const funStr = qualifiers->funStr;

  PixelChannel
    *const chQual = qualifiers->chQual;

  char
    *const expChLimit = qualifiers->expChLimit;

    if (IsCoordFunction(*fe) != MagickFalse) {

      CoordQualifierTargets targets;

      *coordQual =
        (MagickBooleanType) (GetCoordQualifier (pfx, (int) *fe) == 1);

      targets.fe=fe;
      targets.funStr=funStr;
      targets.chQual=chQual;
      targets.expChLimit=expChLimit;
      if (ApplyCoordQualifier (pfx, *coordQual, &targets) == MagickFalse)
        return MagickFalse;
    }
  return MagickTrue;
}

static MagickBooleanType QualifyFunction (FxInfo * pfx,
  const FunctionQualifiers *qualifiers)
{
  MagickBooleanType
    *const coordQual = qualifiers->coordQual;

  FunctionE
    *const fe = qualifiers->fe;

  const char
    **const funStr = qualifiers->funStr;

  PixelChannel
    *const chQual = qualifiers->chQual;

  ImgAttrE
    *const iaQual = qualifiers->iaQual;

  if (IsQualifier (pfx)) {

    QualifierCheck check;

    if (QualifyCoordFunction (pfx, qualifiers) == MagickFalse)
      return MagickFalse;

    if (IsUnqualifiedPixelFunction(*chQual,*fe) != MagickFalse)
    {
      *chQual = GetChannelQualifier (pfx, (int) *fe);
    }

    if (IsUnqualifiedCoordFunction(*chQual,*fe) != MagickFalse) {
      /* Note: we don't allow "p.mean" etc. */
      *iaQual = GetImgAttrQualifier (pfx, (int) *fe);
    }
    if (IsAttributeQualified(pfx,*chQual,*iaQual) != MagickFalse) {
      *chQual = GetChannelQualifier (pfx, (int) *fe);
    }
    check.funStr=*funStr;
    check.coordQual=*coordQual;
    check.chQual=*chQual;
    check.iaQual=*iaQual;
    if (CheckFunctionQualifiers (&check, pfx) == MagickFalse)
      return MagickFalse;
  }
  return MagickTrue;
}

typedef struct _FunctionOpening
{
  OperatorE
    *pushOp;

  const char
    **strLimit;

  char
    *chLimit,
    *expChLimit;

  int
    *nArgs;
} FunctionOpening;

static void OpenPixelFunction (FxInfo * pfx, const FunctionOpening *opening)
{
  OperatorE
    *const pushOp = opening->pushOp;

  const char
    **const strLimit = opening->strLimit;

  char
    *const chLimit = opening->chLimit,
    *const expChLimit = opening->expChLimit;

  int
    *const nArgs = opening->nArgs;

    char p = PeekChar (pfx);
    if (p=='{') {
      (void) ExpectChar (pfx, '{');
      *pushOp = oOpenBrace;
      *strLimit = ",}";
      *chLimit = '}';
      *expChLimit = '}';
    } else if (p=='[') {
      (void) ExpectChar (pfx, '[');
      *pushOp = oOpenBracket;
      *strLimit = ",]";
      *chLimit = ']';
      *expChLimit = ']';
    } else {
      *nArgs = 0;
      *chLimit = ']';
      *expChLimit = ']';
    }
}

static MagickBooleanType OpenFunction (FxInfo * pfx, FunctionE fe,
  const FunctionOpening *opening)
{
  OperatorE
    *const pushOp = opening->pushOp;

  const char
    **const strLimit = opening->strLimit;

  char
    *const chLimit = opening->chLimit,
    *const expChLimit = opening->expChLimit;

  int
    *const nArgs = opening->nArgs;

  if (fe == fP) {
    OpenPixelFunction (pfx, opening);
  } else if (fe == fU) {
    char p = PeekChar (pfx);
    if (p=='[') {
      (void) ExpectChar (pfx, '[');
      *pushOp = oOpenBracket;
      *strLimit = ",]";
      *chLimit = ']';
      *expChLimit = ']';
    } else {
      *nArgs = 0;
      *chLimit = ']';
      *expChLimit = ']';
    }
  } else if (fe == fV || fe == fS) {
      *nArgs = 0;
      *pushOp = oOpenBracket;
      *chLimit = ']';
      *expChLimit = ']';
  } else {
    if (!ExpectChar (pfx, '(')) return MagickFalse;
  }
  return MagickTrue;
}

static MagickBooleanType TryEpochFunction (FxInfo * pfx, FunctionE fe,
  OperatorE pushOp, MagickBooleanType * result)
{
  if (fe==fEpoch) {
    fxFltType
      val,
      seconds;
    ssize_t
      lenOptArt = GetProperty (pfx, &val, &seconds);
    if (seconds == SECONDS_ERR) {
      /* Exception may not have been raised. */
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Function 'epoch' expected date property", "at '%s'",
        SetShortExp(pfx));
      (*result)=MagickFalse;
      return MagickTrue;
    }
    if (lenOptArt < 0) {
      (*result)=MagickFalse;
      return MagickTrue;
    }
    if (lenOptArt > 0) {
      (*result)=(AddEpochSeconds (pfx, seconds, lenOptArt, pushOp));
      return MagickTrue;
    }
  }
  return MagickFalse;
}

static MagickBooleanType ExpectNoArgsClose (FxInfo * pfx,
  const char expChLimit, char * chLimit)
{
    *chLimit = expChLimit;
    if (!ExpectChar (pfx, ')')) return MagickFalse;
  return MagickTrue;
}

static void ApplyZeroUFunction (FxInfo * pfx, const ImgAttrE iaQual,
  FunctionE * fe)
{
    if ((*fe) == fU && iaQual == aNull) {
      ElementT * pel = &pfx->Elements[pfx->usedElements-1];
      if (pel->type == etConstant && pel->val == 0.0) {
        pfx->usedElements--;
        (*fe) = fU0;
      }
    }
}

typedef struct _PixelAccessQualifiers
{
  FunctionE
    fe;

  char
    expChLimit;

  PixelChannel
    chQual;

  ImgAttrE
    iaQual;
} PixelAccessQualifiers;

static void ApplyPixelAccessQualifiers (
  const PixelAccessQualifiers *qualifiers, FxInfo * pfx)
{
  const FunctionE
    fe = qualifiers->fe;

  const char
    expChLimit = qualifiers->expChLimit;

  const PixelChannel
    chQual = qualifiers->chQual;

  const ImgAttrE
    iaQual = qualifiers->iaQual;

    if (IsPixelAccessFunction(fe) != MagickFalse)
    {
      ElementT * pel = &pfx->Elements[pfx->usedElements-1];
      pel->is_relative = ((MagickBooleanType) (expChLimit == ']'));
      if (chQual >= 0) pel->channel_qual = chQual;
      if (IsQualifiedCoordFunction(iaQual,fe) != MagickFalse) {
        /* Note: we don't allow "p[2,3].mean" or "p.mean" etc. */
        pel->img_attr_qual = iaQual;
      }
    }
}

static void AddThirdArgAddressing (FxInfo * pfx, FunctionE fe, int ndx1)
{
  if (fe==fFor) {
    pfx->Elements[pfx->usedElements-1].do_push = MagickFalse;
    (void) AddAddressingElement (pfx, rGotoChk, ndx1);
  }
}

typedef struct _ArgumentAddressing
{
  FunctionE
    fe;

  const char
    *funStr;

  int
    FndArgs,
    ndx0;
} ArgumentAddressing;

typedef struct _ArgumentIndexes
{
  int
    *ndx1,
    *ndx2,
    *ndx3;
} ArgumentIndexes;

static MagickBooleanType AddArgumentAddressing (
  const ArgumentAddressing *addressing, FxInfo * pfx,
  const ArgumentIndexes *targets)
{
  const FunctionE
    fe = addressing->fe;

  const char
    *const funStr = addressing->funStr;

  const int
    FndArgs = addressing->FndArgs,
    ndx0 = addressing->ndx0;

  int
    *const ndx1 = targets->ndx1,
    *const ndx2 = targets->ndx2,
    *const ndx3 = targets->ndx3;

    switch (FndArgs) {
      case 1:
        if (*ndx1 != NULL_ADDRESS) {
          (void) ThrowMagickException (
            pfx->exception, GetMagickModule(), OptionError,
            "For function", "'%s' required argument is missing at '%s'",
            funStr, SetShortExp(pfx));
          return MagickFalse;
        }
        *ndx1 = pfx->usedElements;
        AddFirstArgAddressing (pfx, fe);
        break;
      case 2:
        if (*ndx2 != NULL_ADDRESS) {
          (void) ThrowMagickException (
            pfx->exception, GetMagickModule(), OptionError,
            "For function", "'%s' required argument is missing at '%s'",
            funStr, SetShortExp(pfx));
          return MagickFalse;
        }
        *ndx2 = pfx->usedElements;
        AddSecondArgAddressing (pfx, fe, ndx0);
        break;
      case 3:
        if (*ndx3 != NULL_ADDRESS) {
          (void) ThrowMagickException (
            pfx->exception, GetMagickModule(), OptionError,
            "For function", "'%s' required argument is missing at '%s'",
            funStr, SetShortExp(pfx));
          return MagickFalse;
        }
        AddThirdArgAddressing (pfx, fe, *ndx1);
        *ndx3 = pfx->usedElements;
        break;
      default:
        break;
    }
  return MagickTrue;
}

typedef struct _FunctionCompletion
{
  FunctionE
    fe;

  int
    ndx0,
    ndx1,
    ndx2,
    ndx3;

  char
    expChLimit;

  PixelChannel
    chQual;

  ImgAttrE
    iaQual;

  char
    *pExpStart;

  size_t
    lenExp;
} FunctionCompletion;

#endif
