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

  MagickCore private methods of morphology-primitive.c: the direct (in-place)
  pass, from ReverseDirectChannelValue to MorphologyPrimitiveDirect; included
  by that file only.
*/
#ifndef MAGICKCORE_MORPHOLOGY_PRIMITIVE_DIRECT_PRIVATE_H
#define MAGICKCORE_MORPHOLOGY_PRIMITIVE_DIRECT_PRIVATE_H

static double ReverseDirectChannelValue(const MorphologyDirectScan *scan,
  const MorphologyDirectPixel *input,const Quantum *magick_restrict pixels,
  double pixel)
{
  Image
    *const image = input->image;

  const KernelInfo
    *const kernel = input->kernel;

  Quantum
    *const q = input->q;

  const ssize_t
    x = input->x,
    i = input->i;

  const MorphologyMethod
    method = scan->method;

  const OffsetInfo
    offset = scan->offset;

  const MagickRealType
    *magick_restrict k;

  MorphologyDirectPixel
    row;

  MorphologyKernelRows
    rows;

  switch (method)
  {
    case DistanceMorphology:
    {
      rows.image=image;
      rows.kernel=kernel;
      rows.i=i;
      pixel=MinimizeTrailingRowsDistance(&rows,offset,pixels,pixel);
      k=(&kernel->values[(ssize_t) kernel->width*kernel->y+kernel->x-1]);
      row.image=image;
      row.kernel=kernel;
      row.q=q;
      row.x=x;
      row.i=i;
      pixel=MinimizeTrailingRowDistance(&row,offset,k,pixel);
      break;
    }
    case VoronoiMorphology:
    {
      rows.image=image;
      rows.kernel=kernel;
      rows.i=i;
      pixel=MinimizeTrailingRowsDistance(&rows,offset,pixels,pixel);
      k=(&kernel->values[(ssize_t) kernel->width*(kernel->y+1)-1]);
      row.image=image;
      row.kernel=kernel;
      row.q=q;
      row.x=x;
      row.i=i;
      pixel=MinimizeTrailingRowDistance(&row,offset,k,pixel);
      break;
    }
    default:
      break;
  }
  return(pixel);
}

static inline void MorphologyDirectChannel(const MorphologyDirectScan *scan,
  const MorphologyDirectPixel *input,const Quantum *magick_restrict p,
  size_t *changed)
{
  const DirectScanSide
    side = scan->side;

  Image
    *const image = input->image;

  Quantum
    *const q = input->q;

  const ssize_t
    i = input->i;

  double
    pixel;

  PixelChannel
    channel;

  PixelTrait
    traits;

  const Quantum
    *magick_restrict pixels;

  channel=GetPixelChannelChannel(image,i);
  traits=GetPixelChannelTraits(image,channel);
  if (traits == UndefinedPixelTrait)
    return;
  if ((traits & CopyPixelTrait) != 0)
    return;
  pixels=p;
  pixel=(double) QuantumRange;
  pixel=((side == ForwardDirectScan) ?
    ForwardDirectChannelValue(scan,input,pixels,pixel) :
    ReverseDirectChannelValue(scan,input,pixels,pixel));
  if (fabs(pixel-(double) q[i]) > MagickEpsilon)
    (*changed)++;
  q[i]=ClampToQuantum(pixel);
}

static inline void MorphologyDirectPixels(const MorphologyDirectScan *scan,
  const Quantum *magick_restrict p,Quantum *magick_restrict q,size_t *changed)
{
  const DirectScanSide
    side = scan->side;

  Image
    *const image = scan->image;

  const KernelInfo
    *const kernel = scan->kernel;

  ssize_t
    x;

  for (x=((side == ForwardDirectScan) ? 0 : (ssize_t) image->columns-1);
       ((side == ForwardDirectScan) ? x < (ssize_t) image->columns : x >= 0);
       ((side == ForwardDirectScan) ? x++ : x--))
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      const MorphologyDirectPixel
        input = { image, kernel, q, x, i };

      MorphologyDirectChannel(scan,&input,p,changed);
    }
    ((side == ForwardDirectScan) ? (p+=(ptrdiff_t) GetPixelChannels(image)) :
      (p-=(ptrdiff_t)GetPixelChannels(image)));
    ((side == ForwardDirectScan) ? (q+=(ptrdiff_t) GetPixelChannels(image)) :
      (q-=GetPixelChannels(image)));
  }
}

static inline MagickBooleanType MorphologyDirectScanRow(
  const MorphologyDirectScan *scan,const ssize_t y,size_t *changed,
  MagickBooleanType *status)
{
  const DirectScanSide
    side = scan->side;

  Image
    *const image = scan->image;

  const KernelInfo
    *const kernel = scan->kernel;

  const OffsetInfo
    offset = scan->offset;

  CacheView
    *const image_view = scan->image_view,
    *const morphology_view = scan->morphology_view;

  const size_t
    width = scan->width;

  ExceptionInfo
    *const exception = scan->exception;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(image_view,-offset.x,((side ==
    ForwardDirectScan) ? y-offset.y : y),width,((side == ForwardDirectScan) ?
    (size_t) offset.y+1 : (size_t) kernel->y+1),exception);
  q=GetCacheViewAuthenticPixels(morphology_view,0,y,image->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  if (side == ReverseDirectScan)
    {
      p+=(ptrdiff_t) (image->columns-1)*GetPixelChannels(image);
      q+=(ptrdiff_t) (image->columns-1)*GetPixelChannels(image);
    }
  MorphologyDirectPixels(scan,p,q,changed);
  if (SyncCacheViewAuthenticPixels(morphology_view,exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static inline MagickOffsetType MorphologyDirectPass(
  const MorphologyDirectScan *scan,MagickOffsetType progress,size_t *changed,
  MagickBooleanType *status)
{
  const DirectScanSide
    side = scan->side;

  Image
    *const image = scan->image;

  ssize_t
    y;

  for (y=((side == ForwardDirectScan) ? 0 : (ssize_t) image->rows-1);
       ((side == ForwardDirectScan) ? y < (ssize_t) image->rows : y >= 0);
       ((side == ForwardDirectScan) ? y++ : y--))
  {
    if (MorphologyDirectScanRow(scan,y,changed,status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportMorphologyProgress(image,progress,2*image->rows,status);
      }
  }
  return(progress);
}

static ssize_t MorphologyPrimitiveDirect(Image *image,
  const MorphologyMethod method,const KernelInfo *kernel,
  ExceptionInfo *exception)
{
  CacheView
    *morphology_view,
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  OffsetInfo
    offset;

  size_t
    width,
    changed;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(kernel != (KernelInfo *) NULL);
  assert(kernel->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  status=MagickTrue;
  changed=0;
  progress=0;
  offset=GetDirectKernelOffset(method,kernel);
  /*
    Two views into same image, do not thread.
  */
  image_view=AcquireVirtualCacheView(image,exception);
  morphology_view=AcquireAuthenticCacheView(image,exception);
  width=image->columns+kernel->width-1;
  {
    const MorphologyDirectScan
      scan = { ForwardDirectScan, image, method, kernel, offset, image_view,
        morphology_view, width, exception };

    /*
      Read virtual pixels, and authentic pixels, from the same image!  We read
      using virtual to get virtual pixel handling, but write back into the same
      image.

      Only top half of kernel is processed as we do a single pass downward
      through the image iterating the distance function as we go.
    */
    progress=MorphologyDirectPass(&scan,progress,&changed,&status);
  }
  morphology_view=DestroyCacheView(morphology_view);
  image_view=DestroyCacheView(image_view);
  /*
    Do the reverse pass through the image.
  */
  image_view=AcquireVirtualCacheView(image,exception);
  morphology_view=AcquireAuthenticCacheView(image,exception);
  {
    const MorphologyDirectScan
      scan = { ReverseDirectScan, image, method, kernel, offset, image_view,
        morphology_view, width, exception };

    /*
       Read virtual pixels, and authentic pixels, from the same image.  We
       read using virtual to get virtual pixel handling, but write back
       into the same image.

       Only the bottom half of the kernel is processed as we up the image.
    */
    progress=MorphologyDirectPass(&scan,progress,&changed,&status);
  }
  morphology_view=DestroyCacheView(morphology_view);
  image_view=DestroyCacheView(image_view);
  return(status ? (ssize_t) (changed/GetImageChannels(image)) : -1);
}

/*
  Apply a Morphology by calling one of the above low level primitive
  application functions.  This function handles any iteration loops,
  composition or re-iteration of results, and compound morphology methods that
  is based on multiple low-level (staged) morphology methods.

  Basically this provides the complex glue between the requested morphology
  method and raw low-level implementation (above).
*/

#endif
