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

  MagickCore private methods of log.c: the log configuration loader,
  IsIncludeElementOpen to LoadLogCache; included by that file only.
*/
#ifndef MAGICKCORE_LOG_LOAD_PRIVATE_H
#define MAGICKCORE_LOG_LOAD_PRIVATE_H

#if !MAGICKCORE_ZERO_CONFIGURATION_SUPPORT
static inline MagickBooleanType IsIncludeElementOpen(const char *token,
  const char *q)
{
  return((((*token != '/') && (*(token+1) != '>')) && (*q != '\0')) ?
    MagickTrue : MagickFalse);
}

static const char *SkipQuotedText(const char *q)
{
  const char
    quote = (*q),
    *p = q+1;

  /*
    Return the last character of the quoted text: the closing quote, or the
    last character of the string if the quote is not closed.
  */
  while ((*p != '\0') && (*p != quote))
    p++;
  return(*p == '\0' ? p-1 : p);
}

static const char *SkipDoctypeElement(const char *q)
{
  int
    bracket_depth = 0;

  while (*q != '\0')
  {
    if (strchr("\"'",*q) != (char *) NULL)
      q=SkipQuotedText(q);
    else if (*q == '[')
      bracket_depth++;
    else if ((*q == ']') && (bracket_depth > 0))
      bracket_depth--;
    else if ((*q == '>') && (bracket_depth == 0))
      return(q+1);   /* consume final '>' */
    q++;
  }
  return(q);
}

static const char *SkipCommentElement(const char *q,const size_t extent,
  char *token)
{
  while ((LocaleNCompare(q,"->",2) != 0) && (*q != '\0'))
    (void) GetNextToken(q,&q,extent,token);
  return(q);
}

static MagickBooleanType IsIncludeFileAttribute(const char **q,
  const size_t extent,char *keyword,char *token)
{
  (void) CopyMagickString(keyword,token,MagickPathExtent);
  (void) GetNextToken(*q,q,extent,token);
  if (*token != '=')
    return(MagickFalse);
  (void) GetNextToken(*q,q,extent,token);
  return(LocaleCompare(keyword,"file") == 0 ? MagickTrue : MagickFalse);
}

static MagickBooleanType LoadLogInclude(const LogLoadTarget *target,
  const char *filename,const char *token,const size_t depth)
{
  char
    path[MagickPathExtent],
    *file_xml;

  MagickBooleanType
    status = MagickTrue;

  GetPathComponent(filename,HeadPath,path);
  if (*path != '\0')
    (void) ConcatenateMagickString(path,DirectorySeparator,MagickPathExtent);
  if (*token == *DirectorySeparator)
    (void) CopyMagickString(path,token,MagickPathExtent);
  else
    (void) ConcatenateMagickString(path,token,MagickPathExtent);
  file_xml=FileToXML(path,~0UL);
  if (file_xml != (char *) NULL)
    {
      status=LoadLogCache(target,file_xml,path,depth+1);
      file_xml=DestroyString(file_xml);
    }
  return(status);
}

static void SetLogString(char **target,const char *token)
{
  if (*target != (char *) NULL)
    *target=(char *) RelinquishMagickMemory(*target);
  *target=ConstantString(token);
}

static void SetLogGenerations(LogInfo *log_info,const char *token)
{
  if (LocaleCompare(token,"unlimited") == 0)
    log_info->generations=(~0UL);
  else
    log_info->generations=StringToUnsignedLong(token);
}

static void SetLogLimit(LogInfo *log_info,const char *token)
{
  if (LocaleCompare(token,"unlimited") == 0)
    {
      log_info->limit=(~0UL);
      return;
    }
  log_info->limit=StringToMagickSizeType(token,100.0);
  if (log_info->limit < 1024)
    log_info->limit=1024*1024*StringToUnsignedLong(token);
}

static void SetLogInfoAttribute(LogInfo *log_info,const char *keyword,
  const char *token)
{
  if (LocaleCompare((char *) keyword,"events") == 0)
    log_info->event_mask=(LogEventType) (log_info->event_mask |
      ParseCommandOption(MagickLogEventOptions,MagickTrue,token));
  else if (LocaleCompare((char *) keyword,"filename") == 0)
    SetLogString(&log_info->filename,token);
  else if (LocaleCompare((char *) keyword,"format") == 0)
    SetLogString(&log_info->format,token);
  else if (LocaleCompare((char *) keyword,"generations") == 0)
    SetLogGenerations(log_info,token);
  else if (LocaleCompare((char *) keyword,"limit") == 0)
    SetLogLimit(log_info,token);
  else if (LocaleCompare((char *) keyword,"output") == 0)
    log_info->handler_mask=(LogHandlerType)
      (log_info->handler_mask | ParseLogHandlers(token));
}

static MagickBooleanType SkipMarkupElement(const char *keyword,const char **q,
  const size_t extent,char *token)
{
  if (LocaleNCompare(keyword,"<!DOCTYPE",9) == 0)
    *q=SkipDoctypeElement(*q);
  else
    if (LocaleNCompare(keyword,"<!--",4) == 0)
      *q=SkipCommentElement(*q,extent,token);
    else
      return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType ReadLogAttributeValue(const char **q,
  const size_t extent,char *token)
{
  (void) GetNextToken(*q,(const char **) NULL,extent,token);
  if (*token != '=')
    return(MagickFalse);
  (void) GetNextToken(*q,q,extent,token);
  (void) GetNextToken(*q,q,extent,token);
  return(MagickTrue);
}

static MagickStatusType AppendLogElement(LinkedListInfo *cache,
  LogInfo *log_info,const char *filename,ExceptionInfo *exception)
{
  MagickStatusType
    status;

  status=AppendValueToLinkedList(cache,log_info);
  if (status == MagickFalse)
    (void) ThrowMagickException(exception,GetMagickModule(),
      ResourceLimitError,"MemoryAllocationFailed","`%s'",filename);
  return(status);
}

typedef struct _LogIncludeInput
{
  const LogLoadTarget
    *target;

  const char
    *filename;

  char
    *keyword,
    *token;

  size_t
    extent,
    depth;
} LogIncludeInput;

static void LoadLogIncludes(const LogIncludeInput *include,const char **q,
  MagickStatusType *status)
{
  const LogLoadTarget
    *const target = include->target;

  const char
    *const filename = include->filename;

  char
    *const keyword = include->keyword,
    *const token = include->token;

  const size_t
    extent = include->extent,
    depth = include->depth;

  ExceptionInfo
    *exception = target->exception;

  while (IsIncludeElementOpen(token,(*q)) != MagickFalse)
  {
    if (IsIncludeFileAttribute(q,extent,keyword,token) == MagickFalse)
      continue;
    if (depth >= MagickMaxRecursionDepth)
      (void) ThrowMagickException(exception,GetMagickModule(),
        ConfigureError,"IncludeElementNestedTooDeeply","`%s'",token);
    else
      (*status)&=(MagickStatusType) LoadLogInclude(target,filename,token,
        depth);
  }
}

static MagickBooleanType ReadLogMarkupElement(
  const LogIncludeInput *include,const char **q,MagickStatusType *status)
{
  if (SkipMarkupElement(include->keyword,q,include->extent,include->token) !=
      MagickFalse)
    return(MagickTrue);
  if (LocaleCompare(include->keyword,"<include") != 0)
    return(MagickFalse);
  /*
    Include element.
  */
  LoadLogIncludes(include,q,status);
  return(MagickTrue);
}

typedef struct _LogElementInput
{
  LinkedListInfo
    *cache;

  ExceptionInfo
    *exception;

  char
    *keyword,
    *token;

  const char
    *filename;

  size_t
    extent;
} LogElementInput;

static void ParseLogElement(const LogElementInput *element,
  LogInfo **log_info,MagickStatusType *status,const char **q)
{
  LinkedListInfo
    *const cache = element->cache;

  ExceptionInfo
    *const exception = element->exception;

  char
    *const keyword = element->keyword,
    *const token = element->token;

  const char
    *const filename = element->filename;

  const size_t
    extent = element->extent;

  if (LocaleCompare(keyword,"<logmap>") == 0)
    {
      /*
        Allocate memory for the log list.
      */
      (*log_info)=(LogInfo *) AcquireCriticalMemory(sizeof(*(*log_info)));
      (void) memset((*log_info),0,sizeof(*(*log_info)));
      (*log_info)->path=ConstantString(filename);
      GetTimerInfo((TimerInfo *) &(*log_info)->timer);
      (*log_info)->signature=MagickCoreSignature;
      return;
    }
  if ((*log_info) == (LogInfo *) NULL)
    return;
  if (LocaleCompare(keyword,"</logmap>") == 0)
    {
      (*status)=AppendLogElement(cache,(*log_info),filename,exception);
      (*log_info)=(LogInfo *) NULL;
      return;
    }
  if (ReadLogAttributeValue(q,extent,token) != MagickFalse)
    SetLogInfoAttribute((*log_info),keyword,token);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   L o a d L o g C a c h e                                                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadLogCache() loads the log configurations which provides a
%  mapping between log attributes and log name.
%
%  The format of the LoadLogCache method is:
%
%      MagickBooleanType LoadLogCache(const LogLoadTarget *target,
%        const char *xml,const char *filename,const size_t depth)
%
%  A description of each parameter follows:
%
%    o target: the cache to load into, and the exception structure to
%      return any errors or warnings in.
%
%    o xml:  The log list in XML format.
%
%    o filename:  The log list filename.
%
%    o depth: depth of <include /> statements.
%
*/
static MagickBooleanType LoadLogCache(const LogLoadTarget *target,
  const char *xml,const char *filename,const size_t depth)
{
  LinkedListInfo
    *cache = target->cache;

  ExceptionInfo
    *exception = target->exception;

  char
    keyword[MagickPathExtent],
    *token;

  const char
    *q;

  LogInfo
    *log_info = (LogInfo *) NULL;

  MagickStatusType
    status;

  size_t
    extent;

  /*
    Load the log map file.
  */
  if (xml == (const char *) NULL)
    return(MagickFalse);
  status=MagickTrue;
  token=AcquireString(xml);
  extent=strlen(token)+MagickPathExtent;
  for (q=(const char *) xml; *q != '\0'; )
  {
    LogElementInput
      element;

    LogIncludeInput
      include;

    /*
      Interpret XML.
    */
    (void) GetNextToken(q,&q,extent,token);
    if (*token == '\0')
      break;
    (void) CopyMagickString(keyword,token,MagickPathExtent);
    include.target=target;
    include.filename=filename;
    include.keyword=keyword;
    include.token=token;
    include.extent=extent;
    include.depth=depth;
    if (ReadLogMarkupElement(&include,&q,&status) != MagickFalse)
      continue;
    element.cache=cache;
    element.exception=exception;
    element.keyword=keyword;
    element.token=token;
    element.filename=filename;
    element.extent=extent;
    ParseLogElement(&element,&log_info,&status,&q);
  }
  token=DestroyString(token);
  if (cache == (LinkedListInfo *) NULL)
    return(MagickFalse);
  return((MagickBooleanType) (status != 0));
}
#endif

#endif
