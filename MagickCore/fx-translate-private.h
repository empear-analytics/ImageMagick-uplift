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
