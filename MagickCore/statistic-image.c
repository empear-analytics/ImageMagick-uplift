/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        SSSSS  TTTTT   AAA   TTTTT  IIIII  SSSSS  TTTTT  IIIII   CCCC        %
%        SS       T    A   A    T      I    SS       T      I    C            %
%         SSS     T    AAAAA    T      I     SSS     T      I    C            %
%           SS    T    A   A    T      I       SS    T      I    C            %
%        SSSSS    T    A   A    T    IIIII  SSSSS    T    IIIII   CCCC        %
%                                                                             %
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%                                                                             %
%                  MagickCore Image Statistic Filter Method                   %
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
#include "MagickCore/animate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/compress.h"
#include "MagickCore/constitute.h"
#include "MagickCore/display.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/list.h"
#include "MagickCore/image-private.h"
#include "MagickCore/magic.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/module.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/statistic.h"
#include "MagickCore/statistic-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/timer.h"
#include "MagickCore/utility.h"
#include "MagickCore/version.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S t a t i s t i c I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StatisticImage() makes each pixel the min / max / median / mode / etc. of
%  the neighborhood of the specified width and height.
%
%  The format of the StatisticImage method is:
%
%      Image *StatisticImage(const Image *image,const StatisticType type,
%        const size_t width,const size_t height,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o type: the statistic type (median, mode, etc.).
%
%    o width: the width of the pixel neighborhood.
%
%    o height: the height of the pixel neighborhood.
%
%    o exception: return any errors or warnings in this structure.
%
*/

typedef struct _SkipNode
{
  size_t
    next[9],
    count,
    signature;
} SkipNode;

typedef struct _SkipList
{
  ssize_t
    level;

  SkipNode
    *nodes;
} SkipList;

typedef struct _PixelList
{
  size_t
    length,
    seed;

  SkipList
    skip_list;

  size_t
    signature;
} PixelList;

static PixelList *DestroyPixelList(PixelList *pixel_list)
{
  if (pixel_list == (PixelList *) NULL)
    return((PixelList *) NULL);
  if (pixel_list->skip_list.nodes != (SkipNode *) NULL)
    pixel_list->skip_list.nodes=(SkipNode *) RelinquishAlignedMemory(
      pixel_list->skip_list.nodes);
  pixel_list=(PixelList *) RelinquishMagickMemory(pixel_list);
  return(pixel_list);
}

static PixelList **DestroyPixelListTLS(PixelList **pixel_list)
{
  ssize_t
    i;

  assert(pixel_list != (PixelList **) NULL);
  for (i=0; i < (ssize_t) GetMagickResourceLimit(ThreadResource); i++)
    if (pixel_list[i] != (PixelList *) NULL)
      pixel_list[i]=DestroyPixelList(pixel_list[i]);
  pixel_list=(PixelList **) RelinquishMagickMemory(pixel_list);
  return(pixel_list);
}

static PixelList *AcquirePixelList(const size_t width,const size_t height)
{
  PixelList
    *pixel_list;

  pixel_list=(PixelList *) AcquireMagickMemory(sizeof(*pixel_list));
  if (pixel_list == (PixelList *) NULL)
    return(pixel_list);
  (void) memset((void *) pixel_list,0,sizeof(*pixel_list));
  pixel_list->length=width*height;
  pixel_list->skip_list.nodes=(SkipNode *) AcquireAlignedMemory(65537UL,
    sizeof(*pixel_list->skip_list.nodes));
  if (pixel_list->skip_list.nodes == (SkipNode *) NULL)
    return(DestroyPixelList(pixel_list));
  (void) memset(pixel_list->skip_list.nodes,0,65537UL*
    sizeof(*pixel_list->skip_list.nodes));
  pixel_list->signature=MagickCoreSignature;
  return(pixel_list);
}

static PixelList **AcquirePixelListTLS(const size_t width,
  const size_t height)
{
  PixelList
    **pixel_list;

  ssize_t
    i;

  size_t
    number_threads;

  number_threads=(size_t) GetMagickResourceLimit(ThreadResource);
  pixel_list=(PixelList **) AcquireQuantumMemory(number_threads,
    sizeof(*pixel_list));
  if (pixel_list == (PixelList **) NULL)
    return((PixelList **) NULL);
  (void) memset(pixel_list,0,number_threads*sizeof(*pixel_list));
  for (i=0; i < (ssize_t) number_threads; i++)
  {
    pixel_list[i]=AcquirePixelList(width,height);
    if (pixel_list[i] == (PixelList *) NULL)
      return(DestroyPixelListTLS(pixel_list));
  }
  return(pixel_list);
}

static ssize_t GetSkipListLevel(PixelList *pixel_list)
{
  ssize_t
    level;

  for (level=0; ; level++)
  {
    pixel_list->seed=(pixel_list->seed*42893621L)+1L;
    if ((pixel_list->seed & 0x300) != 0x300)
      break;
  }
  return(level);
}

static void AddNodePixelList(PixelList *pixel_list,const size_t color)
{
  SkipList
    *p;

  ssize_t
    level;

  size_t
    search,
    update[9];

  /*
    Initialize the node.
  */
  p=(&pixel_list->skip_list);
  p->nodes[color].signature=pixel_list->signature;
  p->nodes[color].count=1;
  /*
    Determine where it belongs in the list.
  */
  search=65536UL;
  (void) memset(update,0,sizeof(update));
  for (level=p->level; level >= 0; level--)
  {
    while (p->nodes[search].next[level] < color)
      search=p->nodes[search].next[level];
    update[level]=search;
  }
  /*
    Generate a pseudo-random level for this node.
  */
  level=GetSkipListLevel(pixel_list);
  if (level > 8)
    level=8;
  if (level > (p->level+2))
    level=p->level+2;
  /*
    If we're raising the list's level, link back to the root node.
  */
  while (level > p->level)
  {
    p->level++;
    update[p->level]=65536UL;
  }
  /*
    Link the node into the skip-list.
  */
  do
  {
    p->nodes[color].next[level]=p->nodes[update[level]].next[level];
    p->nodes[update[level]].next[level]=color;
  } while (level-- > 0);
}

static inline void GetMedianPixelList(PixelList *pixel_list,Quantum *pixel)
{
  SkipList
    *p;

  size_t
    color;

  ssize_t
    count;

  /*
    Find the median value for each of the color.
  */
  p=(&pixel_list->skip_list);
  color=65536L;
  count=0;
  do
  {
    color=p->nodes[color].next[0];
    count+=(ssize_t) p->nodes[color].count;
  } while (count <= (ssize_t) (pixel_list->length >> 1));
  *pixel=ScaleShortToQuantum((unsigned short) color);
}

static inline void GetModePixelList(PixelList *pixel_list,Quantum *pixel)
{
  SkipList
    *p;

  size_t
    color,
    max_count,
    mode;

  ssize_t
    count;

  /*
    Make each pixel the 'predominant color' of the specified neighborhood.
  */
  p=(&pixel_list->skip_list);
  color=65536L;
  mode=color;
  max_count=p->nodes[mode].count;
  count=0;
  do
  {
    color=p->nodes[color].next[0];
    if (p->nodes[color].count > max_count)
      {
        mode=color;
        max_count=p->nodes[mode].count;
      }
    count+=(ssize_t) p->nodes[color].count;
  } while (count < (ssize_t) pixel_list->length);
  *pixel=ScaleShortToQuantum((unsigned short) mode);
}

static inline void GetNonpeakPixelList(PixelList *pixel_list,Quantum *pixel)
{
  SkipList
    *p;

  size_t
    color,
    next,
    previous;

  ssize_t
    count;

  /*
    Finds the non peak value for each of the colors.
  */
  p=(&pixel_list->skip_list);
  color=65536L;
  next=p->nodes[color].next[0];
  count=0;
  do
  {
    previous=color;
    color=next;
    next=p->nodes[color].next[0];
    count+=(ssize_t) p->nodes[color].count;
  } while (count <= (ssize_t) (pixel_list->length >> 1));
  if ((previous == 65536UL) && (next != 65536UL))
    color=next;
  else
    if ((previous != 65536UL) && (next == 65536UL))
      color=previous;
  *pixel=ScaleShortToQuantum((unsigned short) color);
}

static inline void InsertPixelList(const Quantum pixel,PixelList *pixel_list)
{
  size_t
    signature;

  unsigned short
    index;

  index=ScaleQuantumToShort(pixel);
  signature=pixel_list->skip_list.nodes[index].signature;
  if (signature == pixel_list->signature)
    {
      pixel_list->skip_list.nodes[index].count++;
      return;
    }
  AddNodePixelList(pixel_list,index);
}

static void ResetPixelList(PixelList *pixel_list)
{
  int
    level;

  SkipNode
    *root;

  SkipList
    *p;

  /*
    Reset the skip-list.
  */
  p=(&pixel_list->skip_list);
  root=p->nodes+65536UL;
  p->level=0;
  for (level=0; level < 9; level++)
    root->next[level]=65536UL;
  pixel_list->seed=pixel_list->signature++;
}

static inline MagickBooleanType IsPixelListStatistic(const StatisticType type)
{
  return(((type == MedianStatistic) || (type == ModeStatistic) ||
    (type == NonpeakStatistic)) ? MagickTrue : MagickFalse);
}

typedef struct _StatisticOperands
{
  StatisticType
    type;

  double
    area,
    minimum,
    maximum,
    sum,
    sum_squared;
} StatisticOperands;

static void ComputeStatisticPixel(const StatisticOperands *operands,
  Quantum *pixel)
{
  const StatisticType
    type = operands->type;

  const double
    area = operands->area,
    minimum = operands->minimum,
    maximum = operands->maximum,
    sum = operands->sum,
    sum_squared = operands->sum_squared;

  switch (type)
  {
    case ContrastStatistic:
    {
      (*pixel)=ClampToQuantum(MagickAbsoluteValue((maximum-minimum)*
        MagickSafeReciprocal(maximum+minimum)));
      break;
    }
    case GradientStatistic:
    {
      (*pixel)=ClampToQuantum(MagickAbsoluteValue(maximum-minimum));
      break;
    }
    case MeanStatistic:
    default:
    {
      (*pixel)=ClampToQuantum(sum/area);
      break;
    }
    case RootMeanSquareStatistic:
    {
      (*pixel)=ClampToQuantum(sqrt(sum_squared/area));
      break;
    }
    case StandardDeviationStatistic:
    {
      (*pixel)=ClampToQuantum(sqrt(sum_squared/area-(sum/area*sum/area)));
      break;
    }
  }
}

static Quantum GetStatisticPixel(const StatisticType type,
  PixelList **magick_restrict pixel_list,const int id,const double area,
  const double minimum,const double maximum,const double sum,
  const double sum_squared)
{
  Quantum
    pixel;

  switch (type)
  {
    case MaximumStatistic:
    {
      pixel=ClampToQuantum(maximum);
      break;
    }
    default:
    {
      StatisticOperands
        operands;

      operands.type=type;
      operands.area=area;
      operands.minimum=minimum;
      operands.maximum=maximum;
      operands.sum=sum;
      operands.sum_squared=sum_squared;
      ComputeStatisticPixel(&operands,&pixel);
      break;
    }
    case MedianStatistic:
    {
      GetMedianPixelList(pixel_list[id],&pixel);
      break;
    }
    case MinimumStatistic:
    {
      pixel=ClampToQuantum(minimum);
      break;
    }
    case ModeStatistic:
    {
      GetModePixelList(pixel_list[id],&pixel);
      break;
    }
    case NonpeakStatistic:
    {
      GetNonpeakPixelList(pixel_list[id],&pixel);
      break;
    }
  }
  return(pixel);
}

typedef struct _StatisticChannel
{
  const Image
    *image,
    *statistic_image;

  StatisticType
    type;

  size_t
    width,
    height;

  const Quantum
    *magick_restrict p;

  ssize_t
    i;

  PixelList
    **magick_restrict pixel_list;

  int
    id;

  PixelChannel
    channel;
} StatisticChannel;

static void SetStatisticChannel(const StatisticChannel *statistic,
  Quantum *magick_restrict q)
{
  const Image
    *const image = statistic->image,
    *const statistic_image = statistic->statistic_image;

  const StatisticType
    type = statistic->type;

  const size_t
    width = statistic->width,
    height = statistic->height;

  const Quantum
    *magick_restrict const p = statistic->p;

  const ssize_t
    i = statistic->i;

  PixelList
    **magick_restrict const pixel_list = statistic->pixel_list;

  const int
    id = statistic->id;

  const PixelChannel
    channel = statistic->channel;

  double
    area,
    maximum,
    minimum,
    sum,
    sum_squared;

  Quantum
    pixel;

  const Quantum
    *magick_restrict pixels;

  ssize_t
    u;

  ssize_t
    v;

  pixels=p;
  area=0.0;
  minimum=pixels[i];
  maximum=pixels[i];
  sum=0.0;
  sum_squared=0.0;
  ResetPixelList(pixel_list[id]);
  for (v=0; v < (ssize_t) MagickMax(height,1); v++)
  {
    for (u=0; u < (ssize_t) MagickMax(width,1); u++)
    {
      if (IsPixelListStatistic(type) != MagickFalse)
        {
          InsertPixelList(pixels[i],pixel_list[id]);
          pixels+=(ptrdiff_t) GetPixelChannels(image);
          continue;
        }
      area++;
      if ((double) pixels[i] < minimum)
        minimum=(double) pixels[i];
      if ((double) pixels[i] > maximum)
        maximum=(double) pixels[i];
      sum+=(double) pixels[i];
      sum_squared+=(double) pixels[i]*(double) pixels[i];
      pixels+=(ptrdiff_t) GetPixelChannels(image);
    }
    pixels+=(ptrdiff_t) GetPixelChannels(image)*image->columns;
  }
  pixel=GetStatisticPixel(type,pixel_list,id,area,minimum,maximum,sum,
    sum_squared);
  SetPixelChannel(statistic_image,channel,pixel,q);
}

static void StatisticImagePixel(const Image *image,
  const Image *statistic_image,const StatisticType type,const size_t width,
  const size_t height,const ssize_t center,const Quantum *magick_restrict p,
  Quantum *magick_restrict q,PixelList **magick_restrict pixel_list,
  const int id)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    StatisticChannel
      statistic;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait statistic_traits=GetPixelChannelTraits(statistic_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((statistic_traits & UpdatePixelTrait) == 0))
      continue;
    if (((statistic_traits & CopyPixelTrait) != 0) ||
        (GetPixelWriteMask(image,p) <= (QuantumRange/2)))
      {
        SetPixelChannel(statistic_image,channel,p[center+i],q);
        continue;
      }
    statistic.image=image;
    statistic.statistic_image=statistic_image;
    statistic.type=type;
    statistic.width=width;
    statistic.height=height;
    statistic.p=p;
    statistic.i=i;
    statistic.pixel_list=pixel_list;
    statistic.id=id;
    statistic.channel=channel;
    SetStatisticChannel(&statistic,q);
  }
}

static MagickBooleanType AcquireStatisticImage(const Image *image,
  Image **statistic_image,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  *statistic_image=CloneImage(image,0,0,MagickTrue,
    exception);
  if (*statistic_image == (Image *) NULL)
    return(MagickFalse);
  status=SetImageStorageClass(*statistic_image,DirectClass,exception);
  if (status == MagickFalse)
    {
      *statistic_image=DestroyImage(*statistic_image);
      return(MagickFalse);
    }
  return(MagickTrue);
}

MagickExport Image *StatisticImage(const Image *image,const StatisticType type,
  const size_t width,const size_t height,ExceptionInfo *exception)
{
#define StatisticImageTag  "Statistic/Image"

  CacheView
    *image_view,
    *statistic_view;

  Image
    *statistic_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelList
    **magick_restrict pixel_list;

  ssize_t
    center,
    y;

  /*
    Initialize statistics image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (AcquireStatisticImage(image,&statistic_image,exception) == MagickFalse)
    return((Image *) NULL);
  pixel_list=AcquirePixelListTLS(MagickMax(width,1),MagickMax(height,1));
  if (pixel_list == (PixelList **) NULL)
    {
      statistic_image=DestroyImage(statistic_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Make each pixel the min / max / median / mode / etc. of the neighborhood.
  */
  center=(ssize_t) GetPixelChannels(image)*((ssize_t) image->columns+
    MagickMax((ssize_t) width,1L))*(MagickMax((ssize_t) height,1)/2L)+(ssize_t)
    GetPixelChannels(image)*(MagickMax((ssize_t) width,1L)/2L);
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  statistic_view=AcquireAuthenticCacheView(statistic_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,statistic_image,statistic_image->rows,1)
#endif
  for (y=0; y < (ssize_t) statistic_image->rows; y++)
  {
    const int
      id = GetOpenMPThreadId();

    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,-((ssize_t) MagickMax(width,1)/2L),y-
      (ssize_t) (MagickMax(height,1)/2L),image->columns+MagickMax(width,1),
      MagickMax(height,1),exception);
    q=QueueCacheViewAuthenticPixels(statistic_view,0,y,statistic_image->columns,      1,exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) statistic_image->columns; x++)
    {
      StatisticImagePixel(image,statistic_image,type,width,height,center,p,q,
        pixel_list,id);
      p+=(ptrdiff_t) GetPixelChannels(image);
      q+=(ptrdiff_t) GetPixelChannels(statistic_image);
    }
    if (SyncCacheViewAuthenticPixels(statistic_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,StatisticImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  statistic_view=DestroyCacheView(statistic_view);
  image_view=DestroyCacheView(image_view);
  pixel_list=DestroyPixelListTLS(pixel_list);
  if (status == MagickFalse)
    statistic_image=DestroyImage(statistic_image);
  return(statistic_image);
}
