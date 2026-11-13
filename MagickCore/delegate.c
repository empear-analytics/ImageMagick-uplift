/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%           DDDD   EEEEE  L      EEEEE   GGGG   AAA   TTTTT  EEEEE            %
%           D   D  E      L      E      G      A   A    T    E                %
%           D   D  EEE    L      EEE    G  GG  AAAAA    T    EEE              %
%           D   D  E      L      E      G   G  A   A    T    E                %
%           DDDD   EEEEE  LLLLL  EEEEE   GGG   A   A    T    EEEEE            %
%                                                                             %
%                                                                             %
%             MagickCore Methods to Read/Write/Invoke Delegates               %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                               October 1998                                  %
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
%  The Delegates methods associate a set of commands with a particular
%  image format.  ImageMagick uses delegates for formats it does not handle
%  directly.
%
%  Thanks to Bob Friesenhahn for the initial inspiration and design of the
%  delegates methods.
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/client.h"
#include "MagickCore/configure.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/delegate-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/linked-list-private.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/policy.h"
#include "MagickCore/policy-private.h"
#include "MagickCore/property.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/signature.h"
#include "MagickCore/string_.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"

/*
  Define declarations.
*/
#if defined(__APPLE__)
  #include "TargetConditionals.h"
  #if TARGET_OS_IOS || TARGET_OS_WATCH || TARGET_OS_TV
    #define system(s) ((s)==NULL ? 0 : -1)
  #endif // end iOS
#elif defined(__ANDROID__)
  #define system(s) ((s)==NULL ? 0 : -1)
#endif
#define DelegateFilename  "delegates.xml"
#if defined(MAGICKCORE_WINDOWS_SUPPORT)
  #define DELEGATE_ESC "&quot;"
#else
  #define DELEGATE_ESC "&apos;"
#endif

/*
  Declare delegate map.
*/
static const char
  *DelegateMap = (const char *)
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<delegatemap>"
    "  <delegate decode=\"bpg\" command=\"" DELEGATE_ESC "bpgdec" DELEGATE_ESC " -b 16 -o " DELEGATE_ESC "%o" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"png\" encode=\"bpg\" command=\"" DELEGATE_ESC "bpgenc" DELEGATE_ESC " -b 12 -q %~ -o " DELEGATE_ESC "%o" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"browse\" stealth=\"True\" spawn=\"True\" command=\"" DELEGATE_ESC "xdg-open" DELEGATE_ESC " https://imagemagick.org/; rm " DELEGATE_ESC "%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"cdr\" command=\"" DELEGATE_ESC "uniconvertor" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC " " DELEGATE_ESC "%o.svg" DELEGATE_ESC "; mv " DELEGATE_ESC "%o.svg" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"cgm\" command=\"" DELEGATE_ESC "uniconvertor" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC " " DELEGATE_ESC "%o.svg" DELEGATE_ESC "; mv " DELEGATE_ESC "%o.svg" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"http:decode\" command=\"" DELEGATE_ESC "curl" DELEGATE_ESC " -s -L -o " DELEGATE_ESC "%u.dat" DELEGATE_ESC " " DELEGATE_ESC "http:%M" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"https:decode\" command=\"" DELEGATE_ESC "curl" DELEGATE_ESC " -s -L -o " DELEGATE_ESC "%u.dat" DELEGATE_ESC " " DELEGATE_ESC "https:%M" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"doc\" command=\"" DELEGATE_ESC "soffice" DELEGATE_ESC " --convert-to pdf -outdir `dirname " DELEGATE_ESC "%i" DELEGATE_ESC "` " DELEGATE_ESC "%i" DELEGATE_ESC " 2&gt; " DELEGATE_ESC "%u" DELEGATE_ESC "; mv " DELEGATE_ESC "%i.pdf" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"docx\" command=\"" DELEGATE_ESC "soffice" DELEGATE_ESC " --convert-to pdf -outdir `dirname " DELEGATE_ESC "%i" DELEGATE_ESC "` " DELEGATE_ESC "%i" DELEGATE_ESC " 2&gt; " DELEGATE_ESC "%u" DELEGATE_ESC "; mv " DELEGATE_ESC "%i.pdf" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"dng:decode\" command=\"" DELEGATE_ESC "ufraw-batch" DELEGATE_ESC " --silent --create-id=also --out-type=png --out-depth=16 " DELEGATE_ESC "--output=%u.png" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"dot\" command=\"" DELEGATE_ESC "dot" DELEGATE_ESC " -Tsvg " DELEGATE_ESC "%i" DELEGATE_ESC " -o " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"dvi\" command=\"" DELEGATE_ESC "dvips" DELEGATE_ESC " -sstdout=%%stderr -o " DELEGATE_ESC "%o" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"dxf\" command=\"" DELEGATE_ESC "uniconvertor" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC " " DELEGATE_ESC "%o.svg" DELEGATE_ESC "; mv " DELEGATE_ESC "%o.svg" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"edit\" stealth=\"True\" command=\"" DELEGATE_ESC "xterm" DELEGATE_ESC " -title " DELEGATE_ESC "Edit Image Comment" DELEGATE_ESC " -e vi " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"eps\" encode=\"pdf\" mode=\"bi\" command=\"" DELEGATE_ESC "gs" DELEGATE_ESC " -sstdout=%%stderr -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 " DELEGATE_ESC "-sDEVICE=pdfwrite" DELEGATE_ESC " " DELEGATE_ESC "-sOutputFile=%o" DELEGATE_ESC " " DELEGATE_ESC "-f%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"eps\" encode=\"ps\" mode=\"bi\" command=\"" DELEGATE_ESC "gs" DELEGATE_ESC " -sstdout=%%stderr -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 " DELEGATE_ESC "-sDEVICE=ps2write" DELEGATE_ESC " " DELEGATE_ESC "-sOutputFile=%o" DELEGATE_ESC " " DELEGATE_ESC "-f%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"fig\" command=\"" DELEGATE_ESC "uniconvertor" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC " " DELEGATE_ESC "%o.svg" DELEGATE_ESC "; mv " DELEGATE_ESC "%o.svg" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"hpg\" command=\"" DELEGATE_ESC "hp2xx" DELEGATE_ESC " -sstdout=%%stderr -m eps -f `basename " DELEGATE_ESC "%o" DELEGATE_ESC "` " DELEGATE_ESC "%i" DELEGATE_ESC ";     mv -f `basename " DELEGATE_ESC "%o" DELEGATE_ESC "` " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"hpgl\" command=\"" DELEGATE_ESC "hp2xx" DELEGATE_ESC " -sstdout=%%stderr -m eps -f `basename " DELEGATE_ESC "%o" DELEGATE_ESC "` " DELEGATE_ESC "%i" DELEGATE_ESC ";     mv -f `basename " DELEGATE_ESC "%o" DELEGATE_ESC "` " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"htm\" command=\"" DELEGATE_ESC "html2ps" DELEGATE_ESC " -U -o " DELEGATE_ESC "%o" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"html\" command=\"" DELEGATE_ESC "html2ps" DELEGATE_ESC " -U -o " DELEGATE_ESC "%o" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"ilbm\" command=\"" DELEGATE_ESC "ilbmtoppm" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC " &gt; " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"jpg\" encode=\"lep\" mode=\"encode\" command=\"" DELEGATE_ESC "lepton" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"jxr\" command=\"mv " DELEGATE_ESC "%i" DELEGATE_ESC " " DELEGATE_ESC "%i.jxr" DELEGATE_ESC "; " DELEGATE_ESC "JxrDecApp" DELEGATE_ESC " -i " DELEGATE_ESC "%i.jxr" DELEGATE_ESC " -o " DELEGATE_ESC "%o.tiff" DELEGATE_ESC "; mv " DELEGATE_ESC "%i.jxr" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC "; mv " DELEGATE_ESC "%o.tiff" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"lep\" mode=\"decode\" command=\"" DELEGATE_ESC "lepton" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"odt\" command=\"" DELEGATE_ESC "soffice" DELEGATE_ESC " --convert-to pdf -outdir `dirname " DELEGATE_ESC "%i" DELEGATE_ESC "` " DELEGATE_ESC "%i" DELEGATE_ESC " 2&gt; " DELEGATE_ESC "%u" DELEGATE_ESC "; mv " DELEGATE_ESC "%i.pdf" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"pcl:cmyk\" stealth=\"True\" command=\"" DELEGATE_ESC "pcl6" DELEGATE_ESC " -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 " DELEGATE_ESC "-sDEVICE=pamcmyk32" DELEGATE_ESC " -dTextAlphaBits=%u -dGraphicsAlphaBits=%u " DELEGATE_ESC "-r%s" DELEGATE_ESC " %s " DELEGATE_ESC "-sOutputFile=%s" DELEGATE_ESC " " DELEGATE_ESC "%s" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"pcl:color\" stealth=\"True\" command=\"" DELEGATE_ESC "pcl6" DELEGATE_ESC " -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 " DELEGATE_ESC "-sDEVICE=ppmraw" DELEGATE_ESC " -dTextAlphaBits=%u -dGraphicsAlphaBits=%u " DELEGATE_ESC "-r%s" DELEGATE_ESC " %s " DELEGATE_ESC "-sOutputFile=%s" DELEGATE_ESC " " DELEGATE_ESC "%s" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"pcl:mono\" stealth=\"True\" command=\"" DELEGATE_ESC "pcl6" DELEGATE_ESC " -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 " DELEGATE_ESC "-sDEVICE=pbmraw" DELEGATE_ESC " -dTextAlphaBits=%u -dGraphicsAlphaBits=%u " DELEGATE_ESC "-r%s" DELEGATE_ESC " %s " DELEGATE_ESC "-sOutputFile=%s" DELEGATE_ESC " " DELEGATE_ESC "%s" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"pdf\" encode=\"eps\" mode=\"bi\" command=\"" DELEGATE_ESC "gs" DELEGATE_ESC " -sstdout=%%stderr -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 -sPDFPassword=" DELEGATE_ESC "%a" DELEGATE_ESC " " DELEGATE_ESC "-sDEVICE=eps2write" DELEGATE_ESC " " DELEGATE_ESC "-sOutputFile=%o" DELEGATE_ESC " " DELEGATE_ESC "-f%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"pdf\" encode=\"ps\" mode=\"bi\" command=\"" DELEGATE_ESC "gs" DELEGATE_ESC " -sstdout=%%stderr -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 " DELEGATE_ESC "-sDEVICE=ps2write" DELEGATE_ESC " -sPDFPassword=" DELEGATE_ESC "%a" DELEGATE_ESC " " DELEGATE_ESC "-sOutputFile=%o" DELEGATE_ESC " " DELEGATE_ESC "-f%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"png\" encode=\"clipboard\" command=\"" DELEGATE_ESC "xclip" DELEGATE_ESC " -selection clipboard -t image/png " DELEGATE_ESC "%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"clipboard\" command=\"" DELEGATE_ESC "xclip" DELEGATE_ESC " -selection clipboard -o &gt; " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"png\" encode=\"webp\" command=\"" DELEGATE_ESC "cwebp" DELEGATE_ESC " -quiet -q %Q " DELEGATE_ESC "%i" DELEGATE_ESC " -o " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"pnm\" encode=\"ilbm\" mode=\"encode\" command=\"" DELEGATE_ESC "ppmtoilbm" DELEGATE_ESC " -24if " DELEGATE_ESC "%i" DELEGATE_ESC " &gt; " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"tiff\" encode=\"jxr\" command=\"mv " DELEGATE_ESC "%i" DELEGATE_ESC " " DELEGATE_ESC "%i.tiff" DELEGATE_ESC "; " DELEGATE_ESC "JxrEncApp" DELEGATE_ESC " -i " DELEGATE_ESC "%i.tiff" DELEGATE_ESC " -o " DELEGATE_ESC "%o.jxr" DELEGATE_ESC "; mv " DELEGATE_ESC "%i.tiff" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC "; mv " DELEGATE_ESC "%o.jxr" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"tiff\" encode=\"wdp\" command=\"mv " DELEGATE_ESC "%i" DELEGATE_ESC " " DELEGATE_ESC "%i.tiff" DELEGATE_ESC "; " DELEGATE_ESC "JxrEncApp" DELEGATE_ESC " -i " DELEGATE_ESC "%i.tiff" DELEGATE_ESC " -o " DELEGATE_ESC "%o.jxr" DELEGATE_ESC "; mv " DELEGATE_ESC "%i.tiff" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC "; mv " DELEGATE_ESC "%o.jxr" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"ppt\" command=\"" DELEGATE_ESC "soffice" DELEGATE_ESC " --convert-to pdf -outdir `dirname " DELEGATE_ESC "%i" DELEGATE_ESC "` " DELEGATE_ESC "%i" DELEGATE_ESC " 2&gt; " DELEGATE_ESC "%u" DELEGATE_ESC "; mv " DELEGATE_ESC "%i.pdf" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"pptx\" command=\"" DELEGATE_ESC "soffice" DELEGATE_ESC " --convert-to pdf -outdir `dirname " DELEGATE_ESC "%i" DELEGATE_ESC "` " DELEGATE_ESC "%i" DELEGATE_ESC " 2&gt; " DELEGATE_ESC "%u" DELEGATE_ESC "; mv " DELEGATE_ESC "%i.pdf" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"ps\" encode=\"prt\" command=\"" DELEGATE_ESC "lpr" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"ps:alpha\" stealth=\"True\" command=\"" DELEGATE_ESC "gs" DELEGATE_ESC " -sstdout=%%stderr -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 " DELEGATE_ESC "-sDEVICE=pngalpha" DELEGATE_ESC " -dTextAlphaBits=%u -dGraphicsAlphaBits=%u " DELEGATE_ESC "-r%s" DELEGATE_ESC " %s " DELEGATE_ESC "-sOutputFile=%s" DELEGATE_ESC " " DELEGATE_ESC "-f%s" DELEGATE_ESC " " DELEGATE_ESC "-f%s" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"ps:cmyk\" stealth=\"True\" command=\"" DELEGATE_ESC "gs" DELEGATE_ESC " -sstdout=%%stderr -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 " DELEGATE_ESC "-sDEVICE=pamcmyk32" DELEGATE_ESC " -dTextAlphaBits=%u -dGraphicsAlphaBits=%u " DELEGATE_ESC "-r%s" DELEGATE_ESC " %s " DELEGATE_ESC "-sOutputFile=%s" DELEGATE_ESC " " DELEGATE_ESC "-f%s" DELEGATE_ESC " " DELEGATE_ESC "-f%s" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"ps:color\" stealth=\"True\" command=\"" DELEGATE_ESC "gs" DELEGATE_ESC " -sstdout=%%stderr -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 " DELEGATE_ESC "-sDEVICE=pnmraw" DELEGATE_ESC " -dTextAlphaBits=%u -dGraphicsAlphaBits=%u " DELEGATE_ESC "-r%s" DELEGATE_ESC " %s " DELEGATE_ESC "-sOutputFile=%s" DELEGATE_ESC " " DELEGATE_ESC "-f%s" DELEGATE_ESC " " DELEGATE_ESC "-f%s" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"ps\" encode=\"eps\" mode=\"bi\" command=\"" DELEGATE_ESC "gs" DELEGATE_ESC " -sstdout=%%stderr -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 " DELEGATE_ESC "-sDEVICE=eps2write" DELEGATE_ESC " " DELEGATE_ESC "-sOutputFile=%o" DELEGATE_ESC " " DELEGATE_ESC "-f%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"ps\" encode=\"pdf\" mode=\"bi\" command=\"" DELEGATE_ESC "gs" DELEGATE_ESC " -sstdout=%%stderr -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 " DELEGATE_ESC "-sDEVICE=pdfwrite" DELEGATE_ESC " " DELEGATE_ESC "-sOutputFile=%o" DELEGATE_ESC " " DELEGATE_ESC "-f%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"ps\" encode=\"print\" mode=\"encode\" command=\"lpr " DELEGATE_ESC "%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"ps:mono\" stealth=\"True\" command=\"" DELEGATE_ESC "gs" DELEGATE_ESC " -sstdout=%%stderr -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 " DELEGATE_ESC "-sDEVICE=pbmraw" DELEGATE_ESC " -dTextAlphaBits=%u -dGraphicsAlphaBits=%u " DELEGATE_ESC "-r%s" DELEGATE_ESC " %s " DELEGATE_ESC "-sOutputFile=%s" DELEGATE_ESC " " DELEGATE_ESC "-f%s" DELEGATE_ESC " " DELEGATE_ESC "-f%s" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"shtml\" command=\"" DELEGATE_ESC "html2ps" DELEGATE_ESC " -U -o " DELEGATE_ESC "%o" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"sid\" command=\"" DELEGATE_ESC "mrsidgeodecode" DELEGATE_ESC " -if sid -i " DELEGATE_ESC "%i" DELEGATE_ESC " -of tif -o " DELEGATE_ESC "%o" DELEGATE_ESC " &gt; " DELEGATE_ESC "%u" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"svg\" command=\"" DELEGATE_ESC "rsvg-convert" DELEGATE_ESC " --dpi-x %x --dpi-y %y -o " DELEGATE_ESC "%o" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC "\"/>"
#ifndef MAGICKCORE_RSVG_DELEGATE
    "  <delegate decode=\"svg:decode\" stealth=\"True\" command=\"" DELEGATE_ESC "inkscape" DELEGATE_ESC " " DELEGATE_ESC "%s" DELEGATE_ESC " --export-png=" DELEGATE_ESC "%s" DELEGATE_ESC " --export-dpi=" DELEGATE_ESC "%s" DELEGATE_ESC " --export-background=" DELEGATE_ESC "%s" DELEGATE_ESC " --export-background-opacity=" DELEGATE_ESC "%s" DELEGATE_ESC "\"/>"
#endif
    "  <delegate decode=\"tiff\" encode=\"launch\" mode=\"encode\" command=\"" DELEGATE_ESC "gimp" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"wdp\" command=\"mv " DELEGATE_ESC "%i" DELEGATE_ESC " " DELEGATE_ESC "%i.jxr" DELEGATE_ESC "; " DELEGATE_ESC "JxrDecApp" DELEGATE_ESC " -i " DELEGATE_ESC "%i.jxr" DELEGATE_ESC " -o " DELEGATE_ESC "%o.tiff" DELEGATE_ESC "; mv " DELEGATE_ESC "%i.jxr" DELEGATE_ESC " " DELEGATE_ESC "%i" DELEGATE_ESC "; mv " DELEGATE_ESC "%o.tiff" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"webp\" command=\"" DELEGATE_ESC "dwebp" DELEGATE_ESC " -pam " DELEGATE_ESC "%i" DELEGATE_ESC " -o " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"xls\" command=\"" DELEGATE_ESC "soffice" DELEGATE_ESC " --convert-to pdf -outdir `dirname " DELEGATE_ESC "%i" DELEGATE_ESC "` " DELEGATE_ESC "%i" DELEGATE_ESC " 2&gt; " DELEGATE_ESC "%u" DELEGATE_ESC "; mv " DELEGATE_ESC "%i.pdf" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"xlsx\" command=\"" DELEGATE_ESC "soffice" DELEGATE_ESC " --convert-to pdf -outdir `dirname " DELEGATE_ESC "%i" DELEGATE_ESC "` " DELEGATE_ESC "%i" DELEGATE_ESC " 2&gt; " DELEGATE_ESC "%u" DELEGATE_ESC "; mv " DELEGATE_ESC "%i.pdf" DELEGATE_ESC " " DELEGATE_ESC "%o" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"xps:cmyk\" stealth=\"True\" command=\"" DELEGATE_ESC "gxps" DELEGATE_ESC " -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 " DELEGATE_ESC "-sDEVICE=bmpsep8" DELEGATE_ESC " -dTextAlphaBits=%u -dGraphicsAlphaBits=%u " DELEGATE_ESC "-r%s" DELEGATE_ESC " %s " DELEGATE_ESC "-sOutputFile=%s" DELEGATE_ESC " " DELEGATE_ESC "%s" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"xps:color\" stealth=\"True\" command=\"" DELEGATE_ESC "gxps" DELEGATE_ESC " -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 " DELEGATE_ESC "-sDEVICE=ppmraw" DELEGATE_ESC " -dTextAlphaBits=%u -dGraphicsAlphaBits=%u " DELEGATE_ESC "-r%s" DELEGATE_ESC " %s " DELEGATE_ESC "-sOutputFile=%s" DELEGATE_ESC " " DELEGATE_ESC "%s" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"xps:mono\" stealth=\"True\" command=\"" DELEGATE_ESC "gxps" DELEGATE_ESC " -dQUIET -dSAFER -dBATCH -dNOPAUSE -dNOPROMPT -dMaxBitmap=500000000 -dAlignToPixels=0 -dGridFitTT=2 " DELEGATE_ESC "-sDEVICE=pbmraw" DELEGATE_ESC " -dTextAlphaBits=%u -dGraphicsAlphaBits=%u " DELEGATE_ESC "-r%s" DELEGATE_ESC " %s " DELEGATE_ESC "-sOutputFile=%s" DELEGATE_ESC " " DELEGATE_ESC "%s" DELEGATE_ESC "\"/>"
    "  <delegate decode=\"video:decode\" command=\"" DELEGATE_ESC "ffmpeg" DELEGATE_ESC " -nostdin -loglevel error -i " DELEGATE_ESC "%s" DELEGATE_ESC " -an -f rawvideo -y %s " DELEGATE_ESC "%s" DELEGATE_ESC "\"/>"
    "  <delegate encode=\"video:encode\" stealth=\"True\" command=\"" DELEGATE_ESC "ffmpeg" DELEGATE_ESC " -nostdin -loglevel error -i " DELEGATE_ESC "%s%%d.%s" DELEGATE_ESC " %s " DELEGATE_ESC "%s.%s" DELEGATE_ESC "\"/>"
    "</delegatemap>";

#undef DELEGATE_ESC
/*
  Typedef declarations.
*/
typedef struct _DelegateLoadTarget
{
  LinkedListInfo
    *cache;

  ExceptionInfo
    *exception;
} DelegateLoadTarget;

/*
  Global declarations.
*/
static LinkedListInfo
  *delegate_cache = (LinkedListInfo *) NULL;

static SemaphoreInfo
  *delegate_semaphore = (SemaphoreInfo *) NULL;

/*
  Forward declarations.
*/
static MagickBooleanType
  IsDelegateCacheInstantiated(ExceptionInfo *),
  LoadDelegateCache(const DelegateLoadTarget *,const char *,const char *,
    const size_t);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  A c q u i r e D e l e g a t e C a c h e                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireDelegateCache() caches one or more delegate configurations which
%  provides a mapping between delegate attributes and a delegate name.
%
%  The format of the AcquireDelegateCache method is:
%
%      LinkedListInfo *AcquireDelegateCache(const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: the font file name.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static LinkedListInfo *AcquireDelegateCache(const char *filename,
  ExceptionInfo *exception)
{
  LinkedListInfo
    *cache;

  DelegateLoadTarget target;

  cache=NewLinkedList(0);
  target.cache=cache;
  target.exception=exception;

#if !MAGICKCORE_ZERO_CONFIGURATION_SUPPORT
  {
    const StringInfo
      *option;

    LinkedListInfo
      *options;

    options=GetConfigureOptions(filename,exception);
    option=(const StringInfo *) GetNextValueInLinkedList(options);
    while (option != (const StringInfo *) NULL)
    {
      (void) LoadDelegateCache(&target,(const char *)
        GetStringInfoDatum(option),GetStringInfoPath(option),0);
      option=(const StringInfo *) GetNextValueInLinkedList(options);
    }
    options=DestroyConfigureOptions(options);
  }
#else
  magick_unreferenced(filename);
#endif
  if (IsLinkedListEmpty(cache) != MagickFalse)
    (void) LoadDelegateCache(&target,DelegateMap,"built-in",0);
  return(cache);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e l e g a t e C o m p o n e n t G e n e s i s                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DelegateComponentGenesis() instantiates the delegate component.
%
%  The format of the DelegateComponentGenesis method is:
%
%      MagickBooleanType DelegateComponentGenesis(void)
%
*/
MagickPrivate MagickBooleanType DelegateComponentGenesis(void)
{
  if (delegate_semaphore == (SemaphoreInfo *) NULL)
    delegate_semaphore=AcquireSemaphoreInfo();
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e l e g a t e C o m p o n e n t T e r m i n u s                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DelegateComponentTerminus() destroys the delegate component.
%
%  The format of the DelegateComponentTerminus method is:
%
%      DelegateComponentTerminus(void)
%
*/

static void *DestroyDelegate(void *delegate_info)
{
  DelegateInfo
    *p;

  p=(DelegateInfo *) delegate_info;
  if (p->path != (char *) NULL)
    p->path=DestroyString(p->path);
  if (p->decode != (char *) NULL)
    p->decode=DestroyString(p->decode);
  if (p->encode != (char *) NULL)
    p->encode=DestroyString(p->encode);
  if (p->commands != (char *) NULL)
    p->commands=DestroyString(p->commands);
  if (p->semaphore != (SemaphoreInfo *) NULL)
    RelinquishSemaphoreInfo(&p->semaphore);
  p=(DelegateInfo *) RelinquishMagickMemory(p);
  return((void *) NULL);
}

MagickPrivate void DelegateComponentTerminus(void)
{
  if (delegate_semaphore == (SemaphoreInfo *) NULL)
    ActivateSemaphoreInfo(&delegate_semaphore);
  LockSemaphoreInfo(delegate_semaphore);
  if (delegate_cache != (LinkedListInfo *) NULL)
    delegate_cache=DestroyLinkedList(delegate_cache,DestroyDelegate);
  UnlockSemaphoreInfo(delegate_semaphore);
  RelinquishSemaphoreInfo(&delegate_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   E x t e r n a l D e l e g a t e C o m m a n d                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExternalDelegateCommand() executes the specified command and waits until it
%  terminates.  The returned value is the exit status of the command.
%
%  The format of the ExternalDelegateCommand method is:
%
%      int ExternalDelegateCommand(const MagickBooleanType asynchronous,
%        const MagickBooleanType verbose,const char *command,
%        char *message,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o asynchronous: a value other than 0 executes the parent program
%      concurrently with the new child process.
%
%    o verbose: a value other than 0 prints the executed command before it is
%      invoked.
%
%    o command: this string is the command to execute.
%
%    o message: an option buffer to receive any message posted to stdout or
%      stderr.
%
%    o exception: return any errors here.
%
*/

static inline MagickBooleanType IsWindowsDrivePath(const char *token)
{
  return(((isalpha((int) *token) != 0) && (token[1] == ':') &&
    ((token[2] == '\\') || (token[2] == '/'))) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsDotRelativePath(const char *token)
{
  return(((*token == '.') && ((token[1] == '\\') || (token[1] == '/'))) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType HasExecutableLead(const char *token)
{
  if ((token == (const char *) NULL) || (*token == '\0'))
    return(MagickFalse);
  if (*token == '-')
    return(MagickFalse);
  if (*token == '%')
    return(MagickFalse);
  if ((*token == '\'') || (*token == '"'))
    return(MagickFalse);
  return(MagickTrue);
}

static inline MagickBooleanType IsExecutableToken(const char *token)
{
  size_t
    length = 0;

  if (HasExecutableLead(token) == MagickFalse)
    return(MagickFalse);
  length=strlen(token);
  if ((length >= 3) && (IsWindowsDrivePath(token) != MagickFalse))
    return(MagickTrue);
  if (*token == '/')
    return(MagickTrue);
  if ((length >= 2) && (IsDotRelativePath(token) != MagickFalse))
    return(MagickTrue);
  if (isalnum((int) *token))
    return(MagickTrue);
  return(MagickFalse);
}

static void DestroyDelegateArguments(char **arguments,
  const int number_arguments)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) number_arguments; i++)
    arguments[i]=DestroyString(arguments[i]);
  (void) RelinquishMagickMemory(arguments);
}

static MagickBooleanType AuthorizeDelegateArguments(char **arguments,
  const int number_arguments,ExceptionInfo *exception)
{
  PolicyDomain
    domain;

  PolicyRights
    rights;

  ssize_t
    i;

  rights=ExecutePolicyRights;
  domain=DelegatePolicyDomain;
  for (i=1; i < (ssize_t) number_arguments; i++)
  {
    if ((i != 1) && (IsExecutableToken(arguments[i]) == MagickFalse))
      continue;
    if (IsRightsAuthorized(domain,rights,arguments[i]) == MagickFalse)
      {
        errno=EPERM;
        (void) ThrowMagickException(exception,GetMagickModule(),PolicyError,
          "NotAuthorized","`%s'",arguments[i]);
        DestroyDelegateArguments(arguments,number_arguments);
        return(MagickFalse);
      }
  }
  return(MagickTrue);
}

#if defined(MAGICKCORE_POSIX_SUPPORT) && defined(MAGICKCORE_HAVE_EXECVP)
static int WaitForDelegateChild(const pid_t child_pid)
{
  int
    child_status = 0,
    status = (-1);

  pid_t
    pid;

  pid=(pid_t) waitpid(child_pid,&child_status,0);
  if (pid == -1)
    return(-1);
  if (WIFEXITED(child_status) != 0)
    status=WEXITSTATUS(child_status);
  else
    if (WIFSIGNALED(child_status))
      status=(-1);
  return(status);
}

static int RunDelegateProgram(char **arguments,const char *sanitize_command)
{
  pid_t
    child_pid;

  /*
    Call application directly rather than from a shell.
  */
  child_pid=(pid_t) fork();
  if (child_pid == (pid_t) -1)
    return(system(sanitize_command));
  if (child_pid != 0)
    return(WaitForDelegateChild(child_pid));
  (void) execvp(arguments[1],arguments+1);
  _exit(1);
}
#endif

#if defined(MAGICKCORE_POSIX_SUPPORT) && defined(MAGICKCORE_HAVE_POPEN)
static int ReadDelegateOutput(const char *sanitize_command,char *message)
{
  FILE
    *file;

  size_t
    offset = 0;

  file=popen_utf8(sanitize_command,"r");
  if (file == (FILE *) NULL)
    return(system(sanitize_command));
  while ((offset < MagickPathExtent) &&
         (fgets(message+offset,(int) (MagickPathExtent-offset),file) != NULL))
    offset+=strlen(message);
  return(pclose(file));
}
#endif

static MagickBooleanType AcceptDelegateArguments(char **arguments,
  const int number_arguments,ExceptionInfo *exception)
{
  if (*arguments[1] == '\0')
    {
      DestroyDelegateArguments(arguments,number_arguments);
      return(MagickFalse);
    }
  if (AuthorizeDelegateArguments(arguments,number_arguments,exception) ==
      MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static char *PrepareDelegateCommand(const MagickBooleanType asynchronous,
  const MagickBooleanType verbose,const char *command,char *message)
{
  char
    *sanitize_command;

  if (verbose != MagickFalse)
    {
      (void) FormatLocaleFile(stderr,"%s\n",command);
      (void) fflush(stderr);
    }
  sanitize_command=SanitizeString(command);
  if (asynchronous != MagickFalse)
    (void) ConcatenateMagickString(sanitize_command,"&",MagickPathExtent);
  if (message != (char *) NULL)
    *message='\0';
  return(sanitize_command);
}

MagickExport int ExternalDelegateCommand(const MagickBooleanType asynchronous,
  const MagickBooleanType verbose,const char *command,char *message,
  ExceptionInfo *exception)
{
  char
    **arguments,
    *sanitize_command;

  int
    number_arguments,
    status;

  status=(-1);
  arguments=StringToArgv(command,&number_arguments);
  if (arguments == (char **) NULL)
    return(status);
  if (AcceptDelegateArguments(arguments,number_arguments,exception) ==
      MagickFalse)
    return(-1);
  sanitize_command=PrepareDelegateCommand(asynchronous,verbose,command,
    message);
#if defined(MAGICKCORE_POSIX_SUPPORT)
#if defined(MAGICKCORE_HAVE_POPEN)
  if ((asynchronous == MagickFalse) && (message !=  (char *) NULL))
    status=ReadDelegateOutput(sanitize_command,message);
  else
#endif
    {
#if !defined(MAGICKCORE_HAVE_EXECVP)
      status=system(sanitize_command);
#else
      if ((asynchronous != MagickFalse) ||
          (strpbrk(sanitize_command,"&;<>|") != (char *) NULL))
        status=system(sanitize_command);
      else
        status=RunDelegateProgram(arguments,sanitize_command);
#endif
    }
#elif defined(MAGICKCORE_WINDOWS_SUPPORT)
  {
    char
      *p;

    /*
      If a command shell is executed we need to change the forward slashes in
      files to a backslash. We need to do this to keep Windows happy when we
      want to 'move' a file.

      TODO: This won't work if one of the delegate parameters has a forward
            slash as a parameter.
    */
    p=strstr(sanitize_command,"cmd.exe /c");
    if (p != (char*) NULL)
      {
        p+=(ptrdiff_t) 10;
        for ( ; *p != '\0'; p++)
          if (*p == '/')
            *p=(*DirectorySeparator);
      }
  }
  status=NTSystemCommand(sanitize_command,message);
#elif defined(vms)
  status=system(sanitize_command);
#else
#  error No suitable system() method.
#endif
  if (status < 0)
    {
      if ((message != (char *) NULL) && (*message != '\0'))
        (void) ThrowMagickException(exception,GetMagickModule(),DelegateError,
          "FailedToExecuteCommand","`%s' (%s)",sanitize_command,message);
      else
        (void) ThrowMagickException(exception,GetMagickModule(),DelegateError,
          "FailedToExecuteCommand","`%s' (%d)",sanitize_command,status);
    }
  sanitize_command=DestroyString(sanitize_command);
  DestroyDelegateArguments(arguments,number_arguments);
  return(status);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t D e l e g a t e C o m m a n d s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDelegateCommands() returns the commands associated with a delegate.
%
%  The format of the GetDelegateCommands method is:
%
%      const char *GetDelegateCommands(const DelegateInfo *delegate_info)
%
%  A description of each parameter follows:
%
%    o delegate_info:  The delegate info.
%
*/
MagickExport const char *GetDelegateCommands(const DelegateInfo *delegate_info)
{
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(delegate_info != (DelegateInfo *) NULL);
  assert(delegate_info->signature == MagickCoreSignature);
  return(delegate_info->commands);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t D e l e g a t e I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDelegateInfo() returns any delegates associated with the specified tag.
%
%  The format of the GetDelegateInfo method is:
%
%      const DelegateInfo *GetDelegateInfo(const char *decode,
%        const char *encode,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o decode: Specifies the decode delegate we are searching for as a
%      character string.
%
%    o encode: Specifies the encode delegate we are searching for as a
%      character string.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType IsBidirectionalMismatch(
  const DelegateInfo *delegate_info,const char *decode,const char *encode)
{
  if ((LocaleCompare(decode,delegate_info->decode) == 0) &&
      (LocaleCompare(encode,delegate_info->encode) == 0))
    return(MagickFalse);
  if ((LocaleCompare(decode,"*") == 0) &&
      (LocaleCompare(encode,delegate_info->encode) == 0))
    return(MagickFalse);
  if ((LocaleCompare(decode,delegate_info->decode) == 0) &&
      (LocaleCompare(encode,"*") == 0))
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType IsDelegateMatch(const DelegateInfo *delegate_info,
  const char *decode,const char *encode)
{
  if (delegate_info->mode > 0)
    return(LocaleCompare(delegate_info->decode,decode) == 0 ? MagickTrue :
      MagickFalse);
  if (delegate_info->mode < 0)
    return(LocaleCompare(delegate_info->encode,encode) == 0 ? MagickTrue :
      MagickFalse);
  return(IsBidirectionalMismatch(delegate_info,decode,encode) == MagickFalse ?
    MagickTrue : MagickFalse);
}

static void FindDelegateElement(const char *decode,const char *encode,
  ElementInfo **p,const DelegateInfo **delegate_info)
{
  while (*p != (ElementInfo *) NULL)
  {
    *delegate_info=(const DelegateInfo* ) (*p)->value;
    if (IsDelegateMatch(*delegate_info,decode,encode) != MagickFalse)
      break;
    *p=(*p)->next;
  }
}

MagickExport const DelegateInfo *GetDelegateInfo(const char *decode,
  const char *encode,ExceptionInfo *exception)
{
  const DelegateInfo
    *delegate_info;

  ElementInfo
    *p;

  assert(exception != (ExceptionInfo *) NULL);
  if (IsDelegateCacheInstantiated(exception) == MagickFalse)
    return((const DelegateInfo *) NULL);
  /*
    Search for named delegate.
  */
  delegate_info=(const DelegateInfo *) NULL;
  LockSemaphoreInfo(delegate_semaphore);
  p=GetHeadElementInLinkedList(delegate_cache);
  if ((LocaleCompare(decode,"*") == 0) && (LocaleCompare(encode,"*") == 0))
    {
      UnlockSemaphoreInfo(delegate_semaphore);
      if (p != (ElementInfo *) NULL)
        delegate_info=(const DelegateInfo* ) p->value;
      return(delegate_info);
    }
  FindDelegateElement(decode,encode,&p,&delegate_info);
  if (p == (ElementInfo *) NULL)
    delegate_info=(const DelegateInfo *) NULL;
  else
    SetHeadElementInLinkedList(delegate_cache,p);
  UnlockSemaphoreInfo(delegate_semaphore);
  return(delegate_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t D e l e g a t e I n f o L i s t                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDelegateInfoList() returns any delegates that match the specified pattern.
%
%  The delegate of the GetDelegateInfoList function is:
%
%      const DelegateInfo **GetDelegateInfoList(const char *pattern,
%        size_t *number_delegates,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_delegates:  This integer returns the number of delegates in the
%      list.
%
%    o exception: return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int DelegateInfoCompare(const void *x,const void *y)
{
  const DelegateInfo
    **p,
    **q;

  int
    cmp;

  p=(const DelegateInfo **) x,
  q=(const DelegateInfo **) y;
  cmp=LocaleCompare((*p)->path,(*q)->path);
  if (cmp == 0)
    {
      if ((*p)->decode == (char *) NULL)
        if (((*p)->encode != (char *) NULL) &&
            ((*q)->encode != (char *) NULL))
          return(strcmp((*p)->encode,(*q)->encode));
      if (((*p)->decode != (char *) NULL) &&
          ((*q)->decode != (char *) NULL))
        return(strcmp((*p)->decode,(*q)->decode));
    }
  return(cmp);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static inline MagickBooleanType IsListedDelegate(
  const DelegateInfo *delegate_info,const char *pattern)
{
  return(((delegate_info->stealth == MagickFalse) &&
    (GlobExpression(delegate_info->decode,pattern,MagickFalse) != MagickFalse ||
     GlobExpression(delegate_info->encode,pattern,MagickFalse) != MagickFalse)) ?
    MagickTrue : MagickFalse);
}

MagickExport const DelegateInfo **GetDelegateInfoList(const char *pattern,
  size_t *number_delegates,ExceptionInfo *exception)
{
  const DelegateInfo
    **delegates;

  ElementInfo
    *p;

  ssize_t
    i;

  assert(number_delegates != (size_t *) NULL);
  assert(pattern != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  *number_delegates=0;
  if (IsDelegateCacheInstantiated(exception) == MagickFalse)
    return((const DelegateInfo **) NULL);
  delegates=(const DelegateInfo **) AcquireQuantumMemory((size_t)
    GetNumberOfElementsInLinkedList(delegate_cache)+1UL,sizeof(*delegates));
  if (delegates == (const DelegateInfo **) NULL)
    return((const DelegateInfo **) NULL);
  LockSemaphoreInfo(delegate_semaphore);
  p=GetHeadElementInLinkedList(delegate_cache);
  for (i=0; p != (ElementInfo *) NULL; )
  {
    const DelegateInfo
      *delegate_info;

    delegate_info=(const DelegateInfo *) p->value;
    if (IsListedDelegate(delegate_info,pattern) != MagickFalse)
      delegates[i++]=delegate_info;
    p=p->next;
  }
  UnlockSemaphoreInfo(delegate_semaphore);
  if (i == 0)
    delegates=(const DelegateInfo **) RelinquishMagickMemory((void*) delegates);
  else
    {
      qsort((void *) delegates,(size_t) i,sizeof(*delegates),DelegateInfoCompare);
      delegates[i]=(DelegateInfo *) NULL;
    }
  *number_delegates=(size_t) i;
  return(delegates);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t D e l e g a t e L i s t                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDelegateList() returns any image format delegates that match the
%  specified  pattern.
%
%  The format of the GetDelegateList function is:
%
%      char **GetDelegateList(const char *pattern,
%        size_t *number_delegates,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_delegates:  This integer returns the number of delegates
%      in the list.
%
%    o exception: return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int DelegateCompare(const void *x,const void *y)
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

static inline MagickBooleanType IsListedDelegateMatch(
  const DelegateInfo *delegate_info,const char *command,const char *pattern)
{
  return(((delegate_info->stealth == MagickFalse) &&
    (GlobExpression(command,pattern,MagickFalse) != MagickFalse)) ?
    MagickTrue : MagickFalse);
}

MagickExport char **GetDelegateList(const char *pattern,
  size_t *number_delegates,ExceptionInfo *exception)
{
  char
    **delegates;

  ElementInfo
    *p;

  ssize_t
    i;

  assert(pattern != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_delegates != (size_t *) NULL);
  *number_delegates=0;
  if (IsDelegateCacheInstantiated(exception) == MagickFalse)
    return((char **) NULL);
  delegates=(char **) AcquireQuantumMemory((size_t)
    GetNumberOfElementsInLinkedList(delegate_cache)+1UL,sizeof(*delegates));
  if (delegates == (char **) NULL)
    return((char **) NULL);
  LockSemaphoreInfo(delegate_semaphore);
  p=GetHeadElementInLinkedList(delegate_cache);
  for (i=0; p != (ElementInfo *) NULL; )
  {
    const DelegateInfo
      *delegate_info;

    delegate_info=(const DelegateInfo *) p->value;
    if (IsListedDelegateMatch(delegate_info,delegate_info->decode,pattern) !=
        MagickFalse)
      delegates[i++]=ConstantString(delegate_info->decode);
    if (IsListedDelegateMatch(delegate_info,delegate_info->encode,pattern) !=
        MagickFalse)
      delegates[i++]=ConstantString(delegate_info->encode);
    p=p->next;
  }
  UnlockSemaphoreInfo(delegate_semaphore);
  if (i == 0)
    delegates=(char **) RelinquishMagickMemory(delegates);
  else
    {
      qsort((void *) delegates,(size_t) i,sizeof(*delegates),DelegateCompare);
      delegates[i]=(char *) NULL;
    }
  *number_delegates=(size_t) i;
  return(delegates);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t D e l e g a t e M o d e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDelegateMode() returns the mode of the delegate.
%
%  The format of the GetDelegateMode method is:
%
%      ssize_t GetDelegateMode(const DelegateInfo *delegate_info)
%
%  A description of each parameter follows:
%
%    o delegate_info:  The delegate info.
%
*/
MagickExport ssize_t GetDelegateMode(const DelegateInfo *delegate_info)
{
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(delegate_info != (DelegateInfo *) NULL);
  assert(delegate_info->signature == MagickCoreSignature);
  return(delegate_info->mode);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t D e l e g a t e T h r e a d S u p p o r t                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDelegateThreadSupport() returns MagickTrue if the delegate supports
%  threads.
%
%  The format of the GetDelegateThreadSupport method is:
%
%      MagickBooleanType GetDelegateThreadSupport(
%        const DelegateInfo *delegate_info)
%
%  A description of each parameter follows:
%
%    o delegate_info:  The delegate info.
%
*/
MagickExport MagickBooleanType GetDelegateThreadSupport(
  const DelegateInfo *delegate_info)
{
  assert(delegate_info != (DelegateInfo *) NULL);
  assert(delegate_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  return(delegate_info->thread_support);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I s D e l e g a t e C a c h e I n s t a n t i a t e d                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsDelegateCacheInstantiated() determines if the delegate cache is
%  instantiated.  If not, it instantiates the cache and returns it.
%
%  The format of the IsDelegateInstantiated method is:
%
%      MagickBooleanType IsDelegateCacheInstantiated(ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType IsDelegateCacheInstantiated(ExceptionInfo *exception)
{
  if (delegate_cache == (LinkedListInfo *) NULL)
    {
      if (delegate_semaphore == (SemaphoreInfo *) NULL)
        ActivateSemaphoreInfo(&delegate_semaphore);
      LockSemaphoreInfo(delegate_semaphore);
      if (delegate_cache == (LinkedListInfo *) NULL)
        delegate_cache=AcquireDelegateCache(DelegateFilename,exception);
      UnlockSemaphoreInfo(delegate_semaphore);
    }
  return(delegate_cache != (LinkedListInfo *) NULL ? MagickTrue : MagickFalse);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L i s t D e l e g a t e I n f o                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListDelegateInfo() lists the image formats to a file.
%
%  The format of the ListDelegateInfo method is:
%
%      MagickBooleanType ListDelegateInfo(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to a FILE.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void ListDelegatePath(FILE *file,const char *path,
  const DelegateInfo **delegate_info,const ssize_t i)
{
  if ((path == (const char *) NULL) ||
      (LocaleCompare(path,delegate_info[i]->path) != 0))
    {
      if (delegate_info[i]->path != (char *) NULL)
        (void) FormatLocaleFile(file,"\nPath: %s\n\n",delegate_info[i]->path);
      (void) FormatLocaleFile(file,"Delegate                Command\n");
      (void) FormatLocaleFile(file,
        "-------------------------------------------------"
        "------------------------------\n");
    }
}

static void ListDelegateCommands(FILE *file,
  const DelegateInfo **delegate_info,const ssize_t i,const char *delegate)
{
  char
    **commands;

  ssize_t
    j;

  commands=StringToList(delegate_info[i]->commands);
  if (commands == (char **) NULL)
    return;
  (void) FormatLocaleFile(file,"%11s%c=%c%s  ",delegate_info[i]->decode ?
    delegate_info[i]->decode : "",delegate_info[i]->mode <= 0 ? '<' : ' ',
    delegate_info[i]->mode >= 0 ? '>' : ' ',delegate);
  (void) FormatLocaleFile(file,"\"%s\"\n",commands[0]);
  for (j=1; commands[j] != (char *) NULL; j++)
    (void) FormatLocaleFile(file,"                     \"%s\"\n",commands[j]);
  for (j=0; commands[j] != (char *) NULL; j++)
    commands[j]=DestroyString(commands[j]);
  commands=(char **) RelinquishMagickMemory(commands);
}

MagickExport MagickBooleanType ListDelegateInfo(FILE *file,
  ExceptionInfo *exception)
{
  const DelegateInfo
    **delegate_info;

  char
    delegate[MagickPathExtent];

  const char
    *path;

  ssize_t
    i;

  size_t
    number_delegates;

  if (file == (const FILE *) NULL)
    file=stdout;
  delegate_info=GetDelegateInfoList("*",&number_delegates,exception);
  if (delegate_info == (const DelegateInfo **) NULL)
    return(MagickFalse);
  path=(const char *) NULL;
  for (i=0; i < (ssize_t) number_delegates; i++)
  {
    if (delegate_info[i]->stealth != MagickFalse)
      continue;
    ListDelegatePath(file,path,delegate_info,i);
    path=delegate_info[i]->path;
    *delegate='\0';
    if (delegate_info[i]->encode != (char *) NULL)
      (void) CopyMagickString(delegate,delegate_info[i]->encode,
        MagickPathExtent);
    (void) ConcatenateMagickString(delegate,"        ",MagickPathExtent);
    delegate[9]='\0';
    ListDelegateCommands(file,delegate_info,i,delegate);
  }
  (void) fflush(file);
  delegate_info=(const DelegateInfo **)
    RelinquishMagickMemory((void *) delegate_info);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   L o a d D e l e g a t e C a c h e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadDelegateCache() loads the delegate configurations which provides a
%  mapping between delegate attributes and a delegate name.
%
%  The format of the LoadDelegateCache method is:
%
%      MagickBooleanType LoadDelegateCache(const DelegateLoadTarget *target,
%        const char *xml,const char *filename,const size_t depth)
%
%  A description of each parameter follows:
%
%    o target: the cache to load into, and the exception structure to
%      return any errors or warnings in.
%
%    o xml:  The delegate list in XML format.
%
%    o filename:  The delegate list filename.
%
%    o depth: depth of <include /> statements.
%
*/
static inline MagickBooleanType IsIncludeElementOpen(const char *token,
  const char *q)
{
  return((((*token != '/') && (*(token+1) != '>')) && (*q != '\0')) ?
    MagickTrue : MagickFalse);
}

static void GetIncludePath(const char *filename,const char *token,
  char *path)
{
  GetPathComponent(filename,HeadPath,path);
  if (*path != '\0')
    (void) ConcatenateMagickString(path,DirectorySeparator,
      MagickPathExtent);
  if (*token == *DirectorySeparator)
    (void) CopyMagickString(path,token,MagickPathExtent);
  else
    (void) ConcatenateMagickString(path,token,MagickPathExtent);
}

typedef struct _DelegateInclude
{
  LinkedListInfo
    *cache;

  ExceptionInfo
    *exception;

  const char
    *filename;

  char
    *keyword,
    *token;

  size_t
    depth,
    extent;
} DelegateInclude;

static void LoadDelegateInclude(const DelegateInclude *include,
  MagickStatusType *status)
{
  LinkedListInfo
    *const cache = include->cache;

  ExceptionInfo
    *const exception = include->exception;

  const char
    *const filename = include->filename;

  char
    *const token = include->token;

  const size_t
    depth = include->depth;

  if (depth >= MagickMaxRecursionDepth)
    (void) ThrowMagickException(exception,GetMagickModule(),
      ConfigureError,"IncludeElementNestedTooDeeply","`%s'",token);
  else
    {
      const DelegateLoadTarget include_target = { cache,
        exception };

      char
        path[MagickPathExtent],
        *file_xml;

      GetIncludePath(filename,token,path);
      file_xml=FileToXML(path,~0UL);
      if (file_xml != (char *) NULL)
        {
          *status&=(MagickStatusType) LoadDelegateCache(
            &include_target,file_xml,path,depth+1);
          file_xml=DestroyString(file_xml);
        }
    }
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

static void SetDelegateCommands(DelegateInfo *delegate_info,
  const char *token)
{
  char
    *commands;

  commands=AcquireString(token);
#if defined(MAGICKCORE_WINDOWS_SUPPORT)
  if (strchr(commands,'@') != (char *) NULL)
    {
      char
        path[MagickPathExtent];

      NTGhostscriptEXE(path,MagickPathExtent);
      (void) SubstituteString((char **) &commands,"@PSDelegate@",path);
      (void) SubstituteString((char **) &commands,"\\","/");
    }
#endif
  (void) SubstituteString((char **) &commands,"&quot;","\"");
  (void) SubstituteString((char **) &commands,"&apos;","'");
  (void) SubstituteString((char **) &commands,"&amp;","&");
  (void) SubstituteString((char **) &commands,"&gt;",">");
  (void) SubstituteString((char **) &commands,"&lt;","<");
  if (delegate_info->commands != (char *) NULL)
    delegate_info->commands=DestroyString(delegate_info->commands);
  delegate_info->commands=commands;
}

static void SetDelegateThreadSupport(DelegateInfo *delegate_info,
  const char *token)
{
  delegate_info->thread_support=IsStringTrue(token);
  if (delegate_info->thread_support == MagickFalse)
    delegate_info->semaphore=AcquireSemaphoreInfo();
}

static void SetDelegateMode(DelegateInfo *delegate_info,const char *token)
{
  delegate_info->mode=1;
  if (LocaleCompare(token,"bi") == 0)
    delegate_info->mode=0;
  else
    if (LocaleCompare(token,"encode") == 0)
      delegate_info->mode=(-1);
}

static void SetDelegateInfoAttribute(DelegateInfo *delegate_info,
  const char *keyword,const char *token)
{
  if (LocaleCompare((char *) keyword,"command") == 0)
    SetDelegateCommands(delegate_info,token);
  else if (LocaleCompare((char *) keyword,"decode") == 0)
    {
      delegate_info->decode=ConstantString(token);
      delegate_info->mode=1;
    }
  else if (LocaleCompare((char *) keyword,"encode") == 0)
    {
      delegate_info->encode=ConstantString(token);
      delegate_info->mode=(-1);
    }
  else if (LocaleCompare((char *) keyword,"mode") == 0)
    SetDelegateMode(delegate_info,token);
  else if (LocaleCompare((char *) keyword,"spawn") == 0)
    delegate_info->spawn=IsStringTrue(token);
  else if (LocaleCompare((char *) keyword,"stealth") == 0)
    delegate_info->stealth=IsStringTrue(token);
  else if (LocaleCompare((char *) keyword,"thread-support") == 0)
    SetDelegateThreadSupport(delegate_info,token);
}

static MagickBooleanType SkipDelegateElement(const char **q,
  const char *keyword,const size_t extent,char *token)
{
  if (LocaleNCompare(keyword,"<!DOCTYPE",9) == 0)
    {
      /*
        DOCTYPE element.
      */
      *q=SkipDoctypeElement(*q);
      return(MagickTrue);
    }
  if (LocaleNCompare(keyword,"<!--",4) == 0)
    {
      /*
        Comment element.
      */
      *q=SkipCommentElement(*q,extent,token);
      return(MagickTrue);
    }
  return(MagickFalse);
}

static inline MagickBooleanType IsDelegateEnd(const char *keyword)
{
  return(((LocaleCompare(keyword,"/>") == 0) ||
    (LocaleCompare(keyword,"</policy>") == 0)) ? MagickTrue : MagickFalse);
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

static void LoadDelegateIncludeElement(const DelegateInclude *include,
  const char **q,MagickStatusType *status)
{
  while (IsIncludeElementOpen(include->token,*q) != MagickFalse)
  {
    if (IsIncludeFileAttribute(q,include->extent,include->keyword,
        include->token) != MagickFalse)
      LoadDelegateInclude(include,status);
  }
}

static DelegateInfo *AcquireDelegateInfo(const char *filename)
{
  DelegateInfo
    *delegate_info;

  delegate_info=(DelegateInfo *) AcquireCriticalMemory(
    sizeof(*delegate_info));
  (void) memset(delegate_info,0,sizeof(*delegate_info));
  delegate_info->path=ConstantString(filename);
  delegate_info->thread_support=MagickTrue;
  delegate_info->signature=MagickCoreSignature;
  return(delegate_info);
}

static MagickBooleanType LoadDelegateCache(const DelegateLoadTarget *target,
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

  DelegateInfo
    *delegate_info;

  MagickStatusType
    status;

  size_t
    extent;

  /*
    Load the delegate map file.
  */
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Loading delegate configuration file \"%s\" ...",filename);
  if (xml == (const char *) NULL)
    return(MagickFalse);
  status=MagickTrue;
  delegate_info=(DelegateInfo *) NULL;
  token=AcquireString(xml);
  extent=strlen(token)+MagickPathExtent;
  for (q=(const char *) xml; *q != '\0'; )
  {
    /*
      Interpret XML.
    */
    (void) GetNextToken(q,&q,extent,token);
    if (*token == '\0')
      break;
    (void) CopyMagickString(keyword,token,MagickPathExtent);
    if (SkipDelegateElement(&q,keyword,extent,token) != MagickFalse)
      continue;
    if (LocaleCompare(keyword,"<include") == 0)
      {
        /*
          Include element.
        */
        const DelegateInclude
          include = { cache, exception, filename, keyword, token, depth,
            extent };

        LoadDelegateIncludeElement(&include,&q,&status);
        continue;
      }
    if (LocaleCompare(keyword,"<delegate") == 0)
      {
        /*
          Delegate element.
        */
        delegate_info=AcquireDelegateInfo(filename);
        continue;
      }
    if (delegate_info == (DelegateInfo *) NULL)
      continue;
    if (IsDelegateEnd(keyword) != MagickFalse)
      {
        status=AppendValueToLinkedList(cache,delegate_info);
        if (status == MagickFalse)
          (void) ThrowMagickException(exception,GetMagickModule(),
            ResourceLimitError,"MemoryAllocationFailed","`%s'",
            delegate_info->commands);
        delegate_info=(DelegateInfo *) NULL;
        continue;
      }
    (void) GetNextToken(q,(const char **) NULL,extent,token);
    if (*token != '=')
      continue;
    (void) GetNextToken(q,&q,extent,token);
    (void) GetNextToken(q,&q,extent,token);
    SetDelegateInfoAttribute(delegate_info,keyword,token);
  }
  token=(char *) RelinquishMagickMemory(token);
  return(status != 0 ? MagickTrue : MagickFalse);
}
