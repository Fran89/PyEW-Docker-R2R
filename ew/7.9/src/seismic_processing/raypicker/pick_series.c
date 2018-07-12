/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pick_series.c 4475 2011-08-04 15:17:09Z kevin $
 *
 *    Revision history:
 *     $Log: pick_series.c,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:49  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.13  2005/06/20 21:33:28  cjbryan
 *     cleanup
 *
 *     Revision 1.12  2004/11/01 02:03:27  cjbryan
 *     removed all amplitude determination code from the picker and changed all error
 *     codes to positive numbers
 *     CVS ----------------------------------------------------------------------
 *
 *     Revision 1.11  2004/10/27 14:33:44  cjbryan
 *     revisions to period determination for amp calculations and collection of
 *     amp info between calculated arrival time and time trigger is declared
 *
 *     Revision 1.10  2004/09/20 21:09:59  labcvs
 *     changed initial polarity from ' ' to '?'
 *
 *     Revision 1.9  2004/09/15 16:08:02  cjbryan
 *     *** empty log message ***
 *
 *     Revision 1.7  2004/07/20 20:10:04  cjbryan
 *     added counter for max lifetime of an arrival time
 *
 *     Revision 1.6  2004/07/16 19:27:22  cjbryan
 *     allowed continuation of a preliminary trigger
 *
 *     Revision 1.5  2004/06/10 20:22:35  cjbryan
 *     re-engineered array initialization
 *
 *     Revision 1.3  2004/04/23 17:30:36  cjbryan
 *     changed bool to int
 *
 *     Revision 1.2  2004/04/22 21:11:56  patton
 *     Quick fixes to make raypicker compile.
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 */
/*
 * Functions used to implement a single pick series processor for the raypicker.
 * 
 * @author Ray Buland, FORTRAN, circa 1979
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 August 2003, dbh C++ port from the original FORTRAN
 */

/* system includes */
#include <stdlib.h>
#include <math.h>

/* earthworm/hydra includes */ 
#include <earthworm.h>
#include <watchdog_client.h>

/* raypicker includes */
#include "config.h"
#include "debug.h"
#include "pick_series.h"
#include "rate_constants.h"
#include "ray_trigger.h" /* SNMIN */
#include "returncodes.h"
#include "macros.h"
#include "channel_states.h"


/************************************************************************
 *                      InitPickSeriesData()                            *
 *                                                                      *
 * Initializes a PICK_SERIES_DATA struct for processing, which includes *
 * allocating the STA buffer.                                           *
 *                                                                      *
 * Must be followed by a call to FreePickSeries() to release the        *
 * allocated memory.                                                    *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *         EW_FAILURE                                                   *
 *                                                                      *
 ************************************************************************/
int InitPickSeriesData(const double maxSampleRate, PICK_SERIES_DATA *staSeries)
{
    if (staSeries == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "Parameter passed in NULL. \n");
      return EW_FAILURE;
    }
   
    ClearPickFilterTrigger(staSeries);
   
    staSeries->highCorner = 0.0;
    staSeries->lowCorner  = 0.0;
   
    staSeries->sta_buffer  = NULL;
    staSeries->sta_write_point  = 0;
   
    /* calculate length of short-term average (STA) buffers (Buland's ntblm =
     * # of samples in pre-trigger STA  = traceSampleRate * STA_BUFF_SECS + 0.5) */
    staSeries->sta_length = (long)(maxSampleRate * STA_BUFF_SECS + 0.5);
     
    /* Allocate the STA buffer */
    if ((staSeries->sta_buffer = (double *) malloc(staSeries->sta_length * sizeof(double))) == NULL)
    {
      staSeries->sta_length = 0;
	  reportError(WD_FATAL_ERROR, MEMALLOC, "Could not allocate staSeries buffer. \n");
      return EW_FAILURE;
    }
   
    staSeries->state = PKCH_STATE_LOOK;
   
    return EW_SUCCESS;
}

/************************************************************************
 *                        FreePickSeries()                              *
 *                                                                      *
 * Frees the memory allocation in a PICK_SERIES_DATA struct.            *
 *                                                                      *
 ************************************************************************/
void FreePickSeries(PICK_SERIES_DATA *staSeries)
{
    if (staSeries == NULL)
      return;
   
    if (staSeries->sta_buffer != NULL)
    {
      free(staSeries->sta_buffer);
      staSeries->sta_buffer = NULL;
    }
}

/************************************************************************
 *                       SetPickFilterParams()                          *
 *                                                                      *
 * Used to set pick series parameters, based upon the actual channel    *
 * sample rate.                                                         *
 *                                                                      *
 * @param  sampleRate nominal sample rate in values per second          *
 * @param  staSeriesId identification of the type of this picker,       *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *         EW_FAILURE - parameter passed in NULL                        *
 *                                                                      *
 ************************************************************************/
int SetPickFilterParams(const double sampleRate, const int staSeriesId, 
                        PICK_SERIES_DATA *staSeries, 
                        const double PICKER_PARAM_DATA[][PICKER_VALUECOUNT])
{
    if (staSeries == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "staSeries passed in NULL. \n");
      return EW_FAILURE;
    }
   
    /* Buland's fcn11, fcn21, fcn21, fcn22; FORTRAN line # 91 */
    staSeries->highCorner = 1.0 - exp((-1.0/sampleRate) * PICKER_PARAM_DATA[staSeriesId][PICKER_CORNER_HI]);
    staSeries->lowCorner  = 1.0 - exp((-1.0/sampleRate) * PICKER_PARAM_DATA[staSeriesId][PICKER_CORNER_LO]);

    /* calculate filter-specific confirm duration */
    staSeries->confirm_lo = sampleRate * PICKER_PARAM_DATA[staSeriesId][PICKER_DURATIONLO] + 0.5;
    staSeries->confirm_hi = sampleRate * PICKER_PARAM_DATA[staSeriesId][PICKER_DURATIONHI] + 0.5;
   
    return EW_SUCCESS;
}

/************************************************************************
 *                    InitPickSeriesLevels()                            *
 *                                                                      *
 * Sets the initially estimated LTA, mid-level triggering and           *
 * fundamental triggering values for use in the initialization          *
 * processing.                                                          *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *         EW_FAILURE - parameter passed in NULL                        *
 *                                                                      *
 ************************************************************************/
int InitPickSeriesLevels(const double lta, const double midlevel, 
                         const double triggervalue, PICK_SERIES_DATA *staSeries)
{
    if (staSeries == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "staSeries passed in NULL to InitPickSeriesLevels. \n");
      return EW_FAILURE;
    }

    staSeries->lta           = lta;
    staSeries->mid_trigger   = midlevel;
    staSeries->trigger_value = triggervalue;

    return EW_SUCCESS;
}

/************************************************************************
 *                   InitPickFilterForSeries()                          *
 *                                                                      *
 * Clears the filter variables in preparation for a new series.         *
 * Generally used at initialization or after a significant gap.         *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *         EW_FAILURE - parameter passed in NULL                        *
 *                                                                      *
 ************************************************************************/
int InitPickFilterForSeries(PICK_SERIES_DATA *staSeries)
{
    if (staSeries == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "staSeries passed in NULL to InitPickFilterForSeries. \n");
      return EW_FAILURE;
    }
   
    staSeries->fmem1 = 0.0;
    staSeries->fmem2 = 0.0;
    staSeries->fmem3 = 0.0;
    staSeries->fmem4 = 0.0;
   
    staSeries->value_limit   = 0.0;
    staSeries->last_value    = 0.0;
    staSeries->last_rect     = 0.0;
    staSeries->last_envelope = 0.0;

    return EW_SUCCESS;
}

/***********************************************************************
 *                     BandpassSample()                                *
 *                                                                     *
 * Process the next sample value through the IIR filter                *
 * to arrive at this channel's next STA value.                         *
 *                                                                     *
 * This function accumulates various persistence values in the         *
 * associated PICK_SERIES_DATA struct.                                 *
 *                                                                     *
 * In practice, the STA values which will be used are stored in the    *
 * pick channel global variable g_Filtered[] until actual trigger      *
 * processing is performed. (That is, the sample buffer is converted   *
 * to STA in advance thereof).                                         *
 *                                                                     *
 * @param sampleValue the next pseudo short-period value in the        *
 *                  channel's time series.                             * 
 * @param minSTA lower boundary to prevent the STA from going to zero. *
 *                  (SNMIN * LTA)                                      *
 * @param rate_constants pointer to the relevant RATE_CONSTANTS_DATA   *
 *               struct for the channel's sample rate.                 *
 * @param staSeries the PICK_SERIES_DATA for the picker process       .*
 * @param value pointer to variable to receive the calculated value.   *
 *                                                                     *
 * @return EW_SUCCESS                                                  *
 *         EW_FAILURE - parameter passed in NULL                       *
 *                                                                     *
 ***********************************************************************/
int BandpassSample(const double sampleValue, const double minSTA, 
                   const RATE_CONSTANTS_DATA *rate_constants, 
                   PICK_SERIES_DATA *staSeries, double *value)
{
    if (rate_constants == NULL || value == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "parameter passed in NULL to Bandpass. \n");
      return EW_FAILURE;
    }
   
    /* Bandpass the signal */
    staSeries->fmem1 += (sampleValue      - staSeries->fmem1) * staSeries->highCorner;
    staSeries->fmem2 += (staSeries->fmem1 - staSeries->fmem2) * staSeries->highCorner;
    staSeries->fmem3 += (staSeries->fmem2 - staSeries->fmem3) * staSeries->lowCorner;
    staSeries->fmem4 += (staSeries->fmem3 - staSeries->fmem4) * staSeries->lowCorner;
   
    /* Get the rectified value for triggering 
     * s1(kl) = abs(fmem21-fmem41) */
    staSeries->last_rect = fabs(staSeries->fmem2 - staSeries->fmem4);

    /* Calculate the filtered value 
     * btm1 = amax1(btm1 + (s1(kl) - btm1) * d1, clt1) */
    staSeries->last_value += (staSeries->last_rect - staSeries->last_value) * rate_constants->lag1sec;

    if (staSeries->last_value < minSTA)
      staSeries->last_value = minSTA;
   
    *value = staSeries->last_value;

    return EW_SUCCESS;
}

/************************************************************************
 *                         NewSTAValue()                                *
 *                                                                      *
 * Copies the appropriate STA value into the PICK_SERIES_DATA struct's  *
 * STA persistence buffer just prior to trigger processing.             *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *         EW_FAILURE - parameter passed in NULL                        *
 *                                                                      *
 ************************************************************************/
int NewSTAValue(const double currentTime, const double newValue, 
                PICK_SERIES_DATA  *staSeries)
{
    if (staSeries == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "staSeries passed in NULL to NewSTAValue. \n");
      return EW_FAILURE;
    }

    /* Copy the new STA time into the persistence buffer */
    staSeries->sta_time = currentTime;
    staSeries->sta_buffer[staSeries->sta_write_point] = newValue;
       
    /* update next write point to wrap buffer if needed */
    if ((++staSeries->sta_write_point) == staSeries->sta_length)
      staSeries->sta_write_point = 0;
   
    return EW_SUCCESS;
}

/************************************************************************
 *                         UpdateLevels()                               *
 *                                                                      *
 * Update the LTA, the mid-level trigger, the base triggering level,    *
 * STA upper bound, and confirm factor                                  *
 *                                                                      *
 * @param currentValue the next STA value                               *
 * @param rateConstants pointer to the relevant RATE_CONSTANTS_DATA     *
 *         struct for the channel's sample rate (see rate_constants.h). *
 * @param staSeriesindex identification of the type of this picker,     *
 *         meaning the index into PICKER_PARAM_DATA[.][],               *
 * @param staSeries the PICK_SERIES_DATA struct for the series          *
 *                                                                      *
 * @return EW_SUCCESS                                                  *
 *         EW_FAILURE - parameter passed in NULL                       *
 *                                                                      *
 ************************************************************************/
int UpdateLevels(const double currentValue, const RATE_CONSTANTS_DATA *rateConstants, 
                 PICK_SERIES_DATA *staSeries)
{
    if (rateConstants == NULL || staSeries == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "parameter passed in NULL to UpdateLevels. \n");
      return EW_FAILURE;
    }
   
    /* Update the long-term average for the picker until a preliminary
     * trigger is identified. Updating will continue after the trigger is confirmed.
     * 
     * This helps to exclude spikes and other non-trigger diversions from
     * calculations of the LTA.
     * 
     * xlt1=amax1(xlt1+(bt1(ntb)-xlt1)*d30,snmin) (just before FORTRAN 23 and 45) */
    if (staSeries->state != PKCH_STATE_PRELIM && staSeries->state != PKCH_STATE_TRIG)
    {
      staSeries->lta += (currentValue - staSeries->lta) * rateConstants->lag30sec;
      staSeries->lta = MAX(staSeries->lta, SNMIN);
    }

    /* Update the mid-level until a trigger is confirmed
     * FORTRAN LINE 23, 45: xmt2=trgvl2*xlt2 */         
    if (staSeries->state < PKCH_STATE_CONF)
    {
      staSeries->mid_trigger = staSeries->trigger_value * staSeries->lta;

      /* also update value_abs_limit (Buland's clt3) 
       * (made parameter adaptive on Sept 10, 2004 to try to catch missed triggers) */
     staSeries->value_abs_limit = 2.0 * staSeries->trigger_value * staSeries->lta;
    }

    return EW_SUCCESS;
}

/************************************************************************
 *                          NewTrigger()                                *
 *                                                                      *
 * Resets the staSeries.max_value to prepare for a new confirmation     *
 * duration calculation [in GetConfirmDuration()] and determines the    *
 * arrival time, if needed.                                             *
 *                                                                      *
 * Called when the trigger first leaves the preliminary state.          *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *                                                                      *
 ************************************************************************/
int NewTrigger(const RATE_CONSTANTS_DATA *rateConstants, 
			   PICK_SERIES_DATA *staSeries, int debug)
{
    /* dbh: believe that these were determined empirically in the
     *      early days of the FORTRAN picker
     */
    static const double FACTOR1 = 0.2;
    static const double FACTOR2 = 0.05;
                 long   c_sta_length;         /* length of the filter's sta buffer       */    
                 long   c_sta_end;            /* latest (last) STA value                 */
                 int    a1;                   /* current sta work location - 3 (k1)
                                                 used in trigger arrival time refinement */
                 int    a2;                   /* current sta work location - 2 (k2) 
                                                 used in trigger arrival time refinement */
                 int    a3;                   /* current sta work location - 1 (k3) 
                                                 used in trigger arrival time refinement 
                                                 so:
                                                 ... | a1 | a2 | a3 | <now> ||           */
                 int    r;                 
                 int    foundarrival = FALSE; /* state for arrival time refinement       */
                 int    refining     = FALSE; /* state for arrival time refinement       */
                 long   maxbackdown;          /* maximum backdown when looking for 
                                                 trigger arrival                         */
                 SERIES_DATA *STA_History;    /* pointer to staSeries->sta_buffer to 
                                                 reduce dereferencing                    */
                 double aa, bb, cc;
                 double zlt;
                 int    ismotm;               /* limit on how far to attempt arrival time 
                                                 refinement (points)                     */
                 int    b;                    /* loop counter                            */
                 long   iarr;                 /* arrival time index relative to current
				                                 sample index                            */
                 int    polcount;             /* max values to check for polarity        */

    /* Initialize the max value tracker */
    staSeries->max_value = 0.0;

    /* Only calculate the arrival time if one does not exist */
    if (staSeries->arrival == PKFL_NO_ARRIVAL)
    {
      /*
       * Currently no arrival time index has been determined,
       * so need to do so now.
       * 
       * If this channel is arriving from state PKCH_STATE_TRIG,
       * which had a drop in signal level to resume state
       * PKCH_STATE_PRELIM, the arrival time would have been
       * determined from the previous pass through here.
       * Returning here implies a signal drop-out that is
       * interpreted as a mere level fluctuation.
       * Therefore, will continue to use the previously
       * determined arrival time.
       * The arrival time will only be recalculated if the
       * signal state has returned to PKCH_STATE_LOOK
       * (which implies that the level has dropped
       * and has not resumed nor remained symmetric.)
       */
      
      c_sta_length = staSeries->sta_length;
      
      /*
       * NOTE: One of the assumptions about this code,
       *       and about the pick filter's sta buffer,
       *       is that the buffer will be completely filled
       *       by the time that execution arrives here.
       * 
       *       As a result, the value in the STA_History[] array at location
       *       staSeries->sta_write_point is the earliest available (since it is
       *       the next to be written) and the value at this _sta_end is the
       *       latest value available.
       * 
       *       Note also that the value at staSeries->sta_buffer[_sta_end] is
       *       for time staSeries->sta_time.
       */
      c_sta_end = staSeries->sta_write_point - 1;
   
      /* maximum backdown when looking for trigger arrival,
       * that is, the farthest distance to search for the wave start.
       * 
       * Don't backdown more than 1/2 of the maximum phase width */
      maxbackdown = (long)(rateConstants->maxph * 0.5);
                
      STA_History = staSeries->sta_buffer;
       
      /* Initialize the pick refinement locations
       * relative to the sta_write_point. */                  
      if ((a1 = c_sta_end - 3) < 0)
          a1 += c_sta_length;
          
      /*
       * It is expected that processing will arrive here when a possible trigger
       * is detected.  So, implicitly, the signal level will have increased past
       * some threshold.  And, following from that, the current processing time
       * will be someplace on the "leading edge" of an amplitude signal (can't
       * be on the "trailing edge" because the leading edge would have already
       * caused this code to be executed.
       * 
       * Therefore, the idea is to work backwards in time -- down that signal --
       * until a zero-crossing is found.  The time of that zero-crossing is
       * considered to be the pick time.
       * 
       * One might note that is is not the raw or pre-filtered data that is
       * used for this, but rather the STA series.  This is because some
       * earlier artifacts may be actually be part of the event being
       * processed, but they could be missed by just backing down the
       * other series.  Simply: not using the STA series would sometimes
       * cause the calculated pick time to be later than appropriate.
       */
            
      /* FORTRAN LINE 30 */
      while (staSeries->mid_trigger < STA_History[a1]) /* if(bt1(k1).le.xmt1) go to 31 */
      {
          /* move working index back one point */
          if ((--a1) < 0)
              a1 += c_sta_length;
                     
          if ((a1 == c_sta_end)) /* if(k1-ntb)30,32,30 */
          {
              /* Backed past the earliest sta value available
               * go to 32 */
              /* k1 = k1 % ntblm + 1;   LINE 32, then go to 41 */
                        
              /* adjust to the earliest time index */
              if ((++a1) == c_sta_length)
                  a1 = 0;
              foundarrival = TRUE;  /* go to 41 */
			  break;
          }
      }
               
               
      if (!foundarrival)
      {
          /*   Compute a new search threshold. */
               
          /* FORTRAN LINE 31 */
    
          cc = 0.5 * (staSeries->lta + staSeries->mid_trigger);
   
          if ((a2 = a1 + 1) == c_sta_length)
              a2 = 0;
   
          if ((a3 = a2 + 1) == c_sta_length)
              a3 = 0;
              
          zlt = MAX(MIN(0.5 * MAX(MAX(STA_History[a1], STA_History[a2]), STA_History[a3]), cc), 
                                      staSeries->lta);
               
          aa = FACTOR1 * zlt;
          bb = FACTOR2 * zlt;
   
          ismotm = (long)rateConstants->nsmotm;
               
          if (debug >= 2)
          {
              /* print *,'k1 nsmotm cc zlt aa bb',k1,nsmotm,cc,zlt,aa,bb */
              logit("" , "finding arr: a1: %d ismotm: %d cc: %lf zlt: %lf aa: %lf bb: %lf\n", 
                          a1, ismotm, cc, zlt, aa, bb);
          }
                     
               
          /* Back down on the new level; do 33 l=1,mxbck */               
          for (b = 0; b < maxbackdown; b++)
          {
              /* Leave the back down loop; go to 34 */
              if (STA_History[a1] <= cc)  
                  break;
                        
              if ((--a1) < 0)
                  a1 += c_sta_length;
                  
              /* Backed past the earliest sta value available; go to 32 */
              if (a1 == c_sta_end)
              {         
                  /* Adjust back to the earliest time index */
                  if ((++a1) == c_sta_length)
                      a1 = 0;
                        
                  foundarrival = TRUE;  /* go to 41 */
                  break;
              }
          } /* 33 continue */
                                  
          do
          {
              /* don't repeat loop unless specifically directed below */
              refining = FALSE;
                        
              /* Refine the pick by looking for a minimum decrease over a smoothing interval */
              if (!foundarrival)
              { 
                  if (debug >= 2)
                      logit( "", "refining arrival time\n" );

                  /* FORTRAN LINE 34; start refining from point a1 */
                  a3 = a1;
               
                  do
                  {                       
                      /* don't repeat loop unless specifically directed below */
                      refining = FALSE;

                      /* FORTRAN LINE 35: do 36 l=1,ismotm;
					   * first pass through loop, point a2 = point a1; a3 = a1 - 1
					   * each pass through loop, move a2 and a3 back one point, but
					   * don't move point a1 */
                      for (r = 0; r < ismotm; r++)  
                      {
                          a2 = a3;
                                 
                          if ((--a3) < 0)
                              a3 += c_sta_length;
                              
                          /* Backed past the earliest sta value available; go to 37 */
                          if (a3 == c_sta_end)
                          {
                              /* Arrived at the end of the buffer */
                              a1 = a3;                 /* FORTRAN LINE 37 */
                                    
                              /* adjust to the earliest time index */
                              if ((++a1) == c_sta_length)
                                  a1 = 0;

                              foundarrival = TRUE;    /* go to 41 */
                              break;
                          }
                           
                          if ((STA_History[a1] - STA_History[a3]) >= aa
                              && (STA_History[a2] - STA_History[a3]) >= bb)
                          {
                              /* go to 38; Go try more smoothing. */
                              a1 = a3;   /* FORTRAN LINE 38 */
                                     
                              if (debug >= 2)
                              {
                                  /* print *,'38',k1,bt1(k1) */
                                  logit("" , "prep more smooth (a1 <- a3 | %d <- %d) STA[%d] = %lf\n", 
                                               a1, a3, a1, STA_History[a1]); 
                              } 
                                    
                              /* go to 35. i.e.: reinitialize the refining loop */
                              refining = TRUE;
                              break;
                          }         
                      } /* 36   continue (back to line 35) */
                  } while (refining);
                                 
                  if (debug >= 2)
                      logit("", "finished refining\n");
              } /* !foundarrival */
                     
                                                     
              /* The above didn't work, try relaxing the smoothing criteria */   
              if (!foundarrival)
              {
                  aa *= 0.7;
                  ismotm /= 2;
                            
                  if (debug >= 2)
                       logit("", "relaxing smoothing criteria  aa: %f,  ismotm: %d\n", 
                                  aa, ismotm);
               
                  if (aa > (2.0 * bb))
                  {
                      /* go to 34, that is, jump up to repeat the previous refining block */
                      refining = TRUE;
                      continue;
                  }
               
				  /* set up for next refining loop */
				  foundarrival = TRUE; /* go to 41 */

                  /* The end game is to back down until there is no more decrease;
				   * Note that we may overshoot the arrival time */                    
                  for (r = 0;  r < ismotm; r++) /* do 39 l=1,ismotm */
                  {
                      a2 = a1;
                      if ((--a1) < 0)
                          a1 += c_sta_length;
   
                      /* Backed past the earliest sta value available; go to 32 */ 
                      if(a1 == c_sta_end)
                      {      
                          /* Adjust to the earliest time index */
                          if ((++a1) ==  c_sta_length)
                              a1 = 0;

                          foundarrival = TRUE; /* as above */
                          break;
                      }
                              
                      /* leave this loop and perform the next block  */
                      if ((STA_History[a2] - STA_History[a1]) <= 0.0) /* go to 40 */
                      {
                          foundarrival = FALSE;
                          break;
                      }
                  }
              } /* 39   continue */
                     
                     
                     
              /* Previous block didn't work; go forward until there is no more decrease,
			   * i.e, see if we overshot the arrival time */
              if (!foundarrival)
              {            
                   if (debug >= 2)
                     logit("", "forward; a1: STA[%d] = %lf, a2: STA[%d] = %lf\n", 
                                 a1, STA_History[a1], a2, STA_History[a2]); 

                  /* FORTRAN LINE 40 */
                  for (r = 0; r < ismotm; r++) /* 40   do 42 l=1,ismotm */
                  {
                      a1 = a2;
                      if ((++a2) == c_sta_length)
                          a2 = 0;
                              
                      /* Incremented to the last available sta value,
                       * can't go any further up the sta time series
                       *  go to 32 */
                      if (a2 == c_sta_end)
                      {
                          /* Adjust a1 to the appropriate location and leave the loop */
                          if ((++a1) ==  c_sta_length)
                              a1 = 0;

                          foundarrival = TRUE;
                          break; /* skip the next comparison, leave the loop */
                      }
                     
                      /* values at a1 and a2 are of opposite polarities,
                       * the trace has turned -- this is the lowest point */
                      if ((STA_History[a1] - STA_History[a2]) <= 0.0 )
                      {
                          foundarrival = TRUE;  /* go to 41 */
                          break;
                      }
                  
                      /* print *,'42',k2,bt1(k2) */
                      if (debug >= 2)
                          logit("", "(a2) STA[%d] = %lf\n", a2, STA_History[a2]);
                               
                  } /* 42   continue */
                        
                  if (!foundarrival)
                      a1 = a2;
              }
            
              /* print *,'39',k1,bt1(k1) */
              if (debug >= 2)
                  logit("", "(a1) STA[%d] = %lf, %s refining\n", 
                              a1, STA_History[a1], ( refining ? "still" : "finished"));

          } while (refining);
                     
      }  /* !foundarrival */
      
      /*
       * At this point, expect a1 to be the index into
       * the buffer, STA_History[], for the trigger arrival time.
       */
                   
      /* =====================================================================
       * Compute the absolute arrival sample
       * 
       * The entire sta_buffer contents are earlier or at the same time as
       * staSeries->sta_time.  However, the buffer can wrap, so different
       * handling is needed if the a1 index is higher than the index
       * of the last sta value received.
       */
      
	   
      /* FORTRAN LINE 41 */
      iarr = a1 - c_sta_end;
      
      if (c_sta_end < a1)
          iarr -= c_sta_length;

      staSeries->arrival = staSeries->sta_time
                           + rateConstants->sample_interval * ((double)iarr);  
           
      if (debug >= 2)
      {
           TimeToDTString(staSeries->arrival , dbg_arrstr);
           logit("o", "%s : %lf + %d * %lf = abs arrival time: %lf = %s\n", 
                            dbg_timestr, staSeries->sta_time, iarr, rateConstants->sample_interval, 
                            staSeries->arrival, dbg_arrstr);
      }
      
      /*
       * Determine the first motion polarity
       * 
       * a1 is index of arrival time into the Short-term array
       * a2 works its way forward in time from the next point,
       * looking for a different value.
       */
      staSeries->polarity = '?'; /* unknown */
                     
      a2 = a1;

      /* look forward at no more than 0.2 secs worth of data */
      polcount = MIN((int)ceil(0.2 * rateConstants->sample_rate), staSeries->sta_length);
     
	  for (b = 0; b < polcount; b++)
      {
          a2++;
          if (a2 == c_sta_end)
              a2 = 0;
                     
          if (STA_History[a1] < STA_History[a2]) 
          {
              staSeries->polarity = 'U'; /* first motion upwards */
              break;
          }
          else if (STA_History[a1] > STA_History[a2])
          {
              staSeries->polarity = 'D'; /* first motion downwards */
              break;
          }
          else
          {
              if (debug >= 2)
                  logit("", "still looking for polarity at time %lf STA[%d] = %lf, STA[%d] = %lf\n",
				        staSeries->arrival, a1, STA_History[a1], a2, STA_History[a2]);
          }
      }
      
    } /* no existing arrival time */
   
    return EW_SUCCESS;
}

/************************************************************************
 *                       GetConfirmDuration()                           *
 *                                                                      *
 * Calculates the number of sample value points for which the STA must  *
 * remain above mid_trigger for the trigger to be confirmed as a pick.  *
 *                                                                      *
 * The value is only calculated if the staValue is greater than         *
 * staSeries.max_value, and the returned value is constrained by the    *
 * bounds of staSeries.confirm_lo and staSeries.confirm_hi.             *
 *                                                                      *
 * @staValue - max value in the trigger                                 *                                                                     *
 * @return the confirmation duration                                    *
 *                                                                      *
 ************************************************************************/
int GetConfirmDuration(const double staValue, PICK_SERIES_DATA *staSeries)
{
    if (staSeries->confirm_lo == staSeries->confirm_hi)
    {
      /* No variance allowed in the duration*/
      staSeries->confirm_duration = staSeries->confirm_hi;
    }
    else
    {
      /* The required confirmation duration is determined by the
       * maximum filtered value encountered after the time that
       * the trigger starts.
       * That means that the the higher the signal encountered,
       * the longer the required duration.
       * And as the sta increases, so does the required duration. */
      staSeries->max_value = MAX(staSeries->max_value, staValue);

/* Dale's code         
      staSeries->confirm_duration = MIN(MAX((staValue * staSeries->confirm_factor 
                                                  * staSeries->confirm_hi), staSeries->confirm_lo), 
                                                  staSeries->confirm_hi);
*/
      staSeries->confirm_duration = MIN(MAX((staSeries->max_value * staSeries->confirm_factor 
                                              * staSeries->confirm_lo), staSeries->confirm_lo), 
                                                staSeries->confirm_hi);
    }

    return (int)staSeries->confirm_duration;
}

/************************************************************************
 *                  ClearPickFilterTrigger()                            *
 *                                                                      *
 * Returns the PICK_SERIES_DATA struct to a trigger searching state     *
 * after a trigger shutdown.                                            *
 *                                                                      *
 * @return EW_SUCCESS                                                  *
 *         EW_FAILURE - parameter passed in NULL                       *
 *                                                                      *
 ************************************************************************/
int ClearPickFilterTrigger(PICK_SERIES_DATA *staSeries)
{
    if (staSeries == NULL)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "staSeries passed in NULL to ClearPickFilterTrigger. \n");
      return EW_FAILURE;
    }
   
    staSeries->confirm_counter   = 0;
    staSeries->prelim_counter    = 0;
	staSeries->continue_counter  = 0;
    staSeries->max_life_counter  = 0;
    staSeries->state             = PKCH_STATE_LOOK;
    staSeries->arrival           = PKFL_NO_ARRIVAL;
   
    return EW_SUCCESS;
}
