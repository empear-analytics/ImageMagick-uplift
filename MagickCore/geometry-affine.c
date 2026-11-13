/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%            GGGG  EEEEE   OOO   M   M  EEEEE  TTTTT  RRRR   Y   Y            %
%           G      E      O   O  MM MM  E        T    R   R   Y Y             %
%           G  GG  EEE    O   O  M M M  EEE      T    RRRR     Y              %
%           G   G  E      O   O  M   M  E        T    R R      Y              %
%            GGG   EEEEE   OOO   M   M  EEEEE    T    R  R     Y              %
%                                                                             %
%                   AAA   FFFFF  FFFFF  IIIII  N   N  EEEEE                   %
%                  A   A  F      F        I    NN  N  E                       %
%                  AAAAA  FFF    FFF      I    N N N  EEE                     %
%                  A   A  F      F        I    N  NN  E                       %
%                  A   A  F      F      IIIII  N   N  EEEEE                   %
%                                                                             %
%                                                                             %
%                      Affine and Scene Geometry Parsing                      %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%  Copyright 1999 ImageMagick Studio LLC, a non-profit organization           %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/geometry-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     I s S c e n e G e o m e t r y                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsSceneGeometry() returns MagickTrue if the geometry is a valid scene
%  specification (e.g. [1], [1-9], [1,7,4]).
%
%  The format of the IsSceneGeometry method is:
%
%      MagickBooleanType IsSceneGeometry(const char *geometry,
%        const MagickBooleanType pedantic)
%
%  A description of each parameter follows:
%
%    o geometry: This string is the geometry specification.
%
%    o pedantic: A value other than 0 invokes a more restrictive set of
%      conditions for a valid specification (e.g. [1], [1-4], [4-1]).
%
*/
MagickExport MagickBooleanType IsSceneGeometry(const char *geometry,
  const MagickBooleanType pedantic)
{
  char
    *p;

  double
    value;

  if (geometry == (const char *) NULL)
    return(MagickFalse);
  p=(char *) geometry;
  value=StringToDouble(geometry,&p);
  if (IsNaN(value) != 0)
    return(MagickFalse);
  if (value > (double) MAGICK_SSIZE_MAX)
    return(MagickFalse);
  if (value < (double) MAGICK_SSIZE_MIN)
    return(MagickFalse);
  if (p == geometry)
    return(MagickFalse);
  if (strspn(geometry,"0123456789-, ") != strlen(geometry))
    return(MagickFalse);
  if ((pedantic != MagickFalse) && (strchr(geometry,',') != (char *) NULL))
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e A f f i n e G e o m e t r y                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseAffineGeometry() returns an affine matrix as defined by a string of 4
%  to 6 comma/space separated floating point values.
%
%  The affine matrix determinant is checked for validity of the values.
%
%  The format of the ParseAffineGeometry method is:
%
%      MagickStatusType ParseAffineGeometry(const char *geometry,
%        AffineMatrix *affine_matrix,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry string (e.g. "1.0,0.0,0.0,1.0,3.2,1.2").
%
%    o affine_matrix: the affine matrix as defined by the geometry string.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickStatusType ParseAffineGeometry(const char *geometry,
  AffineMatrix *affine_matrix,ExceptionInfo *exception)
{
  char
    token[MagickPathExtent];

  const char
    *p;

  double
    determinant;

  MagickStatusType
    flags;

  ssize_t
    i;

  GetAffineMatrix(affine_matrix);
  flags=NoValue;
  p=(char *) geometry;
  for (i=0; (*p != '\0') && (i < 6); i++)
  {
    (void) GetNextToken(p,&p,MagickPathExtent,token);
    if (*token == ',')
      (void) GetNextToken(p,&p,MagickPathExtent,token);
    switch (i)
    {
      case 0:
      {
        affine_matrix->sx=StringToDouble(token,(char **) NULL);
        break;
      }
      case 1:
      {
        affine_matrix->rx=StringToDouble(token,(char **) NULL);
        break;
      }
      case 2:
      {
        affine_matrix->ry=StringToDouble(token,(char **) NULL);
        break;
      }
      case 3:
      {
        affine_matrix->sy=StringToDouble(token,(char **) NULL);
        break;
      }
      case 4:
      {
        affine_matrix->tx=StringToDouble(token,(char **) NULL);
        flags|=XValue;
        break;
      }
      case 5:
      {
        affine_matrix->ty=StringToDouble(token,(char **) NULL);
        flags|=YValue;
        break;
      }
    }
  }
  determinant=(affine_matrix->sx*affine_matrix->sy-affine_matrix->rx*
    affine_matrix->ry);
  if (fabs(determinant) < MagickEpsilon)
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
      "InvalidArgument","'%s' : 'Indeterminate Matrix'",geometry);
  return(flags);
}
