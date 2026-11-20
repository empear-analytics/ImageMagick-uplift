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

  MagickCore private text included in place by ComputeEqualizeImage
  (accelerate-histogram.c) and ComputeContrastStretchImage
  (accelerate-contrast-stretch.c): their #ifdef RECREATEBUFFER re-creation of the input buffer before the second kernel, verbatim. No build defines
  RECREATEBUFFER; this is a fragment, without an include guard.
*/
#ifdef RECREATEBUFFER
  /* If the host pointer is aligned to the size of CLPixelPacket,
     then use the host buffer directly from the GPU; otherwise,
     create a buffer on the GPU and copy the data over */
  if (ALIGNED(inputPixels,CLPixelPacket))
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR;
  }
  else
  {
    mem_flags = CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR;
  }
  /* create a CL buffer from image pixel buffer */
  length = image->columns * image->rows;
  imageBuffer = clEnv->library->clCreateBuffer(context, mem_flags, length * sizeof(CLPixelPacket), (void*)inputPixels, &clStatus);
  if (clStatus != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception, GetMagickModule(), ResourceLimitWarning, "clEnv->library->clCreateBuffer failed.",".");
    goto cleanup;
  }
#endif
