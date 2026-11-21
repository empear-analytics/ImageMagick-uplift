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

  MagickCore private constitute pixel methods.
*/
#ifndef MAGICKCORE_CONSTITUTE_PIXELS_PRIVATE_H
#define MAGICKCORE_CONSTITUTE_PIXELS_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static void SetImageDepthFromStorage(Image *image,const StorageType storage)
{
  switch (storage)
  {
    case CharPixel: image->depth=8*sizeof(unsigned char); break;
    case DoublePixel: image->depth=8*sizeof(double); break;
    case FloatPixel: image->depth=8*sizeof(float); break;
    case LongPixel: image->depth=8*sizeof(unsigned long); break;
    case LongLongPixel: image->depth=8*sizeof(MagickSizeType); break;
    case ShortPixel: image->depth=8*sizeof(unsigned short); break;
    default: break;
  }
}

static void SetImageTraitsFromMap(Image *image,const char *map)
{
  ssize_t
    i;

  size_t
    length;

  length=strlen(map);
  for (i=0; i < (ssize_t) length; i++)
  {
    switch (map[i])
    {
      case 'a':
      case 'A':
      case 'O':
      case 'o':
      {
        image->alpha_trait=BlendPixelTrait;
        break;
      }
      case 'C':
      case 'c':
      case 'm':
      case 'M':
      case 'Y':
      case 'y':
      case 'K':
      case 'k':
      {
        image->colorspace=CMYKColorspace;
        break;
      }
      case 'I':
      case 'i':
      {
        image->colorspace=GRAYColorspace;
        break;
      }
      default:
      {
        if (length == 1)
          image->colorspace=GRAYColorspace;
        break;
      }
    }
  }
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
