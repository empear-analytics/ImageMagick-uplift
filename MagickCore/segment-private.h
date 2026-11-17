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

  MagickCore private methods shared by the histogram segmentation methods:
  the scale-space, zero-crossing and interval-tree core of SegmentImage and
  GetImageDynamicThreshold.
*/
#ifndef MAGICKCORE_SEGMENT_PRIVATE_H
#define MAGICKCORE_SEGMENT_PRIVATE_H

#include "MagickCore/cache-view.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/pixel-accessor.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
  Define declarations.
*/
#define MaxDimension  3
#define DeltaTau  0.5f
#define Tau  5.2f
#define SegmentImageTag  "Segment/Image"

/*
  Typedef declarations.
*/
typedef struct _ExtentPacket
{
  double
    center;

  ssize_t
    index,
    left,
    right;
} ExtentPacket;

typedef struct _Cluster
{
  struct _Cluster
    *next;

  ExtentPacket
    red,
    green,
    blue;

  ssize_t
    count,
    id;
} Cluster;

typedef struct _IntervalTree
{
  double
    tau;

  ssize_t
    left,
    right;

  double
    mean_stability,
    stability;

  struct _IntervalTree
    *sibling,
    *child;
} IntervalTree;

typedef struct _ZeroCrossing
{
  double
    tau,
    histogram[256];

  short
    crossings[256];
} ZeroCrossing;

typedef struct _ZeroCrossingInputs
{
  double
    max_tau,
    min_tau,
    delta_tau,
    smooth_threshold;
} ZeroCrossingInputs;

/*
  Constant declarations.
*/
static const int
  Blue = 2,
  Green = 1,
  Red = 0,
  SafeMargin = 3,
  TreeLength = 600;

/*
  Method prototypes.
*/
static double
  OptimalTau(const ssize_t *,const ZeroCrossingInputs *,short *);

static ssize_t
  DefineRegion(const short *,ExtentPacket *);

static void
  FreeNodes(IntervalTree *),
  InitializeHistogram(const Image *,ssize_t **,ExceptionInfo *),
  ScaleSpace(const ssize_t *,const double,double *),
  ZeroCrossHistogram(double *,const double,short *);


static inline MagickBooleanType IsPixelInCluster(const Cluster *cluster,
  const double red,const double green,const double blue)
{
  return(((red >= (double) (cluster->red.left-SafeMargin)) &&
    (red <= (double) (cluster->red.right+SafeMargin)) &&
    (green >= (double) (cluster->green.left-SafeMargin)) &&
    (green <= (double) (cluster->green.right+SafeMargin)) &&
    (blue >= (double) (cluster->blue.left-SafeMargin)) &&
    (blue <= (double) (cluster->blue.right+SafeMargin))) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsClusterAboveThreshold(const Cluster *cluster,
  const ssize_t count,const double cluster_threshold)
{
  return(((cluster->count > 0) &&
    (cluster->count >= (count*cluster_threshold/100.0))) ? MagickTrue :
    MagickFalse);
}

static inline Cluster *UnlinkCluster(Cluster *head,Cluster *last_cluster,
  const Cluster *cluster,Cluster *next_cluster)
{
  if (cluster == head)
    head=next_cluster;
  else
    last_cluster->next=next_cluster;
  return(head);
}

static inline void InitializeCluster(Cluster *cluster,const ssize_t count)
{
  cluster->id=count;
  cluster->red.center/=cluster->count;
  cluster->green.center/=cluster->count;
  cluster->blue.center/=cluster->count;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C o n s o l i d a t e C r o s s i n g s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConsolidateCrossings() guarantees that an even number of zero crossings
%  always lie between two crossings.
%
%  The format of the ConsolidateCrossings method is:
%
%      ConsolidateCrossings(ZeroCrossing *zero_crossing,
%        const size_t number_crossings)
%
%  A description of each parameter follows.
%
%    o zero_crossing: Specifies an array of structures of type ZeroCrossing.
%
%    o number_crossings: This size_t specifies the number of elements
%      in the zero_crossing array.
%
*/
static inline ssize_t CountCrossingsBetween(const ZeroCrossing *zero_crossing,
  const ssize_t i,const ssize_t k,const ssize_t limit)
{
  ssize_t
    count,
    l;

  count=0;
  for (l=k+1; l < limit; l++)
    if (zero_crossing[i+1].crossings[l] != 0)
      count++;
  return(count);
}

static inline MagickBooleanType IsEvenCrossing(const ZeroCrossing *zero_crossing,
  const ssize_t i,const ssize_t k,const ssize_t candidate)
{
  ssize_t
    count;

  count=CountCrossingsBetween(zero_crossing,i,k,candidate);
  return((((count % 2) == 0) && (candidate != k)) ? MagickTrue : MagickFalse);
}

static inline ssize_t FindCrossingBelow(const ZeroCrossing *zero_crossing,
  const ssize_t i,const ssize_t j)
{
  ssize_t
    k;

  for (k=j-1; k > 0; k--)
    if (zero_crossing[i].crossings[k] != 0)
      break;
  return(k);
}

static inline ssize_t FindCrossingAbove(const ZeroCrossing *zero_crossing,
  const ssize_t i,const ssize_t j)
{
  ssize_t
    k;

  for (k=j+1; k < 255; k++)
    if (zero_crossing[i+1].crossings[k] != 0)
      break;
  return(k);
}

static inline void MoveCrossing(ZeroCrossing *zero_crossing,const ssize_t i,
  const ssize_t j,const ssize_t correct)
{
  ssize_t
    l;

  l=(ssize_t) zero_crossing[i].crossings[j];
  zero_crossing[i].crossings[j]=0;
  if (correct != -1)
    zero_crossing[i].crossings[correct]=(short) l;
}

typedef struct _CrossingCandidates
{
  const ZeroCrossing
    *zero_crossing;

  ssize_t
    i,
    j,
    k,
    center,
    left,
    right;
} CrossingCandidates;

static inline ssize_t SelectEvenCrossing(const CrossingCandidates *candidates)
{
  const ZeroCrossing
    *const zero_crossing = candidates->zero_crossing;

  const ssize_t
    i = candidates->i,
    j = candidates->j,
    k = candidates->k,
    center = candidates->center,
    left = candidates->left,
    right = candidates->right;

  ssize_t
    correct;

  /*
    Check center for an even number of crossings between k and j.
  */
  correct=(-1);
  if (zero_crossing[i+1].crossings[j] != 0)
    if (IsEvenCrossing(zero_crossing,i,k,center) != MagickFalse)
      correct=center;
  /*
    Check left for an even number of crossings between k and j.
  */
  if (correct != -1)
    return(correct);
  if (IsEvenCrossing(zero_crossing,i,k,left) != MagickFalse)
    correct=left;
  /*
    Check right for an even number of crossings between k and j.
  */
  if (correct != -1)
    return(correct);
  if (IsEvenCrossing(zero_crossing,i,k,right) != MagickFalse)
    correct=right;
  return(correct);
}

static inline void ConsolidateCrossings(ZeroCrossing *zero_crossing,
  const size_t number_crossings)
{
  ssize_t
    i,
    j,
    k;

  ssize_t
    center,
    correct,
    left,
    right;

  CrossingCandidates
    candidates;

  /*
    Consolidate zero crossings.
  */
  for (i=(ssize_t) number_crossings-1; i >= 0; i--)
    for (j=0; j <= 255; j++)
    {
      if (zero_crossing[i].crossings[j] == 0)
        continue;
      /*
        Find the entry that is closest to j and still preserves the
        property that there are an even number of crossings between
        intervals.
      */
      k=FindCrossingBelow(zero_crossing,i+1,j);
      left=MagickMax(k,0);
      center=j;
      k=FindCrossingAbove(zero_crossing,i,j);
      right=MagickMin(k,255);
      /*
        K is the zero crossing just left of j.
      */
      k=FindCrossingBelow(zero_crossing,i,j);
      if (k < 0)
        k=0;
      candidates.zero_crossing=zero_crossing;
      candidates.i=i;
      candidates.j=j;
      candidates.k=k;
      candidates.center=center;
      candidates.left=left;
      candidates.right=right;
      correct=SelectEvenCrossing(&candidates);
      MoveCrossing(zero_crossing,i,j,correct);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e f i n e R e g i o n                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DefineRegion() defines the left and right boundaries of a peak region.
%
%  The format of the DefineRegion method is:
%
%      ssize_t DefineRegion(const short *extrema,ExtentPacket *extents)
%
%  A description of each parameter follows.
%
%    o extrema:  Specifies a pointer to an array of integers.  They
%      represent the peaks and valleys of the histogram for each color
%      component.
%
%    o extents:  This pointer to an ExtentPacket represent the extends
%      of a particular peak or valley of a color component.
%
*/
static inline void SkipToRegionMinimum(const short *extrema,ExtentPacket *extents)
{
  for ( ; extents->index <= 255; extents->index++)
    if (extrema[extents->index] < 0)
      break;
}

static inline ssize_t DefineRegion(const short *extrema,ExtentPacket *extents)
{
  /*
    Initialize to default values.
  */
  extents->left=0;
  extents->center=0.0;
  extents->right=255;
  /*
    Find the left side (maxima).
  */
  for ( ; extents->index <= 255; extents->index++)
    if (extrema[extents->index] > 0)
      break;
  if (extents->index > 255)
    return(MagickFalse);  /* no left side - no region exists */
  extents->left=extents->index;
  /*
    Find the right side (minima).
  */
  SkipToRegionMinimum(extrema,extents);
  extents->right=extents->index-1;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e r i v a t i v e H i s t o g r a m                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DerivativeHistogram() determines the derivative of the histogram using
%  central differencing.
%
%  The format of the DerivativeHistogram method is:
%
%      DerivativeHistogram(const double *histogram,
%        double *derivative)
%
%  A description of each parameter follows.
%
%    o histogram: Specifies an array of doubles representing the number
%      of pixels for each intensity of a particular color component.
%
%    o derivative: This array of doubles is initialized by
%      DerivativeHistogram to the derivative of the histogram using central
%      differencing.
%
*/
static inline void DerivativeHistogram(const double *histogram,
  double *derivative)
{
  ssize_t
    i,
    n;

  /*
    Compute endpoints using second order polynomial interpolation.
  */
  n=255;
  derivative[0]=(-1.5*histogram[0]+2.0*histogram[1]-0.5*histogram[2]);
  derivative[n]=(0.5*histogram[n-2]-2.0*histogram[n-1]+1.5*histogram[n]);
  /*
    Compute derivative using central differencing.
  */
  for (i=1; i < n; i++)
    derivative[i]=(histogram[i+1]-histogram[i-1])/2.0;
  return;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  I n i t i a l i z e H i s t o g r a m                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeHistogram() computes the histogram for an image.
%
%  The format of the InitializeHistogram method is:
%
%      InitializeHistogram(const Image *image,ssize_t **histogram)
%
%  A description of each parameter follows.
%
%    o image: Specifies a pointer to an Image structure;  returned from
%      ReadImage.
%
%    o histogram: Specifies an array of integers representing the number
%      of pixels for each intensity of a particular color component.
%
*/
static inline void InitializeHistogram(const Image *image,ssize_t **histogram,
  ExceptionInfo *exception)
{
  const Quantum
    *p;

  ssize_t
    i,
    x;

  ssize_t
    y;

  /*
    Initialize histogram.
  */
  for (i=0; i <= 255; i++)
  {
    histogram[Red][i]=0;
    histogram[Green][i]=0;
    histogram[Blue][i]=0;
  }
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    p=GetVirtualPixels(image,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      histogram[Red][(ssize_t) ScaleQuantumToChar(GetPixelRed(image,p))]++;
      histogram[Green][(ssize_t) ScaleQuantumToChar(GetPixelGreen(image,p))]++;
      histogram[Blue][(ssize_t) ScaleQuantumToChar(GetPixelBlue(image,p))]++;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e I n t e r v a l T r e e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeIntervalTree() initializes an interval tree from the lists of
%  zero crossings.
%
%  The format of the InitializeIntervalTree method is:
%
%      InitializeIntervalTree(IntervalTree **list,ssize_t *number_nodes,
%        IntervalTree *node)
%
%  A description of each parameter follows.
%
%    o zero_crossing: Specifies an array of structures of type ZeroCrossing.
%
%    o number_crossings: This size_t specifies the number of elements
%      in the zero_crossing array.
%
*/

static inline void InitializeList(IntervalTree **list,ssize_t *number_nodes,
  IntervalTree *node)
{
  if (node == (IntervalTree *) NULL)
    return;
  if (node->child == (IntervalTree *) NULL)
    list[(*number_nodes)++]=node;
  InitializeList(list,number_nodes,node->sibling);
  InitializeList(list,number_nodes,node->child);
}

static inline void MeanStability(IntervalTree *node)
{
  IntervalTree
    *child;

  if (node == (IntervalTree *) NULL)
    return;
  node->mean_stability=0.0;
  child=node->child;
  if (child != (IntervalTree *) NULL)
    {
      ssize_t
        count;

      double
        sum;

      sum=0.0;
      count=0;
      for ( ; child != (IntervalTree *) NULL; child=child->sibling)
      {
        sum+=child->stability;
        count++;
      }
      node->mean_stability=sum/(double) count;
    }
  MeanStability(node->sibling);
  MeanStability(node->child);
}

static inline void Stability(IntervalTree *node)
{
  if (node == (IntervalTree *) NULL)
    return;
  if (node->child == (IntervalTree *) NULL)
    node->stability=0.0;
  else
    node->stability=node->tau-(node->child)->tau;
  Stability(node->sibling);
  Stability(node->child);
}

static inline IntervalTree *AppendIntervalNode(const IntervalTree *head,
  IntervalTree *node)
{
  if (node == head)
    {
      node->child=(IntervalTree *) AcquireQuantumMemory(1,
        sizeof(*node->child));
      node=node->child;
    }
  else
    {
      node->sibling=(IntervalTree *) AcquireQuantumMemory(1,
        sizeof(*node->sibling));
      node=node->sibling;
    }
  return(node);
}

static inline IntervalTree *AcquireRootInterval(void)
{
  IntervalTree
    *root;

  root=(IntervalTree *) AcquireCriticalMemory(sizeof(*root));
  root->child=(IntervalTree *) NULL;
  root->sibling=(IntervalTree *) NULL;
  root->tau=0.0;
  root->left=0;
  root->right=255;
  root->mean_stability=0.0;
  root->stability=0.0;
  return(root);
}

typedef struct _IntervalSplit
{
  const ZeroCrossing
    *zero_crossing;

  IntervalTree
    *head,
    ***list,
    *root;

  ssize_t
    i,
    left;
} IntervalSplit;

static inline MagickBooleanType AppendLastInterval(const IntervalSplit *split,
  IntervalTree **node)
{
  if (split->left != split->head->left)
    {
      (*node)->sibling=(IntervalTree *) AcquireQuantumMemory(1,
        sizeof(*(*node)->sibling));
      *node=(*node)->sibling;
      if (*node == (IntervalTree *) NULL)
        {
          *split->list=(IntervalTree **) RelinquishMagickMemory(*split->list);
          FreeNodes(split->root);
          return(MagickFalse);
        }
      (*node)->tau=split->zero_crossing[split->i+1].tau;
      (*node)->child=(IntervalTree *) NULL;
      (*node)->sibling=(IntervalTree *) NULL;
      (*node)->left=split->left;
      (*node)->right=split->head->right;
    }
  return(MagickTrue);
}

static inline MagickBooleanType SplitIntervalList(const ZeroCrossing *zero_crossing,
  const ssize_t i,IntervalTree *root,IntervalTree ***list)
{
  IntervalTree
    *head,
    *node;

  ssize_t
    j,
    k,
    left,
    number_nodes;

  /*
    Initialize list with all nodes with no children.
  */
  number_nodes=0;
  InitializeList(*list,&number_nodes,root);
  /*
    Split list.
  */
  for (j=0; j < number_nodes; j++)
  {
    IntervalSplit
      split;

    head=(*list)[j];
    left=head->left;
    node=head;
    for (k=head->left+1; k < head->right; k++)
    {
      if (zero_crossing[i+1].crossings[k] == 0)
        continue;
      node=AppendIntervalNode(head,node);
      if (node == (IntervalTree *) NULL)
        {
          *list=(IntervalTree **) RelinquishMagickMemory(*list);
          FreeNodes(root);
          return(MagickFalse);
        }
      node->tau=zero_crossing[i+1].tau;
      node->child=(IntervalTree *) NULL;
      node->sibling=(IntervalTree *) NULL;
      node->left=left;
      node->right=k;
      left=k;
    }
    split.zero_crossing=zero_crossing;
    split.head=head;
    split.list=list;
    split.root=root;
    split.i=i;
    split.left=left;
    if (AppendLastInterval(&split,&node) == MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

static inline IntervalTree *InitializeIntervalTree(const ZeroCrossing *zero_crossing,
  const size_t number_crossings)
{
  IntervalTree
    **list,
    *root;

  ssize_t
    i;

  /*
    Allocate interval tree.
  */
  list=(IntervalTree **) AcquireQuantumMemory((size_t) TreeLength,
    sizeof(*list));
  if (list == (IntervalTree **) NULL)
    return((IntervalTree *) NULL);
  /*
    The root is the entire histogram.
  */
  root=AcquireRootInterval();
  (void) memset(list,0,(size_t) TreeLength*sizeof(*list));
  for (i=(-1); i < (ssize_t) number_crossings; i++)
  {
    if (SplitIntervalList(zero_crossing,i,root,&list) == MagickFalse)
      return((IntervalTree *) NULL);
  }
  /*
    Determine the stability: difference between a nodes tau and its child.
  */
  Stability(root->child);
  MeanStability(root->child);
  list=(IntervalTree **) RelinquishMagickMemory(list);
  return(root);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   O p t i m a l T a u                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OptimalTau() finds the optimal tau for each band of the histogram.
%
%  The format of the OptimalTau method is:
%
%    double OptimalTau(const ssize_t *histogram,const double max_tau,
%      const double min_tau,const double delta_tau,
%      const double smooth_threshold,short *extrema)
%
%  A description of each parameter follows.
%
%    o histogram: Specifies an array of integers representing the number
%      of pixels for each intensity of a particular color component.
%
%    o extrema:  Specifies a pointer to an array of integers.  They
%      represent the peaks and valleys of the histogram for each color
%      component.
%
*/

static inline void ActiveNodes(IntervalTree **list,ssize_t *number_nodes,
  IntervalTree *node)
{
  if (node == (IntervalTree *) NULL)
    return;
  if (node->stability >= node->mean_stability)
    {
      list[(*number_nodes)++]=node;
      ActiveNodes(list,number_nodes,node->sibling);
    }
  else
    {
      ActiveNodes(list,number_nodes,node->sibling);
      ActiveNodes(list,number_nodes,node->child);
    }
}

static inline void FreeNodes(IntervalTree *node)
{
  if (node == (IntervalTree *) NULL)
    return;
  FreeNodes(node->sibling);
  FreeNodes(node->child);
  node=(IntervalTree *) RelinquishMagickMemory(node);
}

static inline ssize_t FindFirstCrossing(const ZeroCrossing *zero_crossing,
  const ssize_t i)
{
  ssize_t
    j;

  for (j=0; j < 255; j++)
    if (zero_crossing[i].crossings[j] != 0)
      break;
  return(j);
}

static inline void ForceCrossingEndpoints(ZeroCrossing *zero_crossing,
  const size_t number_crossings)
{
  ssize_t
    i,
    j;

  for (i=0; i <= (ssize_t) number_crossings; i++)
  {
    j=FindFirstCrossing(zero_crossing,i);
    zero_crossing[i].crossings[0]=(-zero_crossing[i].crossings[j]);
    for (j=255; j > 0; j--)
      if (zero_crossing[i].crossings[j] != 0)
        break;
    zero_crossing[i].crossings[255]=(-zero_crossing[i].crossings[j]);
  }
}

static inline ssize_t FindPeakIndex(const ZeroCrossing *zero_crossing,
  const ssize_t k,const IntervalTree *node,const MagickBooleanType peak)
{
  double
    value;

  ssize_t
    index,
    x;

  index=node->left;
  value=zero_crossing[k].histogram[index];
  for (x=node->left; x <= node->right; x++)
  {
    if (peak != MagickFalse)
      {
        if (zero_crossing[k].histogram[x] > value)
          {
            value=zero_crossing[k].histogram[x];
            index=x;
          }
      }
    else
      if (zero_crossing[k].histogram[x] < value)
        {
          value=zero_crossing[k].histogram[x];
          index=x;
        }
  }
  return(index);
}

static inline void SetNodeExtrema(short *extrema,const IntervalTree *node,
  ssize_t index,const MagickBooleanType peak)
{
  ssize_t
    x;

  for (x=node->left; x <= node->right; x++)
  {
    if (index == 0)
      index=256;
    if (peak != MagickFalse)
      extrema[x]=(short) index;
    else
      extrema[x]=(short) (-index);
  }
}

static inline double GetAverageTau(IntervalTree **list,const ssize_t number_nodes)
{
  double
    average_tau;

  ssize_t
    i;

  average_tau=0.0;
  for (i=0; i < number_nodes; i++)
    average_tau+=list[i]->tau;
  average_tau*=MagickSafeReciprocal((double) number_nodes);
  return(average_tau);
}

static inline ssize_t FindTauIndex(const ZeroCrossing *zero_crossing,
  const size_t number_crossings,const IntervalTree *node,ssize_t k)
{
  ssize_t
    j;

  for (j=0; j <= (ssize_t) number_crossings; j++)
    if (zero_crossing[j].tau == node->tau)
      k=j;
  return(k);
}

typedef struct _ActiveNodeInputs
{
  IntervalTree
    **list;

  ssize_t
    number_nodes;

  ZeroCrossing
    *zero_crossing;

  size_t
    number_crossings;
} ActiveNodeInputs;

static inline void SetActiveNodeExtrema(const ActiveNodeInputs *inputs,
  short *extrema)
{
  IntervalTree
    **const list = inputs->list;

  const ssize_t
    number_nodes = inputs->number_nodes;

  ZeroCrossing
    *const zero_crossing = inputs->zero_crossing;

  const size_t
    number_crossings = inputs->number_crossings;

  IntervalTree
    *node;

  MagickBooleanType
    peak;

  ssize_t
    i,
    index,
    k;

  for (i=0; i <= 255; i++)
    extrema[i]=0;
  for (i=0; i < number_nodes; i++)
  {
    /*
      Find this tau in zero crossings list.
    */
    k=0;
    node=list[i];
    k=FindTauIndex(zero_crossing,number_crossings,node,k);
    /*
      Find the value of the peak.
    */
    peak=zero_crossing[k].crossings[node->right] == -1 ? MagickTrue :
      MagickFalse;
    index=FindPeakIndex(zero_crossing,k,node,peak);
    SetNodeExtrema(extrema,node,index,peak);
  }
}

static inline size_t InitializeZeroCrossings(const ssize_t *histogram,
  const ZeroCrossingInputs *crossing_inputs,ZeroCrossing *zero_crossing)
{
  const double
    max_tau = crossing_inputs->max_tau,
    min_tau = crossing_inputs->min_tau,
    delta_tau = crossing_inputs->delta_tau,
    smooth_threshold = crossing_inputs->smooth_threshold;

  double
    *derivative,
    *second_derivative,
    tau;

  size_t
    number_crossings;

  ssize_t
    i,
    j;

  derivative=(double *) AcquireCriticalMemory(256*sizeof(*derivative));
  second_derivative=(double *) AcquireCriticalMemory(256*
    sizeof(*second_derivative));
  i=0;
  for (tau=max_tau; tau >= min_tau; tau-=delta_tau)
  {
    zero_crossing[i].tau=tau;
    ScaleSpace(histogram,tau,zero_crossing[i].histogram);
    DerivativeHistogram(zero_crossing[i].histogram,derivative);
    DerivativeHistogram(derivative,second_derivative);
    ZeroCrossHistogram(second_derivative,smooth_threshold,
      zero_crossing[i].crossings);
    i++;
  }
  /*
    Add an entry for the original histogram.
  */
  zero_crossing[i].tau=0.0;
  for (j=0; j <= 255; j++)
    zero_crossing[i].histogram[j]=(double) histogram[j];
  DerivativeHistogram(zero_crossing[i].histogram,derivative);
  DerivativeHistogram(derivative,second_derivative);
  ZeroCrossHistogram(second_derivative,smooth_threshold,
    zero_crossing[i].crossings);
  number_crossings=(size_t) i;
  derivative=(double *) RelinquishMagickMemory(derivative);
  second_derivative=(double *) RelinquishMagickMemory(second_derivative);
  return(number_crossings);
}

static inline double OptimalTau(const ssize_t *histogram,
  const ZeroCrossingInputs *tau_inputs,short *extrema)
{
  const double
    max_tau = tau_inputs->max_tau,
    min_tau = tau_inputs->min_tau,
    delta_tau = tau_inputs->delta_tau,
    smooth_threshold = tau_inputs->smooth_threshold;

  double
    average_tau;

  ActiveNodeInputs
    inputs;

  ZeroCrossingInputs
    crossing_inputs;

  IntervalTree
    **list,
    *root;

  ssize_t
    i;

  size_t
    count,
    number_crossings;

  ssize_t
    number_nodes;

  ZeroCrossing
    *zero_crossing;

  /*
    Allocate interval tree.
  */
  list=(IntervalTree **) AcquireQuantumMemory((size_t) TreeLength,
    sizeof(*list));
  if (list == (IntervalTree **) NULL)
    return(0.0);
  /*
    Allocate zero crossing list.
  */
  count=(size_t) ((max_tau-min_tau)/delta_tau)+2;
  zero_crossing=(ZeroCrossing *) AcquireQuantumMemory((size_t) count,
    sizeof(*zero_crossing));
  if (zero_crossing == (ZeroCrossing *) NULL)
    {
      list=(IntervalTree **) RelinquishMagickMemory(list);
      return(0.0);
    }
  for (i=0; i < (ssize_t) count; i++)
    zero_crossing[i].tau=(-1.0);
  /*
    Initialize zero crossing list.
  */
  crossing_inputs.max_tau=max_tau;
  crossing_inputs.min_tau=min_tau;
  crossing_inputs.delta_tau=delta_tau;
  crossing_inputs.smooth_threshold=smooth_threshold;
  number_crossings=InitializeZeroCrossings(histogram,&crossing_inputs,
    zero_crossing);
  /*
    Ensure the scale-space fingerprints form lines in scale-space, not loops.
  */
  ConsolidateCrossings(zero_crossing,number_crossings);
  /*
    Force endpoints to be included in the interval.
  */
  ForceCrossingEndpoints(zero_crossing,number_crossings);
  /*
    Initialize interval tree.
  */
  root=InitializeIntervalTree(zero_crossing,number_crossings);
  if (root == (IntervalTree *) NULL)
    {
      zero_crossing=(ZeroCrossing *) RelinquishMagickMemory(zero_crossing);
      list=(IntervalTree **) RelinquishMagickMemory(list);
      return(0.0);
    }
  /*
    Find active nodes: Stability is greater (or equal) to the mean stability of
    its children.
  */
  number_nodes=0;
  ActiveNodes(list,&number_nodes,root->child);
  /*
    Initialize extrema.
  */
  inputs.list=list;
  inputs.number_nodes=number_nodes;
  inputs.zero_crossing=zero_crossing;
  inputs.number_crossings=number_crossings;
  SetActiveNodeExtrema(&inputs,extrema);
  /*
    Determine the average tau.
  */
  average_tau=GetAverageTau(list,number_nodes);
  /*
    Relinquish resources.
  */
  FreeNodes(root);
  zero_crossing=(ZeroCrossing *) RelinquishMagickMemory(zero_crossing);
  list=(IntervalTree **) RelinquishMagickMemory(list);
  return(average_tau);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S c a l e S p a c e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ScaleSpace() performs a scale-space filter on the 1D histogram.
%
%  The format of the ScaleSpace method is:
%
%      ScaleSpace(const ssize_t *histogram,const double tau,
%        double *scale_histogram)
%
%  A description of each parameter follows.
%
%    o histogram: Specifies an array of doubles representing the number
%      of pixels for each intensity of a particular color component.
%
*/
static inline double ScaleSpaceSum(const ssize_t *histogram,const double *gamma,
  const ssize_t x)
{
  double
    sum;

  ssize_t
    u;

  sum=0.0;
  for (u=0; u <= 255; u++)
    sum+=(double) histogram[u]*gamma[MagickAbsoluteValue(x-u)];
  return(sum);
}

static inline void ScaleSpace(const ssize_t *histogram,const double tau,
  double *scale_histogram)
{
  double
    alpha,
    beta,
    *gamma,
    sum;

  ssize_t
    x;

  gamma=(double *) AcquireQuantumMemory(256,sizeof(*gamma));
  if (gamma == (double *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"UnableToAllocateGammaMap");
  alpha=MagickSafeReciprocal(tau*sqrt(2.0*MagickPI));
  beta=(-1.0*MagickSafeReciprocal(2.0*tau*tau));
  for (x=0; x <= 255; x++)
    gamma[x]=0.0;
  for (x=0; x <= 255; x++)
  {
    gamma[x]=exp((double) beta*x*x);
    if (gamma[x] < MagickEpsilon)
      break;
  }
  for (x=0; x <= 255; x++)
  {
    sum=ScaleSpaceSum(histogram,gamma,x);
    scale_histogram[x]=alpha*sum;
  }
  gamma=(double *) RelinquishMagickMemory(gamma);
}

static inline void MergeSmallDerivatives(double *second_derivative,
  const double smooth_threshold)
{
  ssize_t
    i;

  for (i=0; i <= 255; i++)
    if ((second_derivative[i] < smooth_threshold) &&
        (second_derivative[i] >= -smooth_threshold))
      second_derivative[i]=0.0;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   Z e r o C r o s s H i s t o g r a m                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ZeroCrossHistogram() find the zero crossings in a histogram and marks
%  directions as:  1 is negative to positive; 0 is zero crossing; and -1
%  is positive to negative.
%
%  The format of the ZeroCrossHistogram method is:
%
%      ZeroCrossHistogram(double *second_derivative,
%        const double smooth_threshold,short *crossings)
%
%  A description of each parameter follows.
%
%    o second_derivative: Specifies an array of doubles representing the
%      second derivative of the histogram of a particular color component.
%
%    o crossings:  This array of integers is initialized with
%      -1, 0, or 1 representing the slope of the first derivative of the
%      of a particular color component.
%
*/
static inline ssize_t MarkFallingCrossing(short *crossings,const ssize_t i,
  ssize_t parity)
{
  if (parity > 0)
    crossings[i]=(-1);
  parity=1;
  return(parity);
}

static inline ssize_t MarkRisingCrossing(short *crossings,const ssize_t i,
  ssize_t parity)
{
  if (parity < 0)
    crossings[i]=1;
  parity=(-1);
  return(parity);
}

static inline void ZeroCrossHistogram(double *second_derivative,
  const double smooth_threshold,short *crossings)
{
  ssize_t
    i;

  ssize_t
    parity;

  /*
    Merge low numbers to zero to help prevent noise.
  */
  MergeSmallDerivatives(second_derivative,smooth_threshold);
  /*
    Mark zero crossings.
  */
  parity=0;
  for (i=0; i <= 255; i++)
  {
    crossings[i]=0;
    if (second_derivative[i] < 0.0)
      parity=MarkFallingCrossing(crossings,i,parity);
    else
      if (second_derivative[i] > 0.0)
        parity=MarkRisingCrossing(crossings,i,parity);
  }
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
