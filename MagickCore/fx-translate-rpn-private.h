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

  MagickCore private methods of fx.c: its defines, typedefs and tables, the user
  symbols, the RPN builder and dumper and the tokenizer, from MaxTokenLen to
  PeekChar; included by that file only.
*/
#ifndef MAGICKCORE_FX_TRANSLATE_RPN_PRIVATE_H
#define MAGICKCORE_FX_TRANSLATE_RPN_PRIVATE_H

#define MaxTokenLen 100
#define RpnInit 100
#define TableExtend 0.1
#define InitNumOprStack 50
#define MinValStackSize 100
#define InitNumUserSymbols 50

#define SECONDS_ERR -FLT_MAX

typedef enum {
  cEpsilon,
  cE,
  cOpaque,
  cPhi,
  cPi,
  cQuantumRange,
  cQuantumScale,
  cTransparent,
  cMaxRgb,
  cNull
} ConstantE;

typedef struct {
  ConstantE
    cons;

  fxFltType
    val;

  const char
    *str;
} ConstantT;

static const ConstantT Constants[] = {
  {cEpsilon,      MagickEpsilon,         "epsilon"},
  {cE,            2.7182818284590452354, "e"},
  {cOpaque,       1.0,                   "opaque"},
  {cPhi,          MagickPHI,             "phi"},
  {cPi,           MagickPI,              "pi"},
  {cQuantumRange, QuantumRange,          "quantumrange"},
  {cQuantumScale, QuantumScale,          "quantumscale"},
  {cTransparent,  0.0,                   "transparent"},
  {cMaxRgb,       QuantumRange,          "MaxRGB"},
  {cNull,         0.0,                   "cnull"}
};

#define NULL_ADDRESS -2

typedef struct {
  int
    addr_query,
    addr_colon;
} TernaryT;

typedef struct {
  const char
    *str;

  PixelChannel
    pixel_channel;
} ChannelT;

static const ChannelT Channels[] = {
  {"r",          RedPixelChannel},
  {"g",          GreenPixelChannel},
  {"b",          BluePixelChannel},
  {"c",          CyanPixelChannel},
  {"m",          MagentaPixelChannel},
  {"y",          YellowPixelChannel},
  {"k",          BlackPixelChannel},
  {"a",          AlphaPixelChannel},
  {"o",          AlphaPixelChannel},
  {"hue",        HUE_CHANNEL},
  {"saturation", SAT_CHANNEL},
  {"lightness",  LIGHT_CHANNEL},
  {"intensity",  INTENSITY_CHANNEL},
  {"all",        CompositePixelChannel},
  {"this",       THIS_CHANNEL},
  {"",           NO_CHAN_QUAL}
};

static const char * sElementTypes[] = {
  "Operator",
  "Constant",
  "Function",
  "ImgAttr",
  "Symbol",
  "ColConst",
  "Control"
};

/* Forward declarations for recursion.
*/
static MagickBooleanType TranslateStatementList
  (FxInfo * pfx, const char * strLimit, char * chLimit);

static MagickBooleanType TranslateExpression
  (FxInfo * pfx, const char * strLimit, char * chLimit, MagickBooleanType * needPopAll);

static MagickBooleanType GetFunction (FxInfo * pfx, FunctionE fe);

typedef struct _OperandSymbols
{
  MagickBooleanType
    *UserSymbol,
    *NewUserSymbol;

  int
    *UserSymNdx;

  MagickBooleanType
    *needPopAll;
} OperandSymbols;

static MagickBooleanType GetOperand (
  FxInfo * pfx, const OperandSymbols * operand);

static inline MagickBooleanType IsHslOrIntensityChannel(const PixelChannel pc)
{
  return((MagickBooleanType) (pc==HUE_CHANNEL || pc==SAT_CHANNEL ||
    pc==LIGHT_CHANNEL || pc==INTENSITY_CHANNEL));
}

static inline MagickBooleanType ChanIsVirtual (PixelChannel pc)
{
  if (IsHslOrIntensityChannel(pc) != MagickFalse)
    return MagickTrue;

  return MagickFalse;
}

static void DestroyImgViewsBelow (FxInfo * pfx, ssize_t i)
{
  ImgT * pimg;

      for ( ; i > 0; i--) {
        pimg = &pfx->Imgs[i-1];
        pimg->View = DestroyCacheView (pimg->View);
      }
}

static MagickBooleanType InitFx (FxInfo * pfx, const Image * img,
  MagickBooleanType CalcAllStats, ExceptionInfo *exception)
{
  ssize_t i=0;
  const Image * next;

  pfx->ImgListLen = GetImageListLength (img);
  pfx->ImgNum = GetImageIndexInList (img);
  pfx->image = (Image *)img;

  pfx->NeedStats = MagickFalse;
  pfx->GotStats = MagickFalse;
  pfx->NeedHsl = MagickFalse;
  pfx->DebugOpt = IsStringTrue (GetImageArtifact (img, "fx:debug"));
  pfx->statistics = NULL;
  pfx->Imgs = NULL;
  pfx->Images = NULL;
  pfx->exception = exception;
  pfx->precision = GetMagickPrecision ();
  pfx->random_infos = AcquireRandomInfoTLS ();
  pfx->ContainsDebug = MagickFalse;
  pfx->runType = (CalcAllStats) ? rtEntireImage : rtCornerOnly;
  pfx->Imgs = (ImgT *)AcquireQuantumMemory (pfx->ImgListLen, sizeof (ImgT));
  if (!pfx->Imgs) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), ResourceLimitFatalError,
      "Imgs", "%lu",
      (unsigned long) pfx->ImgListLen);
    return MagickFalse;
  }

  next = GetFirstImageInList (img);
  for ( ; next != (Image *) NULL; next=next->next)
  {
    ImgT * pimg = &pfx->Imgs[i];
    pimg->View = AcquireVirtualCacheView (next, pfx->exception);
    if (!pimg->View) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), ResourceLimitFatalError,
        "View", "[%li]",
        (long) i);
      /* dealloc any done so far, and Imgs */
      DestroyImgViewsBelow (pfx, i);
      pfx->Imgs=(ImgT *) RelinquishMagickMemory (pfx->Imgs);
      return MagickFalse;
    }
    i++;
  }

  pfx->Images = ImageListToArray (img, pfx->exception);

  return MagickTrue;
}

static void DestroyImgViews (FxInfo * pfx)
{
  ssize_t i;

  for (i = (ssize_t)GetImageListLength(pfx->image); i > 0; i--) {
    ImgT * pimg = &pfx->Imgs[i-1];
    pimg->View = DestroyCacheView (pimg->View);
  }
  pfx->Imgs=(ImgT *) RelinquishMagickMemory (pfx->Imgs);
}

static void DestroyFxStatistics (FxInfo * pfx)
{
  ssize_t i;

    for (i = (ssize_t)GetImageListLength(pfx->image); i > 0; i--) {
      pfx->statistics[i-1]=(ChannelStatistics *) RelinquishMagickMemory (pfx->statistics[i-1]);
    }

    pfx->statistics = (ChannelStatistics**) RelinquishMagickMemory(pfx->statistics);
}

static MagickBooleanType DeInitFx (FxInfo * pfx)
{
  if (pfx->Images) pfx->Images = (Image**) RelinquishMagickMemory (pfx->Images);

  if (pfx->Imgs) {
    DestroyImgViews (pfx);
  }
  pfx->random_infos = DestroyRandomInfoTLS (pfx->random_infos);

  if (pfx->statistics) {
    DestroyFxStatistics (pfx);
  }

  return MagickTrue;
}

static ElementTypeE TypeOfOpr (int op)
{
  if (op <  oNull) return etOperator;
  if (op == oNull) return etConstant;
  if (op <= fNull) return etFunction;
  if (op <= aNull) return etImgAttr;
  if (op <= sNull) return etSymbol;
  if (op <= rNull) return etControl;

  return (ElementTypeE) 0;
}

static char * SetShortExp (FxInfo * pfx)
{
  return SetPtrShortExp (pfx, pfx->pex, MaxTokenLen-1);
}

static int FindUserSymbol (FxInfo * pfx, char * name)
/* returns index into pfx->UserSymbols, and thus into pfxrt->UserSymVals,
   or NULL_ADDRESS if not found.
*/
{
  int i;
  size_t lenName;
  lenName = strlen (name);
  for (i=0; i < pfx->usedUserSymbols; i++) {
    UserSymbolT *pus = &pfx->UserSymbols[i];
    if (lenName == pus->len && LocaleNCompare (name, pus->pex, lenName)==0) break;
  }
  if (i == pfx->usedUserSymbols) return NULL_ADDRESS;
  return i;
}

static MagickBooleanType ExtendUserSymbols (FxInfo * pfx)
{
  pfx->numUserSymbols = (int) ceil (pfx->numUserSymbols * (1 + TableExtend));
  pfx->UserSymbols = (UserSymbolT*) ResizeMagickMemory (pfx->UserSymbols, (size_t) pfx->numUserSymbols * sizeof(UserSymbolT));
  if (!pfx->UserSymbols) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), ResourceLimitFatalError,
      "UserSymbols", "%i",
      pfx->numUserSymbols);
    return MagickFalse;
  }

  return MagickTrue;
}

static int AddUserSymbol (FxInfo * pfx, char * pex, size_t len)
{
  UserSymbolT *pus;
  if (++pfx->usedUserSymbols >= pfx->numUserSymbols) {
    if (!ExtendUserSymbols (pfx)) return -1;
  }
  pus = &pfx->UserSymbols[pfx->usedUserSymbols-1];
  pus->pex = pex;
  pus->len = len;

  return pfx->usedUserSymbols-1;
}

static const char * NameOfAttrSymbolOrControl (int i, const char * str)
{
  if (i >= (int) FirstImgAttr && i < aNull) str = ImgAttrs[i-(int) FirstImgAttr].str;
  if (i >= (int) FirstSym     && i < sNull) str = Symbols[i-(int) FirstSym].str;
  if (i >= (int) FirstCont    && i < rNull) str = Controls[i-(int) FirstCont].str;
  return(str);
}

static void PrintTableHeading (int i)
{
  if      (i==0    ) fprintf (stderr, "Operators:\n ");
  else if (i==oNull) fprintf (stderr, "\nFunctions:\n ");
  else if (i==fNull) fprintf (stderr, "\nImage attributes:\n ");
  else if (i==aNull) fprintf (stderr, "\nSymbols:\n ");
  else if (i==sNull) fprintf (stderr, "\nControls:\n ");
}

static void DumpTables (FILE * fh)
{

  int i;
  for (i=0; i <= rNull; i++) {
    const char * str = "";
    if (                     i < oNull) str = Operators[i].str;
    if (i >= (int) FirstFunc    && i < fNull) str = Functions[i-(int) FirstFunc].str;
    str = NameOfAttrSymbolOrControl (i, str);
    PrintTableHeading (i);
    fprintf (fh, " %s", str);
  }
  fprintf (fh, "\n");
}

static char * NameOfUserSym (FxInfo * pfx, int ndx, char * buf)
{
  UserSymbolT * pus;
  assert (ndx >= 0 && ndx < pfx->usedUserSymbols);
  pus = &pfx->UserSymbols[ndx];
  (void) CopyMagickString (buf, pus->pex, pus->len+1);
  return buf;
}

static void DumpUserSymbols (FxInfo * pfx, FILE * fh)
{
  char UserSym[MagickPathExtent];
  int i;
  fprintf (fh, "UserSymbols (%i)\n", pfx->usedUserSymbols);
  for (i=0; i < pfx->usedUserSymbols; i++) {
    fprintf (fh, "  %i: '%s'\n", i, NameOfUserSym (pfx, i, UserSym));
  }
}

static MagickBooleanType BuildRPN (FxInfo * pfx)
{
  pfx->numUserSymbols = InitNumUserSymbols;
  pfx->usedUserSymbols = 0;
  pfx->UserSymbols = (UserSymbolT*) AcquireMagickMemory ((size_t) pfx->numUserSymbols * sizeof(UserSymbolT));
  if (!pfx->UserSymbols) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), ResourceLimitFatalError,
      "UserSymbols", "%i",
      pfx->numUserSymbols);
    return MagickFalse;
  }

  pfx->numElements = RpnInit;
  pfx->usedElements = 0;
  pfx->Elements = NULL;

  pfx->Elements = (ElementT*) AcquireMagickMemory ((size_t) pfx->numElements * sizeof(ElementT));

  if (!pfx->Elements) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), ResourceLimitFatalError,
      "Elements", "%i",
      pfx->numElements);
    return MagickFalse;
  }

  pfx->usedOprStack = 0;
  pfx->maxUsedOprStack = 0;
  pfx->numOprStack = InitNumOprStack;
  pfx->OperatorStack = (OperatorE*) AcquireMagickMemory ((size_t) pfx->numOprStack * sizeof(OperatorE));
  if (!pfx->OperatorStack) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), ResourceLimitFatalError,
      "OperatorStack", "%i",
      pfx->numOprStack);
    return MagickFalse;
  }

  return MagickTrue;
}

static MagickBooleanType AllocFxRt (FxInfo * pfx, fxRtT * pfxrt)
{
  int nRnd;
  int i;
  pfxrt->random_info = AcquireRandomInfo ();
  pfxrt->thisPixel = NULL;

  nRnd = 20 + 10 * (int) GetPseudoRandomValue (pfxrt->random_info);
  for (i=0; i < nRnd; i++) (void) GetPseudoRandomValue (pfxrt->random_info);;

  pfxrt->usedValStack = 0;
  pfxrt->numValStack = 2 * pfx->maxUsedOprStack;
  if (pfxrt->numValStack < MinValStackSize) pfxrt->numValStack = MinValStackSize;
  pfxrt->ValStack = (fxFltType*) AcquireMagickMemory ((size_t) pfxrt->numValStack * sizeof(fxFltType));
  if (!pfxrt->ValStack) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), ResourceLimitFatalError,
      "ValStack", "%i",
      pfxrt->numValStack);
    return MagickFalse;
  }

  pfxrt->UserSymVals = NULL;

  if (pfx->usedUserSymbols) {
    pfxrt->UserSymVals = (fxFltType*) AcquireMagickMemory ((size_t) pfx->usedUserSymbols * sizeof(fxFltType));
    if (!pfxrt->UserSymVals) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), ResourceLimitFatalError,
        "UserSymVals", "%i",
        pfx->usedUserSymbols);
      return MagickFalse;
    }
    for (i = 0; i < pfx->usedUserSymbols; i++) pfxrt->UserSymVals[i] = (fxFltType) 0;
  }

  return MagickTrue;
}

static MagickBooleanType ExtendRPN(FxInfo * pfx)
{
  ElementT
    *new_elements;

  int
    element_count;

  /*
    Keep the original table and size intact unless growth succeeds.
  */
  element_count=(int) ceil(pfx->numElements*(1+TableExtend));
  new_elements=(ElementT *) AcquireQuantumMemory((size_t) element_count,
    sizeof(*new_elements));
  if (new_elements == (ElementT *) NULL)
    {
      (void) ThrowMagickException(pfx->exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","RPN table");
      return(MagickFalse);
    }
  (void) memcpy(new_elements,pfx->Elements,(size_t) pfx->usedElements*
    sizeof(*new_elements));
   pfx->Elements=(ElementT *) RelinquishMagickMemory(pfx->Elements);
   pfx->Elements=new_elements;
   pfx->numElements=element_count;
   if (!pfx->Elements)
     {
       (void) ThrowMagickException(pfx->exception,GetMagickModule(),
         ResourceLimitFatalError,"Elements","%i",pfx->numElements);
       return(MagickFalse);
     }
  return(MagickTrue);
}

static inline MagickBooleanType OprInPlace(int op)
{
  return((MagickBooleanType) (op >= oAddEq && op <= oSubSub));
}

static inline MagickBooleanType IsJumpElement(const ElementT *pel)
{
  return((MagickBooleanType) (pel->operator_index == rGoto ||
    pel->operator_index == rGotoChk || pel->operator_index == rIfZeroGoto ||
    pel->operator_index == rIfNotZeroGoto));
}

static inline MagickBooleanType IsPixelReferenceElement(const ElementT *pel)
{
  return((MagickBooleanType) (pel->operator_index == fP ||
    pel->operator_index == fUP || pel->operator_index == fVP ||
    pel->operator_index == fSP));
}

static void CountJumpDestination (FxInfo * pfx, int i)
{
  ElementT * pel = &pfx->Elements[i];
  if (IsJumpElement(pel) != MagickFalse) {
    if (pel->element_index >= 0 && pel->element_index < pfx->numElements) {
      ElementT * pelDest = &pfx->Elements[pel->element_index];
      pelDest->number_dest++;
    }
  }
}

static void DumpChannelQualifier (ElementT * pel)
{
  if (pel->channel_qual != NO_CHAN_QUAL) {
    if (pel->channel_qual == THIS_CHANNEL) fprintf (stderr, "  ch=this");
    else fprintf (stderr, "  ch=%i", pel->channel_qual);
  }
}

static void DumpRpnHeader (FxInfo * pfx, FILE * fh)
{
  fprintf (fh, "DumpRPN:");
  fprintf (fh, "  numElements=%i", pfx->numElements);
  fprintf (fh, "  usedElements=%i", pfx->usedElements);
  fprintf (fh, "  maxUsedOprStack=%i", pfx->maxUsedOprStack);
  fprintf (fh, "  ImgListLen=%g", (double) pfx->ImgListLen);
  fprintf (fh, "  NeedStats=%s", pfx->NeedStats ? "yes" : "no");
  fprintf (fh, "  GotStats=%s", pfx->GotStats ? "yes" : "no");
  fprintf (fh, "  NeedHsl=%s\n", pfx->NeedHsl ? "yes" : "no");
  if      (pfx->runType==rtEntireImage) fprintf (stderr, "EntireImage");
  else if (pfx->runType==rtCornerOnly)  fprintf (stderr, "CornerOnly");
  fprintf (fh, "\n");
}

static void DumpRpnElement (FxInfo * pfx, FILE * fh, ElementT * pel, int i)
{
  const char * str = OprStr (pel->operator_index);
  const char *sRelAbs = "";

  if (IsPixelReferenceElement(pel) != MagickFalse)
    sRelAbs = pel->is_relative ? "[]" : "{}";

  if (pel->type == etColourConstant)
    fprintf (fh, "  %i: %s vals=%.*Lg,%.*Lg,%.*Lg '%s%s' nArgs=%i ndx=%i  %s",
             i, sElementTypes[pel->type],
             pfx->precision, pel->val, pfx->precision, pel->val1, pfx->precision, pel->val2,
             str, sRelAbs, pel->number_args, pel->element_index,
             pel->do_push ? "push" : "NO push");
  else
    fprintf (fh, "  %i: %s val=%.*Lg '%s%s' nArgs=%i ndx=%i  %s",
             i, sElementTypes[pel->type], pfx->precision, pel->val, str, sRelAbs,
             pel->number_args, pel->element_index,
             pel->do_push ? "push" : "NO push");
}

static void DumpRpnUserSymbolLink (FxInfo * pfx, FILE * fh, ElementT * pel)
{
  char UserSym[MagickPathExtent];

  if (pel->operator_index == rCopyTo) {
    fprintf (fh, "  CopyTo ==> %s", NameOfUserSym (pfx, pel->element_index, UserSym));
  } else if (pel->operator_index == rCopyFrom) {
    fprintf (fh, "  CopyFrom <== %s", NameOfUserSym (pfx, pel->element_index, UserSym));
  } else if (OprInPlace (pel->operator_index)) {
    fprintf (fh, "  <==> %s", NameOfUserSym (pfx, pel->element_index, UserSym));
  }
}

static MagickBooleanType DumpRPN (FxInfo * pfx, FILE * fh)
{
  int i;

  DumpRpnHeader (pfx, fh);


  for (i=0; i < pfx->usedElements; i++) {
    ElementT * pel = &pfx->Elements[i];
    pel->number_dest = 0;
  }
  for (i=0; i < pfx->usedElements; i++) {
    CountJumpDestination (pfx, i);
  }
  for (i=0; i < pfx->usedElements; i++) {
    ElementT * pel = &pfx->Elements[i];
    DumpRpnElement (pfx, fh, pel, i);

    if (pel->img_attr_qual != aNull)
      fprintf (fh, " ia=%s", OprStr((int) pel->img_attr_qual));

    DumpChannelQualifier (pel);

    DumpRpnUserSymbolLink (pfx, fh, pel);
    if (pel->number_dest > 0)  fprintf (fh, "  <==dest(%i)", pel->number_dest);
    fprintf (fh, "\n");
  }
  return MagickTrue;
}

static void DestroyRPN (FxInfo * pfx)
{
  pfx->numOprStack = 0;
  pfx->usedOprStack = 0;
  if (pfx->OperatorStack) pfx->OperatorStack = (OperatorE*) RelinquishMagickMemory (pfx->OperatorStack);

  pfx->numElements = 0;
  pfx->usedElements = 0;
  if (pfx->Elements) pfx->Elements = (ElementT*) RelinquishMagickMemory (pfx->Elements);

  pfx->usedUserSymbols = 0;
  if (pfx->UserSymbols) pfx->UserSymbols = (UserSymbolT*) RelinquishMagickMemory (pfx->UserSymbols);
}

static void DestroyFxRt (fxRtT * pfxrt)
{
  pfxrt->usedValStack = 0;
  if (pfxrt->ValStack) pfxrt->ValStack = (fxFltType*) RelinquishMagickMemory (pfxrt->ValStack);
  if (pfxrt->UserSymVals) pfxrt->UserSymVals = (fxFltType*) RelinquishMagickMemory (pfxrt->UserSymVals);

  pfxrt->random_info = DestroyRandomInfo (pfxrt->random_info);
}

static size_t ScanPlainTokenLength (char * p, size_t len)
{
  while (isalpha ((int)*p)) { len++; p++; }
  if (*p == '_')            { len++; p++; }
  while (isalpha ((int)*p)) { len++; p++; }
  while (isdigit ((int)*p)) { len++; p++; }
  return(len);
}

static size_t ScanNonIccTokenLength (char * p, size_t len)
{
  if (LocaleNCompare (p, "device-", 7) == 0) {
    len = 7;
    p += 7;
    while (isalpha ((int)*p)) { len++; p++; }
  } else {
    len = ScanPlainTokenLength (p, len);
  }
  return(len);
}

static size_t LimitTokenLength (FxInfo * pfx, size_t len)
{
  if (len >= MaxTokenLen) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "GetToken: too long", "%g at '%s'",
      (double) len, SetShortExp(pfx));
    len = MaxTokenLen;
  }
  return(len);
}

static void CopyToken (FxInfo * pfx, size_t len)
{
  if (len) {
    (void) CopyMagickString (pfx->token, pfx->pex, (len+1<MaxTokenLen)?len+1:MaxTokenLen);
  }
}

static size_t GetToken (FxInfo * pfx)
/* Returns length of token that starts with an alpha,
     or 0 if it isn't a token that starts with an alpha.
   j0 and j1 have trailing digit.
   Also colours like "gray47" have more trailing digits.
   After initial alpha(s) also allow single "_", eg "standard_deviation".
   Does not advance pfx->pex.
   This splits "mean.r" etc.
*/
{

  char * p = pfx->pex;
  size_t len = 0;
  *pfx->token = '\0';
  pfx->lenToken = 0;
  if (!isalpha((int)*p)) return 0;

  /* Regard strings that start "icc-" or "device-",
     followed by any number of alphas,
     as a token.
  */

  if (LocaleNCompare (p, "icc-", 4) == 0) {
    len = 4;
    p += 4;
    while (isalpha ((int)*p)) { len++; p++; }
  } else {
    len = ScanNonIccTokenLength (p, len);
  }
  len = LimitTokenLength (pfx, len);
  CopyToken (pfx, len);

  pfx->lenToken = strlen (pfx->token);
  return len;
}

static MagickBooleanType TokenMaybeUserSymbol (FxInfo * pfx)
{
  char * p = pfx->token;
  int i = 0;
  while (*p) {
    if (!isalpha ((int)*p++)) return MagickFalse;
    i++;
  }
  if (i < 2) return MagickFalse;
  return MagickTrue;
}

static void SetElementNumberArgs (ElementT * pel, int oprNum)
{
  if (oprNum <= oNull) pel->number_args = Operators[oprNum].number_args;
  else if (oprNum <= fNull) pel->number_args = Functions[oprNum-(int) FirstFunc].number_args;
  else if (oprNum <= aNull) pel->number_args = 0;
  else if (oprNum <= sNull) pel->number_args = 0;
  else                      pel->number_args = Controls[oprNum-(int) FirstCont].number_args;
}

static void InitElement (ElementT * pel, fxFltType val, int oprNum)
{
  pel->type = TypeOfOpr (oprNum);
  pel->val = val;
  pel->val1 = (fxFltType) 0;
  pel->val2 = (fxFltType) 0;
  pel->operator_index = oprNum;
  pel->do_push = MagickTrue;
  pel->element_index = 0;
  pel->channel_qual = NO_CHAN_QUAL;
  pel->img_attr_qual = aNull;
  pel->number_dest = 0;
  pel->exp_start = NULL;
  pel->exp_len = 0;
}

static MagickBooleanType AddElement(FxInfo * pfx,fxFltType val,int oprNum)
{
  ElementT * pel;

  assert (oprNum <= rNull);

  if (++pfx->usedElements >= pfx->numElements)
    {
      if (ExtendRPN(pfx) == MagickFalse)
        {
          pfx->usedElements--;
          return(MagickFalse);
        }
    }
  if (pfx->Elements == (ElementT *) NULL)
    {
      pfx->usedElements--;
      return(MagickFalse);
    }
  pel = &pfx->Elements[pfx->usedElements-1];
  InitElement (pel, val, oprNum);

  SetElementNumberArgs (pel, oprNum);

  return MagickTrue;
}

static inline MagickBooleanType IsAddressingOperator(const int oprNum)
{
  return((MagickBooleanType) (oprNum == rGoto || oprNum == rGotoChk ||
    oprNum == rIfZeroGoto || oprNum == rIfNotZeroGoto || oprNum == rZerStk));
}

static MagickBooleanType AddAddressingElement (FxInfo * pfx, int oprNum, int EleNdx)
{
  ElementT * pel;
  if (!AddElement (pfx, (fxFltType) 0, oprNum)) return MagickFalse;
  pel = &pfx->Elements[pfx->usedElements-1];
  pel->element_index = EleNdx;
  if (IsAddressingOperator(oprNum) != MagickFalse)
  {
    pel->do_push = MagickFalse;
  }

  /* Note: for() may or may not need pushing,
     depending on whether the value is needed, eg "for(...)+2" or debug(for(...)).
  */

  return MagickTrue;
}

static MagickBooleanType AddColourElement (FxInfo * pfx, fxFltType val0, fxFltType val1, fxFltType val2)
{
  ElementT * pel;
  if (!AddElement (pfx, val0, oNull)) return MagickFalse;
  pel = &pfx->Elements[pfx->usedElements-1];
  pel->val1 = val1;
  pel->val2 = val2;
  pel->type = etColourConstant;
  return MagickTrue;
}

static inline void SkipSpaces (FxInfo * pfx)
{
  while (isspace ((int)*pfx->pex)) pfx->pex++;
}

static inline char PeekChar (FxInfo * pfx)
{
  SkipSpaces (pfx);
  return *pfx->pex;
}

#endif
