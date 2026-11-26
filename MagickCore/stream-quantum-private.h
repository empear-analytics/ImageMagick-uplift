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

  MagickCore private methods of stream.c: the per-channel quantum writers of
  each storage type, from StreamCharAlphaIndexQuantum to StreamShortQuantum;
  included by that file only.
*/
#ifndef MAGICKCORE_STREAM_QUANTUM_PRIVATE_H
#define MAGICKCORE_STREAM_QUANTUM_PRIVATE_H

static void StreamCharAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,unsigned char *q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelAlpha(image,p));
      break;
    }
    case OpacityQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelOpacity(image,p));
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=ScaleQuantumToChar(GetPixelBlack(image,p));
      break;
    }
    case IndexQuantum:
    {
      *q=ScaleQuantumToChar(ClampToQuantum(GetPixelIntensity(image,p)));
      break;
    }
    default:
      break;
  }
}

static void StreamCharQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,unsigned char *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelRed(image,p));
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelGreen(image,p));
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelBlue(image,p));
      break;
    }
    default:
    {
      StreamCharAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

typedef struct _StreamDoubleChannel
{
  const Image
    *image;

  const QuantumInfo
    *quantum_info;

  QuantumType
    quantum;

  const Quantum
    *p;

  double
    *q;
} StreamDoubleChannel;

static void StreamDoubleAlphaIndexQuantum(const StreamDoubleChannel *channel)
{
  const Image
    *image = channel->image;

  const QuantumInfo
    *quantum_info = channel->quantum_info;

  const QuantumType
    quantum = channel->quantum;

  const Quantum
    *p = channel->p;

  double
    *q = channel->q;

  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelAlpha(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case OpacityQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelOpacity(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=(double) ((QuantumScale*(double) GetPixelBlack(image,p))*
          quantum_info->scale+quantum_info->minimum);
      break;
    }
    case IndexQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelIntensity(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    default:
      *q=0;
  }
}

static void StreamDoubleQuantum(const Image *image,
  const QuantumInfo *quantum_info,const QuantumType quantum,const Quantum *p,
  double *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    default:
    {
      StreamDoubleChannel
        channel;

      channel.image=image;
      channel.quantum_info=quantum_info;
      channel.quantum=quantum;
      channel.p=p;
      channel.q=q;
      StreamDoubleAlphaIndexQuantum(&channel);
      break;
    }
  }
}

typedef struct _StreamFloatChannel
{
  const Image
    *image;

  const QuantumInfo
    *quantum_info;

  QuantumType
    quantum;

  const Quantum
    *p;

  float
    *q;
} StreamFloatChannel;

static void StreamFloatAlphaIndexQuantum(const StreamFloatChannel *channel)
{
  const Image
    *image = channel->image;

  const QuantumInfo
    *quantum_info = channel->quantum_info;

  const QuantumType
    quantum = channel->quantum;

  const Quantum
    *p = channel->p;

  float
    *q = channel->q;

  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelAlpha(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case OpacityQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelOpacity(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=(float) ((QuantumScale*(double) GetPixelBlack(image,p))*
          quantum_info->scale+quantum_info->minimum);
      break;
    }
    case IndexQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelIntensity(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    default:
      *q=0;
  }
}

static void StreamFloatQuantum(const Image *image,
  const QuantumInfo *quantum_info,const QuantumType quantum,const Quantum *p,
  float *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    default:
    {
      StreamFloatChannel
        channel;

      channel.image=image;
      channel.quantum_info=quantum_info;
      channel.quantum=quantum;
      channel.p=p;
      channel.q=q;
      StreamFloatAlphaIndexQuantum(&channel);
      break;
    }
  }
}

static void StreamLongAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,unsigned int *q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelAlpha(image,p));
      break;
    }
    case OpacityQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelOpacity(image,p));
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=ScaleQuantumToLong(GetPixelBlack(image,p));
      break;
    }
    case IndexQuantum:
    {
      *q=ScaleQuantumToLong(ClampToQuantum(GetPixelIntensity(image,p)));
      break;
    }
    default:
      break;
  }
}

static void StreamLongQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,unsigned int *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelRed(image,p));
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelGreen(image,p));
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelBlue(image,p));
      break;
    }
    default:
    {
      StreamLongAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void StreamLongLongAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,MagickSizeType *q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelAlpha(image,p));
      break;
    }
    case OpacityQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelOpacity(image,p));
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=ScaleQuantumToLongLong(GetPixelBlack(image,p));
      break;
    }
    case IndexQuantum:
    {
      *q=ScaleQuantumToLongLong(ClampToQuantum(
        GetPixelIntensity(image,p)));
      break;
    }
    default:
      *q=0;
  }
}

static void StreamLongLongQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,MagickSizeType *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelRed(image,p));
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelGreen(image,p));
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelBlue(image,p));
      break;
    }
    default:
    {
      StreamLongLongAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void StreamQuantumAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,Quantum *q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=GetPixelAlpha(image,p);
      break;
    }
    case OpacityQuantum:
    {
      *q=GetPixelOpacity(image,p);
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=GetPixelBlack(image,p);
      break;
    }
    case IndexQuantum:
    {
      *q=ClampToQuantum(GetPixelIntensity(image,p));
      break;
    }
    default:
      *q=(Quantum) 0;
  }
}

static void StreamQuantumQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,Quantum *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=GetPixelRed(image,p);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=GetPixelGreen(image,p);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=GetPixelBlue(image,p);
      break;
    }
    default:
    {
      StreamQuantumAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void StreamShortAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,unsigned short *q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelAlpha(image,p));
      break;
    }
    case OpacityQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelOpacity(image,p));
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=ScaleQuantumToShort(GetPixelBlack(image,p));
      break;
    }
    case IndexQuantum:
    {
      *q=ScaleQuantumToShort(ClampToQuantum(
        GetPixelIntensity(image,p)));
      break;
    }
    default:
      break;
  }
}

static void StreamShortQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,unsigned short *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelRed(image,p));
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelGreen(image,p));
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelBlue(image,p));
      break;
    }
    default:
    {
      StreamShortAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

#endif
