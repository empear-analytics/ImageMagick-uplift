/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%         CCCC   OOO   N   N  FFFFF  IIIII   GGGG  U   U  RRRR   EEEEE        %
%        C      O   O  NN  N  F        I    G      U   U  R   R  E            %
%        C      O   O  N N N  FFF      I    G  GG  U   U  RRRR   EEE          %
%        C      O   O  N  NN  F        I    G   G  U   U  R R    E            %
%         CCCC   OOO   N   N  F      IIIII   GGG    UUU   R  R   EEEEE        %
%                                                                             %
%                      PPPP    AAA   TTTTT  H   H  SSSSS                      %
%                      P   P  A   A    T    H   H  SS                         %
%                      PPPP   AAAAA    T    HHHHH   SSS                       %
%                      P      A   A    T    H   H     SS                      %
%                      P      A   A    T    H   H  SSSSS                      %
%                                                                             %
%                                                                             %
%                      MagickCore Configure Path Method                       %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 2003                                   %
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
#include "MagickCore/memory_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t C o n f i g u r e P a t h s                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetConfigurePaths() returns any Magick configuration paths associated
%  with the specified filename.
%
%  The format of the GetConfigurePaths method is:
%
%      LinkedListInfo *GetConfigurePaths(const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: the configure file name.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport LinkedListInfo *GetConfigurePaths(const char *filename,
  ExceptionInfo *exception)
{
  char
    path[MagickPathExtent];

  LinkedListInfo
    *paths;

  assert(filename != (const char *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  (void) CopyMagickString(path,filename,MagickPathExtent);
  paths=NewLinkedList(0);
  {
    char
      *configure_path;

    /*
      Search $MAGICK_CONFIGURE_PATH.
    */
    configure_path=GetEnvironmentValue("MAGICK_CONFIGURE_PATH");
    if (configure_path != (char *) NULL)
      {
        char
          *p,
          *q;

        for (p=configure_path-1; p != (char *) NULL; )
        {
          (void) CopyMagickString(path,p+1,MagickPathExtent);
          q=strchr(path,DirectoryListSeparator);
          if (q != (char *) NULL)
            *q='\0';
          q=path+strlen(path)-1;
          if ((q >= path) && (*q != *DirectorySeparator))
            (void) ConcatenateMagickString(path,DirectorySeparator,
              MagickPathExtent);
          (void) AppendValueToLinkedList(paths,ConstantString(path));
          p=strchr(p+1,DirectoryListSeparator);
        }
        configure_path=DestroyString(configure_path);
      }
  }
#if defined(MAGICKCORE_INSTALLED_SUPPORT)
#if defined(MAGICKCORE_SHARE_PATH)
  (void) AppendValueToLinkedList(paths,ConstantString(MAGICKCORE_SHARE_PATH));
#endif
#if defined(MAGICKCORE_SHAREARCH_PATH)
  (void) AppendValueToLinkedList(paths,ConstantString(
    MAGICKCORE_SHAREARCH_PATH));
#endif
#if defined(MAGICKCORE_CONFIGURE_PATH)
  (void) AppendValueToLinkedList(paths,ConstantString(
    MAGICKCORE_CONFIGURE_PATH));
#endif
#if defined(MAGICKCORE_DOCUMENTATION_PATH)
  (void) AppendValueToLinkedList(paths,ConstantString(
    MAGICKCORE_DOCUMENTATION_PATH));
#endif
#if defined(MAGICKCORE_WINDOWS_SUPPORT) && !(defined(MAGICKCORE_CONFIGURE_PATH) || defined(MAGICKCORE_SHARE_PATH))
  {
    unsigned char
      *key_value;

    /*
      Locate file via registry key.
    */
    key_value=NTRegistryKeyLookup("ConfigurePath");
    if (key_value != (unsigned char *) NULL)
      {
        (void) FormatLocaleString(path,MagickPathExtent,"%s%s",(char *)
          key_value,DirectorySeparator);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
        key_value=(unsigned char *) RelinquishMagickMemory(key_value);
      }
  }
#endif
#else
  {
    char
      *home;

    /*
      Search under MAGICK_HOME.
    */
    home=GetEnvironmentValue("MAGICK_HOME");
    if (home != (char *) NULL)
      {
#if !defined(MAGICKCORE_POSIX_SUPPORT) || defined( __VMS )
        (void) FormatLocaleString(path,MagickPathExtent,"%s%s",home,
          DirectorySeparator);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
#else
        (void) FormatLocaleString(path,MagickPathExtent,"%s/etc/%s/",home,
          MAGICKCORE_CONFIGURE_RELATIVE_PATH);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
        (void) FormatLocaleString(path,MagickPathExtent,"%s/share/%s/",home,
          MAGICKCORE_SHARE_RELATIVE_PATH);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
        (void) FormatLocaleString(path,MagickPathExtent,"%s",
          MAGICKCORE_SHAREARCH_PATH);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
#endif
        home=DestroyString(home);
      }
    }
  if (*GetClientPath() != '\0')
    {
#if !defined(MAGICKCORE_POSIX_SUPPORT) || defined( __VMS )
      (void) FormatLocaleString(path,MagickPathExtent,"%s%s",GetClientPath(),
        DirectorySeparator);
      (void) AppendValueToLinkedList(paths,ConstantString(path));
#else
      char
        prefix[MagickPathExtent];

      /*
        Search based on executable directory if directory is known.
      */
      (void) CopyMagickString(prefix,GetClientPath(),MagickPathExtent);
      ChopPathComponents(prefix,1);
      (void) FormatLocaleString(path,MagickPathExtent,"%s/etc/%s/",prefix,
        MAGICKCORE_CONFIGURE_RELATIVE_PATH);
      (void) AppendValueToLinkedList(paths,ConstantString(path));
      (void) FormatLocaleString(path,MagickPathExtent,"%s/share/%s/",prefix,
        MAGICKCORE_SHARE_RELATIVE_PATH);
      (void) AppendValueToLinkedList(paths,ConstantString(path));
      (void) FormatLocaleString(path,MagickPathExtent,"%s",
        MAGICKCORE_SHAREARCH_PATH);
      (void) AppendValueToLinkedList(paths,ConstantString(path));
#endif
    }
  /*
    Search current directory.
  */
  (void) AppendValueToLinkedList(paths,ConstantString(""));
#endif
  {
    char
      *home;

    home=GetEnvironmentValue("XDG_CONFIG_HOME");
#if defined(MAGICKCORE_WINDOWS_SUPPORT) || defined(__MINGW32__)
    if (home == (char *) NULL)
      home=GetEnvironmentValue("LOCALAPPDATA");
    if (home == (char *) NULL)
      home=GetEnvironmentValue("APPDATA");
    if (home == (char *) NULL)
      home=GetEnvironmentValue("USERPROFILE");
#endif
    if (home != (char *) NULL)
      {
        /*
          Search $XDG_CONFIG_HOME/ImageMagick.
        */
        (void) FormatLocaleString(path,MagickPathExtent,"%s%sImageMagick%s",
          home,DirectorySeparator,DirectorySeparator);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
        home=DestroyString(home);
      }
    home=GetEnvironmentValue("HOME");
    if (home != (char *) NULL)
      {
        /*
          Search $HOME/.config/ImageMagick.
        */
        (void) FormatLocaleString(path,MagickPathExtent,
          "%s%s.config%sImageMagick%s",home,DirectorySeparator,
          DirectorySeparator,DirectorySeparator);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
        home=DestroyString(home);
      }
  }
#if defined(MAGICKCORE_WINDOWS_SUPPORT)
  {
    char
      module_path[MagickPathExtent];

#if defined(_MAGICKDLL_)
    if ((NTGetModulePath("CORE_RL_MagickCore_.dll",module_path) != MagickFalse) ||
        (NTGetModulePath("CORE_DB_MagickCore_.dll",module_path) != MagickFalse))
      {
        unsigned char
          *key_value;

        /*
          Search module path.
        */
        (void) FormatLocaleString(path,MagickPathExtent,"%s%s",module_path,
          DirectorySeparator);
        key_value=NTRegistryKeyLookup("ConfigurePath");
        if (key_value == (unsigned char *) NULL)
          (void) AppendValueToLinkedList(paths,ConstantString(path));
        else
          key_value=(unsigned char *) RelinquishMagickMemory(key_value);
      }
#endif
    if (NTGetModulePath("Magick.dll",module_path) != MagickFalse)
      {
        /*
          Search PerlMagick module path.
        */
        (void) FormatLocaleString(path,MagickPathExtent,"%s%s",module_path,
          DirectorySeparator);
        (void) AppendValueToLinkedList(paths,ConstantString(path));
        (void) FormatLocaleString(path,MagickPathExtent,"%s%s",module_path,
          "\\inc\\lib\\auto\\Image\\Magick\\");
        (void) AppendValueToLinkedList(paths,ConstantString(path));
      }
  }
#endif
  if (GetNumberOfElementsInLinkedList(paths) == 0)
    (void) ThrowMagickException(exception,GetMagickModule(),ConfigureWarning,
      "no configuration paths found","`%s'",filename);
  return(paths);
}
