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

  MagickCore private methods shared by the OpenCL accelerated image methods:
  the acceleration checks, the OpenCL environment fetch and the host-side
  buffer, kernel and queue helpers.
*/
#ifndef MAGICKCORE_ACCELERATE_CORE_PRIVATE_H
#define MAGICKCORE_ACCELERATE_CORE_PRIVATE_H

#include "MagickCore/accelerate-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/opencl-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(MAGICKCORE_OPENCL_SUPPORT)

/*
  Define declarations.
*/
#define ALIGNED(pointer,type) ((((size_t)(pointer)) & (sizeof(type)-1)) == 0)

/*
  Method prototypes.
*/
static void
  TraceAccelerateImage(const Image *);

/*
  Helper functions.
*/
static inline MagickBooleanType IsUnsupportedColorspace(const Image *image)
{
  return((image->colorspace != RGBColorspace &&
      image->colorspace != sRGBColorspace &&
      image->colorspace != LinearGRAYColorspace &&
      image->colorspace != GRAYColorspace) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType HasImageMaskChannel(const Image *image)
{
  return((((image->channels & ReadMaskChannel) != 0) ||
      ((image->channels & WriteMaskChannel) != 0) ||
      ((image->channels & CompositeMaskChannel) != 0)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsUnsupportedVirtualPixelMethod(const Image *image)
{
  return(((GetImageVirtualPixelMethod(image) != UndefinedVirtualPixelMethod) &&
      (GetImageVirtualPixelMethod(image) != EdgeVirtualPixelMethod)) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType HasRGBOrRGBAPixelOrder(const Image *image)
{
  /* check if pixel order is RGB */
  if ((GetPixelChannelOffset(image,GreenPixelChannel) != 1) ||
      (GetPixelChannelOffset(image,BluePixelChannel) != 2))
    return(MagickFalse);

  if (image->number_channels == 3)
    return(MagickTrue);

  /* check if pixel order is RGBA */
  if (GetPixelChannelOffset(image,AlphaPixelChannel) != 3)
    return(MagickFalse);

  return(MagickTrue);
}

static inline MagickBooleanType HasSupportedPixelOrder(const Image *image)
{
  /* check if pixel order is R */
  if (GetPixelChannelOffset(image,RedPixelChannel) != 0)
    return(MagickFalse);

  if (image->number_channels == 1)
    return(MagickTrue);

  /* check if pixel order is RA */
  if ((image->number_channels == 2) &&
      (GetPixelChannelOffset(image,AlphaPixelChannel) == 1))
    return(MagickTrue);

  if (image->number_channels == 2)
    return(MagickFalse);

  return(HasRGBOrRGBAPixelOrder(image));
}

static inline MagickBooleanType checkAccelerateCondition(const Image* image)
{
  /* only direct class images are supported */
  if (image->storage_class != DirectClass)
    return(MagickFalse);

  /* check if the image's colorspace is supported */
  if (IsUnsupportedColorspace(image) != MagickFalse)
    return(MagickFalse);

  /* check if the virtual pixel method is compatible with the OpenCL implementation */
  if (IsUnsupportedVirtualPixelMethod(image) != MagickFalse)
    return(MagickFalse);

  /* check if the image has mask */
  if (HasImageMaskChannel(image) != MagickFalse)
    return(MagickFalse);

  if (image->number_channels > 4)
    return(MagickFalse);

  /* check if */
  if ((image->channel_mask != AllChannels) &&
      (image->channel_mask > 0x7ffffff))
    return(MagickFalse);

  return(HasSupportedPixelOrder(image));
}

static inline MagickCLEnv getOpenCLEnvironment(ExceptionInfo* exception)
{
  MagickCLEnv
    clEnv;

  clEnv=GetCurrentOpenCLEnv();
  if (clEnv == (MagickCLEnv) NULL)
    return((MagickCLEnv) NULL);

  if (clEnv->enabled == MagickFalse)
    return((MagickCLEnv) NULL);

  if (InitializeOpenCL(clEnv,exception) == MagickFalse)
    return((MagickCLEnv) NULL);

  return(clEnv);
}

static inline void ReleaseOpenCLBuffer(cl_mem buffer)
{
  if (buffer != (cl_mem) NULL)
    ReleaseOpenCLMemObject(buffer);
}

static inline void ReleaseKernel(cl_kernel kernel)
{
  if (kernel != (cl_kernel) NULL)
    ReleaseOpenCLKernel(kernel);
}

static inline void ReleaseAccelerateQueue(MagickCLDevice device,
  cl_command_queue queue)
{
  if (queue != (cl_command_queue) NULL)
    ReleaseOpenCLCommandQueue(device,queue);
  if (device != (MagickCLDevice) NULL)
    ReleaseOpenCLDevice(device);
}

static inline Image *DestroyUnfinishedImage(const MagickBooleanType outputReady,
  Image *filteredImage)
{
  if ((outputReady == MagickFalse) && (filteredImage != (Image *) NULL))
    filteredImage=DestroyImage(filteredImage);
  return(filteredImage);
}

static inline void DestroyAccelerateViews(CacheView *image_view,
  CacheView *filteredImage_view)
{
  image_view=DestroyCacheView(image_view);
  if (filteredImage_view != NULL)
    filteredImage_view=DestroyCacheView(filteredImage_view);
}

static inline MagickBooleanType HasUndefinedRGBATraits(const Image *image)
{
  return(((GetPixelRedTraits(image) == UndefinedPixelTrait) ||
      (GetPixelGreenTraits(image) == UndefinedPixelTrait) ||
      (GetPixelBlueTraits(image) == UndefinedPixelTrait) ||
      (GetPixelAlphaTraits(image) == UndefinedPixelTrait)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType checkAccelerateConditionRGBA(const Image* image)
{
  if (checkAccelerateCondition(image) == MagickFalse)
    return(MagickFalse);

  /* the order will be RGBA if the image has 4 channels */
  if (image->number_channels != 4)
    return(MagickFalse);

  if (HasUndefinedRGBATraits(image) != MagickFalse)
    return(MagickFalse);

  return(MagickTrue);
}

static inline MagickBooleanType IsLumaIntensityMethod(
  const PixelIntensityMethod method)
{
  return(((method == Rec601LumaPixelIntensityMethod) ||
    (method == Rec709LumaPixelIntensityMethod)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsLuminanceIntensityMethod(
  const PixelIntensityMethod method)
{
  return(((method == Rec601LuminancePixelIntensityMethod) ||
    (method == Rec709LuminancePixelIntensityMethod)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType checkPixelIntensity(const Image *image,
  const PixelIntensityMethod method)
{
  /* EncodePixelGamma and DecodePixelGamma are not supported */
  if ((IsLumaIntensityMethod(method) != MagickFalse) &&
      (image->colorspace == RGBColorspace))
    return(MagickFalse);

  if ((IsLuminanceIntensityMethod(method) != MagickFalse) &&
      (image->colorspace == sRGBColorspace))
    return(MagickFalse);

  return(MagickTrue);
}

static inline MagickBooleanType checkHistogramCondition(const Image *image,
  const PixelIntensityMethod method)
{
  /* ensure this is the only pass get in for now. */
  if ((image->channel_mask & SyncChannels) == 0)
    return MagickFalse;

  return(checkPixelIntensity(image,method));
}

typedef enum
{
  BasicAccelerateCheck,
  RGBAAccelerateCheck,
  HistogramAccelerateCheck
} AccelerateCheck;

static inline MagickBooleanType checkAccelerateKind(const Image *image,
  const AccelerateCheck check)
{
  if (check == BasicAccelerateCheck)
    return(checkAccelerateCondition(image));
  if (checkAccelerateConditionRGBA(image) == MagickFalse)
    return(MagickFalse);
  if (check == RGBAAccelerateCheck)
    return(MagickTrue);
  return(checkHistogramCondition(image,image->intensity));
}

static inline MagickCLEnv getAccelerateEnvironment(const Image *image,
  const AccelerateCheck check,const char *trace_function,
  ExceptionInfo *exception)
{
  assert(image != NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if ((trace_function != (const char *) NULL) &&
      (IsEventLogging() != MagickFalse))
    (void) LogMagickEvent(TraceEvent,__FILE__,trace_function,(const size_t)
      __LINE__,"%s",image->filename);
  if (checkAccelerateKind(image,check) == MagickFalse)
    return((MagickCLEnv) NULL);
  return(getOpenCLEnvironment(exception));
}

static inline cl_int get32BitChannelValue(const ChannelType channel)
{
#if defined(MAGICKCORE_64BIT_CHANNEL_MASK_SUPPORT)
  if (channel == AllChannels)
    return(0x7ffffff);
#endif
  return((cl_int) channel);
}

static inline cl_mem_flags GetInputBufferFlags(const void *inputPixels)
{
  cl_mem_flags
    mem_flags;

  if (ALIGNED(inputPixels,CLPixelPacket))
  {
    mem_flags = CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR;
  }
  else
  {
    mem_flags = CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR;
  }
  return(mem_flags);
}

static inline void TraceAccelerateImage(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
}

typedef struct _AccelerateImageBufferTargets
{
  MagickCLDevice
    *device;

  cl_command_queue
    *queue;

  cl_mem
    *imageBuffer;
} AccelerateImageBufferTargets;

static inline MagickBooleanType AcquireAccelerateImageBuffer(MagickCLEnv clEnv,
  const Image *image,ExceptionInfo *exception,
  const AccelerateImageBufferTargets *targets)
{
  MagickCLDevice
    *const device = targets->device;

  cl_command_queue
    *const queue = targets->queue;

  cl_mem
    *const imageBuffer = targets->imageBuffer;

  *device=RequestOpenCLDevice(clEnv);
  if (*device == (MagickCLDevice) NULL)
    return(MagickFalse);
  *queue=AcquireOpenCLCommandQueue(*device);
  if (*queue == (cl_command_queue) NULL)
    return(MagickFalse);
  *imageBuffer=GetAuthenticOpenCLBuffer(image,*device,exception);
  if (*imageBuffer == (cl_mem) NULL)
    return(MagickFalse);
  return(MagickTrue);
}

#endif /* MAGICKCORE_OPENCL_SUPPORT */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
