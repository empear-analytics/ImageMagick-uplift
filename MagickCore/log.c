/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                             L       OOO    GGGG                             %
%                             L      O   O  G                                 %
%                             L      O   O  G GG                              %
%                             L      O   O  G   G                             %
%                             LLLLL   OOO    GGG                              %
%                                                                             %
%                                                                             %
%                             MagickCore Log Events                           %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                September 2002                               %
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
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/blob.h"
#include "MagickCore/client.h"
#include "MagickCore/configure.h"
#include "MagickCore/configure-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/linked-list-private.h"
#include "MagickCore/log.h"
#include "MagickCore/log-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/resource-private.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/timer.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"

/*
  Define declarations.
*/
#define LogFilename  "log.xml"

/*
  Typedef declarations.
*/
typedef enum
{
  UndefinedHandler = 0x0000,
  NoHandler = 0x0000,
  ConsoleHandler = 0x0001,
  StdoutHandler = 0x0002,
  StderrHandler = 0x0004,
  FileHandler = 0x0008,
  DebugHandler = 0x0010,
  EventHandler = 0x0020,
  MethodHandler = 0x0040
} LogHandlerType;

typedef struct _EventInfo
{
  char
    *name;

  LogEventType
    event;
} EventInfo;

typedef struct _HandlerInfo
{
  const char
    name[10];

  LogHandlerType
    handler;
} HandlerInfo;

struct _LogInfo
{
  LogEventType
    event_mask;

  LogHandlerType
    handler_mask;

  char
    *path,
    *name,
    *filename,
    *format;

  size_t
    generations;

  FILE
    *file;

  MagickBooleanType
    append,
    stealth;

  MagickSizeType
    limit;

  TimerInfo
    timer;

  MagickLogMethod
    method;

  SemaphoreInfo
    *event_semaphore;

  size_t
    signature;
};

typedef struct _LogMapInfo
{
  const LogEventType
    event_mask;

  const LogHandlerType
    handler_mask;

  const char
    *filename,
    *format;
} LogMapInfo;

typedef struct _LogLoadTarget
{
  LinkedListInfo
    *cache;

  ExceptionInfo
    *exception;
} LogLoadTarget;

typedef struct _LogEventLocation
{
  const char
    *module,
    *function;

  size_t
    line;
} LogEventLocation;

/*
  Static declarations.
*/
static const HandlerInfo
  LogHandlers[32] =
  {
    { "Console", ConsoleHandler },
    { "Debug", DebugHandler },
    { "Event", EventHandler },
    { "File", FileHandler },
    { "None", NoHandler },
    { "Stderr", StderrHandler },
    { "Stdout", StdoutHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler },
    { "", UndefinedHandler }
  };

static const LogMapInfo
  LogMap[] =
  {
    { NoEvents, ConsoleHandler, "Magick-%g.log",
      "%t %r %u %v %d %c[%p]: %m/%f/%l/%d\\n  %e" }
  };

static char
  log_name[MagickPathExtent] = "Magick";

static LinkedListInfo
  *log_cache = (LinkedListInfo *) NULL;

static MagickBooleanType
  event_logging = MagickFalse;

static SemaphoreInfo
  *log_semaphore = (SemaphoreInfo *) NULL;

/*
  Forward declarations.
*/
#if !MAGICKCORE_ZERO_CONFIGURATION_SUPPORT
static LogHandlerType
  ParseLogHandlers(const char *) magick_attribute((__pure__));
#endif

static LogInfo
  *GetLogInfo(const char *,ExceptionInfo *);

static MagickBooleanType
  IsLogCacheInstantiated(ExceptionInfo *) magick_attribute((__pure__));

#if !MAGICKCORE_ZERO_CONFIGURATION_SUPPORT
static MagickBooleanType
  LoadLogCache(const LogLoadTarget *,const char *,const char *,
    const size_t);
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  A c q u i r e L o g C a c h e                                              %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireLogCache() caches one or more log configurations which provides a
%  mapping between log attributes and log name.
%
%  The format of the AcquireLogCache method is:
%
%      LinkedListInfo *AcquireLogCache(const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: the log configuration filename.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static LinkedListInfo *AcquireLogCache(const char *filename,
  ExceptionInfo *exception)
{
  LinkedListInfo
    *cache;

  MagickStatusType
    status;

  ssize_t
    i;

  /*
    Load external log map.
  */
  cache=NewLinkedList(0);
  status=MagickTrue;
#if !MAGICKCORE_ZERO_CONFIGURATION_SUPPORT
  {
    const LogLoadTarget target = { cache, exception };

    const StringInfo
      *option;

    LinkedListInfo
      *options;

    options=GetConfigureOptions(filename,exception);
    option=(const StringInfo *) GetNextValueInLinkedList(options);
    while (option != (const StringInfo *) NULL)
    {
      status&=(MagickStatusType) LoadLogCache(&target,(const char *)
        GetStringInfoDatum(option),GetStringInfoPath(option),0);
      option=(const StringInfo *) GetNextValueInLinkedList(options);
    }
    options=DestroyConfigureOptions(options);
  }
#else
  magick_unreferenced(filename);
#endif
  /*
    Load built-in log map.
  */
  for (i=0; i < (ssize_t) (sizeof(LogMap)/sizeof(*LogMap)); i++)
  {
    LogInfo
      *log_info;

    const LogMapInfo
      *p;

    p=LogMap+i;
    log_info=(LogInfo *) AcquireMagickMemory(sizeof(*log_info));
    if (log_info == (LogInfo *) NULL)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          ResourceLimitError,"MemoryAllocationFailed","`%s'",p->filename);
        continue;
      }
    (void) memset(log_info,0,sizeof(*log_info));
    log_info->path=ConstantString("[built-in]");
    GetTimerInfo((TimerInfo *) &log_info->timer);
    log_info->event_mask=p->event_mask;
    log_info->handler_mask=p->handler_mask;
    log_info->filename=ConstantString(p->filename);
    log_info->format=ConstantString(p->format);
    log_info->signature=MagickCoreSignature;
    status&=(MagickStatusType) AppendValueToLinkedList(cache,log_info);
    if (status == MagickFalse)
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",log_info->name);
  }
  return(cache);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   C l o s e M a g i c k L o g                                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloseMagickLog() closes the Magick log.
%
%  The format of the CloseMagickLog method is:
%
%      CloseMagickLog(void)
%
*/
MagickExport void CloseMagickLog(void)
{
  ExceptionInfo
    *exception;

  LogInfo
    *log_info;

  if (IsEventLogging() == MagickFalse)
    return;
  exception=AcquireExceptionInfo();
  log_info=GetLogInfo("*",exception);
  exception=DestroyExceptionInfo(exception);
  if (log_info == (LogInfo *) NULL)
    return;
  LockSemaphoreInfo(log_semaphore);
  if ((log_info != (LogInfo *) NULL) && (log_info->file != (FILE *) NULL))
    {
      (void) FormatLocaleFile(log_info->file,"</log>\n");
      (void) fclose(log_info->file);
      log_info->file=(FILE *) NULL;
    }
  UnlockSemaphoreInfo(log_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t L o g E v e n t M a s k                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLogEventMask() returns the current log event mask.
%
%  The format of the GetLogEventMask method is:
%
%      LogEventType GetLogEventMask(void)
%
*/
MagickExport LogEventType GetLogEventMask(void)
{
  ExceptionInfo
    *exception;

  LogInfo
    *log_info;

  exception=AcquireExceptionInfo();
  log_info=GetLogInfo("*",exception);
  exception=DestroyExceptionInfo(exception);
  if (log_info == (const LogInfo *) NULL)
    return(NoEvents);
  return(log_info->event_mask);
}

static ElementInfo *FindLogElement(ElementInfo *p,const char *name)
{
  const LogInfo
    *log_info;

  while (p != (ElementInfo *) NULL)
  {
    log_info=(const LogInfo *) p->value;
    if (LocaleCompare(name,log_info->name) == 0)
      break;
    p=p->next;
  }
  return(p);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t L o g I n f o                                                       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLogInfo() searches the log list for the specified name and if found
%  returns attributes for that log.
%
%  The format of the GetLogInfo method is:
%
%      LogInfo *GetLogInfo(const char *name,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o name: the log name.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static LogInfo *GetLogInfo(const char *name,ExceptionInfo *exception)
{
  LogInfo
    *log_info;

  ElementInfo
    *p;

  assert(exception != (ExceptionInfo *) NULL);
  if (IsLogCacheInstantiated(exception) == MagickFalse)
    return((LogInfo *) NULL);
  /*
    Search for log tag.
  */
  log_info=(LogInfo *) NULL;
  LockSemaphoreInfo(log_semaphore);
  p=GetHeadElementInLinkedList(log_cache);
  if ((name == (const char *) NULL) || (LocaleCompare(name,"*") == 0))
    {
      if (p != (ElementInfo *) NULL)
        log_info=(LogInfo *) p->value;
      UnlockSemaphoreInfo(log_semaphore);
      return(log_info);
    }
  p=FindLogElement(p,name);
  if (p == (ElementInfo *) NULL)
    log_info=(LogInfo *) NULL;
  else
    {
      log_info=(LogInfo *) p->value;
      SetHeadElementInLinkedList(log_cache,p);
    }
  UnlockSemaphoreInfo(log_semaphore);
  return(log_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t L o g I n f o L i s t                                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLogInfoList() returns any logs that match the specified pattern.
%
%  The format of the GetLogInfoList function is:
%
%      const LogInfo **GetLogInfoList(const char *pattern,
%        size_t *number_preferences,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_preferences:  This integer returns the number of logs in the list.
%
%    o exception: return any errors or warnings in this structure.
%
*/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int LogInfoCompare(const void *x,const void *y)
{
  const LogInfo
    **p,
    **q;

  p=(const LogInfo **) x,
  q=(const LogInfo **) y;
  if (LocaleCompare((*p)->path,(*q)->path) == 0)
    return(LocaleCompare((*p)->name,(*q)->name));
  return(LocaleCompare((*p)->path,(*q)->path));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport const LogInfo **GetLogInfoList(const char *pattern,
  size_t *number_preferences,ExceptionInfo *exception)
{
  const LogInfo
    **preferences;

  ElementInfo
    *p;

  ssize_t
    i;

  assert(pattern != (char *) NULL);
  assert(number_preferences != (size_t *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  *number_preferences=0;
  if (IsLogCacheInstantiated(exception) == MagickFalse)
    return((const LogInfo **) NULL);
  preferences=(const LogInfo **) AcquireQuantumMemory((size_t)
    GetNumberOfElementsInLinkedList(log_cache)+1UL,sizeof(*preferences));
  if (preferences == (const LogInfo **) NULL)
    return((const LogInfo **) NULL);
  LockSemaphoreInfo(log_semaphore);
  p=GetHeadElementInLinkedList(log_cache);
  for (i=0; p != (ElementInfo *) NULL; )
  {
    const LogInfo
      *log_info;

    log_info=(const LogInfo *) p->value;
    if ((log_info->stealth == MagickFalse) &&
        (GlobExpression(log_info->name,pattern,MagickFalse) != MagickFalse))
      preferences[i++]=log_info;
    p=p->next;
  }
  UnlockSemaphoreInfo(log_semaphore);
  if (i == 0)
    preferences=(const LogInfo **) RelinquishMagickMemory((void*) preferences);
  else
    {
      qsort((void *) preferences,(size_t) i,sizeof(*preferences),LogInfoCompare);
      preferences[i]=(LogInfo *) NULL;
    }
  *number_preferences=(size_t) i;
  return(preferences);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t L o g L i s t                                                       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLogList() returns any logs that match the specified pattern.
%
%  The format of the GetLogList function is:
%
%      char **GetLogList(const char *pattern,size_t *number_preferences,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_preferences:  This integer returns the number of logs in the list.
%
%    o exception: return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int LogCompare(const void *x,const void *y)
{
  const char
    **p,
    **q;

  p=(const char **) x;
  q=(const char **) y;
  return(LocaleCompare(*p,*q));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport char **GetLogList(const char *pattern,size_t *number_preferences,
  ExceptionInfo *exception)
{
  char
    **preferences;

  ElementInfo
    *p;

  ssize_t
    i;

  assert(pattern != (char *) NULL);
  assert(number_preferences != (size_t *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  *number_preferences=0;
  if (IsLogCacheInstantiated(exception) == MagickFalse)
    return((char **) NULL);
  preferences=(char **) AcquireQuantumMemory((size_t)
    GetNumberOfElementsInLinkedList(log_cache)+1UL,sizeof(*preferences));
  if (preferences == (char **) NULL)
    return((char **) NULL);
  LockSemaphoreInfo(log_semaphore);
  p=GetHeadElementInLinkedList(log_cache);
  for (i=0; p != (ElementInfo *) NULL; )
  {
    const LogInfo
      *log_info;

    log_info=(const LogInfo *) p->value;
    if ((log_info->stealth == MagickFalse) &&
        (GlobExpression(log_info->name,pattern,MagickFalse) != MagickFalse))
      preferences[i++]=ConstantString(log_info->name);
    p=p->next;
  }
  UnlockSemaphoreInfo(log_semaphore);
  if (i == 0)
    preferences=(char **) RelinquishMagickMemory(preferences);
  else
    {
      qsort((void *) preferences,(size_t) i,sizeof(*preferences),LogCompare);
      preferences[i]=(char *) NULL;
    }
  *number_preferences=(size_t) i;
  return(preferences);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t L o g N a m e                                                       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLogName() returns the current log name.
%
%  The format of the GetLogName method is:
%
%      const char *GetLogName(void)
%
*/
MagickExport const char *GetLogName(void)
{
  return(log_name);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   I s L o g C a c h e I n s t a n t i a t e d                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsLogCacheInstantiated() determines if the log list is instantiated.  If
%  not, it instantiates the list and returns it.
%
%  The format of the IsLogInstantiated method is:
%
%      MagickBooleanType IsLogCacheInstantiated(ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline void CheckEventLogging(void)
{
  /*
    Are we logging events?
  */
  if (IsLinkedListEmpty(log_cache) != MagickFalse)
    event_logging=MagickFalse;
  else
    {
      ElementInfo
        *p;

      p=GetHeadElementInLinkedList(log_cache);
      event_logging=(p != (ElementInfo *) NULL) &&
        (((LogInfo *) p->value)->event_mask != NoEvents) ?
          MagickTrue: MagickFalse;
    }
}

static MagickBooleanType IsLogCacheInstantiated(ExceptionInfo *exception)
{
  if (log_cache == (LinkedListInfo *) NULL)
    {
      if (log_semaphore == (SemaphoreInfo *) NULL)
        ActivateSemaphoreInfo(&log_semaphore);
      LockSemaphoreInfo(log_semaphore);
      if (log_cache == (LinkedListInfo *) NULL)
        {
          log_cache=AcquireLogCache(LogFilename,exception);
          CheckEventLogging();
        }
      UnlockSemaphoreInfo(log_semaphore);
    }
  return(log_cache != (LinkedListInfo *) NULL ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  I s E v e n t L o g g i n g                                                %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsEventLogging() returns MagickTrue if debug of events is enabled otherwise
%  MagickFalse.
%
%  The format of the IsEventLogging method is:
%
%      MagickBooleanType IsEventLogging(void)
%
*/
MagickExport MagickBooleanType IsEventLogging(void)
{
  return(event_logging);
}

static void ListLogHeader(FILE *file,const LogInfo *log_info)
{
  ssize_t
    j;

  if (log_info->path != (char *) NULL)
    (void) FormatLocaleFile(file,"\nPath: %s\nHandler: ",log_info->path);
  for (j=0; j < (ssize_t) (8*sizeof(LogHandlerType)); j++)
  {
    size_t
      mask;

    if (*LogHandlers[j].name == '\0')
      break;
    mask=1;
    mask<<=j;
    if (((size_t) log_info->handler_mask & mask) != 0)
      (void) FormatLocaleFile(file,"%s ",LogHandlers[j].name);
  }
  (void) FormatLocaleFile(file,"\n\n");
}

static void ListLogEntry(FILE *file,const LogInfo *log_info)
{
  char
    limit[MagickPathExtent];

  (void) FormatLocaleFile(file,
    "Generations  Limit  Filename\n");
  (void) FormatLocaleFile(file,"---------------------------------------------"
    "----------------------------------\n");
  (void) FormatLocaleFile(file,"%g      ",(double) log_info->generations);
  (void) CopyMagickString(limit,"unlimited",MagickPathExtent);
  if (log_info->limit > 0)
    (void) FormatMagickSize(log_info->limit,MagickTrue,"B",MagickFormatExtent,
      limit);
  (void) FormatLocaleFile(file,"  %9s",limit);
  if (log_info->filename != (char *) NULL)
    (void) FormatLocaleFile(file,"  %s",log_info->filename);
  (void) FormatLocaleFile(file,"\n");
  if (log_info->format != (char *) NULL)
    (void) FormatLocaleFile(file,"  Format: %s\n",log_info->format);
  (void) FormatLocaleFile(file,"\n");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  L i s t L o g I n f o                                                      %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListLogInfo() lists the log info to a file.
%
%  The format of the ListLogInfo method is:
%
%      MagickBooleanType ListLogInfo(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to a FILE.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ListLogInfo(FILE *file,ExceptionInfo *exception)
{
#define MegabytesToBytes(value) ((MagickSizeType) (value)*1024*1024)

  const char
    *path;

  const LogInfo
    **log_info;

  ssize_t
    i;

  size_t
    number_aliases;

  if (file == (const FILE *) NULL)
    file=stdout;
  log_info=GetLogInfoList("*",&number_aliases,exception);
  if (log_info == (const LogInfo **) NULL)
    return(MagickFalse);
  path=(const char *) NULL;
  for (i=0; i < (ssize_t) number_aliases; i++)
  {
    if (log_info[i]->stealth != MagickFalse)
      continue;
    if ((path == (const char *) NULL) ||
        (LocaleCompare(path,log_info[i]->path) != 0))
      ListLogHeader(file,log_info[i]);
    ListLogEntry(file,log_info[i]);
    path=log_info[i]->path;
  }
  (void) fflush(file);
  log_info=(const LogInfo **) RelinquishMagickMemory((void *) log_info);
  return(MagickTrue);
}

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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   L o g C o m p o n e n t G e n e s i s                                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LogComponentGenesis() instantiates the log component.
%
%  The format of the LogComponentGenesis method is:
%
%      MagickBooleanType LogComponentGenesis(void)
%
*/
MagickPrivate MagickBooleanType LogComponentGenesis(void)
{
  ExceptionInfo
    *exception;

  if (log_semaphore == (SemaphoreInfo *) NULL)
    log_semaphore=AcquireSemaphoreInfo();
  exception=AcquireExceptionInfo();
  (void) GetLogInfo("*",exception);
  exception=DestroyExceptionInfo(exception);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   L o g C o m p o n e n t T e r m i n u s                                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LogComponentTerminus() destroys the logging component.
%
%  The format of the LogComponentTerminus method is:
%
%      LogComponentTerminus(void)
%
*/

static void *DestroyLogElement(void *log_info)
{
  LogInfo
    *p;

  p=(LogInfo *) log_info;
  if (p->file != (FILE *) NULL)
    {
      (void) FormatLocaleFile(p->file,"</log>\n");
      (void) fclose(p->file);
      p->file=(FILE *) NULL;
    }
  if (p->format != (char *) NULL)
    p->format=DestroyString(p->format);
  if (p->path != (char *) NULL)
    p->path=DestroyString(p->path);
  if (p->filename != (char *) NULL)
    p->filename=DestroyString(p->filename);
  if (p->event_semaphore != (SemaphoreInfo *) NULL)
    RelinquishSemaphoreInfo(&p->event_semaphore);
  p=(LogInfo *) RelinquishMagickMemory(p);
  return((void *) NULL);
}

MagickPrivate void LogComponentTerminus(void)
{
  if (log_semaphore == (SemaphoreInfo *) NULL)
    ActivateSemaphoreInfo(&log_semaphore);
  LockSemaphoreInfo(log_semaphore);
  if (log_cache != (LinkedListInfo *) NULL)
    log_cache=DestroyLinkedList(log_cache,DestroyLogElement);
  event_logging=MagickFalse;
  UnlockSemaphoreInfo(log_semaphore);
  RelinquishSemaphoreInfo(&log_semaphore);
}

static void WriteLogToSystem(const LogInfo *log_info,const char *text)
{
  if ((log_info->handler_mask & ConsoleHandler) != 0)
    {
      (void) FormatLocaleFile(stderr,"%s\n",text);
      (void) fflush(stderr);
    }
  if ((log_info->handler_mask & DebugHandler) != 0)
    {
#if defined(MAGICKCORE_WINDOWS_SUPPORT)
      OutputDebugString(text);
      OutputDebugString("\n");
#endif
    }
  if ((log_info->handler_mask & EventHandler) != 0)
    {
#if defined(MAGICKCORE_WINDOWS_SUPPORT)
      (void) NTReportEvent(text,MagickFalse);
#endif
    }
}

static void WriteLogToStreams(const LogInfo *log_info,const LogEventType type,
  const char *text)
{
  if ((log_info->handler_mask & MethodHandler) != 0)
    {
      if (log_info->method != (MagickLogMethod) NULL)
        log_info->method(type,text);
    }
  if ((log_info->handler_mask & StdoutHandler) != 0)
    {
      (void) FormatLocaleFile(stdout,"%s\n",text);
      (void) fflush(stdout);
    }
  if ((log_info->handler_mask & StderrHandler) != 0)
    {
      (void) FormatLocaleFile(stderr,"%s\n",text);
      (void) fflush(stderr);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   L o g M a g i c k E v e n t                                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LogMagickEvent() logs an event as determined by the log configuration file.
%  If an error occurs, MagickFalse is returned otherwise MagickTrue.
%
%  The format of the LogMagickEvent method is:
%
%      MagickBooleanType LogMagickEvent(const LogEventType type,
%        const char *module,const char *function,const size_t line,
%        const char *format,...)
%
%  A description of each parameter follows:
%
%    o type: the event type.
%
%    o filename: the source module filename.
%
%    o function: the function name.
%
%    o line: the line number of the source module.
%
%    o format: the output format.
%
*/

static ptrdiff_t FormatLogSpecifier(char *q,const size_t extent,
  const char specifier)
{
  switch (specifier)
  {
    case 'c':
      return((ptrdiff_t) CopyMagickString(q,GetClientName(),extent));
    case 'n':
      return((ptrdiff_t) CopyMagickString(q,GetLogName(),extent));
    case 'p':
      return((ptrdiff_t) FormatLocaleString(q,extent,"%.17g",(double)
        getpid()));
    case 'v':
      return((ptrdiff_t) CopyMagickString(q,MagickLibVersionText,extent));
    case '%':
    {
      *q=specifier;
      return(1);
    }
    default:
    {
      q[0]='%';
      q[1]=specifier;
      return(2);
    }
  }
}

static char *FormatLogGeneration(const LogInfo *log_info,
  const ssize_t generation,char *q)
{
  if (log_info->generations == 0)
    {
      (void) CopyMagickString(q,"0",MagickPathExtent);
      q++;
      return(q);
    }
  q+=(ptrdiff_t) FormatLocaleString(q,MagickPathExtent,"%.17g",(double)
    (generation % log_info->generations));
  return(q);
}

static void FormatLogFilename(const LogInfo *log_info,const ssize_t generation,
  char *filename)
{
  char
    *q;

  const char
    *p;

  /*
    Translate event in "human readable" format.
  */
  assert(log_info != (LogInfo *) NULL);
  q=filename;
  for (p=log_info->filename; *p != '\0'; p++)
  {
    /*
      The format of the filename is defined by embedding special format
      characters:

        %c   client name
        %n   log name
        %p   process id
        %v   version
        %%   percent sign
    */
    if (*p != '%')
      {
        *q++=(*p);
        continue;
      }
    p++;
    if (*p == '\0')
      break;
    switch (*p)
    {
      case '\0':
      {
        p--;
        break;
      }
      case 'g':
      {
        q=FormatLogGeneration(log_info,generation,q);
        break;
      }
      default:
      {
        q+=FormatLogSpecifier(q,MagickPathExtent,*p);
        break;
      }
    }
  }
  *q='\0';
}

static const char *GetModuleBasename(const char *module)
{
  const char
    *r;

  for (r=module+strlen(module)-1; r > module; r--)
    if (*r == *DirectorySeparator)
      {
        r++;
        break;
      }
  return(r);
}

static char GetLogEscape(const char *p)
{
  if ((*p == '\\') && (*(p+1) == 'r'))
    return('\r');
  if ((*p == '\\') && (*(p+1) == 'n'))
    return('\n');
  return('\0');
}

static MagickBooleanType GrowLogEventText(char **text,char **q,
  size_t *extent)
{
  if ((size_t) ((*q)-(*text)+MagickPathExtent) >= (*extent))
    {
      (*extent)<<=1;
      (*text)=(char *) ResizeQuantumMemory((*text),(*extent),sizeof(**text));
      if ((*text) == (char *) NULL)
        return(MagickFalse);
      (*q)=(*text)+strlen((*text));
    }
  return(MagickTrue);
}

typedef struct _LogEventText
{
  char
    *text;

  size_t
    extent;

  double
    elapsed_time,
    user_time;

  time_t
    seconds;

  const char
    *event,
    *function,
    *module;

  size_t
    line;
} LogEventText;

static void TranslateLogTimeSpecifier(const LogEventText *event_text,
  const char *p,char **q)
{
  char
    *const text = event_text->text;

  const size_t
    extent = event_text->extent;

  const double
    elapsed_time = event_text->elapsed_time,
    user_time = event_text->user_time;

  const time_t
    seconds = event_text->seconds;

  switch (*p)
  {
    case 'r':
    {
      (*q)+=(ptrdiff_t) FormatLocaleString((*q),extent-((*q)-text),
        "%lu:%02lu.%03lu",(unsigned long) (elapsed_time/60.0),(unsigned long)
        floor(fmod(elapsed_time,60.0)),(unsigned long) (1000.0*(elapsed_time-
        floor(elapsed_time))+0.5));
      break;
    }
    case 't':
    {
      (*q)+=(ptrdiff_t) FormatMagickTime(seconds,extent-((*q)-text),(*q));
      break;
    }
    case 'u':
    {
      (*q)+=(ptrdiff_t) FormatLocaleString((*q),extent-((*q)-text),"%0.3fu",
        user_time);
      break;
    }
    default:
    {
      (*q)+=FormatLogSpecifier((*q),extent-((*q)-text),*p);
      break;
    }
  }
}

static void TranslateLogLocationSpecifier(const LogEventText *event_text,
  const char *p,char **q)
{
  char
    *const text = event_text->text;

  const size_t
    extent = event_text->extent;

  const char
    *const event = event_text->event,
    *const function = event_text->function,
    *const module = event_text->module;

  const size_t
    line = event_text->line;

  switch (*p)
  {
    case 'e':
    {
      (*q)+=(ptrdiff_t) CopyMagickString((*q),event,extent-((*q)-text));
      break;
    }
    case 'f':
    {
      (*q)+=(ptrdiff_t) CopyMagickString((*q),function,extent-((*q)-text));
      break;
    }
    case 'i':
    {
      (*q)+=(ptrdiff_t) FormatLocaleString((*q),extent-((*q)-text),"%.17g",
        (double) GetMagickThreadSignature());
      break;
    }
    case 'l':
    {
      (*q)+=(ptrdiff_t) FormatLocaleString((*q),extent-((*q)-text),"%.17g",
        (double) line);
      break;
    }
    case 'm':
    {
      (*q)+=(ptrdiff_t) CopyMagickString((*q),GetModuleBasename(module),
        extent-((*q)-text));
      break;
    }
    default:
    {
      TranslateLogTimeSpecifier(event_text,p,q);
      break;
    }
  }
}

static void TranslateLogSpecifier(const LogEventText *event_text,
  const char *domain,const char *p,char **q)
{
  char
    *const text = event_text->text;

  const size_t
    extent = event_text->extent;

  switch (*p)
  {
    case 'd':
    {
      (*q)+=(ptrdiff_t) CopyMagickString((*q),domain,extent-((*q)-text));
      break;
    }
    default:
    {
      TranslateLogLocationSpecifier(event_text,p,q);
      break;
    }
  }
}

static void FormatLogEventXML(const LogEventText *event_text,
  const char *domain)
{
  char
    *const text = event_text->text;

  const size_t
    extent = event_text->extent;

  const double
    elapsed_time = event_text->elapsed_time,
    user_time = event_text->user_time;

  const time_t
    seconds = event_text->seconds;

  const char
    *const event = event_text->event,
    *const function = event_text->function,
    *const module = event_text->module;

  const size_t
    line = event_text->line;

  char
    timestamp[MagickTimeExtent];

  /*
    Translate event in "XML" format.
  */
  (void) FormatMagickTime(seconds,sizeof(timestamp),timestamp);
  (void) FormatLocaleString(text,extent,
    "<entry>\n"
    "  <timestamp>%s</timestamp>\n"
    "  <elapsed-time>%lu:%02lu.%06lu</elapsed-time>\n"
    "  <user-time>%0.3f</user-time>\n"
    "  <process-id>%.17g</process-id>\n"
    "  <thread-id>%.17g</thread-id>\n"
    "  <module>%s</module>\n"
    "  <function>%s</function>\n"
    "  <line>%.17g</line>\n"
    "  <domain>%s</domain>\n"
    "  <event>%s</event>\n"
    "</entry>",timestamp,(unsigned long) (elapsed_time/60.0),
    (unsigned long) floor(fmod(elapsed_time,60.0)),(unsigned long)
    (1000000.0*(elapsed_time-floor(elapsed_time))+0.5),user_time,
    (double) getpid(),(double) GetMagickThreadSignature(),module,function,
    (double) line,domain,event);
}

static MagickBooleanType CopyLogFormatLiteral(const char **p,char **q)
{
  char
    escape;

  escape=GetLogEscape((*p));
  if (escape != '\0')
    {
      *(*q)++=escape;
      (*p)++;
      return(MagickTrue);
    }
  if (*(*p) != '%')
    {
      *(*q)++=(*(*p));
      return(MagickTrue);
    }
  return(MagickFalse);
}

static char *TranslateEvent(const LogEventLocation *location,
  const char *domain,const char *event)
{
  const char
    *module = location->module,
    *function = location->function;

  const size_t
    line = location->line;

  char
    *text;

  double
    elapsed_time,
    user_time;

  ExceptionInfo
    *exception;

  LogInfo
    *log_info;

  char
    *q;

  const char
    *p;

  size_t
    extent;

  time_t
    seconds;

  LogEventText
    event_text;

  exception=AcquireExceptionInfo();
  log_info=GetLogInfo("*",exception);
  exception=DestroyExceptionInfo(exception);
  text=AcquireString(event);
  if (log_info == (LogInfo *) NULL)
    return(text);
  seconds=GetMagickTime();
  elapsed_time=GetElapsedTime(&log_info->timer);
  user_time=GetUserTime(&log_info->timer);
  if (log_info->format == (char *) NULL)
    return(text);
  extent=strlen(event)+MagickPathExtent;
  if (LocaleCompare(log_info->format,"xml") == 0)
    {
      event_text.text=text;
      event_text.extent=extent;
      event_text.elapsed_time=elapsed_time;
      event_text.user_time=user_time;
      event_text.seconds=seconds;
      event_text.event=event;
      event_text.function=function;
      event_text.module=module;
      event_text.line=line;
      FormatLogEventXML(&event_text,domain);
      return(text);
    }
  /*
    Translate event in "human readable" format.
  */
  q=text;
  for (p=log_info->format; *p != '\0'; p++)
  {
    *q='\0';
    if (GrowLogEventText(&text,&q,&extent) == MagickFalse)
      return((char *) NULL);
    /*
      The format of the log is defined by embedding special format characters:

        %c   client name
        %d   domain
        %e   event
        %f   function
        %i   thread id
        %l   line
        %m   module
        %n   log name
        %p   process id
        %r   real CPU time
        %t   wall clock time
        %u   user CPU time
        %v   version
        %%   percent sign
        \n   newline
        \r   carriage return
    */
    if (CopyLogFormatLiteral(&p,&q) != MagickFalse)
      continue;
    p++;
    if (*p == '\0')
      break;
    event_text.text=text;
    event_text.extent=extent;
    event_text.elapsed_time=elapsed_time;
    event_text.user_time=user_time;
    event_text.seconds=seconds;
    event_text.event=event;
    event_text.function=function;
    event_text.module=module;
    event_text.line=line;
    TranslateLogSpecifier(&event_text,domain,p,&q);
  }
  *q='\0';
  return(text);
}

static void RotateLogFiles(LogInfo *log_info)
{
  ssize_t
    i;

  /*
    Close log.
  */
  (void) FormatLocaleFile(log_info->file,"</log>\n");
  (void) FormatLocaleFile(log_info->file,"</logEntries>\n");
  (void) fclose(log_info->file);
  for (i=(ssize_t) log_info->generations-1; i > 0; i--)
  {
    char
      new_filename[MagickPathExtent],
      old_filename[MagickPathExtent];

    /*
      Rotate logs.
    */
    FormatLogFilename(log_info,i-1,old_filename);
    FormatLogFilename(log_info,i,new_filename);
    (void) rename_utf8(old_filename,new_filename);
  }
  log_info->file=(FILE *) NULL;
}

static MagickBooleanType OpenLogFile(LogInfo *log_info)
{
  char
    log_filename[MagickPathExtent];

  FormatLogFilename(log_info,0,log_filename);
  log_info->append=IsPathAccessible(log_filename);
  log_info->file=fopen_utf8(log_filename,"ab");
  if (log_info->file == (FILE *) NULL)
    {
      UnlockSemaphoreInfo(log_info->event_semaphore);
      return(MagickFalse);
    }
  if (log_info->append == MagickFalse)
    {
      (void) FormatLocaleFile(log_info->file,"<?xml version=\"1.0\" "
        "encoding=\"UTF-8\" standalone=\"yes\"?>\n");
      (void) FormatLocaleFile(log_info->file,"<logEntries>\n");
    }
  (void) FormatLocaleFile(log_info->file,"<log>\n");
  return(MagickTrue);
}

static MagickBooleanType WriteLogFile(LogInfo *log_info,const char *text)
{
  if ((log_info->handler_mask & FileHandler) != 0)
    {
      struct stat
        file_info;

      file_info.st_size=0;
      if (log_info->file != (FILE *) NULL)
        (void) fstat(fileno(log_info->file),&file_info);
      if (file_info.st_size > (MagickOffsetType) log_info->limit)
        RotateLogFiles(log_info);
      if ((log_info->file == (FILE *) NULL) &&
          (OpenLogFile(log_info) == MagickFalse))
        return(MagickFalse);
      (void) FormatLocaleFile(log_info->file,"  <event>%s</event>\n",text);
      (void) fflush(log_info->file);
    }
  return(MagickTrue);
}

MagickExport MagickBooleanType LogMagickEventList(const LogEventType type,
  const char *module,const char *function,const size_t line,const char *format,
  va_list operands)
{
  const LogEventLocation location = { module, function, line };

  char
    event[MagickPathExtent],
    *text;

  const char
    *domain;

  ExceptionInfo
    *exception;

  int
    n;

  LogInfo
    *log_info;

  exception=AcquireExceptionInfo();
  log_info=(LogInfo *) GetLogInfo("*",exception);
  exception=DestroyExceptionInfo(exception);
  if (log_info == (LogInfo *) NULL)
    return(MagickFalse);
  if (log_info->event_semaphore == (SemaphoreInfo *) NULL)
    ActivateSemaphoreInfo(&log_info->event_semaphore);
  LockSemaphoreInfo(log_info->event_semaphore);
  if ((log_info->event_mask & type) == 0)
    {
      UnlockSemaphoreInfo(log_info->event_semaphore);
      return(MagickTrue);
    }
  domain=CommandOptionToMnemonic(MagickLogEventOptions,type);
  n=vsnprintf(event,MagickPathExtent,format,operands);
  if (n < 0)
    event[MagickPathExtent-1]='\0';
  text=TranslateEvent(&location,domain,event);
  if (text == (char *) NULL)
    {
      (void) ContinueTimer((TimerInfo *) &log_info->timer);
      UnlockSemaphoreInfo(log_info->event_semaphore);
      return(MagickFalse);
    }
  WriteLogToSystem(log_info,text);
  if (WriteLogFile(log_info,text) == MagickFalse)
    return(MagickFalse);
  WriteLogToStreams(log_info,type,text);
  text=(char  *) RelinquishMagickMemory(text);
  (void) ContinueTimer((TimerInfo *) &log_info->timer);
  UnlockSemaphoreInfo(log_info->event_semaphore);
  return(MagickTrue);
}

MagickExport MagickBooleanType LogMagickEvent(const LogEventType type,
  const char *module,const char *function,const size_t line,
  const char *format,...)
{
  va_list
    operands;

  MagickBooleanType
    status;

  if (IsEventLogging() == MagickFalse)
    return(MagickFalse);
  va_start(operands,format);
  status=LogMagickEventList(type,module,function,line,format,operands);
  va_end(operands);
  return(status);
}

#if !MAGICKCORE_ZERO_CONFIGURATION_SUPPORT
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   P a r s e L o g H a n d l e r s                                           %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseLogHandlers() parses a string defining which handlers takes a log
%  message and exports them.
%
%  The format of the ParseLogHandlers method is:
%
%      LogHandlerType ParseLogHandlers(const char *handlers)
%
%  A description of each parameter follows:
%
%    o handlers: one or more handlers separated by commas.
%
*/
static inline MagickBooleanType IsHandlerSeparator(const char *p)
{
  return(((*p != '\0') && ((isspace((int) ((unsigned char) *p)) != 0) ||
    (*p == ','))) ? MagickTrue : MagickFalse);
}

static ssize_t FindLogHandler(const char *p)
{
  ssize_t
    i;

  size_t
    length;

  for (i=0; *LogHandlers[i].name != '\0'; i++)
  {
    length=strlen(LogHandlers[i].name);
    if (LocaleNCompare(p,LogHandlers[i].name,length) == 0)
      break;
  }
  return(i);
}

static LogHandlerType ParseLogHandlers(const char *handlers)
{
  LogHandlerType
    handler_mask;

  const char
    *p;

  ssize_t
    i;

  handler_mask=NoHandler;
  for (p=handlers; p != (char *) NULL; p=strchr(p,','))
  {
    while (IsHandlerSeparator(p) != MagickFalse)
      p++;
    i=FindLogHandler(p);
    if (*LogHandlers[i].name == '\0')
      return(UndefinedHandler);
    handler_mask=(LogHandlerType) (handler_mask | LogHandlers[i].handler);
  }
  return(handler_mask);
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   S e t L o g E v e n t M a s k                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetLogEventMask() accepts a list that determines which events to log.  All
%  other events are ignored.  By default, no debug is enabled.  This method
%  returns the previous log event mask.
%
%  The format of the SetLogEventMask method is:
%
%      LogEventType SetLogEventMask(const char *events)
%
%  A description of each parameter follows:
%
%    o events: log these events.
%
*/
MagickExport LogEventType SetLogEventMask(const char *events)
{
  ExceptionInfo
    *exception;

  LogEventType
    event_mask;

  LogInfo
    *log_info;

  ssize_t
    option;

  event_mask=UndefinedEvents;
  exception=AcquireExceptionInfo();
  log_info=GetLogInfo("*",exception);
  exception=DestroyExceptionInfo(exception);
  if (log_info == (LogInfo *) NULL)
    return(event_mask);
  option=ParseCommandOption(MagickLogEventOptions,MagickTrue,events);
  LockSemaphoreInfo(log_semaphore);
  event_mask=log_info->event_mask;
  if (option == -1)
    log_info->event_mask=UndefinedEvents;
  else
    log_info->event_mask=(LogEventType) option;
  CheckEventLogging();
  UnlockSemaphoreInfo(log_semaphore);
  return(event_mask);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   S e t L o g F o r m a t                                                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetLogFormat() sets the format for the "human readable" log record.
%
%  The format of the LogMagickFormat method is:
%
%      SetLogFormat(const char *format)
%
%  A description of each parameter follows:
%
%    o format: the log record format.
%
*/
MagickExport void SetLogFormat(const char *format)
{
  LogInfo
    *log_info;

  ExceptionInfo
    *exception;

  exception=AcquireExceptionInfo();
  log_info=(LogInfo *) GetLogInfo("*",exception);
  exception=DestroyExceptionInfo(exception);
  if (log_info == (LogInfo *) NULL)
    return;
  LockSemaphoreInfo(log_semaphore);
  if (log_info->format != (char *) NULL)
    log_info->format=DestroyString(log_info->format);
  log_info->format=ConstantString(format);
  UnlockSemaphoreInfo(log_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   S e t L o g M e t h o d                                                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetLogMethod() sets the method that will be called when an event is logged.
%
%  The format of the SetLogMethod method is:
%
%      void SetLogMethod(MagickLogMethod method)
%
%  A description of each parameter follows:
%
%    o method: pointer to a method that will be called when LogMagickEvent is
%      being called.
%
*/
MagickExport void SetLogMethod(MagickLogMethod method)
{
  ExceptionInfo
    *exception;

  LogInfo
    *log_info;

  exception=AcquireExceptionInfo();
  log_info=(LogInfo *) GetLogInfo("*",exception);
  exception=DestroyExceptionInfo(exception);
  if (log_info == (LogInfo *) NULL)
    return;
  LockSemaphoreInfo(log_semaphore);
  log_info=(LogInfo *) GetValueFromLinkedList(log_cache,0);
  log_info->handler_mask=(LogHandlerType) (log_info->handler_mask |
    MethodHandler);
  log_info->method=method;
  UnlockSemaphoreInfo(log_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   S e t L o g N a m e                                                       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetLogName() sets the log name and returns it.
%
%  The format of the SetLogName method is:
%
%      const char *SetLogName(const char *name)
%
%  A description of each parameter follows:
%
%    o log_name: SetLogName() returns the current client name.
%
%    o name: Specifies the new client name.
%
*/
MagickExport const char *SetLogName(const char *name)
{
  if ((name != (char *) NULL) && (*name != '\0'))
    (void) CopyMagickString(log_name,name,MagickPathExtent);
  return(log_name);
}
