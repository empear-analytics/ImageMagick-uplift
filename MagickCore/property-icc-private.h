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

  MagickCore private methods of property.c: the ICC profile property reader
  (the MAGICKCORE_LCMS_DELEGATE block, from the ICCInfoProperty table to
  GetICCProperty); included by that file only, where the run stood.
*/
#ifndef MAGICKCORE_PROPERTY_ICC_PRIVATE_H
#define MAGICKCORE_PROPERTY_ICC_PRIVATE_H

#if defined(MAGICKCORE_LCMS_DELEGATE)
#if !defined(LCMS_VERSION) || (LCMS_VERSION >= 2000)
typedef struct _ICCInfoProperty
{
  cmsInfoType
    type;

  const char
    *key;
} ICCInfoProperty;

static const ICCInfoProperty ICCInfoProperties[] =
{
  { cmsInfoDescription, "icc:description" },
  { cmsInfoManufacturer, "icc:manufacturer" },
  { cmsInfoModel, "icc:model" },
  { cmsInfoCopyright, "icc:copyright" }
};

static void SetICCInfoProperty(const Image *image,cmsHPROFILE icc_profile,
  const ICCInfoProperty *entry,ExceptionInfo *exception)
{
  StringInfo
    *info;

  unsigned int
    extent;

  info=AcquireStringInfo(0);
  extent=cmsGetProfileInfoASCII(icc_profile,entry->type,"en","US",NULL,0);
  if (extent != 0)
    {
      SetStringInfoLength(info,extent+1);
      extent=cmsGetProfileInfoASCII(icc_profile,entry->type,"en","US",
        (char *) GetStringInfoDatum(info),extent);
      if (extent != 0)
        (void) SetImageProperty((Image *) image,entry->key,
          (char *) GetStringInfoDatum(info),exception);
    }
  info=DestroyStringInfo(info);
}
#endif

static void GetICCProperty(const Image *image,ExceptionInfo *exception)
{

  const StringInfo
    *profile;

  cmsHPROFILE
    icc_profile;

  /*
    Return ICC profile property.
  */
  profile=GetImageProfile(image,"icc");
  if (profile == (StringInfo *) NULL)
    profile=GetImageProfile(image,"icm");
  if (profile == (StringInfo *) NULL)
    return;
  if (GetStringInfoLength(profile) < 128)
    return;  /* minimum ICC profile length */
  icc_profile=cmsOpenProfileFromMem(GetStringInfoDatum(profile),
    (cmsUInt32Number) GetStringInfoLength(profile));
  if (icc_profile != (cmsHPROFILE *) NULL)
    {
#if defined(LCMS_VERSION) && (LCMS_VERSION < 2000)
      const char
        *name;

      name=cmsTakeProductName(icc_profile);
      if (name != (const char *) NULL)
        (void) SetImageProperty((Image *) image,"icc:name",name,exception);
#else
      ssize_t
        i;

      for (i=0; i < (ssize_t) (sizeof(ICCInfoProperties)/
           sizeof(*ICCInfoProperties)); i++)
        SetICCInfoProperty(image,icc_profile,ICCInfoProperties+i,exception);
#endif
      (void) cmsCloseProfile(icc_profile);
    }
}
#endif

#endif
