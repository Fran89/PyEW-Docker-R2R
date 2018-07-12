
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: setdecstg.c 6325 2015-05-01 00:44:09Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2002/10/25 17:59:44  dietz
 *     fixed spelling mistakes
 *
 *     Revision 1.1  2000/02/14 16:56:25  lucky
 *     Initial revision
 *
 *
 */

/*
 * setdecstg.c: Set up the decimation/filter stages
 *              1) Parse the Decimation Rate string
 *              2) Set up the filters for each stage
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: SetDecStages                                           */
/*                                                                      */
/*      Inputs:         Pointer to the Network structure                */
/*                      Pointer to DecRateStr character string          */
/*                                                                      */
/*      Outputs:        Filled in FILTER array                          */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in decimate.h on          */
/*                      failure                                         */
/*                                                                      */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <string.h>     /* strspn                                       */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit                                        */

/*******                                                        *********/
/*      Decimate Includes                                            */
/*******                                                        *********/
#include "decimate.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: SetDecStages                                          */
int SetDecStages( WORLD* pDcm, char *DecRateStr )
{
  int nStage = 0;     /* stage counter */
  char digits[] = "0123456789";   /* all the decimal digits             */
  char white[] = " \t";           /* white space characters             */
  char *nextDec;                  /* pointer to next decimation rate    */
  size_t n;
  int stage, i;
  int dec;
  int order;
  int   length;                   /* Number of coefficients in stage filter */ 
  double freqSample = 1.0;        /* input relative sample rate of a stage  */
  double freqPass;                /* pass-band upper freq limit             */
  double freqStop;                /* stop-band lower freq limit             */
  double freq_edges[4];           /* Array of edges of frequency bands      */
  double levels[4];               /* One level for each band edge           */
  double devs[2];                 /* Allowed ripple for each band           */
  double weights[2];              /* Weights for each band                  */
  double max_dev;
  double finalNyquist;            /* Decimate's final Nyquist frequency     */ 
  double *coeffs;                 /* Pointer to coefficients                */
  double *wr, *wi;                /* pointers for complex zeroes            */
  
  /* Scan DecRateStr for integer strings (decimation stage rates)           */
  nextDec = DecRateStr;
  while ( *nextDec != '\0' )
  {
    /* Check for white space */
    if ((n = strspn(nextDec, white)) != 0)
    {
      nextDec += n;
      continue;
    }

    /* Check for non-digits */
    if ((n = strcspn(nextDec, digits)) != 0)
    {
      logit("e", "decimate: Error parsing DecimationRates <%s>; exiting\n",
              nextDec);
      return EW_FAILURE;
    }
    
    /* Read the next integer */
    if ((n = strspn(nextDec, digits)) != 0)
    {
      pDcm->filter[nStage].decRate = atoi(nextDec);
      nextDec += n;
      if (pDcm->filter[nStage].decRate <= 0)
      {
        logit("e", "decimate: zero or negative decimate rate <%d>; exiting\n",
              pDcm->filter[nStage].decRate);
        return EW_FAILURE;
      }

      nStage++;
      if (nStage > MAXSTAGE)
      {
        logit("e", "decimate: too many decimation stages; max is %d; exiting\n",
              MAXSTAGE);
        return EW_FAILURE;
      }
      
      continue;
    }
  }
  if (nStage < 1)
  {
    logit("e", "decimate: no decimation rates found by SetDecStages; exiting\n");
    return EW_FAILURE;
  }
  pDcm->nStage = nStage;

  /*
   * Set up the filters for each stage of decimation.
   * Ref: Digital Signal Processing, Proakis and Manolakis (1996)
   */

  /* Set the upper limit of pass-band frequency to little less than the   *
   * final stage Nyquist frequency (1/2 the final sample rate).           */
  dec = 1;
  for (stage = 0; stage < nStage; stage++)
    dec *= pDcm->filter[stage].decRate;
  finalNyquist = 0.5 * freqSample / (double) dec;

  logit("","\nDecimation Filter description\n");
  logit("", "\tOverall decimation rate: %d in %d stages.\n", dec, nStage);
  
  freqPass = PASS_ALLOW * finalNyquist;
  
  freq_edges[0] = 0.0;
  levels[0] = 1.0;
  levels[1] = 1.0;
  levels[2] = 0.0;
  levels[3] = 0.0;
  freq_edges[3] = 0.5;
  
  /* Set the allowed ripple for a single stage */
  devs[0] = PASS_RIPPLE / nStage;
  devs[1] = STOP_RIPPLE / nStage;

  /* Set the highest weight for the smallest deviation */
  max_dev = ( devs[0] < devs[1] ) ? devs[0] : devs[1];
  for (i = 0; i < 2; i++)
    weights[i] = max_dev / devs[i];
  
  /* Set up each stage                                                    */
  for (stage = 0; stage < nStage; stage++)
  {
    logit("","\nStage %d: decimation rate: %d\n", stage + 1,
          pDcm->filter[stage].decRate );
    if (pDcm->filter[stage].decRate < 2)
    {
      logit("e", "decimate: decimation rate value must be > 1; exiting\n");
      return EW_FAILURE;
    }

    /* Stop-band frequency is stage samplerate - final Nyquist;
       This provides minimum aliasing while minimizing filter order       */
    freqStop = freqSample / (double) pDcm->filter[stage].decRate - 
      finalNyquist;
    if (pDcm->dcmParam.debug)
      logit("", "Remez frequencies: pass: %e, stop: %e, sample: %e\n",
            freqPass, freqStop, freqSample );

    /* Frequencies in remezlp must be referenced to the sample frequency */
    freq_edges[1] = freqPass / freqSample;
    freq_edges[2] = freqStop / freqSample;
    if (remeznp(2, freq_edges, levels, devs, weights, &length, &coeffs) 
        != EW_SUCCESS )
    {
      logit("e", "decimate: call to remeznp failed; exiting\n");
      return EW_FAILURE;
    }
    
    pDcm->filter[stage].Length = length;
    pDcm->filter[stage].coef = coeffs;
    order = length - 1;

    if ( Zeroes(coeffs, &wr, &wi, order) != EW_SUCCESS )
    {
      logit("e", "decimate: failed to calulate filter zeroes; exiting\n");
      return EW_FAILURE;
    }
    
    /* Log some information about this stage */
    if (stage == 0)
      logit("", "Time lag induced by this filter: %.1f times the data sample period.\n",
            order / 2.0);
    else
      logit("", "Time lag induced by this filter: %.1f times the input sample period.\n",
            pDcm->filter[stage - 1].decRate * order / 2.0);
      
#ifdef FIXTIMELAG
    logit("", "This time lag will be removed by adjusting timestamps of the data.\n\n");
#endif  

    logit("", "Filter coefficients:\n");
    for (i = 0; i < (length+1)/2;i++)
      logit("", "\t%4d % 10.8e\n", i+1, coeffs[i]);
    for (i = length/2 - 1; i > -1; i--)
      logit("", "\t%4d % 10.8e\n", length - i, coeffs[i]);
  
    logit("","\nZeroes for FIR filter of order %ld:\n", order);
    for (i = 0; i < order; i++)
      logit("", "\t%4d % 10.8e % 10.8e\n", i+1, wr[i], wi[i]);
    logit("", "\n");

    free( wr );
    free( wi );

    /* Set the output frequency for this stage (input of next stage) */
    freqSample /= (double) pDcm->filter[stage].decRate;
    
  }
  
  return EW_SUCCESS;
}
