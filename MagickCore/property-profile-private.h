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

  MagickCore private methods of property.c: the profile and stored-property
  lookups of GetImageProperty, from GetProfileProperty to
  GetStoredImageProperty; included by that file only, where the run stood.
*/
#ifndef MAGICKCORE_PROPERTY_PROFILE_PRIVATE_H
#define MAGICKCORE_PROPERTY_PROFILE_PRIVATE_H

static void GetProfileProperty(const Image *image,const char *property,
  ExceptionInfo *exception)
{
  if (LocaleNCompare("8bim:",property,5) == 0)
    Get8BIMProperty(image,property,exception);
  else if (LocaleNCompare("exif:",property,5) == 0)
    GetEXIFProperty(image,property,exception);
  else if ((LocaleNCompare("icc:",property,4) == 0) ||
           (LocaleNCompare("icm:",property,4) == 0))
    {
#if defined(MAGICKCORE_LCMS_DELEGATE)
      GetICCProperty(image,exception);
#endif
    }
  else if (LocaleNCompare("iptc:",property,5) == 0)
    GetIPTCProperty(image,property,exception);
  else if (LocaleNCompare("xmp:",property,4) == 0)
    GetXMPProperty(image,property);
}

static inline MagickBooleanType IsWildcardProperty(const char *property,
  const size_t property_length)
{
  return(((property_length > 2) && (*(property+(property_length-2)) == ':') &&
    (*(property+(property_length-1)) == '*')) ? MagickTrue : MagickFalse);
}

static MagickBooleanType GetStoredImageProperty(const Image *image,
  const char *property,const MagickBooleanType read_from_properties,
  const char **result)
{
  const char
    *p;

  if (read_from_properties != MagickFalse)
    {
      p=(const char *) NULL;
      if (image->properties != (void *) NULL)
        {
          p=(const char *) GetValueFromSplayTree((SplayTreeInfo *)
            image->properties,property);
          if (p != (const char *) NULL)
            {
              (*result)=p;
              return(MagickTrue);
            }
        }
      if (strchr(property,':') == (char *) NULL)
        {
          (*result)=p;
          return(MagickTrue);
        }
    }
  return(MagickFalse);
}

#endif
