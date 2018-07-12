
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sample.h 2 2000-02-14 16:16:56Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 16:08:53  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:07:49  lucky
 *     Initial revision
 *
 *
 */


/*
 * sample.h     Lynn Dietz 9/96
 *
 */

/* Define a structure to hold variable-size, 2-dimensional arrays.
 *   If nr=1 and nc=1, then m is a scalar.
 *   If nr=1  or nc=1, then m is a vector.
 *   If nr>1 and nc>1, then m is a matrix.
 ****************************************************************/
typedef struct {
	int   nr;    /* # rows with data in this matrix   */
 	int   nc;    /* # columns with data in m          */
        int   nmax;  /* length of m; nr*nc must be < nmax */
	int  *m;     /* 1D array representing a matrix    */ 
                     /* of nr rows and nc columns         */
} MTX;

/* Define a macro to convert 2-D indices into the 
 * corresonding 1-D index for array m in the MTX structure
 * The matrix is stored in the 1-D array row-by-row.
 *********************************************************/
#define IXMTX( IR, IC, NC ) ( (IR)*(NC) + (IC) )

/* Function prototypes
 *********************/
MTX *cbind( MTX *, MTX *, MTX * );
MTX *rbind( MTX *, MTX *, MTX * );
MTX *n_draw_p( int, int, MTX * );
MTX *sample  ( int, int, int, MTX * );
int  ncombo( int, int );

