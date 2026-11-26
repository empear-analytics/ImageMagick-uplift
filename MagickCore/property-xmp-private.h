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

  MagickCore private methods of property.c: the XMP property reader, from
  SkipXMPValue to GetXMPProperty; included by that file only, where the run
  stood.
*/
#ifndef MAGICKCORE_PROPERTY_XMP_PRIVATE_H
#define MAGICKCORE_PROPERTY_XMP_PRIVATE_H

static MagickBooleanType SkipXMPValue(const char *value)
{
  if (value == (const char*) NULL)
    return(MagickTrue);
  while (*value != '\0')
  {
    if (isspace((int) ((unsigned char) *value)) == 0)
      return(MagickFalse);
    value++;
  }
  return(MagickTrue);
}

static inline MagickBooleanType IsNotNamespaceWildcard(
  const char *xmp_namespace,const size_t xmp_namespace_length)
{
  return(((xmp_namespace_length <= 2) ||
    (*(xmp_namespace+(xmp_namespace_length-2)) != ':') ||
    (*(xmp_namespace+(xmp_namespace_length-1)) != '*')) ? MagickTrue :
    MagickFalse);
}

static void AddXMPPropertyValue(const Image *image,XMLTreeInfo *node,
  const char *content)
{
  char
    *xmp_namespace;

  size_t
    xmp_namespace_length;

  xmp_namespace=ConstantString(GetXMLTreeTag(node));
  (void) SubstituteString(&xmp_namespace,"exif:","xmp:");
  xmp_namespace_length=strlen(xmp_namespace);
  if (IsNotNamespaceWildcard(xmp_namespace,xmp_namespace_length) !=
      MagickFalse)
    (void) AddValueToSplayTree((SplayTreeInfo *) image->properties,
     ConstantString(xmp_namespace),ConstantString(content));
  xmp_namespace=DestroyString(xmp_namespace);
}

static void AddXMPChildValues(const Image *image,XMLTreeInfo *node,
  XMLTreeInfo *child)
{
  const char
    *content;

  while (child != (XMLTreeInfo *) NULL)
  {
    content=GetXMLTreeContent(child);
    if (SkipXMPValue(content) == MagickFalse)
      AddXMPPropertyValue(image,node,content);
    child=GetXMLTreeSibling(child);
  }
}

static void AddXMPDescriptionValues(const Image *image,
  XMLTreeInfo *description)
{
  const char
    *content;

  XMLTreeInfo
    *child,
    *node;

  node=GetXMLTreeChild(description,(const char *) NULL);
  while (node != (XMLTreeInfo *) NULL)
  {
    child=GetXMLTreeChild(node,(const char *) NULL);
    content=GetXMLTreeContent(node);
    if ((child == (XMLTreeInfo *) NULL) &&
        (SkipXMPValue(content) == MagickFalse))
      AddXMPPropertyValue(image,node,content);
    AddXMPChildValues(image,node,child);
    node=GetXMLTreeSibling(node);
  }
}

static void AddXMPRDFValues(const Image *image,XMLTreeInfo *rdf)
{
  XMLTreeInfo
    *description;

  if (image->properties == (void *) NULL)
    ((Image *) image)->properties=NewSplayTree(CompareSplayTreeString,
      RelinquishMagickMemory,RelinquishMagickMemory);
  description=GetXMLTreeChild(rdf,"rdf:Description");
  while (description != (XMLTreeInfo *) NULL)
  {
    AddXMPDescriptionValues(image,description);
    description=GetNextXMLTreeTag(description);
  }
}

static const char *FindXMPPacketStart(const char *xmp_profile)
{
  const char
    *p;

  for (p=xmp_profile; *p != '\0'; p++)
    if ((*p == '<') && (*(p+1) == 'x'))
      break;
  return(p);
}

static void GetXMPProperty(const Image *image,const char *property)
{
  char
    *xmp_profile;

  const StringInfo
    *profile;

  ExceptionInfo
    *exception;

  const char
    *p;

  XMLTreeInfo
    *rdf,
    *xmp;

  profile=GetImageProfile(image,"xmp");
  if (profile == (StringInfo *) NULL)
    return;
  if (GetStringInfoLength(profile) < 17)
    return;
  if ((property == (const char *) NULL) || (*property == '\0'))
    return;
  xmp_profile=StringInfoToString(profile);
  if (xmp_profile == (char *) NULL)
    return;
  p=FindXMPPacketStart(xmp_profile);
  exception=AcquireExceptionInfo();
  xmp=NewXMLTree((char *) p,exception);
  xmp_profile=DestroyString(xmp_profile);
  exception=DestroyExceptionInfo(exception);
  if (xmp == (XMLTreeInfo *) NULL)
    return;
  rdf=GetXMLTreeChild(xmp,"rdf:RDF");
  if (rdf != (XMLTreeInfo *) NULL)
    AddXMPRDFValues(image,rdf);
  xmp=DestroyXMLTree(xmp);
}

#endif
