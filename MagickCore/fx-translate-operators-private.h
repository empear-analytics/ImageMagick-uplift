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

  MagickCore private methods of fx.c: the named operands, the operator handling, the
  ternary and assignment translation, from TryNamedOperand to SetInPlaceTarget;
  included by that file only.
*/
#ifndef MAGICKCORE_FX_TRANSLATE_OPERATORS_PRIVATE_H
#define MAGICKCORE_FX_TRANSLATE_OPERATORS_PRIVATE_H

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

#endif
