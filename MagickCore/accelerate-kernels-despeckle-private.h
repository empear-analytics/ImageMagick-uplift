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
  inside its accelerateKernels initialiser: the Despeckle kernels, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     D e s p e c k l e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

  STRINGIFY(

  __kernel void HullPass1(const __global CLPixelType *inputImage, __global CLPixelType *outputImage
  , const unsigned int imageWidth, const unsigned int imageHeight
  , const int2 offset, const int polarity, const int matte) {

    int x = get_global_id(0);
    int y = get_global_id(1);

    CLPixelType v = inputImage[y*imageWidth+x];

    int2 neighbor;
    neighbor.y = y + offset.y;
    neighbor.x = x + offset.x;

    int2 clampedNeighbor;
    clampedNeighbor.x = ClampToCanvas(neighbor.x, imageWidth);
    clampedNeighbor.y = ClampToCanvas(neighbor.y, imageHeight);

    CLPixelType r = (clampedNeighbor.x == neighbor.x
                     && clampedNeighbor.y == neighbor.y)?inputImage[clampedNeighbor.y*imageWidth+clampedNeighbor.x]
    :(CLPixelType)0;

    int sv[4];
    sv[0] = (int)v.x;
    sv[1] = (int)v.y;
    sv[2] = (int)v.z;
    sv[3] = (int)v.w;

    int sr[4];
    sr[0] = (int)r.x;
    sr[1] = (int)r.y;
    sr[2] = (int)r.z;
    sr[3] = (int)r.w;

    if (polarity > 0) {
      \n #pragma unroll 4\n
      for (unsigned int i = 0; i < 4; i++) {
        sv[i] = (sr[i] >= (sv[i]+ScaleCharToQuantum(2)))?(sv[i]+ScaleCharToQuantum(1)):sv[i];
      }
    }
    else {
      \n #pragma unroll 4\n
      for (unsigned int i = 0; i < 4; i++) {
        sv[i] = (sr[i] <= (sv[i]-ScaleCharToQuantum(2)))?(sv[i]-ScaleCharToQuantum(1)):sv[i];
      }

    }

    v.x = (CLQuantum)sv[0];
    v.y = (CLQuantum)sv[1];
    v.z = (CLQuantum)sv[2];

    if (matte!=0)
      v.w = (CLQuantum)sv[3];

    outputImage[y*imageWidth+x] = v;

    }


  )

  STRINGIFY(

  __kernel void HullPass2(const __global CLPixelType *inputImage, __global CLPixelType *outputImage
  , const unsigned int imageWidth, const unsigned int imageHeight
  , const int2 offset, const int polarity, const int matte) {

    int x = get_global_id(0);
    int y = get_global_id(1);

    CLPixelType v = inputImage[y*imageWidth+x];

    int2 neighbor, clampedNeighbor;

    neighbor.y = y + offset.y;
    neighbor.x = x + offset.x;
    clampedNeighbor.x = ClampToCanvas(neighbor.x, imageWidth);
    clampedNeighbor.y = ClampToCanvas(neighbor.y, imageHeight);

    CLPixelType r = (clampedNeighbor.x == neighbor.x
      && clampedNeighbor.y == neighbor.y)?inputImage[clampedNeighbor.y*imageWidth+clampedNeighbor.x]
    :(CLPixelType)0;


    neighbor.y = y - offset.y;
    neighbor.x = x - offset.x;
    clampedNeighbor.x = ClampToCanvas(neighbor.x, imageWidth);
    clampedNeighbor.y = ClampToCanvas(neighbor.y, imageHeight);

    CLPixelType s = (clampedNeighbor.x == neighbor.x
      && clampedNeighbor.y == neighbor.y)?inputImage[clampedNeighbor.y*imageWidth+clampedNeighbor.x]
    :(CLPixelType)0;


    int sv[4];
    sv[0] = (int)v.x;
    sv[1] = (int)v.y;
    sv[2] = (int)v.z;
    sv[3] = (int)v.w;

    int sr[4];
    sr[0] = (int)r.x;
    sr[1] = (int)r.y;
    sr[2] = (int)r.z;
    sr[3] = (int)r.w;

    int ss[4];
    ss[0] = (int)s.x;
    ss[1] = (int)s.y;
    ss[2] = (int)s.z;
    ss[3] = (int)s.w;

    if (polarity > 0) {
      \n #pragma unroll 4\n
      for (unsigned int i = 0; i < 4; i++) {
        //sv[i] = (ss[i] >= (sv[i]+ScaleCharToQuantum(2)) && sr[i] > sv[i] )   ? (sv[i]+ScaleCharToQuantum(1)):sv[i];
        //
        //sv[i] =(!( (int)(ss[i] >= (sv[i]+ScaleCharToQuantum(2))) && (int) (sr[i] > sv[i] ) ))  ? sv[i]:(sv[i]+ScaleCharToQuantum(1));
        //sv[i] =(( (int)( ss[i] < (sv[i]+ScaleCharToQuantum(2))) || (int) ( sr[i] <= sv[i] ) ))  ? sv[i]:(sv[i]+ScaleCharToQuantum(1));
        sv[i] =(( (int)( ss[i] < (sv[i]+ScaleCharToQuantum(2))) + (int) ( sr[i] <= sv[i] ) ) !=0)  ? sv[i]:(sv[i]+ScaleCharToQuantum(1));
      }
    }
    else {
      \n #pragma unroll 4\n
      for (unsigned int i = 0; i < 4; i++) {
        //sv[i] = (ss[i] <= (sv[i]-ScaleCharToQuantum(2)) && sr[i] < sv[i] )   ? (sv[i]-ScaleCharToQuantum(1)):sv[i];
        //
        //sv[i] = ( (int)(ss[i] <= (sv[i]-ScaleCharToQuantum(2)) ) + (int)( sr[i] < sv[i] ) ==0)   ? sv[i]:(sv[i]-ScaleCharToQuantum(1));
        sv[i] = (( (int)(ss[i] > (sv[i]-ScaleCharToQuantum(2))) + (int)( sr[i] >= sv[i] )) !=0)   ? sv[i]:(sv[i]-ScaleCharToQuantum(1));
      }
    }

    v.x = (CLQuantum)sv[0];
    v.y = (CLQuantum)sv[1];
    v.z = (CLQuantum)sv[2];

    if (matte!=0)
      v.w = (CLQuantum)sv[3];

    outputImage[y*imageWidth+x] = v;

    }
  )

