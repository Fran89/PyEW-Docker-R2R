/* * * * * * * * * * * * * * * * * * * * * * * */
/*                                             */
/*      Copyright (c) 1990, 1991               */
/*      Robert S. Crosson                      */
/*      Geophysics Program, AK-50              */
/*      University of Washington               */
/*      Seattle, WA 98195                      */
/*      All rights reserved.                   */
/*                                             */
/* * * * * * * * * * * * * * * * * * * * * * * */
/*
	Converted to ANSI-C, set up as a stand-alone
	library, added options for user control,
	defined symbolic parameters, by Chris Wood, 4/95
 */
#ifdef _WIN32
# include <windows.h>
# define strcasecmp _stricmp
# define strncasecmp _strnicmp
# define _USE_MATH_DEFINES
# define DllImport	__declspec( dllimport )
# define DllExport	__declspec( dllexport )
#else
# define DllImport	extern
# define DllExport
#endif
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

static int debug = 0;

#ifndef MIN
# define MIN(a,b)	((a) <= (b) ? (a) : (b))
#endif
#ifndef MAX
# define MAX(a,b)	((a) >= (b) ? (a) : (b))
#endif
#ifndef ABS
# define ABS(x)		((x) >= (0) ? (x) : -(x))
#endif
#ifndef ROUND
# define ROUND(x)	((x) < 0. ? (int)((x) - .5) : (int)((x) + .5))
#endif

static int makescan(float *, int, int, float *, int*);
static int get_begend(float *, int, int, int, float, int *, int *);
static int codfit(float *, int, int, int, float, float, float *,
							float *, float *);
static float scal(float *, int, float, float, float, float);
static void mnmx(float *, int, int, float *, float *);

#define SCAN_WIN_LEN	300	/* 400 Scan window length, in seis index counts */
#define SEIS_DECIM 	50	/* 10 decimation of seis to get scan curve */
#define CODA_DMIN	1000	/* minimum delay after P to coda start, in seis
				   index counts */
#define SC_BIAS_WIN	20	/* 100 window length to compute pre-event bias,
				   in scan index counts */
#define SC_MIN		50.	/* scan curve scaled between MIN */
#define SC_RANGE	1000.	/*   and MIN + RANGE */
#define SC_RFACT_MIN	0.005	/* .05 Curve fit to portion of scan curve greater
				   than this, as a fraction of SC_RANGE */
#define SC_RFACT_MAX	0.6	/* 0.8 Curve fit to portion of scan curve less
				   than this, as a fraction of SC_RANGE */
#define	SC_BIAS_DIFF	5.	/* 50. Curve fit to portion of scaled scan curve
				   greater than this above the pre-event bias */
#define SC_REDUCT	0.5	/* Scan curve must decrease by at least this
				   fraction over portion used for curve fit */
#define SC_CODA_LEV	4.	/* 50. Coda defined to be point where value of the
				   fitted scaled scan curve is greater than
				   this	above the pre-event bias */
#define SC_CFMIN_PTS	8	/* 40 minimum nbr of points needed to fit coda */
#define CODA_CORR	0.90	/* minimum correlation factor for coda fit */

/* find end of coda from the seismogram and return counts to that */
/*     position. zero is returned if no coda termination is possible */
/* formal parameters */
/*     seisbuf = array containing seismogram */
/*     lseisbuf = length of seismogram */
/*     pind = pointer in original seismogram counts to beginning of */
/*            event (P onset).  This is in original seismogram */
/*            index counts, not the condensed scan curve counts */
/*     *beg =  */
/*  Returns output coda length in original */
/*            seismogram counts.  This may actually represent */
/*            a point off the end of the physical seismogram */
/*	*info is a generic pointer to return info specific to the coda fitting */
/*		method used.  If the pointer is non-null, this routine */
/*		returns ... */

struct Cinfo {
	float A0;	/* */
	float b;	/* */
};

int coda(float *seisbuf, int lseisbuf, int pind, int get_be,
				int *beg, int *end, void *info)
{
	int lscan, ii, ii_beg, ii_end, clen;
	int sc_beg, sc_end;
	double bias;
	float mn, mx, scan_scale;
	float rho, aa, bb;
	static float *scan = (float *)NULL;
	static int old_lseisbuf = 0;
	int decim = SEIS_DECIM;
	struct Cinfo *coda_fit_info;
	ii_beg = ii_end = 0;

	/* allocate memory for coda scan curve if necessary */
	if ( scan == (float *)NULL || lseisbuf > old_lseisbuf ) {
		size_t siz = lseisbuf / decim * sizeof( *scan );
		if ( scan != (float *)NULL )
			free((void *)scan);
		if ( (scan = (float *)malloc(siz)) == (float *)NULL ) {
			fprintf(stderr,"Can't get space for Scan\n");
			return(0);
		}
		old_lseisbuf = lseisbuf;
	}

	/* make scan curve */
	if ( makescan(seisbuf, lseisbuf, decim, scan, &lscan) != 0 )
		return(0);
		
	/* find min and max of scan curve */
	mnmx(scan, 0, lscan - 1, &mn, &mx);
	if ( debug )
		fprintf(stderr,"coda: scan curve min=%.1f max=%.1f\n",mn,mx);
	
	/* scale scan curve to between SC_MIN and SC_MIN + SC_RANGE */
	scan_scale = scal(scan, lscan, mn, mx, SC_RANGE, SC_MIN);

	/* calculate bias as avg of scan curve above SC_MIN for
	   pre-event (i.e., pre P-pick) window */
	for ( ii = 0 ; ii < lscan ; ++ii ) {
		if ( ii * decim + SCAN_WIN_LEN > pind ) {
			ii_end = MAX(ii - 2, 0);
			ii_beg = MAX(ii - 2 - SC_BIAS_WIN, 0);
			break;
		}
	}
	for ( bias = 0., ii = ii_beg ; ii <= ii_end ; ++ii )
		bias += scan[ii];
	bias /= (ii_end - ii_beg + 1);
	bias -= SC_MIN;
    
	if ( debug ) {
		fprintf(stderr,"coda: pind/decim=%d lseisbuf=%d ",
				(pind-SCAN_WIN_LEN+1)/decim,lseisbuf);
		fprintf(stderr,"lscan=%d ii_beg=%d ii_end=%d bias=%.1f\n",
				lscan,ii_beg,ii_end,(float)bias);
	}
						
	/* get start/end indices of scan curve over which to fit curve */
	if ( get_be ) {
		/* compute from scan curve */
		if ( ! get_begend(scan,lscan,pind,decim,
					bias,&sc_beg,&sc_end) )
			return(0);
		if ( beg != (int *)NULL && end != (int *)NULL ) {
			*beg = MAX(sc_beg * decim - SCAN_WIN_LEN,0);
			*end = MIN(sc_end * decim,lseisbuf-1);
		}
	} else {
		/* user-provided */
		sc_beg = ROUND((float)(*beg + SCAN_WIN_LEN)/decim);
		sc_end = ROUND((float)(*end)/decim);
		if ( sc_beg < 0 || sc_end > lscan - 1 ) {
			if ( debug ) {
				fprintf(stderr,"coda: bad user values: ");
				fprintf(stderr,"sc_beg=%d  sc_end=%d\n",
						sc_beg,sc_end);
			}
			return(0);
		}
	}
	if ( debug )
		fprintf(stderr,"coda: pind=%d beg=%d end=%d\n",pind,*beg,*end);

	/* if fewer than SC_CFMIN_PTS points to fit, then punt */
	if ( sc_end - sc_beg + 1 < SC_CFMIN_PTS ) {
		if ( debug ) {
			fprintf(stderr,"coda: failed: %d - %d + 1 ",
							sc_end,sc_beg);
			fprintf(stderr,"< %d (SC_CFMIN_PTS)\n",
							SC_CFMIN_PTS);
		}	
		return(0);
	}

	/* check that end/start ratio of scan curve is SC_REDUCT or less */
	if ( scan[sc_end] - SC_MIN > SC_REDUCT * (scan[sc_beg] - SC_MIN) ) {
		if ( debug ) {
			fprintf(stderr,"coda: failed: %.1f (scan[%d]) ",
						scan[sc_end],sc_end);
			fprintf(stderr,"> %.2f * %.1f (SC_REDUCT * scan[%d])\n",
						SC_REDUCT,scan[sc_beg],sc_beg);
		}	
		return(0);
	}
	
	/* generate exponential decay fit to selected part of scan curve */
	clen = codfit(scan, lscan, sc_beg, sc_end, SC_MIN + bias,
						scan_scale, &rho, &aa, &bb);
    
	/* reject fitting process if correlation coefficient less than min */
	if ( clen == 0 || ABS(rho) < CODA_CORR )
		return(0);

	/* fill info structure */
	if ( info != (void *)NULL ) {
		coda_fit_info = (struct Cinfo *)info;
		coda_fit_info->A0 = aa;
		coda_fit_info->b = bb;
	}
	
	/* calculate length in seis index counts, and return */	
	return( SCAN_WIN_LEN + clen * decim - pind );
	
} /* coda */

/* This routine generates a scan curve for later use by "coda" routine */
/* enter initial processing loop, get first 'scan_win' points */
/* if not enough data points to fill tank, return */
static int makescan(float *seisbuf, int lseisbuf, int decim,
			float *scan, int *lscan)
{
	int test, i, j, k;
	float xlas1, xlas2, xnew, xmin, xmax;
	int scan_win = SCAN_WIN_LEN;
	double tank;

	*lscan = 0;
	if ( lseisbuf < scan_win ) {
		if ( debug )
			fprintf(stderr,"makescan: lseisbuf (%d) < scan_win (%d)\n",
		lseisbuf,scan_win);
		return -1;
	}
	
    	/* initial fill of 'tank' */
	for ( i = 0, tank = 0.; i < scan_win ; ++i )
		tank += ABS(seisbuf[i]);
	
	/* move tank along trace, saving every decim'th point */
	/*     scan curve represents decimation by "decim" */
	j = 0;
	scan[j] = tank / scan_win;
	k = 0;
	xlas1 = seisbuf[scan_win - 1];
	xlas2 = seisbuf[scan_win - 2];
	for ( i = scan_win ; i < lseisbuf ; ++i ) {
		xnew = seisbuf[i];
		/* if new value for tank is same as last three values to go in,
		   add average value of the tank rather than next new point
		   (this is 'derailing' code) */
		xmax = MAX(xlas2,xlas1);
		xmin = MIN(xlas2,xlas1);
		test = ROUND(MAX(xmax,xnew) - MIN(xmin,xnew));	/* CKW: only works for integer data */
#if 0
		if ( test != 0 )
#endif
			tank += ABS(xnew) - ABS(seisbuf[i - scan_win]);
		if ( ++k == decim ) {
			k = 0;
			scan[++j] = tank / scan_win;
		}
		xlas2 = xlas1;
		xlas1 = xnew;
	}
	*lscan = j;
	return 0;
} /* makescan */

/* find start and end points of scan curve over which to fit coda */
static int get_begend(float *scan, int lscan, int pind,
		int decim, float bias, int *beg, int *end)
{
	int ii, sc_end_max, sc_beg, sc_end;
	float sc_last;

	*beg = *end = 0;
		
	/* find approx end of event by following scan curve to
	   point with value below SC_RFACT_MIN of range */
	for ( ii = 0 ; ii < lscan ; ++ii )
		if ( ii * decim + SCAN_WIN_LEN - 1 > pind + CODA_DMIN )
			if ( scan[ii] < SC_MIN + SC_RFACT_MIN * SC_RANGE )
				break;
	sc_end_max = MIN(ii,lscan-1);
	
	/* find latest point on curve which is at least SC_RFACT_MAX of range
	   above bias.  if earlier than 'pind' then return with no coda */
	for ( ii = sc_end_max ; ii >= 0 ; --ii ) {
		sc_beg = ii;
		if ( ii * decim + SCAN_WIN_LEN - 1 < pind ) {
			if ( debug ) {
				fprintf(stderr,"get_begend: failed: %d before ",
								ii);
				fprintf(stderr,"P: sc_end_max=%d\n",
								sc_end_max);
			}
			return(0);
		}
		if ( scan[ii] >= SC_MIN + bias + SC_RFACT_MAX * SC_RANGE )
			break;
	}

#if 0    
	/* find latest point on curve which is at least SC_BIAS_DIFF
	   above bias */
	for ( ii = sc_end_max - 1 ; ii >= sc_beg ; --ii ) {
		sc_end = ii;
		if ( scan[ii] >= SC_MIN + bias + SC_BIAS_DIFF )
			break;
	}
#endif
	
	/* find first two consecutive points on scan curve that fall
	   within SC_BIAS_DIF of bias */
	sc_last = scan[sc_beg];
	for ( ii = sc_beg + 1 ; ii < sc_end_max ; ii++ ) {
		if ( sc_last < SC_MIN + bias + SC_BIAS_DIFF &&
				scan[ii] < SC_MIN + bias + SC_BIAS_DIFF )
			break;
		sc_last = scan[ii];
	}
	sc_end = ii;
	
	if ( debug ) {
		fprintf(stderr,"get_begend:\tsc_end_max=%d (%.3f of max)\n",
						sc_end_max,SC_RFACT_MIN);
		fprintf(stderr,"\t\tsc_beg=%d (%.3f of max)\n",
						sc_beg,SC_RFACT_MAX);
		fprintf(stderr,"\t\tsc_end=%d (%.3f above bias)\n",
						sc_end,SC_BIAS_DIFF);
	}
	
	*beg = sc_beg;
	*end = sc_end;
	
	return(1);
}

/* fit exponential decay curve to part of scan curve */
/* from ya to yb after removing bias.  Returns coda length in points. */
/* formal parameters */
/*   scan  = scan curve */
/*   ya,yb = beginning and ending points of curve to fit */
/*   bias = amount to remove from y before fitting */
/*   rho = returned correlation coefficient of fit */
static int codfit(float *scan, int lscan, int xa, int xb, float bias,
			float scale, float *rho, float *aa, float *bb)
{
	double temp, temp1, temp2, a, b;
	int xx, len;
	double yy, sumx, sumy, sumx2, sumy2, sumxy;

	sumx = sumy = sumx2 = sumy2 = sumxy = 0.;
	*rho = 0.;
	len = 0;
	
	for ( xx = xa ; xx <= MIN(xb, lscan - 1) ; ++xx ) {
		if ( scan[xx] <= bias )
			yy = 0.;
		else
			yy = log((double)(scan[xx] - bias));
		sumx += xx;
		sumy += yy;
		sumx2 += xx * xx;
		sumy2 += yy * yy;
		sumxy += xx * yy;
	}
    
	temp = xb - xa + 1;
	sumx /= temp;
	sumy /= temp;
	sumx2 /= temp;
	sumy2 /= temp;
	sumxy /= temp;
	temp1 = sumx * sumx - sumx2;
	temp2 = sumy * sumy - sumy2;
	if ( temp1 == 0. || temp2 == 0. ) {
		if ( debug ) {
			fprintf(stderr,"codfit: can't do LS fit: ");
			fprintf(stderr,"sx*sx-sx2 = %.3f, sy*sy-sy2 = %.3f\n",
					temp1,temp2);
		}
		return(0);
	}
	b = (sumx * sumy - sumxy) / temp1;
	a = sumy - b * sumx;
	*rho = (sumxy - sumx * sumy) / sqrt(temp1 * temp2);
	
	/* now calculate point where curve goes to some level above bias */
#if 0
	/* this form has an ad hoc amplitude term */
	/* may want to set SC_CODA_LEV to about 50 or so */
	len = ROUND((log(SC_CODA_LEV) + log(7.5*scale/SCAN_WIN_LEN) - a) / b);
#else
	/* this form is pure coda envelope, and is consistent with method */
	/* may want to set SC_CODA_LEV to about 10 or so */
	len = ROUND((log(SC_CODA_LEV) - a) / b);
#endif
	if ( debug )
		fprintf(stderr,"codfit:\txa,xb=%d %d  a=%.1f b=%.3f rho=%.2f len=%d\n",
							xa,xb,a,b,*rho,len);
							
	*aa = a;
	*bb = b;
	
	return(len);
} /* codfit */

/* scale input array x to between minval and minval+rng, and return
   scale factor */
static float scal(float *x, int lx, float mn, float mx, float rng, float minval)
{
	int i;
	float sc;

	if ( mn == mx )
		return(0.);
	sc = rng / (mx - mn);
	/* remove minimum and scale result to rng max */
	for ( i = 0 ; i < lx ; ++i )
		x[i] = minval + (x[i] - mn) * sc;
	return(sc);
} /* scal */

/* ret global min & max values in ixa - ixb */
static void mnmx(float *x, int ixa, int ixb, float *mn, float *mx)
{
	int i;
	float min, max;

	min = x[ixa];
	max = x[ixa];
	for ( i = ixa + 1 ; i <= ixb ; ++i ) {
		min = MIN(min, x[i]);
		max = MAX(max, x[i]);
	}
	*mn = min;
	*mx = max;
	return;
} /* mnmx */
