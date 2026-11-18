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

  MagickCore private methods of vision-components.c: the labelling,
  equivalence and merge passes of ConnectedComponentsImage, from the
  CCEquivalencePixel typedef to MergeSelectedObjects; included by that file
  only.
*/
#ifndef MAGICKCORE_VISION_COMPONENTS_LABEL_PRIVATE_H
#define MAGICKCORE_VISION_COMPONENTS_LABEL_PRIVATE_H

typedef struct _CCEquivalencePixel
{
  const Image
    *image;

  ssize_t
    x,
    y,
    dx,
    dy;
} CCEquivalencePixel;

static MagickBooleanType ResolveEquivalence(MatrixInfo *equivalences,
  const CCEquivalencePixel *equivalence_pixel)
{
  const Image
    *const image = equivalence_pixel->image;

  const ssize_t
    x = equivalence_pixel->x,
    y = equivalence_pixel->y,
    dx = equivalence_pixel->dx,
    dy = equivalence_pixel->dy;

  MagickBooleanType
    status;

  ssize_t
    neighbor_offset,
    obj,
    offset,
    ox,
    oy,
    root;

  offset=y*(ssize_t) image->columns+x;
  neighbor_offset=dy*(ssize_t) image->columns+dx;
  ox=offset;
  status=GetMatrixElement(equivalences,ox,0,&obj);
  while (obj != ox)
  {
    ox=obj;
    status=GetMatrixElement(equivalences,ox,0,&obj);
  }
  oy=offset+neighbor_offset;
  status=GetMatrixElement(equivalences,oy,0,&obj);
  while (obj != oy)
  {
    oy=obj;
    status=GetMatrixElement(equivalences,oy,0,&obj);
  }
  if (ox < oy)
    {
      status=SetMatrixElement(equivalences,oy,0,&ox);
      root=ox;
    }
  else
    {
      status=SetMatrixElement(equivalences,ox,0,&oy);
      root=oy;
    }
  ox=offset;
  status=SetEquivalencePathRoot(equivalences,ox,root);
  oy=offset+neighbor_offset;
  status=SetEquivalencePathRoot(equivalences,oy,root);
  status=SetMatrixElement(equivalences,y*(ssize_t) image->columns+x,0,
    &root);
  return(status);
}

typedef struct _CCObjectRelabel
{
  Image
    *component_image;

  ssize_t
    i;

  size_t
    id;
} CCObjectRelabel;

static void RelabelObjectRow(const CCObjectRelabel *object_relabel,
  const RectangleInfo bounding_box,Quantum *q)
{
  Image
    *const component_image = object_relabel->component_image;

  const ssize_t
    i = object_relabel->i;

  const size_t
    id = object_relabel->id;

  ssize_t
    x;

  for (x=0; x < (ssize_t) bounding_box.width; x++)
  {
    if ((ssize_t) GetPixelIndex(component_image,q) == i)
      SetPixelIndex(component_image,(Quantum) id,q);
    q+=(ptrdiff_t) GetPixelChannels(component_image);
  }
}

static void MergeObjectsBelowTop(const Image *component_image,
  CCObjectInfo *object,CCObjectInfo *top_objects,const ssize_t top_ids)
{
  ssize_t
    i;

  (void) memcpy(top_objects,object,component_image->colors*sizeof(*object));
  qsort((void *) top_objects,component_image->colors,sizeof(*top_objects),
    CCObjectInfoCompare);
  for (i=top_ids+1; i < (ssize_t) component_image->colors; i++)
  {
    ssize_t id = (ssize_t) top_objects[i].id;
    if ((id >= 0) && (id < (ssize_t) component_image->colors))
      object[id].merge=MagickTrue;
  }
}

static ssize_t GetTopObjectIds(const Image *component_image,
  const char *artifact)
{
  ssize_t
    top_ids;

  top_ids=(ssize_t) StringToLong(artifact);
  if (top_ids < 0)
    top_ids=0;
  if (top_ids >= (ssize_t) component_image->colors)
    top_ids=(ssize_t) component_image->colors-1;
  return(top_ids);
}

static void DiameterThreshold(const Image *component_image,
  CCObjectInfo *object,const ssize_t n,const CCThresholdRange range)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) component_image->colors; i++)
  {
    object[i].metric[n]=ceil(sqrt(4.0*object[i].area/MagickPI)-0.5);
    if (IsMetricOutsideRange(object,i,n,&range) != MagickFalse)
      object[i].merge=MagickTrue;
  }
}

static void NormalizeLabeledObject(const Image *image,CCObjectInfo *object,
  const ssize_t i)
{
  object[i].bounding_box.width=(size_t) ((ssize_t)
    object[i].bounding_box.width-(object[i].bounding_box.x-1));
  object[i].bounding_box.height=(size_t) ((ssize_t)
    object[i].bounding_box.height-(object[i].bounding_box.y-1));
  object[i].color.red/=(QuantumScale*object[i].area);
  object[i].color.green/=(QuantumScale*object[i].area);
  object[i].color.blue/=(QuantumScale*object[i].area);
  if (image->alpha_trait != UndefinedPixelTrait)
    object[i].color.alpha/=(QuantumScale*object[i].area);
  if (image->colorspace == CMYKColorspace)
    object[i].color.black/=(QuantumScale*object[i].area);
  object[i].centroid.x/=object[i].area;
  object[i].centroid.y/=object[i].area;
}

typedef struct _CCObjectStatistics
{
  Image
    *component_image;

  CacheView
    *component_view;

  ExceptionInfo
    *exception;
} CCObjectStatistics;

static MagickBooleanType AccumulateObjectStatistics(CCObjectInfo *object,
  const CCObjectStatistics *object_statistics,MagickBooleanType status)
{
  Image
    *const component_image = object_statistics->component_image;

  CacheView
    *const component_view = object_statistics->component_view;

  ExceptionInfo
    *const exception = object_statistics->exception;

  ssize_t
    y;

  for (y=0; y < (ssize_t) component_image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(component_view,0,y,component_image->columns,
      1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) component_image->columns; x++)
    {
      size_t
        id;

      id=(size_t) GetPixelIndex(component_image,p);
      AddObjectPixel(object,id,x,y);
      p+=(ptrdiff_t) GetPixelChannels(component_image);
    }
  }
  return(status);
}

static void InitializeObjects(const Image *image,CCObjectInfo *object)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) MaxColormapSize; i++)
  {
    object[i].id=i;
    object[i].bounding_box.x=(ssize_t) image->columns;
    object[i].bounding_box.y=(ssize_t) image->rows;
    GetPixelInfo(image,&object[i].color);
  }
}

typedef struct _CCEquivalenceRow
{
  const Image
    *image;

  MatrixInfo
    *equivalences;

  ssize_t
    y,
    dx,
    dy;
} CCEquivalenceRow;

static MagickBooleanType ResolveRowEquivalences(
  const CCEquivalenceRow *equivalence_row,const CCNeighborDirection direction,
  const Quantum *p,MagickBooleanType status)
{
  const Image
    *const image = equivalence_row->image;

  MatrixInfo
    *const equivalences = equivalence_row->equivalences;

  const ssize_t
    y = equivalence_row->y,
    dx = equivalence_row->dx,
    dy = equivalence_row->dy;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    CCEquivalencePixel
      equivalence_pixel;

    PixelInfo
      pixel,
      target;

    ssize_t
      neighbor_offset;

    /*
      Is neighbor an authentic pixel and a different color than the pixel?
    */
    GetPixelInfoPixel(image,p,&pixel);
    if (IsNeighborOutsideImage(image,x,y,&direction) != MagickFalse)
      {
        p+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    neighbor_offset=dy*((ssize_t) GetPixelChannels(image)*(ssize_t)
      image->columns)+dx*(ssize_t) GetPixelChannels(image);
    GetPixelInfoPixel(image,p+neighbor_offset,&target);
    if (IsFuzzyEquivalencePixelInfo(&pixel,&target) == MagickFalse)
      {
        p+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    /*
      Resolve this equivalence.
    */
    equivalence_pixel.image=image;
    equivalence_pixel.x=x;
    equivalence_pixel.y=y;
    equivalence_pixel.dx=dx;
    equivalence_pixel.dy=dy;
    status=ResolveEquivalence(equivalences,&equivalence_pixel);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(status);
}

typedef struct _CCColorMerge
{
  const Image
    *component_image;

  CCObjectInfo
    *object;

  const char
    *artifact;

  MagickBooleanType
    merge;

  ExceptionInfo
    *exception;
} CCColorMerge;

static void MergeObjectsByColor(const CCColorMerge *color_merge)
{
  const Image
    *const component_image = color_merge->component_image;

  CCObjectInfo
    *const object = color_merge->object;

  const char
    *const artifact = color_merge->artifact;

  const MagickBooleanType
    merge = color_merge->merge;

  ExceptionInfo
    *const exception = color_merge->exception;

  const char
    *p;

  for (p=artifact;  ; )
  {
    char
      color[MagickPathExtent];

    PixelInfo
      pixel;

    const char
      *q;

    for (q=p; *q != '\0'; q++)
      if (*q == ';')
        break;
    (void) CopyMagickString(color,p,(size_t) MagickMin(q-p+1,
      MagickPathExtent));
    (void) QueryColorCompliance(color,AllCompliance,&pixel,exception);
    SetObjectMergeByColor(object,component_image->colors,&pixel,merge);
    if (*q == '\0')
      break;
    p=q+1;
  }
}

static void RemoveObjectsByColor(const Image *component_image,
  CCObjectInfo *object,const char *artifact,ExceptionInfo *exception)
{
  CCColorMerge
    color_merge;

  color_merge.component_image=component_image;
  color_merge.object=object;
  color_merge.artifact=artifact;
  color_merge.merge=MagickTrue;
  color_merge.exception=exception;
  MergeObjectsByColor(&color_merge);
}

static void AreaThreshold(const Image *component_image,CCObjectInfo *object,
  const CCThresholdRange range)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) component_image->colors; i++)
    if (IsAreaOutsideRange(object,i,&range) != MagickFalse)
      object[i].merge=MagickTrue;
}

static void PrintObjects(const Image *image,const Image *component_image,
  const CCObjectInfo *object,const ssize_t n)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) component_image->colors; i++)
    if (object[i].census > 0.0)
      PrintObject(image,object,i,n);
}

static void ResetAdjacentCensus(CCObjectInfo *object,const size_t *neighbors,
  const size_t count)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) count; j++)
    object[neighbors[j]].census=0.0;
}

static void MergeAllObjects(const Image *component_image,CCObjectInfo *object)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) component_image->colors; i++)
    object[i].merge=MagickTrue;
}

static void SetObjectMeanColors(Image *component_image,
  const CCObjectInfo *object)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) component_image->colors; i++)
    component_image->colormap[i]=object[i].color;
}

typedef struct _CCAdjacentCensus
{
  Image
    *component_image;

  CacheView
    *object_view;

  CCObjectInfo
    *object;

  size_t
    *neighbors;

  RectangleInfo
    bounding_box;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    x,
    y,
    i;

  ExceptionInfo
    *exception;
} CCAdjacentCensus;

static const Quantum *GetAdjacentObjectPixel(const CCAdjacentCensus *census,
  const ssize_t k)
{
  ssize_t
    dx,
    dy;

  dx=census->connectivity > 4 ? census->connect8[k][1] :
    census->connect4[k][1];
  dy=census->connectivity > 4 ? census->connect8[k][0] :
    census->connect4[k][0];
  return(GetCacheViewVirtualPixels(census->object_view,census->bounding_box.x+
    census->x+dx,census->bounding_box.y+census->y+dy,1,1,census->exception));
}

static void CountAdjacentObjects(const CCAdjacentCensus *census,
  MagickBooleanType *status,size_t *count)
{
  ssize_t
    j,
    k;

  for (k=0; k < (ssize_t) (census->connectivity > 4 ? 4 : 2); k++)
  {
    const Quantum
      *q;

    /*
      Compute area of adjacent objects.
    */
    if (*status == MagickFalse)
      continue;
    q=GetAdjacentObjectPixel(census,k);
    if (q == (const Quantum *) NULL)
      {
        *status=MagickFalse;
        break;
      }
    j=(ssize_t) GetPixelIndex(census->component_image,q);
    if (j != census->i)
      *count=AddAdjacentObject(census->object,census->neighbors,j,*count);
  }
}

typedef struct _CCLabelPixel
{
  const Image
    *image;

  MatrixInfo
    *equivalences;

  CCObjectInfo
    *object;

  Image
    *component_image;

  ssize_t
    x,
    y;
} CCLabelPixel;

typedef struct _CCLabelTargets
{
  MagickBooleanType
    *status;

  ssize_t
    *n;

  const Quantum
    *magick_restrict *p;

  Quantum
    *magick_restrict *q;
} CCLabelTargets;

static MagickBooleanType LabelObjectPixel(const CCLabelPixel *label_pixel,
  const CCLabelTargets *label_targets)
{
  MagickBooleanType
    *const status = label_targets->status;

  ssize_t
    *const n = label_targets->n;

  const Quantum
    *magick_restrict *const p = label_targets->p;

  Quantum
    *magick_restrict *const q = label_targets->q;

  CCObjectPixel
    object_pixel;

  ssize_t
    id,
    offset;

  offset=label_pixel->y*(ssize_t) label_pixel->image->columns+label_pixel->x;
  *status=GetMatrixElement(label_pixel->equivalences,offset,0,&id);
  if (id != offset)
    *status=GetMatrixElement(label_pixel->equivalences,id,0,&id);
  else
    {
      id=(*n)++;
      if (id >= (ssize_t) MaxColormapSize)
        return(MagickFalse);
    }
  *status=SetMatrixElement(label_pixel->equivalences,offset,0,&id);
  object_pixel.image=label_pixel->image;
  object_pixel.p=(*p);
  object_pixel.x=label_pixel->x;
  object_pixel.y=label_pixel->y;
  object_pixel.id=id;
  AddLabeledObjectPixel(label_pixel->object,&object_pixel);
  SetPixelIndex(label_pixel->component_image,(Quantum) id,*q);
  *p+=(ptrdiff_t) GetPixelChannels(label_pixel->image);
  *q+=(ptrdiff_t) GetPixelChannels(label_pixel->component_image);
  return(MagickTrue);
}

typedef struct _CCAdjacentRow
{
  Image
    *component_image;

  CacheView
    *object_view;

  CCObjectInfo
    *object;

  size_t
    *neighbors;

  RectangleInfo
    bounding_box;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    y,
    i;

  ExceptionInfo
    *exception;
} CCAdjacentRow;

typedef struct _CCAdjacentTargets
{
  MagickBooleanType
    *status;

  size_t
    *count;

  const Quantum
    *magick_restrict *p;
} CCAdjacentTargets;

static void CountAdjacentObjectsInRow(const CCAdjacentRow *adjacent_row,
  const CCAdjacentTargets *adjacent_targets)
{
  MagickBooleanType
    *const status = adjacent_targets->status;

  size_t
    *const count = adjacent_targets->count;

  const Quantum
    *magick_restrict *const p = adjacent_targets->p;

  ssize_t
    j,
    x;

  for (x=0; x < (ssize_t) adjacent_row->bounding_box.width; x++)
  {
    CCAdjacentCensus
      census;

    if (*status == MagickFalse)
      continue;
    j=(ssize_t) GetPixelIndex(adjacent_row->component_image,*p);
    if (j == adjacent_row->i)
      {
        census.component_image=adjacent_row->component_image;
        census.object_view=adjacent_row->object_view;
        census.object=adjacent_row->object;
        census.neighbors=adjacent_row->neighbors;
        census.bounding_box=adjacent_row->bounding_box;
        census.connectivity=adjacent_row->connectivity;
        census.connect4=adjacent_row->connect4;
        census.connect8=adjacent_row->connect8;
        census.x=x;
        census.y=adjacent_row->y;
        census.i=adjacent_row->i;
        census.exception=adjacent_row->exception;
        CountAdjacentObjects(&census,status,count);
      }
    *p+=(ptrdiff_t) GetPixelChannels(adjacent_row->component_image);
  }
}

static void SortObjects(const Image *image,const Image *component_image,
  CCObjectInfo *object)
{
  ssize_t
    i,
    key,
    order;

  order=GetObjectSortOrder(image);
  key=GetObjectSortKey(image);
  for (i=0; i < (ssize_t) component_image->colors; i++)
     object[i].key=order*key;
  qsort((void *) object,component_image->colors,sizeof(*object),
    CCObjectInfoCompare);
}

static MagickBooleanType ComputeObjectStatistics(Image *component_image,
  CCObjectInfo *object,MagickBooleanType status,ExceptionInfo *exception)
{
  CacheView
    *component_view;

  CCObjectStatistics
    object_statistics;

  ResetObjectStatistics(component_image,object);
  component_view=AcquireVirtualCacheView(component_image,exception);
  object_statistics.component_image=component_image;
  object_statistics.component_view=component_view;
  object_statistics.exception=exception;
  status=AccumulateObjectStatistics(object,&object_statistics,status);
  NormalizeObjectStatistics(component_image,object);
  component_view=DestroyCacheView(component_view);
  return(status);
}

static CCThresholdRange GetThresholdRange(const char *artifact,
  double *min_threshold,double *max_threshold,const ssize_t background_id)
{
  CCThresholdRange
    range;

  (void) MagickSscanf(artifact,"%lf%*[ -]%lf",min_threshold,max_threshold);
  range.min_threshold=(*min_threshold);
  range.max_threshold=(*max_threshold);
  range.background_id=background_id;
  return(range);
}

typedef struct _CCEquivalencePass
{
  const Image
    *image;

  CacheView
    *image_view;

  MatrixInfo
    *equivalences;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    n;

  ExceptionInfo
    *exception;
} CCEquivalencePass;

static MagickBooleanType ResolveDirectionEquivalences(
  const CCEquivalencePass *equivalence_pass,MagickBooleanType status)
{
  const Image
    *const image = equivalence_pass->image;

  CacheView
    *const image_view = equivalence_pass->image_view;

  MatrixInfo
    *const equivalences = equivalence_pass->equivalences;

  const size_t
    connectivity = equivalence_pass->connectivity;

  ssize_t
    (*const connect4)[2] = equivalence_pass->connect4,
    (*const connect8)[2] = equivalence_pass->connect8;

  const ssize_t
    n = equivalence_pass->n;

  ExceptionInfo
    *const exception = equivalence_pass->exception;

  CCNeighborDirection direction;

  ssize_t
    dx,
    dy,
    y;

  dx=connectivity > 4 ? connect8[n][1] : connect4[n][1];
  dy=connectivity > 4 ? connect8[n][0] : connect4[n][0];
  direction.dx=dx;
  direction.dy=dy;

  for (y=0; y < (ssize_t) image->rows; y++)
  {
    CCEquivalenceRow
      equivalence_row;

    const Quantum
      *magick_restrict p;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y-1,image->columns,3,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    p+=(ptrdiff_t) GetPixelChannels(image)*image->columns;
    equivalence_row.image=image;
    equivalence_row.equivalences=equivalences;
    equivalence_row.y=y;
    equivalence_row.dx=dx;
    equivalence_row.dy=dy;
    status=ResolveRowEquivalences(&equivalence_row,direction,p,status);
  }
  return(status);
}

typedef struct _CCEquivalenceInputs
{
  const Image
    *image;

  CacheView
    *image_view;

  MatrixInfo
    *equivalences;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2];

  ExceptionInfo
    *exception;
} CCEquivalenceInputs;

static MagickBooleanType ResolveEquivalences(
  const CCEquivalenceInputs *equivalence_inputs,MagickBooleanType status)
{
  const Image
    *const image = equivalence_inputs->image;

  CacheView
    *const image_view = equivalence_inputs->image_view;

  MatrixInfo
    *const equivalences = equivalence_inputs->equivalences;

  const size_t
    connectivity = equivalence_inputs->connectivity;

  ssize_t
    (*const connect4)[2] = equivalence_inputs->connect4,
    (*const connect8)[2] = equivalence_inputs->connect8;

  ExceptionInfo
    *const exception = equivalence_inputs->exception;

  ssize_t
    n;

  for (n=0; n < (ssize_t) (connectivity > 4 ? 4 : 2); n++)
  {
    CCEquivalencePass
      equivalence_pass;

    if (status == MagickFalse)
      continue;
    equivalence_pass.image=image;
    equivalence_pass.image_view=image_view;
    equivalence_pass.equivalences=equivalences;
    equivalence_pass.connectivity=connectivity;
    equivalence_pass.connect4=connect4;
    equivalence_pass.connect8=connect8;
    equivalence_pass.n=n;
    equivalence_pass.exception=exception;
    status=ResolveDirectionEquivalences(&equivalence_pass,status);
  }
  return(status);
}

typedef struct _CCObjectCensus
{
  CacheView
    *component_view;

  Image
    *component_image;

  CacheView
    *object_view;

  CCObjectInfo
    *object;

  size_t
    *neighbors;

  RectangleInfo
    bounding_box;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    i;

  ExceptionInfo
    *exception;
} CCObjectCensus;

static void CountMergedObjectNeighbors(const CCObjectCensus *object_census,
  MagickBooleanType *status,size_t *count)
{
  CacheView
    *const component_view = object_census->component_view;

  Image
    *const component_image = object_census->component_image;

  CacheView
    *const object_view = object_census->object_view;

  CCObjectInfo
    *const object = object_census->object;

  size_t
    *const neighbors = object_census->neighbors;

  const RectangleInfo
    bounding_box = object_census->bounding_box;

  const size_t
    connectivity = object_census->connectivity;

  ssize_t
    (*const connect4)[2] = object_census->connect4,
    (*const connect8)[2] = object_census->connect8;

  const ssize_t
    i = object_census->i;

  ExceptionInfo
    *const exception = object_census->exception;

  ssize_t
    y;

  for (y=0; y < (ssize_t) bounding_box.height; y++)
  {
    CCAdjacentRow
      adjacent_row;

    const Quantum
      *magick_restrict p;

    const CCAdjacentTargets
      adjacent_targets = { status, count, &p };

    if (*status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(component_view,bounding_box.x,
      bounding_box.y+y,bounding_box.width,1,exception);
    if (p == (const Quantum *) NULL)
      {
        *status=MagickFalse;
        continue;
      }
    adjacent_row.component_image=component_image;
    adjacent_row.object_view=object_view;
    adjacent_row.object=object;
    adjacent_row.neighbors=neighbors;
    adjacent_row.bounding_box=bounding_box;
    adjacent_row.connectivity=connectivity;
    adjacent_row.connect4=connect4;
    adjacent_row.connect8=connect8;
    adjacent_row.y=y;
    adjacent_row.i=i;
    adjacent_row.exception=exception;
    CountAdjacentObjectsInRow(&adjacent_row,&adjacent_targets);
  }
}

typedef struct _CCObjectMerge
{
  Image
    *component_image;

  CacheView
    *component_view;

  RectangleInfo
    bounding_box;

  ssize_t
    i;

  size_t
    id;

  ExceptionInfo
    *exception;
} CCObjectMerge;

static MagickBooleanType RelabelMergedObject(const CCObjectMerge *object_merge,
  MagickBooleanType status)
{
  Image
    *const component_image = object_merge->component_image;

  CacheView
    *const component_view = object_merge->component_view;

  const RectangleInfo
    bounding_box = object_merge->bounding_box;

  const ssize_t
    i = object_merge->i;

  const size_t
    id = object_merge->id;

  ExceptionInfo
    *const exception = object_merge->exception;

  ssize_t
    y;

  for (y=0; y < (ssize_t) bounding_box.height; y++)
  {
    CCObjectRelabel
      object_relabel;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(component_view,bounding_box.x,
      bounding_box.y+y,bounding_box.width,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    object_relabel.component_image=component_image;
    object_relabel.i=i;
    object_relabel.id=id;
    RelabelObjectRow(&object_relabel,bounding_box,q);
    if (SyncCacheViewAuthenticPixels(component_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  return(status);
}

typedef struct _CCLabelRow
{
  const Image
    *image;

  MatrixInfo
    *equivalences;

  CCObjectInfo
    *object;

  Image
    *component_image;

  ssize_t
    y;
} CCLabelRow;

static void LabelObjectRow(const CCLabelRow *label_row,
  const CCLabelTargets label_targets)
{
  const Image
    *const image = label_row->image;

  MatrixInfo
    *const equivalences = label_row->equivalences;

  CCObjectInfo
    *const object = label_row->object;

  Image
    *const component_image = label_row->component_image;

  const ssize_t
    y = label_row->y;

  ssize_t
    x;

  for (x=0; x < (ssize_t) component_image->columns; x++)
  {
    CCLabelPixel
      label_pixel;

    label_pixel.image=image;
    label_pixel.equivalences=equivalences;
    label_pixel.object=object;
    label_pixel.component_image=component_image;
    label_pixel.x=x;
    label_pixel.y=y;
    if (LabelObjectPixel(&label_pixel,&label_targets) == MagickFalse)
      break;
  }
}

typedef struct _CCLabeledObjects
{
  const Image
    *image;

  CCObjectInfo
    *object;

  Image
    *component_image;

  ssize_t
    n;
} CCLabeledObjects;

static void NormalizeLabeledObjects(const CCLabeledObjects *labeled_objects,
  ssize_t *background_id,double *min_threshold,double *max_threshold)
{
  const Image
    *const image = labeled_objects->image;

  CCObjectInfo
    *const object = labeled_objects->object;

  Image
    *const component_image = labeled_objects->component_image;

  const ssize_t
    n = labeled_objects->n;

  ssize_t
    i;

  *background_id=0;
  *min_threshold=0.0;
  *max_threshold=0.0;
  component_image->colors=(size_t) n;
  for (i=0; i < (ssize_t) component_image->colors; i++)
  {
    NormalizeLabeledObject(image,object,i);
    *max_threshold+=object[i].area;
    if (object[i].area > object[*background_id].area)
      *background_id=i;
  }
  *max_threshold+=MagickEpsilon;
}

static MagickBooleanType KeepTopObjects(const Image *component_image,
  CCObjectInfo *object,const char *artifact)
{
  CCObjectInfo
    *top_objects;

  ssize_t
    top_ids;

  top_ids=GetTopObjectIds(component_image,artifact);
  top_objects=(CCObjectInfo *) AcquireQuantumMemory(component_image->colors,
    sizeof(*top_objects));
  if (top_objects == (CCObjectInfo *) NULL)
    return(MagickFalse);
  MergeObjectsBelowTop(component_image,object,top_objects,top_ids);
  top_objects=(CCObjectInfo *) RelinquishMagickMemory(top_objects);
  return(MagickTrue);
}

typedef struct _CCMergeObject
{
  CacheView
    *component_view;

  Image
    *component_image;

  CacheView
    *object_view;

  CCObjectInfo
    *object;

  size_t
    *neighbors;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    i;

  ExceptionInfo
    *exception;
} CCMergeObject;

static MagickBooleanType MergeObject(const CCMergeObject *merge_object,
  MagickBooleanType status)
{
  CacheView
    *const component_view = merge_object->component_view;

  Image
    *const component_image = merge_object->component_image;

  CacheView
    *const object_view = merge_object->object_view;

  CCObjectInfo
    *const object = merge_object->object;

  size_t
    *const neighbors = merge_object->neighbors;

  const size_t
    connectivity = merge_object->connectivity;

  ssize_t
    (*const connect4)[2] = merge_object->connect4,
    (*const connect8)[2] = merge_object->connect8;

  const ssize_t
    i = merge_object->i;

  ExceptionInfo
    *const exception = merge_object->exception;

  CCObjectCensus
    object_census;

  CCObjectMerge
    object_merge;

  RectangleInfo
    bounding_box;

  size_t
    count,
    id;

  count=0;
  bounding_box=object[i].bounding_box;
  object_census.component_view=component_view;
  object_census.component_image=component_image;
  object_census.object_view=object_view;
  object_census.object=object;
  object_census.neighbors=neighbors;
  object_census.bounding_box=bounding_box;
  object_census.connectivity=connectivity;
  object_census.connect4=connect4;
  object_census.connect8=connect8;
  object_census.i=i;
  object_census.exception=exception;
  CountMergedObjectNeighbors(&object_census,&status,&count);
  /*
    Merge with object of greatest adjacent area.
  */
  id=GetGreatestAdjacentObject(object,neighbors,count);
  ResetAdjacentCensus(object,neighbors,count);
  object[i].area=0.0;
  object_merge.component_image=component_image;
  object_merge.component_view=component_view;
  object_merge.bounding_box=bounding_box;
  object_merge.i=i;
  object_merge.id=id;
  object_merge.exception=exception;
  status=RelabelMergedObject(&object_merge,status);
  return(status);
}

typedef struct _CCMergeInputs
{
  Image
    *component_image;

  CCObjectInfo
    *object;

  size_t
    *neighbors;

  size_t
    connectivity;

  ssize_t
    (*connect4)[2],
    (*connect8)[2],
    background_id;

  ExceptionInfo
    *exception;
} CCMergeInputs;

static MagickBooleanType MergeSelectedObjects(
  const CCMergeInputs *merge_inputs,MagickBooleanType status)
{
  Image
    *const component_image = merge_inputs->component_image;

  CCObjectInfo
    *const object = merge_inputs->object;

  size_t
    *const neighbors = merge_inputs->neighbors;

  const size_t
    connectivity = merge_inputs->connectivity;

  ssize_t
    (*const connect4)[2] = merge_inputs->connect4,
    (*const connect8)[2] = merge_inputs->connect8;

  const ssize_t
    background_id = merge_inputs->background_id;

  ExceptionInfo
    *const exception = merge_inputs->exception;

  CacheView
    *component_view,
    *object_view;

  ssize_t
    i;

  component_view=AcquireAuthenticCacheView(component_image,exception);
  object_view=AcquireVirtualCacheView(component_image,exception);
  (void) SetCacheViewVirtualPixelMethod(object_view,TileVirtualPixelMethod);
  for (i=0; i < (ssize_t) component_image->colors; i++)
  {
    CCMergeObject
      merge_object;

    if (status == MagickFalse)
      continue;
    if ((object[i].merge == MagickFalse) || (i == background_id))
      continue;  /* keep object */
    /*
      Merge this object.
    */
    merge_object.component_view=component_view;
    merge_object.component_image=component_image;
    merge_object.object_view=object_view;
    merge_object.object=object;
    merge_object.neighbors=neighbors;
    merge_object.connectivity=connectivity;
    merge_object.connect4=connect4;
    merge_object.connect8=connect8;
    merge_object.i=i;
    merge_object.exception=exception;
    status=MergeObject(&merge_object,status);
  }
  object_view=DestroyCacheView(object_view);
  component_view=DestroyCacheView(component_view);
  return(status);
}

#endif
