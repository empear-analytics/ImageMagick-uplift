/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               CCCC  H   H   AAA   N   N  N   N  EEEEE   L                   %
%              C      H   H  A   A  NN  N  NN  N  E       L                   %
%              C      HHHHH  AAAAA  N N N  N N N  EEE     L                   %
%              C      H   H  A   A  N  NN  N  NN  E       L                   %
%               CCCC  H   H  A   A  N   N  N   N  EEEEE   LLLLL               %
%                                                                             %
%                                                                             %
%                      MagickCore Image Channel Methods                       %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                               December 2003                                 %
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
#include "MagickCore/cache-private.h"
#include "MagickCore/channel.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/image.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/version.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C h a n n e l F x I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ChannelFxImage() applies a channel expression to the specified image.  The
%  expression consists of one or more channels, either mnemonic or numeric (e.g.
%  r, red, 0), separated by actions as follows:
%
%    <=>     exchange two channels (e.g. red<=>blue)
%    =>      copy one channel to another channel (e.g. red=>green)
%    =       assign a constant value to a channel (e.g. red=50%)
%    ,       write new image channels in the specified order (e.g. red, green)
%    ;       add a new output image for the next set of channel operations
%    |       move to the next input image for the source of channel data
%             If there are no more images in the list, | has no effect.
%
%  For example, to create 3 grayscale images from the red, green, and blue
%  channels of an image, use:
%
%    -channel-fx "red; green; blue"
%
%  A channel without an operation symbol implies separate (i.e, semicolon).
%
%  The format of the ChannelFxImage method is:
%
%      Image *ChannelFxImage(const Image *image,const char *expression,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o expression: A channel expression.
%
%    o exception: return any errors or warnings in this structure.
%
*/

typedef enum
{
  ExtractChannelOp,
  AssignChannelOp,
  ExchangeChannelOp,
  TransferChannelOp
} ChannelFx;

typedef struct _ChannelRow
{
  const Image
    *source_image;

  Image
    *destination_image;

  CacheView
    *source_view,
    *destination_view;

  PixelChannel
    destination_channel,
    source_channel;

  ChannelFx
    channel_op;

  Quantum
    pixel;

  size_t
    width;

  ExceptionInfo
    *exception;
} ChannelRow;

static inline MagickBooleanType IsChannelRowMissing(const Quantum *p,
  const Quantum *q)
{
  return(((p == (const Quantum *) NULL) || (q == (Quantum *) NULL)) ?
    MagickTrue : MagickFalse);
}

static void CopyChannelPixels(const ChannelRow *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) row->width; x++)
  {
    if (row->channel_op == AssignChannelOp)
      SetPixelChannel(row->destination_image,row->destination_channel,
        row->pixel,q);
    else
      SetPixelChannel(row->destination_image,row->destination_channel,
        GetPixelChannel(row->source_image,row->source_channel,p),q);
    p+=(ptrdiff_t) GetPixelChannels(row->source_image);
    q+=(ptrdiff_t) GetPixelChannels(row->destination_image);
  }
}

static inline MagickBooleanType IsChannelPairUndefined(
  const PixelTrait destination_traits,const PixelTrait source_traits)
{
  return(((destination_traits == UndefinedPixelTrait) ||
    (source_traits == UndefinedPixelTrait)) ? MagickTrue : MagickFalse);
}

static void ChannelImageRow(const ChannelRow *row,const ssize_t y,
  MagickBooleanType *status)
{
  PixelTrait
    destination_traits,
    source_traits;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(row->source_view,0,y,row->source_image->columns,1,
    row->exception);
  q=GetCacheViewAuthenticPixels(row->destination_view,0,y,
    row->destination_image->columns,1,row->exception);
  if (IsChannelRowMissing(p,q) != MagickFalse)
    {
      (*status)=MagickFalse;
      return;
    }
  destination_traits=GetPixelChannelTraits(row->destination_image,
    row->destination_channel);
  source_traits=GetPixelChannelTraits(row->source_image,row->source_channel);
  if (IsChannelPairUndefined(destination_traits,source_traits) != MagickFalse)
    return;
  CopyChannelPixels(row,p,q);
  if (SyncCacheViewAuthenticPixels(row->destination_view,row->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
}

typedef struct _ChannelPlan
{
  PixelChannel
    destination_channel;

  ChannelFx
    channel_op;

  const Image
    *source_image;

  PixelChannel
    source_channel;
} ChannelPlan;

static MagickBooleanType ChannelImage(Image *destination_image,
  const ChannelPlan *plan,const Quantum pixel,ExceptionInfo *exception)
{
  const ChannelFx
    channel_op = plan->channel_op;

  const Image
    *const source_image = plan->source_image;

  const PixelChannel
    destination_channel = plan->destination_channel,
    source_channel = plan->source_channel;

  CacheView
    *source_view,
    *destination_view;

  MagickBooleanType
    status = MagickTrue;

  size_t
    height,
    width;

  ssize_t
    y;

  /*
    Copy source channel to destination.
  */
  height=MagickMin(source_image->rows,destination_image->rows);
  width=MagickMin(source_image->columns,destination_image->columns);
  source_view=AcquireVirtualCacheView(source_image,exception);
  destination_view=AcquireAuthenticCacheView(destination_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(source_image,source_image,height,4)
#endif
  for (y=0; y < (ssize_t) height; y++)
  {
    const ChannelRow
      row = { source_image, destination_image, source_view, destination_view,
        destination_channel, source_channel, channel_op, pixel, width,
        exception };

    ChannelImageRow(&row,y,&status);
  }
  destination_view=DestroyCacheView(destination_view);
  source_view=DestroyCacheView(source_view);
  return(status);
}

static MagickBooleanType AddDestinationMetaChannel(
  Image *destination_image,const PixelChannel destination_channel,
  const char *token,ExceptionInfo *exception)
{
  PixelTrait
    traits;

  switch (destination_channel)
  {
    case MetaPixelChannels:
    default:
    {
      traits=GetPixelChannelTraits(destination_image,
        destination_channel);
      if (traits != UndefinedPixelTrait)
        break;
      (void) SetPixelMetaChannels(destination_image,
        GetPixelMetaChannels(destination_image)+1,exception);
      traits=GetPixelChannelTraits(destination_image,
       destination_channel);
      if (traits == UndefinedPixelTrait)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),
            CorruptImageError,"MissingImageChannel","`%s'",token);
          return(MagickFalse);
        }
      break;
    }
  }
  return(MagickTrue);
}

static MagickBooleanType AddDestinationMaskChannel(
  Image *destination_image,const PixelChannel destination_channel,
  const char *token,ExceptionInfo *exception)
{
  switch (destination_channel)
  {
    case CompositeMaskPixelChannel:
    {
      destination_image->channels=(ChannelType)
        (destination_image->channels | CompositeMaskChannel);
      break;
    }
    case ReadMaskPixelChannel:
    {
      destination_image->channels=(ChannelType)
        (destination_image->channels | ReadMaskChannel);
      break;
    }
    case WriteMaskPixelChannel:
    {
      destination_image->channels=(ChannelType)
        (destination_image->channels | WriteMaskChannel);
      break;
    }
    default:
      return(AddDestinationMetaChannel(destination_image,destination_channel,
        token,exception));
  }
  return(MagickTrue);
}

static MagickBooleanType RegisterDestinationChannel(
  Image *destination_image,const PixelChannel destination_channel,
  const char *token,ExceptionInfo *exception)
{
  switch (destination_channel)
  {
    case RedPixelChannel:
    case GreenPixelChannel:
    case BluePixelChannel:
    case BlackPixelChannel:
    case AlphaPixelChannel:
    case IndexPixelChannel:
      break;
    default:
      return(AddDestinationMaskChannel(destination_image,destination_channel,
        token,exception));
  }
  return(MagickTrue);
}

static MagickBooleanType ParseDestinationChannel(Image *destination_image,
  const char *token,ssize_t *i,ExceptionInfo *exception)
{
  (*i)=ParsePixelChannelOption(token);
  if (LocaleCompare(token,"alpha") == 0)
    destination_image->alpha_trait=BlendPixelTrait;
  if ((*i) < 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionError,"UnrecognizedChannelType","`%s'",token);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType CheckSourceChannel(const Image *source_image,
  const PixelChannel source_channel,const char *token,
  ExceptionInfo *exception)
{
  PixelTrait
    traits;

  traits=GetPixelChannelTraits(source_image,source_channel);
  if (traits == UndefinedPixelTrait)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        CorruptImageError,"MissingImageChannel","`%s'",token);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void ParseTransferOperator(ChannelFx *channel_op,const char **p,
  char *token)
{
  if (*token == '>')
    {
      if ((*channel_op) != ExchangeChannelOp)
        (*channel_op)=TransferChannelOp;
      (void) GetNextToken((*p),p,MagickPathExtent,token);
    }
}

static void ParseChannelOperator(ChannelFx *channel_op,const char **p,
  char *token)
{
  (*channel_op)=ExtractChannelOp;
  (void) GetNextToken((*p),p,MagickPathExtent,token);
  if (*token == '<')
    {
      (*channel_op)=ExchangeChannelOp;
      (void) GetNextToken((*p),p,MagickPathExtent,token);
    }
  if (*token == '=')
    {
      if ((*channel_op) != ExchangeChannelOp)
        (*channel_op)=AssignChannelOp;
      (void) GetNextToken((*p),p,MagickPathExtent,token);
    }
  ParseTransferOperator(channel_op,p,token);
}

static void AdvanceDestinationChannel(const ChannelFx channel_op,
  ChannelType *channel_mask,PixelChannel *destination_channel)
{
  switch (channel_op)
  {
    case ExtractChannelOp:
    {
      (*channel_mask)=(ChannelType) ((*channel_mask) |
        (MagickLLConstant(1) << (*destination_channel)));
      (*destination_channel)=(PixelChannel) ((*destination_channel)+1);
      break;
    }
    default:
      break;
  }
}

#define ChannelFxImageTag  "ChannelFx/Image"

static MagickBooleanType SetChannelFxProgress(const Image *source_image,
  const char *p,const char *expression)
{
  MagickBooleanType
    status;

  status=SetImageProgress(source_image,ChannelFxImageTag,p-expression,
    strlen(expression));
  if (status == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static void SetGrayChannelImage(Image *destination_image,
  const ChannelFx channel_op,const ssize_t channels,ExceptionInfo *exception)
{
  if ((channel_op == ExtractChannelOp) && (channels == 1))
    {
      (void) SetPixelMetaChannels(destination_image,0,exception);
      (void) SetImageColorspace(destination_image,GRAYColorspace,exception);
    }
}

typedef struct _ChannelFxApply
{
  PixelChannel
    destination_channel;

  ChannelFx
    channel_op;

  const Image
    *source_image;

  PixelChannel
    source_channel;

  double
    pixel;

  ExceptionInfo
    *exception;
} ChannelFxApply;

typedef struct _ChannelFxTargets
{
  Image
    **destination_image;

  ssize_t
    *channels;
} ChannelFxTargets;

static MagickBooleanType ApplyChannelOperation(const ChannelFxApply *apply,
  const ChannelFxTargets *targets)
{
  const ChannelFx
    channel_op = apply->channel_op;

  const double
    pixel = apply->pixel;

  const Image
    *const source_image = apply->source_image;

  const PixelChannel
    destination_channel = apply->destination_channel,
    source_channel = apply->source_channel;

  ExceptionInfo
    *const exception = apply->exception;

  ChannelPlan
    plan;

  Image
    **const destination_image = targets->destination_image;

  MagickBooleanType
    status;

  ssize_t
    *const channels = targets->channels;

  plan.destination_channel=destination_channel;
  plan.channel_op=channel_op;
  plan.source_image=source_image;
  plan.source_channel=source_channel;
  status=ChannelImage((*destination_image),&plan,ClampToQuantum(pixel),
    exception);
  if (status == MagickFalse)
    {
      (*destination_image)=DestroyImageList((*destination_image));
      return(MagickFalse);
    }
  (*channels)++;
  if (channel_op == ExchangeChannelOp)
    {
      plan.destination_channel=source_channel;
      plan.channel_op=channel_op;
      plan.source_image=source_image;
      plan.source_channel=destination_channel;
      status=ChannelImage((*destination_image),&plan,ClampToQuantum(pixel),
        exception);
      if (status == MagickFalse)
        {
          (*destination_image)=DestroyImageList((*destination_image));
          return(MagickFalse);
        }
      (*channels)++;
    }
  return(MagickTrue);
}

static MagickBooleanType AcquireChannelCanvas(const Image *source_image,
  Image **canvas,ExceptionInfo *exception)
{
  (*canvas)=CloneImage(source_image,0,0,MagickTrue,exception);
  if ((*canvas) == (Image *) NULL)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType AppendChannelCanvas(Image **destination_image,
  Image *canvas,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  AppendImageToList(destination_image,canvas);
  (*destination_image)=GetLastImageInList((*destination_image));
  status=SetImageStorageClass((*destination_image),DirectClass,exception);
  if (status == MagickFalse)
    {
      (*destination_image)=GetLastImageInList((*destination_image));
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void AdvanceSourceImage(const Image **source_image,const char **p,
  char *token)
{
  if (GetNextImageInList((*source_image)) != (Image *) NULL)
    (*source_image)=GetNextImageInList((*source_image));
  (void) GetNextToken((*p),p,MagickPathExtent,token);
}

static ChannelType AddChannelMaskOption(ChannelType channel_mask,
  const char *token)
{
  channel_mask=(ChannelType) (channel_mask |
    (MagickLLConstant(1) << ParseChannelOption(token)));
  return(channel_mask);
}

typedef struct _ChannelFxDestination
{
  ChannelFx
    channel_op;

  const Image
    *image;

  Image
    *destination_image;

  ExceptionInfo
    *exception;
} ChannelFxDestination;

typedef struct _ChannelFxDestinationTargets
{
  double
    *pixel;

  ssize_t
    *i;

  PixelChannel
    *destination_channel;

  ChannelType
    *channel_mask;

  const char
    **p;
} ChannelFxDestinationTargets;

static MagickBooleanType ParseChannelOperands(
  const ChannelFxDestination *destination,char *token,
  const ChannelFxDestinationTargets *targets)
{
  const ChannelFx
    channel_op = destination->channel_op;

  const Image
    *const image = destination->image;

  Image
    *const destination_image = destination->destination_image;

  ExceptionInfo
    *const exception = destination->exception;

  double
    *const pixel = targets->pixel;

  ssize_t
    *const i = targets->i;

  PixelChannel
    *const destination_channel = targets->destination_channel;

  ChannelType
    *const channel_mask = targets->channel_mask;

  const char
    **const p = targets->p;

  if (channel_op == AssignChannelOp)
    *pixel=StringToDoubleInterval(token,(double) QuantumRange+1.0);
  else
    if (ParseDestinationChannel(destination_image,token,i,
          exception) == MagickFalse)
      {
        return(MagickFalse);
      }
  *destination_channel=(PixelChannel) *i;
  if ((image->colorspace != UndefinedColorspace) &&
      (RegisterDestinationChannel(destination_image,*destination_channel,
        token,exception) == MagickFalse))
    {
      return(MagickFalse);
    }
  *channel_mask=AddChannelMaskOption(*channel_mask,token);
  (void) GetNextToken(*p,p,MagickPathExtent,token);
  return(MagickTrue);
}

static MagickBooleanType ParseChannelDestination(
  const ChannelFxDestination *parse,char *token,
  const ChannelFxDestinationTargets *destination_targets)
{
  const ChannelFx
    channel_op = parse->channel_op;

  const Image
    *const image = parse->image;

  Image
    *const destination_image = parse->destination_image;

  ExceptionInfo
    *const exception = parse->exception;

  ChannelFxDestination
    destination;

  switch (channel_op)
  {
    case AssignChannelOp:
    case ExchangeChannelOp:
    case TransferChannelOp:
    {
      destination.channel_op=channel_op;
      destination.image=image;
      destination.destination_image=destination_image;
      destination.exception=exception;
      if (ParseChannelOperands(&destination,token,destination_targets) ==
          MagickFalse)
        {
          return(MagickFalse);
        }
      break;
    }
    default:
      break;
  }
  return(MagickTrue);
}

typedef struct _ChannelFxSeparator
{
  ChannelFx
    channel_op;

  ExceptionInfo
    *exception;

  Image
    **destination_image;

  ssize_t
    *channels;

  PixelChannel
    *destination_channel;

  ChannelType
    *channel_mask;

  const char
    **p;

  const Image
    **source_image;
} ChannelFxSeparator;

static MagickBooleanType TryFxSeparator(
  const ChannelFxSeparator *separator,char *token,Image **result)
{
  const ChannelFx
    channel_op = separator->channel_op;

  ExceptionInfo
    *const exception = separator->exception;

  Image
    **const destination_image = separator->destination_image;

  ssize_t
    *const channels = separator->channels;

  PixelChannel
    *const destination_channel = separator->destination_channel;

  ChannelType
    *const channel_mask = separator->channel_mask;

  const char
    **const p = separator->p;

  const Image
    **const source_image = separator->source_image;

  switch (*token)
  {
    case ',':
    {
      (void) GetNextToken((*p),p,MagickPathExtent,token);
      break;
    }
    case '|':
    {
      AdvanceSourceImage(source_image,p,token);
      break;
    }
    case ';':
    {
      Image
        *canvas;

      (void) SetPixelChannelMask((*destination_image),(*channel_mask));
      SetGrayChannelImage((*destination_image),channel_op,(*channels),
        exception);
      if (AcquireChannelCanvas((*source_image),&canvas,exception) ==
          MagickFalse)
        {
          (*destination_image)=DestroyImageList((*destination_image));
          (*result)=(*destination_image);
          return(MagickTrue);
        }
      if (AppendChannelCanvas(destination_image,canvas,exception) ==
          MagickFalse)
        {
          (*result)=(Image *) NULL;
          return(MagickTrue);
        }
      (void) GetNextToken((*p),p,MagickPathExtent,token);
      (*channels)=0;
      (*destination_channel)=RedPixelChannel;
      (*channel_mask)=(*destination_image)->channel_mask;
      break;
    }
    default:
      break;
  }
  return(MagickFalse);
}

typedef struct _ChannelFxTokens
{
  const Image
    *image;

  const char
    *expression;

  ExceptionInfo
    *exception;

  ChannelType
    channel_mask;
} ChannelFxTokens;

static Image *ParseChannelFxTokens(const ChannelFxTokens *tokens,
  const char *p,const Image *source_image,Image *destination_image)
{
  ExceptionInfo
    *const exception = tokens->exception;

  ChannelFx
    channel_op = ExtractChannelOp;

  ChannelType
    channel_mask = tokens->channel_mask;

  char
    token[MagickPathExtent] = "";

  double
    pixel = 0.0;

  PixelChannel
    source_channel,
    destination_channel = RedPixelChannel;

  ssize_t
    channels = 0;

  (void) GetNextToken(p,&p,MagickPathExtent,token);
  while (*token != '\0')
  {
    const ChannelFxTargets
      targets = { &destination_image, &channels };

    ssize_t
      i;

    const ChannelFxDestinationTargets
      destination_targets = { &pixel, &i, &destination_channel, &channel_mask,
        &p };

    const ChannelFxSeparator
      separator = { channel_op, exception, &destination_image, &channels,
        &destination_channel, &channel_mask, &p, &source_image };

    /*
      Interpret channel expression.
    */
    if (TryFxSeparator(&separator,token,&destination_image) != MagickFalse)
      return(destination_image);
    i=ParsePixelChannelOption(token);
    source_channel=(PixelChannel) i;
    if (CheckSourceChannel(source_image,source_channel,token,
          exception) == MagickFalse)
      {
        destination_image=DestroyImageList(destination_image);
        return(destination_image);
      }
    ParseChannelOperator(&channel_op,&p,token);
    {
      const ChannelFxDestination
        parse = { channel_op, tokens->image, destination_image, exception };

      if (ParseChannelDestination(&parse,token,&destination_targets) ==
          MagickFalse)
        {
          destination_image=DestroyImageList(destination_image);
          return(destination_image);
        }
    }
    {
      const ChannelFxApply
        apply = { destination_channel, channel_op, source_image,
          source_channel, pixel, exception };

      if (ApplyChannelOperation(&apply,&targets) == MagickFalse)
        break;
    }
    AdvanceDestinationChannel(channel_op,&channel_mask,&destination_channel);
    if (SetChannelFxProgress(source_image,p,tokens->expression) == MagickFalse)
      break;
  }
  if (destination_image == (Image *) NULL)
    return(destination_image);
  (void) SetPixelChannelMask(destination_image,channel_mask);
  SetGrayChannelImage(destination_image,channel_op,channels,exception);
  return(GetFirstImageInList(destination_image));
}

MagickExport Image *ChannelFxImage(const Image *image,const char *expression,
  ExceptionInfo *exception)
{
  ChannelType
    channel_mask;

  const char
    *p;

  const Image
    *source_image;

  ChannelFxTokens
    tokens;

  Image
    *destination_image;

  MagickBooleanType
    status = MagickTrue;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  p=expression;
  source_image=image;
  destination_image=CloneImage(image,0,0,MagickTrue,exception);
  if (destination_image == (Image *) NULL)
    return((Image *) NULL);
  if (expression == (const char *) NULL)
    return(destination_image);
  status=SetImageStorageClass(destination_image,DirectClass,exception);
  if (status == MagickFalse)
    {
      destination_image=GetLastImageInList(destination_image);
      return((Image *) NULL);
    }
  channel_mask=destination_image->channel_mask;
  tokens.image=image;
  tokens.expression=expression;
  tokens.exception=exception;
  tokens.channel_mask=channel_mask;
  return(ParseChannelFxTokens(&tokens,p,source_image,destination_image));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e A l p h a C h a n n e l                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageAlphaChannel() returns MagickFalse if the image alpha channel is
%  not activated.  That is, the image is RGB rather than RGBA or CMYK rather
%  than CMYKA.
%
%  The format of the GetImageAlphaChannel method is:
%
%      MagickBooleanType GetImageAlphaChannel(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport MagickBooleanType GetImageAlphaChannel(const Image *image)
{
  assert(image != (const Image *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickCoreSignature);
  return(image->alpha_trait != UndefinedPixelTrait ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S e p a r a t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SeparateImage() separates a channel from the image and returns it as a
%  grayscale image.
%
%  The format of the SeparateImage method is:
%
%      Image *SeparateImage(const Image *image,const ChannelType channel,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o channel: the image channel.
%
%    o exception: return any errors or warnings in this structure.
%
*/
#define GetChannelBit(mask,bit)  (((size_t) (mask) >> (size_t) (bit)) & 0x01)

typedef struct _SeparatePixels
{
  const Image
    *image,
    *separate_image;

  ChannelType
    channel_type;
} SeparatePixels;

static void SeparatePixelChannel(const SeparatePixels *pixels,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const ChannelType
    channel_type = pixels->channel_type;

  const Image
    *const image = pixels->image,
    *const separate_image = pixels->separate_image;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits == UndefinedPixelTrait) ||
        (GetChannelBit(channel_type,channel) == 0))
      continue;
    SetPixelChannel(separate_image,GrayPixelChannel,p[i],q);
  }
}

typedef struct _SeparateRow
{
  const Image
    *image;

  Image
    *separate_image;

  CacheView
    *image_view,
    *separate_view;

  ChannelType
    channel_type;

  ExceptionInfo
    *exception;
} SeparateRow;

static void SeparateRowPixels(const SeparateRow *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  SeparatePixels
    pixels;

  ssize_t
    x;

  pixels.image=row->image;
  pixels.separate_image=row->separate_image;
  pixels.channel_type=row->channel_type;
  for (x=0; x < (ssize_t) row->image->columns; x++)
  {
    SetPixelChannel(row->separate_image,GrayPixelChannel,(Quantum) 0,q);
    SeparatePixelChannel(&pixels,p,q);
    p+=(ptrdiff_t) GetPixelChannels(row->image);
    q+=(ptrdiff_t) GetPixelChannels(row->separate_image);
  }
}

static MagickBooleanType SeparateImageRow(const SeparateRow *row,
  const ssize_t y,MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(row->image_view,0,y,row->image->columns,1,
    row->exception);
  q=QueueCacheViewAuthenticPixels(row->separate_view,0,y,
    row->separate_image->columns,1,row->exception);
  if (IsChannelRowMissing(p,q) != MagickFalse)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  SeparateRowPixels(row,p,q);
  if (SyncCacheViewAuthenticPixels(row->separate_view,row->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

#define SeparateImageTag  "Separate/Image"

static void SetSeparateProgress(const Image *image,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,SeparateImageTag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

static MagickBooleanType SeparateImageRows(const Image *image,
  Image *separate_image,const ChannelType channel_type,
  ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *separate_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  separate_view=AcquireAuthenticCacheView(separate_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const SeparateRow
      row = { image, separate_image, image_view, separate_view, channel_type,
        exception };

    if (SeparateImageRow(&row,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetSeparateProgress(image,progress,&status);
      }
  }
  separate_view=DestroyCacheView(separate_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

static void SetSeparateImageAttributes(const Image *image,
  Image *separate_image,ExceptionInfo *exception)
{
  separate_image->alpha_trait=UndefinedPixelTrait;
  (void) SetImageColorspace(separate_image,GRAYColorspace,exception);
  separate_image->gamma=image->gamma;
}

static Image *FinishSeparateImage(Image *separate_image,
  const MagickBooleanType status)
{
  (void) SetImageChannelMask(separate_image,AllChannels);
  if (status == MagickFalse)
    separate_image=DestroyImage(separate_image);
  return(separate_image);
}

MagickExport Image *SeparateImage(const Image *image,
  const ChannelType channel_type,ExceptionInfo *exception)
{
  Image
    *separate_image;

  MagickBooleanType
    status;

  /*
    Initialize separate image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  separate_image=CloneImage(image,0,0,MagickTrue,exception);
  if (separate_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(separate_image,DirectClass,exception) == MagickFalse)
    {
      separate_image=DestroyImage(separate_image);
      return((Image *) NULL);
    }
  SetSeparateImageAttributes(image,separate_image,exception);
  /*
    Separate image.
  */
  status=SeparateImageRows(image,separate_image,channel_type,exception);
  return(FinishSeparateImage(separate_image,status));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S e p a r a t e I m a g e s                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SeparateImages() returns a separate grayscale image for each channel
%  specified.
%
%  The format of the SeparateImages method is:
%
%      Image *SeparateImages(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsChannelNotUpdated(const PixelTrait traits)
{
  return(((traits == UndefinedPixelTrait) || ((traits & UpdatePixelTrait) == 0)) ?
    MagickTrue : MagickFalse);
}

MagickExport Image *SeparateImages(const Image *image,ExceptionInfo *exception)
{
  Image
    *images,
    *separate_image;

  ssize_t
    i;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  images=NewImageList();
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if (IsChannelNotUpdated(traits) != MagickFalse)
      continue;
    separate_image=SeparateImage(image,(ChannelType)
      (MagickLLConstant(1) << channel),exception);
    if (separate_image != (Image *) NULL)
      AppendImageToList(&images,separate_image);
  }
  if (images == (Image *) NULL)
    images=SeparateImage(image,UndefinedChannel,exception);
  return(images);
}

