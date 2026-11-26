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

  MagickCore private methods of stream.c: StreamImagePixels, with the
  per-storage-type dispatchers and the map and storage tables it uses;
  included by that file only.
*/
#ifndef MAGICKCORE_STREAM_PIXELS_PRIVATE_H
#define MAGICKCORE_STREAM_PIXELS_PRIVATE_H

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S t r e a m I m a g e P i x e l s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StreamImagePixels() extracts pixel data from an image and returns it in the
%  stream_info->pixels structure in the format as defined by
%  stream_info->quantum_info->map and stream_info->quantum_info->storage_type.
%
%  The format of the StreamImagePixels method is:
%
%      MagickBooleanType StreamImagePixels(const StreamInfo *stream_info,
%        const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o stream_info: the stream info.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void StreamCharPixels(const StreamInfo *stream_info,const Image *image,
  QuantumType *quantum_map,size_t length)
{
  StreamRowInfo
    row;

  row.image=image;
  row.quantum_map=quantum_map;
  row.length=length;
  if (LocaleCompare(stream_info->map,"BGR") == 0)
    {
      StreamPixelQueue(&row,StreamCharBGRPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRA") == 0)
    {
      StreamPixelQueue(&row,StreamCharBGRAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRP") == 0)
    {
      StreamPixelQueue(&row,StreamCharBGRPPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"I") == 0)
    {
      StreamPixelQueue(&row,StreamCharIPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGB") == 0)
    {
      StreamPixelQueue(&row,StreamCharRGBPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBA") == 0)
    {
      StreamPixelQueue(&row,StreamCharRGBAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBP") == 0)
    {
      StreamPixelQueue(&row,StreamCharRGBPPixels,stream_info->pixels);
      return;
    }
  StreamPixelQueue(&row,StreamCharMapPixels,stream_info->pixels);
}

static void StreamDoublePixels(const StreamInfo *stream_info,const Image *image,
  QuantumType *quantum_map,size_t length)
{
  QuantumInfo
    *quantum_info;

  StreamRowInfo
    row;

  quantum_info=stream_info->quantum_info;
  row.image=image;
  row.quantum_info=quantum_info;
  row.quantum_map=quantum_map;
  row.length=length;
  if (LocaleCompare(stream_info->map,"BGR") == 0)
    {
      StreamPixelQueue(&row,StreamDoubleBGRPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRA") == 0)
    {
      StreamPixelQueue(&row,StreamDoubleBGRAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRP") == 0)
    {
      StreamPixelQueue(&row,StreamDoubleBGRPPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"I") == 0)
    {
      StreamPixelQueue(&row,StreamDoubleIPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGB") == 0)
    {
      StreamPixelQueue(&row,StreamDoubleRGBPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBA") == 0)
    {
      StreamPixelQueue(&row,StreamDoubleRGBAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBP") == 0)
    {
      StreamPixelQueue(&row,StreamDoubleRGBPPixels,stream_info->pixels);
      return;
    }
  StreamPixelQueue(&row,StreamDoubleMapPixels,stream_info->pixels);
}

static void StreamFloatPixels(const StreamInfo *stream_info,const Image *image,
  QuantumType *quantum_map,size_t length)
{
  QuantumInfo
    *quantum_info;

  StreamRowInfo
    row;

  quantum_info=stream_info->quantum_info;
  row.image=image;
  row.quantum_info=quantum_info;
  row.quantum_map=quantum_map;
  row.length=length;
  if (LocaleCompare(stream_info->map,"BGR") == 0)
    {
      StreamPixelQueue(&row,StreamFloatBGRPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRA") == 0)
    {
      StreamPixelQueue(&row,StreamFloatBGRAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRP") == 0)
    {
      StreamPixelQueue(&row,StreamFloatBGRPPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"I") == 0)
    {
      StreamPixelQueue(&row,StreamFloatIPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGB") == 0)
    {
      StreamPixelQueue(&row,StreamFloatRGBPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBA") == 0)
    {
      StreamPixelQueue(&row,StreamFloatRGBAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBP") == 0)
    {
      StreamPixelQueue(&row,StreamFloatRGBPPixels,stream_info->pixels);
      return;
    }
  StreamPixelQueue(&row,StreamFloatMapPixels,stream_info->pixels);
}

static void StreamLongPixels(const StreamInfo *stream_info,const Image *image,
  QuantumType *quantum_map,size_t length)
{
  StreamRowInfo
    row;

  row.image=image;
  row.quantum_map=quantum_map;
  row.length=length;
  if (LocaleCompare(stream_info->map,"BGR") == 0)
    {
      StreamPixelQueue(&row,StreamLongBGRPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRA") == 0)
    {
      StreamPixelQueue(&row,StreamLongBGRAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRP") == 0)
    {
      StreamPixelQueue(&row,StreamLongBGRPPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"I") == 0)
    {
      StreamPixelQueue(&row,StreamLongIPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGB") == 0)
    {
      StreamPixelQueue(&row,StreamLongRGBPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBA") == 0)
    {
      StreamPixelQueue(&row,StreamLongRGBAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBP") == 0)
    {
      StreamPixelQueue(&row,StreamLongRGBPPixels,stream_info->pixels);
      return;
    }
  StreamPixelQueue(&row,StreamLongMapPixels,stream_info->pixels);
}

static void StreamLongLongPixels(const StreamInfo *stream_info,
  const Image *image,QuantumType *quantum_map,size_t length)
{
  StreamRowInfo
    row;

  row.image=image;
  row.quantum_map=quantum_map;
  row.length=length;
  if (LocaleCompare(stream_info->map,"BGR") == 0)
    {
      StreamPixelQueue(&row,StreamLongLongBGRPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRA") == 0)
    {
      StreamPixelQueue(&row,StreamLongLongBGRAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRP") == 0)
    {
      StreamPixelQueue(&row,StreamLongLongBGRPPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"I") == 0)
    {
      StreamPixelQueue(&row,StreamLongLongIPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGB") == 0)
    {
      StreamPixelQueue(&row,StreamLongLongRGBPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBA") == 0)
    {
      StreamPixelQueue(&row,StreamLongLongRGBAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBP") == 0)
    {
      StreamPixelQueue(&row,StreamLongLongRGBPPixels,stream_info->pixels);
      return;
    }
  StreamPixelQueue(&row,StreamLongLongMapPixels,stream_info->pixels);
}

static void StreamQuantumPixels(const StreamInfo *stream_info,
  const Image *image,QuantumType *quantum_map,size_t length)
{
  StreamRowInfo
    row;

  row.image=image;
  row.quantum_map=quantum_map;
  row.length=length;
  if (LocaleCompare(stream_info->map,"BGR") == 0)
    {
      StreamPixelQueue(&row,StreamQuantumBGRPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRA") == 0)
    {
      StreamPixelQueue(&row,StreamQuantumBGRAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRP") == 0)
    {
      StreamPixelQueue(&row,StreamQuantumBGRPPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"I") == 0)
    {
      StreamPixelQueue(&row,StreamQuantumIPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGB") == 0)
    {
      StreamPixelQueue(&row,StreamQuantumRGBPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBA") == 0)
    {
      StreamPixelQueue(&row,StreamQuantumRGBAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBP") == 0)
    {
      StreamPixelQueue(&row,StreamQuantumRGBPPixels,stream_info->pixels);
      return;
    }
  StreamPixelQueue(&row,StreamQuantumMapPixels,stream_info->pixels);
}

static void StreamShortPixels(const StreamInfo *stream_info,const Image *image,
  QuantumType *quantum_map,size_t length)
{
  StreamRowInfo
    row;

  row.image=image;
  row.quantum_map=quantum_map;
  row.length=length;
  if (LocaleCompare(stream_info->map,"BGR") == 0)
    {
      StreamPixelQueue(&row,StreamShortBGRPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRA") == 0)
    {
      StreamPixelQueue(&row,StreamShortBGRAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRP") == 0)
    {
      StreamPixelQueue(&row,StreamShortBGRPPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"I") == 0)
    {
      StreamPixelQueue(&row,StreamShortIPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGB") == 0)
    {
      StreamPixelQueue(&row,StreamShortRGBPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBA") == 0)
    {
      StreamPixelQueue(&row,StreamShortRGBAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBP") == 0)
    {
      StreamPixelQueue(&row,StreamShortRGBPPixels,stream_info->pixels);
      return;
    }
  StreamPixelQueue(&row,StreamShortMapPixels,stream_info->pixels);
}

typedef struct _StreamMapInfo
{
  char
    key;

  QuantumType
    quantum;

  MagickBooleanType
    separation;
} StreamMapInfo;

static const StreamMapInfo *GetStreamMapInfo(const char key)
{
  static const StreamMapInfo
    map_info[] =
    {
      { 'A', AlphaQuantum, MagickFalse },
      { 'a', AlphaQuantum, MagickFalse },
      { 'B', BlueQuantum, MagickFalse },
      { 'b', BlueQuantum, MagickFalse },
      { 'C', CyanQuantum, MagickTrue },
      { 'c', CyanQuantum, MagickTrue },
      { 'G', GreenQuantum, MagickFalse },
      { 'g', GreenQuantum, MagickFalse },
      { 'I', IndexQuantum, MagickFalse },
      { 'i', IndexQuantum, MagickFalse },
      { 'K', BlackQuantum, MagickTrue },
      { 'k', BlackQuantum, MagickTrue },
      { 'M', MagentaQuantum, MagickTrue },
      { 'm', MagentaQuantum, MagickTrue },
      { 'O', OpacityQuantum, MagickFalse },
      { 'o', OpacityQuantum, MagickFalse },
      { 'P', UndefinedQuantum, MagickFalse },
      { 'p', UndefinedQuantum, MagickFalse },
      { 'R', RedQuantum, MagickFalse },
      { 'r', RedQuantum, MagickFalse },
      { 'Y', YellowQuantum, MagickTrue },
      { 'y', YellowQuantum, MagickTrue }
    };

  ssize_t
    i;

  for (i=0; i < (ssize_t) (sizeof(map_info)/sizeof(*map_info)); i++)
    if (map_info[i].key == key)
      return(map_info+i);
  return((const StreamMapInfo *) NULL);
}

typedef struct _StreamStorageMethod
{
  StorageType
    storage_type;

  void
    (*stream_pixels)(const StreamInfo *,const Image *,QuantumType *,size_t);
} StreamStorageMethod;

static const StreamStorageMethod
  StreamStorageMethods[] =
  {
    { CharPixel, StreamCharPixels },
    { DoublePixel, StreamDoublePixels },
    { FloatPixel, StreamFloatPixels },
    { LongPixel, StreamLongPixels },
    { LongLongPixel, StreamLongLongPixels },
    { QuantumPixel, StreamQuantumPixels },
    { ShortPixel, StreamShortPixels }
  };

static const StreamStorageMethod *GetStreamStorageMethod(
  const StorageType storage_type)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) (sizeof(StreamStorageMethods)/
       sizeof(*StreamStorageMethods)); i++)
    if (StreamStorageMethods[i].storage_type == storage_type)
      return(StreamStorageMethods+i);
  return((const StreamStorageMethod *) NULL);
}

static inline MagickBooleanType IsStreamMapChannelAvailable(
  const StreamMapInfo *map_info,const Image *image)
{
  return(((map_info->separation == MagickFalse) ||
    (image->colorspace == CMYKColorspace)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType StreamImagePixels(const StreamInfo *stream_info,
  const Image *image,ExceptionInfo *exception)
{
  const StreamMapInfo
    *map_info;

  const StreamStorageMethod
    *storage_method;

  QuantumType
    *quantum_map;

  ssize_t
    i;

  size_t
    length;

  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  length=strlen(stream_info->map);
  quantum_map=(QuantumType *) AcquireQuantumMemory(length,sizeof(*quantum_map));
  if (quantum_map == (QuantumType *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(MagickFalse);
    }
  (void) memset(quantum_map,0,length*sizeof(*quantum_map));
  for (i=0; i < (ssize_t) length; i++)
  {
    map_info=GetStreamMapInfo(stream_info->map[i]);
    if (map_info == (const StreamMapInfo *) NULL)
      {
        quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "UnrecognizedPixelMap","`%s'",stream_info->map);
        return(MagickFalse);
      }
    quantum_map[i]=map_info->quantum;
    if (IsStreamMapChannelAvailable(map_info,image) != MagickFalse)
      continue;
    quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
    (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
      "ColorSeparatedImageRequired","`%s'",stream_info->map);
    return(MagickFalse);
  }
  storage_method=GetStreamStorageMethod(stream_info->storage_type);
  if (storage_method != (const StreamStorageMethod *) NULL)
    storage_method->stream_pixels(stream_info,image,quantum_map,length);
  else
    {
      quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "UnrecognizedPixelMap","`%s'",stream_info->map);
    }
  quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
  return(MagickTrue);
}

#endif
