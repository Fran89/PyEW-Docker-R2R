/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ray_trigger.h 4453 2011-07-27 21:10:40Z stefan $
 *
 *    Revision history:
 *     $Log: ray_trigger.h,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:50  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.3  2004/11/01 02:03:27  cjbryan
 *     removed all amplitude determination code from the picker and changed all error
 *     codes to positive numbers
 *     CVS ----------------------------------------------------------------------
 *
 *     Revision 1.2  2004/07/20 20:07:20  cjbryan
 *     moved mag degs from ray_trigger.h to amps.h
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 */
 
/*
 * Declarations of miscellaneous program-wide parameters
 * and functions relating to buffer sizes.
 * 
 * @author Ray Buland, original FORTRAN
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 : August 2003, dbh
 */
 
#ifndef RAY_TRIGGER_H
#define RAY_TRIGGER_H

typedef double SERIES_DATA;   /* data type for the time series */

#define STA_BUFF_SECS  10.0   /* Length (seconds) of pre-trigger STA buffer (tblen)
                               * (used for determining the pick arrival time) */
#define SNMIN           0.1   /* signal-to-noise minimum */

/* Tcon = trlv,cmftm,tmlnk,settm,smotm,cycln,phmin,plmin,phmax.*/
#define LTRLV          18.0   /* seconds of sample data from which to determine initial triggering level (trlv) */
#define SETTM          15.0   /* seconds of sample before trigger may be declared --> 'warmup' */
#define TMLNK           8.0   /* seconds that preliminary signal may continue without trigger identification */
#define SMOTM           0.8   /* max width of signal refinement loop (seconds) */
#define CYCLN           4.5   /* maximum seconds in a symmetry check span */
#define PHMAX           4.0   /* max seconds for phase width */

#define PREFILTER_INIT_AVR_LEN  12.0  /* seconds of data used to obtain initial average/mean 
                                       * for the iir pre-filter (aveiir) */
#define INIT_TAPER_LEN           5.0  /* length (seconds) of data used to perform 
                                       * initial tapering (rmpiir) */     
#define AVEST                    5.0  /* Channel initialization averaging start 
									   * (sample value index; related to INIT_TAPER_LEN) */
#define AVETM                   11.0  /* Channel initialization averaging end (sample value 
									   * index; related to INIT_TAPER_LEN) */

/* function prototypes */
long GetTriggerLevelCalcLength(const double maxSampleRate);
long GetWorkBufferAlloc(const double maxSampleRate);

#endif  /* RAY_TRIGGER_H */
