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

  MagickCore private methods of log.c: the event text translators and the log
  file writer, GetModuleBasename to WriteLogFile; included by that file only.
*/
#ifndef MAGICKCORE_LOG_EVENT_PRIVATE_H
#define MAGICKCORE_LOG_EVENT_PRIVATE_H

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

static LogInfo *GetDefaultLogInfo(void)
{
  ExceptionInfo
    *exception;

  LogInfo
    *log_info;

  exception=AcquireExceptionInfo();
  log_info=GetLogInfo("*",exception);
  exception=DestroyExceptionInfo(exception);
  return(log_info);
}

static char *TranslateLogFormat(const char *format,char *text,size_t extent,
  LogEventText *event_text,const char *domain)
{
  char
    *q;

  const char
    *p;

  /*
    Translate event in "human readable" format.
  */
  q=text;
  for (p=format; *p != '\0'; p++)
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
    (*event_text).text=text;
    (*event_text).extent=extent;
    TranslateLogSpecifier(event_text,domain,p,&q);
  }
  *q='\0';
  return(text);
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

  LogInfo
    *log_info;

  size_t
    extent;

  time_t
    seconds;

  LogEventText
    event_text;

  log_info=GetDefaultLogInfo();
  text=AcquireString(event);
  if (log_info == (LogInfo *) NULL)
    return(text);
  seconds=GetMagickTime();
  elapsed_time=GetElapsedTime(&log_info->timer);
  user_time=GetUserTime(&log_info->timer);
  if (log_info->format == (char *) NULL)
    return(text);
  extent=strlen(event)+MagickPathExtent;
  event_text.text=text;
  event_text.extent=extent;
  event_text.elapsed_time=elapsed_time;
  event_text.user_time=user_time;
  event_text.seconds=seconds;
  event_text.event=event;
  event_text.function=function;
  event_text.module=module;
  event_text.line=line;
  if (LocaleCompare(log_info->format,"xml") == 0)
    {
      FormatLogEventXML(&event_text,domain);
      return(text);
    }
  return(TranslateLogFormat(log_info->format,text,extent,&event_text,domain));
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

#endif
