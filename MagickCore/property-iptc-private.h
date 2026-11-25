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

  MagickCore private methods of property.c: the forward declarations of the
  clip path tracers and the IPTC property reader, from AppendIPTCDataset to
  GetIPTCProperty; included by that file only, where the run stood.
*/
#ifndef MAGICKCORE_PROPERTY_IPTC_PRIVATE_H
#define MAGICKCORE_PROPERTY_IPTC_PRIVATE_H

static char
  *TracePSClippath(const unsigned char *,size_t),
  *TraceSVGClippath(const unsigned char *,size_t,const size_t,
    const size_t);

static char *AppendIPTCDataset(char *attribute,const StringInfo *profile,
  const unsigned char *p,const ssize_t i)
{
  char
    *message = (char *) NULL;

  size_t declared = ((size_t) p[3] << 8) | (size_t) p[4];
  size_t remaining = GetStringInfoLength(profile)-(i+5);
  size_t length = MagickMin(declared,remaining);
  if (~length >= 1)
    message=(char *) AcquireQuantumMemory(length+1UL,sizeof(*message));
  if (message != (char *) NULL)
    {
      /*
        Copy only the clamped length.
      */
      (void) memcpy(message,p+5,length);
      message[length]='\0';
      (void) ConcatenateString(&attribute,message);
      (void) ConcatenateString(&attribute,";");
      message=DestroyString(message);
    }
  return(attribute);
}

static void SetIPTCAttribute(const Image *image,const char *key,
  char *attribute,ExceptionInfo *exception)
{
  if ((attribute == (char *) NULL) || (*attribute == ';'))
    {
      if (attribute != (char *) NULL)
        attribute=DestroyString(attribute);
      return;
    }
  attribute[strlen(attribute)-1]='\0';
  (void) SetImageProperty((Image *) image,key,(const char *) attribute,
    exception);
  attribute=DestroyString(attribute);
}

static char *CollectIPTCAttribute(const StringInfo *profile,
  const long dataset,const long record)
{
  char
    *attribute;

  ssize_t
    i;

  attribute=(char *) NULL;
  for (i=0; i < (ssize_t) GetStringInfoLength(profile)-5; )
  {
    const unsigned char *p = GetStringInfoDatum(profile)+i;
  
    if (p[0] != 0x1c)  /* Look for IPTC marker */
      {
        i++;
        continue;
      }
    /*
      Dataset and record.
    */
    if (((long) p[1] == dataset) && ((long) p[2] == record))
      attribute=AppendIPTCDataset(attribute,profile,p,i);
    /*
      Advance past this record header + data.
    */
    i+=(((size_t) p[3] << 8) | (size_t) p[4])+5;
  }
  return(attribute);
}

static void GetIPTCProperty(const Image *image,const char *key,
  ExceptionInfo *exception)
{
  char
    *attribute;

  const StringInfo
    *profile;

  long
    count,
    dataset,
    record;

  profile=GetImageProfile(image,"iptc");
  if (profile == (StringInfo *) NULL)
    profile=GetImageProfile(image,"8bim");
  if (profile == (StringInfo *) NULL)
    return;
  count=MagickSscanf(key,"IPTC:%ld:%ld",&dataset,&record);
  if (count != 2)
    return;
  attribute=CollectIPTCAttribute(profile,dataset,record);
  SetIPTCAttribute(image,key,attribute,exception);
}

#endif
