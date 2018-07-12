#ifdef _WIN32
# include <windows.h>
# define strcasecmp _stricmp
# define strncasecmp _strnicmp
#	define _USE_MATH_DEFINES
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

static float *sphilb(int *);
static void spmask(float *, int, int, float *); 
static double spwndo(int, int, int);
static void spfilt(float *, float *, int, int, float *, int);

#define HLEN 129
/* HILBERTR
 * replace a float array with its Hilbert transform
 * using a FIR filter method.
 */
void hilbertr(float *x, int len)
{
	float *h;
	float *hfir = (float *)NULL;
	int hlen = HLEN;

	/* get FIR coefficients.  If hlen is even, hlen -> hlen + 1 */	
	if ( (hfir = sphilb(&hlen)) == (float *)NULL )
		return;
	
	/* get work space for transform */	
	if ( (h = (float *)calloc(len + hlen,sizeof(float))) == (float *)NULL) {
		fprintf(stderr,"hilbertr: can't get space\n");
		return;
	}

	/* copy x to work space */
	memcpy((void *)h,(void *)x,len * sizeof(float));
	
	/* filter */
	spfilt(hfir,(float *)NULL,hlen,0,h,len + hlen);

	/* copy transform back to x, removing the group delay hlen/2 */
	h += hlen/2;
	memcpy((void *)x,(void *)h,len * sizeof(float));
	
	return;	
}

/* SPHILB - adapted from Stearns and David, Signal Processing Algorithms
 * generates an array of weights of an fir hilbert transformer.
 * after execution the weights are in x(0) through x(l-1), where
 *   l=lx if lx is odd or l=lx+1 if lx is even.
 * when used as a causal filter, the transformer has approximately
 *   unit gain, a group delay of (l-1)/2 samples, plus
 *   approximately 90 degrees phase shift at all frequencies.
 * 
 */
static float *sphilb(int *lx)
{
	int l2, k;
	float tsv, *x;

	if ( *lx % 2 == 0 )
		*lx += 1;	
	if ( (x = (void *)calloc(*lx,sizeof(float))) == (float *)NULL) {
		fprintf(stderr,"sphilb: can't get space\n");
		return(x);
	}
	
	l2 = *lx / 2;
	x[l2] = 0.;
	for( k = 1 ; k <= l2 ; k++ ) {
		x[l2 + k] = ( k % 2 ? 2./(M_PI*k) : 0. );
		x[l2 - k] = ( k % 2 ? -x[l2 + k] : 0. );
	}
	spmask(x, *lx, 5, &tsv);
	return(x);
}

/* SPMASK - adapted from Stearns and David, Signal Processing Algorithms
 * this routine applies a data window to the data vector x(0:ix).
 * itype=1(rectangular), 2(tapered rectangular), 3(triangular),
 *       4(hanning), 5(hamming), or 6(blackman).
 *       (note:  tapered rectangular has cosine-tapered 10% ends.)
 * lx = length of x = ix + 1
 * tsv= sum of squared window values.
 */
static void spmask(float *x, int lx, int itype, float *tsv) 
{
	int k;
	float w;

	*tsv = 0.;
	if ( itype < 1 || itype > 6 )
		return;
	for( k = 0 ; k < lx ; k++ ) {
		w = spwndo(itype, lx, k);
		x[k] *= w;
		*tsv += w*w;
	}
	return;
}

/* SPWNDO - adapted from Stearns and David, Signal Processing Algorithms
 * this function generates a single sample of a data window.
 * itype=1(rectangular), 2(tapered rectangular), 3(triangular),
 *       4(hanning), 5(hamming), or 6(blackman).
 *       (note:  tapered rectangular has cosine-tapered 10% ends.)
 * n=size (total no. samples) of window.
 * k=sample number within window, from 0 through n-1.
 *   (if k is outside this range, spwndo is set to 0.)
 */
static double spwndo(int itype, int n, int k)
{
	float val = 0.;

	if ( !(itype < 1 || itype > 6) ) {
		if ( !(k < 0 || k >= n) ) {
			if ( itype == 1 )
				val = 1.;
			if ( itype == 2 ) {
				int ll;
				ll = (n - 2)/10;
				if ( k <= ll )
					val = 1.0 - cos(k*M_PI/(ll+1));
				else if ( k > n - ll - 2 )
					val = 1.0 - cos((n-k-1)*M_PI/(ll+1));
				else
					val = 2.;
				val *= .5;
			} else if ( itype == 3 )
				val = 1.0 - fabs(1.0 - 2*k/(n-1.0));
			else if ( itype == 4 )
				val = 0.5 * (1.0 - cos(2*k*M_PI/(n-1)));
			else if ( itype == 5 )
				val = 0.54 - 0.46 * cos(2*k*M_PI/(n-1));
			else if ( itype == 6 )
				val = 0.42 - 0.5 * cos(2*k*M_PI/(n-1))
						+ 0.08*cos(4*k*M_PI/(n-1));
		}
	}
	return( val );
}

/* SPFILT - adapted from Stearns and David, Signal Processing Algorithms
 * filters n-point data sequence in place using array x
 * transfer function coefficients are in arrays b and a
 *            b(0)+b(1)*z**(-1)+.......+b(lb)*z**(-ib)
 *     h(z) = ----------------------------------------
 *              1+a(1)*z**(-1)+.......+a(la)*z**(-ia)
 *
 * lb = length of b-array = ib+1
 * la = length of a-array = ia
 */
static void spfilt(float *b, float *a, int lb, int la, float *x, int n)
{
	int ii, jj;
	float *px, *py;
	double sum;

	if ( lb > 0 ) {
		if ( (px = calloc(lb,sizeof(*px))) == NULL ) {
			fprintf(stderr,"spfilt: can't get space\n");
			return;
		}
	}
	if ( la > 0 ) {
		if ( (py = calloc(la,sizeof(*py))) == NULL ) {
			fprintf(stderr,"spfilt: can't get space\n");
			return;
		}
	}

	for ( ii = 0 ; ii < n ; ii++ ) {
		px[0] = x[ii];
		sum = 0.;
		for( jj = 0 ; jj < lb && jj <= ii ; jj++ )
			sum += b[jj] * px[jj];
		for( jj = 0 ; jj < la && jj < ii ; jj++ )
			sum -= a[jj] * py[jj];
		x[ii] = sum;
		if ( lb > 0 )
			memmove((void *)(px+1), (void *)px, (lb-1)*sizeof(*px));
		if ( la > 0 ) {
			memmove((void *)(py+1), (void *)py, (la-1)*sizeof(*py));
			py[0] = sum;
		}
	}
	if ( lb >0 )
		free((void *)px);
	if ( la > 0 )
		free((void *)py);
	return;
}
