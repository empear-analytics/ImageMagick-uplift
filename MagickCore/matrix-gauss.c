/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  M   M   AAA   TTTTT  RRRR   IIIII  X   X                   %
%                  MM MM  A   A    T    R   R    I     X X                    %
%                  M M M  AAAAA    T    RRRR     I      X                     %
%                  M   M  A   A    T    R R      I     X X                    %
%                  M   M  A   A    T    R  R   IIIII  X   X                   %
%                                                                             %
%                       GGGG   AAA   U   U  SSSSS  SSSSS                      %
%                      G      A   A  U   U  SS     SS                         %
%                      G  GG  AAAAA  U   U   SSS    SSS                       %
%                      G   G  A   A  U   U     SS     SS                      %
%                       GGG   A   A   UUU   SSSSS  SSSSS                      %
%                                                                             %
%                                                                             %
%                          Gauss-Jordan Elimination                           %
%                                                                             %
%                            Software Design                                  %
%                                 Cristy                                      %
%                              August 2007                                    %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/matrix.h"
#include "MagickCore/matrix-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G a u s s J o r d a n E l i m n a t i o n                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GaussJordanElimination() returns a matrix in reduced row echelon form,
%  while simultaneously reducing and thus solving the augmented results
%  matrix.
%
%  The format of the GaussJordanElimination method is:
%
%      MagickBooleanType GaussJordanElimination(double **matrix,
%        double **vectors,const size_t rank,const size_t number_vectors)
%
%  A description of each parameter follows:
%
%    o matrix: the matrix to be reduced, as an 'array of row pointers'.
%
%    o vectors: the additional matrix argumenting the matrix for row reduction.
%             Producing an 'array of column vectors'.
%
%    o rank:  The size of the matrix (both rows and columns).
%             Also represents the number terms that need to be solved.
%
%    o number_vectors: Number of vectors columns, argumenting the above matrix.
%             Usually 1, but can be more for more complex equation solving.
%
%  Note that the 'matrix' is given as a 'array of row pointers' of rank size.
%  That is values can be assigned as   matrix[row][column]   where 'row' is
%  typically the equation, and 'column' is the term of the equation.
%  That is the matrix is in the form of a 'row first array'.
%
%  However 'vectors' is a 'array of column pointers' which can have any number
%  of columns, with each column array the same 'rank' size as 'matrix'.
%
%  This allows for simpler handling of the results, especially is only one
%  column 'vector' is all that is required to produce the desired solution.
%
%  For example, the 'vectors' can consist of a pointer to a simple array of
%  doubles.  when only one set of simultaneous equations is to be solved from
%  the given set of coefficient weighted terms.
%
%     double **matrix = AcquireMagickMatrix(8UL,8UL);
%     double coefficients[8];
%     ...
%     GaussJordanElimination(matrix,&coefficients,8UL,1UL);
%
%  However by specifying more 'columns' (as an 'array of vector columns', you
%  can use this function to solve a set of 'separable' equations.
%
%  For example a distortion function where u = U(x,y) v = V(x,y)
%  And the functions U() and V() have separate coefficients, but are being
%  generated from a common x,y->u,v  data set.
%
%  Another example is generation of a color gradient from a set of colors at
%  specific coordinates, such as a list x,y -> r,g,b,a.
%
%  You can also use the 'vectors' to generate an inverse of the given 'matrix'
%  though as a 'column first array' rather than a 'row first array'. For
%  details see http://en.wikipedia.org/wiki/Gauss-Jordan_elimination
%
*/
static inline MagickBooleanType IsPivotMemoryMissing(const ssize_t *columns,
  const ssize_t *rows,const ssize_t *pivots)
{
  return(((columns == (ssize_t *) NULL) || (rows == (ssize_t *) NULL) ||
    (pivots == (ssize_t *) NULL)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsSingularPivot(const ssize_t column,
  const ssize_t row,const long double max)
{
  return(((column == -1) || (row == -1) || (fabsl(max) < LDBL_MIN)) ?
    MagickTrue : MagickFalse);
}

static void CopyBackMatrix(double **matrix,long double **hp_matrix,
  const size_t rank)
{
  ssize_t
    i,
    j;

  for (i=0; i < (ssize_t) rank; i++)
    for (j=0; j < (ssize_t) rank; j++)
      matrix[i][j]=(double)hp_matrix[i][j];
}

#define GaussJordanSwap(x,y) \
{ \
  double temp = (x); \
  (x)=(y); \
  (y)=temp; \
}
#define GaussJordanSwapLD(x,y) \
{ \
  long double temp = (x); \
  (x)=(y); \
  (y)=temp; \
}
#define ThrowGaussJordanException() \
{ \
  for (i=0; i < (ssize_t) rank; i++) \
    hp_matrix[i]=(long double *) RelinquishMagickMemory(hp_matrix[i]); \
  hp_matrix=(long double **) RelinquishMagickMemory(hp_matrix); \
  if (pivots != (ssize_t *) NULL) \
    pivots=(ssize_t *) RelinquishMagickMemory(pivots); \
  if (rows != (ssize_t *) NULL) \
    rows=(ssize_t *) RelinquishMagickMemory(rows); \
  if (columns != (ssize_t *) NULL) \
    columns=(ssize_t *) RelinquishMagickMemory(columns); \
  return(MagickFalse); \
}

typedef struct _PivotSearch
{
  long double
    **hp_matrix;

  const ssize_t
    *pivots;

  size_t
    rank;
} PivotSearch;

typedef struct _PivotTargets
{
  long double
    *max;

  ssize_t
    *row,
    *column;
} PivotTargets;

static void ScanPivotRow(const PivotSearch *search,
  const PivotTargets *targets,const ssize_t j)
{
  long double
    **const hp_matrix = search->hp_matrix;

  const ssize_t
    *const pivots = search->pivots;

  const size_t
    rank = search->rank;

  long double
    *const max = targets->max;

  ssize_t
    *const row = targets->row,
    *const column = targets->column;

  ssize_t
    k;

  for (k=0; k < (ssize_t) rank; k++)
    if ((pivots[k] == 0) && (fabsl(hp_matrix[j][k]) > *max))
      {
        *max=fabsl(hp_matrix[j][k]);
        *row=j;
        *column=k;
      }
}

static void FindPivot(const PivotSearch *search,long double *max,
  ssize_t *row,ssize_t *column)
{
  const PivotTargets
    targets = { max, row, column };

  const ssize_t
    *const pivots = search->pivots;

  const size_t
    rank = search->rank;

  ssize_t
    j;

  *column=(-1);
  *row=(-1);
  for (j=0; j < (ssize_t) rank; j++)
    if (pivots[j] != 1)
      ScanPivotRow(search,&targets,j);
}

static void SwapMatrixRows(long double **hp_matrix,const size_t rank,
  const ssize_t row,const ssize_t column)
{
  ssize_t
    k;

  for (k=0; k < (ssize_t) rank; k++)
    GaussJordanSwapLD(hp_matrix[row][k],hp_matrix[column][k]);
}

static void SwapVectorEntries(double **vectors,const size_t number_vectors,
  const ssize_t row,const ssize_t column)
{
  ssize_t
    k;

  for (k=0; k < (ssize_t) number_vectors; k++)
    GaussJordanSwap(vectors[k][row],vectors[k][column]);
}

typedef struct _GaussJordanSystem
{
  long double
    **hp_matrix;

  double
    **vectors;

  size_t
    rank,
    number_vectors;
} GaussJordanSystem;

static void EliminateColumn(const GaussJordanSystem *system,
  const ssize_t column)
{
  long double
    **const hp_matrix = system->hp_matrix;

  double
    **const vectors = system->vectors;

  const size_t
    rank = system->rank,
    number_vectors = system->number_vectors;

  long double
    scale;

  ssize_t
    j,
    k;

  for (j=0; j < (ssize_t) rank; j++)
    if (j != column)
      {
        scale=hp_matrix[j][column];
        hp_matrix[j][column]=0.0;
        for (k=0; k < (ssize_t) rank; k++)
          hp_matrix[j][k]-=scale*hp_matrix[column][k];
        for (k=0; k < (ssize_t) number_vectors; k++)
          vectors[k][j]-=(double)(scale*(long double) vectors[k][column]);
      }
}

static void SwapPivotRows(const GaussJordanSystem *system,const ssize_t row,
  const ssize_t column)
{
  long double
    **const hp_matrix = system->hp_matrix;

  double
    **const vectors = system->vectors;

  const size_t
    rank = system->rank,
    number_vectors = system->number_vectors;

  if (row != column)
    {
      SwapMatrixRows(hp_matrix,rank,row,column);
      SwapVectorEntries(vectors,number_vectors,row,column);
    }
}

static void NormalizePivotRow(const GaussJordanSystem *system,
  const ssize_t column)
{
  long double
    **const hp_matrix = system->hp_matrix;

  double
    **const vectors = system->vectors;

  const size_t
    rank = system->rank,
    number_vectors = system->number_vectors;

  long double
    scale;

  ssize_t
    j;

  scale=1.0L/hp_matrix[column][column];
  hp_matrix[column][column]=1.0;
  for (j=0; j < (ssize_t) rank; j++)
    hp_matrix[column][j]*=scale;
  for (j=0; j < (ssize_t) number_vectors; j++)
    vectors[j][column]*=(double) scale;
}

static void UnscrambleColumns(long double **hp_matrix,const size_t rank,
  const ssize_t *columns,const ssize_t *rows)
{
  ssize_t
    i,
    j;

  for (j=(ssize_t) rank-1; j >= 0; j--)
    if (columns[j] != rows[j])
      for (i=0; i < (ssize_t) rank; i++)
        GaussJordanSwapLD(hp_matrix[i][columns[j]],hp_matrix[i][rows[j]]);
}

static void CopyHPMatrixRow(long double **hp_matrix,double **matrix,
  const size_t rank,const ssize_t i)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) rank; j++)
    hp_matrix[i][j]=(long double)matrix[i][j];
}

static long double **RelinquishHPMatrix(long double **hp_matrix,
  const size_t rank)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) rank; i++)
    hp_matrix[i]=(long double *) RelinquishMagickMemory(hp_matrix[i]);
  hp_matrix=(long double **) RelinquishMagickMemory(hp_matrix);
  return(hp_matrix);
}

MagickPrivate MagickBooleanType GaussJordanElimination(double **matrix,
  double **vectors,const size_t rank,const size_t number_vectors)
{

  long double
    **hp_matrix = (long double **) NULL;

  ssize_t
    column,
    *columns = (ssize_t *) NULL,
    i,
    *pivots = (ssize_t *) NULL,
    row,
    *rows = (ssize_t *) NULL;

  /*
    Allocate high precision matrix.
  */
  hp_matrix=(long double **) AcquireQuantumMemory(rank,sizeof(*hp_matrix));
  if (hp_matrix == (long double **) NULL)
    return(MagickFalse);
  for (i=0; i < (ssize_t) rank; i++)
  {
    hp_matrix[i]=(long double *) AcquireQuantumMemory(rank,
      sizeof(*hp_matrix[i]));
    if (hp_matrix[i] == (long double *) NULL)
      ThrowGaussJordanException();
    CopyHPMatrixRow(hp_matrix,matrix,rank,i);
  }
  columns=(ssize_t *) AcquireQuantumMemory(rank,sizeof(*columns));
  rows=(ssize_t *) AcquireQuantumMemory(rank,sizeof(*rows));
  pivots=(ssize_t *) AcquireQuantumMemory(rank,sizeof(*pivots));
  if (IsPivotMemoryMissing(columns,rows,pivots) != MagickFalse)
    ThrowGaussJordanException();
  (void) memset(columns,0,rank*sizeof(*columns));
  (void) memset(rows,0,rank*sizeof(*rows));
  (void) memset(pivots,0,rank*sizeof(*pivots));
  for (i=0; i < (ssize_t) rank; i++)
  {
    const GaussJordanSystem
      system = { hp_matrix, vectors, rank, number_vectors };

    const PivotSearch
      search = { hp_matrix, pivots, rank };

    long double
      max = 0.0;

    /*
      Partial pivoting: find the largest absolute value in the unreduced
      submatrix.
    */
    FindPivot(&search,&max,&row,&column);
    if (IsSingularPivot(column,row,max) != MagickFalse)
      ThrowGaussJordanException();  /* Singular or nearly singular matrix */
    pivots[column]++;
    SwapPivotRows(&system,row,column);
    rows[i]=row;
    columns[i]=column;
    if (fabsl(hp_matrix[column][column]) < LDBL_MIN)
      ThrowGaussJordanException();  /* Singular matrix */
    NormalizePivotRow(&system,column);
    EliminateColumn(&system,column);
  }
  UnscrambleColumns(hp_matrix,rank,columns,rows);
  /*
    Copy back the result to the original matrix.
  */
  CopyBackMatrix(matrix,hp_matrix,rank);
  /*
    Free resources.
  */
  hp_matrix=RelinquishHPMatrix(hp_matrix,rank);
  pivots=(ssize_t *) RelinquishMagickMemory(pivots);
  rows=(ssize_t *) RelinquishMagickMemory(rows);
  columns=(ssize_t *) RelinquishMagickMemory(columns);
  return(MagickTrue);
}
