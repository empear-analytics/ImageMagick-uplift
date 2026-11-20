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
  (accelerate-contrast-stretch.c): their #ifdef RECREATEBUFFER release of the input buffer after the histogram pass, verbatim. No build defines
  RECREATEBUFFER; this is a fragment, without an include guard.
*/
#ifdef RECREATEBUFFER
  if (imageBuffer!=NULL)
    clEnv->library->clReleaseMemObject(imageBuffer);
#endif
