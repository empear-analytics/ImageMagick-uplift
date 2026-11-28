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

  MagickCore private text included in place by accelerate-kernels-private.h,
  inside its accelerateKernels2 initialiser: the ResizeHorizontalFilter kernel, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
  STRINGIFY(
  __kernel __attribute__((reqd_work_group_size(256, 1, 1)))
    void ResizeHorizontalFilter(const __global CLQuantum *inputImage, const unsigned int number_channels,
      const unsigned int inputColumns, const unsigned int inputRows, __global CLQuantum *filteredImage,
      const unsigned int filteredColumns, const unsigned int filteredRows, const float xFactor,
      const int resizeFilterType, const int resizeWindowType, const __global float *resizeFilterCubicCoefficients,
      const float resizeFilterScale, const float resizeFilterSupport, const float resizeFilterWindowSupport,
      const float resizeFilterBlur, __local CLQuantum *inputImageCache, const int numCachedPixels,
      const unsigned int pixelPerWorkgroup, const unsigned int pixelChunkSize,
      __local float4 *outputPixelCache, __local float *densityCache, __local float *gammaCache)
  {
    // calculate the range of resized image pixels computed by this workgroup
    const unsigned int startX = get_group_id(0)*pixelPerWorkgroup;
    const unsigned int stopX = MagickMin(startX + pixelPerWorkgroup,filteredColumns);
    const unsigned int actualNumPixelToCompute = stopX - startX;

    // calculate the range of input image pixels to cache
    float scale = MagickMax(1.0f/xFactor+MagickEpsilon ,1.0f);
    const float support = MagickMax(scale*resizeFilterSupport,0.5f);
    scale = MagickSafeReciprocal(scale);

    const int cacheRangeStartX = MagickMax((int)((startX+0.5f)/xFactor+MagickEpsilon-support+0.5f),(int)(0));
    const int cacheRangeEndX = MagickMin((int)(cacheRangeStartX + numCachedPixels), (int)inputColumns);

    // cache the input pixels into local memory
    const unsigned int y = get_global_id(1);
    const unsigned int pos = getPixelIndex(number_channels, inputColumns, cacheRangeStartX, y);
    const unsigned int num_elements = (cacheRangeEndX - cacheRangeStartX) * number_channels;
    event_t e = async_work_group_copy(inputImageCache, inputImage + pos, num_elements, 0);
    wait_group_events(1, &e);

    unsigned int alpha_index = (number_channels == 4) || (number_channels == 2) ? number_channels - 1 : 0;
    unsigned int totalNumChunks = (actualNumPixelToCompute+pixelChunkSize-1)/pixelChunkSize;
    for (unsigned int chunk = 0; chunk < totalNumChunks; chunk++)
    {
      const unsigned int chunkStartX = startX + chunk*pixelChunkSize;
      const unsigned int chunkStopX = MagickMin(chunkStartX + pixelChunkSize, stopX);
      const unsigned int actualNumPixelInThisChunk = chunkStopX - chunkStartX;

      // determine which resized pixel computed by this workitem
      const unsigned int itemID = get_local_id(0);
      const unsigned int numItems = getNumWorkItemsPerPixel(actualNumPixelInThisChunk, get_local_size(0));

      const int pixelIndex = pixelToCompute(itemID, actualNumPixelInThisChunk, get_local_size(0));

      float4 filteredPixel = (float4)0.0f;
      float density = 0.0f;
      float gamma = 0.0f;
      // -1 means this workitem doesn't participate in the computation
      if (pixelIndex != -1)
      {
        // x coordinated of the resized pixel computed by this workitem
        const int x = chunkStartX + pixelIndex;

        // calculate how many steps required for this pixel
        const float bisect = (x+0.5)/xFactor+MagickEpsilon;
        const unsigned int start = (unsigned int)MagickMax(bisect-support+0.5f,0.0f);
        const unsigned int stop  = (unsigned int)MagickMin(bisect+support+0.5f,(float)inputColumns);
        const unsigned int n = stop - start;

        // calculate how many steps this workitem will contribute
        unsigned int numStepsPerWorkItem = n / numItems;
        numStepsPerWorkItem += ((numItems*numStepsPerWorkItem)==n?0:1);

        const unsigned int startStep = (itemID%numItems)*numStepsPerWorkItem;
        if (startStep < n)
        {
          const unsigned int stopStep = MagickMin(startStep+numStepsPerWorkItem, n);

          unsigned int cacheIndex = start+startStep-cacheRangeStartX;
          for (unsigned int i = startStep; i < stopStep; i++, cacheIndex++)
          {
            float weight = getResizeFilterWeight(resizeFilterCubicCoefficients,
              (ResizeWeightingFunctionType) resizeFilterType,
              (ResizeWeightingFunctionType) resizeWindowType,
              resizeFilterScale, resizeFilterWindowSupport,
              resizeFilterBlur, scale*(start + i - bisect + 0.5));

            float4 cp = (float4)0.0f;

            __local CLQuantum *p = inputImageCache + (cacheIndex*number_channels);
            cp.x = (float) *(p);
            if (number_channels > 2)
            {
              cp.y = (float) *(p + 1);
              cp.z = (float) *(p + 2);
            }

            if (alpha_index != 0)
            {
              cp.w = (float) *(p + alpha_index);

              float alpha = weight * QuantumScale * cp.w;

              filteredPixel.x += alpha * cp.x;
              filteredPixel.y += alpha * cp.y;
              filteredPixel.z += alpha * cp.z;
              filteredPixel.w += weight * cp.w;
              gamma += alpha;
            }
            else
              filteredPixel += ((float4) weight)*cp;

            density += weight;
          }
        }
      }

      // initialize the accumulators to zero
      if (itemID < actualNumPixelInThisChunk) {
        outputPixelCache[itemID] = (float4)0.0f;
        densityCache[itemID] = 0.0f;
        if (alpha_index != 0)
          gammaCache[itemID] = 0.0f;
      }
      barrier(CLK_LOCAL_MEM_FENCE);

      // accumulate the filtered pixel value and the density
      for (unsigned int i = 0; i < numItems; i++) {
        if (pixelIndex != -1) {
          if (itemID%numItems == i) {
            outputPixelCache[pixelIndex]+=filteredPixel;
            densityCache[pixelIndex]+=density;
            if (alpha_index != 0)
              gammaCache[pixelIndex]+=gamma;
          }
        }
        barrier(CLK_LOCAL_MEM_FENCE);
      }

      if (itemID < actualNumPixelInThisChunk)
      {
        float4 filteredPixel = outputPixelCache[itemID];

        float gamma = 0.0f;
        if (alpha_index != 0)
          gamma = gammaCache[itemID];

        float density = densityCache[itemID];
        if ((density != 0.0f) && (density != 1.0f))
        {
          density = MagickSafeReciprocal(density);
          filteredPixel *= (float4) density;
          if (alpha_index != 0)
            gamma *= density;
        }

        if (alpha_index != 0)
        {
          gamma = MagickSafeReciprocal(gamma);
          filteredPixel.x *= gamma;
          filteredPixel.y *= gamma;
          filteredPixel.z *= gamma;
        }

        WriteAllChannels(filteredImage, number_channels, filteredColumns, chunkStartX + itemID, y, filteredPixel);
      }
    }
  }
  )
