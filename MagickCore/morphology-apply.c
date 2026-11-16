/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%    M   M   OOO   RRRR   PPPP   H   H   OOO   L       OOO    GGGG  Y   Y     %
%    MM MM  O   O  R   R  P   P  H   H  O   O  L      O   O  G       Y Y      %
%    M M M  O   O  RRRR   PPPP   HHHHH  O   O  L      O   O  G  GG    Y       %
%    M   M  O   O  R R    P      H   H  O   O  L      O   O  G   G    Y       %
%    M   M   OOO   R  R   P      H   H   OOO   LLLLL   OOO    GGG     Y       %
%                                                                             %
%                       AAA   PPPP   PPPP   L      Y   Y                      %
%                      A   A  P   P  P   P  L       Y Y                       %
%                      AAAAA  PPPP   PPPP   L        Y                        %
%                      A   A  P      P      L        Y                        %
%                      A   A  P      P      LLLLL    Y                        %
%                                                                             %
%                                                                             %
%                MagickCore Morphology Kernels and Application                %
%                                                                             %
%                              Software Design                                %
%                              Anthony Thyssen                                %
%                               January 2010                                  %
%                                                                             %
%                                                                             %
%  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization         %
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
% Morphology is the application of various kernels, of any size or shape, to an
% image in various ways (typically binary, but not always).
%
% Convolution (weighted sum or average) is just one specific type of
% morphology. Just one that is very common for image blurring and sharpening
% effects.  Not only 2D Gaussian blurring, but also 2-pass 1D Blurring.
%
% This module provides not only a general morphology function, and the ability
% to apply more advanced or iterative morphologies, but also functions for the
% generation of many different types of kernel arrays from user supplied
% arguments. Prehaps even the generation of a kernel from a small image.
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/morphology.h"
#include "MagickCore/morphology-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/prepress.h"
#include "MagickCore/quantize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/registry.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/morphology-apply-private.h"
#include "MagickCore/morphology-apply-kernel-private.h"




/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A c q u i r e K e r n e l I n f o                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireKernelInfo() takes the given string (generally supplied by the
%  user) and converts it into a Morphology/Convolution Kernel.  This allows
%  users to specify a kernel from a number of pre-defined kernels, or to fully
%  specify their own kernel for a specific Convolution or Morphology
%  Operation.
%
%  The kernel so generated can be any rectangular array of floating point
%  values (doubles) with the 'control point' or 'pixel being affected'
%  anywhere within that array of values.
%
%  Previously IM was restricted to a square of odd size using the exact
%  center as origin, this is no longer the case, and any rectangular kernel
%  with any value being declared the origin. This in turn allows the use of
%  highly asymmetrical kernels.
%
%  The floating point values in the kernel can also include a special value
%  known as 'nan' or 'not a number' to indicate that this value is not part
%  of the kernel array. This allows you to shaped the kernel within its
%  rectangular area. That is 'nan' values provide a 'mask' for the kernel
%  shape.  However at least one non-nan value must be provided for correct
%  working of a kernel.
%
%  The returned kernel should be freed using the DestroyKernelInfo() when you
%  are finished with it.  Do not free this memory yourself.
%
%  Input kernel definition strings can consist of any of three types.
%
%    "name:args[[@><]"
%         Select from one of the built in kernels, using the name and
%         geometry arguments supplied.  See AcquireKernelBuiltIn()
%
%    "WxH[+X+Y][@><]:num, num, num ..."
%         a kernel of size W by H, with W*H floating point numbers following.
%         the 'center' can be optionally be defined at +X+Y (such that +0+0
%         is top left corner). If not defined the pixel in the center, for
%         odd sizes, or to the immediate top or left of center for even sizes
%         is automatically selected.
%
%    "num, num, num, num, ..."
%         list of floating point numbers defining an 'old style' odd sized
%         square kernel.  At least 9 values should be provided for a 3x3
%         square kernel, 25 for a 5x5 square kernel, 49 for 7x7, etc.
%         Values can be space or comma separated.  This is not recommended.
%
%  You can define a 'list of kernels' which can be used by some morphology
%  operators A list is defined as a semi-colon separated list kernels.
%
%     " kernel ; kernel ; kernel ; "
%
%  Any extra ';' characters, at start, end or between kernel definitions are
%  simply ignored.
%
%  The special flags will expand a single kernel, into a list of rotated
%  kernels. A '@' flag will expand a 3x3 kernel into a list of 45-degree
%  cyclic rotations, while a '>' will generate a list of 90-degree rotations.
%  The '<' also expands using 90-degree rotates, but giving a 180-degree
%  reflected kernel before the +/- 90-degree rotations, which can be important
%  for Thinning operations.
%
%  Note that 'name' kernels will start with an alphabetic character while the
%  new kernel specification has a ':' character in its specification string.
%  If neither is the case, it is assumed an old style of a simple list of
%  numbers generating a odd-sized square kernel has been given.
%
%  The format of the AcquireKernel method is:
%
%      KernelInfo *AcquireKernelInfo(const char *kernel_string)
%
%  A description of each parameter follows:
%
%    o kernel_string: the Morphology/Convolution kernel wanted.
%
*/

static GeometryInfo DefaultDistanceScale(const MagickStatusType flags,
  GeometryInfo args)
{
  if ( (flags & HeightValue) == 0 )           /* no distance scale */
    args.sigma = 100.0;                       /* default distance scaling */
  else if ( (flags & AspectValue ) != 0 )     /* '!' flag */
    args.sigma = (double) QuantumRange/(args.sigma+1); /* maximum pixel distance */
  else if ( (flags & PercentValue ) != 0 )    /* '%' flag */
    args.sigma *= (double) QuantumRange/100.0;         /* percentage of color range */
  return(args);
}

static GeometryInfo DefaultRectangleGeometry(const MagickStatusType flags,
  GeometryInfo args)
{
  if ( (flags & WidthValue) == 0 ) /* if no width then */
    args.rho = args.sigma;         /* then  width = height */
  if ( args.rho < 1.0 )            /* if width too small */
      args.rho = 3;                /* then  width = 3 */
  if ( args.sigma < 1.0 )          /* if height too small */
    args.sigma = args.rho;         /* then  height = width */
  if ( (flags & XValue) == 0 )     /* center offset if not defined */
    args.xi = (double)(((ssize_t)args.rho-1)/2);
  if ( (flags & YValue) == 0 )
    args.psi = (double)(((ssize_t)args.sigma-1)/2);
  return(args);
}

static GeometryInfo DefaultShapeScale(const MagickStatusType flags,
  GeometryInfo args)
{
  if ( (flags & HeightValue) == 0 )
    args.sigma = 1.0;    /* Default scale = 1.0, zero is valid */
  return(args);
}

static GeometryInfo SetDistanceKernelDefaults(const ssize_t type,
  const MagickStatusType flags,GeometryInfo args)
{
  switch( type ) {
    case ChebyshevKernel:
    case ManhattanKernel:
    case OctagonalKernel:
    case EuclideanKernel:
      args=DefaultDistanceScale(flags,args);
      break;
    default:
      break;
  }
  return(args);
}

static GeometryInfo SetRingOrRectangleKernelDefaults(const ssize_t type,
  const MagickStatusType flags,GeometryInfo args)
{
  switch( type ) {
    case RingKernel:
      if ( (flags & XValue) == 0 )
        args.xi = 1.0;       /* Default scale = 1.0, zero is valid */
      break;
    case RectangleKernel:    /* Rectangle - set size defaults */
      args=DefaultRectangleGeometry(flags,args);
      break;
    /* Distance Kernel Defaults */
    default:
      return(SetDistanceKernelDefaults(type,flags,args));
  }
  return(args);
}

static GeometryInfo SetShapeKernelDefaults(const ssize_t type,
  const MagickStatusType flags,GeometryInfo args)
{
  switch( type ) {
    case SquareKernel:
    case DiamondKernel:
    case OctagonKernel:
    case DiskKernel:
    case PlusKernel:
    case CrossKernel:
      args=DefaultShapeScale(flags,args);
      break;
    default:
      return(SetRingOrRectangleKernelDefaults(type,flags,args));
  }
  return(args);
}

static GeometryInfo SetKernelArgumentDefaults(const ssize_t type,
  const MagickStatusType flags,GeometryInfo args)
{
  switch( type ) {
    /* Shape Kernel Defaults */
    case UnityKernel:
      if ( (flags & WidthValue) == 0 )
        args.rho = 1.0;    /* Default scale = 1.0, zero is valid */
      break;
    default:
      return(SetShapeKernelDefaults(type,flags,args));
  }
  return(args);
}

static MagickBooleanType IsKernelNameSeparator(const char *p)
{
  return((((isspace((int) ((unsigned char) *p)) != 0) ||
          (*p == ',') || (*p == ':' )) && (*p != '\0') && (*p != ';')) ?
    MagickTrue : MagickFalse);
}

static const char *SkipKernelNameSeparators(const char *p)
{
  while (IsKernelNameSeparator(p) != MagickFalse)
    p++;
  return(p);
}

static void ExpandSingleKernelByFlags(KernelInfo *kernel,
  const MagickStatusType flags)
{
  if ( kernel->next == (KernelInfo *) NULL ) {
    ExpandKernelByFlags(kernel,flags);
  }
}

static KernelInfo *ParseKernelName(const char *kernel_string,
  ExceptionInfo *exception)
{
  char
    token[MagickPathExtent] = "";

  const char
    *p,
    *end;

  GeometryInfo
    args;

  KernelInfo
    *kernel;

  MagickStatusType
    flags;

  size_t
    length;

  ssize_t
    type;

  /* Parse special 'named' kernel */
  (void) GetNextToken(kernel_string,&p,MagickPathExtent,token);
  type=ParseCommandOption(MagickKernelOptions,MagickFalse,token);
  if ( type < 0 || type == UserDefinedKernel )
    return((KernelInfo *) NULL);  /* not a valid named kernel */

  p=SkipKernelNameSeparators(p);

  end=FindKernelEnd(p); /* end of this kernel definition */

  /* ParseGeometry() needs the geometry separated! -- Arrgghh */
  length=MagickMin((size_t) (end-p),sizeof(token)-1);
  (void) memcpy(token, p, length);
  token[length] = '\0';
  SetGeometryInfo(&args);
  flags = ParseGeometry(token, &args);

#if 0
  /* For Debugging Geometry Input */
  (void) FormatLocaleFile(stderr, "Geometry = 0x%04X : %lg x %lg %+lg %+lg\n",
    flags, args.rho, args.sigma, args.xi, args.psi );
#endif

  /* special handling of missing values in input string */
  args=SetKernelArgumentDefaults(type,flags,args);

  kernel = AcquireKernelBuiltIn((KernelInfoType)type, &args, exception);
  if ( kernel == (KernelInfo *) NULL )
    return(kernel);

  /* global expand to rotated kernel list - only for single kernels */
  ExpandSingleKernelByFlags(kernel,flags);

  return(kernel);
}

static KernelInfo *ParseKernelToken(const char *p,const char *token,
  ExceptionInfo *exception)
{
  KernelInfo
    *new_kernel;

  /* tokens starting with alpha is a Named kernel */
  if (isalpha((int) ((unsigned char) *token)) != 0)
    new_kernel=ParseKernelName(p,exception);
  else /* otherwise a user defined kernel array */
    new_kernel=ParseKernelArray(p);
  return(new_kernel);
}

static void AppendKernelToList(KernelInfo **kernel,KernelInfo *new_kernel)
{
  if (*kernel == (KernelInfo *) NULL)
    *kernel=new_kernel;
  else
    LastKernelInfo(*kernel)->next=new_kernel;
}

static MagickBooleanType AppendKernelToken(const char *p,const char *token,
  KernelInfo **kernel,ExceptionInfo *exception)
{
  KernelInfo
    *new_kernel;

  /* ignore extra or multiple ';' kernel separators */
  if (*token != ';')
    {
      new_kernel=ParseKernelToken(p,token,exception);

      /* Error handling -- this is not proper error handling! */
      if (new_kernel == (KernelInfo *) NULL)
        {
          if (*kernel != (KernelInfo *) NULL)
            *kernel=DestroyKernelInfo(*kernel);
          return(MagickFalse);
        }

      /* initialise or append the kernel list */
      AppendKernelToList(kernel,new_kernel);
    }
  return(MagickTrue);
}

typedef struct _KernelStringTargets
{
  char
    **kernel_cache;

  const char
    **p;
} KernelStringTargets;

static MagickBooleanType ReadKernelCache(const char *kernel_string,
  ExceptionInfo *exception,const KernelStringTargets *targets)
{
  char
    **const kernel_cache = targets->kernel_cache;

  const char
    **const p = targets->p;

  if (*kernel_string == '@')
    {
      *kernel_cache=FileToString(kernel_string,~0UL,exception);
      if (*kernel_cache == (char *) NULL)
        return(MagickFalse);
      *p=(const char *) *kernel_cache;
    }
  return(MagickTrue);
}

static void ReleaseKernelCache(char *kernel_cache)
{
  if (kernel_cache != (char *) NULL)
    kernel_cache=DestroyString(kernel_cache);
}

MagickExport KernelInfo *AcquireKernelInfo(const char *kernel_string,
  ExceptionInfo *exception)
{
  KernelInfo
    *kernel;

  char
    *kernel_cache,
    token[MagickPathExtent];

  const char
    *p;

  KernelStringTargets
    targets;

  if (kernel_string == (const char *) NULL)
    return(ParseKernelArray(kernel_string));
  p=kernel_string;
  kernel_cache=(char *) NULL;
  targets.kernel_cache=(&kernel_cache);
  targets.p=(&p);
  if (ReadKernelCache(kernel_string,exception,&targets) == MagickFalse)
    return((KernelInfo *) NULL);
  kernel=NULL;
  while (GetNextToken(p,(const char **) NULL,MagickPathExtent,token), *token != '\0')
  {
    if (AppendKernelToken(p,token,&kernel,exception) == MagickFalse)
      return((KernelInfo *) NULL);

    /* look for the next kernel in list */
    p=strchr(p,';');
    if (p == (char *) NULL)
      break;
    p++;
  }
  ReleaseKernelCache(kernel_cache);
  return(kernel);
}

static void CopyKernelValues(KernelInfo *new_kernel,const KernelInfo *kernel)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) (kernel->width*kernel->height); i++)
    new_kernel->values[i]=kernel->values[i];
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C l o n e K e r n e l I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneKernelInfo() creates a new clone of the given Kernel List so that its
%  can be modified without effecting the original.  The cloned kernel should
%  be destroyed using DestroyKernelInfo() when no longer needed.
%
%  The format of the CloneKernelInfo method is:
%
%      KernelInfo *CloneKernelInfo(const KernelInfo *kernel)
%
%  A description of each parameter follows:
%
%    o kernel: the Morphology/Convolution kernel to be cloned
%
*/
MagickExport KernelInfo *CloneKernelInfo(const KernelInfo *kernel)
{
  KernelInfo
    *new_kernel;

  assert(kernel != (KernelInfo *) NULL);
  new_kernel=(KernelInfo *) AcquireMagickMemory(sizeof(*kernel));
  if (new_kernel == (KernelInfo *) NULL)
    return(new_kernel);
  *new_kernel=(*kernel); /* copy values in structure */

  /* replace the values with a copy of the values */
  if (AcquireKernelValues(new_kernel) == MagickFalse)
    return(DestroyKernelInfo(new_kernel));
  CopyKernelValues(new_kernel,kernel);

  /* Also clone the next kernel in the kernel list */
  if ( kernel->next != (KernelInfo *) NULL ) {
    new_kernel->next = CloneKernelInfo(kernel->next);
    if ( new_kernel->next == (KernelInfo *) NULL )
      return(DestroyKernelInfo(new_kernel));
  }

  return(new_kernel);
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     U n i t y A d d K e r n a l I n f o                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnityAddKernelInfo() Adds a given amount of the 'Unity' Convolution Kernel
%  to the given pre-scaled and normalized Kernel.  This in effect adds that
%  amount of the original image into the resulting convolution kernel.  This
%  value is usually provided by the user as a percentage value in the
%  'convolve:scale' setting.
%
%  The resulting effect is to convert the defined kernels into blended
%  soft-blurs, unsharp kernels or into sharpening kernels.
%
%  The format of the UnityAdditionKernelInfo method is:
%
%      void UnityAdditionKernelInfo(KernelInfo *kernel, const double scale )
%
%  A description of each parameter follows:
%
%    o kernel: the Morphology/Convolution kernel
%
%    o scale:
%             scaling factor for the unity kernel to be added to
%             the given kernel.
%
*/
MagickExport void UnityAddKernelInfo(KernelInfo *kernel,
  const double scale)
{
  /* do the other kernels in a multi-kernel list first */
  if ( kernel->next != (KernelInfo *) NULL)
    UnityAddKernelInfo(kernel->next, scale);

  /* Add the scaled unity kernel to the existing kernel */
  kernel->values[kernel->x+kernel->y*(ssize_t) kernel->width] += scale;
  CalcKernelMetaData(kernel);  /* recalculate the meta-data */

  return;
}
