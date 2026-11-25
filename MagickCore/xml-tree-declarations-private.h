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

  MagickCore private methods of xml-tree.c: the declarations of the XML
  parser, from ValidateEntities to the entity declarations (processing
  instructions, attribute-list and entity declarations); included by that
  file only.
*/
#ifndef MAGICKCORE_XML_TREE_DECLARATIONS_PRIVATE_H
#define MAGICKCORE_XML_TREE_DECLARATIONS_PRIVATE_H

static MagickBooleanType ValidateEntities(const EntityCheckInfo *check,
  char *xml,const int delimiter,const size_t depth)
{
  char
    *tag = check->tag,
    **entities = check->entities;

  ssize_t
    i;

  /*
    Check for circular entity references.
  */
  if (depth >= MagickMaxRecursionDepth)
    return(MagickFalse);
  for ( ; ; xml++)
  {
    xml=SkipToDelimiter(xml,delimiter);
    if (*xml == '\0')
      return(MagickTrue);
    if (strncmp(xml+1,tag,strlen(tag)) == 0)
      return(MagickFalse);
    i=SkipMatchingEntities(entities,xml);
    if ((entities[i] != (char *) NULL) &&
        (ValidateEntities(check,entities[i+1],delimiter,depth+1) == MagickFalse))
      return(MagickFalse);
  }
}

static void AddProcessingInstructionTarget(XMLTreeRoot *root,const ssize_t i,
  const char *target)
{
  if (root->processing_instructions[i] == (char **) NULL)
    {
      root->processing_instructions=(char ***) ResizeQuantumMemory(
        root->processing_instructions,(size_t) (i+2),
        sizeof(*root->processing_instructions));
      if (root->processing_instructions == (char ***) NULL)
        ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
      root->processing_instructions[i]=(char **) AcquireQuantumMemory(3,
        sizeof(**root->processing_instructions));
      if (root->processing_instructions[i] == (char **) NULL)
        ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
      root->processing_instructions[i+1]=(char **) NULL;
      root->processing_instructions[i][0]=ConstantString(target);
      root->processing_instructions[i][1]=(char *)
        root->processing_instructions[i+1];
      root->processing_instructions[i+1]=(char **) NULL;
      root->processing_instructions[i][2]=ConstantString("");
    }
}

static void AppendProcessingInstruction(XMLTreeRoot *root,const ssize_t i,
  const char *xml)
{
  ssize_t
    j;

  j=1;
  while (root->processing_instructions[i][j] != (char *) NULL)
    j++;
  root->processing_instructions[i]=(char **) ResizeQuantumMemory(
    root->processing_instructions[i],(size_t) (j+3),
    sizeof(**root->processing_instructions));
  if (root->processing_instructions[i] == (char **) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  root->processing_instructions[i][j+2]=(char *) ResizeQuantumMemory(
    root->processing_instructions[i][j+1],(size_t) (j+1),
    sizeof(***root->processing_instructions));
  if (root->processing_instructions[i][j+2] == (char *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) CopyMagickString(root->processing_instructions[i][j+2]+j-1,
    root->root.tag != (char *) NULL ? ">" : "<",2);
  root->processing_instructions[i][j]=ConstantString(xml);
  root->processing_instructions[i][j+1]=(char *) NULL;
}

static ssize_t FindProcessingInstruction(const XMLTreeRoot *root,
  const char *target)
{
  ssize_t
    i;

  i=0;
  while ((root->processing_instructions[i] != (char **) NULL) &&
         (strcmp(target,root->processing_instructions[i][0]) != 0))
    i++;
  return(i);
}

static void ParseProcessingInstructions(XMLTreeRoot *root,char *xml,
  size_t length)
{
  char
    *target;

  ssize_t
    i;

  target=xml;
  xml[length]='\0';
  xml+=strcspn(xml,XMLWhitespace);
  if (*xml != '\0')
    {
      *xml='\0';
      xml+=strspn(xml+1,XMLWhitespace)+1;
    }
  if (strcmp(target,"xml") == 0)
    {
      xml=strstr(xml,"standalone");
      if ((xml != (char *) NULL) &&
          (strncmp(xml+strspn(xml+10,XMLWhitespace "='\"")+10,"yes",3) == 0))
        root->standalone=MagickTrue;
      return;
    }
  if (root->processing_instructions[0] == (char **) NULL)
    {
      root->processing_instructions=(char ***) AcquireCriticalMemory(sizeof(
        *root->processing_instructions));
      *root->processing_instructions=(char **) NULL;
    }
  i=FindProcessingInstruction(root,target);
  AddProcessingInstructionTarget(root,i,target);
  AppendProcessingInstruction(root,i,xml);
}

static inline MagickBooleanType IsDoctypeTextChar(const char *xml)
{
  return(((*xml != '\0') && (*xml != '<') && (*xml != '%')) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsOtherAttributeList(XMLTreeRoot *root,
  const ssize_t i,const char *n)
{
  return(((root->attributes[i] != (char **) NULL) &&
    (n != (char *) NULL) &&
    (strcmp(n,root->attributes[i][0]) != 0)) ? MagickTrue : MagickFalse);
}

static void AddDefaultAttributeTag(XMLTreeRoot *root,const ssize_t i,
  const char *t)
{
  if (root->attributes[i] == (char **) NULL)
    {
      /*
        New attribute tag.
      */
      if (i == 0)
        root->attributes=(char ***) AcquireQuantumMemory(2,
          sizeof(*root->attributes));
      else
        root->attributes=(char ***) ResizeQuantumMemory(
          root->attributes,(size_t) (i+2),
          sizeof(*root->attributes));
      if (root->attributes == (char ***) NULL)
        ThrowFatalException(ResourceLimitFatalError,
          "MemoryAllocationFailed");
      root->attributes[i]=(char **) AcquireQuantumMemory(2,
        sizeof(**root->attributes));
      if (root->attributes[i] == (char **) NULL)
        ThrowFatalException(ResourceLimitFatalError,
          "MemoryAllocationFailed");
      root->attributes[i][0]=ConstantString(t);
      root->attributes[i][1]=(char *) NULL;
      root->attributes[i+1]=(char **) NULL;
    }
}

typedef struct _DefaultAttributeInfo
{
  char
    *c,
    *n,
    *t,
    *v;

  ssize_t
    i;
} DefaultAttributeInfo;

static void AppendDefaultAttribute(XMLTreeRoot *root,
  const DefaultAttributeInfo *attribute)
{
  char
    *const c = attribute->c,
    *const n = attribute->n,
    *const t = attribute->t,
    *const v = attribute->v;

  const ssize_t
    i = attribute->i;

  ssize_t
    j;

  AddDefaultAttributeTag(root,i,t);
  for (j=1; root->attributes[i][j] != (char *) NULL; j+=3) ;
  root->attributes[i]=(char **) ResizeQuantumMemory(
    root->attributes[i],(size_t) (j+4),sizeof(**root->attributes));
  if (root->attributes[i] == (char **) NULL)
    ThrowFatalException(ResourceLimitFatalError,
      "MemoryAllocationFailed");
  root->attributes[i][j+3]=(char *) NULL;
  root->attributes[i][j+2]=ConstantString(c);
  root->attributes[i][j+1]=(char *) NULL;
  if (v != (char *) NULL)
    root->attributes[i][j+1]=ParseEntities(v,root->entities,*c);
  root->attributes[i][j]=ConstantString(n);
}

typedef struct _AttributeTypeTargets
{
  char
    **xml,
    **c;
} AttributeTypeTargets;

static MagickBooleanType ParseAttributeType(char *n,ExceptionInfo *exception,
  const AttributeTypeTargets *targets)
{
  char
    **const xml = targets->xml;

  char
    **const c = targets->c;

  *xml=n+strcspn(n,XMLWhitespace);
  if (**xml != '\0')
    **xml='\0';
  else
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionWarning,"ParseError","malformed <!ATTLIST");
      return(MagickFalse);
    }
  *xml+=strspn(*xml+1,XMLWhitespace)+1;
  *c=(char *) (strncmp(*xml,"CDATA",5) != 0 ? "*" : " ");
  if (strncmp(*xml,"NOTATION",8) == 0)
    *xml+=strspn(*xml+8,XMLWhitespace)+8;
  *xml=(**xml == '(') ? strchr(*xml,')') : *xml+
    strcspn(*xml,XMLWhitespace);
  if (*xml == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionWarning,"ParseError","malformed <!ATTLIST");
      return(MagickFalse);
    }
  *xml+=strspn(*xml,XMLWhitespace ")");
  if (strncmp(*xml,"#FIXED",6) == 0)
    *xml+=strspn(*xml+6,XMLWhitespace)+6;
  return(MagickTrue);
}

typedef struct _AttributeDefinitionInfo
{
  XMLTreeRoot
    *root;

  ExceptionInfo
    *exception;

  char
    **xml,
    **n,
    *t;

  ssize_t
    i;
} AttributeDefinitionInfo;

static inline MagickBooleanType IsQuotedValueStart(char **xml)
{
  return(((**xml == '"') || (**xml == '\'')) ? MagickTrue : MagickFalse);
}

static MagickBooleanType ParseAttributeDefinition(
  const AttributeDefinitionInfo *definition)
{
  XMLTreeRoot
    *const root = definition->root;

  ExceptionInfo
    *const exception = definition->exception;

  char
    **const xml = definition->xml,
    **const n = definition->n,
    *const t = definition->t;

  const ssize_t
    i = definition->i;

  AttributeTypeTargets
    attribute_targets;

  char
    *c,
    *v;

  DefaultAttributeInfo
    attribute;

  attribute_targets.xml=xml;
  attribute_targets.c=(&c);
  if (ParseAttributeType(*n,exception,&attribute_targets) ==
      MagickFalse)
    {
      return(MagickFalse);
    }
  if (**xml == '#')
    {
      *xml+=strcspn(*xml,XMLWhitespace ">")-1;
      if (*c == ' ')
        return(MagickTrue);
      v=(char *) NULL;
    }
  else
    if ((IsQuotedValueStart(xml) != MagickFalse) &&
        ((*xml=strchr(v=*xml+1,**xml)) != (char *) NULL))
      **xml='\0';
    else
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          OptionWarning,"ParseError","malformed <!ATTLIST");
        return(MagickFalse);
      }
  attribute.c=c;
  attribute.n=*n;
  attribute.t=t;
  attribute.v=v;
  attribute.i=i;
  AppendDefaultAttribute(root,&attribute);
  return(MagickTrue);
}

typedef struct _AttributeListTargets
{
  char
    **xml,
    **n;
} AttributeListTargets;

static MagickBooleanType ParseAttributeListDeclaration(XMLTreeRoot *root,
  ExceptionInfo *exception,const AttributeListTargets *targets)
{
  char
    **const xml = targets->xml;

  char
    **const n = targets->n;

  char
    *t;

  ssize_t
    i;

  /*
    Parse default attributes.
  */
  t=*xml+strspn(*xml+9,XMLWhitespace)+9;
  if (*t == '\0')
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionWarning,"ParseError","unclosed <!ATTLIST");
      return(MagickFalse);
    }
  *xml=t+strcspn(t,XMLWhitespace ">");
  if (**xml == '>')
    return(MagickTrue);
  **xml='\0';
  i=0;
  while (IsOtherAttributeList(root,i,*n) != MagickFalse)
    i++;
  while ((*(*n=*xml+strspn(*xml+1,XMLWhitespace)+1) != '\0') &&
         (**n != '>'))
  {
    AttributeDefinitionInfo
      definition;

    definition.root=root;
    definition.exception=exception;
    definition.xml=xml;
    definition.n=n;
    definition.t=t;
    definition.i=i;
    if (ParseAttributeDefinition(&definition) == MagickFalse)
      {
        return(MagickFalse);
      }
  }
  return(MagickTrue);
}

static MagickBooleanType SkipDoctypeMarkup(XMLTreeRoot *root,char **xml)
{
  char
    *c;

  if (strncmp(*xml, "<!--", 4) == 0)
    *xml=strstr(*xml+4,"-->");
  else
    if (strncmp(*xml,"<?", 2) == 0)
      {
        c=*xml+2;
        *xml=strstr(c,"?>");
        if (*xml != (char *) NULL)
          {
            ParseProcessingInstructions(root,c,(size_t) (*xml-c));
            (*xml)++;
          }
      }
     else
       if (**xml == '<')
         *xml=strchr(*xml,'>');
       else
         if ((*((*xml)++) == '%') && (root->standalone == MagickFalse))
           return(MagickFalse);
  return(MagickTrue);
}

typedef struct _EntityHeadTargets
{
  char
    **xml,
    **c,
    **n,
    **v,
    *q;
} EntityHeadTargets;

static MagickBooleanType ParseEntityDeclarationHead(
  const EntityHeadTargets *targets)
{
  char
    **const xml = targets->xml,
    **const c = targets->c,
    **const n = targets->n,
    **const v = targets->v,
    *const q = targets->q;

  if (strspn(*xml+8,XMLWhitespace) == 0)
    return(MagickFalse);
  *xml+=strspn(*xml+8,XMLWhitespace)+8;
  *c=(*xml);
  *n=(*xml)+strspn(*xml,XMLWhitespace "%");
  if ((isalpha((int) ((unsigned char) **n)) == 0) && (**n != '_'))
    return(MagickFalse);
  *xml=(*n)+strcspn(*n,XMLWhitespace);
  if (**xml == '\0')
    return(MagickFalse);
  **xml=';';
  *v=(*xml)+strspn(*xml+1,XMLWhitespace)+1;
  *q=(**v);
  (*v)++;
  return(MagickTrue);
}

static void TerminateEntityValue(char **xml,char *v,const char q)
{
  (*xml)++;
  **xml='\0';
  *xml=strchr(v,q);
  if (*xml != (char *) NULL)
    {
      **xml='\0';
      (*xml)++;
    }
}

static ssize_t CountXMLEntities(char **entities)
{
  ssize_t
    i;

  for (i=0; entities[i] != (char *) NULL; i++) ;
  return(i);
}

static MagickBooleanType IsNonCircularEntity(char *n,char *v,char **entities)
{
  const EntityCheckInfo
    check = { n, entities };

  return(((ValidateEntities(&check,v,'%',0) != MagickFalse) &&
    (ValidateEntities(&check,v,'&',0) != MagickFalse)) ? MagickTrue :
    MagickFalse);
}

static void DestroyParsedEntity(char **entities,const ssize_t i,char *v)
{
  if (entities[i+1] != v)
    entities[i+1]=DestroyString(entities[i+1]);
}

typedef struct _EntityValueInput
{
  XMLTreeRoot
    *root;

  char
    *c,
    *n,
    *v,
    q;

  ExceptionInfo
    *exception;
} EntityValueInput;

typedef struct _EntityValueTargets
{
  char
    **xml,
    ***predefined_entities;
} EntityValueTargets;

static MagickBooleanType AddEntityDeclaration(const EntityValueInput *input,
  const EntityValueTargets *targets)
{
  char
    **const xml = targets->xml,
    ***const predefined_entities = targets->predefined_entities;

  XMLTreeRoot
    *const root = input->root;

  char
    *const c = input->c,
    *const n = input->n,
    *const v = input->v;

  const char
    q = input->q;

  ExceptionInfo
    *const exception = input->exception;

  char
    **entities;

  ssize_t
    i;

  entities=(*c == '%') ? (*predefined_entities) : root->entities;
  i=CountXMLEntities(entities);
  entities=(char **) ResizeQuantumMemory(entities,(size_t) (i+3),
    sizeof(*entities));
  if (entities == (char **) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  if (*c == '%')
    (*predefined_entities)=entities;
  else
    root->entities=entities;
  TerminateEntityValue(xml,v,q);
  entities[i+1]=ParseEntities(v,(*predefined_entities),'%');
  entities[i+2]=(char *) NULL;
  if (IsNonCircularEntity(n,entities[i+1],entities) != MagickFalse)
    entities[i]=n;
  else
    {
      DestroyParsedEntity(entities,i,v);
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionWarning,"ParseError","circular entity declaration %s",n);
      return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _InternalDoctypeInfo
{
  XMLTreeRoot
    *root;

  size_t
    length;

  ExceptionInfo
    *exception;
} InternalDoctypeInfo;

static MagickBooleanType AddInternalEntityDeclaration(
  const EntityValueInput *entity,char **xml,char ***predefined_entities)
{
  XMLTreeRoot
    *const root = entity->root;

  char
    *const c = entity->c,
    *const n = entity->n,
    *const v = entity->v;

  const char
    q = entity->q;

  ExceptionInfo
    *const exception = entity->exception;

  EntityValueInput
    value_input;

  EntityValueTargets
    value_targets;

  if ((q != '"') && (q != '\''))
    {
      /*
        Skip externals.
      */
      (*xml)=strchr((*xml),'>');
      return(MagickTrue);
    }
  value_input.root=root;
  value_input.c=c;
  value_input.n=n;
  value_input.v=v;
  value_input.q=q;
  value_input.exception=exception;
  value_targets.xml=xml;
  value_targets.predefined_entities=predefined_entities;
  if (AddEntityDeclaration(&value_input,&value_targets) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType ParseOtherDoctypeMarkup(XMLTreeRoot *root,
  ExceptionInfo *exception,const AttributeListTargets *targets,
  MagickBooleanType *broke)
{
  char
    **const xml = targets->xml,
    **const n = targets->n;

  (*broke)=MagickFalse;
  if (strncmp((*xml),"<!ATTLIST",9) == 0)
    {
      AttributeListTargets
        attribute_list_targets;

      attribute_list_targets.xml=xml;
      attribute_list_targets.n=n;
      if (ParseAttributeListDeclaration(root,exception,
          &attribute_list_targets) == MagickFalse)
        return(MagickFalse);
    }
  else
    if (SkipDoctypeMarkup(root,xml) == MagickFalse)
      {
        (*broke)=MagickTrue;
        return(MagickTrue);
      }
  return(MagickTrue);
}

static MagickBooleanType ParseOtherDoctypeDeclaration(XMLTreeRoot *root,
  ExceptionInfo *exception,const AttributeListTargets *targets,
  MagickBooleanType *broke)
{
  if (ParseOtherDoctypeMarkup(root,exception,targets,
        broke) == MagickFalse)
    return(MagickFalse);
  if ((*broke) != MagickFalse)
    {
      (*broke)=MagickTrue;
      return(MagickTrue);
    }
  return(MagickTrue);
}

#endif
