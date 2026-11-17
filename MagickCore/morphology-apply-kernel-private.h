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

  MagickCore private methods shared by the kernel construction methods: the
  kernel array parser and the kernel list helpers of AcquireKernelInfo and
  AcquireKernelBuiltIn.
*/
#ifndef MAGICKCORE_MORPHOLOGY_APPLY_KERNEL_PRIVATE_H
#define MAGICKCORE_MORPHOLOGY_APPLY_KERNEL_PRIVATE_H

#include "MagickCore/morphology-apply-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
  Define declarations.
*/
#define Minimize(assign,value) assign=MagickMin(assign,value)
#define Maximize(assign,value) assign=MagickMax(assign,value)

/*
  Method prototypes.
*/
/* Currently these are only internal to this module */
static void
  CalcKernelMetaData(KernelInfo *),
  ExpandMirrorKernelInfo(KernelInfo *),
  ExpandRotateKernelInfo(KernelInfo *, const double);

/* Quick function to find last kernel in a kernel list */
static inline KernelInfo *LastKernelInfo(KernelInfo *kernel)
{
  while (kernel->next != (KernelInfo *) NULL)
    kernel=kernel->next;
  return(kernel);
}

static inline MagickBooleanType AcquireKernelValues(KernelInfo *kernel)
{
  size_t 
    elements;

  kernel->values=(MagickRealType *) NULL;
  if (HeapOverflowSanityCheckGetSize(kernel->width,kernel->height,&elements) != MagickFalse)
    return(MagickFalse);
  kernel->values=(MagickRealType *) MagickAssumeAligned(AcquireAlignedMemory(
    elements,sizeof(*kernel->values)));
  return(kernel->values == (MagickRealType *) NULL ? MagickFalse : MagickTrue);
}

static inline ssize_t CountKernelArrayValues(const char *p,const char *end)
{
  char
    token[MagickPathExtent];

  ssize_t
    i;

  for (i=0; p < end; i++)
  {
    (void) GetNextToken(p,&p,MagickPathExtent,token);
    if (*token == ',')
      (void) GetNextToken(p,&p,MagickPathExtent,token);
  }
  return(i);
}

static inline const char *SkipKernelArrayQuotes(const char *p)
{
  while ((isspace((int) ((unsigned char) *p)) != 0) || (*p == '\''))
    p++;
  return(p);
}

static inline void SetOddSquareSize(KernelInfo *kernel,const ssize_t i)
{
  kernel->width = kernel->height=CastDoubleToSizeT(sqrt((double) i+1.0));
  kernel->x = kernel->y = (ssize_t) (kernel->width-1)/2;
}

/* This was separated so that it could be used as a separate
** array input handling function, such as for -color-matrix
*/
static inline const char *SetOddSquareKernelSize(KernelInfo *kernel,
  const char *kernel_string,const char *end)
{
  const char
    *p;

  ssize_t
    i;

  /* ELSE - Old old specification, forming odd-square kernel */
  /* count up number of values given */
  p=(const char *) kernel_string;
  p=SkipKernelArrayQuotes(p);  /* ignore "'" chars for convolve filter usage - Cristy */
  i=CountKernelArrayValues(p,end);
  /* set the size of the kernel - old sized square */
  SetOddSquareSize(kernel,i);
  p=(const char *) kernel_string;
  while ((isspace((int) ((unsigned char) *p)) != 0) || (*p == '\''))
    p++;  /* ignore "'" chars for convolve filter usage - Cristy */
  return(p);
}

static inline void SetKernelArrayValue(KernelInfo *kernel,const ssize_t i,
  const char *token,const double nan)
{
  if (    LocaleCompare("nan",token) == 0
      || LocaleCompare("-",token) == 0 ) {
    kernel->values[i] = nan; /* this value is not part of neighbourhood */
  }
  else {
    kernel->values[i] = StringToDouble(token,(char **) NULL);
    ( kernel->values[i] < 0)
        ?  ( kernel->negative_range += kernel->values[i] )
        :  ( kernel->positive_range += kernel->values[i] );
    Minimize(kernel->minimum, kernel->values[i]);
    Maximize(kernel->maximum, kernel->values[i]);
  }
}

static inline const char *ReadKernelArrayToken(const char *p,char *token)
{
  (void) GetNextToken(p,&p,MagickPathExtent,token);
  if (*token == ',')
    (void) GetNextToken(p,&p,MagickPathExtent,token);
  return(p);
}

static inline GeometryInfo SetKernelArraySize(KernelInfo *kernel,
  const MagickStatusType flags,GeometryInfo args)
{
  if ( (flags & WidthValue) == 0 ) /* if no width then */
    args.rho = args.sigma;         /* then  width = height */
  if ( args.rho < 1.0 )            /* if width too small */
     args.rho = 1.0;               /* then  width = 1 */
  if ( args.sigma < 1.0 )          /* if height too small */
    args.sigma = args.rho;         /* then  height = width */
  kernel->width = CastDoubleToSizeT(args.rho);
  kernel->height = CastDoubleToSizeT(args.sigma);
  return(args);
}

static inline void SetKernelArrayOrigin(KernelInfo *kernel,
  const MagickStatusType *flags,const GeometryInfo args)
{
  kernel->x = ((*flags & XValue)!=0) ? (ssize_t)args.xi
                                    : (ssize_t) (kernel->width-1)/2;
  kernel->y = ((*flags & YValue)!=0) ? (ssize_t)args.psi
                                    : (ssize_t) (kernel->height-1)/2;
}

static inline MagickBooleanType ParseKernelGeometry(const char *kernel_string,
  const char *p,KernelInfo *kernel,MagickStatusType *flags)
{
  char
    token[MagickPathExtent];

  GeometryInfo
    args;

  size_t
    length;

  /* ParseGeometry() needs the geometry separated! -- Arrgghh */
  length=MagickMin((size_t) (p-kernel_string),sizeof(token)-1);
  (void) memcpy(token, kernel_string, length);
  token[length] = '\0';
  SetGeometryInfo(&args);
  *flags = ParseGeometry(token, &args);

  /* Size handling and checks of geometry settings */
  args=SetKernelArraySize(kernel,*flags,args);

  /* Offset Handling and Checks */
  if ( args.xi  < 0.0 || args.psi < 0.0 )
    return(MagickFalse);
  SetKernelArrayOrigin(kernel,flags,args);
  if ( kernel->x >= (ssize_t) kernel->width ||
       kernel->y >= (ssize_t) kernel->height )
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _KernelSpecificationTargets
{
  MagickStatusType
    *flags;

  const char
    **p;
} KernelSpecificationTargets;

static inline MagickBooleanType ParseKernelSpecification(const char *kernel_string,
  const char *end,KernelInfo *kernel,const KernelSpecificationTargets *targets)
{
  MagickStatusType
    *const flags = targets->flags;

  const char
    **const p = targets->p;

  if ( *p != (char *) NULL && *p < end)
    {
      if (ParseKernelGeometry(kernel_string,*p,kernel,flags) == MagickFalse)
        return(MagickFalse);

      (*p)++; /* advance beyond the ':' */
    }
  else
    *p=SetOddSquareKernelSize(kernel,kernel_string,end);
  return(MagickTrue);
}

static inline void ExpandKernelByFlags(KernelInfo *kernel,
  const MagickStatusType flags)
{
  if ( (flags & AreaValue) != 0 )         /* '@' symbol in kernel size */
    ExpandRotateKernelInfo(kernel, 45.0); /* cyclic rotate 3x3 kernels */
  else if ( (flags & GreaterValue) != 0 ) /* '>' symbol in kernel args */
    ExpandRotateKernelInfo(kernel, 90.0); /* 90 degree rotate of kernel */
  else if ( (flags & LessValue) != 0 )    /* '<' symbol in kernel args */
    ExpandMirrorKernelInfo(kernel);       /* 90 degree mirror rotate */
}

static inline const char *FindKernelEnd(const char *kernel_string)
{
  const char
    *end;

  end = strchr(kernel_string, ';');
  if ( end == (char *) NULL )
    end = strchr(kernel_string, '\0');
  return(end);
}

static inline MagickBooleanType IsKernelArrayTokenLeft(const char *token)
{
  return(( *token != '\0' && *token != ';' && *token != '\'' ) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsKernelArrayReading(const KernelInfo *kernel,
  const ssize_t i,const char *p,const char *end)
{
  return(((i < (ssize_t) (kernel->width*kernel->height)) && (p < end)) ?
    MagickTrue : MagickFalse);
}

static inline KernelInfo *FinishKernelArray(KernelInfo *kernel,
  const MagickStatusType flags)
{
  /* check that we received at least one real (non-nan) value! */
  if (kernel->minimum == MagickMaximumValue)
    return(DestroyKernelInfo(kernel));

  ExpandKernelByFlags(kernel,flags);

  return(kernel);
}

static inline MagickBooleanType IsKernelArrayComplete(KernelInfo *kernel,
  const char *p,const ssize_t i)
{
  char
    token[MagickPathExtent];

  /* sanity check -- no more values in kernel definition */
  (void) GetNextToken(p,&p,MagickPathExtent,token);
  if (IsKernelArrayTokenLeft(token) != MagickFalse)
    return(MagickFalse);

#if 0
  /* this was the old method of handling a incomplete kernel */
  if ( i < (ssize_t) (kernel->width*kernel->height) ) {
    Minimize(kernel->minimum, kernel->values[i]);
    Maximize(kernel->maximum, kernel->values[i]);
    for ( ; i < (ssize_t) (kernel->width*kernel->height); i++)
      kernel->values[i]=0.0;
  }
#else
  /* Number of values for kernel was not enough - Report Error */
  if ( i < (ssize_t) (kernel->width*kernel->height) )
    return(MagickFalse);
#endif

  return(MagickTrue);
}

static inline void ResetUserDefinedKernel(KernelInfo *kernel)
{
  (void) memset(kernel,0,sizeof(*kernel));
  kernel->minimum = kernel->maximum = kernel->angle = 0.0;
  kernel->negative_range = kernel->positive_range = 0.0;
  kernel->type = UserDefinedKernel;
  kernel->next = (KernelInfo *) NULL;
  kernel->signature=MagickCoreSignature;
}

static inline void ResetKernelArrayRanges(KernelInfo *kernel)
{
  kernel->minimum=MagickMaximumValue;
  kernel->maximum=(-MagickMaximumValue);
  kernel->negative_range = kernel->positive_range = 0.0;
}

static inline KernelInfo *ParseKernelArray(const char *kernel_string)
{
  KernelInfo
    *kernel;

  char
    token[MagickPathExtent];

  const char
    *p,
    *end;

  ssize_t
    i;

  double
    nan = sqrt(-1.0);  /* Special Value : Not A Number */

  MagickStatusType
    flags;

  KernelSpecificationTargets
    targets;

  kernel=(KernelInfo *) AcquireMagickMemory(sizeof(*kernel));
  if (kernel == (KernelInfo *) NULL)
    return(kernel);
  ResetUserDefinedKernel(kernel);
  if (kernel_string == (const char *) NULL)
    return(kernel);

  /* find end of this specific kernel definition string */
  end=FindKernelEnd(kernel_string);

  /* clear flags - for Expanding kernel lists through rotations */
   flags = NoValue;

  /* Has a ':' in argument - New user kernel specification
     FUTURE: this split on ':' could be done by StringToken()
   */
  p = strchr(kernel_string, ':');
  targets.flags=(&flags);
  targets.p=(&p);
  if (ParseKernelSpecification(kernel_string,end,kernel,&targets) == MagickFalse)
    return(DestroyKernelInfo(kernel));

  /* Read in the kernel values from rest of input string argument */
  if (AcquireKernelValues(kernel) == MagickFalse)
    return(DestroyKernelInfo(kernel));
  ResetKernelArrayRanges(kernel);
  for (i=0; IsKernelArrayReading(kernel,i,p,end) != MagickFalse; i++)
  {
    p=ReadKernelArrayToken(p,token);
    SetKernelArrayValue(kernel,i,token,nan);
  }

  if (IsKernelArrayComplete(kernel,p,i) == MagickFalse)
    return(DestroyKernelInfo(kernel));

  return(FinishKernelArray(kernel,flags));
}

static inline void AppendFloppedKernel(KernelInfo *last)
{
  KernelInfo
    *clone;

  clone = CloneKernelInfo(last);
  if (clone == (KernelInfo *) NULL)
    return;
  RotateKernelInfo(clone, 180);  /* flop */
  LastKernelInfo(last)->next = clone;
}

static inline void ExpandMirrorKernelInfo(KernelInfo *kernel)
{
  KernelInfo
    *clone,
    *last;

  last = kernel;

  clone = CloneKernelInfo(last);
  if (clone == (KernelInfo *) NULL)
    return;
  RotateKernelInfo(clone, 180);   /* flip */
  LastKernelInfo(last)->next = clone;
  last = clone;

  clone = CloneKernelInfo(last);
  if (clone == (KernelInfo *) NULL)
    return;
  RotateKernelInfo(clone, 90);   /* transpose */
  LastKernelInfo(last)->next = clone;
  last = clone;

  AppendFloppedKernel(last);

  return;
}

static inline MagickBooleanType IsKernelShapeDifferent(
  const KernelInfo *kernel1,const KernelInfo *kernel2)
{
  return((    kernel1->width != kernel2->width
       || kernel1->height != kernel2->height
       || kernel1->x != kernel2->x
       || kernel1->y != kernel2->y ) ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     E x p a n d R o t a t e K e r n e l I n f o                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExpandRotateKernelInfo() takes a kernel list, and expands it by rotating
%  incrementally by the angle given, until the kernel repeats.
%
%  WARNING: 45 degree rotations only works for 3x3 kernels.
%  While 90 degree rotations only works for linear and square kernels
%
%  The format of the ExpandRotateKernelInfo method is:
%
%      void ExpandRotateKernelInfo(KernelInfo *kernel, double angle)
%
%  A description of each parameter follows:
%
%    o kernel: the Morphology/Convolution kernel
%
%    o angle: angle to rotate in degrees
%
% This function is only internal to this module, as it is not finalized,
% especially with regard to non-orthogonal angles, and rotation of larger
% 2D kernels.
*/

/* Internal Routine - Return true if two kernels are the same */
static inline MagickBooleanType SameKernelInfo(const KernelInfo *kernel1,
     const KernelInfo *kernel2)
{
  size_t
    i;

  /* check size and origin location */
  if (IsKernelShapeDifferent(kernel1,kernel2) != MagickFalse)
    return MagickFalse;

  /* check actual kernel values */
  for (i=0; i < (kernel1->width*kernel1->height); i++) {
    /* Test for Nan equivalence */
    if ( IsNaN(kernel1->values[i]) && !IsNaN(kernel2->values[i]) )
      return MagickFalse;
    if ( IsNaN(kernel2->values[i]) && !IsNaN(kernel1->values[i]) )
      return MagickFalse;
    /* Test actual values are equivalent */
    if ( fabs(kernel1->values[i] - kernel2->values[i]) >= MagickEpsilon )
      return MagickFalse;
  }

  return MagickTrue;
}

static inline void ReleaseRepeatedKernel(KernelInfo *clone_info)
{
  if (clone_info != (KernelInfo *) NULL)
    clone_info=DestroyKernelInfo(clone_info);
}

static inline void ExpandRotateKernelInfo(KernelInfo *kernel,const double angle)
{
  KernelInfo
    *clone_info,
    *last;

  clone_info=(KernelInfo *) NULL;
  last=kernel;
DisableMSCWarning(4127)
  while (1) {
RestoreMSCWarning
    clone_info=CloneKernelInfo(last);
    if (clone_info == (KernelInfo *) NULL)
      break;
    RotateKernelInfo(clone_info,angle);
    if (SameKernelInfo(kernel,clone_info) != MagickFalse)
      break;
    LastKernelInfo(last)->next=clone_info;
    last=clone_info;
  }
  ReleaseRepeatedKernel(clone_info);  /* kernel repeated - junk */
  return;
}
static inline void AddKernelMetaValue(KernelInfo *kernel,const size_t i)
{
  if ( fabs(kernel->values[i]) < MagickEpsilon )
    kernel->values[i] = 0.0;
  ( kernel->values[i] < 0)
      ?  ( kernel->negative_range += kernel->values[i] )
      :  ( kernel->positive_range += kernel->values[i] );
  Minimize(kernel->minimum, kernel->values[i]);
  Maximize(kernel->maximum, kernel->values[i]);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     C a l c M e t a K e r n a l I n f o                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CalcKernelMetaData() recalculate the KernelInfo meta-data of this kernel only,
%  using the kernel values.  This should only ne used if it is not possible to
%  calculate that meta-data in some easier way.
%
%  It is important that the meta-data is correct before ScaleKernelInfo() is
%  used to perform kernel normalization.
%
%  The format of the CalcKernelMetaData method is:
%
%      void CalcKernelMetaData(KernelInfo *kernel, const double scale )
%
%  A description of each parameter follows:
%
%    o kernel: the Morphology/Convolution kernel to modify
%
%  WARNING: Minimum and Maximum values are assumed to include zero, even if
%  zero is not part of the kernel (as in Gaussian Derived kernels). This
%  however is not true for flat-shaped morphological kernels.
%
%  WARNING: Only the specific kernel pointed to is modified, not a list of
%  multiple kernels.
%
% This is an internal function and not expected to be useful outside this
% module.  This could change however.
*/
static inline void CalcKernelMetaData(KernelInfo *kernel)
{
  size_t
    i;

  kernel->minimum = kernel->maximum = 0.0;
  kernel->negative_range = kernel->positive_range = 0.0;
  for (i=0; i < (kernel->width*kernel->height); i++)
    AddKernelMetaValue(kernel,i);

  return;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
