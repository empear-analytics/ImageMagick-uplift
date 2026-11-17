/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               SSSSS  EEEEE   GGGG  M   M  EEEEE  N   N  TTTTT               %
%               SS     E      G      MM MM  E      NN  N    T                 %
%                SSS   EEE    G  GG  M M M  EEE    N N N    T                 %
%                  SS  E      G   G  M   M  E      N  NN    T                 %
%               SSSSS  EEEEE   GGG   M   M  EEEEE  N   N    T                 %
%                                                                             %
%        TTTTT  H   H  RRRR   EEEEE  SSSSS  H   H   OOO   L      DDDD         %
%          T    H   H  R   R  E      SS     H   H  O   O  L      D   D        %
%          T    HHHHH  RRRR   EEE     SSS   HHHHH  O   O  L      D   D        %
%          T    H   H  R R    E         SS  H   H  O   O  L      D   D        %
%          T    H   H  R  R   EEEEE  SSSSS  H   H   OOO   LLLLL  DDDD         %
%                                                                             %
%                                                                             %
%                    MagickCore Dynamic Threshold Methods                     %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                April 1993                                   %
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
%  Segment segments an image by analyzing the histograms of the color
%  components and identifying units that are homogeneous with the fuzzy
%  c-means technique.  The scale-space filter analyzes the histograms of
%  the three color components of the image and identifies a set of
%  classes.  The extents of each class is used to coarsely segment the
%  image with thresholding.  The color associated with each class is
%  determined by the mean color of all pixels within the extents of a
%  particular class.  Finally, any unclassified pixels are assigned to
%  the closest class with the fuzzy c-means technique.
%
%  The fuzzy c-Means algorithm can be summarized as follows:
%
%    o Build a histogram, one for each color component of the image.
%
%    o For each histogram, successively apply the scale-space filter and
%      build an interval tree of zero crossings in the second derivative
%      at each scale.  Analyze this scale-space ''fingerprint'' to
%      determine which peaks and valleys in the histogram are most
%      predominant.
%
%    o The fingerprint defines intervals on the axis of the histogram.
%      Each interval contains either a minima or a maxima in the original
%      signal.  If each color component lies within the maxima interval,
%      that pixel is considered ''classified'' and is assigned an unique
%      class number.
%
%    o Any pixel that fails to be classified in the above thresholding
%      pass is classified using the fuzzy c-Means technique.  It is
%      assigned to one of the classes discovered in the histogram analysis
%      phase.
%
%  The fuzzy c-Means technique attempts to cluster a pixel by finding
%  the local minima of the generalized within group sum of squared error
%  objective function.  A pixel is assigned to the closest class of
%  which the fuzzy membership has a maximum value.
%
%  Segment is strongly based on software written by Andy Gallo,
%  University of Delaware.
%
%  The following reference was used in creating this program:
%
%    Young Won Lim, Sang Uk Lee, "On The Color Image Segmentation
%    Algorithm Based on the Thresholding and the Fuzzy c-Means
%    Techniques", Pattern Recognition, Volume 23, Number 9, pages
%    935-952, 1990.
%
%
*/

#include "MagickCore/studio.h"
#include "MagickCore/cache.h"
#include "MagickCore/color.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/string_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/segment-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  G e t I m a g e D y n a m i c T h r e s h o l d                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageDynamicThreshold() returns the dynamic threshold for an image.
%
%  The format of the GetImageDynamicThreshold method is:
%
%      MagickBooleanType GetImageDynamicThreshold(const Image *image,
%        const double cluster_threshold,const double smooth_threshold,
%        PixelInfo *pixel,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o cluster_threshold:  This double represents the minimum number of
%      pixels contained in a hexahedra before it can be considered valid
%      (expressed as a percentage).
%
%    o smooth_threshold: the smoothing threshold eliminates noise in the second
%      derivative of the histogram.  As the value is increased, you can expect a
%      smoother second derivative.
%
%    o pixel: return the dynamic threshold here.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType HasObjectAndBackgroundClusters(
  const ssize_t count,const Cluster *head)
{
  return(((count > 1) && (head != (Cluster *) NULL) &&
    (head->next != (Cluster *) NULL)) ? MagickTrue : MagickFalse);
}

static Cluster *FindObjectCluster(Cluster *head)
{
  Cluster
    *object,
    *cluster;

  object=head->next;
  for (cluster=object; cluster->next != (Cluster *) NULL; )
  {
    if (cluster->count < object->count)
      object=cluster;
    cluster=cluster->next;
  }
  return(object);
}

static ssize_t CountRowPixelsByRGB(const Image *image,Cluster *head,
  const Quantum *p,ssize_t count)
{
  Cluster
    *cluster;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    double
      b,
      g,
      r;

    r=(double) ScaleQuantumToChar(GetPixelRed(image,p));
    g=(double) ScaleQuantumToChar(GetPixelGreen(image,p));
    b=(double) ScaleQuantumToChar(GetPixelBlue(image,p));
    for (cluster=head; cluster != (Cluster *) NULL; cluster=cluster->next)
      if (IsPixelInCluster(cluster,r,g,b) != MagickFalse)
        {
          /*
            Count this pixel.
          */
          count++;
          cluster->red.center+=r;
          cluster->green.center+=g;
          cluster->blue.center+=b;
          cluster->count++;
          break;
        }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(count);
}

static ssize_t CountImagePixelsByRGB(const Image *image,Cluster *head,
  ssize_t count,ExceptionInfo *exception)
{
  const Quantum
    *p;

  MagickBooleanType
    proceed;

  ssize_t
    y;

  for (y=0; y < (ssize_t) image->rows; y++)
  {
    p=GetVirtualPixels(image,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    count=CountRowPixelsByRGB(image,head,p,count);
    proceed=SetImageProgress(image,SegmentImageTag,(MagickOffsetType) y,
      2*image->rows);
    if (proceed == MagickFalse)
      break;
  }
  return(count);
}

static void SetDynamicThresholdPixel(const Cluster *background,
  const Cluster *object,PixelInfo *pixel)
{
  double
    threshold;

  if (background != (Cluster *) NULL)
    {
      threshold=(background->red.center+object->red.center)/2.0;
      pixel->red=(double) ScaleCharToQuantum((unsigned char)
        (threshold+0.5));
      threshold=(background->green.center+object->green.center)/2.0;
      pixel->green=(double) ScaleCharToQuantum((unsigned char)
        (threshold+0.5));
      threshold=(background->blue.center+object->blue.center)/2.0;
      pixel->blue=(double) ScaleCharToQuantum((unsigned char)
        (threshold+0.5));
    }
}

static void RelinquishDynamicThresholdResources(Cluster *head,
  ssize_t **histogram,short **extrema)
{
  Cluster
    *cluster,
    *next_cluster;

  ssize_t
    i;

  for (cluster=head; cluster != (Cluster *) NULL; cluster=next_cluster)
  {
    next_cluster=cluster->next;
    cluster=(Cluster *) RelinquishMagickMemory(cluster);
  }
  for (i=0; i < MaxDimension; i++)
  {
    extrema[i]=(short *) RelinquishMagickMemory(extrema[i]);
    histogram[i]=(ssize_t *) RelinquishMagickMemory(histogram[i]);
  }
}

static void InitializeNewCluster(Cluster *cluster,const ExtentPacket red,
  const ExtentPacket green,const ExtentPacket blue)
{
  cluster->count=0;
  cluster->red=red;
  cluster->green=green;
  cluster->blue=blue;
  cluster->next=(Cluster *) NULL;
}

static Cluster *FindLargestBackground(Cluster *background)
{
  Cluster
    *cluster;

  for (cluster=background; cluster->next != (Cluster *) NULL; )
  {
    if (cluster->count > background->count)
      background=cluster;
    cluster=cluster->next;
  }
  return(background);
}

static void FindDynamicExtrema(ssize_t **histogram,
  const double smooth_threshold,short **extrema)
{
  const ZeroCrossingInputs
    tau_inputs = { Tau, 0.2f, DeltaTau,
      (smooth_threshold == 0.0 ? 1.0 : smooth_threshold) };

  (void) OptimalTau(histogram[Red],&tau_inputs,extrema[Red]);
  (void) OptimalTau(histogram[Green],&tau_inputs,extrema[Green]);
  (void) OptimalTau(histogram[Blue],&tau_inputs,extrema[Blue]);
}

static void RelinquishHistogramsBelow(ssize_t i,ssize_t **histogram,
  short **extrema)
{
  for (i-- ; i >= 0; i--)
  {
    extrema[i]=(short *) RelinquishMagickMemory(extrema[i]);
    histogram[i]=(ssize_t *) RelinquishMagickMemory(histogram[i]);
  }
}

static void ChooseDynamicThreshold(const ssize_t count,Cluster *head,
  PixelInfo *pixel)
{
  Cluster
    *background,
    *object;

  object=head;
  background=head;
  if (HasObjectAndBackgroundClusters(count,head) != MagickFalse)
    {
      object=FindObjectCluster(head);
      background=head->next;
      background=FindLargestBackground(background);
    }
  SetDynamicThresholdPixel(background,object,pixel);
}

static MagickBooleanType ThrowClusterAllocationFailed(const Image *image,
  ExceptionInfo *exception)
{
  (void) ThrowMagickException(exception,GetMagickModule(),
    ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
  return(MagickFalse);
}

static Cluster *AppendCluster(Cluster *cluster)
{
  cluster->next=(Cluster *) AcquireQuantumMemory(1,sizeof(*cluster->next));
  cluster=cluster->next;
  return(cluster);
}

typedef struct _DynamicThresholdTail
{
  Cluster
    *head;

  double
    cluster_threshold;

  PixelInfo
    *pixel;
} DynamicThresholdTail;

static void FinishDynamicThreshold(const DynamicThresholdTail *tail,
  ssize_t **histogram,short **extrema)
{
  Cluster
    *head = tail->head;

  const double
    cluster_threshold = tail->cluster_threshold;

  PixelInfo
    *const pixel = tail->pixel;

  Cluster
    *cluster,
    *last_cluster,
    *next_cluster;

  ssize_t
    count;

  count=0;
  last_cluster=head;
  next_cluster=head;
  for (cluster=head; cluster != (Cluster *) NULL; cluster=next_cluster)
  {
    next_cluster=cluster->next;
    if (IsClusterAboveThreshold(cluster,count,cluster_threshold) != MagickFalse)
      {
        /*
          Initialize cluster.
        */
        InitializeCluster(cluster,count);
        count++;
        last_cluster=cluster;
        continue;
      }
    /*
      Delete cluster.
    */
    head=UnlinkCluster(head,last_cluster,cluster,next_cluster);
    cluster=(Cluster *) RelinquishMagickMemory(cluster);
  }
  ChooseDynamicThreshold(count,head,pixel);
  /*
    Relinquish resources.
  */
  RelinquishDynamicThresholdResources(head,histogram,extrema);
}

typedef struct _DynamicClusterTargets
{
  Cluster
    **cluster,
    **head;

  ExtentPacket
    *green,
    *blue;
} DynamicClusterTargets;

static MagickBooleanType FormDynamicClusters(short **extrema,
  const ExtentPacket red,const DynamicClusterTargets *targets)
{
  Cluster
    **const cluster = targets->cluster,
    **const head = targets->head;

  ExtentPacket
    *const green = targets->green,
    *const blue = targets->blue;

  while (DefineRegion(extrema[Green],green) != 0)
  {
    (*blue).index=0;
    while (DefineRegion(extrema[Blue],blue) != 0)
    {
      /*
        Allocate a new class.
      */
      if (*head != (Cluster *) NULL)
        {
          *cluster=AppendCluster(*cluster);
        }
      else
        {
          *cluster=(Cluster *) AcquireQuantumMemory(1,sizeof(**cluster));
          *head=*cluster;
        }
      if (*cluster == (Cluster *) NULL)
        return(MagickFalse);
      /*
        Initialize a new class.
      */
      InitializeNewCluster(*cluster,red,*green,*blue);
    }
  }
  return(MagickTrue);
}

typedef struct _DynamicRegionTargets
{
  Cluster
    **cluster,
    **head;

  ExtentPacket
    *red,
    *green,
    *blue;
} DynamicRegionTargets;

static MagickBooleanType FormDynamicRegionClusters(short **extrema,
  const DynamicRegionTargets *region)
{
  Cluster
    **const cluster = region->cluster,
    **const head = region->head;

  ExtentPacket
    *const red = region->red,
    *const green = region->green,
    *const blue = region->blue;

  while (DefineRegion(extrema[Red],red) != 0)
  {
    DynamicClusterTargets
      targets;

    (*green).index=0;
    targets.cluster=(cluster);
    targets.head=(head);
    targets.green=(green);
    targets.blue=(blue);
    if (FormDynamicClusters(extrema,*red,&targets) == MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType AcquireDynamicHistograms(const Image *image,
  ssize_t **histogram,short **extrema,ExceptionInfo *exception)
{
  ssize_t
    i;

  for (i=0; i < MaxDimension; i++)
  {
    histogram[i]=(ssize_t *) AcquireQuantumMemory(256UL,sizeof(**histogram));
    extrema[i]=(short *) AcquireQuantumMemory(256UL,sizeof(**histogram));
    if ((histogram[i] == (ssize_t *) NULL) || (extrema[i] == (short *) NULL))
      {
        RelinquishHistogramsBelow(i,histogram,extrema);
        (void) ThrowMagickException(exception,GetMagickModule(),
          ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
        return(MagickFalse);
      }
  }
  return(MagickTrue);
}

MagickExport MagickBooleanType GetImageDynamicThreshold(const Image *image,
  const double cluster_threshold,const double smooth_threshold,
  PixelInfo *pixel,ExceptionInfo *exception)
{
  Cluster
    *cluster,
    *head;

  DynamicThresholdTail
    tail;

  ExtentPacket
    blue,
    green,
    red;

  const DynamicRegionTargets
    region = { &cluster, &head, &red, &green, &blue };

  short
    *extrema[MaxDimension];

  ssize_t
    count,
    *histogram[MaxDimension];

  /*
    Allocate histogram and extrema.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  GetPixelInfo(image,pixel);
  if (AcquireDynamicHistograms(image,histogram,extrema,exception) == MagickFalse)
    return(MagickFalse);
  /*
    Initialize histogram.
  */
  InitializeHistogram(image,histogram,exception);
  FindDynamicExtrema(histogram,smooth_threshold,extrema);
  /*
    Form clusters.
  */
  cluster=(Cluster *) NULL;
  head=(Cluster *) NULL;
  (void) memset(&red,0,sizeof(red));
  (void) memset(&green,0,sizeof(green));
  (void) memset(&blue,0,sizeof(blue));
  if (FormDynamicRegionClusters(extrema,&region) == MagickFalse)
    return(ThrowClusterAllocationFailed(image,exception));
  if (head == (Cluster *) NULL)
    {
      /*
        No classes were identified-- create one.
      */
      cluster=(Cluster *) AcquireQuantumMemory(1,sizeof(*cluster));
      if (cluster == (Cluster *) NULL)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),
            ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
          return(MagickFalse);
        }
      /*
        Initialize a new class.
      */
      InitializeNewCluster(cluster,red,green,blue);
      head=cluster;
    }
  /*
    Count the pixels for each cluster.
  */
  count=0;
  count=CountImagePixelsByRGB(image,head,count,exception);
  /*
    Remove clusters that do not meet minimum cluster threshold.
  */
  tail.head=head;
  tail.cluster_threshold=cluster_threshold;
  tail.pixel=pixel;
  FinishDynamicThreshold(&tail,histogram,extrema);
  return(MagickTrue);
}
