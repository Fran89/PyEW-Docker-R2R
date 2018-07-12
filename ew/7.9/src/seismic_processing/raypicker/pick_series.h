/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pick_series.h 4453 2011-07-27 21:10:40Z stefan $
 *
 *    Revision history:
 *     $Log: pick_series.h,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:49  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.4  2004/10/27 14:33:44  cjbryan
 *     revisions to period determination for amp calculations and collection of
 *     amp info between calculated arrival time and time trigger is declared
 *
 *     Revision 1.3  2004/07/20 20:10:04  cjbryan
 *     added counter for max lifetime of an arrival time
 *
 *     Revision 1.2  2004/07/16 19:27:22  cjbryan
 *     allowed continuation of a preliminary trigger
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 * sta_series.h
 *
 * Defines structure types to contain common picker filter coefficients,
 * persistence and results.
 * 
 * The short-term average (STA) is derived from an IIR filter.
 * 
 */
 
#ifndef PICK_SERIES_H
#define PICK_SERIES_H

#include "rate_constants.h"
#include "pick_params.h"

#define PKFL_NO_ARRIVAL  -1.0   /* arrival is set to this value to indicate that the 
                                 * arrival time must be recalculated. This avoids 
                                 * recalculating the time if the pick series state 
                                 * drops out of PKCH_STATE_TRIG, then returns to that 
                                 * state through a path that does not force a re-calculation */

/* ================================================================================
 * ================================================================================
 * Structure to contain the pick series data which must persist between tracebuf arrivals.
 */
typedef struct _PICK_SERIES_DATA
{
    double    highCorner;        /* high corner frequency */
    double    lowCorner;         /* low corner frequency  */

    /* IIR filter variables */
    double    fmem1;             /* filter memory 1 */
    double    fmem2;             /* filter memory 2 */
    double    fmem3;             /* filter memory 3 */
    double    fmem4;             /* filter memory 4 */
   
    double    last_value;        /* last [IIR] filtered value - In practice, this value is 
                                  * constrained by value_limit and value_abs_limit (Buland's btm)        */
    double    last_rect;         /* last rectified filter value =  fabs(filter.fmem2 - filter.fmem4) 
                                  * (Buland's s)                                                         */
    double    last_envelope;     /* last envelope value (Buland's sf)
                                  * The envelope simulates the boundary described by the peaks of
                                  * the STA.  It is used for trigger shutdowns.
                                  * In practice, the actual envelope is used from the 
                                  * pick_channel_info global buffer g_Envelope                           */
    double    trigger_value;     /* Base trigger value - used as a factor in calculating the mid-level 
                                  * (actual) triggering level and the max limit on the STA 
                                  * (Buland's trgvl)                                                     */
    double    lta;               /* Long-term average approximation. Used in calculating the mid_trigger 
                                  * value (Buland's xlt)                                                 */
    double    mid_trigger;       /* mid-level triggering value;  When the STA rises above this value, 
                                  * a preliminary trigger is identified; if it stays above this level 
                                  * for confirm_duration samples, then the  trigger is confirmed
                                  * (Buland's xmt)                                                       */
    double    shutdown_level;    /* Trigger shut-down value; When the envelope falls below this value, 
                                  * the trigger is shut down (Buland's xmf)                              */
    double    value_abs_limit;   /* Dynamic absolute maximum allowed in the filtered value stream,
                                  * i.e., an upper bound on the STA which is used to keep that series 
                                  * from diverging too far from the baseline
                                  * = 2.0 * trigger_value * lta (Buland's clt3)                          */
    double    value_limit;       /* Point-by-point calculated limit on the size of the filtered value.
                                  * It is recalculated as long as the data is not asymmetric.
                                  * In practice, it is also constrained by value_abs_limit (above)
                                  * (Buland's btv)                                                       */
    double    max_value;         /* Maximum STA value tracked during a trigger. If the STA exceeds 
                                  * this value, the confirmation duration is recalculated, the result 
                                  * being that the required duration for a trigger to be confirmed 
                                  * increases with increasing maximum STA value (Buland's btmx)          */
    double    confirm_lo;        /* The low boundary for confirmation duration (sample points)
                                  * (Buland's lcmftm)                                                    */
    double    confirm_hi;        /* The high boundary for confirmation duration (sample points) 
                                  * (Buland's hcmftm)                                                    */
    double    confirm_factor;    /* One factor in the confirmation duration calculation
                                  * cmt2 = 1.0 / xmt
                                  * cmt3 = BFAC * cmt2
                                  * BFAC / xmt   (BFAC is in ray_trigger.h)
                                  * (Buland's cmt3)                                                      */
    double    confirm_duration;  /* The calculated duration [sample points] that a
                                  * preliminary trigger must be sustained to be confirmed                */
    int       prelim_counter;    /* Preliminary trigger run counter.
                                  * Incremented for each asymmetric sample value during a
                                  * preliminary trigger. If is becomes too large without 
                                  * the signal returning to symmetric, the preliminary trigger 
                                  * is cancelled.(Buland's noff)                                         */
	int       continue_counter;  /* continuation counter for preliminary triggers                        */
    int       confirm_counter;   /* Trigger confirmation run counter. Incremented for each sample 
                                  * value once a trigger has been identified (is no longer preliminary).  
                                  * Upon reaching confirm_duration, the trigger is confirmed 
                                  * (and the pick message is sent). (Buland's non)                       */
    int       max_life_counter;  /* max time from first prelim trigger before resetting arrival time     */
    long      sta_length;        /* allocated size of sta_buffer, (formerly ntlen)                       */
    double   *sta_buffer;        /* short-term average buffer, data will wrap [formerly bt1(), bt2()];
                                  * maintains a history of the STA prior to the current processing time  */
    long      sta_write_point;   /* next write index into sta_buffer[]                                   */
    double    sta_time;          /* the time for the last sta entry (at sta_buffer[sta_write_point-1])   */
    double    arrival;           /* absolute arrival time (normalized to real time)                      */
    char      polarity;          /* first motion polarity, needed for pick report: 'U', 'D'              */
    char      state;             /* Processing state for this pick series                                */
   
} PICK_SERIES_DATA;


/* ================================================================================
 * ================================================================================
 * Funtions to manipulate pick series data
 */

int InitPickSeriesData( const double maxSampleRate, PICK_SERIES_DATA *staSeries);
void FreePickSeries(PICK_SERIES_DATA *staSeries);
int SetPickFilterParams(const double sampleRate, const int staSeriesId, 
                        PICK_SERIES_DATA *staSeries, 
                        const double PICKER_PARAM_DATA[][PICKER_VALUECOUNT]);
int InitPickSeriesLevels(const double lta, const double midlevel, 
                         const double triggervalue, PICK_SERIES_DATA *staSeries);
int InitPickFilterForSeries(PICK_SERIES_DATA *filter);

int BandpassSample(const double sampleValue, const double minSTA, 
                   const RATE_CONSTANTS_DATA *rate_constants, 
                   PICK_SERIES_DATA *staSeries, double *value);
int NewSTAValue(const double currentTime, const double newValue, 
                PICK_SERIES_DATA *staSeries);
int UpdateLevels(const double currentValue, const RATE_CONSTANTS_DATA *constants, 
                 PICK_SERIES_DATA *staSeries);
int NewTrigger(const RATE_CONSTANTS_DATA *rateConstants, 
               PICK_SERIES_DATA *staSeries, int debug);
int GetConfirmDuration(const double staValue, PICK_SERIES_DATA *staSeries);
int ClearPickFilterTrigger(PICK_SERIES_DATA *filter);

#endif  /* PICK_SERIES_H */
