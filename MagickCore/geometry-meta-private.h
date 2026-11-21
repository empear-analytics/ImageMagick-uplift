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

  MagickCore private meta geometry methods.
*/
#ifndef MAGICKCORE_GEOMETRY_META_PRIVATE_H
#define MAGICKCORE_GEOMETRY_META_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _MetaGeometryInput
{
  const char
    *geometry;

  size_t
    stasis_width,
    stasis_height;
} MetaGeometryInput;

static inline MagickBooleanType IsMetaGeometryAspectKept(
  const MagickStatusType flags,const size_t *width,const size_t *height,
  const MetaGeometryInput *input)
{
  const size_t
    stasis_width = input->stasis_width,
    stasis_height = input->stasis_height;

  return((((flags & AspectValue) != 0) || ((*width == stasis_width) &&
    (*height == stasis_height))) ? MagickTrue : MagickFalse);
}

static void ShrinkGeometryToSize(size_t *width,size_t *height,
  const size_t stasis_width,const size_t stasis_height)
{
  if (stasis_width < *width)
    *width=stasis_width;
  if (stasis_height < *height)
    *height=stasis_height;
}

static void EnlargeGeometryToSize(size_t *width,size_t *height,
  const size_t stasis_width,const size_t stasis_height)
{
  if (stasis_width > *width)
    *width=stasis_width;
  if (stasis_height > *height)
    *height=stasis_height;
}

static double GetAspectScaleFactor(const MagickStatusType flags,
  const size_t *width,const size_t *height,const size_t stasis_width,
  const size_t stasis_height)
{
  double
    scale_factor;

  if ((stasis_width == 0) || (stasis_height == 0))
    scale_factor=1.0;
  else
    if (((flags & RhoValue) != 0) && (flags & SigmaValue) != 0)
      {
        scale_factor=(double) *width/(double) stasis_width;
        if ((flags & MinimumValue) == 0)
          {
            if (scale_factor > ((double) *height/(double) stasis_height))
              scale_factor=(double) *height/(double) stasis_height;
          }
        else
          if (scale_factor < ((double) *height/(double) stasis_height))
            scale_factor=(double) *height/(double) stasis_height;
      }
    else
      if ((flags & RhoValue) != 0)
        {
          scale_factor=(double) *width/(double) stasis_width;
          if (((flags & MinimumValue) != 0) &&
              (scale_factor < ((double) *width/(double) stasis_height)))
            scale_factor=(double) *width/(double) stasis_height;
        }
      else
        {
          scale_factor=(double) *height/(double) stasis_height;
          if (((flags & MinimumValue) != 0) &&
              (scale_factor < ((double) *height/(double) stasis_width)))
            scale_factor=(double) *height/(double) stasis_width;
        }
  return(scale_factor);
}

static void LimitGeometryArea(const char *geometry,size_t *width,
  size_t *height,const size_t stasis_width,const size_t stasis_height)
{
  GeometryInfo
    geometry_info;

  double
    area,
    distance;

  PointInfo
    scale;

  /*
    Geometry is a maximum area in pixels.
  */
  (void) ParseGeometry(geometry,&geometry_info);
  area=geometry_info.rho+sqrt(MagickEpsilon);
  distance=sqrt((double) stasis_width*stasis_height);
  scale.x=(double) stasis_width*MagickSafeReciprocal(distance*
    MagickSafeReciprocal(sqrt(area)));
  scale.y=(double) stasis_height*MagickSafeReciprocal(distance*
    MagickSafeReciprocal(sqrt(area)));
  if ((scale.x < (double) *width) || (scale.y < (double) *height))
    {
      *width=CastDoubleToSizeT((double) stasis_width*MagickSafeReciprocal(
        distance*MagickSafeReciprocal(sqrt(area)))+0.5);
      *height=CastDoubleToSizeT((double) stasis_height*MagickSafeReciprocal(
        distance*MagickSafeReciprocal(sqrt(area)))+0.5);
    }
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
