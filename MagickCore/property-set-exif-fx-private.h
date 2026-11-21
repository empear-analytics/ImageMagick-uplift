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

  MagickCore private text included in place by SetImageProperty
  (property.c): its #if 0 block that would refuse to set a property with
  the exif: and fx: prefixes, verbatim. No build compiles it; this is a fragment, without an
  include guard.
*/
#if 0  /* Percent escape's sets values with this prefix: for later use
          Throwing an exception causes this setting to fail */
    case 'E':
    case 'e':
    {
      if (LocaleNCompare("exif:",property,5) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      break; /* not an attribute, add as a property */
    }
    case 'F':
    case 'f':
    {
      if (LocaleNCompare("fx:",property,3) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      break; /* not an attribute, add as a property */
    }
#endif
