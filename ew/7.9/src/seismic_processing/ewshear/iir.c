/******************************************************************************
 *
 *	File:			iir.c
 *
 *	Function:		Butterworth highpass & lowpass filters.
 *
 *	Author(s):		Scott Hunter, ISTI
 *
 *	Source:			PQL
 *
 *	Notes:			
 *
 *	Change History:
 *			4/26/11	Started source
 *	
 *****************************************************************************/

/* From the original file in PQL:
**********************************/
/*   Program to test filters for PASSCAL Instrument                    */
/*	These filters are Butterworth highpass and lowpass filters     */
/*	both filters may be implemented or only one filter             */
/*	This program limits the number of poles to be either 0, 2 or 4 */
/*	The program to calculate the pole position or to filter does   */
/*	not have any limits                                            */
/*                                                                     */
/*   Input parameters                                                  */
/*     NH = order of the high pass filter can be 0, or an even number
		up to 12                                               */
/*     FH = high pass cutoff frequency                                 */
/*     NL = order of the low pass filter can be 0, or an even number 
		up to 12                                               */
/*     FL = low pass cutoff frequency                                  */
/*     dt = sample rate                                                */
/*                                                                     */
/*  The program calculates the filter poles then generates a linear    */
/*	sweep with beginning frequency = 0 to stop frequency = nyqiist */
/*	This sweep is then filtered by the requested filters.          */
/*	The output is then written to disk in ascii format             */
/*	The file names for the output are sweep.asc and filter.asc     */
/*                                                                     */
/*       written by jcf   feb 1988
	modified  july 1993.                                                             */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <earthworm_simple_funcs.h>
#include "iir.h"

#ifndef PI
#ifdef M_PI
#define PI  M_PI
#else
#define PI 3.1415
#endif
#endif


typedef struct
    {
	double real ;
	double imag ;
    } iircomplex ;

/**********************************************************************/
/*          add_c                                                     */
/**********************************************************************/
/*
    Routine to add two iircomplex numbers

	w = add_c(u,v)
        w = u + v
*/

static iircomplex	    add_c (u,v)
					iircomplex	    u ;
					iircomplex	    v;
{
    iircomplex	w ;

    w.real = u.real + v.real ;
    w.imag = u.imag + v.imag ;

    return (w) ;
}

/***********************************************************************/
/*             mul_c                                                   */
/***********************************************************************/
/*
	Routine to multiply two iircomplex numbers

	        w = mul_c(u,v)
		w = u * v

*/
static iircomplex	mul_c (u,v)
iircomplex		u ;
iircomplex		v ;
{
	iircomplex		w ;

	w.real = u.real*v.real - u.imag*v.imag ;
	w.imag = u.real*v.imag + u.imag*v.real ;

	return (w) ;
}

/***********************************************************************/
/*            cmul_c (a,u)                                             */
/***********************************************************************/
/*
    Routine to multiply a real number times a iircomplex number

	w = cmul_c (a,u)

	a - real number
	u - iircomplex number
*/
static iircomplex	    cmul_c (a,u)
double	    a ;
iircomplex	    u ;
{
    iircomplex	w ;

    w.real = a * u.real ;
    w.imag = a * u.imag ;

    return (w) ;
}

/*************************************************************************/
/*               sub_c                                                   */
/*************************************************************************/
/*
    Routine to subtract two iircomplex numbers

	w = sub_c(u,v)
	w = u - v
*/
static iircomplex	sub_c(u,v)
iircomplex	    u ;
iircomplex	    v ;
{
    iircomplex	w ;
    w.real = u.real - v.real ;
    w.imag = u.imag - v.imag ;

    return (w) ;
}

/******************************************************************/
/*                    div_c                                       */
/******************************************************************/
/*
    Routine to divide two iircomplex numbers

	w = div_c(u,v)
	w = u/v

*/
static iircomplex	div_c (u,v)
iircomplex	    u ;
iircomplex	    v ;
{
    iircomplex	w ;

    /*   check for divide by 0    */
    if (v.real != 0 || v.imag != 0)
    {
	w.real = ((u.real * v.real) + (u.imag * v.imag)) /
		((v.real * v.real) + (v.imag * v.imag)) ;
	w.imag = ((u.imag * v.real) - (u.real * v.imag)) /
		((v.real * v.real) + (v.imag * v.imag)) ;

	return (w) ;
    }
    else
    {
	fprintf (stderr, "ERROR: iircomplex division by 0 in div_c\n") ;
	exit (1) ;
    }
}

/***************************************************************/
/*                 conj_c                                      */
/***************************************************************/
/*
	Routine to calculate the iircomplex conjugate

		w = conjugate(u)

*/
static iircomplex	conj_c (u)
iircomplex		u ;
{
	iircomplex		w ;

	w.real = u.real ;
	w.imag = -u.imag ;

	return (w) ;
}


/************************************************************************/
/*        filt (a1, a2, b1, b2, npts, fi, fo)                           */
/************************************************************************/
/*	Routine to apply a second order recursive filter to the data
	denomonator polynomial is z**2 + a1*z + a2
	numerator polynomial is z**2 + b1*z + b2
	    fi = input array
	    fo = output array
	    npts = number of points

*/
static void filt (a1, a2, b1, b2, npts, fi, fo)
double	a1, a2, b1, b2 ;
double	fi[], fo[] ;
int	npts ;
{
    double  d1, d2, out ;
    int i ;

    d1 = 0 ;
    d2 = 0 ;
    for ( i=0 ; i<npts ; i++)
    {
	out = fi[i] + d1 ;
	d1 = b1*fi[i]  - a1*out + d2 ;
	d2 = b2*fi[i] - a2*out ;
	fo[i] = out ;
    }

}


/**************************************************************************/
/*                   lowpass (fc,dt,n,p,b)                               */
/**************************************************************************/
/*
    Routine to compute lowpass filter poles for Butterworth filter 
	fc = desired cutoff frequency
	dt = sample rate in seconds
	n = number of poles (MUST BE EVEN)
	p = pole locations (RETURNED)
	b = gain factor for filter (RETURNED)

*/
/*   Program calculates a continuous Butterworth low pass IIRs with required */
/*    cut off frequency.                                                     */
/*   This program is limited to using an even number of poles                */
/*   Then a discrete filter is calculated utilizing the bilinear transform   */
/*   Methods used here follow those in Digital Filters and Signal Processing */
/*   by Leland B. Jackson  */

static void lowpass	(fc,dt,n,p,b)
double	    fc, dt, *b ;
iircomplex	    p[] ;
int	    n ;
{  
    double	wcp, wc, b0 ;
    int		i, i1 ;
    iircomplex	add_c(), mul_c(), div_c(), cmul_c(), sub_c() ;
    iircomplex	conj_c() ;
    iircomplex	one, x, y ;

/*			    Initialize variables       */
/*    PI = 3.1415927 ; */
    wcp = 2 * fc * PI ;
    wc = (2./dt)*tan(wcp*dt/2.) ;
    one.real = 1. ;
    one.imag = 0. ;
    for (i=0 ; i<n ; i += 2)
    {
/*               Calculate position of poles for continuous filter    */

	i1 = i + 1 ;
        p[i].real = -wc*cos(i1*PI/(2*n)) ;
	p[i].imag = wc*sin(i1*PI/(2*n)) ;
	p[i+1] = conj_c(p[i]) ;
    }
    for ( i=0 ; i<n ; i += 2)
    {
/*             Calculate position of poles for discrete filter using    */
/*              the bilinear transformation                             */

	p[i] = cmul_c(dt/2,p[i]) ;
	x = add_c(one,p[i]) ;
	y = sub_c(one,p[i]) ;
	p[i] = div_c(x,y) ;
	p[i+1] = conj_c(p[i]) ;
    }

/*	calculate filter gain   */

    b0 = 1. ;
    for (i=0 ; i<n ; i +=2)
    {
	x = sub_c(one,p[i]) ;
	y = sub_c(one,p[i+1]) ;
	x = mul_c(x,y) ;
	b0 = b0*4./x.real ;
    }
    b0 = 1./b0 ;
    *b = b0 ;
}

/**************************************************************************/
/*                   highpass (fc,dt,n,p,b)                               */
/**************************************************************************/
/*
    Routine to compute lowpass filter poles for Butterworth filter 
	fc = desired cutoff frequency
	dt = sample rate in seconds
	n = number of poles (MUST BE EVEN)
	p = pole locations (RETURNED)
	b = gain factor for filter (RETURNED)

*/
/*   Program calculates a continuous Butterworth highpass IIRs               */
/*   First a low pass filter is calculated with required cut off frequency.  */
/*   Then this filter is converted to a high pass filter                     */
/*   This program is limited to using an even number of poles                */
/*   Then a discrete filter is calculated utilizing the bilinear transform   */
/*   Methods used here follow those in Digital Filters and Signal Processing */
/*   by Leland B. Jackson  */

static void highpass (fc,dt,n,p,b)
double	    fc, dt, *b ;
iircomplex	    p[] ;
int	    n ;
{  
    double	wcp, wc,  alpha, b0 ;
    int		i ;
    iircomplex	add_c(), mul_c(), div_c(), cmul_c(), sub_c() ;
    iircomplex	conj_c() ;
    iircomplex	one, x, y ;

/*         Initialize variables          */
/*     PI = 3.1415927 ; */
    wcp = 2 * fc * PI ;
    wc = (2./dt)*tan(wcp*dt/2.) ;
    alpha = cos(wc*dt) ;
    one.real = 1. ;
    one.imag = 0. ;

/*            get poles for low pass filter     */

    lowpass(fc,dt,n,p,&b0) ;

/*       now find poles for highpass filter      */

    for (i=0 ; i<n ; i+=2)
    {
	x = cmul_c (alpha,one) ;
	x = sub_c (x,p[i]) ;
	y = cmul_c (alpha,p[i]) ;
	y = sub_c(one,y) ;
	p[i] = div_c(x,y) ;
	p[i+1] = conj_c(p[i]) ;
    }

/*      Calculate gain for high pass filter    */

    b0 = 1. ;
    for (i=0 ; i<n ; i += 2)
    {
	x = add_c(one,p[i]) ;
	y = add_c(one,p[i+1]) ;
	x = mul_c(x,y) ;
	b0 = b0*4./x.real ;
    }					    
    b0 = 1./b0 ;
    *b = b0 ;
}


/*****************************************************************************
	iir():  
*****************************************************************************/
int iir(EW_TIME_SERIES *ewts, double FH, int NH, double FL, int NL )
{ 

     double *af;
     int    number;
     double sam_rate;
  
  /*      Local variable definition       */
  int		i;
  iircomplex	pl[12], ph[12] ;
  double	dt, b0l, b0h ;
  double	f1, f0 ;
  double	a1, a2, b1, b2 ;

	af = ewts->data;
	number = ewts->dataCount;
	sam_rate = 1.0/ewts->trh2x.samprate;
	dt = sam_rate;

	if ( FL > ((1/sam_rate)/2) ) {
		logit( "", "HighCut Frequency Must Be <= %lf; aborting HighCut filter", (double) 1./sam_rate/2.);
		NL = 0;
	}

  /*         Get highpass filter poles if necessary    */
  
  if(NH != 0)
    {
      highpass(FH,dt,NH,ph,&b0h) ;
    }
  
  /*      Get low pass filter poles if necessary     */
  
  if(NL != 0)
    {
      lowpass(FL,dt,NL,pl,&b0l) ;
    }
  
  
  /*   Through with calculation of poles        */
  
  /*       now calculate sweep for filtering     */
  
  f0 = 0. ;     /*  start frequency    */
  f1 = 1./(dt*2.) ;	/*  stop frequency = nyquist  */
    
  if(NH != 0)
    {
      for ( i=0 ; i<NH ; i +=2)
	{
	  
	  /*	get first set of second order filter coeficients  */
	  /*      from each pair of poles                           */
	  
	  a1 = -2*ph[i].real ;
	  a2 = ph[i].real*ph[i].real + ph[i].imag*ph[i].imag ;
	  b1 = -2 ;
	  b2 = 1 ;
	  
	  filt (a1, a2, b1, b2, number, af, af) ;
	  
	}
      /*        apply gain section          */
      for ( i=0 ; i<number ; i++)
		{
		  af[i] = b0h*af[i] ;
		}
    }
  
  /*      apply low pass filter using poles pl         */
  /*	Numerator polynomial is z**2 + 2*z + 1       */

  if ( NL != 0)
    {
      for ( i=0 ; i<NL ; i +=2)
	{
	  
	  a1 = -2*pl[i].real ;
	  a2 = pl[i].real*pl[i].real + pl[i].imag*pl[i].imag ;
	  b1 = 2 ;
	  b2 = 1 ;
	  
	  filt (a1, a2, b1, b2, number, af, af) ;

	}
      
      /*        apply gain section          */
      for ( i=0 ; i<number ; i++)
		{
		  af[i] = b0l*af[i] ;
		}
    }
    
  return 0;
}

