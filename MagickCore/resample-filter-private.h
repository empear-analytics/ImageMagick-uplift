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

  MagickCore private declarations of resample.c: its EWA resampling options
  and the _ResampleFilter structure.
*/
#ifndef MAGICKCORE_RESAMPLE_FILTER_PRIVATE_H
#define MAGICKCORE_RESAMPLE_FILTER_PRIVATE_H

/*
  EWA Resampling Options
*/

/* select ONE resampling method */
#define EWA 1                 /* Normal EWA handling - raw or clamped */
                              /* if 0 then use "High Quality EWA" */
#define EWA_CLAMP 1           /* EWA Clamping from Nicolas Robidoux */

#define FILTER_LUT 1          /* Use a LUT rather then direct filter calls */

/* output debugging information */
#define DEBUG_ELLIPSE 0       /* output ellipse info for debug */
#define DEBUG_HIT_MISS 0      /* output hit/miss pixels (as gnuplot commands) */
#define DEBUG_NO_PIXEL_HIT 0  /* Make pixels that fail to hit anything - RED */

#if ! FILTER_DIRECT
#define WLUT_WIDTH 1024       /* size of the filter cache */
#endif

/*
  Typedef declarations.
*/
struct _ResampleFilter
{
  CacheView
    *view;

  Image
    *image;

  ExceptionInfo
    *exception;

  MagickBooleanType
    debug;

  /* Information about image being resampled */
  ssize_t
    image_area;

  PixelInterpolateMethod
    interpolate;

  VirtualPixelMethod
    virtual_pixel;

  FilterType
    filter;

  /* processing settings needed */
  MagickBooleanType
    limit_reached,
    do_interpolate,
    average_defined;

  PixelInfo
    average_pixel;

  /* current elliptical area being resampled around center point */
  double
    A, B, C,
    Vlimit, Ulimit, Uwidth, slope;

#if FILTER_LUT
  /* LUT of weights for filtered average in elliptical area */
  double
    filter_lut[WLUT_WIDTH];
#else
  /* Use a Direct call to the filter functions */
  ResizeFilter
    *filter_def;

  double
    F;
#endif

  /* the practical working support of the filter */
  double
    support;

  size_t
    signature;
};

#endif
