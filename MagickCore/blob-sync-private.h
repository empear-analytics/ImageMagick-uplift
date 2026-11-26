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

  MagickCore private methods of blob-stream.c that its split files share:
  IsWriteBlobMode and SyncBlob.
*/
#ifndef MAGICKCORE_BLOB_SYNC_PRIVATE_H
#define MAGICKCORE_BLOB_SYNC_PRIVATE_H

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  S y n c B l o b                                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncBlob() flushes the datastream if it is a file or synchronizes the data
%  attributes if it is an blob.  It returns 0 on success; otherwise, it returns
%  -1 and set errno to indicate the error.
%
%  The format of the SyncBlob method is:
%
%      int SyncBlob(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
static inline MagickBooleanType IsWriteBlobMode(const BlobInfo *blob_info)
{
  return(((blob_info->mode == WriteBlobMode) ||
    (blob_info->mode == WriteBinaryBlobMode) ||
    (blob_info->mode == AppendBlobMode) ||
    (blob_info->mode == AppendBinaryBlobMode)) ? MagickTrue : MagickFalse);
}

static int SyncBlob(const Image *image)
{
  BlobInfo
    *magick_restrict blob_info;

  int
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (EOFBlob(image) != 0)
    return(0);
  blob_info=image->blob;
  status=0;
  switch (blob_info->type)
  {
    case UndefinedStream:
      break;
    case StandardStream:
    {
      if (IsWriteBlobMode(blob_info) != MagickFalse)
        status=fflush(blob_info->file_info.file);
      break;
    }
    case FileStream:
    case PipeStream:
    {
      status=fflush(blob_info->file_info.file);
      break;
    }
    case ZipStream:
    {
#if defined(MAGICKCORE_ZLIB_DELEGATE)
      (void) gzflush(blob_info->file_info.gzfile,Z_SYNC_FLUSH);
#endif
      break;
    }
    case BZipStream:
    {
#if defined(MAGICKCORE_BZLIB_DELEGATE)
      status=BZ2_bzflush(blob_info->file_info.bzfile);
#endif
      break;
    }
    case FifoStream:
      break;
    case BlobStream:
      break;
    case CustomStream:
      break;
  }
  return(status);
}

#endif
