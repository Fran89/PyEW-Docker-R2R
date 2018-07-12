

/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sample.c 2 2000-02-14 16:16:56Z lucky $
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
 *  sample.c    contains the functions required to arrive at
 *              a matrix of samples of "n" numbers (from 0:n-1)
 *              taken "p" at a time with no replacement.
 *
 *              n_draw_p() returns all possible combinations,
 *              sample()   returns a requested number of random 
 *                         combinations
 *    
 *  Code was initially in Splus, written by Bill Ellsworth,
 *  converted to C by Lynn Dietz 9/96
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sample.h"

static int SeedRand = 1;  /* flag for seeding the random-number generator */ 


/************************************************************
 *  n_draw_p()                                              *
 * 							    *
 *  function to compute complete set of combinations of the *
 *  numbers 0:n-1 taken p at a time without replacement.    *
 *  There are n!/(p!*(n-p)!) numbers (Binomial Coefficient) *
 *  							    *
 *  If we let {n|p} represent the set of combinations, then *
 *  they satisfy the recursion relation                     *
 * 							    *
 *  	{n|p} = {n-1|p} + c({n-1|p-1},n))		    *
 *   							    *
 * 							    *
 *   	Input:						    *
 *   		n	total number of numbers		    *
 *   		p	size of draw			    *
 * 							    *
 *   	Output:						    *
 *   		matrix of nrow=p, ncol=n!/(p!*(n-p)!) 	    *
 *   	        containing one unique combination in 	    *
 *   		each column				    *
 * 							    *
 ************************************************************/
MTX *n_draw_p( int n, int p, MTX *result )
{
   MTX  r1, r2, r3;   /* results of intermediate steps */
   MTX  scalar;      
   int  i;
      
/* Invalid conditions; return null matrix
 ****************************************/
   if( (n==1 && p==0)  ||              /* end of recursion */
        n < p          ||              /* illegal argument values */
        p*ncombo(n,p) > result->nmax ) /*result will overflow target address*/
   {
       result->nr = 0;
       result->nc = 0;
       return( result );
   }

/* For n things taken one at a time, return a row
 ************************************************/
   if( p==1 )   
   {
       result->nr = 1;
       result->nc = n;
       for( i=0; i<n; i++ )  result->m[i] = i;   /*C-version*/
       /*for( i=0; i<n; i++ )  result->m[i] = i+1;*/ /*Splus compatible*/
       return( result );
   }

/* For n things take all at once, return a column
 ************************************************/
   if( n==p ) 
   {
       result->nr = n;
       result->nc = 1;
       for( i=0; i<n; i++ )  result->m[i] = i;   /*C-version*/
       /*for( i=0; i<n; i++ )  result->m[i] = i+1;*/ /*Splus compatible*/
       return( result );
   }

/* All other cases require a recursive process;
   set up all variables needed for recursion
 **********************************************/
   scalar.nr   = 1;
   scalar.nc   = 1;
   scalar.nmax = 1;
   scalar.m    = (int *) malloc( scalar.nmax*sizeof(int) );
   if( scalar.m==(int *)NULL )  return( (MTX *)NULL );
   scalar.m[0] = n-1; /*C version*/
   /*scalar.m[0] = n;*/   /*Splus version*/

   r1.nr = r2.nr = r3.nr = 0;   
   r1.nc = r2.nc = r3.nc = 0;
   r1.nmax =    p * ncombo(n-1,p);
   r2.nmax = (p-1)* ncombo(n-1,p-1);
   r3.nmax = r2.nmax + r2.nmax/(p-1); 
   r1.m = (int *) malloc( r1.nmax*sizeof(int) );
   r2.m = (int *) malloc( r2.nmax*sizeof(int) );
   r3.m = (int *) malloc( r3.nmax*sizeof(int) ); 

   if( r1.m==(int *)NULL  || 
       r2.m==(int *)NULL  || 
       r3.m==(int *)NULL     )  
   {
       result = (MTX *) NULL;
   }

/* Here's the recursive statement:
 *********************************/
   else {
       cbind( n_draw_p( n-1, p, &r1),
              rbind( n_draw_p( n-1, p-1, &r2 ), &scalar, &r3),
              result );
   }

/* Free storage of intermediate steps
 ************************************/
   free( (void *)scalar.m );
   free( (void *)r1.m );
   free( (void *)r2.m );
   free( (void *)r3.m ); 

   return( result );
}

/********************************************************
 * ncombo() returns the number of possible combinations *
 * of n things taken p at a time, with no replacement.  *
 * (Same as n!/(p!*(n-p)!) without doing factorials)    *
 ********************************************************/
int ncombo( int n, int p )
{
   double ncmb=1;
   double maxint;
   int i, a, b;

   if( n<=0 || p<=0 ) return( 0 );
   if( n<p )          return( 0 );

   if( p > n-p ) {
      a = p;
      b = n-p;
   }
   else {
      a = n-p;
      b = p;
   }

   for( i=n; i>a; i-- ) ncmb *= (double)i;
   for( i=b; i>0; i-- ) ncmb /= (double)i;

   maxint = pow( (double)2, (double)8*sizeof(int)/2 ) - 1;

   if( ncmb > maxint ) return( (int) maxint );
   else                return( (int) ncmb   );
}

/****************************************************************
 * sample() draws set of nsamp random samples from a population *
 * of numbers from 0:n-1.  Each sample contains p elements,     *
 * with no replacement.                                         *
 ****************************************************************/
MTX *sample( int n, int p, int nsamp, MTX *result )
{
   int *pop;
   int  npop;
   int  i, j, ipop, is, ires;
   
   if( nsamp*p > result->nmax ) return(result);  

   if( SeedRand )
   {
      srand( (unsigned int) n );
      SeedRand = 0;
   }

   pop  = (int *) malloc( n*sizeof(int) );

/* Loop over the number of requested samples
 *******************************************/
   for( is=0; is<nsamp; is++ ) 
   {
   /* re-initialize population array */
      npop = n;
      for( i=0; i<npop; i++ )  pop[i]=i;  

   /* loop over all elements in sample */
      for( i=0; i<p; i++ )
      {             
      /* get a random index into the population */
         ipop = rand()%npop;
         ires = IXMTX( i, is, nsamp );
         result->m[ires] = pop[ipop];

      /* remove the chosen element from the population */
         npop--;
         for( j=ipop; j<npop; j++ )  pop[j] = pop[j+1];
      }
   }

   free( (void *)pop );
   result->nr = p;
   result->nc = nsamp;
   return( result );
}


/*******************************************************
 *       cbind() combine columns of 2 matrices         *
 *******************************************************/
MTX *cbind( MTX *a, MTX *b, MTX *result )
{
   int i, j;
   int newix, oldix;
   int nr, nc;

   nr = 0;
   nc = 0;

/* If either argument is empty or NULL, return the other
 *******************************************************/
   if( a->nr==0  ||  a->nc==0  ||  a==(MTX *)NULL )  
   {
      if( b->nr*b->nc > result->nmax ) return( result );
      result->nr = b->nr;
      result->nc = b->nc;
      memcpy( (void *)result->m, (void *)b->m, b->nr*b->nc*sizeof(int) );  
      return( result );
   }
   else if( b->nr==0  ||  b->nc==0  ||  b==(MTX *)NULL ) 
   {
      if( a->nr*a->nc > result->nmax ) return( result );
      result->nr = a->nr;
      result->nc = a->nc;
      memcpy( (void *)result->m, (void *)a->m, a->nr*a->nc*sizeof(int) );   
      return( result );
   }

/* If both matrices have same #rows, join them
 *********************************************/
   else if( a->nr == b->nr )  
   {
      nr = a->nr;
      nc = a->nc + b->nc;
      if( nr*nc > result->nmax ) return( result );
      for( i=0; i<nr; i++ ) {
          for( j=0; j<a->nc; j++ ) {
             newix = IXMTX( i, j, nc );
             oldix = IXMTX( i, j, a->nc  );
             result->m[newix] = a->m[oldix];     
          }
          for( j=0; j<b->nc; j++ ) {
             newix = IXMTX( i, j+a->nc, nc );
             oldix = IXMTX( i, j, b->nc  );
             result->m[newix] = b->m[oldix];     
          }
      }
   }
  
/* If one is argument is a scalar, repeat it as a whole column
 *************************************************************/
   else if( a->nr==1 && a->nc==1 )
   {
      nr = b->nr;
      nc = a->nc + b->nc;
      if( nr*nc > result->nmax ) return( result );
      for( i=0; i<nr; i++ ) {
          for( j=0; j<a->nc; j++ ) {
             newix = IXMTX( i, j, nc );
             oldix = IXMTX( 0, 0, a->nc  );
             result->m[newix] = a->m[oldix];     
          }
          for( j=0; j<b->nc; j++ ) {
             newix = IXMTX( i, j+a->nc, nc );
             oldix = IXMTX( i, j, b->nc  );
             result->m[newix] = b->m[oldix];     
          }
      }
   }
   else if( b->nr==1 && b->nc==1 )
   {
      nr = a->nr;
      nc = a->nc + b->nc;
      if( nr*nc > result->nmax ) return( result );
      for( i=0; i<nr; i++ ) {
          for( j=0; j<a->nc; j++ ) {
             newix = IXMTX( i, j, nc );
             oldix = IXMTX( i, j, a->nc  );
             result->m[newix] = a->m[oldix];     
          }
          for( j=0; j<b->nc; j++ ) {
             newix = IXMTX( i, j+a->nc, nc );
             oldix = IXMTX( 0, 0, b->nc  );
             result->m[newix] = b->m[oldix];     
          }
      }
   }

/* Otherwise, the #rows doesn't match; don't do anything
 *******************************************************/
   else 
   {
       return( result );
   }

/* Complete new matrix structure
 *******************************/
   result->nr = nr;
   result->nc = nc;
   return( result );
}


/*******************************************************
 *        rbind() combine rows of 2 matrices           *
 *******************************************************/
MTX *rbind( MTX *a, MTX *b, MTX *result )
{
   int i,j;
   int newix, oldix;
   int nr, nc;

   nr = 0;
   nc = 0;

/* If either argument is empty or NULL, return the other
 *******************************************************/
   if( a->nr==0  ||  a->nc==0  ||  a==(MTX *)NULL )  
   {
      if( b->nr*b->nc > result->nmax ) return( result );
      result->nr = b->nr;
      result->nc = b->nc;
      memcpy( (void *)result->m, (void *)b->m, b->nr*b->nc*sizeof(int) );   
      return( result );
   }
   else if( b->nr==0  ||  b->nc==0  ||  b==(MTX *)NULL ) 
   {
      if( a->nr*a->nc > result->nmax ) return( result );
      result->nr = a->nr;
      result->nc = a->nc;
      memcpy( (void *)result->m, (void *)a->m, a->nr*a->nc*sizeof(int) );   
      return( result );
   }

/* If both matrices have same #columns, join them
 ************************************************/
   else if( a->nc == b->nc )  
   {
      nr = a->nr + b->nr;
      nc = a->nc;
      if( nr*nc > result->nmax ) return( result );
      for( i=0; i<a->nr; i++ ) {
          for( j=0; j<a->nc; j++ ) {
             newix = IXMTX( i, j, nc );
             oldix = IXMTX( i, j, a->nc  );
             result->m[newix] = a->m[oldix];     
          }
      }
      for( i=0; i<b->nr; i++ ) {
          for( j=0; j<b->nc; j++ ) {
             newix = IXMTX( i+a->nr, j, nc );
             oldix = IXMTX( i,       j, b->nc  );
             result->m[newix] = b->m[oldix];     
          }
      }
   }
  
/* If one is argument is a scalar, repeat it as a whole row
 **********************************************************/
   else if( a->nr==1 && a->nc==1 )
   {
      nr = a->nr + b->nr;
      nc = b->nc;
      if( nr*nc > result->nmax ) return( result );
      for( i=0; i<a->nr; i++ ) {
          for( j=0; j<nc; j++ ) {
             newix = IXMTX( i, j, nc );
             oldix = IXMTX( 0, 0, a->nc  );
             result->m[newix] = a->m[oldix];     
          }
      }
      for( i=0; i<b->nr; i++ ) {
          for( j=0; j<b->nc; j++ ) {
             newix = IXMTX( i+a->nr, j, nc );
             oldix = IXMTX( i,       j, b->nc  );
             result->m[newix] = b->m[oldix];     
          }
      }
   }
   else if( b->nr==1 && b->nc==1 )
   {
      nr = a->nr + b->nr;
      nc = a->nc;
      if( nr*nc > result->nmax ) return( result );
      for( i=0; i<a->nr; i++ ) {
          for( j=0; j<a->nc; j++ ) {
             newix = IXMTX( i, j, nc );
             oldix = IXMTX( i, j, a->nc  );
             result->m[newix] = a->m[oldix];     
          }
      }
      for( i=0; i<b->nr; i++ ) {
          for( j=0; j<nc; j++ ) {
             newix = IXMTX( i+a->nr, j, nc );
             oldix = IXMTX( 0,       0, b->nc  );
             result->m[newix] = b->m[oldix];     
          }
      }
   }

/* Otherwise, the # columns doesn't match; don't do anything
 ***********************************************************/
   else 
   {
       return( result );
   }

/* Complete new matrix structure
 *******************************/
   result->nr = nr;
   result->nc = nc;
   return( result );
}
