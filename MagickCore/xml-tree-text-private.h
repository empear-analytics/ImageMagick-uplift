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

  MagickCore private methods of xml-tree.c: the text decoding of the XML
  parser, from WriteUTF8Bytes to the EntityCheckInfo typedef (UTF-16
  conversion, entity and character references, character content, close
  tags); included by that file only.
*/
#ifndef MAGICKCORE_XML_TREE_TEXT_PRIVATE_H
#define MAGICKCORE_XML_TREE_TEXT_PRIVATE_H

static ssize_t WriteUTF8Bytes(char *utf8,ssize_t j,const int c)
{
  int
    bits,
    byte;

  byte=c;
  for (bits=0; byte != 0; byte/=2)
    bits++;
  bits=(bits-2)/5;
  utf8[j++]=(char) ((0xFF << (7-bits)) | (c >> (6*bits)));
  while (bits != 0)
  {
    bits--;
    utf8[j]=(char) (0x80 | ((c >> (6*bits)) & 0x3f));
    j++;
  }
  return(j);
}

static void AppendUTF8Character(char *utf8,const int c,ssize_t *j)
{
  if (c < 0x80)
    {
      utf8[*j]=(char) c;
      (*j)++;
      return;
    }
  /*
    Multi-byte UTF-8 sequence.
  */
  *j=WriteUTF8Bytes(utf8,*j,c);
}

static int ReadUTF16Unit(const char *content,const ssize_t i,
  const int encoding)
{
  return((encoding != 0) ? ((content[i] & 0xff) << 8) |
    (content[i+1] & 0xff) : ((content[i+1] & 0xff) << 8) |
    (content[i] & 0xff));
}

static int CombineUTF16Surrogates(const char *content,const ssize_t i,
  const int encoding,int c)
{
  int
    byte;

  byte=ReadUTF16Unit(content,i,encoding);
  c=(((c & 0x3ff) << 10) | (byte & 0x3ff))+0x10000;
  return(c);
}

static inline MagickBooleanType IsUTF16Surrogate(const int c)
{
  return(((c >= 0xd800) && (c <= 0xdfff)) ? MagickTrue : MagickFalse);
}

static int ReadUTF16Character(const char *content,const size_t *length,
  const int encoding,ssize_t *i)
{
  int
    c;

  c=ReadUTF16Unit(content,(*i),encoding);
  if ((IsUTF16Surrogate(c) != MagickFalse) &&
      (((*i)+=2) < (ssize_t) (*length-1)))
    {
      c=CombineUTF16Surrogates(content,(*i),encoding,c);
    }
  return(c);
}

static int GetUTF16Encoding(const char *content)
{
  int
    encoding;

  encoding=(*content == '\xFE') ? 1 : (*content == '\xFF') ? 0 : -1;
  return(encoding);
}

static char *ConvertUTF16ToUTF8(const char *content,size_t *length)
{
  char
    *utf8;

  int
    c,
    encoding;

  size_t
    extent;

  ssize_t
    i,
    j;

  utf8=(char *) AcquireQuantumMemory(*length+1,sizeof(*utf8));
  if (utf8 == (char *) NULL)
    return((char *) NULL);
  encoding=GetUTF16Encoding(content);
  if (encoding == -1)
    {
      /*
        Already UTF-8.
      */
      (void) memcpy(utf8,content,*length*sizeof(*utf8));
      utf8[*length]='\0';
      return(utf8);
    }
  j=0;
  extent=(*length);
  for (i=2; i < (ssize_t) (*length-1); i+=2)
  {
    c=ReadUTF16Character(content,length,encoding,&i);
    if ((size_t) (j+MagickPathExtent) > extent)
      {
        extent=(size_t) j+MagickPathExtent;
        utf8=(char *) ResizeQuantumMemory(utf8,extent,sizeof(*utf8));
        if (utf8 == (char *) NULL)
          return(utf8);
      }
    AppendUTF8Character(utf8,c,&j);
  }
  *length=(size_t) j;
  utf8=(char *) ResizeQuantumMemory(utf8,(*length+1),sizeof(*utf8));
  if (utf8 != (char *) NULL)
    utf8[*length]='\0';
  return(utf8);
}

static inline MagickBooleanType IsEntityTextChar(const char *xml,
  const int state)
{
  return(((*xml != '\0') && (*xml != '&') && ((*xml != '%') ||
    (state != '%')) &&
    (isspace((int) ((unsigned char) *xml)) == 0)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsEntityReference(const char *xml,
  const int state)
{
  return((((*xml == '&') && ((state == '&') || (state == ' ') ||
    (state == '*'))) || ((state == '%') && (*xml == '%'))) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsCollapsibleSpace(const char *xml,
  const int state)
{
  return((((state == ' ') || (state == '*')) &&
    (isspace((int) ((unsigned char) *xml)) != 0)) ? MagickTrue : MagickFalse);
}

static char *CopyEntityBuffer(const char *p,const size_t extent)
{
  char
    *extent_xml;

  extent_xml=(char *) AcquireQuantumMemory(extent+1,
    sizeof(*extent_xml));
  if (extent_xml != (char *) NULL)
    {
      memset(extent_xml,0,extent*sizeof(*extent_xml));
      (void) CopyMagickString(extent_xml,p,extent*
        sizeof(*extent_xml));
    }
  return(extent_xml);
}

static char *ResizeEntityBuffer(char *p,const char *q,const size_t extent)
{
  if (p != q)
    {
      p=(char *) ResizeQuantumMemory(p,extent+1,sizeof(*p));
      if (p != (char *) NULL)
        p[extent]='\0';
    }
  else
    p=CopyEntityBuffer(p,extent);
  return(p);
}

static void NormalizeAttributeSpaces(char *p)
{
  char
    *xml;

  ssize_t
    i;

  for (xml=p; *xml != '\0'; xml++)
  {
    char
      accept[] = " ";

    i=(ssize_t) strspn(xml,accept);
    if (i != 0)
      (void) memmove(xml,xml+i,strlen(xml+i)+1);
    while ((*xml != '\0') && (*xml != ' '))
      xml++;
    if (*xml == '\0')
      break;
  }
  xml--;
  if ((xml >= p) && (*xml == ' '))
    *xml='\0';
}

static void NormalizeLineEndings(char *xml)
{
  for ( ; *xml != '\0'; xml++)
    while (*xml == '\r')
    {
      *(xml++)='\n';
      if (*xml == '\n')
        (void) memmove(xml,xml+1,strlen(xml));
    }
}

static void DecodeCharacterReference(char **xml)
{
  char
    *entity;

  int
    c;

  /*
    Character reference.
  */
  if ((*xml)[2] != 'x')
    c=(int) strtol(*xml+2,&entity,10);  /* base 10 */
  else
    c=(int) strtol(*xml+3,&entity,16);  /* base 16 */
  if ((c == 0) || (*entity != ';'))
    {
      /*
        Not a character reference.
      */
      (*xml)++;
      return;
    }
  if (c < 0x80)
    *((*xml)++)=(char) c;
  else
    {
      /*
        Multi-byte UTF-8 sequence.
      */
      *xml+=WriteUTF8Bytes(*xml,0,c);
    }
  (void) memmove(*xml,strchr(*xml,';')+1,strlen(strchr(*xml,';')));
}

static ssize_t FindEntityIndex(char **entities,const char *xml)
{
  ssize_t
    i;

  i=0;
  while ((entities[i] != (char *) NULL) &&
         (strncmp(xml+1,entities[i],strlen(entities[i])) != 0))
    i+=2;
  return(i);
}

static void DecodeEntityReference(char **entities,char *q,char **p,
  char **xml)
{
  char
    *entity;

  size_t
    extent,
    length;

  ssize_t
    i,
    offset;

  /*
    Find entity in the list.
  */
  i=FindEntityIndex(entities,*xml);
  if (entities[i++] == (char *) NULL)
    (*xml)++;
  else
    if (entities[i] != (char *) NULL)
      {
        /*
          Found a match.
        */
        length=strlen(entities[i]);
        entity=strchr(*xml,';');
        if ((entity != (char *) NULL) &&
            ((length-1L) >= (size_t) (entity-*xml)))
          {
            offset=(ssize_t) (*xml-*p);
            extent=((size_t) offset+length+strlen(entity));
            *p=ResizeEntityBuffer(*p,q,extent);
            if (*p == (char *) NULL)
              ThrowFatalException(ResourceLimitFatalError,
                "MemoryAllocationFailed");
            *xml=*p+offset;
            entity=strchr(*xml,';');
          }
        if (entity != (char *) NULL)
          (void) memmove(*xml+length,entity+1,strlen(entity));
        (void) memcpy(*xml,entities[i],length);
      }
}

static MagickBooleanType SkipEntityText(char **xml,int state)
{
  while (IsEntityTextChar(*xml,state) != MagickFalse)
    (*xml)++;
  if (**xml == '\0')
    return(MagickFalse);
  return(MagickTrue);
}

static char *SkipEntityTextChar(char *xml,int state)
{
  if (IsCollapsibleSpace(xml,state) != MagickFalse)
    *(xml++)=' ';
  else
    xml++;
  return(xml);
}

static char *ParseEntities(char *xml,char **entities,int state)
{
  char
    *p,
    *q;

  /*
    Normalize line endings.
  */
  p=xml;
  q=xml;
  NormalizeLineEndings(xml);
  for (xml=p; ; )
  {
    if (SkipEntityText(&xml,state) == MagickFalse)
      break;
    /*
      States include:
        '&' for general entity decoding
        '%' for parameter entity decoding
        'c' for CDATA sections
        ' ' for attributes normalization
        '*' for non-CDATA attributes normalization
    */
    if ((state != 'c') && (strncmp(xml,"&#",2) == 0))
      DecodeCharacterReference(&xml);
    else
      if (IsEntityReference(xml,state) != MagickFalse)
        DecodeEntityReference(entities,q,&p,&xml);
      else
        xml=SkipEntityTextChar(xml,state);
  }
  if (state == '*')
    {
      /*
        Normalize spaces for non-CDATA attributes.
      */
      NormalizeAttributeSpaces(p);
    }
  return(p == q ? ConstantString(p) : p);
}

static inline MagickBooleanType IsCharacterContentSkipped(
  const XMLTreeInfo *xml_info,const size_t length)
{
  return(((xml_info == (XMLTreeInfo *) NULL) ||
    (xml_info->tag == (char *) NULL) ||
    (length == 0)) ? MagickTrue : MagickFalse);
}

static void ParseCharacterContent(XMLTreeRoot *root,char *xml,
  const size_t length,const char state)
{
  XMLTreeInfo
    *xml_info;

  xml_info=root->node;
  if (IsCharacterContentSkipped(xml_info,length) != MagickFalse)
    return;
  xml[length]='\0';
  xml=ParseEntities(xml,root->entities,state);
  if ((xml_info->content != (char *) NULL) && (*xml_info->content != '\0'))
    {
      (void) ConcatenateString(&xml_info->content,xml);
      xml=DestroyString(xml);
    }
  else
    {
      if (xml_info->content != (char *) NULL)
        xml_info->content=DestroyString(xml_info->content);
      xml_info->content=xml;
    }
}

static inline MagickBooleanType IsUnmatchedCloseTag(const XMLTreeRoot *root,
  const char *tag)
{
  return(((root->node == (XMLTreeInfo *) NULL) ||
    (root->node->tag == (char *) NULL) ||
    (strcmp(tag,root->node->tag) != 0)) ? MagickTrue : MagickFalse);
}

static XMLTreeInfo *ParseCloseTag(XMLTreeRoot *root,char *tag,
  ExceptionInfo *exception)
{
  if (IsUnmatchedCloseTag(root,tag) != MagickFalse)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "ParseError","unexpected closing tag </%s>",tag);
      return(&root->root);
    }
  root->node=root->node->parent;
  return((XMLTreeInfo *) NULL);
}

static char *SkipToDelimiter(char *xml,const int delimiter)
{
  while ((*xml != '\0') && (*xml != delimiter))
    xml++;
  return(xml);
}

static ssize_t SkipMatchingEntities(char **entities,const char *xml)
{
  ssize_t
    i;

  i=0;
  while ((entities[i] != (char *) NULL) &&
         (strncmp(entities[i],xml+1,strlen(entities[i])) == 0))
    i+=2;
  return(i);
}

typedef struct _EntityCheckInfo
{
  char
    *tag;

  char
    **entities;
} EntityCheckInfo;

#endif
