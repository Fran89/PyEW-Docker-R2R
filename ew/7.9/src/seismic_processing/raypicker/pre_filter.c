/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pre_filter.c 4475 2011-08-04 15:17:09Z kevin $
 *
 *    Revision history:
 *     $Log: pre_filter.c,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:49  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.11  2004/11/01 02:03:27  cjbryan
 *     removed all amplitude determination code from the picker and changed all error
 *     codes to positive numbers
 *     CVS ----------------------------------------------------------------------
 *
 *     Revision 1.10  2004/09/14 21:23:47  cjbryan
 *     *** empty log message ***
 *
 *     Revision 1.9  2004/07/19 16:25:14  cjbryan
 *     deleted misplaced malloc in SetupShortPeriodFilter
 *
 *     Revision 1.8  2004/07/13 19:23:02  cjbryan
 *     added RECURSIVE_FILTER struct and cleaned up filter code
 *
 *     Revision 1.7  2004/06/10 20:22:35  cjbryan
 *     re-engineered array initialization
 *
 *     Revision 1.6  2004/06/03 16:40:48  cjbryan
 *     more cleanup
 *
 *     Revision 1.5  2004/06/03 16:25:18  cjbryan
 *     fixed bug introduced in creating library routines from embedded code
 *
 *     Revision 1.4  2004/05/27 20:44:23  cjbryan
 *     changed MAGTYPE to AMPTYPE to match Ray's defs for amp msgs
 *
 *     Revision 1.3  2004/04/23 17:31:39  cjbryan
 *     changed bool to int
 *
 *     Revision 1.2  2004/04/16 22:17:47  cjbryan
 *     *** empty log message ***
 *
 *
 *
 */
 
/*
 * Functions used to handle broadband and 
 * short-period pre-filters and the shared 
 * processing to transform each into a
 * similar pseudo short-period time series.
 * 
 * @author Ray Buland (original FORTRAN)
 */

/* system includes */
#include <stdlib.h>
#include <math.h>
#include <string.h>           /* for memcpy */


/* earthworm includes */
#include <platform.h>
#include <earthworm.h>
#include <transferFunction.h>
#include <math_constants.h>
#include <complex_math.h>
#include <convertInstResponse.h>


/* raypicker includes */
#include "pre_filter.h"
#include "butterworth.h"
#include "raypicker.h"
#include "returncodes.h"

#define MAX_POLES  8
#define MAX_ZEROES 4

static FILTER *PreFilters;
static TransferFnStruct transferFn;

/***********************************************************
 *              InitTransferFn()                           *
 *                                                         *
 * Initializes transfer function structure                 *
 * Call once at the start of processing                    *
 ***********************************************************/
int InitTransferFn()
{
    if ((transferFn.Poles = (Complex *)malloc(sizeof(Complex) * MAX_POLES)) == NULL)
    {
        logit("", "InitTransferFn(): Could not allocate transferFn.Poles \n");
        return EW_FAILURE;
    }

    if ((transferFn.Zeroes = (Complex *)malloc(sizeof(Complex) * MAX_ZEROES)) == NULL)
    {
        logit("", "InitTransferFn(): Could not allocate transferFn.Zeroes \n");
        return EW_FAILURE;
    }

    return EW_SUCCESS;
}

/***********************************************************
 *              FreeTransferFn()                           *
 *                                                         *
 * Frees transfer function poles and zeroes                *
 * Call once at the end of processing                      *
 ***********************************************************/
void FreeTransferFn()
{
    if (transferFn.Poles != (Complex *)NULL)
        free(transferFn.Poles);

    if (transferFn.Zeroes != (Complex *)NULL)
        free(transferFn.Zeroes);
}

/***********************************************************
 *              InitAllPreFilters()                        *
 *                                                         *
 * Initializes all shared pre-filter arrays                *
 * Call once at the start of processing                    *
 ***********************************************************/
int InitAllPreFilters(int maxPreFilters)
{
    int f;

    if ((PreFilters = (FILTER *)calloc(maxPreFilters, sizeof(FILTER))) ==  NULL)
    {
        logit ("e", "raypicker: InitAllPreFilters cannot allocate PreFilters; exiting.\n");
        return EW_FAILURE;
    }

    for (f = 0; f < maxPreFilters; f++)
        InitPreFilter(&PreFilters[f]);

    return EW_SUCCESS;
}

/***********************************************************
 *               InitPreFilter()                           *
 *                                                         *
 * Initializes a single shared pre-filter array            *
 ***********************************************************/
static int InitPreFilter(FILTER *filter)
{
    filter->sampleRate  = 0.0;
    filter->isBroadband = TRUE;
    filter->len         = 0;
    filter->passband.freq_min = 0;
    filter->passband.freq_max = 0;
    return 0;
}

/***********************************************************
 *                  SetupBroadbandFilter()                 *
 *                                                         *
 * Sets up a broadband filter                              *
 ***********************************************************/
static int SetupBroadbandFilter(const double sampleInterval, FILTER *filter,
									int debug)
{
    static const unsigned int nhp1 = 2;                        /* number of poles for high pass filter 1  */    
    static const unsigned int nhp2 = 2;                        /* number of poles for high pass filter 2  */ 
    static const unsigned int nlp1 = 2;                        /* number of poles for low pass filter 1   */
    static const unsigned int nlp2 = 2;                        /* number of poles for low pass filter 2   */ 
    static const double       hp1 = 0.5;                       /* cutoff freq for high pass filter 1      */
    static const double       hp2 = 1.05;                      /* cutoff freq for high pass filter 2      */
    static const double       lp1 = 2.65;                      /* cutoff freq for low pass filter 1       */
    static const double       lp2 = 6.5;                       /* cutoff freq for low pass filter 2       */
                        
                 double       hp1pi2 = hp1 * TWO_PI;           /* cutoff freq (radians) for hp1 */
                 double       hp2pi2 = hp2 * TWO_PI;           /* cutoff freq (radians) for hp2 */
                 double       lp1pi2 = lp1 * TWO_PI;           /* cutoff freq (radians) for lp1 */
                 double       lp2pi2 = lp2 * TWO_PI;           /* cutoff freq (radians) for lp2 */
                 int          np;                              /* number of poles               */
                 int          nz;                              /* number of zeroes              */

                 Complex      p[8];                            /* array of poles                */
                 Complex      z[4] = {{0.0, 0.0}, {0.0, 0.0}, 
                                      {0.0, 0.0}, {0.0, 0.0}}; /* array of zeroes               */
	 
                 double       a0, a1, a2;                      /* normalization constants       */
                 int          retc;                            /* return code                   */
                 int          pctr;                            /* counter for poles             */
                 int          zctr;                            /* counter for zeroes            */
                 int          j;


    if (filter == NULL)
	{
		logit("", "SetupBroadbandFilter; filter passed in NULL \n");
		return EW_FAILURE;
	}
			
	/* Make up a recursive filter for broadband data. */
    nz = nhp1 + nhp2;
    np = nz + nlp1 + nlp2;

    if (np > MAX_POLES || nz > MAX_ZEROES)
    {
        logit("", "Too many poles (%d > %d allowed) or zeroes (%d . %d allowed) \n",
                     np, MAX_POLES, nz, MAX_ZEROES);
        return EW_FAILURE;
    }

    /* make first high pass filter */   
    if ((retc = make_butterworth_filter(nhp1, &p[0], &a2, hp1pi2)) != 0)
    {
        /* 'bad call to make_butterworth_filter/HP1' */
        if (debug >= 2)
            logit("", "SetupBroadbandFilter(): make_butterworth_filter(%d %lf) returned error %d\n", 
                       nhp1, hp1pi2, retc);
		return EW_FAILURE;
    }
	
    /* make second high pass filter */   
    if ((retc = make_butterworth_filter(nhp2, &p[nhp1], &a2, hp2pi2)) != 0)
    {
        /* 'bad call to make_butterworth_filter/HP2' */
        if (debug >= 2)
            logit("", "SetupBroadbandFilter(): make_butterworth_filter(%d, %f) returned error %d\n", 
                      nhp2, hp2pi2 , retc);
		return EW_FAILURE;
    }
      
    /* make first low pass filter */
    if ((retc = make_butterworth_filter(nlp1, &p[nz], &a1, lp1pi2)) != 0)
    {
        /* 'bad call to make_butterworth_filter/LP1' */
        if (debug >= 2)
            logit("", "SetupBroadbandFilter(): make_butterworth_filter(%d, %f) returned error %d\n", 
                       nlp1, lp1pi2, retc);
		return EW_FAILURE;
    }
      
    /* make second low pass filter */
    if ((retc = make_butterworth_filter(nlp2, &p[nz+nlp1], &a0, lp2pi2)) != 0)
    {
        /* 'bad call to make_butteworth_filter/LP2' */
        if (debug >= 2)
            logit("", "SetupBroadbandFilter(): make_butterworth_filter(%d, %f) returned error %d\n", 
                      nlp2, lp2pi2, retc);
		return EW_FAILURE;
    }

    /* normalization constant depends only on low-pass filter constants */
    a1 = a1 * a0;

   /* fill in the transfer function structure */
    transferFn.numPoles = np;
    memcpy(transferFn.Poles, p, sizeof(Complex) * transferFn.numPoles);

    transferFn.numZeroes = nz;
    memcpy(transferFn.Zeroes, z, sizeof(Complex) * transferFn.numZeroes);

    transferFn.tfFreq = 1.0 / sampleInterval;
    transferFn.normConstant = a1;
   
    /* construct digital filter */
    if (debug >= 2)
    {
      logit("", "SetupBroadbandFilter(), digitalFilterCoeffs(); a1: %f, int: %f\n", a1, sampleInterval);
      logit("", "POLES:\n" );
      for (pctr = 0; pctr < np; pctr++)
          logit("", "Real(p[%d]) = %f , imag() = %f\n", pctr, Real(p[pctr]), Imag(p[pctr]));
		
          logit( "", "ZEROES:\n" );
          for (zctr = 0; zctr < nz; zctr++ )
              logit("", "Real(z[%d]) = %f , imag() = %f\n", zctr, Real(z[zctr]), Imag(z[zctr]));
    }

    if (digitalFilterCoeffs(transferFn, filter) != EW_SUCCESS)
    {
        logit("", "SetupBroadbandFilter(): digitalFilterCoeffs() returned error. \n");
		return EW_FAILURE;
    }
   
    if (debug >= 2)
    {
        logit("", "SetupBroadbandFilter(), digitalFilterCoeffs() results:\n" );
        for (j = 0; j < filter->len; j++)
            logit("", ".a[0][%d] %f  [1] %f\n.g[0]    %f  [1] %f\n", 
                      j, filter->a[j][0], filter->a[j][1], filter->g[j][0], filter->g[j][1]);
    }

    return EW_SUCCESS;
}

/***********************************************************
 *              SetupShortPeriodFilter()                   *
 *                                                         *
 * Sets up a short-period filter                           *
 ***********************************************************/
static int SetupShortPeriodFilter(const double sampleInterval, FILTER *filter,
                                  int debug)
{
    Complex           p[4];                            /* array of poles              */
    Complex           z[2] = {{0.0, 0.0}, {0.0, 0.0}}; /* array of zeroes             */
    double            a1, a2;                          /* normalization constants     */
    int               retc;                            /* return code                 */


    /* set up high pass filter */
    if ((retc = make_butterworth_filter(2, &p[0], &a2, M_PI)) != 0)
    {
        /* 'bad call to make_butterworth_filter/HP' */
        if (debug >= 2)
            logit("", "SetupShortPeriodFilter(): make_butterworth_filter(2, M_PI) returned error %d\n", retc);
		return EW_FAILURE;
    }

    /* set up low pass filter */
    if ((retc = make_butterworth_filter(2, &p[2], &a1, EIGHT_PI)) != 0)
    {
        /* stop 'bad call to make_butterworth_filter/LP' */
        if (debug >= 2)
            logit("", "SetupShortPeriodFilter(): make_butterworth_filter(2, EIGHT_PI) returned error %d\n", retc);
		return EW_FAILURE;
    }

    /* fill in the transfer function structure */
    transferFn.numPoles = 4;
    memcpy(transferFn.Poles, p, sizeof(Complex) * transferFn.numPoles);

    transferFn.numZeroes = 2;
    memcpy(transferFn.Zeroes, z, sizeof(Complex) * transferFn.numZeroes);

    transferFn.tfFreq = 1.0 / sampleInterval;
    transferFn.normConstant = a1;
 
    /* construct digital filter */
    if (digitalFilterCoeffs(transferFn, filter) != EW_SUCCESS)
    {
        if (debug >= 2)
            logit("", "SetupShortPeriodFilter(): digitalFilterCoeffs() returned error.\n");
		return EW_FAILURE;
    }
   
    return EW_SUCCESS;
}

/***********************************************************
 *                  InitChannelPreFilter()                 *
 *                                                         *
 * Initializes the pre-filter for a given sampling rate.   *
 *                                                         *
 * Call once, when the channel is first encountered.       *
 *                                                         *
 * @ return  0 = okay                                      *
 *          -1 = no matching common pre-filter             *
 *          -2 = failed initializing common pre-filter     *
 ***********************************************************/
int InitChannelPreFilter(const double sampleRate, const double mean, 
                         const int isBroadband, RECURSIVE_FILTER *channelData,
                         int maxPreFilters, int debug)
{
    /* double check_value = sampleRate * 0.05;    Dale's code */  
    double check_value = 0.05 / sampleRate;    /* Buland's ntrgr loop do .. 101 */      
    int    isNew    = TRUE; 
    int    filterIndex;
    int    f;                                  /* index into filter array */
    int    i, j;
    double sample_interval;                    /* sampling interval       */
   
    if (debug >= 2)
    {
        logit("", "InitChannelPreFilter(): rate %f , mean %f, is %s broadband\n",
                   sampleRate, mean, (isBroadband ? "" : "NOT"));
    }

    /* set up */
    filterIndex = maxPreFilters;
   
    /* See if we have a filter already set up for this rate. */
    for (f = 0; f < maxPreFilters; f++)
    {
        if (debug >= 2)
            logit("", "sampleRate %lf PreFilters[%d].sampleRate %lf check_value %lf isBroadband %d \n",
		              sampleRate, f, PreFilters[f].sampleRate, check_value, PreFilters[f].isBroadband);

        /* Buland's ntrgr loop do .. 101 */
        /* at any time filter matches, exit loop to use it */		      
        if (fabs((1.0 / sampleRate) - (1.0 / PreFilters[f].sampleRate)) <= check_value
           && PreFilters[f].isBroadband == isBroadband)    
        {
            filterIndex = f;
            isNew = FALSE;
         
            if (debug >= 2)
               logit("", "InitChannelPreFilter(): matching filter %d\n", filterIndex);

            break;
        }
    } /* checked all existing pre-filters for matching sampling rate */


    /* did not find a matching pre-filter, so check for an empty slot - 
     * sample interval 0.0 means this pre-filter slot not yet used. */
	if (filterIndex == maxPreFilters)
    {
        for (f = 0; f < maxPreFilters; f++)
        {
            if (PreFilters[f].sampleRate == 0.0)
            {
                filterIndex = f;
			    break;
            }
        }
    } /* checked for an empty slot */
   
    /* Unable to find a matching filter or an unused filter. Exit with an error condition.*/
    if (filterIndex == maxPreFilters)
    {
        logit("e", "InitChannelPreFilter(): Insufficient space for additional pre-filter\n");
        return -1;
    }
   
    /* No matching pre-filter found, Set up the new coefficients.*/
    if (isNew)
    { 
        if (debug >= 2)
            logit( "e", "InitChannelPreFilter(): pre-filter is new\n");
      
        PreFilters[f].sampleRate = sampleRate;
	    sample_interval = 1.0 / sampleRate;

        /* set high end of passband to Nyquist frequency to prevent aliasing */
        PreFilters[f].passband.freq_max = 0.5 * PreFilters[f].sampleRate;

	    if (debug >= 2)
            logit( "e", "InitChannelPreFilter(): PreFilters[f].sampleRate %lf, BB %d \n", 
		  	             PreFilters[f].sampleRate, isBroadband);
      
        if (isBroadband)
        {
            if (SetupBroadbandFilter(sample_interval, &PreFilters[filterIndex], debug) != EW_SUCCESS)
            {	
			    if (debug >=2)
			        logit("", "InitChannelPreFilter: SetupBroadbandFilter failed for filter %d \n", 
				        filterIndex);
                /* failed setup; this filter no longer any good */
                InitPreFilter(&PreFilters[f]);
                return -2;
            }
        }
        else
        {
            if (SetupShortPeriodFilter(sample_interval, &PreFilters[filterIndex], debug) != EW_SUCCESS)
            {
			    if (debug >= 2)
			        logit("", "InitChannelPreFilter: SetupShortPeriodFilter failed for filter %d \n", 
				               filterIndex);
                /* failed setup; his filter no longer any good */
                InitPreFilter(&PreFilters[filterIndex]);
                return -2;
            }
        }
    }
   
    /* Initialize this channel's pre-filter data (for the start of a new time series) */
    channelData->filter              = &PreFilters[filterIndex];
    channelData->filter->isBroadband =  isBroadband;
    channelData->mean                =  mean;
   
    if (debug >= 2)
        logit("", "=====>Pre-filter (%d): rate: %f  length: %d\n", filterIndex, 
		           sampleRate, (channelData->filter == NULL ? -1 : channelData->filter->len));
   
    if (isBroadband)
    {
        for (i = 0; i < channelData->filter->len; i++)
        {
            /* ntrger.fun, line 113; also nprfrc.fun, line 126 */
            for (j = 0; j < 2; j++)
                channelData->q[j][i] = 0.0;
        }
    }

    return 0;
}

/***********************************************************
 *                  PreFilterSamplePoint()                 *
 *                                                         *
 * Transforms one broadband or short-period sample value   *
 * into a pseudo short-period value suitable for similar   *
 * processing.                                             *
 *                                                         *
 * Bullland's ntrgr: between line nos 93 and 94            *
 ***********************************************************/
int PreFilterSamplePoint(const double  sampleValue, 
                          RECURSIVE_FILTER *prefilter, double *filteredValue)
{
    if (prefilter->filter->isBroadband)
    { 
        double xn = sampleValue - prefilter->mean;
       
        *filteredValue = filterTimeSeriesSample(xn, prefilter);
    }
    else
        *filteredValue = sampleValue - prefilter->mean;
   
    return RAYPICK_SUCCESS;
}

/***********************************************************
 *                  PreFilterCompensate()                  *
 *                                                         *
 * Compensates for [removes] the pre-filtering, for use in *
 * amplitude determinations.                               *
 *                                                         *
 * Bullland's ntrgr: do 116 i=1,3                          *
 ***********************************************************/
double PreFilterCompensate(const double filteredValue, 
						   const RECURSIVE_FILTER *prefilter)
{
   static const double   NEG_TWO_PI = M_PI * -2.0;
   static       Complex  hd, zi;
                Complex  numerator  = COMPLEX(0.0, 0.0);
                Complex  denom      = COMPLEX(0.0, 0.0);
   static       int      j;                                     /* loop counter */
                int      flen       = prefilter->filter->len;   /* filter length */
                FILTER  *FINFO      = prefilter->filter;
            
                
   zi = Cexp(COMPLEX(0.0, NEG_TWO_PI / filteredValue));
   hd = COMPLEX(0.0, 0.0);
  
   for (j = 0; j < flen; j++)
   {
      /* FORTRAN LINE 117, 119 */
      /* hd += ((FINFO->g[j][0] + (zi * FINFO->g[j][1]))
		      / (1.0 - (zi * (FINFO->a[j][0] + (zi * FINFO->a[j][1]))))); */
	  numerator = rcadd(FINFO->g[j][0], rcmult(FINFO->g[j][1], zi));
	  denom = rcadd(FINFO->a[j][0], rcmult(FINFO->a[j][1], zi));
	  denom = Cmult(denom, zi);
	  denom = rcsub(1.0, denom);
	  hd = Cadd(hd, Cdiv(numerator, denom));
   }
   
   /* FORTRAN 117 + 2:	acor(i)=1./cabs(hd) */
   return (1.0 / Cabs(hd));
}
