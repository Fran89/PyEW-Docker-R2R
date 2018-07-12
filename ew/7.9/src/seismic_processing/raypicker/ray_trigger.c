/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ray_trigger.c 4453 2011-07-27 21:10:40Z stefan $
 *
 *    Revision history:
 *     $Log: ray_trigger.c,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:50  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.2  2005/02/03 21:05:11  davidk
 *     Updated to SCNL.
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 */
 
/*
 * Declarations of functions relating to buffer sizes.
 * 
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 : August 2003, dbh
 */
 
#include "ray_trigger.h"
#include <trace_buf.h>        /* MAX_TRACEBUF_SIZ */
#include <math.h>

/* minimum sample data length needed to start processing */
long GetTriggerLevelCalcLength(const double maxSampleRate)
{
    /*
     * This thing is a memory waster....
     * 
     * Per instructions, allocating the maximum array needed to hold
     * pre-filter averaging size for minimum 
     */
    
    /*
     * This allocates sufficient space to hold data for the entire pre-filter
     * averaging buffer, with as much wasted space needed to ensure that the
     * next highest number of tracebuf message data can fit.
     */
    /* min point size for buffer to contain the
     * longest span that will be collected prior
     * to processing (which happens to be the
     * number of points needed to determine the
     * trigger level).
     */
    return (long)floor(LTRLV * maxSampleRate + 0.5);
}

/* GetWorkBufferLength() plus one additional tracebuf message's data */
long GetWorkBufferAlloc(const double maxSampleRate)
{
    /* maximum number of points in a (packed) tracebuf message 
			= space without header / smallest data storage size */
    const long _max_pts_in_tb = (MAX_TRACEBUF_SIZ - sizeof(TRACE2_HEADER)) / 2;
    
    return GetTriggerLevelCalcLength(maxSampleRate) + _max_pts_in_tb;
}
