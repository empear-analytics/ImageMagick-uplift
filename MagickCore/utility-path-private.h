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

  MagickCore private methods of utility.c: the path component helpers of
  GetPathComponent, IsImageFormatPrefix to OpenPathComponent; included by
  that file only.
*/
#ifndef MAGICKCORE_UTILITY_PATH_PRIVATE_H
#define MAGICKCORE_UTILITY_PATH_PRIVATE_H

static inline MagickBooleanType IsImageFormatPrefix(const char *component,
  const char *p)
{
  return(((p != component) && (*p == ':') && (IsPathDirectory(component) < 0) &&
    (IsPathAccessible(component) == MagickFalse)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsCompressionExtension(const char *extension)
{
  return(((LocaleCompare(extension,"bz2") == 0) ||
    (LocaleCompare(extension,"gz") == 0) ||
    (LocaleCompare(extension,"svgz") == 0) ||
    (LocaleCompare(extension,"wmz") == 0) ||
    (LocaleCompare(extension,"Z") == 0)) ? MagickTrue : MagickFalse);
}

static void RemoveRootExtension(char *component)
{
  char
    *p;

  for (p=component+(strlen(component)-1); p > component; p--)
  {
    if (IsBasenameSeparator(*p) != MagickFalse)
      break;
    if (*p == '.')
      break;
  }
  if (*p == '.')
    *p='\0';
}

static void RemoveBaseExtension(char *component)
{
  char
    *p;

  if (*component != '\0')
    for (p=component+(strlen(component)-1); p > component; p--)
      if (*p == '.')
        {
          *p='\0';
          break;
        }
}

static void GetExtensionComponent(char *component,char *p)
{
  if (IsBasenameSeparator(*p) != MagickFalse)
    (void) CopyMagickString(component,p+1,MagickPathExtent);
  if (*component != '\0')
    for (p=component+strlen(component)-1; p > component; p--)
      if (*p == '.')
        break;
  *component='\0';
  if (*p == '.')
    (void) CopyMagickString(component,p+1,MagickPathExtent);
}

static size_t StripImageFormatPrefix(char *component,char *p,
  size_t magick_length)
{
  char
    *q;

  *p='\0';
  if (IsMagickConflict(component) != MagickFalse)
    *p=':';
  else
    {
      magick_length=(size_t) (p-component+1);
      for (q=component; *(++p) != '\0'; q++)
        *q=(*p);
      *q='\0';
    }
  return(magick_length);
}

typedef struct _SubimageBrackets
{
  char
    *component,
    *p,
    *q;
} SubimageBrackets;

static void GetSceneSpecification(const SubimageBrackets *brackets,
  size_t *subimage_length,size_t *subimage_offset)
{
  char
    *const component = brackets->component,
    *const p = brackets->p,
    *const q = brackets->q;

  *p='\0';
  if ((IsSceneGeometry(q+1,MagickFalse) == MagickFalse) &&
      (IsGeometry(q+1) == MagickFalse))
    *p=']';
  else
    {
      (*subimage_length)=(size_t) (p-q);
      (*subimage_offset)=(size_t) (q-component+1);
      *q='\0';
    }
}

static void GetSubimageSpecification(const char *path,char *component,
  size_t *subimage_length,size_t *subimage_offset)
{
  char
    *p,
    *q;

  p=component+strlen(component)-1;
  if ((strlen(component) > 2) && (*p == ']'))
    {
      q=strrchr(component,'[');
      if (q != (char *) NULL)
        {
          ExceptionInfo *exception = AcquireExceptionInfo();
          char *literal = (char *) GetImageRegistry(StringRegistryType,
            "filename:literal",exception);
          exception=DestroyExceptionInfo(exception);
          if ((IsStringTrue(literal) == MagickFalse) ||
              (IsPathAccessible(path) == MagickFalse))
            {
              SubimageBrackets
                brackets;

              /*
                Look for scene specification (e.g. img0001.pcd[4]).
              */
              brackets.component=component;
              brackets.p=p;
              brackets.q=q;
              GetSceneSpecification(&brackets,&(*subimage_length),
                &(*subimage_offset));
            }
          if (literal != (char *) NULL)
            literal=DestroyString(literal);
        }
    }
}

static size_t GetImageFormatPrefix(char *component,size_t magick_length)
{
  char
    *p;

  for (p=component; *p != '\0'; p++)
  {
    if ((*p == '%') && (*(p+1) == '['))
      {
        /*
          Skip over %[...].
        */
        for (p++; (*p != ']') && (*p != '\0'); p++) ;
        if (*p == '\0')
          break;
      }
    if (IsImageFormatPrefix(component,p) != MagickFalse)
      {
        /*
          Look for image format specification (e.g. ps3:image).
        */
        magick_length=StripImageFormatPrefix(component,p,magick_length);
        break;
      }
  }
  return(magick_length);
}

static char *LocateBasenameSeparator(char *component)
{
  char
    *p;

  p=component;
  if (*p != '\0')
    for (p=component+strlen(component)-1; p > component; p--)
      if (IsBasenameSeparator(*p) != MagickFalse)
        break;
  return(p);
}

static void GetBasePathSansCompressExtension(const char *path,
  char *component)
{
  char
    extension[MagickPathExtent];

  /*
    Base path sans any compression extension.
  */
  GetPathComponent(path,ExtensionPath,extension);
  if (IsCompressionExtension(extension) != MagickFalse)
    GetPathComponent(path,BasePath,component);
}

static void SelectPathHeadComponent(char *component,PathType type,char *p)
{
  switch (type)
  {
    case RootPath:
    {
      if (*component != '\0')
        {
          RemoveRootExtension(component);
          break;
        }
      magick_fallthrough;
    }
    case HeadPath:
    {
      *p='\0';
      break;
    }
    case TailPath:
    {
      if (IsBasenameSeparator(*p) != MagickFalse)
        (void) CopyMagickString(component,p+1,MagickPathExtent);
      break;
    }
    default:
      break;
  }
}

static void SelectPathTailComponent(const char *path,PathType type,
  char *component,char *p)
{
  switch (type)
  {
    case BasePath:
    {
      if (IsBasenameSeparator(*p) != MagickFalse)
        (void) CopyMagickString(component,p+1,MagickPathExtent);
      RemoveBaseExtension(component);
      break;
    }
    case BasePathSansCompressExtension:
    {
      GetBasePathSansCompressExtension(path,component);
      break;
    }
    case ExtensionPath:
    {
      GetExtensionComponent(component,p);
      break;
    }
    default:
    {
      SelectPathHeadComponent(component,type,p);
      break;
    }
  }
}

static void CopyMagickComponent(const char *path,char *component,
  const size_t magick_length)
{
  if (magick_length != 0)
    (void) CopyMagickString(component,path,magick_length);
  else
    *component='\0';
}

static inline MagickBooleanType IsSubimageComponent(const size_t magick_length,
  const size_t subimage_offset,const size_t subimage_length)
{
  return(((subimage_length != 0) && (magick_length < subimage_offset)) ?
    MagickTrue : MagickFalse);
}

static MagickBooleanType OpenPathComponent(const char *path,char *component)
{
  assert(path != (const char *) NULL);
  assert(component != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",path);
  if (*path == '\0')
    {
      *component='\0';
      return(MagickFalse);
    }
  return(MagickTrue);
}

#endif
