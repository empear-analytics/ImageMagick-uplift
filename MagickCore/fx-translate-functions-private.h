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

  MagickCore private methods of fx.c: the completion and closing of function calls and
  the prefix, number, colour, attribute and function operands, from CompleteFunction
  to TryFunctionOperand; included by that file only.
*/
#ifndef MAGICKCORE_FX_TRANSLATE_FUNCTIONS_PRIVATE_H
#define MAGICKCORE_FX_TRANSLATE_FUNCTIONS_PRIVATE_H

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

static FunctionE FindFunctionToken (FxInfo * pfx)
{
  FunctionE fe;
  for (fe = FirstFunc; fe < fNull; fe=(FunctionE) (fe+1)) {
    const char * feStr = Functions[fe-(int) FirstFunc].str;
    if (LocaleCompare (feStr, pfx->token)==0) {
      break;
    }
  }
  return(fe);
}

static MagickBooleanType TryFunctionOperand (FxInfo * pfx,
  MagickBooleanType * needPopAll, MagickBooleanType * result)
{
  FunctionE fe;
  fe=FindFunctionToken (pfx);

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

#endif
