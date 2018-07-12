/******************************************************************************
 *
 *	File:			ewspectra.c
 *
 *	Function:		Earthworm module for computing spectra from waveserver data
 *
 *	Author(s):		Scott Hunter, ISTI
 *
 *	Source:			Started anew.
 *
 *	Notes:			
 *
 *	Change History:
 *			4/26/11	Started source
 *	
 *****************************************************************************/


#define EWSPECTRA_VERSION "0.0.3 Mar 16, 2012"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <earthworm.h>
#include <ws2ts.h>
#include <math.h>
#include <string.h>
#include <kom.h>
#include <imp_exp_gen.h>
#include <mem_circ_queue.h>
#include <ew_spectra.h>
#include <transport.h>
#include "iir.h"
#include <ew_spectra_io.h>
#include <ws_clientII.h>
#include <socket_ew.h>

#define MAX_EWS_PROCS	10	/* maximum number of spectra processing definitions */

#define MSGSTK_OFF    0              /* MessageStacker has not been started      */
#define MSGSTK_ALIVE  1              /* MessageStacker alive and well            */
#define MSGSTK_ERR   -1              /* MessageStacker encountered error quit    */
volatile int MessageStackerStatus = MSGSTK_OFF;

QUEUE OutQueue;              /* from queue.h, queue.c; sets up linked    */
                                     /*    list via malloc and free              */
thr_ret MessageStacker( void * );    /* used to pass messages between main thread */
thr_ret Process( void * );

/* Error messages used by export
 ***********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring        */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer      */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded  */
#define  ERR_BADMSG        3   /* message w/ bad timespan                 */
#define  ERR_QUEUE         4   /* error queueing message for sending      */
static char  errText[256];     /* string for log/error messages           */

#define backward 0

char *progname;

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InRingKey;     /* key of transport ring for input    */
static long          OutRingKey;    /* key of transport ring for output   */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */

static  SHM_INFO  InRegion;     /* shared memory region to use for input  */
static  SHM_INFO  OutRegion;    /* shared memory region to use for output */

MSG_LOGO  GetLogo[1];     			/* requesting module,type,instid */
pid_t MyPid;        /* Our own pid, sent with heartbeat for restart purposes */
	
time_t now;        /* current time, used for timing heartbeats */
time_t MyLastBeat;         /* time of last local (into Earthworm) hearbeat */

static int     HeartBeatInt = 30;        /* seconds between heartbeats        */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;

static char *MSrawmsg = NULL;        /* MessageStacker's "raw" retrieved message */
static char *SSmsg = NULL;           /* Process's incoming message buffer   */
static MSG_LOGO Logo;        /* logo of message to re-send */

int static long    MaxMsgSize = 100;          /* max size for input msgs    */
static int     RingSize = 10;        /* max messages in output circular buffer       */

WShandle *wsh;
char *configFileName;

#define THREAD_STACK 8192
static unsigned tidStacker;          /* Thread moving messages from transport */
static unsigned tidProcess;          /* Processor thread id */

int numPeaks = 0;
double peakBand[2];



/*****************************************************************************
   Discrete fourier transform subroutine
   based on the fast fourier transform algorithm
   which computes the quantity:

	     n-1
	x(j)=sum  x(k) exp( 2*pi*i*isign*k*j/n )
	     k=0

   n must be a integral power of 2.
*****************************************************************************/
static void ah_four(double *data, int n, int isign) {
	int ip0, ip1, ip2, ip3, i3rev;
	int i1, i2a, i2b, i3;
	double sinth, wstpr, wstpi, wr, wi, tempr, tempi, theta;

	ip0= 2;
	ip3= ip0*n;
	i3rev= 1;
	for (i3= 1; i3 <= ip3; i3+= ip0) {
		if (i3 < i3rev) {
			tempr= data[i3-1];
			tempi= data[i3];
			data[i3-1]= data[i3rev-1];
			data[i3]= data[i3rev];
			data[i3rev-1]= tempr;
			data[i3rev]= tempi;
		}
		ip1= ip3/2;
		do {
			if (i3rev <= ip1)
				break;
			i3rev-= ip1;
			ip1/= 2;
		} while (ip1 >= ip0);
		i3rev+= ip1;
	}
	ip1= ip0;
	while (ip1 < ip3) {
		ip2= ip1 * 2;
		theta= 6.283185/((double) (isign*ip2/ip0));
		sinth= (double) sin((double) (theta/2.));
		wstpr= -2.*sinth*sinth;
		wstpi= (double) sin((double) theta);
		wr= 1.;
		wi= 0.;
		for (i1= 1; i1 <= ip1; i1+= ip0) {
			for (i3= i1; i3 <ip3; i3+= ip2) {
				i2a= i3;
				i2b= i2a+ip1;
				tempr= wr*data[i2b-1] - wi*data[i2b];
				tempi= wr*data[i2b] + wi*data[i2b-1];
				data[i2b-1]= data[i2a-1] - tempr;
				data[i2b]= data[i2a] - tempi;
				data[i2a-1]+= tempr;
				data[i2a]+= tempi;
			}
			tempr= wr;
			wr= wr*wstpr - wi*wstpi + wr;
			wi= wi*wstpr + tempr*wstpi + wi;
		}
		ip1=ip2;
	}
	return;
}

/*****************************************************************************
   This subroutine takes a real time series and computes its
   discrete fourier transform. The time series has n real points
   and the transform has n/2+1 complex values starting with
   frequency zero and ending at the nyquist frequency.

   Parameters:
   	x	floating point data array
   	n	the number of samples, an integral power of 2
*****************************************************************************/
static void ah_fftr(double *x, int n) {
	int nn, is, nm, j, i;
	int k1j, k1i, k2j, k2i;
	double s, fn, ex, wr, wi, wwr, wrr, wwi, a1, a2, b1, b2;

	nn= n/2;
	is= 1;
	ah_four(x, nn, is);
	nm= nn/2;
	s= x[0];
	x[0]+= x[1];
	x[n]= s - x[1];
	x[1]= 0.0 ;
	x[n+1]= 0.0;
	x[nn+1]= (-x[nn+1]);
	fn= (double) n;
	ex= 6.2831853/fn;
	j= nn;
	wr= 1.0;
	wi= 0.0;
	wwr= (double) cos((double) ex);
	wwi= (double) (-sin((double) ex));
	for (i= 2; i <= nm; i++) {
		wrr= wr*wwr-wi*wwi;
		wi= wr*wwi+wi*wwr;
		wr= wrr;
		k1j= 2*j-1;
		k1i= 2*i-1;
		k2j= 2*j;
		k2i= 2*i;
		a1= 0.5*(x[k1i-1]+x[k1j-1]);
		a2= 0.5*(x[k2i-1]-x[k2j-1]);
		b1= 0.5*(-x[k1i-1]+x[k1j-1]);
		b2= 0.5*(-x[k2i-1]-x[k2j-1]);
		s= b1;
		b1= b1*wr+b2*wi;
		b2= b2*wr-s*wi;
		x[k1i-1]= a1-b2;
		x[k2i-1]= (-a2-b1);
		x[k1j-1]= a1+b2;
		x[k2j-1]=  a2-b1;
		j-= 1;
	}
	return;
}

int scale;							/* 0=no scaling */
int white;							/* 1=scale amplitude output only by mean */
double start, stop;					/* time period from config */
int smooth;							/* 0=no smoothing */
double smooth_width;				/* width (in secs) of smoothing window */
char SCNL[MAX_EWS_PROCS][3][4][10];	/* SCNLs to process */
char binary[MAX_EWS_PROCS];			/* 1=use 2 SCNLs for processing */
char processing_mode[MAX_EWS_PROCS];/* what processing to do:
										p = Plain (unary)
										d = difference (binary)
										*/
int taperType;						/* what kind of tapering to do:
										BARTLETT=1, HANNING=2, 
										PARZAN=3, BMHARRIS=4 
										*/
double taperFrac;					/* fraction of each end to taper */
int num_ews_procs;					/* # of processing requests */
char outpath[100];					/* path for output file */
FILE *fp;							/* output file pointer */
char inRing[MAX_RING_STR];			/* name of input ring */
char outRing[MAX_RING_STR];			/* name of output ring */
int find_triggers;
double highcutoff, lowcutoff;		/* freq for high and low cutoffs */
int highpoles, lowpoles;			/* # poles for high & low cutoffs (0=none) */

char *taperName[5] = {"", "BARTLETT", "HANNING", "PARZAN", "BMHARRIS"};
char *Argv0 = "ewspectra";

static char    MyModName[MAX_MOD_STR];       /* speak as this module name/id      */
int spectraOut = 0;

/***********************************************************************
 * parse_my_command () - handle config file commands not handled in ws2ts
 *		return 0 if handled, 1 if unknown or an error
 ***********************************************************************/
int parse_my_command() {
        char p_char;

	if (k_its("Scale")) {
		scale = 1;
	} else if (k_its("White")) {
		white = 1;
	} else if (k_its("HeartbeatInt")) {
		HeartBeatInt = k_int();
	} else if (k_its("Smooth")) {
		smooth = 1;
		smooth_width = k_val();
	} else if (k_its("Taper")) {
		char *taperTypeName;
		taperTypeName = k_str();
		for ( taperType = 4; taperType>0; taperType-- )
			if ( strcmp( taperTypeName, taperName[taperType] )==0 )
				break;
		if ( taperType == 0 ) {
			logit( "e", "Invalid type of taper: '%s'\n", taperTypeName );
			return 1;
		}
		taperFrac = k_val();
	} else if (k_its("MyModuleId")) {
		strcpy( MyModName, k_str() );
		if ( GetModId( MyModName, &MyModId ) != 0 ) {
		  logit( "e", "%s: Invalid module name <%s>; exiting!\n",
				   Argv0, MyModName );
		  return 1;
		}
	} else if (k_its("DeconvolveSpectraSCNLs") || k_its("DiffSpectraSCNLs")
		|| k_its("PlainSpectraSCNL")) {
		int i,j,step;
		if ( num_ews_procs >= MAX_EWS_PROCS ) {
			logit( "e", "Too many processing requests; ignoring '%s'\n", k_com() );
			return 0;
		}
		step = 1;
                if (k_its("DeconvolveSpectraSCNLs"))
                     p_char='d';
                else if (k_its("DiffSpectraSCNLs"))
                     p_char='s';
                else {
                     p_char='p';
                     step = 2;
                }
		*SCNL[num_ews_procs][2][3] = 0;
		for ( i=0; i<3; i+=step )
			for ( j=0; j<4; j++ ) {
				char *str = k_str();
				if ( str == NULL ) {
					if ( i < 2 ) {
						logit( "e", "Incomplete source SCNL for %s\n", k_com() );
						return 0;
					} else if ( j > 0 ) {
						logit( "e", "Target SCNL must be fully specified for %s\n", k_com() );
						return 0;
					} else {
						break;
					}
				}
				strcpy( SCNL[num_ews_procs][i][j], str );
			}
		processing_mode[num_ews_procs] = p_char;
		binary[num_ews_procs] = (step == 1);
		num_ews_procs++;
	} else if (k_its("TimeSpan")) {
		char *arg1s = k_str();
		double arg1 = atof( arg1s );
		if ( arg1 < 0 ) { 
			start = time(NULL) + arg1;
		} else if ( EWSConvertTime (arg1s, &start) == EW_FAILURE ) {
			return 1;
		}
		stop = start + k_val();
	} else if (k_its("OutFile")) {
		strcpy( outpath, k_str() );
	} else if (k_its("InRing")) {
		find_triggers = 1;
		strcpy( inRing, k_str() );
	   	if ( ( InRingKey = GetKey(inRing) ) == -1 ) {
			logit( "e", "%s:  Invalid ring name <%s>; exiting!\n",
					 Argv0, inRing);
			return 1;
	   	}
	} else if (k_its("OutRing")) {
		strcpy( outRing, k_str() );
	    if ( ( OutRingKey = GetKey(outRing) ) == -1 ) {
			logit( "e", "%s:  Invalid ring name <%s>; exiting!\n",
					 Argv0, outRing);
			return 1;
	   	}
	} else if (k_its("HighCut")) {
		highcutoff = k_val();
		if ( highcutoff <= 0 ) {
			logit( "e", "HighCut cutoff frequency must be > 0\n" );
			return 1;
		}
		highpoles = k_int();
		if ( highpoles % 2 || highpoles < 2 || highpoles > 12 )  {
			logit( "e", "# poles for HighCut must be 2, 4, 6, 8, 10 or 12\n" );
			return 1;
		}
	} else if (k_its("LowCut")) {
		lowcutoff = k_val();
		if ( lowcutoff <= 0 ) {
			logit( "e", "LowCut cutoff frequency must be > 0\n" );
			return 1;
		}
		lowpoles = k_int();
		if ( lowpoles % 2 || lowpoles < 2 || lowpoles > 12 )  {
			logit( "e", "# poles for LowCut must be 2, 4, 6, 8, 10 or 12\n" );
			return 1;
		}
	} else if (k_its("ReportSpectra")) {
		spectraOut = 1;
	} else if (k_its("ReportPeaks")) {
		numPeaks = k_int();
		peakBand[0] = k_val();
		peakBand[1] = k_val();
    } else if( k_its("RingSize") ) {
		RingSize = k_long();
	} else
		return 1;
	return 0;
}



/***********************************************************************
 * deconvolve_spectra () - deconvolve spectra b into a
 *		return 0 if no problems, 1 otherwise
 ***********************************************************************/
int deconvolve_spectra( EW_TIME_SERIES *spec_a, EW_TIME_SERIES *spec_b ) {
	int ndata = spec_a->dataCount * 2;
	int i;
	double denom, result_0, result_1;
	double *a = spec_a->data, *b = spec_b->data;
	for ( i=0; i<=ndata; i+=2 ) {
		denom = b[i]*b[i] + b[i+1]*b[i+1];
		if ( denom == 0.0 )
			a[i] = a[i+1] = 0.0;
		else {
			result_0 = a[i]*b[i] + a[i+1]*b[i+1];
			result_1 = b[i]*a[i+1] - a[i]*b[i+1];
			a[i]   = result_0 / denom;
			a[i+1] = result_1 / denom;
		}
			
	}
	return 0;
}

/***********************************************************************
 * smooth_spectra () - smooth the spectra timeseries
 *		return 0 if no problems, 1 otherwise
 ***********************************************************************/
int smooth_spectra( EW_TIME_SERIES * ewspec, int mode ) {
	double *specsm, even_sum, odd_sum;
	int i, imod;
	double delta = 1/ewspec->trh2x.samprate;
	int smoother= smooth_width/delta;
	int ndata = ewspec->dataCount * 2;
	double *spec = ewspec->data;
	
	/* make sure that smoother is even */
	smoother= (smoother/2)*2;
	if ((specsm= (double *) malloc((smoother)*sizeof(double))) == NULL ) {
		logit("et","%s: memory allocation error\n", progname);
		return 1;
	}
	/* Create & fill smoothing window */
	memcpy( specsm, spec, smoother*2*sizeof(double) );
	even_sum = odd_sum = 0;
	for ( i=0; i<smoother*2; i+=2) {
		if ( mode & EWTS_MODE_FIRST )
			even_sum   += spec[i];
		if ( mode & EWTS_MODE_SECOND )
			odd_sum += spec[i+1];
	}
		
	for (i= smoother, imod=0; i <= ndata-smoother; i+= 2, imod = (imod+2)%(smoother*2) ) {
		if ( mode & EWTS_MODE_FIRST ) {
			even_sum += spec[i+smoother];		/* Add next item to window sums */
			spec[i]   = even_sum / (smoother+1);/* Store next smoothed result */
			even_sum -= specsm[imod];			/* Remove oldest value from sums... */
			specsm[imod]   = spec[i+smoother];	/*... and replace it with newest in window */
		}
		if ( mode & EWTS_MODE_SECOND ) {
			odd_sum  += spec[i+smoother+1];		/* Add next item to window sums */
			spec[i+1] = odd_sum  / (smoother+1);/* Store next smoothed result */
			odd_sum -= specsm[imod+1];		/* Remove oldest value from sums... */
			specsm[imod+1] = spec[i+smoother+1];/*... and replace it with newest in window */
		}
	}
	free(specsm);
	return 0;
}

/***********************************************************************
 * convert_spectra () - convert complex spectra to amp/phase representation
 *		return 0 if no problems, 1 otherwise
 ***********************************************************************/
int convert_spectra( EW_TIME_SERIES * ewspec ) {
	int i, ndata = ewspec->dataCount * 2;
	double *spec = ewspec->data;
	double amp, phase;
	for (i= 0; i <= ndata; i+= 2) {
		amp= (double) sqrt((double) (spec[i]*spec[i]+spec[i+1]*spec[i+1]));
		phase= (double) atan2(spec[i+1], spec[i]);
		spec[i]= amp;
		spec[i+1]= phase;
	}
	ewspec->dataType = EWTS_TYPE_AMPPHS;
	return 0;
}

/* Similar thing for white */

/***********************************************************************
 * calc_spectra () - calculate the spectra for a timeseries, store in 
 *      provided timeseries
 *		return 0 if no problems, 1 otherwise
 ***********************************************************************/
int calc_spectra( EW_TIME_SERIES * ewts, EW_TIME_SERIES * ewspec ) 
{
	double *spec;
	int i, ndata;
	/*double delta = 1/ewts->trh2x.samprate;*/
	
	*ewspec = *ewts;

	ndata = ewts->dataCount;

	/* get nearest power of 2 for listed data length */
	for (i= 1; (int) pow((double) 2., (double) i) < ewts->dataCount; i++);
	ndata= (int) pow((double) 2.,(double) (i+1));
	ewspec->dataCount = ndata/2;
	ewspec->dataType = EWTS_TYPE_COMPLEX;

	/* allocate space for data and smoothing area */
	if ((spec= (double *) malloc((unsigned) (ndata+2)*sizeof(double))) == NULL ) {
		logit("et","%s: memory allocation error\n", progname);
		return 1;
	}
	ewspec->data = spec;
	memcpy( spec, ewts->data, ewts->dataCount*sizeof(double) );
	memset( spec+ewts->dataCount, 0, sizeof(double)*(ndata-ewts->dataCount+2) );

	ah_fftr(spec, ndata);
	
	return 0;
}


void findmax(EW_TIME_SERIES *ewspec, int top3[])
{
	double *arr = ewspec->data;
	int n = ewspec->dataCount;
	double prev = arr[0];
	int i, j, cap, locap;
	int goup = 1;
	double *top3val;

	double delta = 1/ewspec->trh2x.samprate;
	double nyquist= 1.0/(2.0*delta);
	double ndata = ewspec->dataCount - 1;
	double scale = nyquist/ndata;
	
	printf( "Finding %d peaks\n", numPeaks );
	top3val = malloc( sizeof(double) * numPeaks );
	if ( top3val == NULL ) {
		logit( "e", "Failed to allocate memory for peak-finding\n" );
		exit(1);
	}
	for ( i=0; i<numPeaks; i++ ) {
		top3[i] = -1;
		top3val[i] = -1;
	}

	locap = (peakBand[0] / scale)*2;
	if ( locap < 0 )
		locap = 0;
	if ( locap % 2 )
		locap--;
	cap = (peakBand[1] / scale)*2;
	if ( cap % 2 )
		cap++;
	if ( cap > n )
		cap = n;
	for( i=0; i <= cap; i+=2)
	{
		double l,r;
		double cur;
		l = (i - 2 < 0)?arr[0]:arr[i-2];
		r = (i + 2 >= n)?arr[n-2]:arr[i+2];
		cur = (l + 2*arr[i] + r)/4;
		if(goup) {
			if(prev > cur && i > 0) {
				int newidx = i-2;
				double newval = arr[newidx];
				if ( newidx >= locap && newval > top3val[numPeaks-1] ) {
					for ( j=numPeaks-2; j>=0 && newval > top3val[j]; j-- ) {
						top3val[j+1] = top3val[j];
						top3[j+1] = top3[j];
					}
					top3[j+1] = newidx;
					top3val[j+1] = newval;
				}
				goup = 0;
			}
		} else {
			if(prev < cur)
				goup = 1;
		}
		prev = cur;
	}
	free( top3val );
}

/***********************************************************************
 * process_timespan () - process the specified timespan for data gotten
 *		for the SCNLs specified in config from the set of waveservers 
 *		in wsh
 ***********************************************************************/
void process_timespan( 	WShandle *wsh, double start, double stop ) 
{
	int ret, i, pn;
	EW_TIME_SERIES *ewts = NULL, *ewts2 = NULL, ewspec, ewspec2;
	TRACE_REQ tr;
	int *top3 = malloc( sizeof(int)*numPeaks );

	tr.pBuf = NULL;
	/* Try each of the processing requests from config */
	for ( pn=0; pn < num_ews_procs; pn++ ) {
		double delta, nyquist, ndata, scale;
		MSG_LOGO logo;
		char date[2][20];
		char buffer[2000], *specData, *peakData;
	
		/* Get the tracebufs for the first/only SCNL */
		tr.bufLen = (stop - start)*1000000;
		if ( tr.pBuf == NULL ) {
			tr.pBuf = malloc( tr.bufLen );
			if ( tr.pBuf == NULL ) {
				logit( "et", "%s: failed to allocate waveserver buffer\n", progname );
				continue;
			}
		}
		tr.timeout = 500;
		tr.reqStarttime = start;
		tr.reqEndtime = stop;		
		ret = getTraceBufsFromWS( wsh, SCNL[pn][0][0], SCNL[pn][0][1], 
									SCNL[pn][0][2], SCNL[pn][0][3], &tr );
	
		/* Convert to a timeseries */
		if ( ret == 0 )
			ewts = convertTraceBufReq2WEW( &tr, 0 );
		else 
			continue; /* move on to next SCNL */
			
		if ( ewts == NULL )
			continue; /* move on to next SCNL */
		if ( 1 ) {
			char date[2][20];
			EWSUnConvertTime( date[0], tr.actStarttime );
			EWSUnConvertTime( date[1], tr.actEndtime);
			EWSUnConvertTime( date[0], ewts->trh2x.starttime );
			EWSUnConvertTime( date[1], ewts->trh2x.endtime );
		}

		/* Handle the second timeseries, if present */
		if ( binary[pn] ) {
			ret = getTraceBufsFromWS( wsh, SCNL[pn][1][0], SCNL[pn][1][1], 
										SCNL[pn][1][2], SCNL[pn][1][3], &tr );
			if ( ret == 0 )
				ewts2 = convertTraceBufReq2WEW( &tr, 0 );	
			else {
				ws2ts_purge( NULL, NULL, ewts );
				continue;
			}
			if ( ewts2 == NULL ) {
				ws2ts_purge( NULL, NULL, ewts );
				continue;
			}
			if ( 1 ) {
				char date[2][20];
				EWSUnConvertTime( date[0], tr.actStarttime );
				EWSUnConvertTime( date[1], tr.actEndtime);
				EWSUnConvertTime( date[0], ewts2->trh2x.starttime );
				EWSUnConvertTime( date[1], ewts2->trh2x.endtime );
			}
		}
		
		/* Free tracebuf space */
		free( tr.pBuf );
		tr.pBuf = NULL;
		
		/* Complete second timeseries for diff */
		if ( processing_mode[pn] == 's' ) {
			demean_EWTS( *ewts );
			if ( taperType )
				taper_EWTS( *ewts, taperType, taperFrac, EWTS_MODE_BOTH );
			demean_EWTS( *ewts2 );
			if ( taperType )
				taper_EWTS( *ewts2, taperType, taperFrac, EWTS_MODE_BOTH );
			
			subtract_from_EWTS( *ewts, *ewts2, EWTS_MODE_BOTH );
				
			ws2ts_purge( NULL, NULL, ewts2 );
		}		

		/* Demean, filter, taper, & produce spectra */
		demean_EWTS( *ewts );	
		if ( highpoles | lowpoles ) {
			if ( 0 && taperType )
				taper_EWTS( *ewts, taperType, taperFrac, EWTS_MODE_BOTH );
			iir( ewts, lowcutoff, lowpoles, highcutoff, highpoles );
			demean_EWTS( *ewts );
		}
		if ( taperType )
			taper_EWTS( *ewts, taperType, taperFrac, EWTS_MODE_BOTH );
		calc_spectra( ewts, &ewspec );	
		ws2ts_purge( NULL, NULL, ewts );
		
		if ( processing_mode[pn] == 'd' ) {
			demean_EWTS( *ewts2 );
			if ( highpoles | lowpoles ) {
				if ( 0 && taperType )
					taper_EWTS( *ewts2, taperType, taperFrac, EWTS_MODE_BOTH );
				iir( ewts2, lowcutoff, lowpoles, highcutoff, highpoles );
				demean_EWTS( *ewts2 );
			}
			if ( taperType )
				taper_EWTS( *ewts2, taperType, taperFrac, EWTS_MODE_BOTH );
			calc_spectra( ewts2, &ewspec2 );
			ws2ts_purge( NULL, NULL, ewts2 );
			
			deconvolve_spectra( &ewspec, &ewspec2 );
			if ( ewspec2.data != NULL )
				free( ewspec2.data );
		}
		
		convert_spectra( &ewspec );
		if ( smooth )
			smooth_spectra( &ewspec, EWTS_MODE_FIRST | EWTS_MODE_SECOND );
		
		delta = 1/ewspec.trh2x.samprate;
		nyquist= 1.0/(2.0*delta);
		ndata = ewspec.dataCount - 1;
		scale = nyquist/ndata;
		
		if ( OutRingKey != -1 ) {
			memcpy( ewspec.trh2x.sta,  SCNL[pn][2][0], TRACE2_STA_LEN );
			memcpy( ewspec.trh2x.chan,  SCNL[pn][2][1], TRACE2_CHAN_LEN );
			memcpy( ewspec.trh2x.net, SCNL[pn][2][2], TRACE2_NET_LEN );
			memcpy( ewspec.trh2x.loc,  SCNL[pn][2][3], TRACE2_LOC_LEN );
		}
		
		logo.instid = InstId;
		logo.mod    = MyModId;
		
		specData = peakData = buffer;
		EWSUnConvertTime( date[0], start );
		EWSUnConvertTime( date[1], stop );
		sprintf( peakData, "Requested start=%15s\tend=%15s\n", date[0], date[1] );
		peakData += strlen(peakData);
		EWSUnConvertTime( date[0], ewspec.trh2x.starttime );
		EWSUnConvertTime( date[1], ewspec.trh2x.endtime );
		sprintf( peakData, "Actual    start=%15s\tend=%15s\n", 
			date[0], date[1] );
		peakData += strlen(peakData);	
		sprintf( peakData, "Sampling  nsamp=%15ld\trate=%15.3lf\n", 
			ewspec.dataCount, ewspec.trh2x.samprate );
		peakData += strlen(peakData);
		sprintf( peakData, "Source 1: %s.%s.%s.%s", 
			SCNL[pn][0][0], SCNL[pn][0][1], SCNL[pn][0][2], SCNL[pn][0][3] );
		peakData += strlen(peakData);
		if ( binary[pn] ) {
			sprintf( peakData, "\tSource 2: %s.%s.%s.%s\t%s\n", 
				SCNL[pn][1][0], SCNL[pn][1][1], SCNL[pn][1][2], SCNL[pn][1][3],
				processing_mode[pn] == 's' ? "(difference)" : "(deconvolution)");
			peakData += strlen(peakData);
		} else 
			*peakData++ = '\n';
		if ( taperType )
			sprintf( peakData, "Tapered %5.2lf%% (%s)\n", taperFrac*100, taperName[taperType] );
		else
			sprintf( peakData, "Tapering disabled\n" );
		peakData += strlen(peakData);
		if ( smooth ) 
			sprintf( peakData, "Smoothed (%5.2lf Hz)\n", smooth_width );
		else
			sprintf( peakData, "Smoothing disabled\n");			
		peakData += strlen(peakData);
		if ( lowpoles )
			sprintf( peakData, "LowCut  (%d poles) %lf\n", lowpoles, lowcutoff );
		else
			sprintf( peakData, "LowCut disabled\n" );
		peakData += strlen(peakData);
		if ( highpoles )
			sprintf( peakData, "HighCut (%d poles) %lf\n", highpoles, highcutoff );
		else
			sprintf( peakData, "HighCut disabled\n" );
		peakData += strlen(peakData);
		specData = peakData;

		if ( numPeaks > 0 ) {
			findmax( &ewspec, top3 );
			
			sprintf( peakData, "Peak band: %15.3lf thru %15.3lf\n", peakBand[0], peakBand[1] );
			peakData += strlen(peakData);
			for ( i=0; i<numPeaks; i++ ) {
				if ( top3[i] == -1 )
					sprintf( peakData, "Peak %d: Not in band\n", i+1 );
				else
					sprintf( peakData, "Peak %d: Freq = %lf, amp = %lf\n", i+1, 
						top3[i]*scale/2, ewspec.data[top3[i]] );
				peakData += strlen( peakData );
			}

			if ( OutRingKey != -1 ) {
				printf("Writing peaks to ring\n");
				GetType( "TYPE_SPECTRA_PEAKS", &(logo.type) );
				peakData[0] = 0;
				if( tport_putmsg( &OutRegion, &logo, peakData-buffer, buffer ) != PUT_OK ) {
				   logit("et", "%s:  Error sending writing peaks.\n",
						  Argv0 );
				}
			} 
			if ( fp == NULL && OutRingKey == -1 )
				logit( "w", "ewspectra: Peaks requested, but not output means specified\n" );
		}
 
		if ( outpath[0] != 0 ) {
			fp = fopen( outpath, "a" );
			if ( fp == NULL )
				logit( "e", "ewspectra: Could not open output file: '%s'\n", outpath );
 			fwrite( buffer, peakData-buffer, 1, fp );
 		}
 			
		if ( spectraOut ) {			
			printf("ReportSpectra %lf %lf %lf %lf\n", delta, nyquist, ndata, scale);
			if ( fp != NULL ) {
				printf("Writing spectra to file\n");
				write_EWTS_as_spectra_to_file( &ewspec, Argv0, fp );
			} 
			if ( OutRingKey != -1 ) {
				write_EWTS_as_spectra_to_ring( &ewspec, Argv0, &OutRegion, &logo );
			}
			if ( fp == NULL && OutRingKey == -1 )
				logit( "w", "ewspectra: Spectra output requested, but not output means specified\n" );
		}
		
		if ( fp != NULL ) {
			fclose( fp );
			fp = NULL;
		}
			
		if ( ewspec.data != NULL )
			free( ewspec.data );
		
	}
	
	/* Free tracebuf space, if still allocated */
	if ( tr.pBuf != NULL ) 
		free( tr.pBuf );

}

/***************************************************************************
 * ewspectra_status() builds a heartbeat or error message & puts it into      *
 *                 shared memory.  Writes errors to log file & screen.     *
 ***************************************************************************/
void ewspectra_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char        msg[256];
   long        size;
   time_t      t;

/* Build the message
 *******************/
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
    sprintf( msg, "%ld %ld\n%c", (long) t, (long) MyPid, (char)0);
   else if( type == TypeError )
   {
    sprintf( msg, "%ld %hd %s\n%c", (long) t, ierr, note, 0);

    logit( "et", "%s(%s): %s\n", Argv0, MyModName, note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &InRegion, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","%s(%s):  Error sending heartbeat.\n",
                  Argv0, MyModName );
    }
    else if( type == TypeError ) {
           logit("et", "%s(%s):  Error sending error:%d.\n",
                  Argv0, MyModName, ierr );
    }
   }

   return;
}

/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *dummy )
{
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;       /* logo of retrieved message             */
   int       res;
   int       ret;
   char      date[100], *msgName;
   int modid, len, okMsg;
   int           NumOfTimesQueueLapped= 0; /* number of messages lost due to
                                             queue lap */

   /* Tell the main thread we're ok
   ********************************/
   MessageStackerStatus = MSGSTK_ALIVE;

   /* Start main export service loop for current connection
   ********************************************************/
   while( 1 )
   {
      /* Get a message from transport ring
      ************************************/
      recsize = 0;
      res = tport_getmsg( &InRegion, GetLogo, 1,
                          &reclogo, &recsize, MSrawmsg, MaxMsgSize );
      MSrawmsg[recsize<MaxMsgSize?recsize:MaxMsgSize] = '\0';

      /* Wait if no messages for us
       ****************************/
      if( res == GET_NONE ) {sleep_ew(100); continue;}
      
      /* Check return code; report errors
      ***********************************/
      if( res != GET_OK )
      {
         if( res==GET_TOOBIG )
         {
            sprintf( errText, "msg[%ld] i%d m%d t%d too long for target",
                            recsize, (int) reclogo.instid,
                (int) reclogo.mod, (int)reclogo.type );
            ewspectra_status( TypeError, ERR_TOOBIG, errText );
            continue;
         }
/* we don't care about GET_MISS because activate_module will never be tracked, it is transient

         else if( res==GET_MISS )
         {
            sprintf( errText, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
                    (int) reclogo.mod, (int)reclogo.type, inRing );
            ewspectra_status( TypeError, ERR_MISSMSG, errText );
         }
*/
         else if( res==GET_NOTRACK )
         {
            sprintf( errText, "no tracking for logo i%d m%d t%d in %s",
                     (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                     inRing );
            ewspectra_status( TypeError, ERR_NOTRACK, errText );
         }
      }
      
      res = sscanf( MSrawmsg, "%d %s %d", &modid, date, &len );
      if ( modid != MyModId )
      	continue;
      msgName = "ACTIVATE_MODULE";
      logit( "e", "%s: received new ACTIVATE_MODULE message: %s\n", Argv0, MSrawmsg);
      okMsg = (res == 3);
	  if ( !okMsg ) {
			sprintf( errText, "malformed %s msg i%d m%d t%d in %s",msgName,
					(int) reclogo.instid,
					(int) reclogo.mod, (int)reclogo.type, inRing );
			ewspectra_status( TypeError, ERR_BADMSG, errText );
			continue;
      }

      /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
      ***********************************************************/
      if ( wsh == NULL ) {
      	wsh = init_ws( configFileName, progname, NULL );
      	if ( wsh == NULL ) {
      		logit( "e", "%s: Retry connecting to waveservers failed; exiting!\n", Argv0 );
      		exit(-1);
      	}
      }

      /* put it into the 'to be shipped' queue */
      /* the Process thread is in the biz of de-queueng and processing */
      RequestMutex();
      ret=enqueue( &OutQueue, MSrawmsg, recsize, reclogo );
      ReleaseMutex_ew();

      if ( ret!= 0 )
      {
         if (ret==-2)  /* Serious: quit */
         {    /* Currently, eneueue() in mem_circ_queue.c never returns this error. */
        sprintf(errText,"internal queue error. Terminating.");
            ewspectra_status( TypeError, ERR_QUEUE, errText );
        goto error;
         }
         if (ret==-1)
         {
            sprintf(errText,"queue cannot allocate memory. Lost message.");
            ewspectra_status( TypeError, ERR_QUEUE, errText );
            continue;
         }
         if (ret==-3)  /* Log only while client's connected */
         {
         /* Queue is lapped too often to be logged to screen.
          * Log circular queue laps to logfile.
          * Maybe queue laps should not be logged at all.
          */
            NumOfTimesQueueLapped++;
            if (!(NumOfTimesQueueLapped % 5))
            {
               logit("t",
                     "%s(%s): Circular queue lapped 5 times. Messages lost.\n",
                      Argv0, MyModName);
               if (!(NumOfTimesQueueLapped % 100))
               {
                  logit( "et",
                        "%s(%s): Circular queue lapped 100 times. Messages lost.\n",
                         Argv0, MyModName);
               }
            }
            continue;
         }
      }


   } /* end of while */

   /* we're quitting
   *****************/
error:
   MessageStackerStatus = MSGSTK_ERR; /* file a complaint to the main thread */
   KillSelfThread(); /* main thread will restart us */
   return NULL; /* Should never get here */
}

/**************************  Main Dup Thread   ***********************
*          Pull a messsage from the queue, and put it on OutRing     *
**********************************************************************/

thr_ret Process( void *dummy )
{
   int      ret;
   int       res;
   long     msgSize;
   double    start, stop;
   int modid, len;
   char      date[100], *msgName;

   while (1)   /* main loop */
   {
     /* Get message from queue
      *************************/
     RequestMutex();
     ret=dequeue( &OutQueue, SSmsg, &msgSize, &Logo);
     ReleaseMutex_ew();
     if(ret < 0 )
     { /* -1 means empty queue */
       sleep_ew(500); /* wait a bit (changed from 1000 to 500 on 970624:ldd) */
       continue;
     }

      /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
      ***********************************************************/

      res = sscanf( SSmsg, "%d %s %d", &modid, date, &len );
      logit("t", "Processing new ACTIVATE_MODULE message: %s\n", SSmsg);
      msgName = "ACTIVATE_MODULE";

  	  start = atof( date );
	  if ( start < 0 ) { 
		start += time(NULL);
	  } else if ( EWSConvertTime (date, &start) == EW_FAILURE ) {
		sprintf( errText, "%s message w/ bad time (%s)", msgName, date );
		ewspectra_status( TypeError, ERR_BADMSG, errText );
	  }
	  stop = start + len;
  
	  process_timespan( wsh, start, stop );

   }   /* End of main loop */

}

int main(int argc, char **argv)
{
/* Other variables: */
   int           res;
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;   /* logo of retrieved message             */
	
	/* Look up installations of interest
	*********************************/
	if ( GetLocalInst( &InstId ) != 0 ) {
	  fprintf( stderr,
			  "%s: error getting local installation id; exiting!\n",
			   Argv0 );
	  exit( -1 );
	}
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", Argv0 );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_ERROR>; exiting!\n", Argv0 );
      exit( -1 );
   }

	if ((progname= strrchr(*argv, (int) '/')) != (char *) 0)
		progname++;
	else
		progname= *argv;
	
    logit_init (progname, 0, 1024, 1);

    /* Check command line arguments */
    if (argc != 2) {
		fprintf (stderr, "Usage: %s <configfile>\n", progname);
		fprintf (stderr, "Version: %s\n", EWSPECTRA_VERSION);
		return EW_FAILURE;
    }
    	
	/* Get our own Pid for restart purposes
   	***************************************/
   	MyPid = getpid();
   	if(MyPid == -1)
   	{
      logit("e", "%s: Cannot get pid; exiting!\n", Argv0);
      return(0);
   	}

	/* Read-in & interpret config file */
	scale= white= smooth= taperType= 0;
	num_ews_procs = 0;
	outpath[0] = 0;
	find_triggers = 0;
	lowpoles = highpoles = lowcutoff = highcutoff = 0;
	InRingKey = OutRingKey = -1;
	start = -1;
	wsh = init_ws( argv[1], progname, parse_my_command );
	
	if ( wsh == NULL ) {
		logit( "w", "%s: Problem initializing WSH (waveservers not up yet?); will try again later\n",
			Argv0 );
		configFileName = argv[1];
	}

	if ( num_ews_procs == 0 ) {
		logit( "e", "No SCNLs specified; exiting\n" );
		ws2ts_purge( wsh, NULL, NULL );
		exit(1);
	}
	if ( OutRingKey == -1 && outpath[0] == 0 ) {
		logit( "e", "ewspectra: No output specified; exiting\n" );
		ws2ts_purge( wsh, NULL, NULL );
		exit(1);
	}
	if ( lowcutoff > 0 && highcutoff > 0 && lowcutoff >= highcutoff ) {
		logit( "e", "ewspectra: Low Cutoff Frequency Must Be < High Cutoff Frequency.\n" );
		ws2ts_purge( wsh, NULL, NULL );
		exit(1);
	}
	if ( OutRingKey != -1 ) {
		int i;
		for ( i=num_ews_procs-1; i>=0; i-- )
			if ( *SCNL[i][2][3] == 0 ) {
				if ( outpath[0] != 0 ) {
					logit( "w", "ewspectra: No SCNL for ring messages specified for %s.%s.%s.%s; will only write to file\n",
						SCNL[i][0][0], SCNL[i][0][1], SCNL[i][0][2], SCNL[i][0][3] );
				} else {
					logit( "w", "ewspectra: No SCNL for ring messages specified for %s.%s.%s.%s; aborting this SCNL\n",
						SCNL[i][0][0], SCNL[i][0][1], SCNL[i][0][2], SCNL[i][0][3] );
					num_ews_procs--;
					if ( i < num_ews_procs ) {
						int j,k;
						for ( j=0; j<3; j+=(binary[i]?1:2) )
							for ( k=0; j<4; k++ )
								strcpy( SCNL[i][j][k], SCNL[num_ews_procs][j][k] );
						binary[i] = binary[num_ews_procs];
						processing_mode[i] = processing_mode[num_ews_procs];
					}
				}
			}
		if ( num_ews_procs == 0 ) {
			logit( "e", "ewspectra: All SCNLs aborted; exiting\n" );
			ws2ts_purge( wsh, NULL, NULL );
			exit(1);
		}
		
	}
	if ( find_triggers ) {
	   	int geti = GetInst( "INST_WILDCARD", &(GetLogo[0].instid) );
	   	int getm = GetModId( "MOD_WILDCARD", &(GetLogo[0].mod) );
	   	int gett = GetType( "TYPE_ACTIVATE_MODULE", &(GetLogo[0].type) );
	   	int getMsg = 0;
		if ( ( MSrawmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
		{
		  logit( "e", "%s: error allocating MSrawmsg; exiting!\n",
				 Argv0 );
		  getMsg = 1;
		}
	   	if ( geti || getm || gett || getMsg ) {
	   		if ( geti )
	   			logit( "e", "%s: INST_WILDCARD unknown; exiting!\n", Argv0 );
	   		if ( getm )
	   			logit( "e", "%s: MOD_WILDCARD unknown; exiting!\n", Argv0 );
	   		if ( gett )
	   			logit( "e", "%s: TYPE_ACTIVATE_MODULE unknown; exiting!\n", Argv0 );
	   		ws2ts_purge( wsh, NULL, NULL );
	   		exit(1);
	   	}
		tport_attach( &InRegion, InRingKey );		
	}
	if ( OutRingKey != -1 ) {
		tport_attach( &OutRegion, OutRingKey );
	}
	
   /* Buffers for Process thread: */
   if ( ( SSmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating SSmsg; exiting!\n",
              Argv0, MyModName );
            if ( InRingKey != -1 )
	      		tport_detach( &InRegion );
	      	if ( OutRingKey != -1 )
      			tport_detach( &OutRegion );
	   		ws2ts_purge( wsh, NULL, NULL );
	   		exit(1);
   }

   /* Create a Mutex to control access to queue
   ********************************************/
   CreateMutex_ew();

   /* Initialize the message queue
   *******************************/
   initqueue( &OutQueue, (unsigned long)RingSize,(unsigned long)MaxMsgSize+1 );

   /* Start the socket writing thread
   ***********************************/
   if ( StartThread(  Process, (unsigned)THREAD_STACK, &tidProcess ) == -1 )
   {
      logit( "e", "%s(%s): Error starting Process thread; exiting!\n",
              Argv0, MyModName );
	  if ( InRingKey != -1 )
			tport_detach( &InRegion );
	  if ( OutRingKey != -1 )
			tport_detach( &OutRegion );
	  free( SSmsg );
	  ws2ts_purge( wsh, NULL, NULL );
      exit( -1 );
   }

	if ( start != -1 ) {
	    int len = (stop-start+0.9);
	    int ret;

 	    reclogo.instid = InstId;
 	    reclogo.mod    = MyModId;
	    reclogo.type   = GetLogo[0].type;
		sprintf( MSrawmsg, "%5d %14s %9d%c", MyModId, "", len, 0 );
		EWSUnConvertTime( MSrawmsg+6, start );
		MSrawmsg[20] = ' ';
        RequestMutex();
        ret=enqueue( &OutQueue, MSrawmsg, 31, reclogo );
        ReleaseMutex_ew();
	}

	/* Here's where, if InputRing is defined, we'd listen for 
		COMPUTE_SPECTRA messages, and call process_timespan
		for their timespans */
	if ( find_triggers ) {

	   /* step over all messages from transport ring
	   *********************************************/
	   /* As Lynn pointed out: if we're restarted by startstop after hanging,
		  we should throw away any of our messages in the transport ring.
		  Else we could end up re-sending a previously sent message, causing
		  time to go backwards... */
	   do
	   {
		 res = tport_getmsg( &InRegion, GetLogo, 1,
							 &reclogo, &recsize, MSrawmsg, MaxMsgSize );
	   } while (res !=GET_NONE);
	
	   /* One heartbeat to announce ourselves to statmgr
	   ************************************************/
	   ewspectra_status( TypeHeartBeat, 0, "" );
	   time(&MyLastBeat);
	
	
	   /* Start the message stacking thread if it isn't already running.
		****************************************************************/
	   if (MessageStackerStatus != MSGSTK_ALIVE )
	   {
		 if ( StartThread(  MessageStacker, (unsigned)THREAD_STACK, &tidStacker ) == -1 )
		 {
		   logit( "e",
				  "%s(%s): Error starting  MessageStacker thread; exiting!\n",
			  Argv0, MyModName );
		   tport_detach( &InRegion );
		   tport_detach( &OutRegion );
		   return( -1 );
		 }
		 MessageStackerStatus = MSGSTK_ALIVE;
	   }
	
	
	   /* Start main ringdup service loop
	   **********************************/
	   while( tport_getflag( &InRegion ) != TERMINATE  &&
			  tport_getflag( &InRegion ) != MyPid         )
	   {
		 /* Beat the heart into the transport ring
		  ****************************************/
		  time(&now);
		  if (difftime(now,MyLastBeat) > (double)HeartBeatInt )
		  {
			  ewspectra_status( TypeHeartBeat, 0, "" );
		  time(&MyLastBeat);
		  }
	
		  /* take a brief nap; added 970624:ldd
		   ************************************/
		  sleep_ew(500);
	   } /*end while of monitoring loop */
		
	}
		
	/* Clean up after ourselves */
	if ( InRingKey != -1 )
		tport_detach( &InRegion );
	if ( OutRingKey != -1 )
		tport_detach( &OutRegion );
	ws2ts_purge( wsh, NULL, NULL );
	return(0);
}

