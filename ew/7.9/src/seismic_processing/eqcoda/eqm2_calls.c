
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: eqm2_calls.c 818 2001-12-12 19:16:08Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2001/12/12 19:16:08  dietz
 *     Fixed bug in cdxtrp that resulted in a bad extrapolated coda length
 *     being returned.
 *
 *     Revision 1.2  2000/07/21 23:09:03  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/02/14 17:07:37  lucky
 *     Initial revision
 *
 *
 */

/*  eqm2_calls.c
 *
 *  All routines in this source file were translated from fortran to C
 *  by Lynn Dietz 10/95
 *  Original code in eqm2_call1.f and eqm2_call2.f came from
 *  Barry Hirshorn.
 */
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "eqcoda.h"

extern int ForceExtrapolation;   /* for Debugging purposes only */

/*********************************************************************
  Function codasub()
     Decide which average absolute value amplitudes to fit
     an l1 norm line to in log(amp)-log(time) space.

     Calls:
       1) cntrtime2  - returns the center times of the 2 second
                    windows over which the avg. abs. value amps.
                    are taken.

       2) l1fit   - calculates both "fixed" and "free" l1 fits
                    of log(amp) on log(time).
                    (codasub3.3 passes it the log vales.)
**********************************************************************/

int  codasub( float  tau,	/* raw coda length from picker 			 */
	      float  sminp,  	/* S-P time (seconds)   			 */
	      long  *iiamps,	/* 6 coda-win amplitudes determined by picker	 */
	      long   klip,	/* magic number denoting clipped data 		 */
	      float *cntrtime,	/* center times of 6 coda windows 		 */
	      short *na,	/* number of coda amps used in l1fit 		 */
	      float *afree,	/* coda length from free-slope fit to coda amps  */
	      float *qfree,	/* slope of free log-log fit to coda amps 	 */
	      float *afix,	/* coda length from fixed-slope fit to coda amps */
	      float *qfix, 	/* fixed slope of log-log fit to coda amps  	 */
	      float *del )	/* rms of l1-fit to coda windows 		 */
{
      float x[6]; 	/* log10 of center-times of coda windows   	 */
      float y[6];       /* log10 of avg amplitudes of coda windows       */
      short kgood;	/* number of coda windows with coda amps < klip	 */
      short kbad;	/* number of coda windows with coda amps > klip  */
      short n;	        /* number of coda windows with non-zero amps     */
      long  iamps[6];   /* internal storage for coda amplitudes 	 */
      short i, in, ns;

/* Initialize any variables not reassigned a value every time
 ************************************************************/
      ns    =  0;
      n     =  0;
      in    =  0;
     *na    =  0;
      kgood =  0;
      kbad  =  0;
     *afree =  0.0;
     *qfree =  0.0;
     *afix  =  0.0;
     *qfix  = (float) -1.8;
     *del   =  0.0;

     for ( i=0; i<6; i++ )  {
        if ( iiamps[i] == 0 )
          iamps[i] = 1;
        else
          iamps[i] = iiamps[i];

        cntrtime[i] = 0.;
        x[i]        = 0.0;
        y[i]        = 0.0;
     }

/* Get center times of reported coda windows:
 ********************************************/
    if( cntrtime2( tau, &n, cntrtime ) != 0 ) {
	return( -1 );
    }

/* Find na, the number of consecutive, unclipped amplitudes sampled	
   at t-tp > than that of the last clipped amplitude:	
 ******************************************************************/
    for (i=0; i<n; i++ ) {
       if ( iamps[i] <= klip ) {
	  kgood = kgood + 1;
	  if (kbad == 0) ns = i+1;
       }
       else {
	  kbad = kbad + 1;
       }
    }
   *na = ns;

/* Reduce na by 1 if there's an amplitude at 1 sec. after P-arrival	
   na then becomes the number of amplitudes sampled at center 	
   times >= 1.1*sminp + 1 . (more conservative than keeping amps	
   at center times > sminp+1.8.)					
   (empirically, the maximum amplitude often occurs just after ts.  	
   ,ie, s-sn increasing w/dist beyond ~120km., where s is max ampl.)
   also, get best fits to the amps. at c.times after 1.1*sminp+1):
 *****************************************************************/

    if ( *na > 0 ) {
        if ( cntrtime[*na-1] == 1. ) *na = *na-1;
        ns = *na;
	for ( i=0; i<*na; i++ ) {
	    if ( cntrtime[i] >= ( (1.1*sminp) + 1. ) ) continue;
	    ns = i;
	    break;
	}
       *na = ns;
    }


/* Generate log(time) & log(amp) arrays
 **************************************/

    if ( iamps[0] <= 1 &&  *na > 1 ) {
       in = 0;
       for ( i=1; i<*na; i++ ) {
          x[in] = (float) log10( (double) cntrtime[i] );
          y[in] = (float) log10( (double) iamps[i]    );
	  in++;
       }
       *na = *na - 1;
    }
    else if ( *na >= 1) {
       for( i=0; i<*na; i++ ) {
          x[i] = (float) log10 ( (double) cntrtime[i] );
          y[i] = (float) log10 ( (double) iamps[i]    );
       }
    }


/* Do l1 norm fit to the log(time) & log(amp) arrays.
  (fixed and free fits)
 ****************************************************/				
/*      fprintf(stderr,"log10(times): %.2f %.2f %.2f %.2f %.2f %.2f\n",
		     x[0],x[1],x[2],x[3],x[4],x[5] );*/ /*DEBUG*/
/*      fprintf(stderr,"log10(caavs): %.2f %.2f %.2f %.2f %.2f %.2f\n",
		     y[0],y[1],y[2],y[3],y[4],y[5] );*/ /*DEBUG*/
/*      fprintf(stderr,"IN  afix:%.2f qfix:%.1f  afree:%.2f qfree:%.1f\n",
	              *afix, *qfix, *afree, *qfree );*/ /*DEBUG*/

      if (*na >= 1) {
           l1fit( x, y, *na, qfree, afree, qfix, afix, del );
      }

/*      fprintf(stderr,"OUT afix:%.2f qfix:%.1f  afree:%.2f qfree:%.1f\n",
	              *afix, *qfix, *afree, *qfree );*/ /*DEBUG*/

      return( 0 );
}

/************************************************************************
 function cntrtime2()
   tau is the reported coda length (from picker)
   n is the number of amplitude samples reported
      if  abs(tau) > 9    n = 6
      else                n = abs(tau)/2 + 1
   cntrtime is the array of times returned
	itimes(n) = abs(tau)
   itimes are the center points of the 2-second windows over which
      the average absolute value amplitudes are taken.
   This version has been modified to reflect the changes
      made in the picker at 1730 gmt on the 25th of jan 1984.
   The major change is that the last amplitude sample reported from the
      coda is for the coda sample.
					AGL 2/4/88
************************************************************************/
int  cntrtime2( float  tau,	  /* coda length from picker		   */
	        short *n, 	  /* number of coda amplitudes expected    */
	        float *cntrtime ) /* center times of reported coda windows */
{
   static short istim[23] = {  1,   3,   5,  7,  9, 11, 13, 15, 19,  23,
			      31,  39,  47, 55, 63, 71, 79, 87, 95, 111,
			     127, 139, 144 };
   short itau;
   int   i, j, it0;
   short nwin;

   for (j=0; j<6; j++ )  cntrtime[j] = 0.;

   itau = (short) ABS( tau );

   if ( itau == 0 ) {
      *n    = 0;
       return( -1 );
   }

/* Find the number of coda windows reported given the coda length
 ****************************************************************/
   nwin = itau / 2 + 1;
   if ( nwin > 6 ) nwin = 6;
   *n = nwin;

/* Find the index to the time of the earliest the coda window
 ************************************************************/
   if (itau > 144) {
      return( -2 );
   }
   for( i=0; i<23; i++ ) {
      if ( itau <= istim[i] ) break;
   }
   it0 = i - nwin + 1;
   if (it0 < 0) it0 = 0;

/* Save all the center-times of the coda windows:
   Put into array in reverse chronological order
 ***********************************************/
   for( j=0; j<nwin; j++ ) {
	cntrtime[nwin-1-j] = (float) istim[it0+j];
   }
   if ( itau < istim[i] ) {          /* correct time of last coda window if it */
        cntrtime[0] = (float) itau;  /* wasn't one of the "standard" windows   */
   }
   return( 0 );
}


/**********************************************************************
 function l1fit()
 Fits an l1 line to the n points in x and y
    obtains a y0 (afix) for a fixed slope (qfix)
    qfix must be passed as an input parameter
 Outputs are:
    qfree  -  the slope of a free solution to the data
    afree  -  the corresponding intercept (y0)
    afix   -  the intercept obtained when the slope is held fixed
              at qfix
    rms    -  the average absolute residual from the fitted line
              (an l1 equivalent of rms in least squares)
              when n=1  rms = 0
                    =2  rms is for afix and qfix
                    >2             afree and qfree
 **********************************************************************/

int  l1fit ( float *x, 		/* log10 of center times    	  */
	     float *y, 		/* log10 of coda amplitudes 	  */
	     short  n, 		/* number of values in x & y 	  */
	     float *qfree, 	/* slope of free-fit to x&y	  */
	     float *afree, 	/* intercept of free-fit to x & y */
	     float *qfix, 	/* fixed slope to use in fit      */
	     float *afix, 	/* intercept from fixed-slope fit */
	     float *rms ) 	/* rms of fit to x & y 		  */
{
   float best;
   float a0, q0;
   float sum, res;
   float xnum, xden;
   int i, j;
   int iseg;

   *qfree = -99.;
   *afree = 0.;
   *afix  = 0.;
    best  = 100000.;
   *rms   = 0.0;

   if ( *qfix >= 0.01  ||  n < 1) {
      return( -1) ;
   }

/* Do a fixed-slope fit to coda amplitudes:
 ******************************************/
   q0 = *qfix;

   for ( i=0; i<n; i++ ) {
      a0 = ( y[i] - x[i]*q0 );
      sum = 0.0;
      for ( j=0; j<n; j++ ) {
        res  = a0 + x[j]*q0 - y[i];
 	sum += ABS(res);
      }
      if ( sum <= best ) {
 	*afix = a0;
 	 best = sum;
      }
   }

/* Do a free-slope fit to coda amplitudes:
 ****************************************/
   if (n == 2) *rms = best;

   if (n >= 2 ) {
      best = 100000.;
      for ( i=1; i<n; i++ ) {
 	  for ( j=0; j<i; j++ ) {
	      xnum = y[i]*x[j] - y[j]*x[i];
              xden = x[j] - x[i];
	      if (xden < 0.001)  xden = (float) 0.001;
              a0 = xnum / xden;
	      q0 = (y[i] - a0) / x[i];
	      sum = 0.0;
	      for ( iseg=0; iseg<n; iseg++ ) {
		  res  = a0 + x[iseg]*q0 - y[iseg];
		  sum += ABS(res);
	      } /* end for over iseg */
	      if(sum <= best) {
		  *afree = a0;
                  *qfree = q0;
                   best  = sum;
              }
          } /* end for over j */
      } /* end for over i */

      if (n > 2)  *rms = best/(n-2);
   }
	
   return( 0 );
}
	
/*********************************************************************
 Function codmag()
     Compute md by lee's original, 1972, formula.
     3/14/88 - modified to run on -tau's. -bfh-
**********************************************************************/
int codmag( float  tau,		/* coda length from picker */
	    float  dist,	/* epicentral distance     */
	    float *codamag )	/* calculated magnitude    */
{
   if ( tau == 0. ) {
      *codamag = 0.;
      return( -2 );
   }
   else if ( tau < 0. || dist < 0. ) {
      *codamag = (float) (-0.87 + 2.0 * log10((double) ABS(tau)) + 0.0035 * ABS(dist));
      return( -1 );
   }
   else {
      *codamag = (float) (-0.87 + 2.0 * log10((double) tau) + 0.0035 * dist);
      return( 0 );
   }
}
	
/*********************************************************************
 Function cphs2()
   cphs assigns a 3-letter phase descriptor to RTP data.

   CPHASE(4:4) is passed around as the INTEGER, icwt.

   2/21/88 - bfh.
**********************************************************************/
void cphs2( float  tau,		/* coda length from picker */
	    float  sminp,	/* S minus P time	   */
	    char  *cphase )	/* phase descriptor	   */
{
   strcpy( cphase, "   " );

   if ( tau == 0.0 ) return;

/* Assign first letter of descriptor based on coda length from picker
 ********************************************************************/
   if ( tau < 0.0 ) {	
        cphase[0] = 'N';	/* noisy */
   }
   else if ( tau >= 144.0 ) {
        cphase[0] = 'S';	/* short */
   }
   else {
        cphase[0] = 'P';	/* picker's */
   }

/* Assign second letter of descriptor based on S-P time
 ******************************************************/
   if ( ABS(tau) >= sminp ) {
        cphase[1] = 'S';	/* S-coda */
   }
   else {
        cphase[1] = 'P';	/* P-coda */
   }

/* Assign 3rd letter based on first letter
   This letter may change later on if a coda extrapolation is done
 *****************************************************************/

   if ( cphase[0] == 'P' ) cphase[2] = 'N';	/* normal termination */

   return;
}

/*********************************************************************
 Function asnwgt()
   This function computes icwt, a measure of the quality of
        this coda arrival as measured by rms and slope vs. fmag
   VERY sensitive to the rms of the fit to the L1 line.
   VERY INsensitive to the Md/slope test. (Ie- the (Md,slope)
        "point" has to be VERY far off of the Md/slope regression
        line to have much of an effect on the icwt.
        (Hence the term "JUNK"- GOOD!)

  3/15/88 - Replaced "KGOOD" with "na" after a scrutiny of the
            VAX/750's "eqmeasure.for" revealed that "KGOOD"
            was being used as the number of amplitude samples
            used in the log(A)-log(t-tp)- "na" on isunix!!!
            (NOTE that "KGOOD" is used in the unix code as the
            number of unclipped amplitude samples......)

  4/4/88  - Is now passed the EVENT Md, as originally averaged over
            all of the coda lengths of the event, instead of the
            single station Md. --> better filtering of Magnitudes
            based on a better "Md/slope" determination for com-
            parison to Al's regression line.
**********************************************************************/

void asnwgt( short na, 		/* number of coda amps used in l1 fit */
	     float fmag, 	/* trial magnitude for this station   */
	     float qfree, 	/* slope determined from free l1fit   */
	     float rms, 	/* rms from l1fit		      */
	     short *icwt, 	/* coda-weight determined here        */
	     short *ires )	/* error return			      */
{
   float   junk;
   short   tmpwt;

   tmpwt = 9;
   *ires = 1;

/* If there was enough data to do a free l1fit,
   adjust weight by fit to mag and rms
 *********************************************/
   if ( na >= 2 ) {
        junk = (float) (0.7*fmag + 0.5 + qfree);
        junk = ABS( junk );
	if (junk > 0.6  ) tmpwt--;
	if (junk > 1.2  ) tmpwt--;
	if (junk > 1.8  ) tmpwt--;
	if (junk > 2.4  ) tmpwt--;
	if (junk > 3.0  ) tmpwt--;
	if (rms  > 0.08 ) tmpwt--;
	if (rms  > 0.16 ) tmpwt--;
	if (rms  > 0.24 ) tmpwt--;
	if (rms  > 0.32 ) tmpwt--;
	if (rms  > 0.40 ) tmpwt--;
   }

/* If only 1 coda amp was usable (ie, no
   free-fit possible), assign a default weight
 *********************************************/
   else if (na == 1) {
        tmpwt = 3;
   }

/* If no coda amps were usable, give it NO weight
 ************************************************/
   else if (na == 0) {
	tmpwt = 0;
   }

/* If you got here, you fed this routine some
   bad information!
 ********************************************/
   else {
       tmpwt = 0;
       *ires = 0;
   }

/* Make sure the weight is within valid range
 ********************************************/
   if ( tmpwt < 0  ||  tmpwt > 9 ) {
	tmpwt = 0;
   }

/* Convert from a 9(best) to 0(worst) weight
	     to a 0(best) to 4(worst) weight
 *******************************************/
   *icwt = tmpwt;
    cnvwt2( icwt );
    return;
}

/*********************************************************************
 Function cnvwt2()

 Converts asnwgt coda weights, which go from 9 (best) to 0 (worst)
  to hypoinverse coda weights, which go from 0 (best) to 4 (worst)
 This is to be the new "one and only" standard till death do us part,
  or so the saying goes
 aglindh  11 feb 88
**********************************************************************/
void cnvwt2( short *icwt )
{

   if ( *icwt >= 7 ) {
	*icwt = 0;
   } else if ( *icwt >= 5 ) {
	*icwt = 1;
   } else if ( *icwt >= 3 ) {
	*icwt = 2;
   } else if ( *icwt >= 1 ) {
 	*icwt = 3;
   } else  {
        *icwt = 4;
   }

   return;
}

/*********************************************************************
 Function codxtrp()
 Extrapolates the coda decay to a given level (by NCSN standards,
 the number of counts equivalent to 60 mV)
**********************************************************************/
void cdxtrp( float  x60mv,	/* number of counts to extrapolate coda to   	*/
	     int    itau,	/* raw coda length from picker			*/
	     char  *cphs13,	/* phase descriptor from cphs2()		*/
	     short *icwt,	/* coda weight 			     		*/
	     short  neq,	/* number of coda amps used in l1fit 		*/
	     float  afix,	/* intercept from fixed-slope l1fit to coda amps*/
	     float  qfix,      	/* slope used in fixed-slope l1fit to coda amps */
	     float  afree,	/* intercept from free-slope l1fit to coda amps */
	     float  qfree,	/* slope found in free l1fit to coda amps 	*/
             float  rms,	/* rms from one of the l1fits to coda amps	*/
	     int   *iotau, 	/* extrapolated coda length 			*/
	     short *ires ) 	/* return value 				*/
{
   float  yc;   /* log10 of coda-length cutoff level  */
   double exp;  /* temporary variable for power       */
   double tau;  /* temporary extrapolated coda length */

   *ires = 1;

   yc = (float) log10( (double) x60mv );

/* P-wave coda; set coda length to zero and return
 *************************************************/
   if ( cphs13[1] != 'S' ) {
	*iotau     = 0;
        *icwt      = 4;
         cphs13[2] = ' ';
         return;
   }

/* Extrapolate a noisy or 'short' coda:
 *************************************/
   if (cphs13[0] == 'N'  ||  cphs13[0] == 'S'  || ForceExtrapolation) { 

   /* If the free l1fit was good, use that to extrapolate the coda */
  	 if( neq   >= 3    &&
	     rms   <  0.3  &&
     	     afree >  0.0  &&
             qfree < -1.0  &&
             qfree > -5.0     ) {
             /*       iotau = 10**((yc-afree)/qfree) + 0.5 */ /*original fortran*/
		      exp   = (double) ((yc-afree)/qfree );
		      tau   =  pow( 10.0, exp ) + 0.5;
		     *iotau = (int) tau;
                      cphs13[2]  ='R';
         }
   /* Or if there was at least one on-scale coda amp,
      use the fixed l1fit information to extrapolate the coda */
	 else if ( neq  >= 1   &&
     		   afix >  0.0 &&
                   qfix < -1.0    ) {
               /*     iotau = 10**((yc-afix)/qfix) + 0.5 */ /*original fortran*/
		      exp   = (double) ((yc-afix)/qfix );
		      tau   =  pow( 10.0, exp ) + 0.5;
		     *iotau = (int) tau;
                      cphs13[2] = 'X';
	 }
    /* Otherwise, we can't extrapolate the coda properly;
       return the original coda length, but give it a 4-weight */
	 else {
		     *iotau     = ABS( itau );
                     *icwt      = 4;
                      cphs13[2] = ' ';
	 }
   }

/* Otherwise, don't try to extrapolate; set coda length to zero
 **************************************************************/
   else {
       *iotau     = 0;
       *icwt      = 4;
        cphs13[2] = ' ';
       *ires      = 0;
   }

   return;
}
