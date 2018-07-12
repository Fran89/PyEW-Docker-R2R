
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: setfilt.c 7 2000-02-14 17:27:23Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/*
 * setfilt.c: Set up the FIR filter 
 *              1) Validate the BAND structures
 *              2) Calculate FIR filter coefficients
 *              3) Calculate and log filter zeroes
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: SetFilter                                             */
/*                                                                      */
/*      Inputs:         Pointer to the Fir WORLD structure              */
/*                                                                      */
/*      Outputs:        Filled in FILTER array                          */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in fir.h on               */
/*                      failure                                         */
/*                                                                      */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit                                        */

/*******                                                        *********/
/*      Fir Includes                                            */
/*******                                                        *********/
#include "fir.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: SetFilter                                             */
int SetFilter( WORLD* pFir )
{
  int nband, i;
  int order;
  int length;                     /* Number of coefficients in filter   */ 
  PBAND this, next;
  double max_dev;
  double eps = MIN_TRANS_BAND;     /* Smallest allowable transition band */
  double freq_edges[MAX_BANDS * 2]; /* Array of edges of frequency bands */
  double levels[MAX_BANDS * 2];    /* One level for each band edge       */
  double devs[MAX_BANDS];          /* Allowed ripple for each band       */
  double weights[MAX_BANDS];       /* Weights for each band              */  
  double *coeffs;                  /* Pointer to coefficients            */
  double *wr, *wi;                 /* pointers for complex zeroes        */
  
  /* Validate the BAND structures and fill in band arrays */
  this = pFir->pBand;     /* Otherwise ReadConfig would complain */
  nband = 0;
  if (this->next == (PBAND) NULL)
  {
    logit("e", "fir: only one Band found; need at least two\n");
    return EW_FAILURE;
  }
  if (this->f_low != 0.0)
  {
    logit("e", "fir: first band starts at %lf; must be 0.0\n", this->f_low);
    return EW_FAILURE;
  }
  max_dev = this->dev;
  while ( (next = this->next) != (PBAND) NULL)
  {
    if (next->f_low - this->f_high < eps)
    {
      logit("e", "fir: transition between bands %d - %d is %lf; minimum is %lf\n",
            nband, nband + 1, next->f_low - this->f_high, eps);
      return EW_FAILURE;
    }
    if (this->level == next->level)
    {
      logit("e", "fir: adjacent bands have identical levels; exitting\n");
      return EW_FAILURE;
    }
          
    /* Remeznp wants frequencies normalized to sample rate */
    freq_edges[2 * nband] = this->f_low * 0.5;
    freq_edges[2 * nband + 1] = this->f_high * 0.5;
    levels[2 * nband + 1] = levels[2 * nband] = (double) this->level;
    devs[nband] = this->dev;
    if (next->dev > max_dev) max_dev = next->dev;
    nband++;
    if (nband + 1 > MAX_BANDS)
    {
      logit("e", "fir: too many Bands; max is %d\n", MAX_BANDS);
      return EW_FAILURE;
    }
    this = next;
  }
  /* Now this points to the last BAND structure */
  if (this->f_high != 1.0)
  {
    logit("e", "fir: last band ends at %lf; must be 1.0\n", this->f_high);
    return EW_FAILURE;
  }
  freq_edges[2 * nband] = this->f_low * 0.5;
  freq_edges[2 * nband + 1] = this->f_high * 0.5;
  levels[2 * nband + 1] = levels[2 * nband] = (double) this->level;
  devs[nband] = this->dev;
  nband++;

  /* Set the highest weight for the smallest deviation */
  for (i = 0; i < nband; i++)
    weights[i] = max_dev / devs[i];
  
  if ( remeznp( nband, freq_edges, levels, devs, weights, &length, &coeffs,
               pFir->firParam.testMode )
      != EW_SUCCESS )
  {
    logit("e", "fir: call to remeznp failed; exitting\n");
    return EW_FAILURE;
  }
    
  pFir->filter.Length = length;
  pFir->filter.coef = coeffs;
  order = length - 1;

  if ( Zeroes(coeffs, &wr, &wi, order) != EW_SUCCESS )
  {
    logit("e", "fir: failed to calulate filter zeroes; exitting\n");
    return EW_FAILURE;
  }

  /* Log some information about this filter */
  logit("","\nFir Filter description\n");
  logit("", "Time lag induced by this filter: %.1f times the data sample period.\n",
        order / 2.0);
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
  free( wr );
  free( wi );

  return EW_SUCCESS;
}
