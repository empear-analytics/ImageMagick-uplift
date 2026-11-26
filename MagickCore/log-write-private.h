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

  MagickCore private methods of log.c: the log writers and the format helpers,
  WriteLogToSystem to FormatLogFilename; included by that file only.
*/
#ifndef MAGICKCORE_LOG_WRITE_PRIVATE_H
#define MAGICKCORE_LOG_WRITE_PRIVATE_H

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

#endif
