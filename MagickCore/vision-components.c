/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  V   V  IIIII  SSSSS  IIIII   OOO   N   N                   %
%                  V   V    I    SS       I    O   O  NN  N                   %
%                  V   V    I     SSS     I    O   O  N N N                   %
%                   V V     I       SS    I    O   O  N  NN                   %
%                    V    IIIII  SSSSS  IIIII   OOO   N   N                   %
%                                                                             %
%     CCCC   OOO   M   M  PPPP    OOO   N   N  EEEEE  N   N  TTTTT  SSSSS     %
%    C      O   O  MM MM  P   P  O   O  NN  N  E      NN  N    T    SS        %
%    C      O   O  M M M  PPPP   O   O  N N N  EEE    N N N    T     SSS      %
%    C      O   O  M   M  P      O   O  N  NN  E      N  NN    T       SS     %
%     CCCC   OOO   M   M  P       OOO   N   N  EEEEE  N   N    T    SSSSS     %
%                                                                             %
%                                                                             %
%                MagickCore Image Connected Components Method                 %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                               September 2014                                %
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
*/

#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/effect.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/matrix.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/morphology.h"
#include "MagickCore/morphology-private.h"
#include "MagickCore/opencl-private.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/resource_.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/vision.h"
#include "MagickCore/vision-components-object-private.h"
#include "MagickCore/vision-components-label-private.h"

#define ConnectedComponentsImageTag  "ConnectedComponents/Image"

static void UpdateComponentsProgress(const Image *image,
  MagickOffsetType *progress,MagickBooleanType *status)
{
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      (*progress)++;
      proceed=SetImageProgress(image,ConnectedComponentsImageTag,*progress,
        image->rows);
      if (proceed == MagickFalse)
        *status=MagickFalse;
    }
}

typedef struct _CCMetricInputs
{
  const Image
    *image;

  Image
    *component_image;

  CCObjectInfo
    *object;

  ExceptionInfo
    *exception;

  ssize_t
    background_id;
} CCMetricInputs;

typedef struct _CCMetricTargets
{
  double
    *min_threshold,
    *max_threshold;

  ssize_t
    *n;

  const char
    *(*metrics)[CCMaxMetrics];
} CCMetricTargets;

static void ApplyShapeThresholds(const CCMetricInputs *inputs,
  const CCMetricTargets *targets)
{
  const Image
    *const image = inputs->image;

  Image
    *const component_image = inputs->component_image;

  CCObjectInfo
    *const object = inputs->object;

  ExceptionInfo
    *const exception = inputs->exception;

  const ssize_t
    background_id = inputs->background_id;

  double
    *const min_threshold = targets->min_threshold,
    *const max_threshold = targets->max_threshold;

  ssize_t
    *const n = targets->n;

  const char
    *(*const metrics)[CCMaxMetrics] = targets->metrics;

  const char
    *artifact;

  artifact=GetImageArtifact(image,"connected-components:perimeter-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max perimeter threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        background_id);
      (*metrics)[++(*n)]="perimeter";
      PerimeterThreshold(component_image,object,(*n),exception);
      MergeObjectsOutsideRange(object,component_image->colors,(*n),&range);
    }
  artifact=GetImageArtifact(image,"connected-components:circularity-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max circularity threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        background_id);
      (*metrics)[++(*n)]="circularity";
      CircularityThreshold(component_image,object,(*n),exception);
      MergeObjectsOutsideRange(object,component_image->colors,(*n),&range);
    }
  artifact=GetImageArtifact(image,"connected-components:diameter-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max diameter threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        background_id);
      (*metrics)[++(*n)]="diameter";
      DiameterThreshold(component_image,object,(*n),range);
    }
}

static void ApplyAxisThresholds(const CCMetricInputs *inputs,
  const CCMetricTargets *targets)
{
  const Image
    *const image = inputs->image;

  Image
    *const component_image = inputs->component_image;

  CCObjectInfo
    *const object = inputs->object;

  ExceptionInfo
    *const exception = inputs->exception;

  const ssize_t
    background_id = inputs->background_id;

  double
    *const min_threshold = targets->min_threshold,
    *const max_threshold = targets->max_threshold;

  ssize_t
    *const n = targets->n;

  const char
    *(*const metrics)[CCMaxMetrics] = targets->metrics;

  const char
    *artifact;

  artifact=GetImageArtifact(image,"connected-components:major-axis-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max ellipse major threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        background_id);
      (*metrics)[++(*n)]="major-axis";
      MajorAxisThreshold(component_image,object,(*n),exception);
      MergeObjectsOutsideRange(object,component_image->colors,(*n),&range);
    }
  artifact=GetImageArtifact(image,"connected-components:minor-axis-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max ellipse minor threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        background_id);
      (*metrics)[++(*n)]="minor-axis";
      MinorAxisThreshold(component_image,object,(*n),exception);
      MergeObjectsOutsideRange(object,component_image->colors,(*n),&range);
    }
  artifact=GetImageArtifact(image,"connected-components:eccentricity-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max eccentricity threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        background_id);
      (*metrics)[++(*n)]="eccentricity";
      EccentricityThreshold(component_image,object,(*n),exception);
      MergeObjectsOutsideRange(object,component_image->colors,(*n),&range);
    }
  artifact=GetImageArtifact(image,"connected-components:angle-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max ellipse angle threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        background_id);
      (*metrics)[++(*n)]="angle";
      AngleThreshold(component_image,object,(*n),exception);
      MergeObjectsOutsideRange(object,component_image->colors,(*n),&range);
    }
}

static void KeepSelectedObjects(const Image *image,
  const Image *component_image,CCObjectInfo *object,ExceptionInfo *exception)
{
  const char
    *artifact;

  artifact=GetImageArtifact(image,"connected-components:keep-colors");
  if (artifact != (const char *) NULL)
    {
      CCColorMerge
        color_merge;

      /*
        Keep selected objects based on color, merge others.
      */
      MergeAllObjects(component_image,object);
      color_merge.component_image=component_image;
      color_merge.object=object;
      color_merge.artifact=artifact;
      color_merge.merge=MagickFalse;
      color_merge.exception=exception;
      MergeObjectsByColor(&color_merge);
    }
  artifact=GetImageArtifact(image,"connected-components:keep-ids");
  if (artifact == (const char *) NULL)
    artifact=GetImageArtifact(image,"connected-components:keep");
  if (artifact != (const char *) NULL)
    {
      /*
        Keep selected objects based on id, merge others.
      */
      MergeAllObjects(component_image,object);
      SetObjectMergeByIds(object,component_image->colors,artifact,MagickFalse);
    }
}

static void RemoveSelectedObjects(const Image *image,
  const Image *component_image,CCObjectInfo *object,ExceptionInfo *exception)
{
  const char
    *artifact;

  artifact=GetImageArtifact(image,"connected-components:remove-colors");
  if (artifact != (const char *) NULL)
    {
      /*
        Remove selected objects based on color, keep others.
      */
      RemoveObjectsByColor(component_image,object,artifact,exception);
    }
  artifact=GetImageArtifact(image,"connected-components:remove-ids");
  if (artifact == (const char *) NULL)
    artifact=GetImageArtifact(image,"connected-components:remove");
  if (artifact != (const char *) NULL)
    {
      /*
        Remove selected objects based on id, keep others.
      */
      SetObjectMergeByIds(object,component_image->colors,artifact,MagickTrue);
    }
}

typedef struct _CCReportInputs
{
  const Image
    *image;

  Image
    *component_image;

  CCObjectInfo
    *object,
    **objects;

  ssize_t
    n;

  ExceptionInfo
    *exception;
} CCReportInputs;

static Image *ReportConnectedComponents(const CCReportInputs *report_inputs,
  const char **metrics,MagickBooleanType status)
{
  const Image
    *const image = report_inputs->image;

  Image
    *const component_image = report_inputs->component_image;

  CCObjectInfo
    *object = report_inputs->object,
    **const objects = report_inputs->objects;

  const ssize_t
    n = report_inputs->n;

  ExceptionInfo
    *const exception = report_inputs->exception;

  const char
    *artifact;

  artifact=GetImageArtifact(image,"connected-components:mean-color");
  if (IsStringTrue(artifact) != MagickFalse)
    {
      /*
        Replace object with mean color.
      */
      SetObjectMeanColors(component_image,object);
    }
  (void) SyncImage(component_image,exception);
  artifact=GetImageArtifact(image,"connected-components:verbose");
  if ((IsStringTrue(artifact) != MagickFalse) ||
      (objects != (CCObjectInfo **) NULL))
    {
      /*
        Report statistics on each unique object.
      */
      status=ComputeObjectStatistics(component_image,object,status,exception);
      SortObjects(image,component_image,object);
      if (objects == (CCObjectInfo **) NULL)
        {
          PrintObjectsHeader(image,metrics,n);
          PrintObjects(image,component_image,object,n);
        }
    }
  if (objects == (CCObjectInfo **) NULL)
    object=(CCObjectInfo *) RelinquishMagickMemory(object);
  else
    *objects=object;
  return(component_image);
}

static MagickBooleanType SyncComponentView(CacheView *component_view,
  MagickBooleanType status,ExceptionInfo *exception)
{
  if (SyncCacheViewAuthenticPixels(component_view,exception) == MagickFalse)
    status=MagickFalse;
  return(status);
}

typedef struct _CCLabelInputs
{
  const Image
    *image;

  CacheView
    *image_view;

  MatrixInfo
    *equivalences;

  CCObjectInfo
    *object;

  Image
    *component_image;

  ExceptionInfo
    *exception;
} CCLabelInputs;

typedef struct _CCLabelCounts
{
  ssize_t
    *n;

  MagickBooleanType
    *status;

  MagickOffsetType
    *progress;
} CCLabelCounts;

static void LabelConnectedComponents(const CCLabelInputs *label_inputs,
  const CCLabelCounts *label_counts)
{
  const Image
    *const image = label_inputs->image;

  CacheView
    *const image_view = label_inputs->image_view;

  MatrixInfo
    *const equivalences = label_inputs->equivalences;

  CCObjectInfo
    *const object = label_inputs->object;

  Image
    *const component_image = label_inputs->component_image;

  ExceptionInfo
    *const exception = label_inputs->exception;

  ssize_t
    *const n = label_counts->n;

  MagickBooleanType
    *const status = label_counts->status;

  MagickOffsetType
    *const progress = label_counts->progress;

  CacheView
    *component_view;

  ssize_t
    y;

  (*n)=0;
  component_view=AcquireAuthenticCacheView(component_image,exception);
  for (y=0; y < (ssize_t) component_image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    const CCLabelTargets
      label_targets = { status, n, &p, &q };

    CCLabelRow
      label_row;

    if ((*status) == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    q=QueueCacheViewAuthenticPixels(component_view,0,y,component_image->columns,
      1,exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        (*status)=MagickFalse;
        continue;
      }
    label_row.image=image;
    label_row.equivalences=equivalences;
    label_row.object=object;
    label_row.component_image=component_image;
    label_row.y=y;
    LabelObjectRow(&label_row,label_targets);
    if ((*n) > (ssize_t) MaxColormapSize)
      break;
    (*status)=SyncComponentView(component_view,(*status),exception);
    UpdateComponentsProgress(image,progress,status);
  }
  component_view=DestroyCacheView(component_view);
}

static void InitializeEquivalences(const Image *image,
  MatrixInfo *equivalences)
{
  ssize_t
    n;

  for (n=0; n < (ssize_t) (image->columns*image->rows); n++)
    (void) SetMatrixElement(equivalences,n,0,&n);
}

typedef struct _CCFinishInputs
{
  const Image
    *image;

  Image
    *component_image;

  CCObjectInfo
    *object,
    **objects;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    background_id,
    n;

  MagickBooleanType
    status;

  ExceptionInfo
    *exception;
} CCFinishInputs;

static Image *MergeAndReportComponents(const CCFinishInputs *finish_inputs,
  const char **metrics)
{
  const Image
    *const image = finish_inputs->image;

  Image
    *component_image = finish_inputs->component_image;

  CCObjectInfo
    *object = finish_inputs->object,
    **const objects = finish_inputs->objects;

  const size_t
    connectivity = finish_inputs->connectivity;

  ssize_t
    (*const connect4)[2] = finish_inputs->connect4,
    (*const connect8)[2] = finish_inputs->connect8;

  const ssize_t
    background_id = finish_inputs->background_id,
    n = finish_inputs->n;

  MagickBooleanType
    status = finish_inputs->status;

  ExceptionInfo
    *const exception = finish_inputs->exception;

  CCMergeInputs
    merge_inputs;

  CCReportInputs
    report_inputs;

  size_t
    *neighbors;

  /*
    Merge any object not within the min and max area threshold.
  */
  neighbors=(size_t *) AcquireQuantumMemory(component_image->colors+1,
    sizeof(*neighbors));
  if (neighbors == (size_t *) NULL)
    {
      object=(CCObjectInfo *) RelinquishMagickMemory(object);
      component_image=DestroyImage(component_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  merge_inputs.component_image=component_image;
  merge_inputs.object=object;
  merge_inputs.neighbors=neighbors;
  merge_inputs.connectivity=connectivity;
  merge_inputs.connect4=connect4;
  merge_inputs.connect8=connect8;
  merge_inputs.background_id=background_id;
  merge_inputs.exception=exception;
  status=MergeSelectedObjects(&merge_inputs,status);
  neighbors=(size_t *) RelinquishMagickMemory(neighbors);
  report_inputs.image=image;
  report_inputs.component_image=component_image;
  report_inputs.object=object;
  report_inputs.objects=objects;
  report_inputs.n=n;
  report_inputs.exception=exception;
  return(ReportConnectedComponents(&report_inputs,metrics,status));
}

typedef struct _CCSelectionInputs
{
  const Image
    *image;

  CCObjectInfo
    *object;

  Image
    *component_image;
} CCSelectionInputs;

typedef struct _CCSelectionTargets
{
  ssize_t
    *background_id;

  double
    *min_threshold,
    *max_threshold;

  ssize_t
    *n;
} CCSelectionTargets;

static void PrepareObjectSelection(const CCSelectionInputs *selection_inputs,
  const CCSelectionTargets *selection_targets)
{
  const Image
    *const image = selection_inputs->image;

  CCObjectInfo
    *const object = selection_inputs->object;

  Image
    *const component_image = selection_inputs->component_image;

  ssize_t
    *const background_id = selection_targets->background_id;

  double
    *const min_threshold = selection_targets->min_threshold,
    *const max_threshold = selection_targets->max_threshold;

  ssize_t
    *const n = selection_targets->n;

  CCLabeledObjects
    labeled_objects;

  const char
    *artifact;

  labeled_objects.image=image;
  labeled_objects.object=object;
  labeled_objects.component_image=component_image;
  labeled_objects.n=(*n);
  NormalizeLabeledObjects(&labeled_objects,background_id,min_threshold,
    max_threshold);
  (*n)=(-1);
  artifact=GetImageArtifact(image,"connected-components:background-id");
  if (artifact != (const char *) NULL)
    (*background_id)=(ssize_t) StringToLong(artifact);
  artifact=GetImageArtifact(image,"connected-components:area-threshold");
  if (artifact != (const char *) NULL)
    {
      CCThresholdRange range;

      /*
        Merge any object not within the min and max area threshold.
      */
      range=GetThresholdRange(artifact,min_threshold,max_threshold,
        (*background_id));
      AreaThreshold(component_image,object,range);
    }
}

typedef struct _CCThresholdInputs
{
  const Image
    *image;

  Image
    *component_image;

  CCObjectInfo
    *object,
    **objects;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    background_id;

  double
    min_threshold,
    max_threshold;

  ssize_t
    n;

  MagickBooleanType
    status;

  ExceptionInfo
    *exception;
} CCThresholdInputs;

static Image *ApplyObjectThresholds(const CCThresholdInputs *threshold_inputs)
{
  CCFinishInputs
    finish_inputs;

  CCMetricInputs
    inputs;

  CCMetricTargets
    targets;

  const char
    *metrics[CCMaxMetrics];

  double
    max_threshold = threshold_inputs->max_threshold,
    min_threshold = threshold_inputs->min_threshold;

  ssize_t
    n = threshold_inputs->n;

  inputs.image=threshold_inputs->image;
  inputs.component_image=threshold_inputs->component_image;
  inputs.object=threshold_inputs->object;
  inputs.exception=threshold_inputs->exception;
  inputs.background_id=threshold_inputs->background_id;
  targets.min_threshold=(&min_threshold);
  targets.max_threshold=(&max_threshold);
  targets.n=(&n);
  targets.metrics=(&metrics);
  ApplyShapeThresholds(&inputs,&targets);
  inputs.image=threshold_inputs->image;
  inputs.component_image=threshold_inputs->component_image;
  inputs.object=threshold_inputs->object;
  inputs.exception=threshold_inputs->exception;
  inputs.background_id=threshold_inputs->background_id;
  targets.min_threshold=(&min_threshold);
  targets.max_threshold=(&max_threshold);
  targets.n=(&n);
  targets.metrics=(&metrics);
  ApplyAxisThresholds(&inputs,&targets);
  finish_inputs.image=threshold_inputs->image;
  finish_inputs.component_image=threshold_inputs->component_image;
  finish_inputs.object=threshold_inputs->object;
  finish_inputs.objects=threshold_inputs->objects;
  finish_inputs.connectivity=threshold_inputs->connectivity;
  finish_inputs.connect4=threshold_inputs->connect4;
  finish_inputs.connect8=threshold_inputs->connect8;
  finish_inputs.background_id=threshold_inputs->background_id;
  finish_inputs.n=n;
  finish_inputs.status=threshold_inputs->status;
  finish_inputs.exception=threshold_inputs->exception;
  return(MergeAndReportComponents(&finish_inputs,metrics));
}

static Image *SelectConnectedComponents(const CCThresholdInputs *selection)
{
  const Image
    *const image = selection->image;

  Image
    *component_image = selection->component_image;

  CCObjectInfo
    *object = selection->object,
    **const objects = selection->objects;

  const size_t
    connectivity = selection->connectivity;

  ssize_t
    (*const connect4)[2] = selection->connect4,
    (*const connect8)[2] = selection->connect8;

  const ssize_t
    background_id = selection->background_id,
    n = selection->n;

  const double
    min_threshold = selection->min_threshold,
    max_threshold = selection->max_threshold;

  const MagickBooleanType
    status = selection->status;

  ExceptionInfo
    *const exception = selection->exception;

  CCThresholdInputs
    threshold_inputs;

  const char
    *artifact;

  KeepSelectedObjects(image,component_image,object,exception);
  artifact=GetImageArtifact(image,"connected-components:keep-top");
  /*
    Keep top objects.
  */
  if ((artifact != (const char *) NULL) &&
      (KeepTopObjects(component_image,object,artifact) == MagickFalse))
    {
      object=(CCObjectInfo *) RelinquishMagickMemory(object);
      component_image=DestroyImage(component_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  RemoveSelectedObjects(image,component_image,object,exception);
  threshold_inputs.image=image;
  threshold_inputs.component_image=component_image;
  threshold_inputs.object=object;
  threshold_inputs.objects=objects;
  threshold_inputs.connectivity=connectivity;
  threshold_inputs.connect4=connect4;
  threshold_inputs.connect8=connect8;
  threshold_inputs.background_id=background_id;
  threshold_inputs.min_threshold=min_threshold;
  threshold_inputs.max_threshold=max_threshold;
  threshold_inputs.n=n;
  threshold_inputs.status=status;
  threshold_inputs.exception=exception;
  return(ApplyObjectThresholds(&threshold_inputs));
}

static void ResetObjectsResult(CCObjectInfo **objects)
{
  if (objects != (CCObjectInfo **) NULL)
    *objects=(CCObjectInfo *) NULL;
}

typedef struct _CCLabeledInputs
{
  const Image
    *image;

  Image
    *component_image;

  CCObjectInfo
    *object,
    **objects;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    n;

  MagickBooleanType
    status;

  ExceptionInfo
    *exception;
} CCLabeledInputs;

static Image *SelectLabeledComponents(const CCLabeledInputs *labeled)
{
  const Image
    *const image = labeled->image;

  Image
    *component_image = labeled->component_image;

  CCObjectInfo
    *object = labeled->object,
    **const objects = labeled->objects;

  const size_t
    connectivity = labeled->connectivity;

  ssize_t
    (*const connect4)[2] = labeled->connect4,
    (*const connect8)[2] = labeled->connect8,
    background_id,
    n = labeled->n;

  const MagickBooleanType
    status = labeled->status;

  ExceptionInfo
    *const exception = labeled->exception;

  CCSelectionInputs
    selection_inputs;

  CCSelectionTargets
    selection_targets;

  CCThresholdInputs
    selection;

  double
    max_threshold,
    min_threshold;

  if (n > (ssize_t) MaxColormapSize)
    {
      object=(CCObjectInfo *) RelinquishMagickMemory(object);
      component_image=DestroyImage(component_image);
      ThrowImageException(ResourceLimitError,"TooManyObjects");
    }
  selection_inputs.image=image;
  selection_inputs.object=object;
  selection_inputs.component_image=component_image;
  selection_targets.background_id=(&background_id);
  selection_targets.min_threshold=(&min_threshold);
  selection_targets.max_threshold=(&max_threshold);
  selection_targets.n=(&n);
  PrepareObjectSelection(&selection_inputs,&selection_targets);
  selection.image=image;
  selection.component_image=component_image;
  selection.object=object;
  selection.objects=objects;
  selection.connectivity=connectivity;
  selection.connect4=connect4;
  selection.connect8=connect8;
  selection.background_id=background_id;
  selection.min_threshold=min_threshold;
  selection.max_threshold=max_threshold;
  selection.n=n;
  selection.status=status;
  selection.exception=exception;
  return(SelectConnectedComponents(&selection));
}

typedef struct _CCComponentInputs
{
  const Image
    *image;

  Image
    *component_image;

  MatrixInfo
    *equivalences;

  CCObjectInfo
    *object,
    **objects;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2];

  ExceptionInfo
    *exception;
} CCComponentInputs;

static Image *FindConnectedComponents(const CCComponentInputs *components)
{
  CacheView
    *image_view;

  CCEquivalenceInputs
    equivalence_inputs;

  CCLabelCounts
    label_counts;

  CCLabelInputs
    label_inputs;

  CCLabeledInputs
    labeled;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  MatrixInfo
    *equivalences = components->equivalences;

  ssize_t
    n;

  /*
    Find connected components.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(components->image,components->exception);
  equivalence_inputs.image=components->image;
  equivalence_inputs.image_view=image_view;
  equivalence_inputs.equivalences=equivalences;
  equivalence_inputs.connectivity=components->connectivity;
  equivalence_inputs.connect4=components->connect4;
  equivalence_inputs.connect8=components->connect8;
  equivalence_inputs.exception=components->exception;
  status=ResolveEquivalences(&equivalence_inputs,status);
  /*
    Label connected components.
  */
  label_inputs.image=components->image;
  label_inputs.image_view=image_view;
  label_inputs.equivalences=equivalences;
  label_inputs.object=components->object;
  label_inputs.component_image=components->component_image;
  label_inputs.exception=components->exception;
  label_counts.n=(&n);
  label_counts.status=(&status);
  label_counts.progress=(&progress);
  LabelConnectedComponents(&label_inputs,&label_counts);
  image_view=DestroyCacheView(image_view);
  equivalences=DestroyMatrixInfo(equivalences);
  labeled.image=components->image;
  labeled.component_image=components->component_image;
  labeled.object=components->object;
  labeled.objects=components->objects;
  labeled.connectivity=components->connectivity;
  labeled.connect4=components->connect4;
  labeled.connect8=components->connect8;
  labeled.n=n;
  labeled.status=status;
  labeled.exception=components->exception;
  return(SelectLabeledComponents(&labeled));
}

MagickExport Image *ConnectedComponentsImage(const Image *image,
  const size_t connectivity,CCObjectInfo **objects,ExceptionInfo *exception)
{
  CCComponentInputs
    components;

  CCObjectInfo
    *object;

  Image
    *component_image;

  MatrixInfo
    *equivalences;

  size_t
    size;

  ssize_t
    connect4[2][2] = { { -1,  0 }, {  0, -1 } },
    connect8[4][2] = { { -1, -1 }, { -1,  0 }, { -1,  1 }, {  0, -1 } };

  /*
    Initialize connected components image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  ResetObjectsResult(objects);
  component_image=CloneImage(image,0,0,MagickTrue,exception);
  if (component_image == (Image *) NULL)
    return((Image *) NULL);
  component_image->depth=MAGICKCORE_QUANTUM_DEPTH;
  if (AcquireImageColormap(component_image,MaxColormapSize,exception) == MagickFalse)
    {
      component_image=DestroyImage(component_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Initialize connected components equivalences.
  */
  size=image->columns*image->rows;
  if (image->columns != (size/image->rows))
    {
      component_image=DestroyImage(component_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  equivalences=AcquireMatrixInfo(size,1,sizeof(ssize_t),exception);
  if (equivalences == (MatrixInfo *) NULL)
    {
      component_image=DestroyImage(component_image);
      return((Image *) NULL);
    }
  InitializeEquivalences(image,equivalences);
  object=(CCObjectInfo *) AcquireQuantumMemory(MaxColormapSize,sizeof(*object));
  if (object == (CCObjectInfo *) NULL)
    {
      equivalences=DestroyMatrixInfo(equivalences);
      component_image=DestroyImage(component_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  (void) memset(object,0,MaxColormapSize*sizeof(*object));
  InitializeObjects(image,object);
  /*
    Find connected components.
  */
  components.image=image;
  components.component_image=component_image;
  components.equivalences=equivalences;
  components.object=object;
  components.objects=objects;
  components.connectivity=connectivity;
  components.connect4=connect4;
  components.connect8=connect8;
  components.exception=exception;
  return(FindConnectedComponents(&components));
}
