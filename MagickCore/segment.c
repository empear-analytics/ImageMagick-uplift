/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               SSSSS  EEEEE   GGGG  M   M  EEEEE  N   N  TTTTT               %
%               SS     E      G      MM MM  E      NN  N    T                 %
%                SSS   EEE    G GGG  M M M  EEE    N N N    T                 %
%                  SS  E      G   G  M   M  E      N  NN    T                 %
%               SSSSS  EEEEE   GGGG  M   M  EEEEE  N   N    T                 %
%                                                                             %
%                                                                             %
%    MagickCore Methods to Segment an Image with Thresholding Fuzzy c-Means   %
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
  Define declarations.
*/
#if defined(FastClassify)
#define WeightingExponent  2.0
#define SegmentPower(ratio) (ratio)
#else
#define WeightingExponent  2.5
#define SegmentPower(ratio) pow(ratio,(double) (1.0/(weighting_exponent-1.0)));
#endif

/*
  Typedef declarations.
*/
typedef struct _ClassifySettings
{
  double
    cluster_threshold,
    weighting_exponent;

  MagickBooleanType
    verbose;
} ClassifySettings;


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C l a s s i f y                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Classify() defines one or more classes.  Each pixel is thresholded to
%  determine which class it belongs to.  If the class is not identified it is
%  assigned to the closest class based on the fuzzy c-Means technique.
%
%  The format of the Classify method is:
%
%      MagickBooleanType Classify(Image *image,short **extrema,
%        const ClassifySettings *settings,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o extrema:  Specifies a pointer to an array of integers.  They
%      represent the peaks and valleys of the histogram for each color
%      component.
%
%    o settings: the classification settings:
%
%      cluster_threshold:  This double represents the minimum number of
%      pixels contained in a hexahedra before it can be considered valid
%      (expressed as a percentage).
%
%      weighting_exponent: Specifies the membership weighting exponent.
%
%      verbose:  A value greater than zero prints detailed information about
%      the identified classes.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static void PrintClusterStatistics(const Cluster *head,
  const double cluster_threshold,const double weighting_exponent,
  const size_t number_clusters)
{
  const Cluster
    *cluster;

  /*
    Print cluster statistics.
  */
  (void) FormatLocaleFile(stdout,"Fuzzy C-means Statistics\n");
  (void) FormatLocaleFile(stdout,"===================\n\n");
  (void) FormatLocaleFile(stdout,"\tCluster Threshold = %g\n",(double)
    cluster_threshold);
  (void) FormatLocaleFile(stdout,"\tWeighting Exponent = %g\n",(double)
    weighting_exponent);
  (void) FormatLocaleFile(stdout,"\tTotal Number of Clusters = %.17g\n\n",
    (double) number_clusters);
  /*
    Print the total number of points per cluster.
  */
  (void) FormatLocaleFile(stdout,"\n\nNumber of Vectors Per Cluster\n");
  (void) FormatLocaleFile(stdout,"=============================\n\n");
  for (cluster=head; cluster != (Cluster *) NULL; cluster=cluster->next)
    (void) FormatLocaleFile(stdout,"Cluster #%.17g = %.17g\n",(double)
      cluster->id,(double) cluster->count);
  /*
    Print the cluster extents.
  */
  (void) FormatLocaleFile(stdout,
    "\n\n\nCluster Extents:        (Vector Size: %d)\n",MaxDimension);
  (void) FormatLocaleFile(stdout,"================");
  for (cluster=head; cluster != (Cluster *) NULL; cluster=cluster->next)
  {
    (void) FormatLocaleFile(stdout,"\n\nCluster #%.17g\n\n",(double)
      cluster->id);
    (void) FormatLocaleFile(stdout,
      "%.17g-%.17g  %.17g-%.17g  %.17g-%.17g\n",(double)
      cluster->red.left,(double) cluster->red.right,(double)
      cluster->green.left,(double) cluster->green.right,(double)
      cluster->blue.left,(double) cluster->blue.right);
  }
  /*
    Print the cluster center values.
  */
  (void) FormatLocaleFile(stdout,
    "\n\n\nCluster Center Values:        (Vector Size: %d)\n",MaxDimension);
  (void) FormatLocaleFile(stdout,"=====================");
  for (cluster=head; cluster != (Cluster *) NULL; cluster=cluster->next)
  {
    (void) FormatLocaleFile(stdout,"\n\nCluster #%.17g\n\n",(double)
      cluster->id);
    (void) FormatLocaleFile(stdout,"%g  %g  %g\n",(double)
      cluster->red.center,(double) cluster->green.center,(double)
      cluster->blue.center);
  }
  (void) FormatLocaleFile(stdout,"\n");
}

static void SetClusterColormap(Image *image,const Cluster *head)
{
  const Cluster
    *cluster;

  ssize_t
    i;

  i=0;
  for (cluster=head; cluster != (Cluster *) NULL; cluster=cluster->next)
  {
    image->colormap[i].red=(double) ScaleCharToQuantum((unsigned char)
      (cluster->red.center+0.5));
    image->colormap[i].green=(double) ScaleCharToQuantum((unsigned char)
      (cluster->green.center+0.5));
    image->colormap[i].blue=(double) ScaleCharToQuantum((unsigned char)
      (cluster->blue.center+0.5));
    i++;
  }
}

typedef struct _FuzzyWeights
{
  const double
    *squares;

  double
    weighting_exponent;
} FuzzyWeights;

static void ClassifyFuzzyPixel(Image *image,const FuzzyWeights *weights,
  const PixelInfo *pixel,Quantum *magick_restrict q)
{
  const double
    *const squares = weights->squares,
    weighting_exponent = weights->weighting_exponent;

  const PixelInfo
    *magick_restrict p;

  double
    distance_squared,
    local_minima,
    numerator,
    ratio,
    sum;

  ssize_t
    j,
    k;

  /*
    Compute fuzzy membership.
  */
  local_minima=0.0;
  for (j=0; j < (ssize_t) image->colors; j++)
  {
    sum=0.0;
    p=image->colormap+j;
    distance_squared=
      squares[(ssize_t) (pixel->red-ScaleQuantumToChar((const Quantum) p->red))]+
      squares[(ssize_t) (pixel->green-ScaleQuantumToChar((const Quantum) p->green))]+
      squares[(ssize_t) (pixel->blue-ScaleQuantumToChar((const Quantum) p->blue))];
    numerator=distance_squared;
    for (k=0; k < (ssize_t) image->colors; k++)
    {
      p=image->colormap+k;
      distance_squared=
        squares[(ssize_t) (pixel->red-ScaleQuantumToChar((const Quantum) p->red))]+
        squares[(ssize_t) (pixel->green-ScaleQuantumToChar((const Quantum) p->green))]+
        squares[(ssize_t) (pixel->blue-ScaleQuantumToChar((const Quantum) p->blue))];
      ratio=numerator/distance_squared;
      sum+=SegmentPower(ratio);
    }
    if ((sum != 0.0) && ((1.0/sum) > local_minima))
      {
        /*
          Classify this pixel.
        */
        local_minima=1.0/sum;
        SetPixelIndex(image,(Quantum) j,q);
      }
  }
}

static ssize_t CountClusterPixel(Cluster *head,const PixelInfo pixel,
  ssize_t count)
{
  Cluster
    *cluster;

  for (cluster=head; cluster != (Cluster *) NULL; cluster=cluster->next)
    if (IsPixelInCluster(cluster,pixel.red,pixel.green,pixel.blue) !=
        MagickFalse)
      {
        /*
          Count this pixel.
        */
        count++;
        cluster->red.center+=pixel.red;
        cluster->green.center+=pixel.green;
        cluster->blue.center+=pixel.blue;
        cluster->count++;
        break;
      }
  return(count);
}

static ssize_t CountRowClusterPixels(const Image *image,Cluster *head,
  const Quantum *p,ssize_t count)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    PixelInfo
      pixel;

    pixel.red=(double) ScaleQuantumToChar(GetPixelRed(image,p));
    pixel.green=(double) ScaleQuantumToChar(GetPixelGreen(image,p));
    pixel.blue=(double) ScaleQuantumToChar(GetPixelBlue(image,p));
    count=CountClusterPixel(head,pixel,count);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(count);
}

static Cluster *ClassifyPixelByCluster(const Image *image,Cluster *head,
  const PixelInfo pixel,Quantum *magick_restrict q)
{
  Cluster
    *c;

  for (c=head; c != (Cluster *) NULL; c=c->next)
  {
    if (IsPixelInCluster(c,pixel.red,pixel.green,pixel.blue) != MagickFalse)
      {
        /*
          Classify this pixel.
        */
        SetPixelIndex(image,(Quantum) c->id,q);
        break;
      }
  }
  return(c);
}

typedef struct _ClassifyRowInputs
{
  Cluster
    *head;

  double
    *squares;

  double
    weighting_exponent;
} ClassifyRowInputs;

static void ClassifyRowPixels(Image *image,const ClassifyRowInputs *inputs,
  Quantum *magick_restrict q)
{
  Cluster
    *const head = inputs->head;

  double
    *const squares = inputs->squares;

  const double
    weighting_exponent = inputs->weighting_exponent;

  const FuzzyWeights
    weights = { squares, weighting_exponent };

  Cluster
    *c;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    PixelInfo
      pixel;

    SetPixelIndex(image,(Quantum) 0,q);
    pixel.red=(double) ScaleQuantumToChar(GetPixelRed(image,q));
    pixel.green=(double) ScaleQuantumToChar(GetPixelGreen(image,q));
    pixel.blue=(double) ScaleQuantumToChar(GetPixelBlue(image,q));
    c=ClassifyPixelByCluster(image,head,pixel,q);
    if (c == (Cluster *) NULL)
      ClassifyFuzzyPixel(image,&weights,&pixel,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

typedef struct _ClusterTargets
{
  Cluster
    **cluster,
    **head;

  ExtentPacket
    *blue;
} ClusterTargets;

static MagickBooleanType FormBlueClusters(short **extrema,
  const ExtentPacket red,const ExtentPacket green,
  const ClusterTargets *targets)
{
  Cluster
    **const cluster = targets->cluster,
    **const head = targets->head;

  ExtentPacket
    *const blue = targets->blue;

  while (DefineRegion(extrema[Blue],blue) != 0)
  {
    /*
      Allocate a new class.
    */
    if (*head != (Cluster *) NULL)
      {
        (*cluster)->next=(Cluster *) AcquireQuantumMemory(1,
          sizeof(*(*cluster)->next));
        *cluster=(*cluster)->next;
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
    (void) memset(*cluster,0,sizeof(**cluster));
    (*cluster)->red=red;
    (*cluster)->green=green;
    (*cluster)->blue=*blue;
  }
  return(MagickTrue);
}

static MagickBooleanType AcquireDefaultCluster(const ExtentPacket red,
  const ExtentPacket green,const ExtentPacket blue,Cluster **head)
{
  Cluster
    *cluster;

  if (*head == (Cluster *) NULL)
    {
      /*
        No classes were identified-- create one.
      */
      cluster=(Cluster *) AcquireQuantumMemory(1,sizeof(*cluster));
      if (cluster == (Cluster *) NULL)
        return(MagickFalse);
      /*
        Initialize a new class.
      */
      (void) memset(cluster,0,sizeof(*cluster));
      cluster->red=red;
      cluster->green=green;
      cluster->blue=blue;
      *head=cluster;
    }
  return(MagickTrue);
}

typedef struct _GreenClusterTargets
{
  Cluster
    **cluster,
    **head;

  ExtentPacket
    *green,
    *blue;
} GreenClusterTargets;

static MagickBooleanType FormGreenClusters(short **extrema,
  const ExtentPacket red,const GreenClusterTargets *green_targets)
{
  Cluster
    **const cluster = green_targets->cluster,
    **const head = green_targets->head;

  ExtentPacket
    *const green = green_targets->green,
    *const blue = green_targets->blue;

  (*green).index=0;
  while (DefineRegion(extrema[Green],green) != 0)
  {
    ClusterTargets
      targets;

    (*blue).index=0;
    targets.cluster=(cluster);
    targets.head=(head);
    targets.blue=(blue);
    if (FormBlueClusters(extrema,red,*green,&targets) == MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType AcquireSquares(double **squares)
{
  ssize_t
    i;

  *squares=(double *) AcquireQuantumMemory(513UL,sizeof(*(*squares)));
  if ((*squares) == (double *) NULL)
    return(MagickFalse);
  (*squares)+=255;
  for (i=(-255); i <= 255; i++)
    (*squares)[i]=(double) i*(double) i;
  return(MagickTrue);
}

static void RelinquishClassifyResources(Cluster *head,double *squares)
{
  Cluster
    *cluster,
    *next_cluster;

  double
    *free_squares;

  for (cluster=head; cluster != (Cluster *) NULL; cluster=next_cluster)
  {
    next_cluster=cluster->next;
    cluster=(Cluster *) RelinquishMagickMemory(cluster);
  }
  squares-=255;
  free_squares=squares;
  free_squares=(double *) RelinquishMagickMemory(free_squares);
}

static void ReportClassifyProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickStatusType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,2*image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

static void RemoveSmallClusters(const double cluster_threshold,
  Cluster **head,ssize_t *count,Cluster **last_cluster)
{
  Cluster
    *cluster,
    *next_cluster;

  for (cluster=(*head); cluster != (Cluster *) NULL; cluster=next_cluster)
  {
    next_cluster=cluster->next;
    if (IsClusterAboveThreshold(cluster,(*count),cluster_threshold) !=
        MagickFalse)
      {
        /*
          Initialize cluster.
        */
        InitializeCluster(cluster,(*count));
        (*count)++;
        (*last_cluster)=cluster;
        continue;
      }
    /*
      Delete cluster.
    */
    (*head)=UnlinkCluster((*head),(*last_cluster),cluster,next_cluster);
    cluster=(Cluster *) RelinquishMagickMemory(cluster);
  }
}

typedef struct _ClassifyCountInputs
{
  const Image
    *image;

  CacheView
    *image_view;

  Cluster
    *head;

  ExceptionInfo
    *exception;
} ClassifyCountInputs;

static MagickBooleanType CountClassifyRow(const ClassifyCountInputs *inputs,
  const ssize_t y,ssize_t *count)
{
  const Quantum
    *p;

  p=GetCacheViewVirtualPixels(inputs->image_view,0,y,inputs->image->columns,1,
    inputs->exception);
  if (p == (const Quantum *) NULL)
    return(MagickFalse);
  (*count)=CountRowClusterPixels(inputs->image,inputs->head,p,(*count));
  return(MagickTrue);
}

typedef struct _ClassifyLine
{
  Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;

  ClassifyRowInputs
    inputs;
} ClassifyLine;

static MagickBooleanType ClassifyImageLine(const ClassifyLine *line,
  const ssize_t y,MagickStatusType *status)
{
  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(line->image_view,0,y,line->image->columns,1,
    line->exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  ClassifyRowPixels(line->image,&line->inputs,q);
  if (SyncCacheViewAuthenticPixels(line->image_view,line->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

#define ThrowClassifyException(severity,tag,label) \
{\
  for (cluster=head; cluster != (Cluster *) NULL; cluster=next_cluster) \
  { \
    next_cluster=cluster->next; \
    cluster=(Cluster *) RelinquishMagickMemory(cluster); \
  } \
  if (squares != (double *) NULL) \
    { \
      squares-=255; \
      free_squares=squares; \
      free_squares=(double *) RelinquishMagickMemory(free_squares); \
    } \
  ThrowBinaryException(severity,tag,label); \
}

typedef struct _ClassifyClusterTargets
{
  ExtentPacket
    *red;

  Cluster
    **cluster,
    **head;

  ExtentPacket
    *green,
    *blue;
} ClassifyClusterTargets;

static MagickBooleanType FormClassifyClusters(short **extrema,
  const ClassifyClusterTargets *targets)
{
  ExtentPacket
    *const red = targets->red;

  Cluster
    **const cluster = targets->cluster,
    **const head = targets->head;

  ExtentPacket
    *const green = targets->green,
    *const blue = targets->blue;

  while (DefineRegion(extrema[Red],red) != 0)
  {
    GreenClusterTargets
      green_targets;

    green_targets.cluster=(cluster);
    green_targets.head=(head);
    green_targets.green=(green);
    green_targets.blue=(blue);
    if (FormGreenClusters(extrema,*red,&green_targets) == MagickFalse)
      return(MagickFalse);
  }
  if (AcquireDefaultCluster(*red,*green,*blue,head) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _ClassifyPixelsInputs
{
  ExceptionInfo
    *exception;

  Cluster
    *head;

  double
    *squares;

  double
    weighting_exponent;
} ClassifyPixelsInputs;

static void ClassifyImagePixels(Image *image,const ClassifyPixelsInputs *inputs,
  MagickOffsetType progress,MagickStatusType status)
{
  ExceptionInfo
    *const exception = inputs->exception;

  Cluster
    *const head = inputs->head;

  double
    *const squares = inputs->squares;

  const double
    weighting_exponent = inputs->weighting_exponent;

  CacheView
    *image_view;

  ssize_t
    y;

  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ClassifyLine
      line;

    line.image=image;
    line.image_view=image_view;
    line.exception=exception;
    line.inputs.head=head;
    line.inputs.squares=squares;
    line.inputs.weighting_exponent=weighting_exponent;
    if (ClassifyImageLine(&line,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportClassifyProgress(image,SegmentImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  status&=(MagickStatusType) SyncImage(image,exception);
}

static MagickOffsetType CountClassifyPixels(Image *image,Cluster *head,
  MagickStatusType *status,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickOffsetType
    progress;

  ssize_t
    count,
    y;

  count=0;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const ClassifyCountInputs
      inputs = { image, image_view, head, exception };

    if (CountClassifyRow(&inputs,y,&count) == MagickFalse)
      break;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportClassifyProgress(image,SegmentImageTag,progress,status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(progress);
}

static MagickBooleanType Classify(Image *image,short **extrema,
  const ClassifySettings *settings,ExceptionInfo *exception)
{
  const double
    cluster_threshold = settings->cluster_threshold,
    weighting_exponent = settings->weighting_exponent;

  const MagickBooleanType
    verbose = settings->verbose;

  ClassifyPixelsInputs
    pixels_inputs;

  Cluster
    *cluster,
    *head,
    *last_cluster,
    *next_cluster;

  double
    *free_squares;

  ExtentPacket
    blue,
    green,
    red;

  const ClassifyClusterTargets
    cluster_targets = { &red, &cluster, &head, &green, &blue };

  MagickOffsetType
    progress;

  MagickStatusType
    status;

  double
    *squares;

  size_t
    number_clusters;

  ssize_t
    count;

  /*
    Form clusters.
  */
  cluster=(Cluster *) NULL;
  head=(Cluster *) NULL;
  squares=(double *) NULL;
  (void) memset(&red,0,sizeof(red));
  (void) memset(&green,0,sizeof(green));
  (void) memset(&blue,0,sizeof(blue));
  if (FormClassifyClusters(extrema,&cluster_targets) == MagickFalse)
    ThrowClassifyException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  /*
    Count the pixels for each cluster.
  */
  status=MagickTrue;
  progress=CountClassifyPixels(image,head,&status,exception);
  /*
    Remove clusters that do not meet minimum cluster threshold.
  */
  count=0;
  last_cluster=head;
  next_cluster=head;
  RemoveSmallClusters(cluster_threshold,&head,&count,&last_cluster);
  number_clusters=(size_t) count;
  if (verbose != MagickFalse)
    PrintClusterStatistics(head,cluster_threshold,weighting_exponent,
      number_clusters);
  if (number_clusters > 256)
    ThrowClassifyException(ImageError,"TooManyClusters",image->filename);
  /*
    Speed up distance calculations.
  */
  if (AcquireSquares(&squares) == MagickFalse)
    ThrowClassifyException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  /*
    Allocate image colormap.
  */
  if (AcquireImageColormap(image,number_clusters,exception) == MagickFalse)
    ThrowClassifyException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  SetClusterColormap(image,head);
  /*
    Do course grain classes.
  */
  pixels_inputs.exception=exception;
  pixels_inputs.head=head;
  pixels_inputs.squares=squares;
  pixels_inputs.weighting_exponent=weighting_exponent;
  ClassifyImagePixels(image,&pixels_inputs,progress,status);
  /*
    Relinquish resources.
  */
  RelinquishClassifyResources(head,squares);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  S e g m e n t I m a g e                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SegmentImage() segment an image by analyzing the histograms of the color
%  components and identifying units that are homogeneous with the fuzzy
%  C-means technique.
%
%  The format of the SegmentImage method is:
%
%      MagickBooleanType SegmentImage(Image *image,
%        const ColorspaceType colorspace,const MagickBooleanType verbose,
%        const double cluster_threshold,const double smooth_threshold,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o colorspace: Indicate the colorspace.
%
%    o verbose:  Set to MagickTrue to print detailed information about the
%      identified classes.
%
%    o cluster_threshold:  This represents the minimum number of pixels
%      contained in a hexahedra before it can be considered valid (expressed
%      as a percentage).
%
%    o smooth_threshold: the smoothing threshold eliminates noise in the second
%      derivative of the histogram.  As the value is increased, you can expect a
%      smoother second derivative.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType AcquireSegmentHistograms(const Image *image,
  ssize_t **histogram,short **extrema,ExceptionInfo *exception)
{
  ssize_t
    i;

  for (i=0; i < MaxDimension; i++)
  {
    histogram[i]=(ssize_t *) AcquireQuantumMemory(256,sizeof(**histogram));
    extrema[i]=(short *) AcquireQuantumMemory(256,sizeof(**extrema));
    if ((histogram[i] == (ssize_t *) NULL) || (extrema[i] == (short *) NULL))
      {
        for (i-- ; i >= 0; i--)
        {
          extrema[i]=(short *) RelinquishMagickMemory(extrema[i]);
          histogram[i]=(ssize_t *) RelinquishMagickMemory(histogram[i]);
        }
        ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
          image->filename)
      }
  }
  return(MagickTrue);
}

MagickExport MagickBooleanType SegmentImage(Image *image,
  const ColorspaceType colorspace,const MagickBooleanType verbose,
  const double cluster_threshold,const double smooth_threshold,
  ExceptionInfo *exception)
{
  const ClassifySettings settings = { cluster_threshold, WeightingExponent,
    verbose };

  const ZeroCrossingInputs
    tau_inputs = { Tau, 0.2, DeltaTau,
      smooth_threshold == 0.0 ? 1.0 : smooth_threshold };

  ColorspaceType
    previous_colorspace;

  MagickBooleanType
    status;

  ssize_t
    i;

  short
    *extrema[MaxDimension];

  ssize_t
    *histogram[MaxDimension];

  /*
    Allocate histogram and extrema.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (AcquireSegmentHistograms(image,histogram,extrema,exception) ==
      MagickFalse)
    return(MagickFalse);
  /*
    Initialize histogram.
  */
  previous_colorspace=image->colorspace;
  (void) TransformImageColorspace(image,colorspace,exception);
  InitializeHistogram(image,histogram,exception);
  (void) OptimalTau(histogram[Red],&tau_inputs,extrema[Red]);
  (void) OptimalTau(histogram[Green],&tau_inputs,extrema[Green]);
  (void) OptimalTau(histogram[Blue],&tau_inputs,extrema[Blue]);
  /*
    Classify using the fuzzy c-Means technique.
  */
  status=Classify(image,extrema,&settings,exception);
  (void) TransformImageColorspace(image,previous_colorspace,exception);
  /*
    Relinquish resources.
  */
  for (i=0; i < MaxDimension; i++)
  {
    extrema[i]=(short *) RelinquishMagickMemory(extrema[i]);
    histogram[i]=(ssize_t *) RelinquishMagickMemory(histogram[i]);
  }
  return(status);
}

