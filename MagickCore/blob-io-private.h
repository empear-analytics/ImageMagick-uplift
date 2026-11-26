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

  MagickCore private methods of blob.c that its split files share:
  ThrowBlobException and WriteBlobStream.
*/
#ifndef MAGICKCORE_BLOB_IO_PRIVATE_H
#define MAGICKCORE_BLOB_IO_PRIVATE_H

static inline void ThrowBlobException(BlobInfo *blob_info)
{
  if ((blob_info->status == 0) && (errno != 0))
    blob_info->error_number=errno;
  blob_info->status=(-1);
}

static inline ssize_t WriteBlobStream(Image *image,const size_t length,
  const void *magick_restrict data)
{
  BlobInfo
    *magick_restrict blob_info;

  MagickSizeType
    extent;

  unsigned char
    *magick_restrict q;

  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  assert(data != NULL);
  blob_info=image->blob;
  if (blob_info->type != BlobStream)
    return(WriteBlob(image,length,(const unsigned char *) data));
  if (blob_info->offset > (MagickOffsetType) (MAGICK_SSIZE_MAX-length))
    {
      errno=EOVERFLOW;
      return(0);
    }
  extent=(MagickSizeType) (blob_info->offset+(MagickOffsetType) length);
  if (extent >= blob_info->extent)
    {
      extent+=blob_info->quantum+length;
      blob_info->quantum<<=1;
      if (SetBlobExtent(image,extent) == MagickFalse)
        return(0);
    }
  q=blob_info->data+blob_info->offset;
  (void) memcpy(q,data,length);
  blob_info->offset+=(MagickOffsetType) length;
  if (blob_info->offset >= (MagickOffsetType) blob_info->length)
    blob_info->length=(size_t) blob_info->offset;
  return((ssize_t) length);
}

#endif
