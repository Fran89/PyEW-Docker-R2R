#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
# include <windows.h>
# define strcasecmp _stricmp
# define strncasecmp _strnicmp
# define _USE_MATH_DEFINES
#endif
#include <math.h>
#ifndef M_PI
# define M_PI		3.14159265358979323846
#endif
#define TWOPI (2. * M_PI)

static void fftrc(float *, int, int);
static int power_of_2(int);

#define NAIVE
/* Replaces a real series, float data, arbitrary length with its
   Hilbert transform using an FFT method.  If there are no bugs.
   Otherwise, replaces a real series with its Dilbert transform. */
void hilbertr(float *x, int n)
{
	int nn, mm;
	double cosdx, cosx, sindx, sinx, tmp;
	float *wrk, *wp, *xp, ftmp;
	size_t n_aug;
	
	/* get smallest power of 2 >= n */
	n_aug = power_of_2(n);
	
	/* need twice this much space to do the Hilbert transform */
	if ( (wrk = (float *)calloc(2*n_aug,sizeof(float))) == (float *)NULL ) {
		fprintf(stderr,"Can't get space for Hilbert transform\n");
		exit(1);
	}
	
	/* copy real series into first half of work space */
	for ( xp = x, wp = wrk ; xp < x + n ; *wp++ = *xp++ );

	/* take the forward transform of the n_aug-point real series by
	   treating it as a n_aug/2-point complex series */		
	fftrc(wrk, n_aug/2, -1);

	/* unscramble, get negative frequency components, and put in phase
	   delay.  Need the complete n_aug-point complex spectrum to get
	   the Hilbert transform */
#ifdef NAIVE
	cosdx = cos(TWOPI/n_aug);	sindx = sin(TWOPI/n_aug);
	cosx = cosdx;			sinx = sindx;
#else
	cosdx = sin(0.5*TWOPI/n_aug); cosdx *= -2. * cosdx;
	sindx = sin(TWOPI/n_aug);
	cosx = 1. + cosdx;		sinx = sindx;
#endif
		/* special cases, order is important */
	wrk[n_aug] = wrk[0] - wrk[1];
	wrk[n_aug+1] = 0.;
	wrk[0] = wrk[0] + wrk[1];
	wrk[1] = 0.;
	wrk[n_aug/2] = wrk[n_aug/2];		/* for completeness */
	wrk[n_aug/2+1] = -wrk[n_aug/2+1];
	wrk[n_aug] = wrk[0] - wrk[1];
	wrk[n_aug+1] = 0.;
	wrk[3*n_aug/2] = wrk[n_aug/2];
	wrk[3*n_aug/2+1] = -wrk[n_aug/2+1];
#ifndef DO_NOTHING
	/* multiply positive frequencies by -i, negative freqencies by i */
	ftmp = wrk[1], wrk[1] = -wrk[0], wrk[0] = ftmp;
	ftmp = wrk[3*n_aug/2+1], wrk[3*n_aug/2+1] = wrk[3*n_aug/2],
						wrk[3*n_aug/2] = -ftmp;
	wrk[0] = wrk[1] = wrk[n_aug/2] = wrk[n_aug/2+1] = 0.;
						/* by definition */
#endif	
		/* and now, the rest of the story */
	for ( nn = 2, mm = n_aug - 2 ; nn < n_aug/2 ; nn+=2, mm-=2 ) {
		int nr, ni, mr, mi, nnr, nni, mmr, mmi;
		double aa, bb, cc, dd;
		
		/* get some constants */
		nr = nn;
		ni = nr + 1;
		mr = mm;
		mi = mr + 1;
		aa = wrk[nr] + wrk[mr];
		bb = wrk[ni] + wrk[mi];
		cc = wrk[nr] - wrk[mr];
		dd = wrk[ni] - wrk[mi];
		
		/* unscramble first quarter */
		wrk[nr] = (aa + cosx * bb - sinx * cc) * 0.5 ;
		wrk[ni] = (dd - sinx * bb - cosx * cc) * 0.5 ;
		
		/* unscramble second quarter */
		wrk[mr] = (aa - cosx * bb + sinx * cc) * 0.5 ;
		wrk[mi] = (-dd - sinx * bb - cosx * cc) * 0.5 ;
		
		/* get third and fourth quarters from first two */
		nnr = 2 * n_aug - nr;
		nni = nnr + 1;
		mmr = 2 * n_aug - mr;
		mmi = mmr + 1;
		wrk[nnr] = wrk[nr];
		wrk[nni] = -wrk[ni];
		wrk[mmr] = wrk[mr];
		wrk[mmi] = -wrk[mi];
		
		/* update sin and cos */
		tmp = cosx;
#ifdef NAIVE
		cosx = cosx * cosdx - sinx * sindx;
		sinx = sinx * cosdx + tmp * sindx;
#else
		cosx += cosx * cosdx - sinx * sindx;
		sinx += sinx * cosdx + tmp * sindx;
#endif
		
		/* Have complete n_aug-point complex Fourier spectrum.
		   Now construct the DFT of the Hilbert transform by
		   applying the uniform pi/2 phase delay. */
		   
		/* lag positive frequencies (first half) by pi/2, i.e.,
		   multiply by -i. */ 
#ifndef DO_NOTHING
		ftmp = wrk[ni], wrk[ni] = -wrk[nr], wrk[nr] = ftmp;
		ftmp = wrk[mi], wrk[mi] = -wrk[mr], wrk[mr] = ftmp;
		
		/* advance negative frequencies (second half) by pi/2, i.e.,
		   multiply by i */
		ftmp = wrk[nni], wrk[nni] = wrk[nnr], wrk[nnr] = -ftmp;
		ftmp = wrk[mmi], wrk[mmi] = wrk[mmr], wrk[mmr] = -ftmp;
#endif	
	}
	
	/* take the inverse transform of the n_aug-length complex series */
	fftrc(wrk, n_aug, 1);
	
	/* copy n points of the inverse transform (which is real up to machine
	   round-off errors) back out to the input, after dividing by the
	   FFT length */
	for ( xp = x, wp = wrk; xp < x + n ; xp++, wp+=2 )
		*xp = *wp / n_aug;
		
	/* free work space */
	free((void *)wrk);
	
	return;
}

/* forward/inverse transform, complex float data */
static void fftrc(float *data, int n, int isign)
{
	int ip0, ip1, ip2, ip3, i3rev;
	int i1, i2a, i2b, i3;
	double sinth, wstpr, wstpi, wr, wi, tempr, tempi, theta;

	ip0=2;
	ip3=ip0*n;
	i3rev=1;
	for( i3=1; i3<=ip3; i3+=ip0 ) {
		if( i3 < i3rev ) {
			tempr = data[i3-1];
			tempi = data[i3];
			data[i3-1] = data[i3rev-1];
			data[i3] = data[i3rev];
			data[i3rev-1] = tempr;
			data[i3rev] = tempi;
		}
		ip1 = ip3 / 2;
		do {
			if( i3rev <= ip1 )
				break;
			i3rev -= ip1;
			ip1 /= 2;
		} while ( ip1 >= ip0 );
		i3rev += ip1;
	}
	ip1 = ip0;
	while ( ip1 < ip3 ) {
		ip2 = ip1 * 2;
		theta = isign * TWOPI * ip0 / ip2;
		sinth = sin( theta/2. );
		wstpr = -2.*sinth*sinth;
		wstpi = sin(theta);
		wr = 1.;
		wi = 0.;
		for ( i1=1; i1<=ip1; i1+=ip0 ) {
			for ( i3=i1; i3<ip3; i3+=ip2 ) {
				i2a=i3;
				i2b=i2a+ip1;
				tempr = wr*data[i2b-1] - wi*data[i2b];
				tempi = wr*data[i2b] + wi*data[i2b-1];
				data[i2b-1] = data[i2a-1] - tempr;
				data[i2b] = data[i2a] - tempi;
				data[i2a-1] += tempr;
				data[i2a] += tempi;
			}
			tempr = wr;
			wr += wr*wstpr - wi*wstpi;
			wi += wi*wstpr + tempr*wstpi;
		}
		ip1=ip2;
	}
	return;
}

/* power_of_2
 *	returns the power of 2 >= npts
 */
static int power_of_2(int npts)
{
	int n = 2;

	while( n < npts )
		n *= 2;

	return( n );
}
