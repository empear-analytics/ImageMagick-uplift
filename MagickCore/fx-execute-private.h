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

  MagickCore private methods shared by the f/x expression methods: the
  translated program types and tables, and the run-time execution of the
  program (ExecuteRPN and its helpers), used by FxImage and by
  FxEvaluateChannelExpression.
*/
#ifndef MAGICKCORE_FX_EXECUTE_PRIVATE_H
#define MAGICKCORE_FX_EXECUTE_PRIVATE_H

#include "MagickCore/attribute.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/statistic.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/timer-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(MAGICKCORE_WINDOWS_SUPPORT)
#define __j0 _j0
#define __j1 _j1
#else
#define __j0 j0
#define __j1 j1
#endif

typedef long double fxFltType;

typedef enum {
  oAddEq,
  oSubtractEq,
  oMultiplyEq,
  oDivideEq,
  oPlusPlus,
  oSubSub,
  oAdd,
  oSubtract,
  oMultiply,
  oDivide,
  oModulus,
  oUnaryPlus,
  oUnaryMinus,
  oLshift,
  oRshift,
  oEq,
  oNotEq,
  oLtEq,
  oGtEq,
  oLt,
  oGt,
  oLogAnd,
  oLogOr,
  oLogNot,
  oBitAnd,
  oBitOr,
  oBitNot,
  oPow,
  oQuery,
  oColon,
  oOpenParen,
  oCloseParen,
  oOpenBracket,
  oCloseBracket,
  oOpenBrace,
  oCloseBrace,
  oAssign,
  oNull
} OperatorE;

typedef struct {
  OperatorE
    op;

  const char *
    str;

  int
    precedence, /* Higher number is higher precedence */
    number_args;
} OperatorT;

static const OperatorT Operators[] = {
  {oAddEq,       "+=",    12, 1},
  {oSubtractEq,  "-=",    12, 1},
  {oMultiplyEq,  "*=",    13, 1},
  {oDivideEq,    "/=",    13, 1},
  {oPlusPlus,    "++",    12, 0},
  {oSubSub,      "--",    12, 0},
  {oAdd,         "+",     12, 2},
  {oSubtract,    "-",     12, 2},
  {oMultiply,    "*",     13, 2},
  {oDivide,      "/",     13, 2},
  {oModulus,     "%",     13, 2},
  {oUnaryPlus,   "+",     14, 1},
  {oUnaryMinus,  "-",     14, 1},
  {oLshift,      "<<",    11, 2},
  {oRshift,      ">>",    11, 2},
  {oEq,          "==",     9, 2},
  {oNotEq,       "!=",     9, 2},
  {oLtEq,        "<=",    10, 2},
  {oGtEq,        ">=",    10, 2},
  {oLt,          "<",     10, 2},
  {oGt,          ">",     10, 2},
  {oLogAnd,      "&&",     6, 2},
  {oLogOr,       "||",     5, 2},
  {oLogNot,      "!",     16, 1},
  {oBitAnd,      "&",      8, 2},
  {oBitOr,       "|",      7, 2},
  {oBitNot,      "~",     16, 1},
  {oPow,         "^",     15, 2},
  {oQuery,       "?",      4, 1},
  {oColon,       ":",      4, 1},
  {oOpenParen,   "(",      0, 0},
  {oCloseParen,  ")",      0, 0},
  {oOpenBracket, "[",      0, 0},
  {oCloseBracket,"]",      0, 0},
  {oOpenBrace,   "{",      0, 0},
  {oCloseBrace,  "}",      0, 0},
  {oAssign,      "=",      3, 1},
  {oNull,        "onull",  17, 0}
};

#define FirstFunc ((FunctionE) (oNull+1))

typedef enum {
  fAbs = oNull+1,
#if defined(MAGICKCORE_HAVE_ACOSH)
  fAcosh,
#endif
  fAcos,
#if defined(MAGICKCORE_HAVE_J1)
  fAiry,
#endif
  fAlt,
#if defined(MAGICKCORE_HAVE_ASINH)
  fAsinh,
#endif
  fAsin,
#if defined(MAGICKCORE_HAVE_ATANH)
  fAtanh,
#endif
  fAtan2,
  fAtan,
  fCeil,
  fChannel,
  fClamp,
  fCosh,
  fCos,
  fDebug,
  fDrc,
#if defined(MAGICKCORE_HAVE_ERF)
  fErf,
#endif
  fEpoch,
  fExp,
  fFloor,
  fGauss,
  fGcd,
  fHypot,
  fInt,
  fIsnan,
#if defined(MAGICKCORE_HAVE_J0)
  fJ0,
#endif
#if defined(MAGICKCORE_HAVE_J1)
  fJ1,
#endif
#if defined(MAGICKCORE_HAVE_J1)
  fJinc,
#endif
  fLn,
  fLogtwo,
  fLog,
  fMagickTime,
  fMax,
  fMin,
  fMod,
  fNot,
  fPow,
  fRand,
  fRound,
  fSign,
  fSinc,
  fSinh,
  fSin,
  fSqrt,
  fSquish,
  fTanh,
  fTan,
  fTrunc,
  fDo,
  fFor,
  fIf,
  fWhile,
  fU,
  fU0,
  fUP,
  fS,
  fV,
  fP,
  fSP,
  fVP,

  fNull
} FunctionE;

typedef struct {
  FunctionE
    func;

  const char
    *str;

  int
    number_args;
} FunctionT;

static const FunctionT Functions[] = {
  {fAbs,     "abs"   , 1},
#if defined(MAGICKCORE_HAVE_ACOSH)
  {fAcosh,   "acosh" , 1},
#endif
  {fAcos,    "acos"  , 1},
#if defined(MAGICKCORE_HAVE_J1)
  {fAiry,    "airy"  , 1},
#endif
  {fAlt,     "alt"   , 1},
#if defined(MAGICKCORE_HAVE_ASINH)
  {fAsinh,   "asinh" , 1},
#endif
  {fAsin,    "asin"  , 1},
#if defined(MAGICKCORE_HAVE_ATANH)
  {fAtanh,   "atanh" , 1},
#endif
  {fAtan2,   "atan2" , 2},
  {fAtan,    "atan"  , 1},
  {fCeil,    "ceil"  , 1},
  {fChannel, "channel", 5}, /* Special case: allow zero to five arguments. */
  {fClamp,   "clamp" , 1},
  {fCosh,    "cosh"  , 1},
  {fCos,     "cos"   , 1},
  {fDebug,   "debug" , 1},
  {fDrc,     "drc"   , 2},
#if defined(MAGICKCORE_HAVE_ERF)
  {fErf,     "erf"   , 1},
#endif
  {fEpoch,   "epoch" , 1}, /* Special case: needs a string date from a property eg %[date:modify] */
  {fExp,     "exp"   , 1},
  {fFloor,   "floor" , 1},
  {fGauss,   "gauss" , 1},
  {fGcd,     "gcd"   , 2},
  {fHypot,   "hypot" , 2},
  {fInt,     "int"   , 1},
  {fIsnan,   "isnan" , 1},
#if defined(MAGICKCORE_HAVE_J0)
  {fJ0,      "j0"    , 1},
#endif
#if defined(MAGICKCORE_HAVE_J1)
  {fJ1,      "j1"    , 1},
#endif
#if defined(MAGICKCORE_HAVE_J1)
  {fJinc,    "jinc"  , 1},
#endif
  {fLn,      "ln"    , 1},
  {fLogtwo,  "logtwo", 1},
  {fLog,     "log"   , 1},
  {fMagickTime,"magicktime", 0},
  {fMax,     "max"   , 2},
  {fMin,     "min"   , 2},
  {fMod,     "mod"   , 2},
  {fNot,     "not"   , 1},
  {fPow,     "pow"   , 2},
  {fRand,    "rand"  , 0},
  {fRound,   "round" , 1},
  {fSign,    "sign"  , 1},
  {fSinc,    "sinc"  , 1},
  {fSinh,    "sinh"  , 1},
  {fSin,     "sin"   , 1},
  {fSqrt,    "sqrt"  , 1},
  {fSquish,  "squish", 1},
  {fTanh,    "tanh"  , 1},
  {fTan,     "tan"   , 1},
  {fTrunc,   "trunc" , 1},
  {fDo,      "do",     2},
  {fFor,     "for",    3},
  {fIf,      "if",     3},
  {fWhile,   "while",  2},
  {fU,       "u",      1},
  {fU0,      "u0",     0},
  {fUP,      "up",     3},
  {fS,       "s",      0},
  {fV,       "v",      0},
  {fP,       "p",      2},
  {fSP,      "sp",     2},
  {fVP,      "vp",     2},

  {fNull,    "fnull" , 0}
};

#define FirstImgAttr ((ImgAttrE) (fNull+1))

typedef enum {
  aDepth = fNull+1,
  aExtent,
  aKurtosis,
  aMaxima,
  aMean,
  aMedian,
  aMinima,
  aPage,
  aPageX,
  aPageY,
  aPageWid,
  aPageHt,
  aPrintsize,
  aPrintsizeX,
  aPrintsizeY,
  aQuality,
  aRes,
  aResX,
  aResY,
  aSkewness,
  aStdDev,
  aH,
  aN,
  aT,
  aW,
  aZ,
  aNull
} ImgAttrE;

typedef struct {
  ImgAttrE
    attr;

  const char
    *str;

  MagickBooleanType
    need_stats;
} ImgAttrT;

static const ImgAttrT ImgAttrs[] = {
  {aDepth,      "depth",              MagickTrue},
  {aExtent,     "extent",             MagickFalse},
  {aKurtosis,   "kurtosis",           MagickTrue},
  {aMaxima,     "maxima",             MagickTrue},
  {aMean,       "mean",               MagickTrue},
  {aMedian,     "median",             MagickTrue},
  {aMinima,     "minima",             MagickTrue},
  {aPage,       "page",               MagickFalse},
  {aPageX,      "page.x",             MagickFalse},
  {aPageY,      "page.y",             MagickFalse},
  {aPageWid,    "page.width",         MagickFalse},
  {aPageHt,     "page.height",        MagickFalse},
  {aPrintsize,  "printsize",          MagickFalse},
  {aPrintsizeX, "printsize.x",        MagickFalse},
  {aPrintsizeY, "printsize.y",        MagickFalse},
  {aQuality,    "quality",            MagickFalse},
  {aRes,        "resolution",         MagickFalse},
  {aResX,       "resolution.x",       MagickFalse},
  {aResY,       "resolution.y",       MagickFalse},
  {aSkewness,   "skewness",           MagickTrue},
  {aStdDev,     "standard_deviation", MagickTrue},
  {aH,          "h",                  MagickFalse},
  {aN,          "n",                  MagickFalse},
  {aT,          "t",                  MagickFalse},
  {aW,          "w",                  MagickFalse},
  {aZ,          "z",                  MagickFalse},
  {aNull,       "anull",              MagickFalse},
  {aNull,       "anull",              MagickFalse},
  {aNull,       "anull",              MagickFalse},
  {aNull,       "anull",              MagickFalse}
};

#define FirstSym ((SymbolE) (aNull+1))

typedef enum {
  sHue = aNull+1,
  sIntensity,
  sLightness,
  sLuma,
  sLuminance,
  sSaturation,
  sA,
  sB,
  sC,
  sG,
  sI,
  sJ,
  sK,
  sM,
  sO,
  sR,
  sY,
  sNull
} SymbolE;

typedef struct {
  SymbolE
    sym;

  const char
    *str;
} SymbolT;

static const SymbolT Symbols[] = {
  {sHue,         "hue"},
  {sIntensity,   "intensity"},
  {sLightness,   "lightness"},
  {sLuma,        "luma"},
  {sLuminance,   "luminance"},
  {sSaturation,  "saturation"},
  {sA,           "a"},
  {sB,           "b"},
  {sC,           "c"},
  {sG,           "g"},
  {sI,           "i"},
  {sJ,           "j"},
  {sK,           "k"},
  {sM,           "m"},
  {sO,           "o"},
  {sR,           "r"},
  {sY,           "y"},
  {sNull,        "snull"}
};
/*
   There is no way to access new value of pixels. This might be a future enhancement, eg "q".
   fP, oU and oV can have channel qualifier such as "u.r".
   For meta channels, we might also allow numbered channels eg "u.2" or "u.16".
   ... or have extra argument to p[].
*/

#define FirstCont (sNull+1)

/* Run-time controls are in the RPN, not explicitly in the input string. */
typedef enum {
  rGoto = FirstCont,
  rGotoChk,
  rIfZeroGoto,
  rIfNotZeroGoto,
  rCopyFrom,
  rCopyTo,
  rZerStk,
  rNull
} ControlE;

typedef struct {
  ControlE
    cont;

  const char
    *str;

  int
    number_args;
} ControlT;

static const ControlT Controls[] = {
  {rGoto,          "goto",          0},
  {rGotoChk,       "gotochk",       0},
  {rIfZeroGoto,    "ifzerogoto",    1},
  {rIfNotZeroGoto, "ifnotzerogoto", 1},
  {rCopyFrom,      "copyfrom",      0},
  {rCopyTo,        "copyto",        1},
  {rZerStk,        "zerstk",        0},
  {rNull,          "rnull",         0}
};

#define NO_CHAN_QUAL      ((PixelChannel) (-1))
#define THIS_CHANNEL      ((PixelChannel) (-2))
#define HUE_CHANNEL       ((PixelChannel) (-3))
#define SAT_CHANNEL       ((PixelChannel) (-4))
#define LIGHT_CHANNEL     ((PixelChannel) (-5))
#define INTENSITY_CHANNEL ((PixelChannel) (-6))

/* The index into UserSymbols is also the index into run-time UserSymVals.
*/
typedef struct {
  char
    *pex;

  size_t
    len;
} UserSymbolT;

typedef enum {
  etOperator,
  etConstant,
  etFunction,
  etImgAttr,
  etSymbol,
  etColourConstant,
  etControl
} ElementTypeE;

typedef struct {
  char
    *exp_start;

  ElementTypeE
    type;

  fxFltType
    val,
    val1,
    val2;

  ImgAttrE
    img_attr_qual;

  int
    element_index,
    number_args,
    number_dest, /* Number of Elements that "goto" this element */
    operator_index;

  MagickBooleanType
    do_push,
    is_relative;

  PixelChannel
    channel_qual;

  size_t
    exp_len;
} ElementT;

typedef enum {
  rtUnknown,
  rtEntireImage,
  rtCornerOnly
} RunTypeE;

typedef struct {
  CacheView *View;
  /* Other per-image metadata could go here. */
} ImgT;

typedef struct {
  RandomInfo * magick_restrict random_info;
  int numValStack;
  int usedValStack;
  fxFltType * ValStack;
  fxFltType * UserSymVals;
  Quantum * thisPixel;
} fxRtT;

typedef struct {
  PixelChannel channel;
  ssize_t imgx;
  ssize_t imgy;
} fxPixelPosT;

typedef struct {
  ssize_t imgx;
  ssize_t imgy;
} fxImgXYT;

typedef struct {
  fxFltType fx;
  fxFltType fy;
} fxFltXYT;

struct _FxInfo {
  Image * image;
  size_t ImgListLen;
  ssize_t ImgNum;
  MagickBooleanType NeedStats;
  MagickBooleanType GotStats;
  MagickBooleanType NeedHsl;
  MagickBooleanType DebugOpt;       /* Whether "-debug" option is in effect */
  MagickBooleanType ContainsDebug;  /* Whether expression contains "debug ()" function */
  char * expression;
  char * pex;
  char ShortExp[MagickPathExtent]; /* for reporting */
  int teDepth;
  char token[MagickPathExtent];
  size_t lenToken;
  int numElements;
  int usedElements;
  ElementT * Elements;  /* Elements is read-only at runtime. */
  int numUserSymbols;
  int usedUserSymbols;
  UserSymbolT * UserSymbols;
  int numOprStack;
  int usedOprStack;
  int maxUsedOprStack;
  OperatorE * OperatorStack;
  ChannelStatistics ** statistics;
  int precision;
  RunTypeE runType;

  RandomInfo
    **magick_restrict random_infos;

  ImgT * Imgs;
  Image ** Images;

  ExceptionInfo * exception;

  fxRtT * fxrts;
};

static inline char * SetPtrShortExp (FxInfo *pfx, char *pExp, size_t len)
{
  #define MaxLen 20

  size_t slen;
  char *p;

  *pfx->ShortExp = '\0';

  if (pExp && len) {
    slen = CopyMagickString(pfx->ShortExp, pExp, MagickPathExtent);

    if (slen > MaxLen) {
      (void) CopyMagickString(pfx->ShortExp + MaxLen, "...", 4);
    }

    p = strchr(pfx->ShortExp, '\n');
    if (p) (void) CopyMagickString(p, "...", 4);

    p = strchr(pfx->ShortExp, '\r');
    if (p) (void) CopyMagickString(p, "...", 4);
  }

  return pfx->ShortExp;
}

static inline const char * OprStr (int oprNum)
{
  const char * str;
  if      (oprNum < 0) str = "bad OprStr";
  else if (oprNum <= oNull) str = Operators[oprNum].str;
  else if (oprNum <= fNull) str = Functions[oprNum-(int) FirstFunc].str;
  else if (oprNum <= aNull) str = ImgAttrs[oprNum-(int) FirstImgAttr].str;
  else if (oprNum <= sNull) str = Symbols[oprNum-(int) FirstSym].str;
  else if (oprNum <= rNull) str = Controls[oprNum-(int) FirstCont].str;
  else {
    str = "bad OprStr";
  }
  return str;
}

static inline MagickBooleanType IsHslChannelQualifier(const PixelChannel chQual)
{
  return((chQual == HUE_CHANNEL || chQual == SAT_CHANNEL ||
    chQual == LIGHT_CHANNEL) ? MagickTrue : MagickFalse);
}

/*--------------------------------------------------------------------
   Run-time
*/

static inline ChannelStatistics *CollectOneImgStats (FxInfo * pfx, Image * img)
{
  int ch;
  ChannelStatistics * cs = GetImageStatistics (img, pfx->exception);
  /* Use RelinquishMagickMemory() somewhere. */

  if (cs == (ChannelStatistics *) NULL)
    return((ChannelStatistics *) NULL);

  for (ch=0; ch <= (int) MaxPixelChannels; ch++) {
    cs[ch].mean *= QuantumScale;
    cs[ch].median *= QuantumScale;
    cs[ch].maxima *= QuantumScale;
    cs[ch].minima *= QuantumScale;
    cs[ch].standard_deviation *= QuantumScale;
  }

  return cs;
}

static inline MagickBooleanType PushVal (FxInfo * pfx, fxRtT * pfxrt, fxFltType val, int addr)
{
  if (pfxrt->usedValStack >=pfxrt->numValStack) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "ValStack overflow at addr=", "%i",
      addr);
    return MagickFalse;
  }

  pfxrt->ValStack[pfxrt->usedValStack++] = val;
  return MagickTrue;
}

static inline fxFltType PopVal (FxInfo * pfx, fxRtT * pfxrt, int addr)
{
  if (pfxrt->usedValStack <= 0) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "ValStack underflow at addr=", "%i",
      addr);
    return (fxFltType) 0;
  }

  return pfxrt->ValStack[--pfxrt->usedValStack];
}

static inline PixelChannel CheckStatisticsChannel (FxInfo * pfx, PixelChannel channel)
{
  if ((channel < 0) || (channel > MaxPixelChannels))
    {
      (void) ThrowMagickException(pfx->exception,GetMagickModule(),
        OptionError,"NoSuchImageChannel","%i",channel);
      channel=(PixelChannel) 0;
    }
  return(channel);
}

typedef struct _ImageStatInputs
{
  FxInfo
    *pfx;

  ssize_t
    ImgNum;

  PixelChannel
    channel;

  ChannelStatistics
    *cs;

  ImgAttrE
    ia;
} ImageStatInputs;

static inline MagickBooleanType HasChannelStatistics(
  const ChannelStatistics *cs,const PixelChannel channel)
{
  return(((cs != (ChannelStatistics *) NULL) && (channel >= 0)) ?
    MagickTrue : MagickFalse);
}

static inline void ImageStatHToZ (const ImageStatInputs *attr, fxFltType *ret)
{
  FxInfo
    *const pfx = attr->pfx;

  const ssize_t
    ImgNum = attr->ImgNum;

  const ImgAttrE
    ia = attr->ia;

  switch (ia) {
    case aH:
      (*ret) = (fxFltType) pfx->Images[ImgNum]->rows;
      break;
    case aN:
      (*ret) = (fxFltType) pfx->ImgListLen;
      break;
    case aT: /* image index in list */
      (*ret) = (fxFltType) ImgNum;
      break;
    case aW:
      (*ret) = (fxFltType) pfx->Images[ImgNum]->columns;
      break;
    case aZ:
      (*ret) = (fxFltType) GetImageDepth (pfx->Images[ImgNum], pfx->exception);
      break;
    default:
      (void) ThrowMagickException (pfx->exception,GetMagickModule(),OptionError,
        "Unknown ia=","%i",ia);
  }
}

static inline void ImageStatResolutionToStdDev (const ImageStatInputs *attr, fxFltType *ret)
{
  FxInfo
    *const pfx = attr->pfx;

  const ssize_t
    ImgNum = attr->ImgNum;

  const PixelChannel
    channel = attr->channel;

  ChannelStatistics
    *const cs = attr->cs;

  const ImgAttrE
    ia = attr->ia;

  switch (ia) {
    case aResX:
      (*ret) = pfx->Images[ImgNum]->resolution.x;
      break;
    case aResY:
      (*ret) = pfx->Images[ImgNum]->resolution.y;
      break;
    case aSkewness:
      if (HasChannelStatistics(cs,channel) != MagickFalse)
        (*ret) = cs[channel].skewness;
      break;
    case aStdDev:
      if (HasChannelStatistics(cs,channel) != MagickFalse)
        (*ret) = cs[channel].standard_deviation;
      break;
    default:
      {
        ImageStatHToZ (attr, &(*ret));
        break;
      }
  }
}

static inline void ImageStatPageHeightToRes (const ImageStatInputs *attr, fxFltType *ret)
{
  FxInfo
    *const pfx = attr->pfx;

  const ssize_t
    ImgNum = attr->ImgNum;

  const ImgAttrE
    ia = attr->ia;

  switch (ia) {
    case aPageHt:
      (*ret) = (fxFltType) pfx->Images[ImgNum]->page.height;
      break;
    case aPrintsize:
      /* Do nothing */
      break;
    case aPrintsizeX:
      (*ret) = (fxFltType) MagickSafeReciprocal (pfx->Images[ImgNum]->resolution.x)
                        * pfx->Images[ImgNum]->columns;
      break;
    case aPrintsizeY:
      (*ret) = (fxFltType) MagickSafeReciprocal (pfx->Images[ImgNum]->resolution.y)
                        * pfx->Images[ImgNum]->rows;
      break;
    case aQuality:
      (*ret) = (fxFltType) pfx->Images[ImgNum]->quality;
      break;
    case aRes:
      /* Do nothing */
      break;
    default:
      {
        ImageStatResolutionToStdDev (attr, &(*ret));
        break;
      }
  }
}

static inline void ImageStatMinimaToPageWidth (const ImageStatInputs *attr, fxFltType *ret)
{
  FxInfo
    *const pfx = attr->pfx;

  const ssize_t
    ImgNum = attr->ImgNum;

  const PixelChannel
    channel = attr->channel;

  ChannelStatistics
    *const cs = attr->cs;

  const ImgAttrE
    ia = attr->ia;

  switch (ia) {
    case aMinima:
      if (HasChannelStatistics(cs,channel) != MagickFalse)
        (*ret) = cs[channel].minima;
      break;
    case aPage:
      /* Do nothing */
      break;
    case aPageX:
      (*ret) = (fxFltType) pfx->Images[ImgNum]->page.x;
      break;
    case aPageY:
      (*ret) = (fxFltType) pfx->Images[ImgNum]->page.y;
      break;
    case aPageWid:
      (*ret) = (fxFltType) pfx->Images[ImgNum]->page.width;
      break;
    default:
      {
        ImageStatPageHeightToRes (attr, &(*ret));
        break;
      }
  }
}

static inline void ImageStatMaximaToMedian (const ImageStatInputs *attr, fxFltType *ret)
{
  const PixelChannel
    channel = attr->channel;

  ChannelStatistics
    *const cs = attr->cs;

  const ImgAttrE
    ia = attr->ia;

  switch (ia) {
    case aMaxima:
      if (HasChannelStatistics(cs,channel) != MagickFalse)
        (*ret) = cs[channel].maxima;
      break;
    case aMean:
      if (HasChannelStatistics(cs,channel) != MagickFalse)
        (*ret) = cs[channel].mean;
      break;
    case aMedian:
      if (HasChannelStatistics(cs,channel) != MagickFalse)
        (*ret) = cs[channel].median;
      break;
    default:
      {
        ImageStatMinimaToPageWidth (attr, &(*ret));
        break;
      }
  }
}

static inline void ImageStatKurtosis (const ImageStatInputs *attr, fxFltType *ret)
{
  const PixelChannel
    channel = attr->channel;

  ChannelStatistics
    *const cs = attr->cs;

  const ImgAttrE
    ia = attr->ia;

  switch (ia) {
    case aKurtosis:
      if (HasChannelStatistics(cs,channel) != MagickFalse)
        (*ret) = cs[channel].kurtosis;
      break;
    default:
      {
        ImageStatMaximaToMedian (attr, &(*ret));
        break;
      }
  }
}

static inline fxFltType ImageStat (
  FxInfo * pfx, ssize_t ImgNum, PixelChannel channel, ImgAttrE ia)
{
  ChannelStatistics * cs = NULL;
  fxFltType ret = 0;
  MagickBooleanType NeedRelinq = MagickFalse;
  ImageStatInputs attr;

  if (ImgNum < 0)
    {
      (void) ThrowMagickException(pfx->exception,GetMagickModule(),
        OptionError,"NoSuchImage","%lu",(unsigned long) ImgNum);
      ImgNum=0;
    }

  if (pfx->GotStats) {
    channel=CheckStatisticsChannel (pfx, channel);
    cs = pfx->statistics[ImgNum];
  } else if (pfx->NeedStats) {
    /* If we need more than one statistic per pixel, this is inefficient. */
    channel=CheckStatisticsChannel (pfx, channel);
    cs = CollectOneImgStats (pfx, pfx->Images[ImgNum]);
    NeedRelinq = MagickTrue;
  }

  attr.pfx=pfx;
  attr.ImgNum=ImgNum;
  attr.channel=channel;
  attr.cs=cs;
  attr.ia=ia;
  switch (ia) {
    case aDepth:
      ret = (fxFltType) GetImageDepth (pfx->Images[ImgNum], pfx->exception);
      break;
    case aExtent:
      ret = (fxFltType) GetBlobSize (pfx->image);
      break;
    default:
      {
        ImageStatKurtosis (&attr, &ret);
        break;
      }
  }
  if (NeedRelinq) cs = (ChannelStatistics *)RelinquishMagickMemory (cs);

  return ret;
}

static inline fxFltType FxGcd (fxFltType x, fxFltType y, const size_t depth)
{
#define FxMaxFunctionDepth  200

  if (x < y)
    return (FxGcd (y, x, depth+1));
  if ((fabs((double) y) < 0.001) || (depth >= FxMaxFunctionDepth))
    return (x);
  return (FxGcd (y, x-y*floor((double) (x/y)), depth+1));
}

static inline ssize_t ChkImgNum (FxInfo * pfx, fxFltType f)
/* Returns -1 if f is too large. */
{
  ssize_t i = (ssize_t) floor ((double) f + 0.5);
  if (i < 0) i += (ssize_t) pfx->ImgListLen;
  if (i < 0 || i >= (ssize_t) pfx->ImgListLen) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "ImgNum", "%lu bad for ImgListLen %lu",
      (unsigned long) i, (unsigned long) pfx->ImgListLen);
    i = -1;
  }
  return i;
}

#define WHICH_ATTR_CHAN \
  (pel->channel_qual == NO_CHAN_QUAL) ? CompositePixelChannel : \
  (pel->channel_qual == THIS_CHANNEL) ? channel : pel->channel_qual

#define WHICH_NON_ATTR_CHAN \
  (pel->channel_qual == NO_CHAN_QUAL || \
   pel->channel_qual == THIS_CHANNEL || \
   pel->channel_qual == CompositePixelChannel \
  ) ? (channel == CompositePixelChannel ? RedPixelChannel: channel) \
    : pel->channel_qual

static inline fxFltType GetHslFlt (FxInfo * pfx, ssize_t ImgNum, const fxFltXYT * fxy,
  PixelChannel channel)
{
  const fxFltType fx = fxy->fx;
  const fxFltType fy = fxy->fy;
  Image * img = pfx->Images[ImgNum];

  double red, green, blue;
  double hue=0, saturation=0, lightness=0;

  MagickBooleanType okay = MagickTrue;
  if(!InterpolatePixelChannel (img, pfx->Imgs[ImgNum].View, RedPixelChannel, img->interpolate,
    (double) fx, (double) fy, &red, pfx->exception)) okay = MagickFalse;
  if(!InterpolatePixelChannel (img, pfx->Imgs[ImgNum].View, GreenPixelChannel, img->interpolate,
    (double) fx, (double) fy, &green, pfx->exception)) okay = MagickFalse;
  if(!InterpolatePixelChannel (img, pfx->Imgs[ImgNum].View, BluePixelChannel, img->interpolate,
    (double) fx, (double) fy, &blue, pfx->exception)) okay = MagickFalse;

  if (!okay)
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "GetHslFlt failure", "%lu %g,%g %i", (unsigned long) ImgNum,
      (double) fx, (double) fy, channel);

  ConvertRGBToHSL (
    red, green, blue,
    &hue, &saturation, &lightness);

  if (channel == HUE_CHANNEL)   return hue;
  if (channel == SAT_CHANNEL)   return saturation;
  if (channel == LIGHT_CHANNEL) return lightness;

  return 0.0;
}

static inline fxFltType GetHslInt (FxInfo * pfx, ssize_t ImgNum, const fxImgXYT * xy, PixelChannel channel)
{
  const ssize_t imgx = xy->imgx;
  const ssize_t imgy = xy->imgy;
  Image * img = pfx->Images[ImgNum];

  double hue=0, saturation=0, lightness=0;

  const Quantum * p = GetCacheViewVirtualPixels (pfx->Imgs[ImgNum].View, imgx, imgy, 1, 1, pfx->exception);
  if (p == (const Quantum *) NULL)
    {
      (void) ThrowMagickException (pfx->exception,GetMagickModule(),
        OptionError,"GetHslInt failure","%lu %li,%li %i",(unsigned long) ImgNum,
        (long) imgx,(long) imgy,channel);
      return(0.0);
    }

  ConvertRGBToHSL (
    GetPixelRed (img, p), GetPixelGreen (img, p), GetPixelBlue (img, p),
    &hue, &saturation, &lightness);

  if (channel == HUE_CHANNEL)   return hue;
  if (channel == SAT_CHANNEL)   return saturation;
  if (channel == LIGHT_CHANNEL) return lightness;

  return 0.0;
}

static inline fxFltType GetIntensity (FxInfo * pfx, ssize_t ImgNum, const fxFltType fx, const fxFltType fy)
{
  Quantum
    quantum_pixel[MaxPixelChannels];

  PixelInfo
    pixelinf;

  Image * img = pfx->Images[ImgNum];

  (void) GetPixelInfo (img, &pixelinf);

  if (!InterpolatePixelInfo (img, pfx->Imgs[ImgNum].View, img->interpolate,
              (double) fx, (double) fy, &pixelinf, pfx->exception))
  {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "GetIntensity failure", "%lu %g,%g", (unsigned long) ImgNum,
      (double) fx, (double) fy);
  }

  SetPixelViaPixelInfo (img, &pixelinf, quantum_pixel);
  return QuantumScale * GetPixelIntensity (img, quantum_pixel);
}

static inline fxFltType GetElementValue (ElementT * pel, const PixelChannel channel)
{
  fxFltType
    regA;

  if (pel->type == etColourConstant) {
    switch (channel) { default:
      case (PixelChannel) 0:
        regA = pel->val;
        break;
      case (PixelChannel) 1:
        regA = pel->val1;
        break;
      case (PixelChannel) 2:
        regA = pel->val2;
        break;
    }
  } else {
    regA = pel->val;
  }
  return(regA);
}

typedef struct _RpnPixelInputs
{
  FxInfo
    *pfx;

  ElementT
    *pel;

  PixelChannel
    channel;

  ssize_t
    imgx,
    imgy;
} RpnPixelInputs;

static inline fxFltType ExecuteFuncP (const RpnPixelInputs * in, fxFltType regA,
  const fxFltType regB)
{
  FxInfo
    *const pfx = in->pfx;

  ElementT
    *const pel = in->pel;

  const PixelChannel
    channel = in->channel;

  const ssize_t
    imgx = in->imgx,
    imgy = in->imgy;

  /* 2 args are: x, y */
  fxFltType fx, fy;
  ssize_t ImgNum = pfx->ImgNum;
  if (pel->operator_index == fVP) ImgNum = 1;
  if (pel->is_relative) {
    fx = imgx + regA;
    fy = imgy + regB;
  } else {
    fx = regA;
    fy = regB;
  }
  if ((int) pel->channel_qual < 0) {
    if (IsHslChannelQualifier(pel->channel_qual) != MagickFalse) {
      const fxFltXYT fxy = { fx, fy };
      regA = GetHslFlt (pfx, ImgNum, &fxy, pel->channel_qual);
      return(regA);
    } else if (pel->channel_qual == INTENSITY_CHANNEL) {
      regA = GetIntensity (pfx, ImgNum, fx, fy);
      return(regA);
    }
  }

  {
    double v;

    if (! InterpolatePixelChannel (pfx->Images[ImgNum], pfx->Imgs[ImgNum].View,
                                   WHICH_NON_ATTR_CHAN, pfx->Images[ImgNum]->interpolate,
                                   (double) fx, (double) fy, &v, pfx->exception)
                                  )
    {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "fSP or fVP can't get interp", "%lu", (unsigned long) ImgNum);
      return(regA);
    }
    regA = v * (fxFltType)QuantumScale;
  }

  return(regA);
}

static inline fxFltType GetOtherImageValueU (const RpnPixelInputs * in,
  const fxImgXYT xy, const ssize_t ImgNum, fxFltType regA)
{
  FxInfo
    *const pfx = in->pfx;

  ElementT
    *const pel = in->pel;

  const PixelChannel
    channel = in->channel;

  const ssize_t
    imgx = in->imgx,
    imgy = in->imgy;

  /* We have non-zero ImgNum. */
  if (pel->img_attr_qual == aNull) {
    const Quantum * pv;
    if ((int) pel->channel_qual < 0) {
      if (IsHslChannelQualifier(pel->channel_qual) != MagickFalse)
      {
        regA = GetHslInt (pfx, ImgNum, &xy, pel->channel_qual);
        return(regA);
      } else if (pel->channel_qual == INTENSITY_CHANNEL)
      {
        regA = GetIntensity (pfx, ImgNum, (fxFltType) imgx, (fxFltType) imgy);
        return(regA);
      }
    }

    pv = GetCacheViewVirtualPixels (
           pfx->Imgs[ImgNum].View, imgx, imgy, 1,1, pfx->exception);
    if (!pv) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "fU can't get cache", "%lu", (unsigned long) ImgNum);
      return(regA);
    }
    regA = QuantumScale * (double)
      pv[pfx->Images[ImgNum]->channel_map[WHICH_NON_ATTR_CHAN].offset];
  } else {
    regA = ImageStat (pfx, ImgNum, WHICH_ATTR_CHAN, pel->img_attr_qual);
  }
  return(regA);
}

typedef struct _RpnFirstImageInputs
{
  FxInfo
    *pfx;

  ElementT
    *pel;

  const Quantum
    *p;

  Image
    *pimg;

  PixelChannel
    channel;

  ssize_t
    imgx,
    imgy;
} RpnFirstImageInputs;

static inline fxFltType GetFirstImageValueU (const RpnFirstImageInputs * in,
  const ssize_t ImgNum, fxFltType regA)
{
  FxInfo
    *const pfx = in->pfx;

  ElementT
    *const pel = in->pel;

  const Quantum
    *const p = in->p;

  Image
    *const pimg = in->pimg;

  const PixelChannel
    channel = in->channel;

  const ssize_t
    imgx = in->imgx,
    imgy = in->imgy;

  if (pfx->ImgNum==0) {
    regA = QuantumScale * (double) p[pimg->channel_map[WHICH_NON_ATTR_CHAN].offset];
  } else {
    const Quantum * pv = GetCacheViewVirtualPixels (
                   pfx->Imgs[0].View, imgx, imgy, 1,1, pfx->exception);
    if (!pv) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "fU can't get cache", "%lu", (unsigned long) ImgNum);
      return(regA);
    }
    regA = QuantumScale * (double) pv[pimg->channel_map[WHICH_NON_ATTR_CHAN].offset];
  }
  return(regA);
}

static inline fxFltType GetImageZeroValueU (const RpnFirstImageInputs * in,
  const fxImgXYT xy, const ssize_t ImgNum, fxFltType regA)
{
  FxInfo
    *const pfx = in->pfx;

  ElementT
    *const pel = in->pel;

  const Quantum
    *const p = in->p;

  Image
    *const pimg = in->pimg;

  const PixelChannel
    channel = in->channel;

  const ssize_t
    imgx = in->imgx,
    imgy = in->imgy;

  if (pel->img_attr_qual == aNull) {
    if ((int) pel->channel_qual < 0) {
      if (pel->channel_qual == NO_CHAN_QUAL || pel->channel_qual == THIS_CHANNEL) {
        RpnFirstImageInputs
          first_in;

        first_in.pfx=pfx;
        first_in.pel=pel;
        first_in.p=p;
        first_in.pimg=pimg;
        first_in.channel=channel;
        first_in.imgx=imgx;
        first_in.imgy=imgy;
        regA = GetFirstImageValueU (&first_in, ImgNum, regA);
      } else if (IsHslChannelQualifier(pel->channel_qual) != MagickFalse) {
        regA = GetHslInt (pfx, ImgNum, &xy, pel->channel_qual);
        return(regA);
      } else if (pel->channel_qual == INTENSITY_CHANNEL) {
        regA = GetIntensity (pfx, 0, (double) imgx, (double) imgy);
        return(regA);
      }
    } else {
      RpnFirstImageInputs
        first_in;

      first_in.pfx=pfx;
      first_in.pel=pel;
      first_in.p=p;
      first_in.pimg=pimg;
      first_in.channel=channel;
      first_in.imgx=imgx;
      first_in.imgy=imgy;
      regA = GetFirstImageValueU (&first_in, ImgNum, regA);
    }
  } else {
    /* we have an image attribute */
    regA = ImageStat (pfx, 0, WHICH_ATTR_CHAN, pel->img_attr_qual);
  }
  return(regA);
}

static inline fxFltType GetImageZeroCacheValueU0 (const RpnPixelInputs * in,
  Image * pimg, fxFltType regA)
{
  FxInfo
    *const pfx = in->pfx;

  ElementT
    *const pel = in->pel;

  const PixelChannel
    channel = in->channel;

  const ssize_t
    imgx = in->imgx,
    imgy = in->imgy;

  const Quantum * pv = GetCacheViewVirtualPixels (
                 pfx->Imgs[0].View, imgx, imgy, 1,1, pfx->exception);
  if (!pv) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "fU0 can't get cache", "%i", 0);
    return(regA);
  }
  regA = QuantumScale * (double) pv[pimg->channel_map[WHICH_NON_ATTR_CHAN].offset];
  return(regA);
}

static inline fxFltType GetPixelValueSV (const RpnPixelInputs * in,
  const fxImgXYT xy, const ssize_t ImgNum, fxFltType regA)
{
  FxInfo
    *const pfx = in->pfx;

  ElementT
    *const pel = in->pel;

  const PixelChannel
    channel = in->channel;

  const ssize_t
    imgx = in->imgx,
    imgy = in->imgy;

  const Quantum * pv = GetCacheViewVirtualPixels (
                         pfx->Imgs[ImgNum].View, imgx, imgy, 1,1, pfx->exception);
  if (!pv) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "fV can't get cache", "%lu", (unsigned long) ImgNum);
    return(regA);
  }

  if ((int) pel->channel_qual < 0) {
    if (IsHslChannelQualifier(pel->channel_qual) != MagickFalse) {
      regA = GetHslInt (pfx, ImgNum, &xy, pel->channel_qual);
      return(regA);
    } else if (pel->channel_qual == INTENSITY_CHANNEL) {
      regA = GetIntensity (pfx, ImgNum, (double) imgx, (double) imgy);
      return(regA);
    }
  }

  regA = QuantumScale * (double)
    pv[pfx->Images[ImgNum]->channel_map[WHICH_NON_ATTR_CHAN].offset];
  return(regA);
}

static inline fxFltType GetImageZeroValueU0 (const RpnFirstImageInputs * in,
  fxFltType regA)
{
  FxInfo
    *const pfx = in->pfx;

  ElementT
    *const pel = in->pel;

  const Quantum
    *const p = in->p;

  Image
    *const pimg = in->pimg;

  const PixelChannel
    channel = in->channel;

  const ssize_t
    imgx = in->imgx,
    imgy = in->imgy;

  RpnPixelInputs
    rpn_in;

  if (pfx->ImgNum==0) {
    regA = QuantumScale * (double) p[pimg->channel_map[WHICH_NON_ATTR_CHAN].offset];
  } else {
    rpn_in.pfx=pfx;
    rpn_in.pel=pel;
    rpn_in.channel=channel;
    rpn_in.imgx=imgx;
    rpn_in.imgy=imgy;
    regA = GetImageZeroCacheValueU0 (&rpn_in, pimg, regA);
  }
  return(regA);
}

static inline fxFltType GetQualifiedValueU0 (const RpnFirstImageInputs * in,
  const fxImgXYT xy, fxFltType regA)
{
  FxInfo
    *const pfx = in->pfx;

  ElementT
    *const pel = in->pel;

  const Quantum
    *const p = in->p;

  Image
    *const pimg = in->pimg;

  const PixelChannel
    channel = in->channel;

  const ssize_t
    imgx = in->imgx,
    imgy = in->imgy;

  if (pel->channel_qual == NO_CHAN_QUAL || pel->channel_qual == THIS_CHANNEL) {
    RpnFirstImageInputs
      first_in;

    first_in.pfx=pfx;
    first_in.pel=pel;
    first_in.p=p;
    first_in.pimg=pimg;
    first_in.channel=channel;
    first_in.imgx=imgx;
    first_in.imgy=imgy;
    regA = GetImageZeroValueU0 (&first_in, regA);
  } else if (IsHslChannelQualifier(pel->channel_qual) != MagickFalse) {
    regA = GetHslInt (pfx, 0, &xy, pel->channel_qual);
    return(regA);
  } else if (pel->channel_qual == INTENSITY_CHANNEL) {
    regA = GetIntensity (pfx, 0, (fxFltType) imgx, (fxFltType) imgy);
  }
  return(regA);
}

typedef struct _RpnInterpInputs
{
  FxInfo
    *pfx;

  ElementT
    *pel;

  PixelChannel
    channel;

  ssize_t
    ImgNum;

  fxFltType
    fx,
    fy;
} RpnInterpInputs;

static inline fxFltType GetInterpolatedValueUP (const RpnInterpInputs * in,
  fxFltType regA)
{
  FxInfo
    *const pfx = in->pfx;

  ElementT
    *const pel = in->pel;

  const PixelChannel
    channel = in->channel;

  const ssize_t
    ImgNum = in->ImgNum;

  const fxFltType
    fx = in->fx,
    fy = in->fy;

  if ((int) pel->channel_qual < 0) {
    if (IsHslChannelQualifier(pel->channel_qual) != MagickFalse) {
      const fxFltXYT fxy = { fx, fy };
      regA = GetHslFlt (pfx, ImgNum, &fxy, pel->channel_qual);
      return(regA);
    } else if (pel->channel_qual == INTENSITY_CHANNEL) {
      regA = GetIntensity (pfx, ImgNum, fx, fy);
      return(regA);
    }
  }

  {
    double v;
    Image * imUP = pfx->Images[ImgNum];
    if (! InterpolatePixelChannel (imUP, pfx->Imgs[ImgNum].View, WHICH_NON_ATTR_CHAN,
            imUP->interpolate, (double) fx, (double) fy, &v, pfx->exception))
    {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "fUP can't get interpolate", "%lu", (unsigned long) ImgNum);
      return(regA);
    }
    regA = v * QuantumScale;
  }

  return(regA);
}

static inline void CheckRunTimeAddress (FxInfo * pfx, const int i)
{
  if (i < 0) {
    (void) ThrowMagickException (
      pfx->exception, GetMagickModule(), OptionError,
      "Bad run-time address", "%i", i);
  }
}

typedef struct _PopInputs
{
  FxInfo
    *pfx;

  fxRtT
    *pfxrt;

  ElementT
    *pel;

  int
    i;
} PopInputs;

typedef struct _PopTargets
{
  fxFltType
    *regA,
    *regB,
    *regC,
    *regD,
    *regE;
} PopTargets;

static inline void PopArguments (const PopInputs * pop, const PopTargets * targets)
{
  FxInfo
    *const pfx = pop->pfx;

  fxRtT
    *const pfxrt = pop->pfxrt;

  ElementT
    *const pel = pop->pel;

  const int
    i = pop->i;

  fxFltType
    *const regA = targets->regA,
    *const regB = targets->regB,
    *const regC = targets->regC,
    *const regD = targets->regD,
    *const regE = targets->regE;

  switch (pel->number_args) {
      case 0:
        break;
      case 1:
        (*regA) = PopVal (pfx, pfxrt, i);
        break;
      case 2:
        (*regB) = PopVal (pfx, pfxrt, i);
        (*regA) = PopVal (pfx, pfxrt, i);
        break;
      case 3:
        (*regC) = PopVal (pfx, pfxrt, i);
        (*regB) = PopVal (pfx, pfxrt, i);
        (*regA) = PopVal (pfx, pfxrt, i);
        break;
      case 4:
        (*regD) = PopVal (pfx, pfxrt, i);
        (*regC) = PopVal (pfx, pfxrt, i);
        (*regB) = PopVal (pfx, pfxrt, i);
        (*regA) = PopVal (pfx, pfxrt, i);
        break;
      case 5:
        (*regE) = PopVal (pfx, pfxrt, i);
        (*regD) = PopVal (pfx, pfxrt, i);
        (*regC) = PopVal (pfx, pfxrt, i);
        (*regB) = PopVal (pfx, pfxrt, i);
        (*regA) = PopVal (pfx, pfxrt, i);
        break;
      default:
        (void) ThrowMagickException (
          pfx->exception, GetMagickModule(), OptionError,
          "Too many args:", "%i", pel->number_args);
        break;
    }
}

static inline MagickBooleanType IsAttrStatisticsChannel(
  const ChannelStatistics *cs,const PixelChannel channel)
{
  return(((cs != (ChannelStatistics *) NULL) && (channel > 0)) ?
    MagickTrue : MagickFalse);
}

static inline fxFltType GetFxModValue (fxFltType regA, fxFltType regB)
{
          if (regB == 0) {
            regA = 0;
          } else {
            regA = regA - floor((double) (regA/regB))*regB;
          }
  return(regA);
}

static inline fxFltType GetFxTruncValue (fxFltType regA)
{
          if (regA >= 0) regA = floor ((double) regA);
          else regA = ceil ((double) regA);
  return(regA);
}

static inline MagickBooleanType ExecuteRPN (FxInfo * pfx, fxRtT * pfxrt, fxFltType *result,
  const fxPixelPosT * pos)
{
  const PixelChannel channel = pos->channel;
  const ssize_t imgx = pos->imgx;
  const ssize_t imgy = pos->imgy;
  const fxImgXYT xy = { imgx, imgy };
  const Quantum * p = pfxrt->thisPixel;
  fxFltType regA=0, regB=0, regC=0, regD=0, regE=0;
  Image * img = pfx->image;
  ChannelStatistics * cs = NULL;
  MagickBooleanType NeedRelinq = MagickFalse;
  double hue=0, saturation=0, lightness=0;
  int i;
  const PopTargets targets = { &regA, &regB, &regC, &regD, &regE };

  /* For -fx, this sets p to ImgNum 0.
     for %[fx:...], this sets p to the current image.
     Similarly img.
  */
  if (!p) p = GetCacheViewVirtualPixels (
    pfx->Imgs[pfx->ImgNum].View, imgx, imgy, 1, 1, pfx->exception);

  if (p == (const Quantum *) NULL)
    {
      (void) ThrowMagickException (pfx->exception,GetMagickModule(),
        OptionError,"Can't get virtual pixels","%lu %li,%li",(unsigned long)
        pfx->ImgNum,(long) imgx,(long) imgy);
      return(MagickFalse);
    }

  if (pfx->GotStats) {
    cs = pfx->statistics[pfx->ImgNum];
  } else if (pfx->NeedStats) {
    cs = CollectOneImgStats (pfx, pfx->Images[pfx->ImgNum]);
    NeedRelinq = MagickTrue;
  }

  /*  Following is only for expressions like "saturation", with no image specifier.
  */
  if (pfx->NeedHsl) {
    ConvertRGBToHSL (
      GetPixelRed (img, p), GetPixelGreen (img, p), GetPixelBlue (img, p),
      &hue, &saturation, &lightness);
  }

  for (i=0; i < pfx->usedElements; i++) {
    ElementT
      *pel;

    PopInputs
      pop_in;

    RpnPixelInputs
      rpn_in;

    CheckRunTimeAddress (pfx, i);
    pel=&pfx->Elements[i];
    pop_in.pfx=pfx;
    pop_in.pfxrt=pfxrt;
    pop_in.pel=pel;
    pop_in.i=i;
    PopArguments (&pop_in, &targets);

      switch (pel->operator_index) {
        case oAddEq:
          regA = (pfxrt->UserSymVals[pel->element_index] += regA);
          break;
        case oSubtractEq:
          regA = (pfxrt->UserSymVals[pel->element_index] -= regA);
          break;
        case oMultiplyEq:
          regA = (pfxrt->UserSymVals[pel->element_index] *= regA);
          break;
        case oDivideEq:
          regA = (pfxrt->UserSymVals[pel->element_index] /= regA);
          break;
        case oPlusPlus:
          regA = pfxrt->UserSymVals[pel->element_index]++;
          break;
        case oSubSub:
          regA = pfxrt->UserSymVals[pel->element_index]--;
          break;
        case oAdd:
          regA += regB;
          break;
        case oSubtract:
          regA -= regB;
          break;
        case oMultiply:
          regA *= regB;
          break;
        case oDivide:
          regA /= regB;
          break;
        case oModulus:
          regA = fmod ((double) regA, fabs(floor((double) regB+0.5)));
          break;
        case oUnaryPlus:
          /* Do nothing. */
          break;
        case oUnaryMinus:
          regA = -regA;
          break;
        case oLshift:
          if (CastDoubleToSizeT((double) regB+0.5) >= (8*sizeof(size_t)))
            {
              (void) ThrowMagickException ( pfx->exception, GetMagickModule(),
                OptionError, "undefined shift", "%g", (double) regB);
              regA = (fxFltType) 0.0;
              break;
            }
          regA = (fxFltType) (CastDoubleToSizeT((double) regA+0.5) << CastDoubleToSizeT((double) regB+0.5));
          break;
        case oRshift:
          if (CastDoubleToSizeT((double) regB+0.5) >= (8*sizeof(size_t)))
            {
              (void) ThrowMagickException ( pfx->exception, GetMagickModule(),
                OptionError, "undefined shift", "%g", (double) regB);
              regA = (fxFltType) 0.0;
              break;
            }
          regA = (fxFltType) (CastDoubleToSizeT((double) regA+0.5) >> CastDoubleToSizeT((double) regB+0.5));
          break;
        case oEq:
          regA = fabs((double) (regA-regB)) < MagickEpsilon ? 1.0 : 0.0;
          break;
        case oNotEq:
          regA = fabs((double) (regA-regB)) >= MagickEpsilon ? 1.0 : 0.0;
          break;
        case oLtEq:
          regA = (regA <= regB) ? 1.0 : 0.0;
          break;
        case oGtEq:
          regA = (regA >= regB) ? 1.0 : 0.0;
          break;
        case oLt:
          regA = (regA < regB) ? 1.0 : 0.0;
          break;
        case oGt:
          regA = (regA > regB) ? 1.0 : 0.0;
          break;
        case oLogAnd:
          regA = (regA<=0) ? 0.0 : (regB > 0) ? 1.0 : 0.0;
          break;
        case oLogOr:
          regA = (regA>0) ? 1.0 : (regB > 0.0) ? 1.0 : 0.0;
          break;
        case oLogNot:
          regA = (regA==0) ? 1.0 : 0.0;
          break;
        case oBitAnd:
          regA = (fxFltType) (CastDoubleToSizeT((double) regA+0.5) & CastDoubleToSizeT((double) regB+0.5));
          break;
        case oBitOr:
          regA = (fxFltType) (CastDoubleToSizeT((double) regA+0.5) | CastDoubleToSizeT((double) regB+0.5));
          break;
        case oBitNot:
          {
            size_t
              new_value;

            /* Old fx doesn't add 0.5. */
            new_value=~CastDoubleToSizeT((double) regA+0.5);
            regA=(fxFltType) new_value;
            break;
          }
        case oPow:
          regA = pow ((double) regA, (double) regB);
          break;
        case oQuery:
        case oColon:
          break;
        case oOpenParen:
        case oCloseParen:
        case oOpenBracket:
        case oCloseBracket:
        case oOpenBrace:
        case oCloseBrace:
          break;
        case oAssign:
          pel->val = regA;
          break;
        case oNull: {
          regA = GetElementValue (pel, channel);
          break;
        }
        case fAbs:
          regA = fabs ((double) regA);
          break;
#if defined(MAGICKCORE_HAVE_ACOSH)
        case fAcosh:
          regA = acosh ((double) regA);
          break;
#endif
        case fAcos:
          regA = acos ((double) regA);
          break;
#if defined(MAGICKCORE_HAVE_J1)
        case fAiry:
          if (regA==0) regA = 1.0;
          else {
            fxFltType gamma = 2.0 * __j1((double) (MagickPI*regA)) / (MagickPI*regA);
            regA = gamma * gamma;
          }
          break;
#endif
        case fAlt:
          regA = (fxFltType) (((ssize_t) regA) & 0x01 ? -1.0 : 1.0);
          break;
#if defined(MAGICKCORE_HAVE_ASINH)
        case fAsinh:
          regA = asinh ((double) regA);
          break;
#endif
        case fAsin:
          regA = asin ((double) regA);
          break;
#if defined(MAGICKCORE_HAVE_ATANH)
        case fAtanh:
          regA = atanh ((double) regA);
          break;
#endif
        case fAtan2:
          regA = atan2 ((double) regA, (double) regB);
          break;
        case fAtan:
          regA = atan ((double) regA);
          break;
        case fCeil:
          regA = ceil ((double) regA);
          break;
        case fChannel:
          switch (channel) {
            case (PixelChannel) 0: break;
            case (PixelChannel) 1: regA = regB; break;
            case (PixelChannel) 2: regA = regC; break;
            case (PixelChannel) 3: regA = regD; break;
            case (PixelChannel) 4: regA = regE; break;
            default: regA = 0.0;
          }
          break;
        case fClamp:
          if (regA < 0) regA = 0.0;
          else if (regA > 1.0) regA = 1.0;
          break;
        case fCosh:
          regA = cosh ((double) regA);
          break;
        case fCos:
          regA = cos ((double) regA);
          break;
        case fDebug:
          /* FIXME: debug() should give channel name. */

          (void) fprintf (stderr, "%s[%g,%g].[%i]: %s=%.*g\n",
                   img->filename, (double) imgx, (double) imgy,
                   channel, SetPtrShortExp (pfx, pel->exp_start, (size_t) (pel->exp_len+1)),
                   pfx->precision, (double) regA);
          break;
        case fDrc:
          regA = regA / (regB*(regA-1.0) + 1.0);
          break;
#if defined(MAGICKCORE_HAVE_ERF)
        case fErf:
          regA = erf ((double) regA);
          break;
#endif
        case fEpoch:
          /* Do nothing. */
          break;
        case fExp:
          regA = exp ((double) regA);
          break;
        case fFloor:
          regA = floor ((double) regA);
          break;
        case fGauss:
          regA = exp((double) (-regA*regA/2.0))/sqrt(2.0*MagickPI);
          break;
        case fGcd:
          if (!IsNaN((double) regA))
            regA = FxGcd (regA, regB, 0);
          break;
        case fHypot:
          regA = hypot ((double) regA, (double) regB);
          break;
        case fInt:
          regA = floor ((double) regA);
          break;
        case fIsnan:
          regA = (fxFltType) (!!IsNaN ((double) regA));
          break;
#if defined(MAGICKCORE_HAVE_J0)
        case fJ0:
          regA = __j0((double) regA);
          break;
#endif
#if defined(MAGICKCORE_HAVE_J1)
        case fJ1:
          regA = __j1((double) regA);
          break;
#endif
#if defined(MAGICKCORE_HAVE_J1)
        case fJinc:
          if (regA==0) regA = 1.0;
          else regA = 2.0 * __j1((double) (MagickPI*regA))/(MagickPI*regA);
          break;
#endif
        case fLn:
          regA = log ((double) regA);
          break;
        case fLogtwo:
          regA = log10((double) regA) / log10(2.0);
          break;
        case fLog:
          regA = log10 ((double) regA);
          break;
        case fMagickTime:
          regA = (fxFltType) GetMagickTime();
          break;
        case fMax:
          regA = (regA > regB) ? regA : regB;
          break;
        case fMin:
          regA = (regA < regB) ? regA : regB;
          break;
        case fMod:
          regA = GetFxModValue (regA, regB);
          break;
        case fNot:
          regA = (fxFltType) (regA < MagickEpsilon);
          break;
        case fPow:
          regA = pow ((double) regA, (double) regB);
          break;
        case fRand: {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
          #pragma omp critical (MagickCore_ExecuteRPN)
#endif
          regA = GetPseudoRandomValue (pfxrt->random_info);
          break;
        }
        case fRound:
          regA = floor ((double) regA + 0.5);
          break;
        case fSign:
          regA = (regA < 0) ? -1.0 : 1.0;
          break;
        case fSinc:
          regA = sin ((double) (MagickPI*regA)) / (MagickPI*regA);
          break;
        case fSinh:
          regA = sinh ((double) regA);
          break;
        case fSin:
          regA = sin ((double) regA);
          break;
        case fSqrt:
          regA = sqrt ((double) regA);
          break;
        case fSquish:
          regA = 1.0 / (1.0 + exp ((double) -regA));
          break;
        case fTanh:
          regA = tanh ((double) regA);
          break;
        case fTan:
          regA = tan ((double) regA);
          break;
        case fTrunc:
          regA = GetFxTruncValue (regA);
          break;

        case fDo:
        case fFor:
        case fIf:
        case fWhile:
          break;
        case fU: {
          /* Note: 1 value is available, index into image list.
             May have ImgAttr qualifier or channel qualifier or both.
          */
          ssize_t ImgNum = ChkImgNum (pfx, regA);
          if (ImgNum < 0) break;
          regA = (fxFltType) 0;
          if (ImgNum == 0) {
            Image * pimg = pfx->Images[0];

            RpnFirstImageInputs
              zero_in;

            zero_in.pfx=pfx;
            zero_in.pel=pel;
            zero_in.p=p;
            zero_in.pimg=pimg;
            zero_in.channel=channel;
            zero_in.imgx=imgx;
            zero_in.imgy=imgy;
            regA = GetImageZeroValueU (&zero_in, xy, ImgNum, regA);
          } else {
            rpn_in.pfx=pfx;
            rpn_in.pel=pel;
            rpn_in.channel=channel;
            rpn_in.imgx=imgx;
            rpn_in.imgy=imgy;
            regA = GetOtherImageValueU (&rpn_in, xy, ImgNum, regA);
          }
          break;
        }
        case fU0: {
          /* No args. No image attribute. We may have a ChannelQual.
             If called from %[fx:...], ChannelQual will be CompositePixelChannel.
          */
          Image * pimg = pfx->Images[0];
          if ((int) pel->channel_qual < 0) {
            RpnFirstImageInputs
              zero_in;

            zero_in.pfx=pfx;
            zero_in.pel=pel;
            zero_in.p=p;
            zero_in.pimg=pimg;
            zero_in.channel=channel;
            zero_in.imgx=imgx;
            zero_in.imgy=imgy;
            regA = GetQualifiedValueU0 (&zero_in, xy, regA);
          } else {
            RpnFirstImageInputs
              first_in;

            first_in.pfx=pfx;
            first_in.pel=pel;
            first_in.p=p;
            first_in.pimg=pimg;
            first_in.channel=channel;
            first_in.imgx=imgx;
            first_in.imgy=imgy;
            regA = GetImageZeroValueU0 (&first_in, regA);
          }
          break;
        }
        case fUP: {
          /* 3 args are: ImgNum, x, y */
          ssize_t ImgNum = ChkImgNum (pfx, regA);
          fxFltType fx, fy;
          RpnInterpInputs
            up_in;

          if (ImgNum < 0) break;

          if (pel->is_relative) {
            fx = imgx + regB;
            fy = imgy + regC;
          } else {
            fx = regB;
            fy = regC;
          }

          up_in.pfx=pfx;
          up_in.pel=pel;
          up_in.channel=channel;
          up_in.ImgNum=ImgNum;
          up_in.fx=fx;
          up_in.fy=fy;
          regA = GetInterpolatedValueUP (&up_in, regA);

          break;
        }
        case fS:
        case fV: {
          /* No args. */
          ssize_t ImgNum = 1;
          if (pel->operator_index == fS) ImgNum = pfx->ImgNum;

          if (pel->img_attr_qual == aNull) {
            rpn_in.pfx=pfx;
            rpn_in.pel=pel;
            rpn_in.channel=channel;
            rpn_in.imgx=imgx;
            rpn_in.imgy=imgy;
            regA = GetPixelValueSV (&rpn_in, xy, ImgNum, regA);
          } else {
            regA = ImageStat (pfx, ImgNum, WHICH_ATTR_CHAN, pel->img_attr_qual);
          }

          break;
        }
        case fP:
        case fSP:
        case fVP: {
          rpn_in.pfx=pfx;
          rpn_in.pel=pel;
          rpn_in.channel=channel;
          rpn_in.imgx=imgx;
          rpn_in.imgy=imgy;
          regA = ExecuteFuncP (&rpn_in, regA, regB);
          break;
        }
        case fNull:
          break;
        case aDepth:
          regA = (fxFltType) GetImageDepth (img, pfx->exception);
          break;
        case aExtent:
          regA = (fxFltType) img->extent;
          break;
        case aKurtosis:
          if (IsAttrStatisticsChannel(cs,channel) != MagickFalse)
            regA = cs[WHICH_ATTR_CHAN].kurtosis;
          break;
        case aMaxima:
          if (IsAttrStatisticsChannel(cs,channel) != MagickFalse)
            regA = cs[WHICH_ATTR_CHAN].maxima;
          break;
        case aMean:
          if (IsAttrStatisticsChannel(cs,channel) != MagickFalse)
            regA = cs[WHICH_ATTR_CHAN].mean;
          break;
        case aMedian:
          if (IsAttrStatisticsChannel(cs,channel) != MagickFalse)
            regA = cs[WHICH_ATTR_CHAN].median;
          break;
        case aMinima:
          if (IsAttrStatisticsChannel(cs,channel) != MagickFalse)
            regA = cs[WHICH_ATTR_CHAN].minima;
          break;
        case aPage:
          break;
        case aPageX:
          regA = (fxFltType) img->page.x;
          break;
        case aPageY:
          regA = (fxFltType) img->page.y;
          break;
        case aPageWid:
          regA = (fxFltType) img->page.width;
          break;
        case aPageHt:
          regA = (fxFltType) img->page.height;
          break;
        case aPrintsize:
          break;
        case aPrintsizeX:
          regA = (fxFltType) MagickSafeReciprocal (img->resolution.x) * img->columns;
          break;
        case aPrintsizeY:
          regA = (fxFltType) MagickSafeReciprocal (img->resolution.y) * img->rows;
          break;
        case aQuality:
          regA = (fxFltType) img->quality;
          break;
        case aRes:
          break;
        case aResX:
          regA = (fxFltType) img->resolution.x;
          break;
        case aResY:
          regA = (fxFltType) img->resolution.y;
          break;
        case aSkewness:
          if (IsAttrStatisticsChannel(cs,channel) != MagickFalse)
            regA = cs[WHICH_ATTR_CHAN].skewness;
          break;
        case aStdDev:
          if (IsAttrStatisticsChannel(cs,channel) != MagickFalse)
            regA = cs[WHICH_ATTR_CHAN].standard_deviation;
          break;
        case aH: /* image->rows */
          regA = (fxFltType) img->rows;
          break;
        case aN: /* image list length */
          regA = (fxFltType) pfx->ImgListLen;
          break;
        case aT: /* image index in list */
          regA = (fxFltType) pfx->ImgNum;
          break;
        case aW: /* image->columns */
          regA = (fxFltType) img->columns;
          break;
        case aZ: /* image depth */
          regA = (fxFltType) GetImageDepth (img, pfx->exception);
          break;
        case aNull:
          break;
        case sHue: /* of conversion to HSL */
          regA = hue;
          break;
        case sIntensity:
          regA = GetIntensity (pfx, pfx->ImgNum, (double) imgx, (double) imgy);
          break;
        case sLightness: /* of conversion to HSL */
          regA = lightness;
          break;
        case sLuma: /* calculation */
        case sLuminance: /* as Luma */
          regA = QuantumScale * (0.212656 * (double) GetPixelRed (img,p) +
                                 0.715158 * (double) GetPixelGreen (img,p) +
                                 0.072186 * (double) GetPixelBlue (img,p));
          break;
        case sSaturation: /* from conversion to HSL */
          regA = saturation;
          break;
        case sA: /* alpha */
          regA = QuantumScale * (double) GetPixelAlpha (img, p);
          break;
        case sB: /* blue */
          regA = QuantumScale * (double) GetPixelBlue (img, p);
          break;
        case sC: /* red (ie cyan) */
          regA = QuantumScale * (double) GetPixelCyan (img, p);
          break;
        case sG: /* green */
          regA = QuantumScale * (double) GetPixelGreen (img, p);
          break;
        case sI: /* current x-coordinate */
          regA = (fxFltType) imgx;
          break;
        case sJ: /* current y-coordinate */
          regA = (fxFltType) imgy;
          break;
        case sK: /* black of CMYK */
          regA = QuantumScale * (double) GetPixelBlack (img, p);
          break;
        case sM: /* green (ie magenta) */
          regA = QuantumScale * (double) GetPixelGreen (img, p);
          break;
        case sO: /* alpha */
          regA = QuantumScale * (double) GetPixelAlpha (img, p);
          break;
        case sR:
          regA = QuantumScale * (double) GetPixelRed (img, p);
          break;
        case sY:
          regA = QuantumScale * (double) GetPixelYellow (img, p);
          break;
        case sNull:
          break;

        case rGoto:
          assert (pel->element_index >= 0);
          i = pel->element_index-1; /* -1 because 'for' loop will increment. */
          break;
        case rGotoChk:
          assert (pel->element_index >= 0);
          i = pel->element_index-1; /* -1 because 'for' loop will increment. */
          if (IsImageTTLExpired(img) != MagickFalse) {
            i = pfx->usedElements-1; /* Do no more opcodes. */
            (void) ThrowMagickException (pfx->exception, GetMagickModule(),
              ResourceLimitFatalError, "TimeLimitExceeded", "`%s'", img->filename);
          }
          break;
        case rIfZeroGoto:
          assert (pel->element_index >= 0);
          if (fabs((double) regA) < MagickEpsilon) i = pel->element_index-1;
          break;
        case rIfNotZeroGoto:
          assert (pel->element_index >= 0);
          if (fabs((double) regA) > MagickEpsilon) i = pel->element_index-1;
          break;
        case rCopyFrom:
          assert (pel->element_index >= 0);
          regA = pfxrt->UserSymVals[pel->element_index];
          break;
        case rCopyTo:
          assert (pel->element_index >= 0);
          pfxrt->UserSymVals[pel->element_index] = regA;
          break;
        case rZerStk:
          pfxrt->usedValStack = 0;
          break;
        case rNull:
          break;

        default:
          (void) ThrowMagickException (
            pfx->exception, GetMagickModule(), OptionError,
            "pel->oprNum", "%i '%s' not yet implemented",
            (int)pel->operator_index, OprStr(pel->operator_index));
          break;
    }
    if ((pel->do_push) && (!PushVal (pfx, pfxrt, regA, i)))
      break;
  }

  if (pfxrt->usedValStack > 0) regA = PopVal (pfx, pfxrt, 9999);

  *result = regA;

  if (NeedRelinq) cs = (ChannelStatistics *)RelinquishMagickMemory (cs);

  if (pfx->exception->severity >= ErrorException)
    return MagickFalse;

  if (pfxrt->usedValStack != 0) {
      (void) ThrowMagickException (
        pfx->exception, GetMagickModule(), OptionError,
        "ValStack not empty", "(%i)", pfxrt->usedValStack);
    return MagickFalse;
  }

  return MagickTrue;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
