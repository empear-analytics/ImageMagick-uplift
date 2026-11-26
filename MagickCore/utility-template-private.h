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

  MagickCore private methods of utility.c: the temporary directory helpers
  of GetPathTemplate, GetTemporaryDirectory to FormatPathTemplate; included
  by that file only.
*/
#ifndef MAGICKCORE_UTILITY_TEMPLATE_PRIVATE_H
#define MAGICKCORE_UTILITY_TEMPLATE_PRIVATE_H

static char *GetTemporaryDirectory(void)
{
  char
    *directory;

  ExceptionInfo
    *exception;

  exception=AcquireExceptionInfo();
  directory=(char *) GetImageRegistry(StringRegistryType,"temporary-path",
    exception);
  exception=DestroyExceptionInfo(exception);
  if (directory == (char *) NULL)
    directory=GetEnvironmentValue("MAGICK_TEMPORARY_PATH");
  if (directory == (char *) NULL)
    directory=GetEnvironmentValue("MAGICK_TMPDIR");
  if (directory == (char *) NULL)
    directory=GetEnvironmentValue("TMPDIR");
  return(directory);
}

static MagickBooleanType ValidateTemporaryDirectory(char *directory)
{
  MagickBooleanType
    status;

  struct stat
    attributes;

  if (strlen(directory) > (MagickPathExtent-25))
    {
      directory=DestroyString(directory);
      return(MagickFalse);
    }
  status=GetPathAttributes(directory,&attributes);
  if ((status == MagickFalse) || !S_ISDIR(attributes.st_mode))
    {
      directory=DestroyString(directory);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void ApplyTemporaryPathPolicy(char **directory)
{
  char
    *value;

  value=GetPolicyValue("resource:temporary-path");
  if (value != (char *) NULL)
    {
      (void) CloneString(&(*directory),value);
      value=DestroyString(value);
    }
}

static void FormatPathTemplate(char *path,const char *directory)
{
  if (directory[strlen(directory)-1] == *DirectorySeparator)
    (void) FormatLocaleString(path,MagickPathExtent,"%smagick-"
      MagickPathTemplate,directory);
  else
    (void) FormatLocaleString(path,MagickPathExtent,
      "%s%smagick-" MagickPathTemplate,directory,DirectorySeparator);
}

#endif
