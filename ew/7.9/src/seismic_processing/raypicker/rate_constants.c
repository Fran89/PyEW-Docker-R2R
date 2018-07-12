/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rate_constants.c 4475 2011-08-04 15:17:09Z kevin $
 *
 *    Revision history:
 *     $Log: rate_constants.c,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:50  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.5  2005/06/20 21:31:15  cjbryan
 *     cleanup error reporting and remove unnecessary includes
 *
 *     Revision 1.4  2004/11/01 02:03:27  cjbryan
 *     removed all amplitude determination code from the picker and changed all error
 *     codes to positive numbers
 *     CVS ----------------------------------------------------------------------
 *
 *     Revision 1.3  2004/07/20 20:06:43  cjbryan
 *     reorganized includes
 *
 *     Revision 1.2  2004/06/10 20:22:35  cjbryan
 *     re-engineered array initialization
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 */
/*
 * Implementations of functions used to manage and
 * maintain values which are constant to specific sample rates.
 * This allows the constants to be shared and reused among
 * all channels that have the same nominal rate.
 * 
 * Many variable names are from the original FORTRAN.
 * 
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 : August 2003, dbh 
 */

/* standard C includes */
#include <math.h>
#include <stdlib.h>

/* earthworm includes */
#include <earthworm.h>
#include <watchdog_client.h>

/* raypicker includes */
#include "ray_trigger.h"
#include "raypicker.h"
#include "rate_constants.h"
#include "returncodes.h"

#define RateConstantIncrement  10 /* number of RateConstants structures to
                                   * allocate each time more are needed */

/* Throughout processing, various values [such as triggering
 * levels] are tied to others in a manner that causes the derived
 * value to lag behind the source value by some approximate
 * number of seconds.
 * Ifthe source value has been constant for some time, the derived value 
 * is the same as the original. If the source value changes to a new
 * level -- and remains constant at that new level -- then
 * the derived value should trend towards the new level and
 * reach the new level in approximately some given number of
 * seconds.
 * 
 * These LAG_xxx values manage the available lag rates. */

/* Indices into LAG_CONST[], which defines the available lag rates. */
#define  LAG_CONST_D1   0
#define  LAG_CONST_D2   1
#define  LAG_CONST_D10  2
#define  LAG_CONST_D30  3

#define  LAG_CONST_LEN  4

/* inverse of Buland's fcon */
static const double LAG_CONST[LAG_CONST_LEN] = {1.0, 2.0, 10.0, 30.0};


/* variables for structure memory management */
static int                   RateConstantsUsed  = 0;
static RATE_CONSTANTS_DATA  *RateConstants = NULL;


/********************************************************
 *            InitRateConstants()                       *
 *                                                      *
 * Initializes rate constant memory pointers to prevent *
 * invalid deallocations when FreeRateConstants() is    *
 * called.                                              *
 *                                                      *
 * Call once at the start of program execution.         *
 *******************************************************/
int InitRateConstants(int maxRateConstants)
{
    RateConstantsUsed  = 0;
    RateConstants      = NULL;

    if ((RateConstants = (RATE_CONSTANTS_DATA *)calloc(maxRateConstants, 
                            sizeof(RATE_CONSTANTS_DATA))) == NULL)
    {
      reportError(WD_FATAL_ERROR, MEMALLOC, "raypicker: Call to calloc rateConstants failed; exiting!\n");
	  return EW_FAILURE;
    } 

   return EW_SUCCESS;
}

/********************************************************
 *            FreeRateConstants()                       *
 *                                                      *
 * Frees rate constant memory allocations made by       *
 * GetRateConstants()                                   *
 *                                                      *
 * Call once at the end of program execution.           *
 *******************************************************/
void FreeRateConstants()
{
    if (RateConstants != NULL)
      free(RateConstants);
}

/********************************************************
 *             GetRateConstants()                       *
 *                                                      *
 * Used to obtain a pointer to a RateConstants struct   *
 * for a given sample rate.                             *
 *                                                      *
 * WARNING: Do not delete or free the pointer returned  *
 * from this function. The memory will be cleaned up by *
 * FreeRateConstants() [at the end of program execution]*
 *                                                      *
 * @param p_constants pointer to receive the pointer to *
 * the appropriate constants struct (NULL on error).    *
 *******************************************************/
int GetRateConstants(const double sampleRate, 
                     RATE_CONSTANTS_DATA **p_constants, 
                     int maxRateConstants, int debug)
{    
    RATE_CONSTANTS_DATA *workRC;
    int                  c;                  /* loop counter                  */
    double               check_value = 0.1;  /* tolerance in sample rate (Hz) */

    if (RateConstants == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "RateConstants is NULL Setup incomplete. \n");
      return EW_FAILURE;
    }

    if (p_constants == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "p_constants passed in NULL to GetRateConstants. \n");
      return EW_FAILURE;
    }
    
    /* set p_constants to point to NULL */
    (*p_constants) = NULL;
   
    if (RateConstants != NULL)
    {
      /* Look for filter constants with matching sample rate */
      for (c = 0; c < maxRateConstants; c++)
      {
          if (fabs(RateConstants[c].sample_rate - sampleRate) <= check_value)
          {
              /* these constants match the desired characteristics */
              (*p_constants) = &RateConstants[c];
              break;
          }  
      }
    }

    /* no matching filter constants struct found, attempt to create new one */
    if ((*p_constants) == NULL)
    { 

      /* no usable space remains */
      if (RateConstantsUsed == maxRateConstants) 
      {
          reportError(WD_WARNING_ERROR, NORESULT, "raypicker: GetRateConstants(): Not enough rateConstants Structs. Returning!\n");
          return EW_WARNING;
      }
      
      workRC = RateConstants + RateConstantsUsed;
      
      /* Set up constants for the new rate */
      workRC->sample_rate = sampleRate;
      workRC->sample_interval = 1.0 / sampleRate;
      
      /* Buland's fset(i) = 1.0 - exp(-sint0 * fcon(i)) where sintt0 = sampleInterval 
		 for  d1, d2, d10, d30; FORTRAN line # 91*/
      workRC->lag1sec  = 1.0 - exp(-workRC->sample_interval / LAG_CONST[LAG_CONST_D1]);
      workRC->lag2sec  = 1.0 - exp(-workRC->sample_interval / LAG_CONST[LAG_CONST_D2]);
      workRC->lag10sec = 1.0 - exp(-workRC->sample_interval / LAG_CONST[LAG_CONST_D10]);
      workRC->lag30sec = 1.0 - exp(-workRC->sample_interval / LAG_CONST[LAG_CONST_D30]);
      
      workRC->iirrmp = INIT_TAPER_LEN * sampleRate + 0.5;

      /* part of Buland's nset(i) = tcon(i) * sampleRate + 0.5 */
      workRC->navest = (long)(AVEST * sampleRate + 0.5);
      workRC->navetm = (long)(AVETM * sampleRate + 0.5);
      workRC->aveln  = 1.0 / (double)(workRC->navetm - workRC->navest + 1);
      
      workRC->nsettm = (int)(SETTM * sampleRate + 0.5);
      workRC->nlnktm = (int)(TMLNK * sampleRate + 0.5);
      workRC->nsmotm = (int)(SMOTM * sampleRate + 0.5);
      workRC->maxph  = PHMAX * sampleRate + 0.5;

      workRC->avemn  = SNMIN * sampleRate;
      
      if (debug >= 2) 
      {
          /* Output the constants for debugging */
          logit( "", "\nInitialized a new set of constants for sample rate %f pts/sec:\n", 
                         sampleRate);
          logit("", "interval: %f sec\n", workRC->sample_interval);
          logit("", "d1: %f\nd2: %f\nd10: %f\nd30: %lf\n", 
                         workRC->lag1sec, workRC->lag2sec, workRC->lag10sec, workRC->lag30sec);
          logit("", "Initial tapering time: %f sample points\n", workRC->iirrmp);
          logit("", "Pre-filter averaging from: %.1f to %.1f (%.1f) secs or %d to %d sample points (%.4f/pt)\n", 
                         AVEST, AVETM, AVETM - AVEST, workRC->navest, workRC->navetm, workRC->aveln);
          logit("", "Signal initialization time (no trigger before): %f secs or %d sample points\n", 
                         SETTM, workRC->nsettm);
          logit("", "Max asymmetric signal duration before rejection: %f secs or %d sample points\n",
                         TMLNK, workRC->nlnktm);
          logit("", "Min s/n level: %f\n", workRC->avemn);
          logit("", "Max phase widths for backing down to arrival time: %f (sample points)\n\n", workRC->maxph);

      }
      
      (*p_constants) = &RateConstants[RateConstantsUsed];
   
      RateConstantsUsed++;
    }
   
    return EW_SUCCESS;
}
