/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                             X   X  M   M  L                                 %
%                              X X   MM MM  L                                 %
%                               X    M M M  L                                 %
%                              X X   M   M  L                                 %
%                             X   X  M   M  LLLLL                             %
%                                                                             %
%                         TTTTT  RRRR   EEEEE  EEEEE                          %
%                           T    R   R  E      E                              %
%                           T    RRRR   EEE    EEE                            %
%                           T    R R    E      E                              %
%                           T    R  R   EEEEE  EEEEE                          %
%                                                                             %
%                                                                             %
%                              XML Tree Methods                               %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                               December 2004                                 %
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
%  This module implements the standard handy xml-tree methods for storing and
%  retrieving nodes and attributes from an XML string.
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token-private.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"

/*
  Define declarations.
*/
#define NumberPredefinedEntities  10
#define XMLWhitespace "\t\r\n "

/*
  Typedef declarations.
*/
struct _XMLTreeInfo
{
  char
    *tag,
    **attributes,
    *content;

  size_t
    offset;

  XMLTreeInfo
    *parent,
    *next,
    *sibling,
    *ordered,
    *child;

  MagickBooleanType
    debug;

  SemaphoreInfo
    *semaphore;

  size_t
    signature;
};

typedef struct _XMLTreeRoot
  XMLTreeRoot;

struct _XMLTreeRoot
{
  struct _XMLTreeInfo
    root;

  XMLTreeInfo
    *node;

  MagickBooleanType
    standalone;

  char
    ***processing_instructions,
    **entities,
    ***attributes;

  MagickBooleanType
    debug;

  SemaphoreInfo
    *semaphore;

  size_t
    signature;
};

/*
  Global declarations.
*/
static char
  *sentinel[] = { (char *) NULL };

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   A d d C h i l d T o X M L T r e e                                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AddChildToXMLTree() adds a child tag at an offset relative to the start of
%  the parent tag's character content.  Return the child tag.
%
%  The format of the AddChildToXMLTree method is:
%
%      XMLTreeInfo *AddChildToXMLTree(XMLTreeInfo *xml_info,const char *tag,
%        const size_t offset)
%
%  A description of each parameter follows:
%
%    o xml_info: the xml info.
%
%    o tag: the tag.
%
%    o offset: the tag offset.
%
*/

static void TraceXMLTree(XMLTreeInfo *xml_info)
{
  magick_unreferenced(xml_info);
  assert(xml_info != (XMLTreeInfo *) NULL);
  assert((xml_info->signature == MagickCoreSignature) ||
         (((XMLTreeRoot *) xml_info)->signature == MagickCoreSignature));
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
}

static void InsertIntoTagList(XMLTreeInfo *node,XMLTreeInfo *child,
  const size_t offset)
{
  while ((node->next != (XMLTreeInfo *) NULL) &&
         (node->next->offset <= offset))
    node=node->next;
  child->next=node->next;
  node->next=child;
}

static void InsertIntoOrderedList(XMLTreeInfo *xml_info,XMLTreeInfo *head,
  XMLTreeInfo *child,const size_t offset)
{
  XMLTreeInfo
    *node;

  if (head->offset > offset)
    {
      child->ordered=head;
      xml_info->child=child;
    }
  else
    {
      node=head;
      while ((node->ordered != (XMLTreeInfo *) NULL) &&
             (node->ordered->offset <= offset))
        node=node->ordered;
      child->ordered=node->ordered;
      node->ordered=child;
    }
}

typedef struct _SiblingListInfo
{
  XMLTreeInfo
    *head;

  size_t
    offset;
} SiblingListInfo;

static void InsertAsNewSibling(XMLTreeInfo *child,XMLTreeInfo *node,
  XMLTreeInfo *previous,const SiblingListInfo *siblings)
{
  XMLTreeInfo
    *const head = siblings->head;

  const size_t
    offset = siblings->offset;

  if ((previous != (XMLTreeInfo *) NULL) && (node != (XMLTreeInfo *) NULL))
    previous->sibling=node->sibling;
  child->next=node;
  previous=(XMLTreeInfo *) NULL;
  node=head;
  while ((node != (XMLTreeInfo *) NULL) && (node->offset <= offset))
  {
    previous=node;
    node=node->sibling;
  }
  child->sibling=node;
  if (previous != (XMLTreeInfo *) NULL)
    previous->sibling=child;
}

static void InsertIntoSiblingList(XMLTreeInfo *child,XMLTreeInfo *node,
  XMLTreeInfo *previous,const SiblingListInfo *siblings)
{
  const size_t
    offset = siblings->offset;

  if ((node != (XMLTreeInfo *) NULL) && (node->offset <= offset))
    {
      InsertIntoTagList(node,child,offset);
    }
  else
    InsertAsNewSibling(child,node,previous,siblings);
}

static XMLTreeInfo *InsertTagIntoXMLTree(XMLTreeInfo *xml_info,
  XMLTreeInfo *child,const size_t offset)
{
  SiblingListInfo
    siblings;

  XMLTreeInfo
    *head,
    *node,
    *previous;

  child->ordered=(XMLTreeInfo *) NULL;
  child->sibling=(XMLTreeInfo *) NULL;
  child->next=(XMLTreeInfo *) NULL;
  child->offset=offset;
  child->parent=xml_info;
  if (xml_info->child == (XMLTreeInfo *) NULL)
    {
      xml_info->child=child;
      return(child);
    }
  head=xml_info->child;
  InsertIntoOrderedList(xml_info,head,child,offset);
  previous=(XMLTreeInfo *) NULL;
  node=head;
  while ((node != (XMLTreeInfo *) NULL) && (strcmp(node->tag,child->tag) != 0))
  {
    previous=node;
    node=node->sibling;
  }
  siblings.head=head;
  siblings.offset=offset;
  InsertIntoSiblingList(child,node,previous,&siblings);
  return(child);
}

MagickExport XMLTreeInfo *AddChildToXMLTree(XMLTreeInfo *xml_info,
  const char *tag,const size_t offset)
{
  XMLTreeInfo
    *child;

  if (xml_info == (XMLTreeInfo *) NULL)
    return((XMLTreeInfo *) NULL);
  child=(XMLTreeInfo *) AcquireMagickMemory(sizeof(*child));
  if (child == (XMLTreeInfo *) NULL)
    return((XMLTreeInfo *) NULL);
  (void) memset(child,0,sizeof(*child));
  child->tag=ConstantString(tag);
  child->attributes=sentinel;
  child->content=ConstantString("");
  child->debug=IsEventLogging();
  child->signature=MagickCoreSignature;
  return(InsertTagIntoXMLTree(xml_info,child,offset));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   A d d P a t h T o X M L T r e e                                           %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AddPathToXMLTree() adds a child tag at an offset relative to the start of
%  the parent tag's character content.  This method returns the child tag.
%
%  The format of the AddPathToXMLTree method is:
%
%      XMLTreeInfo *AddPathToXMLTree(XMLTreeInfo *xml_info,const char *path,
%        const size_t offset)
%
%  A description of each parameter follows:
%
%    o xml_info: the xml info.
%
%    o path: the path.
%
%    o offset: the tag offset.
%
*/

static inline MagickBooleanType IsDisallowedControlChar(const unsigned char *p)
{
  return(((*p < 0x20) && (*p != 0x09) && (*p != 0x0a) &&
    (*p != 0x0d)) ? MagickTrue : MagickFalse);
}

static char *EncodeBase64XMLContent(unsigned char *utf8)
{
  char
    *base64,
    *canonical_content;

  size_t
    length;

  /*
    String is binary, base64-encode it.
  */
  base64=Base64Encode(utf8,strlen((char *) utf8),&length);
  utf8=(unsigned char *) RelinquishMagickMemory(utf8);
  if (base64 == (char *) NULL)
    return((char *) NULL);
  canonical_content=AcquireString("<base64>");
  (void) ConcatenateString(&canonical_content,base64);
  base64=DestroyString(base64);
  (void) ConcatenateString(&canonical_content,"</base64>");
  return(canonical_content);
}

static char *CanonicalXMLContent(const char *content,
  const MagickBooleanType pedantic)
{
  char
    *canonical_content;

  const unsigned char
    *p;

  unsigned char
    *utf8;

  utf8=ConvertLatin1ToUTF8((const unsigned char *) content);
  if (utf8 == (unsigned char *) NULL)
    return((char *) NULL);
  for (p=utf8; *p != '\0'; p++)
    if (IsDisallowedControlChar(p) != MagickFalse)
      break;
  if (*p != '\0')
    return(EncodeBase64XMLContent(utf8));
  canonical_content=SubstituteXMLEntities((const char *) utf8,pedantic);
  utf8=(unsigned char *) RelinquishMagickMemory(utf8);
  return(canonical_content);
}

static XMLTreeInfo *GetOrderedXMLNode(XMLTreeInfo *node,const char *subnode)
{
  ssize_t
    j;

  for (j=(ssize_t) StringToLong(subnode)-1; j > 0; j--)
  {
    node=node->ordered;
    if (node == (XMLTreeInfo *) NULL)
      break;
  }
  return(node);
}

static XMLTreeInfo *GetOrAddXMLTreeChild(XMLTreeInfo *node,const char *tag,
  const size_t offset)
{
  XMLTreeInfo
    *child;

  child=GetXMLTreeChild(node,tag);
  if (child == (XMLTreeInfo *) NULL)
    child=AddChildToXMLTree(node,tag,offset);
  return(child);
}

MagickPrivate XMLTreeInfo *AddPathToXMLTree(XMLTreeInfo *xml_info,
  const char *path,const size_t offset)
{
  char
    **components,
    subnode[MagickPathExtent],
    tag[MagickPathExtent];

  size_t
    number_components;

  ssize_t
    i;

  XMLTreeInfo
    *child,
    *node;

  TraceXMLTree(xml_info);
  node=xml_info;
  components=GetPathComponents(path,&number_components);
  if (components == (char **) NULL)
    return((XMLTreeInfo *) NULL);
  for (i=0; i < (ssize_t) number_components; i++)
  {
    GetPathComponent(components[i],SubimagePath,subnode);
    GetPathComponent(components[i],CanonicalPath,tag);
    child=GetOrAddXMLTreeChild(node,tag,offset);
    node=child;
    if (node == (XMLTreeInfo *) NULL)
      break;
    node=GetOrderedXMLNode(node,subnode);
    if (node == (XMLTreeInfo *) NULL)
      break;
    components[i]=DestroyString(components[i]);
  }
  for ( ; i < (ssize_t) number_components; i++)
    components[i]=DestroyString(components[i]);
  components=(char **) RelinquishMagickMemory(components);
  return(node);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   D e s t r o y X M L T r e e                                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyXMLTree() destroys the xml-tree.
%
%  The format of the DestroyXMLTree method is:
%
%      XMLTreeInfo *DestroyXMLTree(XMLTreeInfo *xml_info)
%
%  A description of each parameter follows:
%
%    o xml_info: the xml info.
%
*/

static XMLTreeInfo
  *DestroyXMLTree_(XMLTreeInfo *,const size_t);

static char **DestroyXMLTreeAttributes(char **attributes)
{
  ssize_t
    i;

  /*
    Destroy a tag attribute list.
  */
  if ((attributes == (char **) NULL) || (attributes == sentinel))
    return((char **) NULL);
  for (i=0; attributes[i] != (char *) NULL; i+=2)
  {
    /*
      Destroy attribute tag and value.
    */
    if (attributes[i] != (char *) NULL)
      attributes[i]=DestroyString(attributes[i]);
    if (attributes[i+1] != (char *) NULL)
      attributes[i+1]=DestroyString(attributes[i+1]);
  }
  attributes=(char **) RelinquishMagickMemory(attributes);
  return((char **) NULL);
}

static void DestroyXMLTreeList(XMLTreeInfo *xml_info,
  const size_t depth,const size_t member)
{
  XMLTreeInfo
    *next,
    *node;

  next=(*(XMLTreeInfo **) ((char *) xml_info+member));
  while (next != (XMLTreeInfo *) NULL)
  {
    node=next;
    next=(*(XMLTreeInfo **) ((char *) node+member));
    (*(XMLTreeInfo **) ((char *) node+member))=(XMLTreeInfo *) NULL;
    (void) DestroyXMLTree_(node,depth+1);
  }
}

static void DestroyDefaultAttributeValues(char **attributes)
{
  ssize_t
    j;

  for (j=1; attributes[j] != (char *) NULL; j+=3)
  {
    if (attributes[j] != (char *) NULL)
      attributes[j]=DestroyString(attributes[j]);
    if (attributes[j+1] != (char *) NULL)
      attributes[j+1]=DestroyString(attributes[j+1]);
    if (attributes[j+2] != (char *) NULL)
      attributes[j+2]=DestroyString(attributes[j+2]);
  }
}

static void DestroyXMLRootAttributes(XMLTreeRoot *root)
{
  char
    **attributes;

  ssize_t
    i;

  for (i=0; root->attributes[i] != (char **) NULL; i++)
  {
    attributes=root->attributes[i];
    if (attributes[0] != (char *) NULL)
      attributes[0]=DestroyString(attributes[0]);
    DestroyDefaultAttributeValues(attributes);
    attributes=(char **) RelinquishMagickMemory(attributes);
  }
}

static void DestroyXMLRootInstructions(XMLTreeRoot *root)
{
  ssize_t
    i,
    j;

  for (i=0; root->processing_instructions[i] != (char **) NULL; i++)
  {
    for (j=0; root->processing_instructions[i][j] != (char *) NULL; j++)
      root->processing_instructions[i][j]=DestroyString(
        root->processing_instructions[i][j]);
    root->processing_instructions[i][j+1]=DestroyString(
      root->processing_instructions[i][j+1]);
    root->processing_instructions[i]=(char **) RelinquishMagickMemory(
      root->processing_instructions[i]);
  }
  root->processing_instructions=(char ***) RelinquishMagickMemory(
    root->processing_instructions);
}

static void DestroyXMLTreeRoot(XMLTreeInfo *xml_info)
{
  ssize_t
    i;

  XMLTreeRoot
    *root;

  TraceXMLTree(xml_info);
  if (xml_info->parent != (XMLTreeInfo *) NULL)
    return;
  /*
    Free root tag allocations.
  */
  root=(XMLTreeRoot *) xml_info;
  for (i=NumberPredefinedEntities; root->entities[i] != (char *) NULL; i+=2)
    root->entities[i+1]=DestroyString(root->entities[i+1]);
  root->entities=(char **) RelinquishMagickMemory(root->entities);
  DestroyXMLRootAttributes(root);
  if (root->attributes[0] != (char **) NULL)
    root->attributes=(char ***) RelinquishMagickMemory(root->attributes);
  if (root->processing_instructions[0] != (char **) NULL)
    DestroyXMLRootInstructions(root);
}

static XMLTreeInfo *DestroyXMLTree_(XMLTreeInfo *xml_info,
  const size_t depth)
{
  TraceXMLTree(xml_info);
  if (depth >= MagickMaxRecursionDepth)
    return((XMLTreeInfo *) NULL);
  DestroyXMLTreeList(xml_info,depth+1,offsetof(XMLTreeInfo,child));
  DestroyXMLTreeList(xml_info,depth+1,offsetof(XMLTreeInfo,ordered));
  DestroyXMLTreeRoot(xml_info);
  xml_info->attributes=DestroyXMLTreeAttributes(xml_info->attributes);
  xml_info->content=DestroyString(xml_info->content);
  xml_info->tag=DestroyString(xml_info->tag);
  xml_info=(XMLTreeInfo *) RelinquishMagickMemory(xml_info);
  return((XMLTreeInfo *) NULL);
}

MagickExport XMLTreeInfo *DestroyXMLTree(XMLTreeInfo *xml_info)
{
  return(DestroyXMLTree_(xml_info,0));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t N e x t X M L T r e e T a g                                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNextXMLTreeTag() returns the next tag or NULL if not found.
%
%  The format of the GetNextXMLTreeTag method is:
%
%      XMLTreeInfo *GetNextXMLTreeTag(XMLTreeInfo *xml_info)
%
%  A description of each parameter follows:
%
%    o xml_info: the xml info.
%
*/
MagickExport XMLTreeInfo *GetNextXMLTreeTag(XMLTreeInfo *xml_info)
{
  TraceXMLTree(xml_info);
  return(xml_info->next);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t X M L T r e e A t t r i b u t e                                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetXMLTreeAttribute() returns the value of the attribute tag with the
%  specified tag if found, otherwise NULL.
%
%  The format of the GetXMLTreeAttribute method is:
%
%      const char *GetXMLTreeAttribute(XMLTreeInfo *xml_info,const char *tag)
%
%  A description of each parameter follows:
%
%    o xml_info: the xml info.
%
%    o tag: the attribute tag.
%
*/
static XMLTreeRoot *GetXMLTreeRoot(XMLTreeInfo *xml_info)
{
  XMLTreeRoot
    *root;

  root=(XMLTreeRoot *) xml_info;
  while (root->root.parent != (XMLTreeInfo *) NULL)
    root=(XMLTreeRoot *) root->root.parent;
  return(root);
}

static ssize_t FindAttributeTagIndex(char ***attributes,
  const XMLTreeInfo *xml_info)
{
  ssize_t
    i;

  i=0;
  while ((attributes[i] != (char **) NULL) &&
         (strcmp(attributes[i][0],xml_info->tag) != 0))
    i++;
  return(i);
}

static const char *GetRootXMLAttribute(XMLTreeInfo *xml_info,const char *tag)
{
  ssize_t
    i,
    j;

  XMLTreeRoot
    *root;

  root=GetXMLTreeRoot(xml_info);
  i=FindAttributeTagIndex(root->attributes,xml_info);
  if (root->attributes[i] == (char **) NULL)
    return((const char *) NULL);
  j=1;
  while ((root->attributes[i][j] != (char *) NULL) &&
         (strcmp(root->attributes[i][j],tag) != 0))
    j+=3;
  if (root->attributes[i][j] == (char *) NULL)
    return((const char *) NULL);
  return(root->attributes[i][j+1]);
}

MagickExport const char *GetXMLTreeAttribute(XMLTreeInfo *xml_info,
  const char *tag)
{
  ssize_t
    i;

  TraceXMLTree(xml_info);
  if (xml_info->attributes == (char **) NULL)
    return((const char *) NULL);
  i=0;
  while ((xml_info->attributes[i] != (char *) NULL) &&
         (strcmp(xml_info->attributes[i],tag) != 0))
    i+=2;
  if (xml_info->attributes[i] != (char *) NULL)
    return(xml_info->attributes[i+1]);
  return(GetRootXMLAttribute(xml_info,tag));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t X M L T r e e C h i l d                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetXMLTreeChild() returns the first child tag with the specified tag if
%  found, otherwise NULL.
%
%  The format of the GetXMLTreeChild method is:
%
%      XMLTreeInfo *GetXMLTreeChild(XMLTreeInfo *xml_info,const char *tag)
%
%  A description of each parameter follows:
%
%    o xml_info: the xml info.
%
*/
MagickExport XMLTreeInfo *GetXMLTreeChild(XMLTreeInfo *xml_info,const char *tag)
{
  XMLTreeInfo
    *child;

  TraceXMLTree(xml_info);
  child=xml_info->child;
  if (tag != (const char *) NULL)
    while ((child != (XMLTreeInfo *) NULL) && (strcmp(child->tag,tag) != 0))
      child=child->sibling;
  return(child);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t X M L T r e e C o n t e n t                                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetXMLTreeContent() returns any content associated with specified
%  xml-tree node.
%
%  The format of the GetXMLTreeContent method is:
%
%      const char *GetXMLTreeContent(XMLTreeInfo *xml_info)
%
%  A description of each parameter follows:
%
%    o xml_info: the xml info.
%
*/
MagickExport const char *GetXMLTreeContent(XMLTreeInfo *xml_info)
{
  TraceXMLTree(xml_info);
  return(xml_info->content);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t X M L T r e e S i b l i n g                                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetXMLTreeSibling() returns the node sibling if found, otherwise NULL.
%
%  The format of the GetXMLTreeSibling method is:
%
%      XMLTreeInfo *GetXMLTreeSibling(XMLTreeInfo *xml_info)
%
%  A description of each parameter follows:
%
%    o xml_info: the xml info.
%
*/
MagickExport XMLTreeInfo *GetXMLTreeSibling(XMLTreeInfo *xml_info)
{
  TraceXMLTree(xml_info);
  return(xml_info->sibling);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t X M L T r e e T a g                                                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetXMLTreeTag() returns the tag associated with specified xml-tree node.
%
%  The format of the GetXMLTreeTag method is:
%
%      const char *GetXMLTreeTag(XMLTreeInfo *xml_info)
%
%  A description of each parameter follows:
%
%    o xml_info: the xml info.
%
*/
MagickExport const char *GetXMLTreeTag(XMLTreeInfo *xml_info)
{
  TraceXMLTree(xml_info);
  return(xml_info->tag);
}

#include "MagickCore/xml-tree-text-private.h"

#include "MagickCore/xml-tree-declarations-private.h"

#include "MagickCore/xml-tree-markup-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   N e w X M L T r e e                                                       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NewXMLTree() returns a XMLTreeInfo xml-tree as defined by the specified
%  XML string.
%
%  The format of the NewXMLTree method is:
%
%      XMLTreeInfo *NewXMLTree(const char *xml,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o xml:  A null-terminated XML string.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport XMLTreeInfo *NewXMLTree(const char *xml,ExceptionInfo *exception)
{
  char
    *p,
    *utf8;

  int
    terminal;

  size_t
    length;

  XMLTreeRoot
    *root;

  XMLTreeUTF8Targets
    utf8_targets;

  /*
    Convert xml-string to UTF8.
  */
  if ((xml == (const char *) NULL) || (strlen(xml) == 0))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "ParseError","root tag missing");
      return((XMLTreeInfo *) NULL);
    }
  root=(XMLTreeRoot *) NewXMLTreeTag((char *) NULL);
  utf8_targets.utf8=(&utf8);
  utf8_targets.length=(&length);
  if (ConvertXMLTreeToUTF8(xml,exception,&utf8_targets) == MagickFalse)
    return((XMLTreeInfo *) NULL);
  terminal=utf8[length-1];
  utf8[length-1]='\0';
  p=utf8;
  if (SkipToRootTag(&p,exception) == MagickFalse)
    {
      utf8=DestroyString(utf8);
      return((XMLTreeInfo *) NULL);
    }
  if (ParseXMLTreeTags(root,p,terminal,exception) == MagickFalse)
    {
      utf8=DestroyString(utf8);
      return(&root->root);
    }
  utf8=DestroyString(utf8);
  return(CheckXMLTreeClosed(root,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   N e w X M L T r e e T a g                                                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NewXMLTreeTag() returns a new empty xml structure for the xml-tree tag.
%
%  The format of the NewXMLTreeTag method is:
%
%      XMLTreeInfo *NewXMLTreeTag(const char *tag)
%
%  A description of each parameter follows:
%
%    o tag: the tag.
%
*/
MagickExport XMLTreeInfo *NewXMLTreeTag(const char *tag)
{
  static const char
    *predefined_entities[NumberPredefinedEntities+1] =
    {
      "lt;", "&#60;", "gt;", "&#62;", "quot;", "&#34;",
      "apos;", "&#39;", "amp;", "&#38;", (char *) NULL
    };

  XMLTreeRoot
    *root;

  root=(XMLTreeRoot *) AcquireCriticalMemory(sizeof(*root));
  (void) memset(root,0,sizeof(*root));
  root->root.tag=(char *) NULL;
  if (tag != (char *) NULL)
    root->root.tag=ConstantString(tag);
  root->node=(&root->root);
  root->root.content=ConstantString("");
  root->entities=(char **) AcquireCriticalMemory(sizeof(predefined_entities));
  (void) memcpy(root->entities,predefined_entities,sizeof(predefined_entities));
  root->root.attributes=sentinel;
  root->attributes=(char ***) root->root.attributes;
  root->processing_instructions=(char ***) root->root.attributes;
  root->debug=IsEventLogging();
  root->signature=MagickCoreSignature;
  return(&root->root);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   S e t X M L T r e e C o n t e n t                                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetXMLTreeContent() sets the character content for the given tag and
%  returns the tag.
%
%  The format of the SetXMLTreeContent method is:
%
%      XMLTreeInfo *SetXMLTreeContent(XMLTreeInfo *xml_info,
%        const char *content)
%
%  A description of each parameter follows:
%
%    o xml_info: the xml info.
%
%    o content:  The content.
%
*/
MagickExport XMLTreeInfo *SetXMLTreeContent(XMLTreeInfo *xml_info,
  const char *content)
{
  TraceXMLTree(xml_info);
  if (xml_info->content != (char *) NULL)
    xml_info->content=DestroyString(xml_info->content);
  xml_info->content=(char *) ConstantString(content);
  return(xml_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   X M L T r e e I n f o T o X M L                                           %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMLTreeInfoToXML() converts an xml-tree to an XML string.
%
%  The format of the XMLTreeInfoToXML method is:
%
%      char *XMLTreeInfoToXML(XMLTreeInfo *xml_info)
%
%  A description of each parameter follows:
%
%    o xml_info: the xml info.
%
*/

static MagickBooleanType ExtendXMLBufferBy(char **source,const size_t *length,
  size_t *extent,const size_t size)
{
  if ((*length+size+MagickPathExtent) > *extent)
    {
      *extent=(*length)+size+MagickPathExtent;
      *source=(char *) ResizeQuantumMemory(*source,*extent,sizeof(**source));
      if (*source == (char *) NULL)
        return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _XMLBufferInfo
{
  char
    **destination;

  size_t
    *length,
    *extent;
} XMLBufferInfo;

static char *EncodePredefinedEntities(const char *source,ssize_t offset,
  const XMLBufferInfo *buffer,MagickBooleanType pedantic)
{
  char
    **const destination = buffer->destination;

  size_t
    *const length = buffer->length,
    *const extent = buffer->extent;

  char
    *canonical_content;

  if (offset < 0)
    canonical_content=CanonicalXMLContent(source,pedantic);
  else
    {
      char
        *content;

      content=AcquireString(source);
      content[offset]='\0';
      canonical_content=CanonicalXMLContent(content,pedantic);
      content=DestroyString(content);
    }
  if (canonical_content == (char *) NULL)
    return(*destination);
  if (ExtendXMLBufferBy(destination,length,extent,
        strlen(canonical_content)) == MagickFalse)
    return(*destination);
  *length+=(size_t) FormatLocaleString(*destination+(*length),*extent,"%s",
    canonical_content);
  canonical_content=DestroyString(canonical_content);
  return(*destination);
}

static MagickBooleanType AppendTagAttributes(XMLTreeInfo *xml_info,
  char **source,size_t *length,size_t *extent)
{
  const char
    *attribute;

  const XMLBufferInfo
    buffer = { source, length, extent };

  ssize_t
    i;

  for (i=0; xml_info->attributes[i]; i+=2)
  {
    attribute=GetXMLTreeAttribute(xml_info,xml_info->attributes[i]);
    if (attribute != xml_info->attributes[i+1])
      continue;
    if (ExtendXMLBufferBy(source,length,extent,
          strlen(xml_info->attributes[i])) == MagickFalse)
      return(MagickFalse);
    *length+=(size_t) FormatLocaleString(*source+(*length),*extent," %s=\"",
      xml_info->attributes[i]);
    (void) EncodePredefinedEntities(xml_info->attributes[i+1],-1,&buffer,
      MagickTrue);
    *length+=(size_t) FormatLocaleString(*source+(*length),*extent,"\"");
  }
  return(MagickTrue);
}

typedef struct _TagBufferInfo
{
  XMLTreeInfo
    *xml_info;

  char
    **source;

  size_t
    *length,
    *extent;

  char
    ***attributes;
} TagBufferInfo;

static MagickBooleanType AppendDefaultTagAttributes(
  const TagBufferInfo *tag_buffer)
{
  XMLTreeInfo
    *const xml_info = tag_buffer->xml_info;

  char
    **const source = tag_buffer->source;

  size_t
    *const length = tag_buffer->length,
    *const extent = tag_buffer->extent;

  char
    ***const attributes = tag_buffer->attributes;

  const XMLBufferInfo
    buffer = { source, length, extent };

  ssize_t
    i,
    j;

  i=FindAttributeTagIndex(attributes,xml_info);
  j=1;
  while ((attributes[i] != (char **) NULL) &&
         (attributes[i][j] != (char *) NULL))
  {
    if ((attributes[i][j+1] == (char *) NULL) ||
        (GetXMLTreeAttribute(xml_info,attributes[i][j]) != attributes[i][j+1]))
      {
        j+=3;
        continue;
      }
    if (ExtendXMLBufferBy(source,length,extent,strlen(attributes[i][j])) ==
        MagickFalse)
      return(MagickFalse);
    *length+=(size_t) FormatLocaleString(*source+(*length),*extent," %s=\"",
      attributes[i][j]);
    (void) EncodePredefinedEntities(attributes[i][j+1],-1,&buffer,MagickTrue);
    *length+=(size_t) FormatLocaleString(*source+(*length),*extent,"\"");
    j+=3;
  }
  return(MagickTrue);
}

static MagickBooleanType AppendAllTagAttributes(XMLTreeInfo *xml_info,
  const XMLBufferInfo *buffer,char ***attributes)
{
  char
    **const source = buffer->destination;

  size_t
    *const length = buffer->length,
    *const extent = buffer->extent;

  TagBufferInfo
    tag_buffer;

  if (AppendTagAttributes(xml_info,source,length,extent) == MagickFalse)
    return(MagickFalse);
  tag_buffer.xml_info=xml_info;
  tag_buffer.source=source;
  tag_buffer.length=length;
  tag_buffer.extent=extent;
  tag_buffer.attributes=attributes;
  if (AppendDefaultTagAttributes(&tag_buffer) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static size_t SkipTagContentOffset(const XMLTreeInfo *xml_info,
  const char *content,size_t offset)
{
  while ((offset < xml_info->offset) && (content[offset] != '\0'))
    offset++;
  return(offset);
}

static MagickBooleanType AppendCloseTag(const XMLTreeInfo *xml_info,
  const XMLBufferInfo *buffer)
{
  char
    **const source = buffer->destination;

  size_t
    *const length = buffer->length,
    *const extent = buffer->extent;

  if (ExtendXMLBufferBy(source,length,extent,strlen(xml_info->tag)) ==
      MagickFalse)
    return(MagickFalse);
  if (*xml_info->content != '\0')
    *length+=(size_t) FormatLocaleString(*source+(*length),*extent,"</%s>",
      xml_info->tag);
  return(MagickTrue);
}

static char *XMLTreeTagToXML(XMLTreeInfo *xml_info,const XMLBufferInfo *buffer,
  size_t start,char ***attributes)
{
  char
    **const source = buffer->destination;

  size_t
    *const length = buffer->length,
    *const extent = buffer->extent;

  char
    *content;

  size_t
    offset;

  content=(char *) "";
  if (xml_info->parent != (XMLTreeInfo *) NULL)
    content=xml_info->parent->content;
  offset=0;
  *source=EncodePredefinedEntities(content+start,(ssize_t) (xml_info->offset-
    start),buffer,MagickFalse);
  if (ExtendXMLBufferBy(source,length,extent,strlen(xml_info->tag)) ==
      MagickFalse)
    return(*source);
  *length+=(size_t) FormatLocaleString(*source+(*length),*extent,
    "<%s",xml_info->tag);
  if (AppendAllTagAttributes(xml_info,buffer,attributes) == MagickFalse)
    return((char *) NULL);
  *length+=(size_t) FormatLocaleString(*source+(*length),*extent,
    *xml_info->content ? ">" : "/>");
  if (xml_info->child != (XMLTreeInfo *) NULL)
    *source=XMLTreeTagToXML(xml_info->child,buffer,0,attributes);
  else
    *source=EncodePredefinedEntities(xml_info->content,-1,buffer,MagickFalse);
  if (AppendCloseTag(xml_info,buffer) == MagickFalse)
    return((char *) NULL);
  offset=SkipTagContentOffset(xml_info,content,offset);
  if (xml_info->ordered != (XMLTreeInfo *) NULL)
    content=XMLTreeTagToXML(xml_info->ordered,buffer,offset,attributes);
  else
    content=EncodePredefinedEntities(content+offset,-1,buffer,MagickFalse);
  return(content);
}

static void AppendRootInstruction(const TagBufferInfo *tag_buffer,
  const char skip,const char *q,const char *p)
{
  char
    **const source = tag_buffer->source;

  size_t
    *const extent = tag_buffer->extent,
    *const length = tag_buffer->length;

  if (skip == '>')
    *length+=(size_t) FormatLocaleString(*source+(*length),*extent,
      "<?%s%s%s?>\n",q,*p != '\0' ? " " : "",p);
  else
    *length+=(size_t) FormatLocaleString(*source+(*length),*extent,
      "\n<?%s%s%s?>",q,*p != '\0' ? " " : "",p);
}

static ssize_t CountInstructionFields(const XMLTreeRoot *root,
  const ssize_t i)
{
  ssize_t
    k;

  for (k=2; root->processing_instructions[i][k-1]; k++) ;
  return(k);
}

static MagickBooleanType AppendRootInstructions(const XMLTreeRoot *root,
  const char skip,const TagBufferInfo *tag_buffer)
{
  char
    **const source = tag_buffer->source,
    *p,
    *q;

  size_t
    *const extent = tag_buffer->extent,
    *const length = tag_buffer->length;

  ssize_t
    i,
    j,
    k;

  for (i=0; root->processing_instructions[i] != (char **) NULL; i++)
  {
    /*
      Pre-root or post-root processing instructions.
    */
    k=CountInstructionFields(root,i);
    p=root->processing_instructions[i][1];
    for (j=1; p != (char *) NULL; j++)
    {
      if (root->processing_instructions[i][k][j-1] == skip)
        {
          p=root->processing_instructions[i][j];
          continue;
        }
      q=root->processing_instructions[i][0];
      if (ExtendXMLBufferBy(source,length,extent,strlen(p)+strlen(q)) ==
          MagickFalse)
        return(MagickFalse);
      AppendRootInstruction(tag_buffer,skip,q,p);
      p=root->processing_instructions[i][j];
    }
  }
  return(MagickTrue);
}

MagickExport char *XMLTreeInfoToXML(XMLTreeInfo *xml_info)
{
  char
    *xml;

  size_t
    extent,
    length;

  TagBufferInfo
    tag_buffer;

  const XMLBufferInfo
    buffer = { &xml, &length, &extent };

  XMLTreeInfo
    *ordered,
    *parent;

  XMLTreeRoot
    *root;

  TraceXMLTree(xml_info);
  if (xml_info->tag == (char *) NULL)
    return((char *) NULL);
  xml=AcquireString((char *) NULL);
  length=0;
  extent=MagickPathExtent;
  root=GetXMLTreeRoot(xml_info);
  parent=xml_info->parent;
  tag_buffer.xml_info=xml_info;
  tag_buffer.source=(&xml);
  tag_buffer.length=(&length);
  tag_buffer.extent=(&extent);
  tag_buffer.attributes=root->attributes;
  if ((parent == (XMLTreeInfo *) NULL) &&
      (AppendRootInstructions(root,'>',&tag_buffer) == MagickFalse))
    return((char *) NULL);
  ordered=xml_info->ordered;
  xml_info->parent=(XMLTreeInfo *) NULL;
  xml_info->ordered=(XMLTreeInfo *) NULL;
  xml=XMLTreeTagToXML(xml_info,&buffer,0,root->attributes);
  xml_info->parent=parent;
  xml_info->ordered=ordered;
  if ((parent == (XMLTreeInfo *) NULL) &&
      (AppendRootInstructions(root,'<',&tag_buffer) == MagickFalse))
    return((char *) NULL);
  return((char *) ResizeQuantumMemory(xml,length+1,sizeof(*xml)));
}
