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

  MagickCore private methods of profile.c: the LCMS helpers of ProfileImage,
  from the LCMSInfo typedefs to SetImageTypeFromSignature, in their
  MAGICKCORE_LCMS_DELEGATE branch; included by that file only.
*/
#ifndef MAGICKCORE_PROFILE_LCMS_PRIVATE_H
#define MAGICKCORE_PROFILE_LCMS_PRIVATE_H

#if defined(MAGICKCORE_LCMS_DELEGATE)

typedef struct _LCMSInfo
{
  ColorspaceType
    colorspace;

  cmsUInt32Number
    type;

  size_t
    channels;

  cmsHPROFILE
    profile;

  int
    intent;

  double
    scale[4],
    translate[4];

  void
    **magick_restrict pixels;
} LCMSInfo;

#if LCMS_VERSION < 2060
static void* cmsGetContextUserData(cmsContext ContextID)
{
  return(ContextID);
}

static cmsContext cmsCreateContext(void *magick_unused(Plugin),void *UserData)
{
  magick_unreferenced(Plugin);
  return((cmsContext) UserData);
}

static void cmsSetLogErrorHandlerTHR(cmsContext magick_unused(ContextID),
  cmsLogErrorHandlerFunction Fn)
{
  magick_unreferenced(ContextID);
  cmsSetLogErrorHandler(Fn);
}

static void cmsDeleteContext(cmsContext magick_unused(ContextID))
{
  magick_unreferenced(ContextID);
}
#endif

static void **DestroyPixelTLS(void **pixels)
{
  ssize_t
    i;

  if (pixels == (void **) NULL)
    return((void **) NULL);
  for (i=0; i < (ssize_t) GetMagickResourceLimit(ThreadResource); i++)
    if (pixels[i] != (void *) NULL)
      pixels[i]=RelinquishMagickMemory(pixels[i]);
  pixels=(void **) RelinquishMagickMemory(pixels);
  return(pixels);
}

static void **AcquirePixelTLS(const size_t columns,const size_t channels,
  MagickBooleanType highres)
{
  ssize_t
    i;

  size_t
    number_threads;

  size_t
    size;

  void
    **pixels;

  number_threads=(size_t) GetMagickResourceLimit(ThreadResource);
  pixels=(void **) AcquireQuantumMemory(number_threads,sizeof(*pixels));
  if (pixels == (void **) NULL)
    return((void **) NULL);
  (void) memset(pixels,0,number_threads*sizeof(*pixels));
  size=sizeof(double);
  if (highres == MagickFalse)
    size=sizeof(Quantum);
  for (i=0; i < (ssize_t) number_threads; i++)
  {
    pixels[i]=AcquireQuantumMemory(columns,channels*size);
    if (pixels[i] == (void *) NULL)
      return(DestroyPixelTLS(pixels));
  }
  return(pixels);
}

static cmsHTRANSFORM *DestroyTransformTLS(cmsHTRANSFORM *transform)
{
  ssize_t
    i;

  assert(transform != (cmsHTRANSFORM *) NULL);
  for (i=0; i < (ssize_t) GetMagickResourceLimit(ThreadResource); i++)
    if (transform[i] != (cmsHTRANSFORM) NULL)
      cmsDeleteTransform(transform[i]);
  transform=(cmsHTRANSFORM *) RelinquishMagickMemory(transform);
  return(transform);
}

static cmsHTRANSFORM *AcquireTransformTLS(const LCMSInfo *source_info,
  const LCMSInfo *target_info,const cmsUInt32Number flags,
  cmsContext cms_context)
{
  cmsHTRANSFORM
    *transform;

  size_t
    number_threads;

  ssize_t
    i;

  number_threads=(size_t) GetMagickResourceLimit(ThreadResource);
  transform=(cmsHTRANSFORM *) AcquireQuantumMemory(number_threads,
    sizeof(*transform));
  if (transform == (cmsHTRANSFORM *) NULL)
    return((cmsHTRANSFORM *) NULL);
  (void) memset(transform,0,number_threads*sizeof(*transform));
  for (i=0; i < (ssize_t) number_threads; i++)
  {
    transform[i]=cmsCreateTransformTHR(cms_context,source_info->profile,
      source_info->type,target_info->profile,target_info->type,
      (cmsUInt32Number) target_info->intent,flags);
    if (transform[i] == (cmsHTRANSFORM) NULL)
      return(DestroyTransformTLS(transform));
  }
  return(transform);
}

static void CMSExceptionHandler(cmsContext context,cmsUInt32Number severity,
  const char *message)
{
  CMSExceptionInfo
    *cms_exception;

  ExceptionInfo
    *exception;

  Image
    *image;

  cms_exception=(CMSExceptionInfo *) cmsGetContextUserData(context);
  if (cms_exception == (CMSExceptionInfo *) NULL)
    return;
  exception=cms_exception->exception;
  if (exception == (ExceptionInfo *) NULL)
    return;
  image=cms_exception->image;
  if (image == (Image *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageWarning,
        "UnableToTransformColorspace","`%s'","unknown context");
      return;
    }
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TransformEvent,GetMagickModule(),"lcms: #%u, %s",
      severity,message != (char *) NULL ? message : "no message");
  (void) ThrowMagickException(exception,GetMagickModule(),ImageWarning,
    "UnableToTransformColorspace","`%s', %s (#%u)",image->filename,
    message != (char *) NULL ? message : "no message",severity);
}

static void StoreTargetDoublePixels(const int id,const Image* image,
  const LCMSInfo *target_info,Quantum *q)
{
#define SetLCMSPixel(target_info,pixel,index) ClampToQuantum( \
  target_info->scale[index]*(((double) QuantumRange*(double) pixel)+ \
  target_info->translate[index]))

  double
    *p;

  ssize_t
    x;

  p=(double *) target_info->pixels[id];
  q-=GetPixelChannels(image)*image->columns;
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (target_info->channels == 1)
      SetPixelGray(image,SetLCMSPixel(target_info,*p,0),q);
    else
      SetPixelRed(image,SetLCMSPixel(target_info,*p,0),q);
    p++;
    if (target_info->channels > 1)
      {
        SetPixelGreen(image,SetLCMSPixel(target_info,*p,1),q);
        p++;
        SetPixelBlue(image,SetLCMSPixel(target_info,*p,2),q);
        p++;
      }
    if (target_info->channels > 3)
      {
        SetPixelBlack(image,SetLCMSPixel(target_info,*p,3),q);
        p++;
      }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void TransformDoublePixels(const int id,const Image* image,
  const LCMSInfo *source_info,const LCMSInfo *target_info,
  const cmsHTRANSFORM *transform,Quantum *q)
{
#define GetLCMSPixel(source_info,pixel,index) \
  (source_info->scale[index]*(((double) QuantumScale*(double) pixel)+ \
  source_info->translate[index]))

  double
    *p;

  ssize_t
    x;

  p=(double *) source_info->pixels[id];
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    *p++=GetLCMSPixel(source_info,GetPixelRed(image,q),0);
    if (source_info->channels > 1)
      {
        *p++=GetLCMSPixel(source_info,GetPixelGreen(image,q),1);
        *p++=GetLCMSPixel(source_info,GetPixelBlue(image,q),2);
      }
    if (source_info->channels > 3)
      *p++=GetLCMSPixel(source_info,GetPixelBlack(image,q),3);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  cmsDoTransform(transform[id],source_info->pixels[id],target_info->pixels[id],
    (unsigned int) image->columns);
  StoreTargetDoublePixels(id,image,target_info,q);
}

static void StoreTargetQuantumPixels(const int id,const Image* image,
  const LCMSInfo *target_info,Quantum *q)
{
  Quantum
    *p;

  ssize_t
    x;

  p=(Quantum *) target_info->pixels[id];
  q-=GetPixelChannels(image)*image->columns;
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (target_info->channels == 1)
      SetPixelGray(image,*p++,q);
    else
      SetPixelRed(image,*p++,q);
    if (target_info->channels > 1)
      {
        SetPixelGreen(image,*p++,q);
        SetPixelBlue(image,*p++,q);
      }
    if (target_info->channels > 3)
      SetPixelBlack(image,*p++,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void TransformQuantumPixels(const int id,const Image* image,
  const LCMSInfo *source_info,const LCMSInfo *target_info,
  const cmsHTRANSFORM *transform,Quantum *q)
{
  Quantum
    *p;

  ssize_t
    x;

  p=(Quantum *) source_info->pixels[id];
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    *p++=GetPixelRed(image,q);
    if (source_info->channels > 1)
      {
        *p++=GetPixelGreen(image,q);
        *p++=GetPixelBlue(image,q);
      }
    if (source_info->channels > 3)
      *p++=GetPixelBlack(image,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  cmsDoTransform(transform[id],source_info->pixels[id],target_info->pixels[id],
    (unsigned int) image->columns);
  StoreTargetQuantumPixels(id,image,target_info,q);
}

static inline void SetLCMSInfoTranslate(LCMSInfo *info,const double translate)
{
  info->translate[0]=translate;
  info->translate[1]=translate;
  info->translate[2]=translate;
  info->translate[3]=translate;
}

static inline void SetLCMSInfoScale(LCMSInfo *info,const double scale)
{
  info->scale[0]=scale;
  info->scale[1]=scale;
  info->scale[2]=scale;
  info->scale[3]=scale;
}

static void SetLCMSInfoIntent(const Image *image,LCMSInfo *target_info)
{
  switch (image->rendering_intent)
  {
    case AbsoluteIntent:
    {
      (*target_info).intent=INTENT_ABSOLUTE_COLORIMETRIC;
      break;
    }
    case PerceptualIntent:
    {
      (*target_info).intent=INTENT_PERCEPTUAL;
      break;
    }
    case RelativeIntent:
    {
      (*target_info).intent=INTENT_RELATIVE_COLORIMETRIC;
      break;
    }
    case SaturationIntent:
    {
      (*target_info).intent=INTENT_SATURATION;
      break;
    }
    default:
    {
      (*target_info).intent=INTENT_PERCEPTUAL;
      break;
    }
  }
}

typedef struct _LCMSInfoScales
{
  double
    cmyk_scale,
    lab_scale_l,
    lab_scale_ab,
    lab_translate_ab;
} LCMSInfoScales;

static MagickBooleanType SetLCMSInfoFromColorSignature(LCMSInfo *info,
  const cmsColorSpaceSignature signature,const MagickBooleanType highres,
  const LCMSInfoScales *scales)
{
  switch (signature)
  {
    case cmsSigLabData:
    {
      info->colorspace=LabColorspace;
      if (highres != MagickFalse)
        {
          info->type=(cmsUInt32Number) TYPE_Lab_DBL;
          info->scale[0]=scales->lab_scale_l;
          info->scale[1]=scales->lab_scale_ab;
          info->scale[2]=scales->lab_scale_ab;
#if !defined(MAGICKCORE_HDRI_SUPPORT)
          info->translate[1]=scales->lab_translate_ab;
          info->translate[2]=scales->lab_translate_ab;
#endif
        }
#if (MAGICKCORE_QUANTUM_DEPTH == 8)
      else
        info->type=(cmsUInt32Number) TYPE_Lab_8;
#elif (MAGICKCORE_QUANTUM_DEPTH == 16)
      else
        info->type=(cmsUInt32Number) TYPE_Lab_16;
#endif
      break;
    }
    case cmsSigRgbData:
    {
      info->colorspace=sRGBColorspace;
      if (highres != MagickFalse)
        info->type=(cmsUInt32Number) TYPE_RGB_DBL;
#if (MAGICKCORE_QUANTUM_DEPTH == 8)
      else
        info->type=(cmsUInt32Number) TYPE_RGB_8;
#elif (MAGICKCORE_QUANTUM_DEPTH == 16)
      else
        info->type=(cmsUInt32Number) TYPE_RGB_16;
#endif
      break;
    }
    case cmsSigXYZData:
    {
      info->colorspace=XYZColorspace;
      if (highres != MagickFalse)
        info->type=(cmsUInt32Number) TYPE_XYZ_DBL;
#if (MAGICKCORE_QUANTUM_DEPTH == 8)
      else
        info->type=(cmsUInt32Number) TYPE_XYZ_8;
#elif (MAGICKCORE_QUANTUM_DEPTH == 16)
      else
        info->type=(cmsUInt32Number) TYPE_XYZ_16;
#endif
      break;
    }
    default:
      return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType SetLCMSInfoFromSignature(LCMSInfo *info,
  const cmsColorSpaceSignature signature,const MagickBooleanType highres,
  const LCMSInfoScales *scales)
{
  switch (signature)
  {
    case cmsSigCmykData:
    {
      info->colorspace=CMYKColorspace;
      info->channels=4;
      if (highres != MagickFalse)
        {
          info->type=(cmsUInt32Number) TYPE_CMYK_DBL;
          SetLCMSInfoScale(info,scales->cmyk_scale);
        }
#if (MAGICKCORE_QUANTUM_DEPTH == 8)
      else
        info->type=(cmsUInt32Number) TYPE_CMYK_8;
#elif (MAGICKCORE_QUANTUM_DEPTH == 16)
      else
        info->type=(cmsUInt32Number) TYPE_CMYK_16;
#endif
      break;
    }
    case cmsSigGrayData:
    {
      info->colorspace=GRAYColorspace;
      info->channels=1;
      if (highres != MagickFalse)
        info->type=(cmsUInt32Number) TYPE_GRAY_DBL;
#if (MAGICKCORE_QUANTUM_DEPTH == 8)
      else
        info->type=(cmsUInt32Number) TYPE_GRAY_8;
#elif (MAGICKCORE_QUANTUM_DEPTH == 16)
      else
        info->type=(cmsUInt32Number) TYPE_GRAY_16;
#endif
      break;
    }
    default:
      return(SetLCMSInfoFromColorSignature(info,signature,highres,scales));
  }
  return(MagickTrue);
}

static void SetImageTypeFromSignature(Image *image,
  const cmsColorSpaceSignature signature)
{
  switch (signature)
  {
    case cmsSigRgbData:
    {
      image->type=image->alpha_trait == UndefinedPixelTrait ?
        TrueColorType : TrueColorAlphaType;
      break;
    }
    case cmsSigCmykData:
    {
      image->type=image->alpha_trait == UndefinedPixelTrait ?
        ColorSeparationType : ColorSeparationAlphaType;
      break;
    }
    case cmsSigGrayData:
    {
      image->type=image->alpha_trait == UndefinedPixelTrait ?
        GrayscaleType : GrayscaleAlphaType;
      break;
    }
    default:
      break;
  }
}
#endif

#endif
