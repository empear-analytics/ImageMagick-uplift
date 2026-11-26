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

  MagickCore private methods of resource.c: the temporary file helpers of
  AcquireUniqueFileResource, DestroyTemporaryResources to
  AcquireResourceRandomInfo; included by that file only.
*/
#ifndef MAGICKCORE_RESOURCE_UNIQUE_PRIVATE_H
#define MAGICKCORE_RESOURCE_UNIQUE_PRIVATE_H

static void *DestroyTemporaryResources(void *temporary_resource)
{
  (void) ShredFile((char *) temporary_resource);
  (void) remove_utf8((char *) temporary_resource);
  temporary_resource=DestroyString((char *) temporary_resource);
  return((void *) NULL);
}

static void FillPathTemplate(char *path,const size_t length)
{
  char
    *p;

  int
    c;

  ssize_t
    j;

  static const char
    portable_filename[65] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";

  StringInfo
    *key;

  unsigned char
    *datum;

  key=GetRandomKey(random_info,length);
  p=path+strlen(path)-strlen(MagickPathTemplate);
  datum=GetStringInfoDatum(key);
  for (j=0; j < (ssize_t) GetStringInfoLength(key); j++)
  {
    c=(int) (datum[j] & 0x3f);
    *p++=portable_filename[c];
  }
  key=DestroyStringInfo(key);
}

static MagickBooleanType AddTemporaryResource(const char *path)
{
  MagickBooleanType
    status;

  struct stat
    *attributes;

  LockResourceSemaphore(FileResource);
  if (temporary_resources == (SplayTreeInfo *) NULL)
    temporary_resources=NewSplayTree(CompareSplayTreeString,
      DestroyTemporaryResources,RelinquishMagickMemory);
  UnlockSemaphoreInfo(resource_semaphore[FileResource]);
  attributes=(struct stat *) AcquireCriticalMemory(sizeof(struct stat));
  status=GetPathAttributes(path,attributes);
  if (status == MagickFalse)
    attributes=(struct stat *) RelinquishMagickMemory(attributes);
  else
    status=AddValueToSplayTree(temporary_resources,ConstantString(path),
      attributes);
  return(status);
}

static int RegisterUniqueFileResource(const char *path,int file)
{
  MagickBooleanType
    status;

  if (file == -1)
    return(file);
  status=AddTemporaryResource(path);
  if (status == MagickFalse)
    file=close_utf8(file)-1;
  return(file);
}

static void AcquireResourceRandomInfo(void)
{
  if (random_info == (RandomInfo *) NULL)
    {
      LockResourceSemaphore(FileResource);
      if (random_info == (RandomInfo *) NULL)
        random_info=AcquireRandomInfo();
      UnlockSemaphoreInfo(resource_semaphore[FileResource]);
    }
}

#endif
