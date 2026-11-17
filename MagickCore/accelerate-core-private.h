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
#include "MagickCore/exception-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/opencl-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(MAGICKCORE_OPENCL_SUPPORT)

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

#endif /* MAGICKCORE_OPENCL_SUPPORT */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
