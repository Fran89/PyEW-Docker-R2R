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
/*	2/25/94 - CKW	Converted to ANSI-C, wholesale removal of
 *			f2c detritus, misc cleanup.
 */

/* Routine to do detailed picking on integer seismogram */
/* This routine is stand-alone portable */

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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define extern
#include "piki.h"
#undef extern
#include "piki_init.h"

/* prototypes */
static int gtcor(float);
static int gterr(float);
static void winmax(float *, int, int, int *, float *, int *, float *);
static int gtpol(void *, char, int, int, int);
static void filt(float *, int, float *, int *, int);
static void hpfil(float *, int , float *);
void do_abs(float *, int);
void do_env(float *, int);

extern void hilbertr(float *, int);

/* Pick phase times for seismogram using front/back ratio technique.
	It is desirable to have the approximate detection point
	in the middle of the data buffer passed to piki to avoid
	minor end effects.	
	Piki returns a negative number error flag if no pick is found.
	Return values are:
    		=  the pick index if a normal pick is made (> 0)
    			note that 0 (first element) is never an allowable pick
    		= -8 if no maximum is located in vicinity of detection point
    		= -9 if ratio curve maximum does not meet threshold
*/
int piki(
	int *epik,	/* error in pick in index counts */
	int *polar,	/* polarity indicator; 0 = none, +1 = up, -1 = down */
	int *pikwt,	/* pick weight (0 = on, 9 = off) */
	void *x,	/* input seismogram */
	char xfmt,	/* input seismogram type ('F', 'L' or 'S') */
	int lx,		/* length of input seismogram, in points */
	int tar,	/* position of target pick */
	int hpfilt,	/* flag to high-pass filter before picking */
	int cfun)	/* flag to select characteristic function (piki.h) */
{
	static int bwin = BWIN, fwin = FWIN, rng = RNG;
	static float offthr = OFFTHR, pikthr = PIKTHR,
					pik9thr = PIK9THR, minprod = MINPROD;
	float *rc, *seis, *seis1;	/* ratio curve, copy of seismogram */
	int *lp, *lp1;
	short *sp, *sp1;
	int begpt, endpt;
	float valmn, valmx;
	int ptrmn, ptrmx, ret_val;
	size_t siz;

	/* allocate float arrays for seismogram and ratio curve */
    	siz = lx * sizeof(float);
	if ( (seis = (float *)malloc(siz)) == NULL ) {
		fprintf(stderr,"Can't get space for float trace\n");
		return(FALSE);
	}
	if ( (rc = (float *)malloc(siz)) == NULL ) {
		fprintf(stderr,"Can't get space for ratio curve\n");
		free((void *)seis);
		return(FALSE);
	}
	
	/* copy input seis pick window into float array */
	switch ( xfmt ) {
		case 'S':	/* S -> F */
			sp = (short *)x;
			for ( sp1 = sp, seis1 = seis ; sp1 < sp + lx ;
							sp1++, seis1++ )
				*seis1 = *sp1;
			break;
		case 'L':	/* L -> F */
			lp = (int *)x;
			for ( lp1 = lp, seis1 = seis ; lp1 < lp + lx ;
							lp1++, seis1++ )
				*seis1 = *lp1;
			break;
		case 'F':	/* F -> F */
			memcpy((void *)seis, x, lx * sizeof(*seis));
			break;
		default:	/* huh? */
			fprintf(stderr,"Unknown data type: '%c'\n",xfmt);
			free((void *)seis);
			free((void *)rc);
			return(FALSE);
	}
			
	/* initialization of returned picking data */
	ret_val = 0;
	*epik = 0;
	*polar = 0;
	*pikwt = -1;

	/* High pass filter the seismogram.  The filter is a 4 pole Bessel
	   with high cut at 0.01 of sample frequency (1 Hz for 100 Hz sample
	   rate). Implemented by a recursive algorithm that allows overwriting
	   of input.  Results of filter operation overwrite seis */
	/* CKW - add option to not filter, because data may already be
	   filtered.  Also, .01 corner is too high for low-frequency data.
	   Probably need a more general filter option that does zero-phase
	   band-pass filtering */
	if ( hpfilt && cfun != SELF )
		hpfil(seis, lx, seis);	/* seis -> hpfilt(seis) */

	/* replace seismogram with characteristic function */
	switch ( cfun ) {
		case SELF:	/* already had characteristic function */
			break;
		case ENVELOPE:	/* seis -> sqrt(seis^2 + H(seis)^2) */
			do_env(seis,lx);
			break;
		case ABSVAL:	/* seis -> abs(seis) */
		default:
			do_abs(seis,lx);
			break;
	}
	
	/* get the f/b ratio curve of the characteristic function */
	fbrat(seis, rc, lx, fwin, bwin, FALSE);

	/* find pick by looking for maximum value in the vicinity of
	   the detection point */
	begpt = MAX(0, tar-rng);
	endpt = MIN(lx-1, tar+rng);
	winmax(rc, begpt, endpt, &ptrmx, &valmx, &ptrmn, &valmn);
	if ( ptrmx != 0 && valmx >= pik9thr && valmn <= offthr && valmx * valmn >= minprod ) {
		ret_val = ptrmx - ( PICKCORR ? gtcor(valmx) : 0 );
		*epik = gterr(valmx);
		*polar = gtpol(x, xfmt, lx, ptrmx, hpfilt);
		*pikwt = ( valmx >= pikthr ? 0 : 9 );
	} else if ( ptrmx == 0 ) {
		ret_val = -8;
		*epik = 0;
	} else if (valmx < pik9thr) {
		ret_val = -9;
		*epik = 0;
	} else if ( valmn > offthr ) {
		ret_val = -7;
		*epik = 0;
	} else if ( valmx * valmn < minprod ) {
		ret_val = -10;
		*epik = 0;
	}
	
	free((void *)seis);
	free((void *)rc);
	
	return (ret_val);
} /* piki */

/* return pick corr from max value of ratio curve */
static int gtcor(float vmax) /* max value of ratio curve */
{
	int corr = ROUND(15 * pow(vmax, GTCOR_EXP));
	return( MAX(0,corr) );
} /* gtcor */

/* ret pick error from max value of ratio curve */
static int gterr(float vmax) /* max value of ratio curve */
{
	int err = ROUND(30 * pow(vmax, GTERR_EXP)) + 1;
	return( MAX(3,err) );
} /* gterr */

/* find global extremum in array 'a' which does not lie at an */
/*     endpoint.  a local maximum will be missed if it is less than */
/*     one or both endpoints.  In that case, a zero is returned */
/*     for the pointer value of the maximum value. */
static void winmax(
	float *a,	/* input array (float) */
	int ia, int ib,	/* start and end indices for search */
	int *mxptr,	/* index of maximum value in a */
	float *valmx,	/* value of maximum found */
	int *mnptr,	/* index of minimum value in a */
	float *valmn)	/* value of minimum found */
{
	int i;

	if (ib <= ia) {
		*mxptr = 0;
		*mnptr = 0;
		*valmn = 0.;
		*valmx = 0.;
		return;
	}
	
	*mxptr = ia;
	*mnptr = ia;
	*valmx = a[ia];
	*valmn = a[ia];
	for ( i = ia ; i <= ib ; ++i ) {
		if (a[i] > *valmx) {
			*mxptr = i;
			*valmx = a[i];
		}
		if (a[i] < *valmn) {
			*mnptr = i;
			*valmn = a[i];
		}
	}
	if (*mxptr == ia || *mxptr == ib)
		*mxptr = 0;
	if (*mnptr == ia || *mnptr == ib)
		*mnptr = 0;
	return;
} /* winmax */

/* find and return polarity at specified pick position
   gtpol = returned polarity;
         = 0 if no polarity determined
         = 1 if up polarity
         = -1 if down polarity */
#define POL_THRESH	4.	/* polarity threshold */
#define POL_WIN		23	/* window for threshold test - must be odd */
#define POL_MAX_PTS	30	/* f.m. must change sign by this point */
static int gtpol(
	void *x,	/* buffer of trace values surrounding pick position */
	char xfmt,	/* input seismogram type ('F', 'L' or 'S') */
	int lx,		/* length of x */
	int pikpos,	/* index of pick within buffer */
	int hpfilt)	/* flag to indicate whether to high-pass filter data */
{

	static int fil[] = {1, 2, 1};	/* unnormalized filter */
	int i, sdelta, osdelta, pol;
	double ratio, sum1, sum2;
	int *lp, *lp1;			/* pointers for copying seismogram */
	short *sp, *sp1;
	float *seis, *seis1, delta, zz[POL_WIN];
	size_t siz;
	
	/* If not enough room, return with zero return value */
	if ( lx < POL_WIN || lx - pikpos < POL_WIN / 2 )
		return (0);
	
	/* allocate float array for seismogram */
	siz = lx * sizeof(float);
	if ( (seis = (float *)malloc(siz)) == NULL ) {
		fprintf(stderr,"Can't get space for float trace\n");
		return(0);
	}
	
	/* copy seismogram to float array */
	switch ( xfmt ) {
		case 'S':	/* S -> F */
			sp = (short *)x;
			for ( sp1 = sp, seis1 = seis ; sp1 < sp + POL_WIN ;
							sp1++, seis1++ )
				*seis1 = *sp1;
			break;
		case 'L':	/* L -> F */
			lp = (int *)x;
			for ( lp1 = lp, seis1 = seis ; lp1 < lp + POL_WIN ;
							lp1++, seis1++ )
				*seis1 = *lp1;
			break;
		case 'F':	/* F -> F */
			memcpy((void *)seis, x, lx * sizeof(*seis));
			break;
		default:	/* huh? */
			fprintf(stderr,"Unknown data type: '%c'\n",xfmt);
			free(seis);
			return(0);
	}
	
	/* pull out window of POL_WIN points around pick */
	for ( i = 0 ; i < POL_WIN ; ++i )
		zz[i] = seis[pikpos - POL_WIN / 2 + i];

	/* filter the seismogram with low pass filter */
	if ( hpfilt ) {
		float zzz[POL_WIN];
		int nf = sizeof(fil) / sizeof(*fil);	/* length of fil */
		filt(zz, POL_WIN, zzz, fil, nf);
		/* put result back into zz */
		memcpy((void *)zz, (const void *)zzz, POL_WIN * sizeof(*zz));
	}
	
	/* compute sum of slopes before and after pick */
	sum1 = 0.;
	sum2 = 0.;
	for ( i = 0 ; i < POL_WIN / 2 - 2 ; ++i ) {
		sum1 += ABS(zz[-i + POL_WIN/2    ] - zz[-i + POL_WIN/2 - 1]);
		sum2 += ABS(zz[ i + POL_WIN/2 + 1] - zz[ i + POL_WIN/2    ]);
	}
	
	/* calculate ratio of these slopes to determine if polarity
	   will be determined at all (ratio must exceed thres) */
	ratio = (sum2 + .1) / (sum1 + .1);
	if ( ratio < POL_THRESH ) {
		free(seis);
		return (0);
	}
	
	/* find first extrema in raw trace after pick, then compare that
	   with value at pick to decide polarity */
	/* first, move past any flat spots */
	for ( i = pikpos ; i < lx - 1 ; i++ ) {
		if ( i - pikpos > POL_MAX_PTS )
			return (0);
		delta = seis[i + 1] - seis[i];
		if ( delta != 0. )
			break;
	}
	osdelta = ( delta < 0. ? -1 : 1 );
	
	/* now, look for change in sign of slope */
	for ( ++i ; i < lx - 1 ; i++ ) {
		if ( i - pikpos > POL_MAX_PTS )
			return (0);
		delta = seis[i + 1] - seis[i];
		sdelta = ( delta < 0. ? -1 : 1 );
		if ( osdelta != sdelta )
			break;
		osdelta = sdelta;
	}
		
	if ( seis[i] > seis[pikpos] )
		pol = 1;
	else if ( seis[i] < seis[pikpos] )
		pol = -1;
	else
		pol = 0;
    	
	free(seis);

	return(pol);
} /* gtpol */

/* Filter input int data array 'din' of length 'ld' using moving
   average filter array 'fil' of length 'lf' and put results in int
   array 'dout'. find wt by calculating area under filter */

static void filt(float *din, int ld, float *dout, int *fil, int lf)
{
	int i, j;
	float wt;
	int lf2;
	double sum;

	for ( i = 0, sum = 0. ; i < lf ; ++i )
		sum += fil[i];
	if ( sum == 0. )
		return;
	wt = 1. / sum;
	lf2 = lf / 2;
	
	/* perform actual filtering loop */
	for ( i = 0 ; i < ld - lf + 1 ; ++i ) {
		sum = 0.;
		for ( j = 0 ; j < lf ; ++j )
			sum += fil[j] * din[i + j];
		dout[lf2+i] = sum * wt;
	}
	
	/* now fill out endpoints */
	for ( i = 0 ; i < lf2 ; ++i ) {
		dout[i] = dout[lf2];
		dout[ld - 1 - i] = dout[ld - lf + lf2];
	}
	return;
} /* ifilt */

/* High pass filter */
/* note: this routine excised from Dave Harris general
   filter routine "recfil" in SAC; it could be made more efficient
   by not allowing overwrite so that buffers need not be shifted */
static void hpfil(float *data, int ndata, float *fdata)
{
	/* Fixed filter coefficients; 4th order Bessel HP filter */
	/* Corner freq = 0.01 of sample frequency */
	
	static double a[] = { 5390958.29333015,-21563833.1733206,
			32345749.7599809,-21563833.1733206,5390958.29333015 };
	static double b[] = { 5759441.35423723,-22279798.1925077,
			32325362.7719655,-20847860.1541335,5042870.22043846 };
	static int order = sizeof(a) / sizeof(*a);

	static double *dbuf = NULL, *fbuf = NULL;
	int i, j, point;
	double out;
	size_t siz;

	/*  FILTER DATA - FORWARD DIRECTION */

	/*  INITIALIZE BUFFER ARRAYS */
	if ( dbuf == NULL || fbuf == NULL ) {
		siz = order * sizeof(double);
		if ( (dbuf = (double *)malloc(siz)) == NULL ) {
			fprintf(stderr,"can't get space for pick filter\n");
			return;
		}
		if ( (fbuf = (double *)malloc(siz)) == NULL ) {
			fprintf(stderr,"can't get space for pick filter\n");
			return;
		}
	}
	for ( i = 0 ; i < order ; ++i ) {
		dbuf[i] = 0.;
		fbuf[i] = 0.;
	}

	for ( point = 0 ; point < ndata ; point++ ) {
		/*  FETCH NEW INPUT DATUM */
		dbuf[0] = data[point];

		/*  CALCULATE NEW OUTPUT POINT */
		out = a[0] * dbuf[0];
		for ( i = 1 ; i < order ; ++i )
			out += a[i] * dbuf[i] - b[i] * fbuf[i];
		fbuf[0] = out / b[0];
		fdata[point] = fbuf[0];
	
		/*  SHIFT BUFFERS */
		for ( i = 1 ; i < order ; ++i ) {
			j = order - i;
			fbuf[j] = fbuf[j - 1];
			dbuf[j] = dbuf[j - 1];
		}
	}

	return;
} /* hpfil */

/* Generate a front-back ratio curve from a characteristic function
   of the seismogram.  Characteristic function usually must be a non-negative
   function for the ratio curve to be useful.  Examples of characteristic
   functions of the seismogram are absolute value, or the envelope.
   
   Routine uses fast tanking algorithm invented by Eric Crosson
   
*/
void fbrat(
	float *seis,	/* Input characteristic of seismogram */
	float *fbcurv,	/* Space for output ratio curve */
	int npts,	/* Number of points in characteristic seismogram */
	int fwlen,	/* Front (later) window (triangle) length */
	int bwlen,	/* Back window (triangle) length */
	int logflg)	/* If TRUE, deliver the log10 of ratio curve */
{
	int i, begpt, endpt, winpt;	/* indices */
	int beg, end;
	double bbox, fbox, btri, ftri;	/* sums of float - should be double */
	double tmp1, tmp2;		/* temporary seis values */
	double tscale;

	beg = 0;
	end = npts - 1;
	begpt = MAX(beg, bwlen - 1);
	endpt = MIN(end, npts - fwlen);
	tscale = bwlen * (bwlen + 1.) / (fwlen * (fwlen + 1.));

	/* Set up the windows to start at beginning and compute first
	   value.  Characteristic seis curve is normally non-negative,
	   but is not required to be so. */
	for ( ftri = 0., fbox = 0., i = 1 ; i <= fwlen ; ++i ) {
		ftri += i * seis[begpt+fwlen-i];
		fbox += seis[begpt+fwlen-i];
	}
	for ( btri = 0., bbox = 0., i = 1 ; i <= bwlen; ++i) {
		btri += i * seis[begpt-bwlen+i];
		bbox += seis[begpt-bwlen+i];
	}
	if ( btri != 0. && ftri != 0. )
		fbcurv[begpt] = tscale * ftri / btri;
	else
		fbcurv[begpt] = 1.;

	/* Set ratio curve to initial computed value before beginning. */
	for ( winpt = 0 ; winpt < begpt ; ++winpt )
		fbcurv[winpt] = fbcurv[begpt];

	/* Compute the rest of the ratio curve using fast tanking algorithm */
	for ( winpt = begpt + 1 ; winpt <= endpt ; ++winpt ) {
		tmp1 = seis[winpt-1];
		tmp2 = seis[winpt+fwlen-1];
		ftri -= fwlen * tmp1;
		fbox += (tmp2 - tmp1);
		ftri += fbox;
		btri -= bbox;
		tmp1 = seis[winpt];
		tmp2 = seis[winpt-bwlen];
		bbox += (tmp1 - tmp2);
		btri += bwlen * tmp1;
		if ( btri != 0. && ftri != 0. ) {
			fbcurv[winpt] = tscale * ftri / btri;
		} else {
			fbcurv[winpt] = 1.;
		}
	}

	/* Set ratio curve to final computed value after end */
	for ( winpt = endpt + 1 ; winpt < npts ; ++winpt )
		fbcurv[winpt] = fbcurv[endpt];

	/* Compute log if requested */
	if ( logflg )
		for ( winpt = 0 ; winpt < npts ; ++winpt ) {
			if ( fbcurv[winpt] < 0. ) {
				fprintf(stderr,"fbcurv(%d)=%f < 0\n",winpt,
						fbcurv[winpt]);
				exit(1);
			}
			fbcurv[winpt] = log10(fbcurv[winpt]);
		}

	return;
} /* fbrat */

/* replace seismogram with absolute value */	
void do_abs(float *s, int len)
{
	int ii;
	
	for ( ii = 0 ; ii < len ; ++ii )
		if ( s[ii] < 0. )
			s[ii] = -s[ii];
	
}

/* replace seismogram with envelope function */
void do_env(float *s, int lx)
{
	float *ht;
	int ii;
	
	/* get space for hilbert transform */
	if ( (ht = (float *)calloc(lx,sizeof(float))) == (float *)NULL ) {
		fprintf(stderr,"can't get space for Hilbert transform\n");
		return;
	}
	
	/* copy seismogram */
	memcpy((void *)ht, (void *)s, lx * sizeof(*s));
	
	/* get hilbert transform */
	hilbertr(ht,lx);
		
	/* get envelope */
	for ( ii = 0 ; ii < lx ; ii++ )
		s[ii] = sqrt((double)(s[ii]*s[ii] + ht[ii]*ht[ii]));
}	

