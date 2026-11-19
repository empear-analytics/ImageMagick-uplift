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

  MagickCore private methods of fx.c: the expression translator and its runtime helpers,
  from MaxTokenLen to DumpUserSymbolValues; included by that file only.
*/
#ifndef MAGICKCORE_FX_TRANSLATE_PRIVATE_H
#define MAGICKCORE_FX_TRANSLATE_PRIVATE_H

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

static MagickBooleanType CompleteFunction (FxInfo * pfx,
  const FunctionCompletion *completion)
{
  FunctionE
    fe = completion->fe;

  const int
    ndx0 = completion->ndx0,
    ndx1 = completion->ndx1,
    ndx2 = completion->ndx2,
    ndx3 = completion->ndx3;

  const char
    expChLimit = completion->expChLimit;

  const PixelChannel
    chQual = completion->chQual;

  const ImgAttrE
    iaQual = completion->iaQual;

  char
    *const pExpStart = completion->pExpStart;

  const size_t
    lenExp = completion->lenExp;

  if (fe==fWhile) {
    pfx->Elements[ndx1].element_index = ndx2+1;
  } else if (fe==fDo) {
    pfx->Elements[ndx0].element_index = ndx1+1;
    pfx->Elements[ndx1].element_index = ndx2+1;
  } else if (fe==fFor) {
    pfx->Elements[ndx2].element_index = ndx3;
  } else if (fe==fIf) {
    pfx->Elements[ndx1].element_index = ndx2 + 1;
    pfx->Elements[ndx2].element_index = ndx3;
  } else {
    PixelAccessQualifiers access;

    ApplyZeroUFunction (pfx, iaQual, &fe);
    (void) AddElement (pfx, (fxFltType) 0, (int) fe);
    access.fe=fe;
    access.expChLimit=expChLimit;
    access.chQual=chQual;
    access.iaQual=iaQual;
    ApplyPixelAccessQualifiers (&access, pfx);
  }

  if (pExpStart && lenExp) {
    ElementT * pel = &pfx->Elements[pfx->usedElements-1];
    pel->exp_start = pExpStart;
    pel->exp_len = lenExp;
  }

  if (fe == fDebug)
    pfx->ContainsDebug = MagickTrue;

  return MagickTrue;
}

typedef struct _FunctionClosing
{
  FunctionE
    fe;

  const char
    *funStr;

  int
    ndx0,
    ndx1,
    ndx2,
    ndx3;

  char
    expChLimit;

  OperatorE
    pushOp;

  MagickBooleanType
    coordQual;

  PixelChannel
    chQual;

  ImgAttrE
    iaQual;

  char
    *pExpStart;

  size_t
    lenExp;
} FunctionClosing;

static MagickBooleanType CloseFunction (FxInfo * pfx, const char chLimit,
  const FunctionClosing *closing)
{
  FunctionE
    fe = closing->fe;

  const char
    *funStr = closing->funStr;

  const int
    ndx0 = closing->ndx0,
    ndx1 = closing->ndx1,
    ndx2 = closing->ndx2,
    ndx3 = closing->ndx3;

  char
    expChLimit = closing->expChLimit;

  const OperatorE
    pushOp = closing->pushOp;

  MagickBooleanType
    coordQual = closing->coordQual;

  PixelChannel
    chQual = closing->chQual;

  ImgAttrE
    iaQual = closing->iaQual;

  char
    *const pExpStart = closing->pExpStart;

  const size_t
    lenExp = closing->lenExp;

  FunctionCompletion completion;

  const FunctionQualifiers qualifiers = { &coordQual, &fe, &funStr, &chQual,
    &expChLimit, &iaQual };

  if (chLimit != expChLimit) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "For function", "'%s', arguments don't end with '%c' at '%s'",
      funStr, expChLimit, SetShortExp(pfx));
    return MagickFalse;
  }
  if (!PopOprOpenParen (pfx, pushOp)) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Bug: For function", "'%s' tos not '%s' at '%s'",
      funStr, Operators[pushOp].str, SetShortExp(pfx));
    return MagickFalse;
  }

  if (QualifyFunction (pfx, &qualifiers) == MagickFalse)
    return MagickFalse;

  if ((IsChannelQualifiedAttr(iaQual,chQual) != MagickFalse) &&
      (!CheckAttrChannelQualifier (pfx, iaQual, chQual)))
    return MagickFalse;

  completion.fe=fe;
  completion.ndx0=ndx0;
  completion.ndx1=ndx1;
  completion.ndx2=ndx2;
  completion.ndx3=ndx3;
  completion.expChLimit=expChLimit;
  completion.chQual=chQual;
  completion.iaQual=iaQual;
  completion.pExpStart=pExpStart;
  completion.lenExp=lenExp;
  return(CompleteFunction (pfx, &completion));
}

static MagickBooleanType FinishFunctionArguments (FxInfo * pfx, int FndArgs,
  char chLimit, const FunctionClosing *closing)
{
  const FunctionE
    fe = closing->fe;

  const char
    *const funStr = closing->funStr;

  const char
    expChLimit = closing->expChLimit;

  if (IsUnexpectedLimit(chLimit,expChLimit) != MagickFalse) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "For function", "'%s' expected '%c', found '%c' at '%s'",
      funStr, expChLimit, chLimit ? chLimit : ' ', SetShortExp(pfx));
    return MagickFalse;
  }

  if (IsDefaultArgsFunction(fe) != MagickFalse) {
    FndArgs = AddDefaultArgs (pfx, fe, FndArgs);
  }

  if (FndArgs > Functions[fe-(int) FirstFunc].number_args)
  {
    return (ReportTooManyArgs (pfx, fe, funStr, FndArgs));
  }
  if (FndArgs < Functions[fe-(int) FirstFunc].number_args) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "For function", "'%s' expected %i arguments, found too few (%i) at '%s'",
      funStr, Functions[fe-(int) FirstFunc].number_args, FndArgs, SetShortExp(pfx));
    return MagickFalse;
  }
  /* This is for "rand()" and similar. */
  if ((IsNoArgsFunction(fe,FndArgs) != MagickFalse) &&
      (ExpectNoArgsClose (pfx, expChLimit, &chLimit) == MagickFalse))
    return MagickFalse;

  return(CloseFunction (pfx, chLimit, closing));
}

typedef struct _ArgumentCount
{
  FunctionE
    fe;

  const char
    *funStr,
    *strLimit;

  char
    chLimit;

  int
    FndOne,
    ndx0;

  ArgumentIndexes
    indexes;
} ArgumentCount;

typedef struct _ArgumentCountTargets
{
  int
    *FndArgs,
    *nArgs;
} ArgumentCountTargets;

static MagickBooleanType CountFunctionArgument (FxInfo * pfx,
  const ArgumentCount *count, const ArgumentCountTargets *targets)
{
  const FunctionE
    fe = count->fe;

  const char
    *const funStr = count->funStr,
    *const strLimit = count->strLimit;

  const char
    chLimit = count->chLimit;

  const int
    FndOne = count->FndOne,
    ndx0 = count->ndx0;

  const ArgumentIndexes
    indexes = count->indexes;

  int
    *const FndArgs = targets->FndArgs,
    *const nArgs = targets->nArgs;

  ArgumentAddressing addressing;

    if (strchr (strLimit, chLimit)==NULL) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "For function", "'%s' expected one of '%s' after expression but found '%c' at '%s'",
        funStr, strLimit, chLimit ? chLimit : ' ', SetShortExp(pfx));
      return MagickFalse;
    }
    if (FndOne) {
      (*FndArgs)++;
      (*nArgs)--;
    }
    addressing.fe=fe;
    addressing.funStr=funStr;
    addressing.FndArgs=*FndArgs;
    addressing.ndx0=ndx0;
    if (AddArgumentAddressing (&addressing, pfx, &indexes) == MagickFalse)
      return MagickFalse;
  return(MagickTrue);
}

typedef struct _FunctionArgumentInputs
{
  FunctionE
    fe;

  const char
    *funStr,
    *strLimit;

  char
    expChLimit;

  int
    ndx0,
    nArgs;

  char
    *pExpStart;
} FunctionArgumentInputs;

typedef struct _FunctionArgumentTargets
{
  char
    *chLimit;

  int
    *FndArgs,
    *ndx1,
    *ndx2,
    *ndx3;

  size_t
    *lenExp;
} FunctionArgumentTargets;

static MagickBooleanType TranslateFunctionArguments (FxInfo * pfx,
  const FunctionArgumentInputs *inputs, const FunctionArgumentTargets *targets)
{
  const FunctionE
    fe = inputs->fe;

  const char
    *const funStr = inputs->funStr,
    *const strLimit = inputs->strLimit;

  const char
    expChLimit = inputs->expChLimit;

  const int
    ndx0 = inputs->ndx0;

  int
    nArgs = inputs->nArgs;

  char
    *const pExpStart = inputs->pExpStart;

  char
    *const chLimit = targets->chLimit;

  int
    *const FndArgs = targets->FndArgs,
    *const ndx1 = targets->ndx1,
    *const ndx2 = targets->ndx2,
    *const ndx3 = targets->ndx3;

  size_t
    *const lenExp = targets->lenExp;

  while (nArgs > 0) {
    int FndOne = 0;
    ArgumentCount count;
    ArgumentCountTargets countTargets;
    const ArgumentIndexes indexes = { ndx1, ndx2, ndx3 };
    if (TranslateStatementList (pfx, strLimit, chLimit)) {
      FndOne = 1;
    } else if (!*pfx->pex) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "For function", "'%s' expected ')' at '%s'",
        funStr, SetShortExp(pfx));
      return MagickFalse;
    } else if (!*chLimit) {
      /* Maybe don't break because other expressions may be not empty. */
      break;
    } else if (IsPadFunction(fe) != MagickFalse) {
      (void) AddElement (pfx, (fxFltType) 0, oNull);
      FndOne = 1;
    }

    count.fe=fe;
    count.funStr=funStr;
    count.strLimit=strLimit;
    count.chLimit=*chLimit;
    count.FndOne=FndOne;
    count.ndx0=ndx0;
    count.indexes=indexes;
    countTargets.FndArgs=(FndArgs);
    countTargets.nArgs=(&nArgs);
    if (CountFunctionArgument (pfx, &count, &countTargets) == MagickFalse)
      return MagickFalse;
    if (*chLimit == expChLimit) {
      *lenExp = (size_t) (pfx->pex - pExpStart - 1);
      break;
    }
  } /* end while args of a function */
  return(MagickTrue);
}

static MagickBooleanType GetFunction (FxInfo * pfx, FunctionE fe)
{
  /* A function, so get open-parens, n args, close-parens
  */
  const char * funStr = Functions[fe-(int) FirstFunc].str;
  int nArgs = Functions[fe-(int) FirstFunc].number_args;
  char chLimit = ')';
  char expChLimit = ')';
  const char *strLimit = ",)";
  OperatorE pushOp = oOpenParen;

  char * pExpStart;

  size_t lenExp = 0;

  int FndArgs = 0;
  int ndx0 = NULL_ADDRESS, ndx1 = NULL_ADDRESS, ndx2 = NULL_ADDRESS, ndx3 = NULL_ADDRESS;

  MagickBooleanType coordQual = MagickFalse;
  PixelChannel chQual = NO_CHAN_QUAL;
  ImgAttrE iaQual = aNull;

  MagickBooleanType returned = MagickFalse;

  FunctionClosing closing;

  FunctionArgumentInputs arguments;

  const FunctionArgumentTargets argumentTargets = { &chLimit, &FndArgs, &ndx1,
    &ndx2, &ndx3, &lenExp };

  const FunctionOpening opening = { &pushOp, &strLimit, &chLimit, &expChLimit,
    &nArgs };

  pfx->pex += pfx->lenToken;

  if (OpenFunction (pfx, fe, &opening) == MagickFalse)
    return MagickFalse;
  if (!PushOperatorStack (pfx, (int) pushOp)) return MagickFalse;

  pExpStart = pfx->pex;
  ndx0 = pfx->usedElements;
  if (fe==fDo) {
    (void) AddAddressingElement (pfx, rGoto, NULL_ADDRESS); /* address will be ndx1+1 */
  }
  if (TryEpochFunction (pfx, fe, pushOp, &returned) != MagickFalse)
    return returned;

  arguments.fe=fe;
  arguments.funStr=funStr;
  arguments.strLimit=strLimit;
  arguments.expChLimit=expChLimit;
  arguments.ndx0=ndx0;
  arguments.nArgs=nArgs;
  arguments.pExpStart=pExpStart;
  if (TranslateFunctionArguments (pfx, &arguments, &argumentTargets) ==
      MagickFalse)
    return MagickFalse;
  closing.fe=fe;
  closing.funStr=funStr;
  closing.ndx0=ndx0;
  closing.ndx1=ndx1;
  closing.ndx2=ndx2;
  closing.ndx3=ndx3;
  closing.expChLimit=expChLimit;
  closing.pushOp=pushOp;
  closing.coordQual=coordQual;
  closing.chQual=chQual;
  closing.iaQual=iaQual;
  closing.pExpStart=pExpStart;
  closing.lenExp=lenExp;
  return(FinishFunctionArguments (pfx, FndArgs, chLimit, &closing));
}

static MagickBooleanType IsStealth (int op)
{
  return ((MagickBooleanType) (op == fU0 || op == fUP || op == fSP ||
    op == fVP || (op >= FirstCont && op <= rNull)) );
}

static inline MagickBooleanType IsLoopOrIfFunction(const FunctionE fe)
{
  return((MagickBooleanType) (fe == fDo || fe == fFor || fe == fIf ||
    fe == fWhile));
}

static inline MagickBooleanType IsHslSymbol(const SymbolE se)
{
  return((MagickBooleanType) (se==sHue || se==sSaturation || se==sLightness));
}

static double GetPrefixPower (char * tailptr, double Pow)
{
  const char Prefixes[] = "yzafpnum.kMGTPEZY";
  const char * pSi = strchr (Prefixes, *tailptr);
  if (pSi && *pSi != '.') Pow = (double) ((pSi - Prefixes) * 3 - 24);
  else if (*tailptr == 'c') Pow = -2;
  else if (*tailptr == 'h') Pow =  2;
  else if (*tailptr == 'k') Pow =  3;
  return(Pow);
}

static fxFltType ApplyNumberPrefix (FxInfo * pfx, char * tailptr, fxFltType val)
{
  /* Could have "prefix" K, Ki, M etc.
     See https://en.wikipedia.org/wiki/Metric_prefix
     and https://en.wikipedia.org/wiki/Binary_prefix
  */
  double Pow = 0.0;
  Pow = GetPrefixPower (tailptr, Pow);
  if (Pow != 0.0) {
    if (*(++pfx->pex) == 'i') {
      val *= pow (2.0, Pow/0.3);
      pfx->pex++;
    } else {
      val *= pow (10.0, Pow);
    }
  }
  return val;
}

static MagickBooleanType GetParenthesisedOperand (
  FxInfo * pfx, OperatorE op, MagickBooleanType * needPopAll)
{
  char chLimit = '\0';
  if (!PushOperatorStack (pfx, (int) op)) return MagickFalse;
  pfx->pex++;
  if (!TranslateExpression (pfx, ")", &chLimit, needPopAll)) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Empty expression in parentheses at", "'%s'",
      SetShortExp(pfx));
    return MagickFalse;
  }
  if (chLimit != ')') {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "'(' but no ')' at", "'%s'",
      SetShortExp(pfx));
    return MagickFalse;
  }
  /* Top of opr stack should be '('. */
  if (!PopOprOpenParen (pfx, oOpenParen)) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Bug: tos not '(' at", "'%s'",
      SetShortExp(pfx));
    return MagickFalse;
  }
  return MagickTrue;
}

static MagickBooleanType GetHexColourOperand (FxInfo * pfx)
{
  fxFltType v0=0, v1=0, v2=0;
  ssize_t lenToken = GetHexColour (pfx, &v0, &v1, &v2);
  if (lenToken < 0) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Bad hex number at", "'%s'",
      SetShortExp(pfx));
    return MagickFalse;
  } else if (lenToken > 0) {
    (void) AddColourElement (pfx, v0, v1, v2);
    pfx->pex+=lenToken;
  }
  return MagickTrue;
}

static void SetLastElementChannel (FxInfo * pfx, PixelChannel chQual)
{
  ElementT * pel;
  pel = &pfx->Elements[pfx->usedElements-1];
  pel->channel_qual = chQual;
}

static MagickBooleanType AddImgAttrOperand (FxInfo * pfx, ImgAttrE ia)
{
  fxFltType val = 0;
  (void) AddElement (pfx, val, (int) ia);

  if (ImgAttrs[ia-(int) FirstImgAttr].need_stats != MagickFalse) {
    if (IsQualifier (pfx)) {
      PixelChannel chQual = GetChannelQualifier (pfx, (int) ia);
      if (chQual == NO_CHAN_QUAL) {
        (void) ThrowMagickException (
          pfx->exception, GetMagickModule(), OptionError,
          "Bad channel qualifier at", "'%s'",
          SetShortExp(pfx));
        return MagickFalse;
      }
      /* Adjust the element */
      SetLastElementChannel (pfx, chQual);
    }
  }
  return MagickTrue;
}

static MagickBooleanType AddArtifactOperand (FxInfo * pfx, const char * artifact)
{
  char * tailptr;
  fxFltType val = strtold (artifact, &tailptr);
  if (pfx->token == tailptr) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Artifact", "'%s' has value '%s', not a number, at '%s'",
      pfx->token, artifact, SetShortExp(pfx));
    return MagickFalse;
  }
  (void) AddElement (pfx, val, oNull);
  pfx->pex+=pfx->lenToken;
  return MagickTrue;
}

static SymbolE FindSymbolToken (FxInfo * pfx)
{
  SymbolE se;
  for (se = FirstSym; se < sNull; se=(SymbolE) (se+1)) {
    const char * seStr = Symbols[se-(int) FirstSym].str;
    if (LocaleCompare (seStr, pfx->token)==0) {
      break;
    }
  }
  return(se);
}

static MagickBooleanType NotASymbolOperand (FxInfo * pfx)
{
  SymbolE se;
  se = FindSymbolToken (pfx);
  if (se != sNull) {
    fxFltType val = 0;
    (void) AddElement (pfx, val, (int) se);
    pfx->pex += pfx->lenToken;

    if (IsHslSymbol(se) != MagickFalse) pfx->NeedHsl = MagickTrue;
    return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType NotAUserSymbolOperand (FxInfo * pfx,
  MagickBooleanType * UserSymbol, MagickBooleanType * NewUserSymbol,
  int * UserSymNdx)
{
  if (TokenMaybeUserSymbol (pfx)) {
    *UserSymbol = MagickTrue;
    *UserSymNdx = FindUserSymbol (pfx, pfx->token);
    if (*UserSymNdx == NULL_ADDRESS) {
      *UserSymNdx = AddUserSymbol (pfx, pfx->pex, pfx->lenToken);
      *NewUserSymbol = MagickTrue;
    } else {
    }
    pfx->pex += pfx->lenToken;

    return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType GetUnaryPrefixOperand (FxInfo * pfx, OperatorE op,
  const OperandSymbols * symbols)
{
  MagickBooleanType
    *const UserSymbol = symbols->UserSymbol,
    *const NewUserSymbol = symbols->NewUserSymbol;

  int
    *const UserSymNdx = symbols->UserSymNdx;

  MagickBooleanType operand_ok;
  if (!PushOperatorStack (pfx, (int) op)) return MagickFalse;
  pfx->pex++;
  SkipSpaces (pfx);
  if (!*pfx->pex) return MagickFalse;
  if (pfx->teDepth >= MagickMaxRecursionDepth) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Expression too deeply nested", "(depth %i exceeds limit %i)",
      pfx->teDepth, MagickMaxRecursionDepth);
    return MagickFalse;
  }
  pfx->teDepth++;
  operand_ok=GetOperand (pfx, symbols);
  pfx->teDepth--;
  if (!operand_ok) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "After unary, bad operand at", "'%s'",
      SetShortExp(pfx));
    return MagickFalse;
  }

  if (*NewUserSymbol) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "After unary, NewUserSymbol at", "'%s'",
      SetShortExp(pfx));
    return MagickFalse;
  }

  if (*UserSymbol) {
    (void) AddAddressingElement (pfx, rCopyFrom, *UserSymNdx);
    *UserSymNdx = NULL_ADDRESS;

    *UserSymbol = MagickFalse;
    *NewUserSymbol = MagickFalse;
  }

  (void) GetToken (pfx);
  return MagickTrue;
}

static MagickBooleanType NotAConstantOperand (FxInfo * pfx)
{
  ConstantE ce;
  for (ce = (ConstantE)0; ce < cNull; ce=(ConstantE) (ce+1)) {
    const char * ceStr = Constants[ce].str;
    if (LocaleCompare (ceStr, pfx->token)==0) {
      break;
    }
  }

  if (ce != cNull) {
    (void) AddElement (pfx, Constants[ce].val, oNull);
    pfx->pex += pfx->lenToken;
    return MagickFalse;
  }
  return MagickTrue;
}

static MagickBooleanType TryPrefixOperand (FxInfo * pfx,
  const OperandSymbols * operand, MagickBooleanType * result)
{
  /* Try '(' or unary prefix
  */
  OperatorE op = GetLeadingOp (pfx);
  if (op==oOpenParen) {
    (*result)=GetParenthesisedOperand (pfx, op, operand->needPopAll);
    return MagickTrue;
  } else if (OprIsUnaryPrefix (op)) {
    OperandSymbols
      symbols;

    symbols.UserSymbol=operand->UserSymbol;
    symbols.NewUserSymbol=operand->NewUserSymbol;
    symbols.UserSymNdx=operand->UserSymNdx;
    symbols.needPopAll=operand->needPopAll;
    (*result)=GetUnaryPrefixOperand (pfx, op, &symbols);
    return MagickTrue;
  } else if (*pfx->pex == '#') {
    (*result)=GetHexColourOperand (pfx);
    return MagickTrue;
  }
  return MagickFalse;
}

static MagickBooleanType TryNumberOperand (FxInfo * pfx,
  MagickBooleanType * result)
{
  /* Try a constant number.
  */
  char * tailptr;
  ssize_t lenOptArt;
  fxFltType val = strtold (pfx->pex, &tailptr);
  if (pfx->pex != tailptr) {
    pfx->pex = tailptr;
    if (*tailptr) {
      val = ApplyNumberPrefix (pfx, tailptr, val);
    }
    (void) AddElement (pfx, val, oNull);
    (*result)=MagickTrue;
    return MagickTrue;
  }

  val = (fxFltType) 0;
  lenOptArt = GetProperty (pfx, &val, NULL);
  if (lenOptArt < 0) {
    (*result)=MagickFalse;
    return MagickTrue;
  }
  if (lenOptArt > 0) {
    (void) AddElement (pfx, val, oNull);
    pfx->pex += lenOptArt;
    (*result)=MagickTrue;
    return MagickTrue;
  }
  return MagickFalse;
}

static MagickBooleanType TryLeadingOperand (FxInfo * pfx,
  const OperandSymbols * operand, MagickBooleanType * result)
{
  MagickBooleanType
    returned;

  if (pfx->lenToken==0) {

    if (TryPrefixOperand (pfx, operand, &returned) != MagickFalse) {
      (*result)=returned;
      return MagickTrue;
    }

    if (TryNumberOperand (pfx, &returned) != MagickFalse) {
      (*result)=returned;
      return MagickTrue;
    }

  } /* end of lenToken==0 */
  return MagickFalse;
}

static MagickBooleanType TryFunctionOperand (FxInfo * pfx,
  MagickBooleanType * needPopAll, MagickBooleanType * result)
{
  FunctionE fe;
  for (fe = FirstFunc; fe < fNull; fe=(FunctionE) (fe+1)) {
    const char * feStr = Functions[fe-(int) FirstFunc].str;
    if (LocaleCompare (feStr, pfx->token)==0) {
      break;
    }
  }

  if (fe == fV && pfx->ImgListLen < 2) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Symbol 'v' but fewer than two images at", "'%s'",
      SetShortExp(pfx));
    (*result)=MagickFalse;
    return MagickTrue;
  }

  if (IsStealth ((int) fe)) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Function", "'%s' not permitted at '%s'",
      pfx->token, SetShortExp(pfx));
  }

  if (IsLoopOrIfFunction(fe) != MagickFalse) {
    *needPopAll = MagickTrue;
  }

  if (fe != fNull) {
    (*result)=GetFunction (pfx, fe);
    return MagickTrue;
  }
  return MagickFalse;
}

static MagickBooleanType TryNamedOperand (FxInfo * pfx,
  const OperandSymbols * operand, MagickBooleanType * result)
{
  MagickBooleanType
    *const UserSymbol = operand->UserSymbol,
    *const NewUserSymbol = operand->NewUserSymbol;

  int
    *const UserSymNdx = operand->UserSymNdx;

  /* Try image attribute
  */
  {
    ImgAttrE ia = GetImgAttrToken (pfx);
    if (ia != aNull) {
      (*result)=(AddImgAttrOperand (pfx, ia));
      return(MagickTrue);
    }
  }

  /* Try symbol
  */
  if (NotASymbolOperand (pfx) == MagickFalse) {
    (*result)=MagickTrue;
    return(MagickTrue);
  }

  /* Try constant colour.
  */
  {
    fxFltType v0, v1, v2;
    ssize_t ColLen = GetConstantColour (pfx, &v0, &v1, &v2);
    if (ColLen < 0) {
      (*result)=MagickFalse;
      return(MagickTrue);
    }
    if (ColLen > 0) {
      (void) AddColourElement (pfx, v0, v1, v2);
      pfx->pex+=ColLen;
      (*result)=MagickTrue;
      return(MagickTrue);
    }
  }

  /* Try image artifact.
  */
  {
    const char *artifact;
    artifact = GetImageArtifact (pfx->image, pfx->token);
    if (artifact != (const char *) NULL) {
      (*result)=(AddArtifactOperand (pfx, artifact));
      return(MagickTrue);
    }
  }

  /* Try user symbols. If it is, don't AddElement yet.
  */
  if (NotAUserSymbolOperand (pfx, UserSymbol, NewUserSymbol, UserSymNdx) ==
      MagickFalse) {
    (*result)=MagickTrue;
    return(MagickTrue);
  }
  return(MagickFalse);
}

static MagickBooleanType GetOperand (
  FxInfo * pfx, const OperandSymbols * operand)
{
  MagickBooleanType
    *const UserSymbol = operand->UserSymbol,
    *const NewUserSymbol = operand->NewUserSymbol;

  int
    *const UserSymNdx = operand->UserSymNdx;

  MagickBooleanType
    *const needPopAll = operand->needPopAll;

  MagickBooleanType
    returned;

  *NewUserSymbol = *UserSymbol = MagickFalse;
  *UserSymNdx = NULL_ADDRESS;

  SkipSpaces (pfx);
  if (!*pfx->pex) return MagickFalse;
  (void) GetToken (pfx);

  if (TryLeadingOperand (pfx, operand, &returned) != MagickFalse)
    return returned;

  if (pfx->lenToken > 0) {
    /* Try a constant
    */
    if (NotAConstantOperand (pfx) == MagickFalse)
      return MagickTrue;

    /* Try a function
    */
    if (TryFunctionOperand (pfx, needPopAll, &returned) != MagickFalse)
      return returned;

    if (TryNamedOperand (pfx, operand, &returned) != MagickFalse)
      return(returned);
  }

  (void) ThrowMagickException (
    pfx->exception, GetMagickModule(), OptionError,
    "Expected operand at", "'%s'",
    SetShortExp(pfx));

  return MagickFalse;
}

static inline MagickBooleanType IsRealOperator (OperatorE op)
{
  return (MagickBooleanType) (op < oOpenParen || op > oCloseBrace);
}

static MagickBooleanType ProcessTernaryColon (FxInfo * pfx, TernaryT * ptern)
{
  if (pfx->OperatorStack[pfx->usedOprStack-1] == oColon) {
    if (ptern->addr_query == NULL_ADDRESS) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Need '?' in sub-expression at", "'%s'",
        SetShortExp(pfx));
      return(MagickFalse);
    }
    if (ptern->addr_colon != NULL_ADDRESS) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Already have ':' in sub-expression at", "'%s'",
        SetShortExp(pfx));
      return(MagickFalse);
    }
    pfx->usedOprStack--;
    ptern->addr_colon = pfx->usedElements;
    pfx->Elements[pfx->usedElements-1].do_push = MagickTrue;
    (void) AddAddressingElement (pfx, rGoto, NULL_ADDRESS);
    /* address will be after the subexpression */
  }
  return(MagickTrue);
}

static inline MagickBooleanType ProcessTernaryOpr (FxInfo * pfx, TernaryT * ptern)
/* Ternary operator "... ? ... : ..."
   returns false iff we have exception
*/
{
  if (pfx->usedOprStack == 0)
    return MagickFalse;
  if (pfx->OperatorStack[pfx->usedOprStack-1] == oQuery) {
    if (ptern->addr_query != NULL_ADDRESS) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Already have '?' in sub-expression at", "'%s'",
        SetShortExp(pfx));
      return MagickFalse;
    }
    if (ptern->addr_colon != NULL_ADDRESS) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Already have ':' in sub-expression at", "'%s'",
        SetShortExp(pfx));
      return MagickFalse;
    }
    pfx->usedOprStack--;
    ptern->addr_query = pfx->usedElements;
    (void) AddAddressingElement (pfx, rIfZeroGoto, NULL_ADDRESS);
    /* address will be one after the Colon address. */
  }
  else if (ProcessTernaryColon (pfx, ptern) == MagickFalse)
    return MagickFalse;
  return MagickTrue;
}

static inline MagickBooleanType IsOperatorStackBarrier(const OperatorE top)
{
  return((MagickBooleanType) (top == oOpenParen || top == oAssign ||
    OprInPlace ((int) top)));
}

static void MoveHigherPrecedenceOperators (FxInfo * pfx, OperatorE op)
{
  while (pfx->usedOprStack > 0) {
    OperatorE top = pfx->OperatorStack[pfx->usedOprStack-1];
    int precTop, precNew;
    if (IsOperatorStackBarrier(top) != MagickFalse) break;
    precTop = Operators[top].precedence;
    precNew = Operators[op].precedence;
    /* Assume left associativity.
       If right assoc, this would be "<=".
    */
    if (precTop < precNew) break;
    (void) AddElement (pfx, (fxFltType) 0, (int) top);
    pfx->usedOprStack--;
  }
}

static MagickBooleanType PopCloseParen (FxInfo * pfx, OperatorE op,
  MagickBooleanType * DoneIt)
{
  if (op==oCloseParen) {
    if (pfx->usedOprStack == 0) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Found ')' but nothing on stack at", "'%s'",
        SetShortExp(pfx));
      return(MagickFalse);
    }

    if (pfx->OperatorStack[pfx->usedOprStack-1] != oOpenParen) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Found ')' but no '(' on stack at", "'%s'",
        SetShortExp(pfx));
      return(MagickFalse);
    }
    pfx->usedOprStack--;
    *DoneIt = MagickTrue;
  }
  return(MagickTrue);
}

static MagickBooleanType PushOperatorUnlessDone (FxInfo * pfx, OperatorE op,
  MagickBooleanType DoneIt)
{
  if (!DoneIt) {
    if (!PushOperatorStack (pfx, (int) op)) return(MagickFalse);
  }
  return(MagickTrue);
}

static void FindOperatorToken (FxInfo * pfx, OperatorE * op, size_t * len)
{
  for (*op = (OperatorE)0; *op != oNull; *op=(OperatorE) (*op+1)) {
    const char * opStr = Operators[*op].str;
    *len = strlen(opStr);
    if (LocaleNCompare (opStr, pfx->pex, *len)==0) {
      break;
    }
  }
}

static void SetOperatorFlags (OperatorE op, MagickBooleanType * Assign,
  MagickBooleanType * Update, MagickBooleanType * IncrDecr)
{
  *Assign = (MagickBooleanType) (op==oAssign);
  *Update = OprInPlace ((int) op);
  *IncrDecr = (MagickBooleanType) (op == oPlusPlus || op == oSubSub);
}

static MagickBooleanType GetOperator (
  FxInfo * pfx,
  MagickBooleanType * Assign, MagickBooleanType * Update, MagickBooleanType * IncrDecr)
{
  OperatorE op;
  size_t len = 0;
  MagickBooleanType DoneIt = MagickFalse;
  SkipSpaces (pfx);
  FindOperatorToken (pfx, &op, &len);

  if (!IsRealOperator (op)) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Not a real operator at", "'%s'",
      SetShortExp(pfx));
    return MagickFalse;
  }

  if (op==oNull) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Expected operator at", "'%s'",
      SetShortExp(pfx));
    return MagickFalse;
  }

  SetOperatorFlags (op, Assign, Update, IncrDecr);

  /* while top of OperatorStack is not empty and is not open-parens or assign,
       and top of OperatorStack is higher precedence than new op,
     then move top of OperatorStack to Element list.
  */

  MoveHigherPrecedenceOperators (pfx, op);

  /* If new op is close paren, and stack top is open paren,
     remove stack top.
  */
  if (PopCloseParen (pfx, op, &DoneIt) == MagickFalse)
    return MagickFalse;

  if (PushOperatorUnlessDone (pfx, op, DoneIt) == MagickFalse)
    return MagickFalse;

  pfx->pex += len;

  return MagickTrue;
}

static void LinkTernaryAddresses (FxInfo * pfx, TernaryT * ptern)
{
  pfx->Elements[ptern->addr_query].element_index = ptern->addr_colon + 1;
  pfx->Elements[ptern->addr_colon].element_index = pfx->usedElements;
  ptern->addr_query = NULL_ADDRESS;
  ptern->addr_colon = NULL_ADDRESS;
}

static MagickBooleanType ResolveTernaryAddresses (FxInfo * pfx, TernaryT * ptern)
{
  if (ptern->addr_query == NULL_ADDRESS && ptern->addr_colon == NULL_ADDRESS)
    return MagickTrue;

  if (ptern->addr_query != NULL_ADDRESS && ptern->addr_colon != NULL_ADDRESS) {
    LinkTernaryAddresses (pfx, ptern);
  } else if (ptern->addr_query != NULL_ADDRESS) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "'?' with no corresponding ':'", "'%s' at '%s'",
        pfx->token, SetShortExp(pfx));
      return MagickFalse;
  } else if (ptern->addr_colon != NULL_ADDRESS) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "':' with no corresponding '?'", "'%s' at '%s'",
        pfx->token, SetShortExp(pfx));
      return MagickFalse;
  }
  return MagickTrue;
}

static inline MagickBooleanType IsExpressionContinuing(FxInfo *pfx,
  const char *strLimit)
{
  return((MagickBooleanType) (*pfx->pex && (!*strLimit ||
    (strchr(strLimit,*pfx->pex)==NULL))));
}

static inline MagickBooleanType IsUserSymbolAssignment(
  const MagickBooleanType UserSymbol,const MagickBooleanType Assign,
  const MagickBooleanType Update,const MagickBooleanType IncrDecr)
{
  return((MagickBooleanType) (UserSymbol && (Assign || Update) && !IncrDecr));
}

static inline MagickBooleanType IsExpressionEnd(FxInfo *pfx,
  const char *strLimit)
{
  return((MagickBooleanType) ( (!*pfx->pex) || (*strLimit &&
    (strchr(strLimit,*pfx->pex)!=NULL) ) ));
}

static inline MagickBooleanType IsPendingLimitChar(FxInfo *pfx,
  const char *chLimit,const char *strLimit)
{
  return((MagickBooleanType) (*pfx->pex && !*chLimit &&
    (strchr(strLimit,*pfx->pex)!=NULL)));
}

static inline MagickBooleanType IsOpenGroupOperator(const OperatorE op)
{
  return((MagickBooleanType) (op == oOpenParen || op == oOpenBracket ||
    op == oOpenBrace));
}

static inline MagickBooleanType IsMisplacedAssignment(const OperatorE op,
  const MagickBooleanType Assign,const MagickBooleanType Update)
{
  return((MagickBooleanType) ( (op==oAssign && !Assign) ||
    (OprInPlace((int) op) && !Update) ));
}

static inline MagickBooleanType IsUnassignedUserSymbolRead(
  const MagickBooleanType UserSymbol,const MagickBooleanType Assign,
  const MagickBooleanType Update,const int UserSymNdx0)
{
  return((MagickBooleanType) (UserSymbol && !Assign && !Update &&
    UserSymNdx0 != NULL_ADDRESS));
}

static void FlushUnaryPrefixOperators (FxInfo * pfx)
{
  while (TopOprIsUnaryPrefix (pfx)) {
    OperatorE op = pfx->OperatorStack[pfx->usedOprStack-1];
    (void) AddElement (pfx, (fxFltType) 0, (int) op);
    pfx->usedOprStack--;
  }
}

static MagickBooleanType AddFinalCopyFrom (FxInfo * pfx,
  MagickBooleanType NewUserSymbol, int UserSymNdx0)
{
  ElementT * pel;
  if (NewUserSymbol) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "NewUserSymbol", "'%s' needs assignment operator at '%s'",
      pfx->token, SetShortExp(pfx));
    return MagickFalse;
  }
  (void) AddAddressingElement (pfx, rCopyFrom, UserSymNdx0);
  pel = &pfx->Elements[pfx->usedElements-1];
  pel->do_push = MagickTrue;
  return MagickTrue;
}

static MagickBooleanType AddAssignmentCopyTo (FxInfo * pfx, int UserSymNdx0)
{
  if (UserSymNdx0 < 0) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Assignment to unknown user symbol at", "'%s'",
      SetShortExp(pfx));
    return MagickFalse;
  }
  /* Adjust last element, by deletion and add.
  */
  pfx->usedElements--;
  (void) AddAddressingElement (pfx, rCopyTo, UserSymNdx0);
  return MagickTrue;
}

static MagickBooleanType SetInPlaceTarget (FxInfo * pfx, int UserSymNdx0)
{
  if (UserSymNdx0 < 0) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Operator-in-place to unknown user symbol at", "'%s'",
      SetShortExp(pfx));
    return MagickFalse;
  }
  /* Modify latest element.
  */
  pfx->Elements[pfx->usedElements-1].element_index = UserSymNdx0;
  return MagickTrue;
}

static MagickBooleanType ExpectOperandAfterOperator (FxInfo * pfx,
  const char * strLimit, MagickBooleanType IncrDecr, MagickBooleanType * broke)
{
  (*broke)=MagickFalse;
  if (IsExpressionEnd(pfx,strLimit) != MagickFalse)
  {
    if (IncrDecr) {
      (*broke)=MagickTrue;
      return(MagickTrue);
    }

    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Expected operand after operator", "at '%s'",
      SetShortExp(pfx));
    return MagickFalse;
  }
  return(MagickTrue);
}

typedef struct _ExpressionContext
{
  FxInfo
    *pfx;

  char
    *chLimit;

  MagickBooleanType
    *needPopAll;
} ExpressionContext;

static MagickBooleanType TranslateTernaryElse (
  const ExpressionContext * context, TernaryT ternary,
  MagickBooleanType * broke)
{
  FxInfo
    *const pfx = context->pfx;

  char
    *const chLimit = context->chLimit;

  MagickBooleanType
    *const needPopAll = context->needPopAll;

  (*broke)=MagickFalse;
  if (ternary.addr_colon != NULL_ADDRESS) {
    if (!TranslateExpression (pfx, ",);", chLimit, needPopAll)) return MagickFalse;
    (*broke)=MagickTrue;
    return(MagickTrue);
  }
  return(MagickTrue);
}

static MagickBooleanType SetInPlaceOperatorTarget (FxInfo * pfx,
  OperatorE op, int UserSymNdx0, MagickBooleanType * broke)
{
  (*broke)=MagickFalse;
  if (OprInPlace ((int) op)) {
    if (!SetInPlaceTarget (pfx, UserSymNdx0)) return MagickFalse;
    (*broke)=MagickTrue;
    return(MagickTrue);
  }
  return(MagickTrue);
}

typedef struct _UserSymbolAssignment
{
  FxInfo
    *pfx;

  const char
    *strLimit;

  char
    *chLimit;

  MagickBooleanType
    *needPopAll,
    UserSymbol,
    Assign,
    Update,
    IncrDecr;
} UserSymbolAssignment;

static MagickBooleanType TranslateAssignedExpression (
  const UserSymbolAssignment * assignment, MagickBooleanType * broke)
{
  FxInfo
    *const pfx = assignment->pfx;

  const char
    *const strLimit = assignment->strLimit;

  char
    *const chLimit = assignment->chLimit;

  MagickBooleanType
    *const needPopAll = assignment->needPopAll;

  const MagickBooleanType
    UserSymbol = assignment->UserSymbol,
    Assign = assignment->Assign,
    Update = assignment->Update,
    IncrDecr = assignment->IncrDecr;

  (*broke)=MagickFalse;
  if (IsUserSymbolAssignment(UserSymbol,Assign,Update,IncrDecr) != MagickFalse) {

    if (!TranslateExpression (pfx, strLimit, chLimit, needPopAll)) return MagickFalse;
    if (!*pfx->pex) {
      (*broke)=MagickTrue;
      return(MagickTrue);
    }
    if (!*strLimit) {
      (*broke)=MagickTrue;
      return(MagickTrue);
    }
    if (strchr(strLimit,*chLimit)!=NULL) {
      (*broke)=MagickTrue;
      return(MagickTrue);
    }
  }
  return(MagickTrue);
}

static MagickBooleanType SetAssignmentTarget (FxInfo * pfx,
  OperatorE op, int UserSymNdx0, MagickBooleanType * broke)
{
  (*broke)=MagickFalse;
  if (op == oAssign) {
    if (!AddAssignmentCopyTo (pfx, UserSymNdx0)) return MagickFalse;
    (*broke)=MagickTrue;
    return(MagickTrue);
  } else if (SetInPlaceOperatorTarget (pfx, op, UserSymNdx0, broke) ==
      MagickFalse) {
    return MagickFalse;
  } else if ((*broke) != MagickFalse) {
    (*broke)=MagickTrue;
    return(MagickTrue);
  }
  return(MagickTrue);
}

static MagickBooleanType FlushOperatorStack (FxInfo * pfx,
  MagickBooleanType Assign, MagickBooleanType Update, int UserSymNdx0)
{
  MagickBooleanType broke = MagickFalse;

  while (pfx->usedOprStack) {
    OperatorE op = pfx->OperatorStack[pfx->usedOprStack-1];
    if (IsOpenGroupOperator(op) != MagickFalse) {
      break;
    }
    if (IsMisplacedAssignment(op,Assign,Update) != MagickFalse) {
      break;
    }
    pfx->usedOprStack--;
    (void) AddElement (pfx, (fxFltType) 0, (int) op);
    if (SetAssignmentTarget (pfx, op, UserSymNdx0, &broke) == MagickFalse)
      return(MagickFalse);
    if (broke != MagickFalse) break;
  }
  return(MagickTrue);
}

typedef struct _OperatorFlags
{
  MagickBooleanType
    *Assign,
    *Update,
    *IncrDecr;
} OperatorFlags;

static MagickBooleanType GetExpressionOperator (FxInfo * pfx,
  const MagickBooleanType UserSymbol, const MagickBooleanType NewUserSymbol,
  const OperatorFlags * targets)
{
  MagickBooleanType
    *const Assign = targets->Assign,
    *const Update = targets->Update,
    *const IncrDecr = targets->IncrDecr;

    if (!GetOperator (pfx, Assign, Update, IncrDecr)) return MagickFalse;
    SkipSpaces (pfx);
    if (NewUserSymbol && !*Assign) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Expected assignment after new UserSymbol", "'%s' at '%s'",
        pfx->token, SetShortExp(pfx));
      return MagickFalse;
    }
    if (!UserSymbol && *Assign) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Attempted assignment to non-UserSymbol", "'%s' at '%s'",
        pfx->token, SetShortExp(pfx));
      return MagickFalse;
    }
    if (!UserSymbol && *Update) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Attempted update to non-UserSymbol", "'%s' at '%s'",
        pfx->token, SetShortExp(pfx));
      return MagickFalse;
    }
  return(MagickTrue);
}

typedef struct _OperandTargets
{
  MagickBooleanType
    *UserSymbol,
    *NewUserSymbol;

  int
    *UserSymNdx0;
} OperandTargets;

static int AddUserSymbolCopyFrom (FxInfo * pfx,
  MagickBooleanType *const UserSymbol, MagickBooleanType *const NewUserSymbol,
  int UserSymNdx1)
{
    if (*UserSymbol && !*NewUserSymbol) {
      (void) AddAddressingElement (pfx, rCopyFrom, UserSymNdx1);
      UserSymNdx1 = NULL_ADDRESS;
    }
  return(UserSymNdx1);
}

static MagickBooleanType GetExpressionOperand (FxInfo * pfx,
  const MagickBooleanType Assign, MagickBooleanType * needPopAll,
  const OperandTargets * targets)
{
  MagickBooleanType
    *const UserSymbol = targets->UserSymbol,
    *const NewUserSymbol = targets->NewUserSymbol;

  int
    *const UserSymNdx0 = targets->UserSymNdx0;

  int UserSymNdx1;

  const OperandSymbols
    operand1 = { UserSymbol, NewUserSymbol, &UserSymNdx1, needPopAll };

    if (!GetOperand (pfx, &operand1)) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "Expected operand at", "'%s'",
        SetShortExp(pfx));
      return MagickFalse;
    }
    SkipSpaces (pfx);
    if (*NewUserSymbol && !Assign) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "NewUserSymbol", "'%s' after non-assignment operator at '%s'",
        pfx->token, SetShortExp(pfx));
      return MagickFalse;
    }
    UserSymNdx1 = AddUserSymbolCopyFrom (pfx, UserSymbol, NewUserSymbol,
      UserSymNdx1);
    *UserSymNdx0 = UserSymNdx1;
  return(MagickTrue);
}

static MagickBooleanType TranslateAssignmentOperand (
  const UserSymbolAssignment * assignment, int * UserSymNdx0,
  MagickBooleanType * broke)
{
  FxInfo
    *const pfx = assignment->pfx;

  const MagickBooleanType
    UserSymbol = assignment->UserSymbol,
    Assign = assignment->Assign,
    Update = assignment->Update;

  (*broke)=MagickFalse;
    if (TranslateAssignedExpression (assignment, broke) == MagickFalse)
      return MagickFalse;
    if ((*broke) != MagickFalse) {
      (*broke)=MagickTrue;
      return(MagickTrue);
    }
    if (IsUnassignedUserSymbolRead(UserSymbol,Assign,Update,*UserSymNdx0) !=
        MagickFalse) {
      ElementT * pel;
      (void) AddAddressingElement (pfx, rCopyFrom, *UserSymNdx0);
      *UserSymNdx0 = NULL_ADDRESS;
      pel = &pfx->Elements[pfx->usedElements-1];
      pel->do_push = MagickTrue;
    }

    if (UserSymbol) {
      FlushUnaryPrefixOperators (pfx);
    }
  return(MagickTrue);
}

typedef struct _NextOperandTargets
{
  TernaryT
    *ternary;

  MagickBooleanType
    *UserSymbol,
    *NewUserSymbol;
} NextOperandTargets;

static MagickBooleanType PrepareNextOperand (
  const ExpressionContext * context, const UserSymbolAssignment * assignment,
  const NextOperandTargets * targets, MagickBooleanType * broke)
{
  FxInfo
    *const pfx = assignment->pfx;

  const char
    *const strLimit = assignment->strLimit;

  const MagickBooleanType
    IncrDecr = assignment->IncrDecr;

  TernaryT
    *const ternary = targets->ternary;

  MagickBooleanType
    *const UserSymbol = targets->UserSymbol,
    *const NewUserSymbol = targets->NewUserSymbol;

  (*broke)=MagickFalse;
    if (!ProcessTernaryOpr (pfx, ternary)) return MagickFalse;

    if (TranslateTernaryElse (context, *ternary, broke) == MagickFalse)
      return MagickFalse;
    if ((*broke) != MagickFalse) {
      (*broke)=MagickTrue;
      return(MagickTrue);
    }

    *UserSymbol = *NewUserSymbol = MagickFalse;

    if (ExpectOperandAfterOperator (pfx, strLimit, IncrDecr, broke) ==
        MagickFalse)
      return MagickFalse;
    if ((*broke) != MagickFalse) {
      (*broke)=MagickTrue;
      return(MagickTrue);
    }

    if (IncrDecr) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "'++' and '--' must be the final operators in an expression at", "'%s'",
        SetShortExp(pfx));
      return MagickFalse;
    }
  return(MagickTrue);
}

typedef struct _ExpressionTargets
{
  MagickBooleanType
    *Assign,
    *Update;

  int
    *UserSymNdx0;

  TernaryT
    *ternary;

  MagickBooleanType
    *UserSymbol,
    *NewUserSymbol;
} ExpressionTargets;

static MagickBooleanType TranslateOperatorsAndOperands (
  const ExpressionContext * context, const char * strLimit,
  const ExpressionTargets * targets)
{
  FxInfo
    *const pfx = context->pfx;

  char
    *const chLimit = context->chLimit;

  MagickBooleanType
    *const needPopAll = context->needPopAll;

  MagickBooleanType
    *const Assign = targets->Assign,
    *const Update = targets->Update;

  int
    *const UserSymNdx0 = targets->UserSymNdx0;

  TernaryT
    *const ternary = targets->ternary;

  MagickBooleanType
    *const UserSymbol = targets->UserSymbol,
    *const NewUserSymbol = targets->NewUserSymbol;

  MagickBooleanType
    IncrDecr = MagickFalse;

  MagickBooleanType broke = MagickFalse;

  UserSymbolAssignment assignment;

  while (IsExpressionContinuing(pfx,strLimit) != MagickFalse) {
    OperatorFlags flags;
    OperandTargets operandTargets;
    NextOperandTargets nextTargets;

    flags.Assign=(Assign);
    flags.Update=(Update);
    flags.IncrDecr=(&IncrDecr);
    if (GetExpressionOperator (pfx, *UserSymbol, *NewUserSymbol, &flags) ==
        MagickFalse)
      return MagickFalse;
    assignment.pfx=pfx;
    assignment.strLimit=strLimit;
    assignment.chLimit=chLimit;
    assignment.needPopAll=needPopAll;
    assignment.UserSymbol=*UserSymbol;
    assignment.Assign=*Assign;
    assignment.Update=*Update;
    assignment.IncrDecr=IncrDecr;
    if (TranslateAssignmentOperand (&assignment, UserSymNdx0, &broke) ==
        MagickFalse)
      return MagickFalse;
    if (broke != MagickFalse) break;

    nextTargets.ternary=(ternary);
    nextTargets.UserSymbol=(UserSymbol);
    nextTargets.NewUserSymbol=(NewUserSymbol);
    if (PrepareNextOperand (context, &assignment, &nextTargets, &broke) ==
        MagickFalse)
      return MagickFalse;
    if (broke != MagickFalse) break;

    operandTargets.UserSymbol=(UserSymbol);
    operandTargets.NewUserSymbol=(NewUserSymbol);
    operandTargets.UserSymNdx0=(UserSymNdx0);
    if (GetExpressionOperand (pfx, *Assign, needPopAll, &operandTargets) ==
        MagickFalse)
      return MagickFalse;
  }
  return(MagickTrue);
}

static MagickBooleanType BeginExpression (FxInfo * pfx,
  const char * strLimit, char * chLimit)
{
  int StartEleNdx;

  if (pfx->teDepth >= MagickMaxRecursionDepth) {
    (void) ThrowMagickException(pfx->exception, GetMagickModule(), OptionError,
        "Expression too deeply nested", "(depth %i exceeds limit %i)",
        pfx->teDepth, MagickMaxRecursionDepth);
    return MagickFalse;
  }

  pfx->teDepth++;

  *chLimit = '\0';

  StartEleNdx = pfx->usedElements-1;
  if (StartEleNdx < 0) StartEleNdx = 0;

  SkipSpaces (pfx);

  if (!*pfx->pex) {
    pfx->teDepth--;
    return MagickFalse;
  }

  if (strchr(strLimit,*pfx->pex)!=NULL) {
    *chLimit = *pfx->pex;
    pfx->pex++;
    pfx->teDepth--;

    return MagickFalse;
  }
  return(MagickTrue);
}

static MagickBooleanType FinishExpression (FxInfo * pfx, TernaryT ternary,
  MagickBooleanType * needPopAll)
{
  if (ternary.addr_query != NULL_ADDRESS) *needPopAll = MagickTrue;

  (void) ResolveTernaryAddresses (pfx, &ternary);

  pfx->teDepth--;

  if (!pfx->teDepth && *needPopAll) {
    (void) AddAddressingElement (pfx, rZerStk, NULL_ADDRESS);
    *needPopAll = MagickFalse;
  }

  if (pfx->exception->severity >= ErrorException)
    return MagickFalse;

  return MagickTrue;
}

static MagickBooleanType TranslateExpression (
  FxInfo * pfx, const char * strLimit, char * chLimit, MagickBooleanType * needPopAll)
{
  /* There should be only one New per expression (oAssign), but can be many Old.
  */
  MagickBooleanType UserSymbol, NewUserSymbol;
  int UserSymNdx0;

  MagickBooleanType
    Assign = MagickFalse,
    Update = MagickFalse;

  const OperandSymbols
    operand0 = { &UserSymbol, &NewUserSymbol, &UserSymNdx0, needPopAll };

  ExpressionContext context;

  TernaryT ternary;

  const ExpressionTargets
    targets = { &Assign, &Update, &UserSymNdx0, &ternary, &UserSymbol,
      &NewUserSymbol };

  ternary.addr_query = NULL_ADDRESS;
  ternary.addr_colon = NULL_ADDRESS;

  if (BeginExpression (pfx, strLimit, chLimit) == MagickFalse)
    return MagickFalse;

  if (!GetOperand (pfx, &operand0)) return MagickFalse;
  SkipSpaces (pfx);

  /* Loop through Operator, Operand, Operator, Operand, ...
  */
  context.pfx=pfx;
  context.chLimit=chLimit;
  context.needPopAll=needPopAll;
  if (TranslateOperatorsAndOperands (&context, strLimit, &targets) ==
      MagickFalse)
    return MagickFalse;

  if ((IsUnassignedUserSymbolRead(UserSymbol,Assign,Update,UserSymNdx0) !=
      MagickFalse) && (!AddFinalCopyFrom (pfx, NewUserSymbol, UserSymNdx0)))
    return MagickFalse;

  if (IsPendingLimitChar(pfx,chLimit,strLimit) != MagickFalse) {
    *chLimit = *pfx->pex;
    pfx->pex++;
  }
  if (FlushOperatorStack (pfx, Assign, Update, UserSymNdx0) == MagickFalse)
    return MagickFalse;

  return(FinishExpression (pfx, ternary, needPopAll));
}


static MagickBooleanType TranslateStatement (FxInfo * pfx, char * strLimit, char * chLimit)
{
  MagickBooleanType NeedPopAll = MagickFalse;

  SkipSpaces (pfx);

  if (!*pfx->pex) return MagickFalse;

  if (!TranslateExpression (pfx, strLimit, chLimit, &NeedPopAll)) {
    return MagickFalse;
  }
  if (pfx->usedElements && *chLimit==';') {
    /* FIXME: not necessarily the last element,
       but the last _executed_ element, eg "goto" in a "for()".,
       Pending a fix, we will use rZerStk.
    */
    ElementT * pel = &pfx->Elements[pfx->usedElements-1];
    if (pel->do_push) pel->do_push = MagickFalse;
  }

  return MagickTrue;
}

static MagickBooleanType TranslateStatementList (FxInfo * pfx, const char * strLimit, char * chLimit)
{
#define MAX_SLIMIT 10
  char sLimits[MAX_SLIMIT];
  SkipSpaces (pfx);

  if (!*pfx->pex) return MagickFalse;
  (void) CopyMagickString (sLimits, strLimit, MAX_SLIMIT-1);

  if (strchr(strLimit,';')==NULL)
    (void) ConcatenateMagickString (sLimits, ";", MAX_SLIMIT);

  for (;;) {
    if (!TranslateStatement (pfx, sLimits, chLimit)) return MagickFalse;

    if (!*pfx->pex) break;

    if (*chLimit != ';') {
      break;
    }
  }

  if (pfx->exception->severity >= ErrorException)
    return MagickFalse;

  return MagickTrue;
}

static MagickBooleanType CollectStatistics (FxInfo * pfx)
{
  Image * img = GetFirstImageInList (pfx->image);

  size_t imgNum=0;

  pfx->statistics = (ChannelStatistics**) AcquireMagickMemory ((size_t) pfx->ImgListLen * sizeof (ChannelStatistics *));
  if (!pfx->statistics) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), ResourceLimitFatalError,
      "Statistics", "%lu",
      (unsigned long) pfx->ImgListLen);
    return MagickFalse;
  }

  for (;;) {
    pfx->statistics[imgNum] = CollectOneImgStats (pfx, img);

    if (++imgNum == pfx->ImgListLen) break;
    img = GetNextImageInList (img);
    assert (img != (Image *) NULL);
  }
  pfx->GotStats = MagickTrue;

  return MagickTrue;
}


static inline MagickBooleanType IsStatisticsPending(const FxInfo *pfx)
{
  return((MagickBooleanType) (pfx->NeedStats && pfx->runType == rtEntireImage &&
    !pfx->statistics));
}

static void DestroyFxRuntimesBelow (FxInfo * pfx, ssize_t t)
{
  ssize_t t2;
  for (t2 = t-1; t2 >= 0; t2--) {
    DestroyFxRt (&pfx->fxrts[t2]);
  }
}

static FxInfo *AllocateFxRuntimes (FxInfo * pfx)
{
  size_t number_threads=(size_t) GetMagickResourceLimit(ThreadResource);
  ssize_t t;

  pfx->fxrts = (fxRtT *)AcquireQuantumMemory (number_threads, sizeof(fxRtT));
  if (!pfx->fxrts) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), ResourceLimitFatalError,
      "fxrts", "%lu",
      (unsigned long) number_threads);
    (void) DestroyRPN (pfx);
    pfx->expression = DestroyString (pfx->expression);
    pfx->pex = NULL;
    (void) DeInitFx (pfx);
    pfx = (FxInfo*) RelinquishMagickMemory(pfx);
    return NULL;
  }
  for (t=0; t < (ssize_t) number_threads; t++) {
    if (!AllocFxRt (pfx, &pfx->fxrts[t])) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), ResourceLimitFatalError,
        "AllocFxRt t=", "%g",
        (double) t);
      DestroyFxRuntimesBelow (pfx, t);
      pfx->fxrts = (fxRtT *) RelinquishMagickMemory (pfx->fxrts);
      (void) DestroyRPN (pfx);
      pfx->expression = DestroyString (pfx->expression);
      pfx->pex = NULL;
      (void) DeInitFx (pfx);
      pfx = (FxInfo*) RelinquishMagickMemory(pfx);
      return NULL;
    }
  }
  return pfx;
}

static FxInfo *DestroyFxInfoOnFailure (FxInfo * pfx)
{
  (void) DestroyRPN (pfx);
  pfx->expression = DestroyString (pfx->expression);
  pfx->pex = NULL;
  (void) DeInitFx (pfx);
  pfx = (FxInfo*) RelinquishMagickMemory(pfx);
  return NULL;
}

static MagickBooleanType TranslateFxExpression (FxInfo * pfx)
{
  char chLimit;

  pfx->teDepth = 0;
  if (!TranslateStatementList (pfx, ";", &chLimit)) {
    return(MagickFalse);
  }

  if (pfx->teDepth) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Translate expression depth", "(%i) not 0",
      pfx->teDepth);

    return(MagickFalse);
  }

  if (chLimit != '\0' && chLimit != ';') {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "AcquireFxInfo: TranslateExpression did not exhaust input", "(chLimit=%i) at'%s'",
      (int)chLimit, pfx->pex);

    return(MagickFalse);
  }

  if (IsStatisticsPending(pfx) != MagickFalse) {
    if (!CollectStatistics (pfx)) {
      return(MagickFalse);
    }
  }
  return(MagickTrue);
}

static FxInfo *AcquireFxInfoPrivate (const Image * images, const char * expression,
  MagickBooleanType CalcAllStats, ExceptionInfo *exception)
{
  FxInfo * pfx = (FxInfo*) AcquireCriticalMemory (sizeof (*pfx));

  memset (pfx, 0, sizeof (*pfx));

  if (!InitFx (pfx, images, CalcAllStats, exception)) {
    pfx = (FxInfo*) RelinquishMagickMemory(pfx);
    return NULL;
  }

  if (!BuildRPN (pfx)) {
    (void) DeInitFx (pfx);
    pfx = (FxInfo*) RelinquishMagickMemory(pfx);
    return((FxInfo *) NULL);
  }

  if ((*expression == '@') && (strlen(expression) > 1))
    pfx->expression=FileToString(expression,~0UL,exception);
  if (pfx->expression == (char *) NULL)
    pfx->expression=ConstantString(expression);
  pfx->pex = (char *) pfx->expression;

  if (TranslateFxExpression (pfx) == MagickFalse)
    return (DestroyFxInfoOnFailure (pfx));

  if (pfx->DebugOpt) {
    DumpTables (stderr);
    DumpUserSymbols (pfx, stderr);
    (void) DumpRPN (pfx, stderr);
  }

  return (AllocateFxRuntimes (pfx));
}

static void DestroyFxRuntimes (FxInfo * pfx)
{
  ssize_t t;

  for (t=0; t < (ssize_t) GetMagickResourceLimit(ThreadResource); t++) {
    DestroyFxRt (&pfx->fxrts[t]);
  }
}

typedef struct _FxPixelInputs
{
  const Image
    *image;

  Image
    *fx_image;

  FxInfo
    *pfx;

  int
    id;

  ssize_t
    x,
    y;
} FxPixelInputs;

static MagickBooleanType ApplyFxToPixel (const FxPixelInputs * in,
  const Quantum *magick_restrict p, Quantum *magick_restrict q)
{
  const Image
    *const image = in->image;

  Image
    *const fx_image = in->fx_image;

  FxInfo
    *const pfx = in->pfx;

  const int
    id = in->id;

  const ssize_t
    x = in->x,
    y = in->y;

  MagickBooleanType
    status;

  ssize_t
    i;

  fxFltType
    result = 0.0;

  status=MagickTrue;
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel (image, i);
    PixelTrait traits = GetPixelChannelTraits (image, channel);
    PixelTrait fx_traits = GetPixelChannelTraits (fx_image, channel);
    const fxPixelPosT pos = { channel, x, y };
    if ((traits == UndefinedPixelTrait) ||
        (fx_traits == UndefinedPixelTrait))
      continue;
    if ((fx_traits & CopyPixelTrait) != 0) {
        SetPixelChannel (fx_image, channel, p[i], q);
        continue;
    }

    if (!ExecuteRPN (pfx, &pfx->fxrts[id], &result, &pos)) {
      status=MagickFalse;
      break;
    }

    q[i] = ClampToQuantum ((MagickRealType) (QuantumRange*result));
  }
  return(status);
}

typedef struct _FxRowInputs
{
  const Image
    *image;

  Image
    *fx_image;

  CacheView
    *image_view,
    *fx_view;

  FxInfo
    *pfx;

  int
    id;
} FxRowInputs;

static MagickBooleanType ApplyFxToRow (const FxRowInputs * row,
  const ssize_t y, MagickBooleanType *status)
{
  const Image
    *const image = row->image;

  Image
    *const fx_image = row->fx_image;

  FxInfo
    *const pfx = row->pfx;

  const int
    id = row->id;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p = GetCacheViewVirtualPixels (row->image_view, 0, y, image->columns, 1, pfx->exception);
  q = QueueCacheViewAuthenticPixels (row->fx_view, 0, y, fx_image->columns, 1, pfx->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL)) {
      (*status)=MagickFalse;
      return(MagickFalse);
  }
  for (x=0; x < (ssize_t) fx_image->columns; x++) {
    FxPixelInputs
      pixel_in;

    pfx->fxrts[id].thisPixel = (Quantum *)p;

    pixel_in.image=image;
    pixel_in.fx_image=fx_image;
    pixel_in.pfx=pfx;
    pixel_in.id=id;
    pixel_in.x=x;
    pixel_in.y=y;
    if (ApplyFxToPixel (&pixel_in, p, q) == MagickFalse)
      (*status)=MagickFalse;
    p+=(ptrdiff_t) GetPixelChannels (image);
    q+=(ptrdiff_t) GetPixelChannels (fx_image);
  }
  if (SyncCacheViewAuthenticPixels(row->fx_view, pfx->exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static void DumpUserSymbolValues (FxInfo * pfx)
{
  int t, i;
  char UserSym[MagickPathExtent];
  fprintf (stderr, "User symbols (%i):\n", pfx->usedUserSymbols);
  for (t=0; t < (int) GetMagickResourceLimit(ThreadResource); t++) {
    for (i = 0; i < (int) pfx->usedUserSymbols; i++) {
      fprintf (stderr, "th=%i us=%i '%s': %.*Lg\n",
               t, i, NameOfUserSym (pfx, i, UserSym), pfx->precision, pfx->fxrts[t].UserSymVals[i]);
    }
  }
}

#endif
