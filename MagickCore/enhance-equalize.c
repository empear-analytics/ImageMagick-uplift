/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               EEEEE  N   N  H   H   AAA   N   N   CCCC  EEEEE               %
%               E      NN  N  H   H  A   A  NN  N  C      E                   %
%               EEE    N N N  HHHHH  AAAAA  N N N  C      EEE                 %
%               E      N  NN  H   H  A   A  N  NN  C      E                   %
%               EEEEE  N   N  H   H  A   A  N   N   CCCC  EEEEE               %
%                                                                             %
%           EEEEE   QQQ   U   U   AAA   L      IIIII  ZZZZZ  EEEEE            %
%           E      Q   Q  U   U  A   A  L        I       ZZ  E                %
%           EEE    Q   Q  U   U  AAAAA  L        I     ZZZ   EEE              %
%           E      Q  QQ  U   U  A   A  L        I    ZZ     E                %
%           EEEEE   QQQQ   UUU   A   A  LLLLL  IIIII  ZZZZZ  EEEEE            %
%                                                                             %
%                                                                             %
%                      MagickCore Image Equalize Methods                      %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
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
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/accelerate-private.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/histogram.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/pixel-private.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/token.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"
#include "MagickCore/enhance-private.h"

static inline MagickBooleanType IsEqualizeMemoryMissing(
  const double *equalize_map,const double *histogram,const double *map)
{
  return(((equalize_map == (double *) NULL) || (histogram == (double *) NULL) ||
    (map == (double *) NULL)) ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     E q u a l i z e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EqualizeImage() applies a histogram equalization to the image.
%
%  The format of the EqualizeImage method is:
%
%      MagickBooleanType EqualizeImage(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void AccumulateEqualizeHistogram(const Image *image,
  const Quantum *p,double *histogram)
{
  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      double
        intensity;

      intensity=(double) p[i];
      if ((image->channel_mask & SyncChannels) != 0)
        intensity=GetPixelIntensity(image,p);
      histogram[GetPixelChannels(image)*ScaleQuantumToMap(
        ClampToQuantum(intensity))+(size_t) i]++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

typedef struct _EqualizeColors
{
  Image
    *image;

  const double
    *black,
    *white,
    *equalize_map;
} EqualizeColors;

static void EqualizeColormapValue(const EqualizeColors *colors,
  const PixelTrait traits,const PixelChannel component,MagickRealType *value)
{
  Image
    *const image = colors->image;

  const double
    *const black = colors->black,
    *const white = colors->white,
    *const equalize_map = colors->equalize_map;

  if ((traits & UpdatePixelTrait) != 0)
    {
      PixelChannel channel = GetPixelChannelChannel(image,
        component);
      if (black[channel] != white[channel])
        (*value)=equalize_map[(ssize_t)
          GetPixelChannels(image)*ScaleQuantumToMap(
          ClampToQuantum((*value)))+channel];
    }
}

static void EqualizeColormap(Image *image,const double *black,
  const double *white,const double *equalize_map)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) image->colors; j++)
  {
    EqualizeColors
      colors;

    colors.image=image;
    colors.black=black;
    colors.white=white;
    colors.equalize_map=equalize_map;
    EqualizeColormapValue(&colors,GetPixelRedTraits(image),RedPixelChannel,
      &image->colormap[j].red);
    EqualizeColormapValue(&colors,GetPixelGreenTraits(image),
      GreenPixelChannel,&image->colormap[j].green);
    EqualizeColormapValue(&colors,GetPixelBlueTraits(image),BluePixelChannel,
      &image->colormap[j].blue);
    EqualizeColormapValue(&colors,GetPixelAlphaTraits(image),
      AlphaPixelChannel,&image->colormap[j].alpha);
  }
}

static void EqualizeImageRow(const Image *image,const double *black,
  const double *white,const double *equalize_map,Quantum *q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      j;

    for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,j);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if (((traits & UpdatePixelTrait) == 0) || (black[j] == white[j]))
        continue;
      q[j]=ClampToQuantum(equalize_map[GetPixelChannels(image)*
        ScaleQuantumToMap(q[j])+(size_t) j]);
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void IntegrateEqualizeHistogram(const Image *image,
  const double *histogram,double *map)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      intensity;

    ssize_t
      j;

    intensity=0.0;
    for (j=0; j <= (ssize_t) MaxMap; j++)
    {
      intensity+=histogram[(ssize_t) GetPixelChannels(image)*j+i];
      map[(ssize_t) GetPixelChannels(image)*j+i]=intensity;
    }
  }
}

static MagickBooleanType FormEqualizeHistogram(const Image *image,
  double *histogram,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  status=MagickTrue;
  (void) memset(histogram,0,(MaxMap+1)*GetPixelChannels(image)*
    sizeof(*histogram));
  image_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    AccumulateEqualizeHistogram(image,p,histogram);
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

typedef struct _EqualizeChannel
{
  Image
    *image;

  double
    *map;

  ssize_t
    i;
} EqualizeChannel;

static void BuildEqualizeChannelMap(const EqualizeChannel *equalize,
  const double *black,const double *white,double *equalize_map)
{
  Image
    *const image = equalize->image;

  double
    *const map = equalize->map;

  const ssize_t
    i = equalize->i;

  ssize_t
    j;

  if (black[i] != white[i])
    for (j=0; j <= (ssize_t) MaxMap; j++)
      equalize_map[GetPixelChannels(image)*(size_t) j+(size_t) i]=(double)
        ScaleMapToQuantum((double) ((MaxMap*(map[GetPixelChannels(image)*
        (size_t) j+(size_t) i]-black[i]))/(white[i]-black[i])));
}

typedef struct _EqualizeLevels
{
  Image
    *image;

  double
    *map;
} EqualizeLevels;

static void BuildEqualizeMaps(const EqualizeLevels *levels,double *black,
  double *white,double *equalize_map)
{
  Image
    *const image = levels->image;

  double
    *const map = levels->map;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    EqualizeChannel
      equalize;

    black[i]=map[i];
    white[i]=map[GetPixelChannels(image)*MaxMap+(size_t) i];
    equalize.image=image;
    equalize.map=map;
    equalize.i=i;
    BuildEqualizeChannelMap(&equalize,black,white,equalize_map);
  }
}

static void RelinquishEqualizeMemory(double *map,double *histogram,
  double *equalize_map)
{
  if (map != (double *) NULL)
    map=(double *) RelinquishMagickMemory(map);
  if (histogram != (double *) NULL)
    histogram=(double *) RelinquishMagickMemory(histogram);
  if (equalize_map != (double *) NULL)
    equalize_map=(double *) RelinquishMagickMemory(equalize_map);
}

typedef struct _EqualizeMemory
{
  double
    **equalize_map,
    **histogram,
    **map;
} EqualizeMemory;

static MagickBooleanType AcquireEqualizeMemory(const Image *image,
  const EqualizeMemory *targets,ExceptionInfo *exception)
{
  double
    **const equalize_map = targets->equalize_map,
    **const histogram = targets->histogram,
    **const map = targets->map;

  (*equalize_map)=(double *) AcquireQuantumMemory(MaxMap+1UL,MaxPixelChannels*
    sizeof(*(*equalize_map)));
  (*histogram)=(double *) AcquireQuantumMemory(MaxMap+1UL,MaxPixelChannels*
    sizeof(*(*histogram)));
  (*map)=(double *) AcquireQuantumMemory(MaxMap+1UL,MaxPixelChannels*
    sizeof(*(*map)));
  if (IsEqualizeMemoryMissing((*equalize_map),(*histogram),(*map)) !=
      MagickFalse)
    {
      RelinquishEqualizeMemory((*map),(*histogram),(*equalize_map));
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  return(MagickTrue);
}

typedef struct _EqualizeTables
{
  Image
    *image;

  double
    *histogram,
    *map,
    *equalize_map;
} EqualizeTables;

static void EqualizeMapsAndColormap(const EqualizeTables *equalize,
  double *black,double *white)
{
  Image
    *const image = equalize->image;

  double
    *histogram = equalize->histogram,
    *map = equalize->map;

  double
    *const equalize_map = equalize->equalize_map;

  EqualizeLevels
    levels;

  IntegrateEqualizeHistogram(image,histogram,map);
  (void) memset(equalize_map,0,(MaxMap+1)*GetPixelChannels(image)*
    sizeof(*equalize_map));
  (void) memset(black,0,sizeof(*black));
  (void) memset(white,0,sizeof(*white));
  levels.image=image;
  levels.map=map;
  BuildEqualizeMaps(&levels,black,white,equalize_map);
  histogram=(double *) RelinquishMagickMemory(histogram);
  map=(double *) RelinquishMagickMemory(map);
  /*
    Equalize colormap.
  */
  if (image->storage_class == PseudoClass)
    EqualizeColormap(image,black,white,equalize_map);
}

MagickExport MagickBooleanType EqualizeImage(Image *image,
  ExceptionInfo *exception)
{
#define EqualizeImageTag  "Equalize/Image"

  CacheView
    *image_view;

  double
    black[2*CompositePixelChannel+1],
    *equalize_map,
    *histogram,
    *map,
    white[2*CompositePixelChannel+1];

  const EqualizeMemory
    targets = { &equalize_map, &histogram, &map };

  EqualizeTables
    equalize;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Allocate and initialize histogram arrays.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  if (AccelerateEqualizeImage(image,exception) != MagickFalse)
    return(MagickTrue);
#endif
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (AcquireEqualizeMemory(image,&targets,exception) == MagickFalse)
    return(MagickFalse);
  /*
    Form histogram.
  */
  status=FormEqualizeHistogram(image,histogram,exception);
  /*
    Integrate the histogram to get the equalization map.
  */
  equalize.image=image;
  equalize.histogram=histogram;
  equalize.map=map;
  equalize.equalize_map=equalize_map;
  EqualizeMapsAndColormap(&equalize,black,white);
  /*
    Equalize image.
  */
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    const ToneLine
      line = { image,image_view,exception };

    if (GetToneRow(&line,y,&q,&status) == MagickFalse)
      continue;
    EqualizeImageRow(image,black,white,equalize_map,q);
    SyncToneRow(&line,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetToneProgress(image,EqualizeImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  equalize_map=(double *) RelinquishMagickMemory(equalize_map);
  return(status);
}
