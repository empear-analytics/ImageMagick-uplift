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

  MagickCore private methods of xml-tree.c: the markup of the XML parser,
  from ParseDoctypeDeclaration to ParseXMLTreeTags (doctype, tags and
  attributes, comments, CDATA, the start-tag and content loop); included by
  that file only.
*/
#ifndef MAGICKCORE_XML_TREE_MARKUP_PRIVATE_H
#define MAGICKCORE_XML_TREE_MARKUP_PRIVATE_H

static MagickBooleanType ParseDoctypeDeclaration(
  const InternalDoctypeInfo *doctype,const AttributeListTargets *targets,
  char ***predefined_entities,MagickBooleanType *broke)
{
  XMLTreeRoot
    *const root = doctype->root;

  ExceptionInfo
    *const exception = doctype->exception;

  char
    **const xml = targets->xml,
    **const n = targets->n;

  char
    *c,
    q,
    *v;

  EntityHeadTargets
    entity_targets;

  (*broke)=MagickFalse;
  if ((strlen((*xml)) > 9) && (strncmp((*xml),"<!ENTITY",8) == 0))
    {
      /*
        Parse entity definitions.
      */
      entity_targets.xml=xml;
      entity_targets.c=(&c);
      entity_targets.n=n;
      entity_targets.v=(&v);
      entity_targets.q=(&q);
      if (ParseEntityDeclarationHead(&entity_targets) == MagickFalse)
        {
          (*broke)=MagickTrue;
          return(MagickTrue);
        }
      {
        const EntityValueInput
          entity = { root, c, (*n), v, q, exception };

        if (AddInternalEntityDeclaration(&entity,xml,
              predefined_entities) == MagickFalse)
          return(MagickFalse);
      }
      }
    else
      return(ParseOtherDoctypeDeclaration(root,exception,targets,broke));
  return(MagickTrue);
}

static MagickBooleanType ParseInternalDoctypeDeclarations(
  const InternalDoctypeInfo *doctype,char *xml,char *n,
  char ***predefined_entities)
{
  const size_t
    length = doctype->length;

  const AttributeListTargets
    markup_targets = { &xml, &n };

  MagickBooleanType
    broke = MagickFalse;

  for (xml[length]='\0'; xml != (char *) NULL; )
  {
    while (IsDoctypeTextChar(xml) != MagickFalse)
      xml++;
    if (*xml == '\0')
      break;
    if (ParseDoctypeDeclaration(doctype,&markup_targets,predefined_entities,
          &broke) == MagickFalse)
      return(MagickFalse);
    if (broke != MagickFalse)
      break;
    }
  return(MagickTrue);
}

static MagickBooleanType ParseInternalDoctype(XMLTreeRoot *root,char *xml,
  size_t length,ExceptionInfo *exception)
{
  char
    *n,
    **predefined_entities;

  n=(char *) NULL;
  predefined_entities=(char **) AcquireMagickMemory(sizeof(sentinel));
  if (predefined_entities == (char **) NULL)
    ThrowFatalException(ResourceLimitError,"MemoryAllocationFailed");
  (void) memcpy(predefined_entities,sentinel,sizeof(sentinel));
  {
    const InternalDoctypeInfo
      doctype = { root, length, exception };

    if (ParseInternalDoctypeDeclarations(&doctype,xml,n,
          &predefined_entities) == MagickFalse)
      {
        predefined_entities=(char **) RelinquishMagickMemory(
          predefined_entities);
        return(MagickFalse);
      }
  }
  predefined_entities=(char **) RelinquishMagickMemory(predefined_entities);
  return(MagickTrue);
}

static void ParseOpenTag(XMLTreeRoot *root,char *tag,char **attributes)
{
  XMLTreeInfo
    *xml_info;

  xml_info=root->node;
  if (xml_info->tag == (char *) NULL)
    xml_info->tag=ConstantString(tag);
  else
    xml_info=AddChildToXMLTree(xml_info,tag,strlen(xml_info->content));
  if (xml_info != (XMLTreeInfo *) NULL)
    xml_info->attributes=attributes;
  root->node=xml_info;
}

static const char
  *ignore_tags[3] =
  {
    "rdf:Bag",
    "rdf:Seq",
    (const char *) NULL
  };

static inline MagickBooleanType IsSkipTag(const char *tag)
{
  ssize_t
    i;

  i=0;
  while (ignore_tags[i] != (const char *) NULL)
  {
    if (LocaleCompare(tag,ignore_tags[i]) == 0)
      return(MagickTrue);
    i++;
  }
  return(MagickFalse);
}

static inline MagickBooleanType IsXMLNameStart(const char *p,const int c)
{
  return(((isalpha((int) ((unsigned char) *p)) != 0) || (*p == '_') ||
    (*p == ':') || (c < '\0')) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsNameStartOutsideIgnore(const char *p,
  const size_t ignore_depth)
{
  return((((isalpha((int) ((unsigned char) *p)) != 0) || (*p == '_')) &&
    (ignore_depth == 0)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsOtherDefaultAttribute(char **attribute,
  const ssize_t j,char **attributes,const ssize_t l)
{
  return(((attribute != (char **) NULL) &&
    (attribute[j] != (char *) NULL) &&
    (strcmp(attribute[j],attributes[l]) != 0)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsTagNotClosed(const char *p,const int terminal)
{
  return((((*p != '\0') && (*p != '>')) ||
    ((*p == '\0') && (terminal != '>'))) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsTagClosed(const char *p,const int terminal)
{
  return(((*p == '>') || ((*p == '\0') && (terminal == '>'))) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsInsideXMLTag(const char *p)
{
  return(((*p != '\0') && (*p != '/') && (*p != '>')) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsUnterminatedSection(const char *p,
  const int terminal)
{
  return(((p == (char *) NULL) || ((*p == '\0') &&
    (terminal != '>'))) ? MagickTrue : MagickFalse);
}

static char *SkipProcessingInstruction(char *p)
{
  do
  {
    p=strchr(p,'?');
    if (p == (char *) NULL)
      break;
    p++;
  } while ((*p != '\0') && (*p != '>'));
  return(p);
}

static char **GrowAttributeList(char **attributes,const ssize_t l)
{
  if (l == 0)
    attributes=(char **) AcquireQuantumMemory(4,
      sizeof(*attributes));
  else
    attributes=(char **) ResizeQuantumMemory(attributes,(size_t)
      (l+4),sizeof(*attributes));
  return(attributes);
}

static char **FindDefaultAttributes(XMLTreeRoot *root,const char *tag)
{
  ssize_t
    i;

  i=0;
  while ((root->attributes[i] != (char **) NULL) &&
         (strcmp(root->attributes[i][0],tag) != 0))
    i++;
  return(root->attributes[i]);
}

static void DecodeAttributeValue(XMLTreeRoot *root,char **attribute,
  char **attributes,const ssize_t l)
{
  ssize_t
    j;

  j=1;
  while (IsOtherDefaultAttribute(attribute,j,attributes,
      l) != MagickFalse)
    j+=3;
  attributes[l+1]=ParseEntities(attributes[l+1],
    root->entities,(attribute != (char **) NULL) &&
    (attribute[j] != (char *) NULL) ? *attribute[j+2] :
    ' ');
}

static size_t OpenOrIgnoreXMLTag(XMLTreeRoot *root,char *tag,
  char **attributes,size_t ignore_depth)
{
  if ((ignore_depth == 0) && (IsSkipTag(tag) == MagickFalse))
    ParseOpenTag(root,tag,attributes);
  else
    {
      ignore_depth++;
      (void) DestroyXMLTreeAttributes(attributes);
    }
  return(ignore_depth);
}

typedef struct _AttributeValueInfo
{
  XMLTreeRoot
    *root;

  char
    **attribute,
    ***attributes,
    **p;

  ssize_t
    *l;

  ExceptionInfo
    *exception;
} AttributeValueInfo;

static MagickBooleanType ParseAttributeValue(const AttributeValueInfo *value)
{
  XMLTreeRoot
    *const root = value->root;

  char
    **const attribute = value->attribute;

  char
    ***const attributes = value->attributes;

  char
    **const p = value->p;

  ssize_t
    *const l = value->l;

  ExceptionInfo
    *const exception = value->exception;

  int
    c;

  *(*p)++='\0';
  *p+=(ptrdiff_t) strspn(*p,XMLWhitespace "=");
  c=(**p);
  if ((c == '"') || (c == '\''))
    {
      /*
        Attributes value.
      */
      (*p)++;
      (*attributes)[*l+1]=*p;
      while ((**p != '\0') && (**p != c))
        (*p)++;
      if (**p != '\0')
        *(*p)++='\0';
      else
        {
          (*attributes)[*l]=ConstantString("");
          (*attributes)[*l+1]=ConstantString("");
          (void) DestroyXMLTreeAttributes(*attributes);
          (void) ThrowMagickException(exception,
            GetMagickModule(),OptionWarning,"ParseError",
            "missing %c",c);
          return(MagickFalse);
        }
      DecodeAttributeValue(root,attribute,*attributes,*l);
    }
  (*attributes)[*l]=ConstantString((*attributes)[*l]);
  return(MagickTrue);
}

typedef struct _TagAttributeTargets
{
  char
    ***attributes,
    **p;

  ssize_t
    *l;
} TagAttributeTargets;

static MagickBooleanType ParseTagAttribute(XMLTreeRoot *root,
  char **attribute,ExceptionInfo *exception,const TagAttributeTargets *targets)
{
  char
    ***const attributes = targets->attributes;

  char
    **const p = targets->p;

  ssize_t
    *const l = targets->l;

  /*
    Attribute.
  */
  *attributes=GrowAttributeList(*attributes,*l);
  if (*attributes == (char **) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return(MagickFalse);
    }
  (*attributes)[*l+2]=(char *) NULL;
  (*attributes)[*l+1]=(char *) NULL;
  (*attributes)[*l]=*p;
  *p+=(ptrdiff_t) strcspn(*p,XMLWhitespace "=/>");
  if ((**p != '=') && (isspace((int) ((unsigned char) **p)) == 0))
    (*attributes)[*l]=ConstantString("");
  else
    {
      AttributeValueInfo
        value;

      value.root=root;
      value.attribute=attribute;
      value.attributes=attributes;
      value.p=p;
      value.l=l;
      value.exception=exception;
      if (ParseAttributeValue(&value) == MagickFalse)
        return(MagickFalse);
    }
  while (isspace((int) ((unsigned char) **p)) != 0)
    (*p)++;
  return(MagickTrue);
}

static MagickBooleanType ParseTagAttributes(XMLTreeRoot *root,
  char **attribute,ExceptionInfo *exception,const TagAttributeTargets *targets)
{
  char
    **const p = targets->p;

  ssize_t
    *const l = targets->l;

  for (*l=0; IsInsideXMLTag(*p) != MagickFalse; *l+=2)
  {
    if (ParseTagAttribute(root,attribute,exception,targets) == MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

typedef struct _AttributeSectionInfo
{
  XMLTreeRoot
    *root;

  char
    *tag;

  size_t
    ignore_depth;

  ExceptionInfo
    *exception;
} AttributeSectionInfo;

typedef struct _AttributeSectionTargets
{
  char
    ***attribute,
    ***attributes,
    **p;

  ssize_t
    *l;
} AttributeSectionTargets;

static MagickBooleanType SkipStartTagAttributes(char **p)
{
  while (IsInsideXMLTag(*p) != MagickFalse)
    (*p)++;
  return(MagickTrue);
}

static MagickBooleanType ParseStartTagAttributes(
  const AttributeSectionInfo *section,
  const AttributeSectionTargets *section_targets)
{
  XMLTreeRoot
    *const root = section->root;

  char
    *const tag = section->tag;

  const size_t
    ignore_depth = section->ignore_depth;

  ExceptionInfo
    *const exception = section->exception;

  char
    ***const attribute = section_targets->attribute;

  char
    ***const attributes = section_targets->attributes;

  char
    **const p = section_targets->p;

  ssize_t
    *const l = section_targets->l;

  if (IsNameStartOutsideIgnore(*p,ignore_depth) != MagickFalse)
    {
      TagAttributeTargets
        targets;

      if (IsInsideXMLTag(*p) != MagickFalse)
        {
          /*
            Find tag in default attributes list.
          */
          *attribute=FindDefaultAttributes(root,tag);
        }
      targets.attributes=attributes;
      targets.p=p;
      targets.l=l;
      if (ParseTagAttributes(root,*attribute,exception,&targets) ==
          MagickFalse)
        {
          return(MagickFalse);
        }
    }
  else
    return(SkipStartTagAttributes(p));
  return(MagickTrue);
}

typedef struct _EndTagTargets
{
  char
    **p;

  size_t
    *ignore_depth;
} EndTagTargets;

static void EndCloseTag(char **p,size_t *ignore_depth,const int c)
{
  if (*ignore_depth > 0)
    (*ignore_depth)--;
  **p=(char) c;
  if (isspace((int) ((unsigned char) **p)) != 0)
    *p+=(ptrdiff_t) strspn(*p,XMLWhitespace);
}

static MagickBooleanType ParseEndTag(XMLTreeRoot *root,const int terminal,
  ExceptionInfo *exception,const EndTagTargets *targets)
{
  char
    **const p = targets->p;

  size_t
    *const ignore_depth = targets->ignore_depth;

  char
    *tag;

  int
    c;

  /*
    Close tag.
  */
  tag=*p+1;
  *p+=(ptrdiff_t) strcspn(tag,XMLWhitespace ">")+1;
  c=(**p);
  if ((c == '\0') && (terminal != '>'))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionWarning,"ParseError","missing >");
      return(MagickFalse);
    }
  **p='\0';
  if ((*ignore_depth == 0) &&
      (ParseCloseTag(root,tag,exception) != (XMLTreeInfo *) NULL))
    {
      return(MagickFalse);
    }
  EndCloseTag(p,ignore_depth,c);
  return(MagickTrue);
}

static void SkipDoctypeDeclaration(char **p,ssize_t *l)
{
  for (*l=0; (**p != '\0') && (((*l == 0) && (**p != '>')) ||
       ((*l != 0) && ((**p != ']') ||
       (*(*p+strspn(*p+1,XMLWhitespace)+1) != '>'))));
    *l=(ssize_t) ((**p == '[') ? 1 : *l))
  *p+=(ptrdiff_t) strcspn(*p+1,"[]>")+1;
}

typedef struct _DoctypeTargets
{
  char
    **p,
    **tag;

  ssize_t
    *l;
} DoctypeTargets;

static MagickBooleanType ParseDoctype(XMLTreeRoot *root,const int terminal,
  ExceptionInfo *exception,const DoctypeTargets *targets)
{
  char
    **const p = targets->p;

  char
    **const tag = targets->tag;

  ssize_t
    *const l = targets->l;

  MagickBooleanType
    status;

  /*
    DTD.
  */
  SkipDoctypeDeclaration(p,l);
  if ((**p == '\0') && (terminal != '>'))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionWarning,"ParseError","unclosed <!DOCTYPE");
      return(MagickFalse);
    }
  if (*l != 0)
    *tag=strchr(*tag,'[')+1;
  if (*l != 0)
    {
      status=ParseInternalDoctype(root,*tag,(size_t) (*p-*tag),
        exception);
      if (status == MagickFalse)
        {
          return(MagickFalse);
        }
      (*p)++;
    }
  return(MagickTrue);
}

static inline MagickBooleanType IsUnclosedComment(char **p,
  const int terminal)
{
  return(((*p == (char *) NULL) || ((*(*p+=2) != '>') && (**p != '\0')) ||
    ((**p == '\0') && (terminal != '>'))) ? MagickTrue : MagickFalse);
}

static MagickBooleanType SkipComment(char **p,const int terminal,
  ExceptionInfo *exception)
{
  /*
    Comment.
  */
  *p=strstr(*p+3,"--");
  if (IsUnclosedComment(p,terminal) != MagickFalse)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionWarning,"ParseError","unclosed <!--");
      return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _StartTagInfo
{
  XMLTreeRoot
    *root;

  char
    *tag,
    **attributes;

  ssize_t
    l;

  int
    terminal;

  ExceptionInfo
    *exception;
} StartTagInfo;

typedef struct _StartTagTargets
{
  char
    **p;

  size_t
    *ignore_depth;
} StartTagTargets;

static MagickBooleanType ThrowMissingTagEnd(const ssize_t l,
  char **attributes,ExceptionInfo *exception)
{
  if (l != 0)
    (void) DestroyXMLTreeAttributes(attributes);
  (void) ThrowMagickException(exception,GetMagickModule(),
    OptionWarning,"ParseError","missing >");
  return(MagickFalse);
}

static MagickBooleanType EndSelfClosingTag(const StartTagInfo *start_tag,
  const StartTagTargets *targets)
{
  XMLTreeRoot
    *const root = start_tag->root;

  char
    *const tag = start_tag->tag;

  char
    **const attributes = start_tag->attributes;

  const ssize_t
    l = start_tag->l;

  const int
    terminal = start_tag->terminal;

  ExceptionInfo
    *const exception = start_tag->exception;

  char
    **const p = targets->p;

  size_t
    *const ignore_depth = targets->ignore_depth;

  /*
    Self closing tag.
  */
  *(*p)++='\0';
  if (IsTagNotClosed(*p,terminal) != MagickFalse)
    return(ThrowMissingTagEnd(l,attributes,exception));
  if ((*ignore_depth != 0) || (IsSkipTag(tag) != MagickFalse))
    (void) DestroyXMLTreeAttributes(attributes);
  else
    {
      ParseOpenTag(root,tag,attributes);
      (void) ParseCloseTag(root,tag,exception);
    }
  return(MagickTrue);
}

static MagickBooleanType EndStartTag(const StartTagInfo *start_tag,
  const StartTagTargets *targets)
{
  XMLTreeRoot
    *const root = start_tag->root;

  char
    *const tag = start_tag->tag;

  char
    **const attributes = start_tag->attributes;

  const ssize_t
    l = start_tag->l;

  const int
    terminal = start_tag->terminal;

  ExceptionInfo
    *const exception = start_tag->exception;

  char
    **const p = targets->p;

  size_t
    *const ignore_depth = targets->ignore_depth;

  int
    c;

  if (**p == '/')
    return(EndSelfClosingTag(start_tag,targets));
  else
    {
      c=(**p);
      if (IsTagClosed(*p,terminal) != MagickFalse)
        {
          **p='\0';
          *ignore_depth=OpenOrIgnoreXMLTag(root,tag,attributes,
            *ignore_depth);
          **p=(char) c;
        }
      else
        return(ThrowMissingTagEnd(l,attributes,exception));
    }
  return(MagickTrue);
}

static MagickBooleanType ParseTagCharacterContent(XMLTreeRoot *root,
  char *tag,const size_t ignore_depth,char **p)
{
  if ((**p != '\0') && (**p != '<'))
    {
      /*
        Tag character content.
      */
      while ((**p != '\0') && (**p != '<'))
        (*p)++;
      if (**p == '\0')
        return(MagickFalse);
      if (ignore_depth == 0)
        ParseCharacterContent(root,tag,(size_t) (*p-tag),'&');
    }
  else
    if (**p == '\0')
      return(MagickFalse);
  return(MagickTrue);
}

static char *ParseCDataContent(XMLTreeRoot *root,char *tag,char *p,
  const size_t ignore_depth)
{
  p+=(ptrdiff_t) 2;
  if (ignore_depth == 0)
    ParseCharacterContent(root,tag+8,(size_t) (p-tag-10),'c');
  return(p);
}

typedef struct _DeclarationInfo
{
  XMLTreeRoot
    *root;

  size_t
    ignore_depth;

  int
    terminal;

  ExceptionInfo
    *exception;
} DeclarationInfo;

typedef struct _DeclarationTargets
{
  char
    **p,
    **tag;

  ssize_t
    *l;
} DeclarationTargets;

static MagickBooleanType ParseInstructionMarkup(
  const DeclarationInfo *declaration,
  const DeclarationTargets *declaration_targets)
{
  XMLTreeRoot
    *const root = declaration->root;

  const int
    terminal = declaration->terminal;

  ExceptionInfo
    *const exception = declaration->exception;

  char
    **const p = declaration_targets->p;

  char
    **const tag = declaration_targets->tag;

  if (**p == '?')
    {
      /*
        Processing instructions.
      */
      *p=SkipProcessingInstruction(*p);
      if (IsUnterminatedSection(*p,terminal) != MagickFalse)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),
            OptionWarning,"ParseError","unclosed <?");
          return(MagickFalse);
        }
      ParseProcessingInstructions(root,*tag+1,(size_t) (*p-*tag-2));
    }
  else
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionWarning,"ParseError","unexpected <");
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType ParseDoctypeOrInstruction(
  const DeclarationInfo *declaration,
  const DeclarationTargets *declaration_targets)
{
  XMLTreeRoot
    *const root = declaration->root;

  const int
    terminal = declaration->terminal;

  ExceptionInfo
    *const exception = declaration->exception;

  char
    **const p = declaration_targets->p;

  char
    **const tag = declaration_targets->tag;

  ssize_t
    *const l = declaration_targets->l;

  if (strncmp(*p,"!DOCTYPE",8) == 0)
    {
      DoctypeTargets
        targets;

      targets.p=p;
      targets.tag=tag;
      targets.l=l;
      if (ParseDoctype(root,terminal,exception,&targets) ==
          MagickFalse)
        {
          return(MagickFalse);
        }
    }
  else
    return(ParseInstructionMarkup(declaration,declaration_targets));
  return(MagickTrue);
}

static MagickBooleanType ParseDeclaration(const DeclarationInfo *declaration,
  const DeclarationTargets *declaration_targets)
{
  XMLTreeRoot
    *const root = declaration->root;

  const size_t
    ignore_depth = declaration->ignore_depth;

  ExceptionInfo
    *const exception = declaration->exception;

  char
    **const p = declaration_targets->p;

  char
    **const tag = declaration_targets->tag;

  if (strncmp(*p,"![CDATA[",8) == 0)
    {
      /*
        Cdata.
      */
      *p=strstr(*p,"]]>");
      if (*p != (char *) NULL)
        {
          *p=ParseCDataContent(root,*tag,*p,ignore_depth);
        }
      else
        {
          (void) ThrowMagickException(exception,GetMagickModule(),
            OptionWarning,"ParseError","unclosed <![CDATA[");
          return(MagickFalse);
        }
    }
  else
    return(ParseDoctypeOrInstruction(declaration,declaration_targets));
  return(MagickTrue);
}

typedef struct _MarkupTargets
{
  char
    **p,
    **tag;

  ssize_t
    *l;

  size_t
    *ignore_depth;
} MarkupTargets;

static MagickBooleanType ParseMarkupDeclaration(XMLTreeRoot *root,
  const int terminal,ExceptionInfo *exception,const MarkupTargets *markup)
{
  char
    **const p = markup->p;

  char
    **const tag = markup->tag;

  ssize_t
    *const l = markup->l;

  size_t
    *const ignore_depth = markup->ignore_depth;

  DeclarationInfo
    declaration;

  DeclarationTargets
    declaration_targets;

  declaration.root=root;
  declaration.ignore_depth=*ignore_depth;
  declaration.terminal=terminal;
  declaration.exception=exception;
  declaration_targets.p=p;
  declaration_targets.tag=tag;
  declaration_targets.l=l;
  if (ParseDeclaration(&declaration,&declaration_targets) ==
      MagickFalse)
    {
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType ParseCommentOrDeclaration(XMLTreeRoot *root,
  const int terminal,ExceptionInfo *exception,const MarkupTargets *markup)
{
  char
    **const p = markup->p;

  if (strncmp(*p,"!--",3) == 0)
    {
      if (SkipComment(p,terminal,exception) == MagickFalse)
        {
          return(MagickFalse);
        }
    }
  else
    return(ParseMarkupDeclaration(root,terminal,exception,markup));
  return(MagickTrue);
}

static MagickBooleanType ParseMarkup(XMLTreeRoot *root,const int terminal,
  ExceptionInfo *exception,const MarkupTargets *markup)
{
  char
    **const p = markup->p;

  size_t
    *const ignore_depth = markup->ignore_depth;

  if (**p == '/')
    {
      EndTagTargets
        targets;

      targets.p=p;
      targets.ignore_depth=ignore_depth;
      if (ParseEndTag(root,terminal,exception,&targets) == MagickFalse)
        {
          return(MagickFalse);
        }
    }
  else
    return(ParseCommentOrDeclaration(root,terminal,exception,markup));
  return(MagickTrue);
}

static XMLTreeInfo *CheckXMLTreeClosed(XMLTreeRoot *root,
  ExceptionInfo *exception)
{
  if (root->node == (XMLTreeInfo *) NULL)
    return(&root->root);
  if (root->node->tag == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "ParseError","root tag missing");
      return(&root->root);
    }
  (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
    "ParseError","unclosed tag: '%s'",root->node->tag);
  return(&root->root);
}

static MagickBooleanType SkipToRootTag(char **p,ExceptionInfo *exception)
{
  while ((**p != '\0') && (**p != '<'))
    (*p)++;
  if (**p == '\0')
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "ParseError","root tag missing");
      return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _XMLStartTagInput
{
  XMLTreeRoot
    *root;

  char
    *tag;

  int
    terminal;

  ExceptionInfo
    *exception;
} XMLStartTagInput;

typedef struct _XMLStartTagTargets
{
  char
    ***attribute,
    ***attributes,
    **p;

  ssize_t
    *l;

  size_t
    *ignore_depth;
} XMLStartTagTargets;

static MagickBooleanType ParseXMLStartTag(const XMLStartTagInput *input,
  const XMLStartTagTargets *targets)
{
  char
    ***const attribute = targets->attribute,
    ***const attributes = targets->attributes,
    **const p = targets->p;

  ssize_t
    *const l = targets->l;

  size_t
    *const ignore_depth = targets->ignore_depth;

  XMLTreeRoot
    *const root = input->root;

  char
    *const tag = input->tag;

  const int
    terminal = input->terminal;

  ExceptionInfo
    *const exception = input->exception;

  AttributeSectionInfo
    section;

  AttributeSectionTargets
    section_targets;

  StartTagInfo
    start_tag;

  StartTagTargets
    start_tag_targets;

  /*
    Tag.
  */
  if (root->node == (XMLTreeInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionWarning,"ParseError","root tag missing");
      return(MagickFalse);
    }
  (*p)+=(ptrdiff_t) strcspn((*p),XMLWhitespace "/>");
  while (isspace((int) ((unsigned char) *(*p))) != 0)
    *(*p)++='\0';
  section.root=root;
  section.tag=tag;
  section.ignore_depth=(*ignore_depth);
  section.exception=exception;
  section_targets.attribute=attribute;
  section_targets.attributes=attributes;
  section_targets.p=p;
  section_targets.l=l;
  if (ParseStartTagAttributes(&section,&section_targets) == MagickFalse)
    return(MagickFalse);
  start_tag.root=root;
  start_tag.tag=tag;
  start_tag.attributes=(*attributes);
  start_tag.l=(*l);
  start_tag.terminal=terminal;
  start_tag.exception=exception;
  start_tag_targets.p=p;
  start_tag_targets.ignore_depth=ignore_depth;
  if (EndStartTag(&start_tag,&start_tag_targets) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _XMLTreeUTF8Targets
{
  char
    **utf8;

  size_t
    *length;
} XMLTreeUTF8Targets;

static MagickBooleanType ConvertXMLTreeToUTF8(const char *xml,
  ExceptionInfo *exception,const XMLTreeUTF8Targets *targets)
{
  char
    **const utf8 = targets->utf8;

  size_t
    *const length = targets->length;

  *length=strlen(xml);
  *utf8=ConvertUTF16ToUTF8(xml,length);
  if (*utf8 == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "ParseError","UTF16 to UTF8 failed");
      return(MagickFalse);
    }
  if (*length == 0)
    {
      *utf8=DestroyString(*utf8);
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "ParseError","root tag missing");
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType ParseXMLTreeContent(XMLTreeRoot *root,
  const size_t ignore_depth,char **p)
{
  char
    *tag;

  if (((*p) == (char *) NULL) || (*(*p) == '\0'))
    return(MagickFalse);
  *(*p)++='\0';
  tag=(*p);
  if (ParseTagCharacterContent(root,tag,ignore_depth,p) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType ParseXMLTreeTags(XMLTreeRoot *root,char *p,
  const int terminal,ExceptionInfo *exception)
{
  char
    **attribute,
    **attributes,
    *tag;

  int
    c;

  MagickBooleanType
    status;

  size_t
    ignore_depth;

  ssize_t
    l;

  attribute=(char **) NULL;
  l=0;
  ignore_depth=0;
  for (p++; ; p++)
  {
    attributes=(char **) sentinel;
    tag=p;
    c=(*p);
    if (IsXMLNameStart(p,c) != MagickFalse)
      {
        const XMLStartTagTargets
          start_targets = { &attribute, &attributes, &p, &l, &ignore_depth };

        const XMLStartTagInput
          start_input = { root, tag, terminal, exception };

        status=ParseXMLStartTag(&start_input,&start_targets);
      }
    else
      {
        MarkupTargets
          markup;

        markup.p=(&p);
        markup.tag=(&tag);
        markup.l=(&l);
        markup.ignore_depth=(&ignore_depth);
        status=ParseMarkup(root,terminal,exception,&markup);
      }
    if (status == MagickFalse)
      return(MagickFalse);
    if (ParseXMLTreeContent(root,ignore_depth,&p) == MagickFalse)
      break;
  }
  return(MagickTrue);
}

#endif
