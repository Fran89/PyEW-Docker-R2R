
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ingelada.c 3306 2008-04-02 17:41:35Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2008/04/02 17:41:35  dietz
 *     added comment calling out the mis-spelling of Inglada.
 *
 *     Revision 1.1  2000/02/14 16:08:53  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:07:49  lucky
 *     Initial revision
 *
 *
 */

/****************************************************************
 *   ingelada()							*
 *								* 
 *   C function to compute hypocenter using the method of       *
 *   Inglada (1928), mis-spelled Ingelada in this code.         *
 *   Originally in a fortran function (EPIGES11) by Rex Allen,  *
 *   intermediately in an Splus function by Bill Ellsworth, 	*
 *   converted to C by Lynn Dietz 9/96     		  	*
 *								* 
 *   Returns an epicenter, in cartesian coordinates, and an     *
 *   origin time based on the P arrivals at 4 stations          *
 *								* 
 *   Solution method is non-iterative, and is based on the      *
 *   assumption of a uniform velocity half-space with all       *
 *   stations at the same elevation (z=0)			*
 *                                                              *
 *   Returns  0 if there were no problems                       *
 *           -1 if the incorrect # of phases was passed         *
 ****************************************************************/
#include <math.h>

#define ABS(X) (((X) >= 0) ? (X) : -(X))

void   logit( char *, char *, ... );  /* logit.c  sys-independent  */

static double zMin = 1.0;   /* Use this depth when the solution's  */
			    /* depth is imaginary                  */

int ingelada( int    num,   /* # elements passed in each of x,y,t  */
              double *x,    /* cartesian station coordinates (lon) */
              double *y,    /* cartesian station coordinates (lat) */
              double *t,    /* observed arrival times              */
              float   v,    /* uniform velocity of halfspace (km/s)*/
              double *e  )  /* output vector:  e[0] x coordinate   */
                            /*                 e[1] y coordinate   */
                            /*                 e[2] origin time    */
                            /*                 e[3] focal depth    */
{
   double a[3][3];      /* matrix */
   double b[3];         /* vector */
   double r[4];         /* squared distances from coordinate origin */
   double p[4];         /* travel-time diff relative to 1st arrival */
   double dx, dy, zsq;  /* working variables */
   double k;            /* constant */
   float  vsq;          /* velocity squared */
   int   i;

/* Initialize output vector
 **************************/
   for( i=0; i<4; i++ ) e[i] = 0.0;

/* Make sure that the correct number 
   of arrivals have been passed. 
 ***********************************/
   if( num != 4 ) return( -1 );

/* Form intermediate vectors 
 ***************************/
   for( i=0; i<4; i++ ) 
   {
      r[i] = x[i]*x[i] + y[i]*y[i];   /* squared distances from origin */
      p[i] = t[0] - t[i];             /* traveltime differences, WLE   */
      if( ABS(p[i]) < 0.0001 ) {
          p[i] = 0.0001;          /* Bullet-proofing of Allen */
      }
   }

/* Form equations
 ****************/
   vsq = v*v;
   for( i=1; i<4; i++ )
   {
      a[i-1][0] = x[i] - x[0];
      a[i-1][1] = y[i] - y[0];
      a[i-1][2] = p[i] * vsq;
      b[i-1]    = (r[i] - r[0] - vsq*p[i]*p[i])/2.;
   }

/* Now solve the equations.
   Taken from Allen's EPIGES11 code 
  (Ellsworth used Splus function lsfit)
 **************************************/
   if( ABS(a[0][0]) < 0.0001 ) {
       a[0][0] = 0.0001;         /* Bullet-proofing of Allen */
   }

   k = a[1][0]/a[0][0];
   a[1][1] -= k*a[0][1];
   a[1][2] -= k*a[0][2];
   b[1]    -= k*b[0];

   k = a[2][0]/a[0][0];
   a[2][1] -= k*a[0][1];
   a[2][2] -= k*a[0][2];
   b[2]    -= k*b[0];
 
   if( ABS(a[1][1]) < 0.0001 ) {
       a[1][1] = 0.0001;         /* Bullet-proofing of Allen */
   }

   k = a[2][1]/a[1][1];
   a[2][2] -= k*a[1][2];
   b[2]    -= k*b[1];

   if( ABS(a[2][2]) < 0.0001 ) {
       a[2][2] = 0.0001;         /* Bullet-proofing of Allen */
   }

   e[2] = b[2]/a[2][2];
   e[1] = (b[1]-a[1][2]*e[2])/a[1][1];
   e[0] = (b[0]-a[0][1]*e[1]-a[0][2]*e[2])/a[0][0];

   if( ABS(e[2]) < 0.0001 ) {
       e[2] = 0.0001;            /* Bullet-proofing of Allen */
   }

/* compute the depth
 *******************/
   dx  = e[0] - x[0];
   dy  = e[1] - y[0];
   zsq = e[2]*e[2]*vsq - dx*dx - dy*dy;


/* avoid imaginary depth--
      Allen used:      if( zsq < 0 )  zsq = 1.;               
      Ellsworth used:  if( zsq < 0 )  zsq = -zsq;             
   LDD asked Ellsworth 9/27/96, he said use Allen's method,
   otherwise you may get very big traveltime residuals
 **********************************************************/
    if( zsq < 0 )  zsq = zMin;
    e[3] = (float) sqrt((double)zsq);

    /*logit(""," ingelada : x:%.1lf  y:%.1lf  z:%.1lf  dt0:%.2lf\n", 
	   e[0],e[1],e[3],e[2] );*//*DEBUG*/   

/* convert e[2] into origin time
 *******************************/
    e[2] += t[0];  /*WLE*/


    return( 0 );
}

