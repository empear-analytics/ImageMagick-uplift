/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        SSSSS  TTTTT   AAA   TTTTT  IIIII  SSSSS  TTTTT  IIIII   CCCC        %
%        SS       T    A   A    T      I    SS       T      I    C            %
%         SSS     T    AAAAA    T      I     SSS     T      I    C            %
%           SS    T    A   A    T      I       SS    T      I    C            %
%        SSSSS    T    A   A    T    IIIII  SSSSS    T    IIIII   CCCC        %
%                                                                             %
%               M   M   OOO   M   M  EEEEE  N   N  TTTTT  SSSSS               %
%               MM MM  O   O  MM MM  E      NN  N    T    SS                  %
%               M M M  O   O  M M M  EEE    N N N    T     SSS                %
%               M   M  O   O  M   M  E      N  NN    T       SS               %
%               M   M   OOO   M   M  EEEEE  N   N    T    SSSSS               %
%                                                                             %
%                                                                             %
%                       MagickCore Image Moments Method                       %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
%                                                                             %
%                                                                             %
%  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization         %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    https://imagemagick.org/license/                                         %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/accelerate-private.h"
#include "MagickCore/animate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/compress.h"
#include "MagickCore/constitute.h"
#include "MagickCore/display.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/list.h"
#include "MagickCore/image-private.h"
#include "MagickCore/magic.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/module.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/statistic.h"
#include "MagickCore/statistic-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/timer.h"
#include "MagickCore/utility.h"
#include "MagickCore/version.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e M o m e n t s                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageMoments() returns the normalized moments of one or more image
%  channels.
%
%  The format of the GetImageMoments method is:
%
%      ChannelMoments *GetImageMoments(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport ChannelMoments *GetImageMoments(const Image *image,
  ExceptionInfo *exception)
{
#define MaxNumberImageMoments  8

  CacheView
    *image_view;

  ChannelMoments
    *channel_moments;

  double
    channels,
    M00[2*MaxPixelChannels+1] = { 0.0 },
    M01[2*MaxPixelChannels+1] = { 0.0 },
    M02[2*MaxPixelChannels+1] = { 0.0 },
    M03[2*MaxPixelChannels+1] = { 0.0 },
    M10[2*MaxPixelChannels+1] = { 0.0 },
    M11[2*MaxPixelChannels+1] = { 0.0 },
    M12[2*MaxPixelChannels+1] = { 0.0 },
    M20[2*MaxPixelChannels+1] = { 0.0 },
    M21[2*MaxPixelChannels+1] = { 0.0 },
    M22[2*MaxPixelChannels+1] = { 0.0 },
    M30[2*MaxPixelChannels+1] = { 0.0 };

  PointInfo
    centroid[2*MaxPixelChannels+1] = {{ 0 }};

  ssize_t
    c,
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  channel_moments=(ChannelMoments *) AcquireQuantumMemory(MaxPixelChannels+1,
    sizeof(*channel_moments));
  if (channel_moments == (ChannelMoments *) NULL)
    return(channel_moments);
  (void) memset(channel_moments,0,(MaxPixelChannels+1)*
    sizeof(*channel_moments));
  image_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    /*
      Compute center of mass (centroid).
    */
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      ssize_t
        i;

      for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
      {
        double
          pixel;

        PixelChannel channel = GetPixelChannelChannel(image,i);
        PixelTrait traits = GetPixelChannelTraits(image,channel);
        if ((traits & UpdatePixelTrait) == 0)
          continue;
        pixel=QuantumScale*p[i];
        M00[channel]+=pixel;
        M00[CompositePixelChannel]+=pixel;
        M10[channel]+=x*pixel;
        M10[CompositePixelChannel]+=x*pixel;
        M01[channel]+=y*pixel;
        M01[CompositePixelChannel]+=y*pixel;
      }
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  for (c=0; c <= MaxPixelChannels; c++)
  {
    /*
       Compute center of mass (centroid).
    */
    centroid[c].x=M10[c]*MagickSafeReciprocal(M00[c]);
    centroid[c].y=M01[c]*MagickSafeReciprocal(M00[c]);
  }
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    /*
      Compute the image moments.
    */
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      ssize_t
        i;

      for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
      {
        PixelChannel channel = GetPixelChannelChannel(image,i);
        PixelTrait traits = GetPixelChannelTraits(image,channel);
        if ((traits & UpdatePixelTrait) == 0)
          continue;
        M11[channel]+=(x-centroid[channel].x)*(y-centroid[channel].y)*
          QuantumScale*(double) p[i];
        M11[CompositePixelChannel]+=(x-centroid[channel].x)*(y-
          centroid[channel].y)*QuantumScale*(double) p[i];
        M20[channel]+=(x-centroid[channel].x)*(x-centroid[channel].x)*
          QuantumScale*(double) p[i];
        M20[CompositePixelChannel]+=(x-centroid[channel].x)*(x-
          centroid[channel].x)*QuantumScale*(double) p[i];
        M02[channel]+=(y-centroid[channel].y)*(y-centroid[channel].y)*
          QuantumScale*(double) p[i];
        M02[CompositePixelChannel]+=(y-centroid[channel].y)*(y-
          centroid[channel].y)*QuantumScale*(double) p[i];
        M21[channel]+=(x-centroid[channel].x)*(x-centroid[channel].x)*
          (y-centroid[channel].y)*QuantumScale*(double) p[i];
        M21[CompositePixelChannel]+=(x-centroid[channel].x)*(x-
          centroid[channel].x)*(y-centroid[channel].y)*QuantumScale*(double)
          p[i];
        M12[channel]+=(x-centroid[channel].x)*(y-centroid[channel].y)*
          (y-centroid[channel].y)*QuantumScale*(double) p[i];
        M12[CompositePixelChannel]+=(x-centroid[channel].x)*(y-
          centroid[channel].y)*(y-centroid[channel].y)*QuantumScale*(double)
          p[i];
        M22[channel]+=(x-centroid[channel].x)*(x-centroid[channel].x)*
          (y-centroid[channel].y)*(y-centroid[channel].y)*QuantumScale*(double)
          p[i];
        M22[CompositePixelChannel]+=(x-centroid[channel].x)*(x-
          centroid[channel].x)*(y-centroid[channel].y)*(y-centroid[channel].y)*
          QuantumScale*(double) p[i];
        M30[channel]+=(x-centroid[channel].x)*(x-centroid[channel].x)*
          (x-centroid[channel].x)*QuantumScale*(double) p[i];
        M30[CompositePixelChannel]+=(x-centroid[channel].x)*(x-
          centroid[channel].x)*(x-centroid[channel].x)*QuantumScale*(double)
          p[i];
        M03[channel]+=(y-centroid[channel].y)*(y-centroid[channel].y)*
          (y-centroid[channel].y)*QuantumScale*(double) p[i];
        M03[CompositePixelChannel]+=(y-centroid[channel].y)*(y-
          centroid[channel].y)*(y-centroid[channel].y)*QuantumScale*(double)
          p[i];
      }
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  channels=(double) GetImageChannels(image);
  M00[CompositePixelChannel]/=channels;
  M01[CompositePixelChannel]/=channels;
  M02[CompositePixelChannel]/=channels;
  M03[CompositePixelChannel]/=channels;
  M10[CompositePixelChannel]/=channels;
  M11[CompositePixelChannel]/=channels;
  M12[CompositePixelChannel]/=channels;
  M20[CompositePixelChannel]/=channels;
  M21[CompositePixelChannel]/=channels;
  M22[CompositePixelChannel]/=channels;
  M30[CompositePixelChannel]/=channels;
  for (c=0; c <= MaxPixelChannels; c++)
  {
    /*
      Compute elliptical angle, major and minor axes, eccentricity, & intensity.
    */
    channel_moments[c].centroid=centroid[c];
    channel_moments[c].ellipse_axis.x=sqrt((2.0*MagickSafeReciprocal(M00[c]))*
      ((M20[c]+M02[c])+sqrt(4.0*M11[c]*M11[c]+(M20[c]-M02[c])*
       (M20[c]-M02[c]))));
    channel_moments[c].ellipse_axis.y=sqrt((2.0*MagickSafeReciprocal(M00[c]))*
      ((M20[c]+M02[c])-sqrt(4.0*M11[c]*M11[c]+(M20[c]-M02[c])*
       (M20[c]-M02[c]))));
    channel_moments[c].ellipse_angle=RadiansToDegrees(1.0/2.0*atan(2.0*
      M11[c]*MagickSafeReciprocal(M20[c]-M02[c])));
    if (fabs(M11[c]) < 0.0)
      {
        if ((fabs(M20[c]-M02[c]) >= 0.0) && ((M20[c]-M02[c]) < 0.0))
          channel_moments[c].ellipse_angle+=90.0;
      }
    else
      if (M11[c] < 0.0)
        {
          if (fabs(M20[c]-M02[c]) >= 0.0)
            {
              if ((M20[c]-M02[c]) < 0.0)
                channel_moments[c].ellipse_angle+=90.0;
              else
                channel_moments[c].ellipse_angle+=180.0;
            }
        }
      else
        if ((fabs(M20[c]-M02[c]) >= 0.0) && ((M20[c]-M02[c]) < 0.0))
          channel_moments[c].ellipse_angle+=90.0;
    channel_moments[c].ellipse_eccentricity=sqrt(1.0-(
      channel_moments[c].ellipse_axis.y*
      channel_moments[c].ellipse_axis.y*MagickSafeReciprocal(
      channel_moments[c].ellipse_axis.x*
      channel_moments[c].ellipse_axis.x)));
    channel_moments[c].ellipse_intensity=M00[c]*
      MagickSafeReciprocal(MagickPI*channel_moments[c].ellipse_axis.x*
      channel_moments[c].ellipse_axis.y+MagickEpsilon);
  }
  for (c=0; c <= MaxPixelChannels; c++)
  {
    /*
      Normalize image moments.
    */
    M10[c]=0.0;
    M01[c]=0.0;
    M11[c]*=MagickSafeReciprocal(pow(M00[c],1.0+(1.0+1.0)/2.0));
    M20[c]*=MagickSafeReciprocal(pow(M00[c],1.0+(2.0+0.0)/2.0));
    M02[c]*=MagickSafeReciprocal(pow(M00[c],1.0+(0.0+2.0)/2.0));
    M21[c]*=MagickSafeReciprocal(pow(M00[c],1.0+(2.0+1.0)/2.0));
    M12[c]*=MagickSafeReciprocal(pow(M00[c],1.0+(1.0+2.0)/2.0));
    M22[c]*=MagickSafeReciprocal(pow(M00[c],1.0+(2.0+2.0)/2.0));
    M30[c]*=MagickSafeReciprocal(pow(M00[c],1.0+(3.0+0.0)/2.0));
    M03[c]*=MagickSafeReciprocal(pow(M00[c],1.0+(0.0+3.0)/2.0));
    M00[c]=1.0;
  }
  image_view=DestroyCacheView(image_view);
  for (c=0; c <= MaxPixelChannels; c++)
  {
    /*
      Compute Hu invariant moments.
    */
    channel_moments[c].invariant[0]=M20[c]+M02[c];
    channel_moments[c].invariant[1]=(M20[c]-M02[c])*(M20[c]-M02[c])+4.0*M11[c]*
      M11[c];
    channel_moments[c].invariant[2]=(M30[c]-3.0*M12[c])*(M30[c]-3.0*M12[c])+
      (3.0*M21[c]-M03[c])*(3.0*M21[c]-M03[c]);
    channel_moments[c].invariant[3]=(M30[c]+M12[c])*(M30[c]+M12[c])+(M21[c]+
      M03[c])*(M21[c]+M03[c]);
    channel_moments[c].invariant[4]=(M30[c]-3.0*M12[c])*(M30[c]+M12[c])*
      ((M30[c]+M12[c])*(M30[c]+M12[c])-3.0*(M21[c]+M03[c])*(M21[c]+M03[c]))+
      (3.0*M21[c]-M03[c])*(M21[c]+M03[c])*(3.0*(M30[c]+M12[c])*(M30[c]+M12[c])-
      (M21[c]+M03[c])*(M21[c]+M03[c]));
    channel_moments[c].invariant[5]=(M20[c]-M02[c])*((M30[c]+M12[c])*(M30[c]+
      M12[c])-(M21[c]+M03[c])*(M21[c]+M03[c]))+4.0*M11[c]*(M30[c]+M12[c])*
      (M21[c]+M03[c]);
    channel_moments[c].invariant[6]=(3.0*M21[c]-M03[c])*(M30[c]+M12[c])*
      ((M30[c]+M12[c])*(M30[c]+M12[c])-3.0*(M21[c]+M03[c])*(M21[c]+M03[c]))-
      (M30[c]-3*M12[c])*(M21[c]+M03[c])*(3.0*(M30[c]+M12[c])*(M30[c]+M12[c])-
      (M21[c]+M03[c])*(M21[c]+M03[c]));
    channel_moments[c].invariant[7]=M11[c]*((M30[c]+M12[c])*(M30[c]+M12[c])-
      (M03[c]+M21[c])*(M03[c]+M21[c]))-(M20[c]-M02[c])*(M30[c]+M12[c])*
      (M03[c]+M21[c]);
  }
  if (y < (ssize_t) image->rows)
    channel_moments=(ChannelMoments *) RelinquishMagickMemory(channel_moments);
  return(channel_moments);
}
