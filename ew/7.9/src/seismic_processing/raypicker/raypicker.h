/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: raypicker.h 4453 2011-07-27 21:10:40Z stefan $
 *
 *    Revision history:
 *     $Log: raypicker.h,v $
 *     Revision 1.4  2010/08/31 18:36:02  davidk
 *     Added a standard earthwormy function (WriteError())to log an error to a ring for statmgr
 *     (standard *_status function found in most EW modules)
 *
 *     Revision 1.3  2010/08/30 16:21:35  davidk
 *     Added externs for variables defined in raypicker.c.
 *     Needed to support issuing a status message from rp_messaging.c,
 *     which is needed to send out an alarm when the picker station list is
 *     decimated by > 25% during an auto-update (i.e. alarming on possible problems
 *     with the station-list auto-update code).
 *
 *     Revision 1.2  2009/02/13 20:40:36  mark
 *     Added SCNL parsing library
 *
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:50  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.8  2005/02/03 21:05:41  davidk
 *     Updated to SCNL.
 *
 *     Revision 1.7  2004/11/01 02:03:27  cjbryan
 *     removed all amplitude determination code from the picker and changed all error
 *     codes to positive numbers
 *     CVS ----------------------------------------------------------------------
 *
 *     Revision 1.6  2004/07/16 19:27:22  cjbryan
 *     allowed continuation of a preliminary trigger
 *
 *     Revision 1.5  2004/07/13 19:23:02  cjbryan
 *     added RECURSIVE_FILTER struct and cleaned up filter code
 *
 *     Revision 1.4  2004/06/10 20:22:35  cjbryan
 *     re-engineered array initialization
 *
 *     Revision 1.3  2004/04/23 17:35:07  cjbryan
 *     changed bool to int
 *
 *     Revision 1.2  2004/04/21 20:31:10  cjbryan
 *     *** empty log message ***
 *
 *
 *
 */
/*
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 : August 2003, dbh
 */

#ifndef	RAYPICKER_H
#define RAYPICKER_H

/* earthworm/hydra includes */
#include <trace_buf.h>
#include <transport.h>
#include <ioc_filter.h>

/* raypicker includes */
#include "symmetry.h"
#include "pre_filter.h"
#include "pick_params.h"
#include "pick_series.h"
#include "ray_trigger.h"

#define SCNL_INCREMENT   100 /* how many more are allocated each time we run out */
#define PICK_FILELEN     50  /* max length of name of PickID file */

/* ================================================================================
 * ================================================================================
 * Structure to contain the channel data which must persist between
 * tracebuf arrivals.
 */
typedef struct _PICK_CHANNEL_INFO
{   
   
    short               state;         /* processing state for the channel,
                                        * see rp_channel_states.h for the 
                                        * acceptable values.*/
    double              sampleRate;    /* seconds */   
    double              prefiltmean;   /* a0 -> g1 */
                                       /* A floating value which simulates the 
                                        * mean of the pre-filtered data over time.
                                        * Used to determine S/N in the amps.*/ 
    SYMMETRY_CHECK_DATA symmetry_data; /* Presistent storage for symmetry tracking.
                                        * See symmetry.h for usage.*/
    RECURSIVE_FILTER    pre_filter;    /* Broadband or short-period filter used 
                                        * to render to pseudo short-period */
   
    /*
     * Picker aggregate states.
     * 
     * These are bitmap fields, with one bit per pick series.
     * (Bit location equivalent to its position in filter_data[].)
     * 
     * Each filter turns its bit on for true, off for false.
     * 
     * A filter starts in a search ("looking") state until the rectified
     * signal goes high, at which time the filter goes into the preliminary
     * state.  Once the signal is symmetric, the filter goes into the
     * possible trigger state (isTrigger).  Once the trigger persists
     * long enough the trigger is confirmed for the filter.  Finally,
     * once the signal level drops again, the trigger is finished.
     * So, a filter -- and correspondingly the channel -- tends to
     * move "up" the state heirarchy from looking to preliminary
     * trigger to possible trigger to confirmed trigger (a pick)
     * and the finished states.
     * 
     * Therefore, a channel's aggregate filter state can be determined
     * by the test: (variable == 0) means false
     *              (variable != 0) means true
     * 
     * Generally, when going to a new state, only the appropriate
     * new bit will be set; or if reverting to a lower state, only
     * the appropriate bit will be removed.
     * For example, when a trigger first confirmed only the bit in
     * isConfirmed which matches the filter that just confirmed is
     * set to 1.  And if reverting from a possible trigger to a
     * mere preliminary, the bit in isTrigger that matches the filter
     * that is reverting is set to 0.
     * 
     * Thus, bits in the lower states remain set, even as bits
     * are newly set in the higher states.
     * 
     * THERE IS ONE EXCEPTION: When the trigger from a filter
     * enters the finished state, the matching isConfirmed bit is
     * turned off.  This is the way that the complete trigger/pick
     * shutdown/termination is detected.
     * 
     * Therefore, the first filter entering the isConfirmed state
     * is the one that determines that a pick is truly identified
     * (regardless of any others triggered, which may or may not
     * be subsequently confirmed).
     * When both isFinished != 0 and isConfirmed == 0, it is known that
     * there had been a pick and the signal has dropped on all
     * filters (so the remaining amplitudes may be reported).
     */
    PICKER_AGGR_STATE   isPrelim;      /* Is it a preliminary trigger?                     */
    PICKER_AGGR_STATE   isContinuing;  /* Is preliminary trigger still allowed to resume?  */
    PICKER_AGGR_STATE   isTrigger;     /* Is it a possible trigger?                        */
    PICKER_AGGR_STATE   isConfirmed;   /* Is the trigger confirmed?                        */
    PICKER_AGGR_STATE   isFinished;    /* Is the trigger finished and ready for reporting? */
   
    PICK_SERIES_DATA    pick_series[PICKER_TYPE_COUNT]; /* Persistent data used by the 
                                                         * pick filtering for this channel.                   */
    unsigned long       processed_points;               /* Number of points processed for the 
                                                         * time series.Used to ensure that triggering 
                                                         * does not take place too early in the series.
                                                         * (Once the minimum time has passed, this is 
                                                         * no longer relevant.)                               */   
    SERIES_DATA        *rawBuffer;                      /* Raw sample data;  Although it would be 
                                                         * preferable to just use the global buffer,
                                                         * channel initialization requires about 18 seconds 
                                                         * of data  -- which far exceeds the tracebuf length 
                                                         * -- so, several tracebufs messages are needed 
                                                         * for channel initialization                         */
    long                rawAlloc;                       /* actual buffer length                               */
    long                rawLength;                      /* number or points in rawBuffer (first point 
                                                         * always at index 0)                                 */
    double              rawLastTime;                    /* Time of last sample value received.  
	                                                     * When processing for a tracebuf is
                                                         * completed, this ends up being the time of 
	                                                     * the last sample in the message.
                                                         * Thus, this is used to detect gaps.                 */
    double              rawStartTime;                   /* start time of raw data in buffer, from trace_buf   */
    long                pickId;                         /* from pick report, needed for [later] amp reporting */
    double              pickArrTime;                    /* from pick report, needed for [later] amp reporting */    
} PICK_CHANNEL_INFO;


typedef struct scnl_struct {
    char		sta[TRACE2_STA_LEN];   /* Site name                             */
    char		chan[TRACE2_CHAN_LEN]; /* Component/channel code                */
    char		net[TRACE2_NET_LEN];   /* Network name                          */
    char		loc[TRACE2_LOC_LEN];   /* Location code                         */
    PICK_CHANNEL_INFO  *pchanInfo;     /* pick channel information              */
	int         bDeleteMe;             /* non-zero if this SCNL needs to be deleted after auto-updating station lists */
} RaypickerSCNL;

typedef struct {
    unsigned char MyModId;             /* Module id of this program           */
    long   InKey;                      /* Key to ring where waveforms live    */
    long   OutKey;                     /* Key to ring where picks will live   */
    long   HeartBeatInterval;          /* Heartbeat interval in seconds       */
    int    LogFile;                    /* 0 = no log file; 
                                          1 = log to disk and stderr/stdout; 
                                          2 = log to disk                     */
    int    Debug;                      /* If 1, print debug messages          */
    char   PickIDFile[PICK_FILELEN];   /* Name of file containing pick number */
    double MaxSampleRate;              /* Maximum sampling rate               */
    double MaxTriggerSecs;             /* Maximum length of trigger           */
    double MaxGapNoTriggerSecs;        /* Max gap when no trigger             */
    double MaxGapInTriggerSecs;        /* Max gap when a trigger exists       */
    int    QueueSize;                  /* Max no of msgs to queue             */
    int    MaxPreFilters;              /* Max no of prefilters allowed        */
	char   SCNLFile[64];               /* Name of file containing SCNL list   */
	int    tUpdateInterval;            /* Time to wait before updating SCNL list */
} RParams;

typedef struct {
    unsigned char MyInstId;            /* Local installation                  */
    unsigned char GetThisInstId;       /* Get messages from this inst id      */
    unsigned char GetThisModId;        /* Get messages from this module       */
    unsigned char TypeGlobalAmp;       /* Global amplitude message type       */
    unsigned char TypeGlobalPick;      /* Global pick message type            */
    unsigned char TypeError;           /* Error message type                  */
    unsigned char TypeHeartBeat;       /* Heartbeat message type              */
    unsigned char TypeTracebuf2;        /* Waveform buffer for data input      */
    SHM_INFO      InRing;              /* Info structure for input region     */
    SHM_INFO      OutRing;             /* Info structure for output region    */
} EWParameters;


/* Function prototypes */
int GetEWParams(EWParameters *ewp );
thr_ret MessageStacker (void *dummy);
thr_ret	Processor (void *dummy);
RaypickerSCNL *find_SCNL(TRACE2_HEADER *TraceHeader);
int WriteError(EWParameters ewp, RParams rparams, pid_t MyPid, char * szError);  /* write a TypeError to a ring */

extern EWParameters    EwParameters;  /* structure containing ew-type parameters */
extern RParams         params;        /* stucture containing configuration parameters  */
extern pid_t           myPid;	       /* Hold our process ID to be sent with heartbeats */

#endif /* RAYPICKER_H  */
